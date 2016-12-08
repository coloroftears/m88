//  $Id: srcbuf.cpp,v 1.2 2003/05/12 22:26:34 cisc Exp $

#include "common/sampling_rate_converter.h"

#include <assert.h>
#include <math.h>
#include <string.h>

#include <algorithm>

#include "common/clamp.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

template <class T>
inline T gcd(T x, T y) {
  T t;
  while (y) {
    t = x % y;
    x = y;
    y = t;
  }
  return x;
}

template <class T>
T bessel0(T x) {
  T p, r, s;

  r = 1.0;
  s = 1.0;
  p = (x / 2.0) / s;

  while (p > 1.0E-10) {
    r += p * p;
    s += 1.0;
    p *= (x / 2.0) / s;
  }
  return r;
}

// ---------------------------------------------------------------------------
//  Sound Buffer
//
bool SamplingRateConverter::Init(SoundSource<Sample32>* _source,
                                 int _buffersize,
                                 uint32_t outrate) {
  CriticalSection::Lock lock(cs);

  buffer.reset();

  source = nullptr;
  if (!_source)
    return true;

  buffersize = _buffersize;
  assert(buffersize > (2 * M + 1));

  ch = _source->GetChannels();
  read = 0;
  write = 0;

  if (!ch || buffersize <= 0)
    return false;

  buffer.reset(new Sample32[ch * buffersize]);
  if (!buffer)
    return false;

  memset(buffer.get(), 0, ch * buffersize * sizeof(Sample32));
  source = _source;

  outputrate = outrate;

  MakeFilter(outrate);
  read = 2 * M + 1;  // zero fill
  return true;
}

// ---------------------------------------------------------------------------
//  バッファに音を追加
//
int SamplingRateConverter::Fill(int samples) {
  CriticalSection::Lock lock(cs);
  if (source)
    return FillInternal(samples);
  return 0;
}

int SamplingRateConverter::FillInternal(int samples) {
  // リングバッファの空きを計算
  int free = buffersize - Avail();

  if (!fillwhenempty && (samples > free - 1)) {
    int skip = std::min(samples - free + 1, buffersize - free);
    free += skip;
    read += skip;
    if (read > buffersize)
      read -= buffersize;
  }

  // 書きこむべきデータ量を計算
  samples = std::min(samples, free - 1);
  if (samples > 0) {
    // 書きこむ
    if (buffersize - write >= samples) {
      // 一度で書ける場合
      source->Get(buffer.get() + write * ch, samples);
    } else {
      // ２度に分けて書く場合
      source->Get(buffer.get() + write * ch, buffersize - write);
      source->Get(buffer.get(), samples - (buffersize - write));
    }
    write += samples;
    if (write >= buffersize)
      write -= buffersize;
  }
  return samples;
}

// ---------------------------------------------------------------------------
//  フィルタを構築
//
void SamplingRateConverter::MakeFilter(uint32_t out) {
  uint32_t in = source->GetRate();

  // 変換前、変換後レートの比を求める
  // ソースを ic 倍アップサンプリングして LPF を掛けた後
  // oc 分の 1 にダウンサンプリングする

  // FM 音源対策(w
  if (in == 55467) {
    in = 166400;
    out *= 3;
  }
  int32_t g = gcd(in, out);
  ic = out / g;
  oc = in / g;

  // あまり次元を高くしすぎると、係数テーブルが巨大になってしまうのでてけとうに精度を落とす
  while (ic > osmax && oc >= osmin) {
    ic = (ic + 1) / 2;
    oc = (oc + 1) / 2;
  }

  double r = ic * in;  // r = lpf かける時のレート

  // カットオフ 周波数
  double c = .95 * M_PI / std::max(ic, oc);  // c = カットオフ
  double fc = c * r / (2 * M_PI);

  // フィルタを作ってみる
  // FIR LPF (窓関数はカイザー窓)
  n = (M + 1) * ic;  // n = フィルタの次数

  h2.reset(new float[(ic + 1) * (M + 1)]);

  double gain = 2 * ic * fc / r;
  double a = 10.;  // a = 阻止域での減衰量を決める
  double d = bessel0(a);

  int j = 0;
  for (int i = 0; i <= ic; ++i) {
    int ii = i;
    for (int o = 0; o <= M; ++o) {
      if (ii > 0) {
        double x = (double)ii / (double)(n);
        double x2 = x * x;
        double w = bessel0(sqrt(1.0 - x2) * a) / d;
        double g = c * (double)ii;
        double z = sin(g) / g * w;
        h2[j] = gain * z;
      } else {
        h2[j] = gain;
      }
      ++j;
      ii += ic;
    }
  }
  oo = 0;
}

// ---------------------------------------------------------------------------
//  バッファから音を貰う
//
int SamplingRateConverter::Get(Sample16* dest, int samples) {
  CriticalSection::Lock lock(cs);
  if (!buffer)
    return 0;

  int ss = samples;
  for (int count = samples; count > 0; --count) {
    int p = read;

    int i;
    float* h;

    float z0 = 0.f, z1 = 0.f;

    h = &h2[(ic - oo) * (M + 1) + (M)];
    for (i = -M; i <= 0; ++i) {
      z0 += *h * buffer[p * 2];
      z1 += *h * buffer[p * 2 + 1];
      --h;
      ++p;
      if (p == buffersize)
        p = 0;
    }

    h = &h2[oo * (M + 1)];
    for (; i <= M; ++i) {
      z0 += *h * buffer[p * 2];
      z1 += *h * buffer[p * 2 + 1];
      ++h;
      ++p;
      if (p == buffersize)
        p = 0;
    }
    *dest++ = Limit16(z0);
    *dest++ = Limit16(z1);

    oo -= oc;
    while (oo < 0) {
      ++read;
      if (read == buffersize)
        read = 0;
      if (Avail() < 2 * M + 1)
        FillInternal(std::max(ss, count));
      ss = 0;
      oo += ic;
    }
  }
  return samples;
}
