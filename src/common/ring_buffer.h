#pragma once

#include <algorithm>
#include <memory>

#include <pthread.h>
#include <string.h>

namespace common {

template <typename T>
class RingBuffer {
 public:
  explicit RingBuffer(unsigned int size)
      : size_(size),
        buffer_(new T[size_]),
        head_(nullptr),
        tail_(nullptr),
        buffered_(0) {
    memset(buffer_.get(), 0, size_ * sizeof(T));
    head_ = buffer_.get();

    // TODO: remove this half-filling logic.
    buffered_ = size_ / 2;
    tail_ = head_ + buffered_;
  }
  ~RingBuffer() {}

  // Wrapper to handle mutex locking on queue.
  // Returns samples written.
  uint32_t Queue(T* block, uint32_t samples);
  // Wrapper to handle mutex locking on queue.
  uint32_t Dequeue(T* block, uint32_t samples);

  // Adds a block of audio samples to the end of the queue. This function is
  // 'unsafe' and is thus wrapped by a function that handles the queue mutex
  // lock.
  uint32_t UnsafeQueue(T* block, uint32_t samples);

  // Dequeue a certain number of audio bytes from the fifo. For blocks
  // smaller than the remaining number we simply copy them and reduce
  // the count. For blocks that go over this limit we have to make a
  // recursive call on the function to fill out the rest. When there
  // are no blocks left the buffer is zero padded. As with the queueing
  // function this is unsafe and wrapped by a mutex handling funcion.
  uint32_t UnsafeDequeue(T* block, uint32_t samples);

  uint32_t free() const { return size_ - buffered_; }
  uint32_t buffered() const { return buffered_; }

 private:
  // Size (in T)
  uint32_t size_;
  // Buffer start pointer.
  std::unique_ptr<T[]> buffer_;
  // Current buffer head.
  T* head_;
  // Current buffer tail.
  T* tail_;
  // Buffered size in samples.
  unsigned int buffered_;
  // Queue locking mutex.
  pthread_mutex_t mutex_;
};

template <typename T>
uint32_t RingBuffer<T>::UnsafeQueue(T* src, uint32_t samples) {
  uint32_t freesize = size_ - buffered_;
  // Buffer end pointer + 1.
  const T* buffer_end = buffer_.get() + size_;

  if (samples > freesize) {
    // log(kLogWarning, "buffer overflow: discarding %lu samples\n",
    //    (samples - freesize) / /*channels*/2);
    samples = freesize;
  }

  uint32_t copy_len =
      (tail_ >= head_)
          ? std::min(samples, static_cast<uint32_t>(buffer_end - tail_))
          : std::min(samples, static_cast<uint32_t>(head_ - tail_));

  memcpy(tail_, src, copy_len * sizeof(T));
  tail_ += copy_len;
  if (tail_ >= buffer_end)
    tail_ -= size_;
  buffered_ += copy_len;
  samples -= copy_len;

  uint32_t written = copy_len;
  if (samples)
    written += UnsafeQueue(src + copy_len, samples);
  return written;
}

template <typename T>
uint32_t RingBuffer<T>::Queue(T* src, uint32_t samples) {
  pthread_mutex_lock(&mutex_);
  uint32_t written = UnsafeQueue(src, samples);
  pthread_mutex_unlock(&mutex_);
  return written;
}

// Dequeue a certain number of audio bytes from the fifo. For blocks
// smaller than the remaining number we simply copy them and reduce
// the count. For blocks that go over this limit we have to make a
// recursive call on the function to fill out the rest. When there
// are no blocks left the buffer is zero padded. As with the queueing function
// this is unsafe and wrapped by a mutex handling funcion.
template <typename T>
uint32_t RingBuffer<T>::UnsafeDequeue(T* dest, uint32_t samples) {
  // Buffer end pointer + 1.
  const T* buffer_end = buffer_.get() + size_;

  if (samples > buffered_) {
    fprintf(stderr, "buffer underrun: %d samples\n",
            (samples - buffered_) / /* channels */ 2);
    memset(dest + buffered_, 0, (samples - buffered_) * sizeof(T));
    samples = buffered_;
  }

  if (samples == 0)
    return 0;

  uint32_t copy_len =
      (tail_ > head_)
          ? std::min(samples, static_cast<uint32_t>(tail_ - head_))
          : std::min(samples, static_cast<uint32_t>(buffer_end - head_));

  memcpy(dest, head_, copy_len * sizeof(T));
  head_ += copy_len;
  if (head_ >= buffer_end)
    head_ -= size_;
  buffered_ -= copy_len;
  samples -= copy_len;

  // recurse at most once.
  if (samples)
    copy_len += UnsafeDequeue(dest + copy_len, samples);

  return copy_len;
}

// Wrapper to handle mutex locking on queue
template <typename T>
uint32_t RingBuffer<T>::Dequeue(T* block, uint32_t samples) {
  pthread_mutex_lock(&mutex_);
  uint32_t copied_length = UnsafeDequeue(block, samples);
  pthread_mutex_unlock(&mutex_);
  return copied_length;
}

}  // namespace common
