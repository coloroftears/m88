//	$Id: sndbuf2.h,v 1.2 2003/05/12 22:26:34 cisc Exp $

#ifndef common_soundbuf2_h
#define common_soundbuf2_h

#include "critsect.h"
#include "if/ifcommon.h"
#include "soundsrc.h"

// ---------------------------------------------------------------------------
//	SoundBuffer2
//
class SoundBuffer2 : public SoundSource
{
public:
	SoundBuffer2();
	~SoundBuffer2();

	bool	Init(SoundSource* source, int bufsize);	// bufsize �̓T���v���P��
	void	Cleanup();

	int		Get(Sample* dest, int size);
	ulong	GetRate();
	int		GetChannels();

	int		Fill(int samples);			// �o�b�t�@�ɍő� sample ���f�[�^��ǉ�
	bool	IsEmpty();
	void	FillWhenEmpty(bool f);		// �o�b�t�@����ɂȂ������[���邩

	int		GetAvail();

private:
	int		FillMain(int samples);

	CriticalSection cs;
	
	SoundSource* source;
	Sample* buffer;
	int buffersize;						// �o�b�t�@�̃T�C�Y (in samples)
	int read;							// �Ǎ��ʒu (in samples)
	int write;							// �������݈ʒu (in samples)
	int ch;								// �`���l����(1sample = ch*Sample)
	bool fillwhenempty;
};

// ---------------------------------------------------------------------------

inline void SoundBuffer2::FillWhenEmpty(bool f)
{
	fillwhenempty = f;
}

inline ulong SoundBuffer2::GetRate()
{
	return source ? source->GetRate() : 0;
}

inline int SoundBuffer2::GetChannels()
{
	return source ? ch : 0;
}

inline int SoundBuffer2::GetAvail()
{
	int avail;
	if (write >= read)
		avail = write - read;
	else
		avail = buffersize + write - read;
	return avail;
}

#endif // common_soundbuf_h
