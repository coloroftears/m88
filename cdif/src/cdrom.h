// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	$Id: cdrom.h,v 1.1 1999/08/26 08:04:37 cisc Exp $

#ifndef incl_cdrom_h
#define incl_cdrom_h

#include "types.h"
#include "misc.h"

class ASPI;

// ---------------------------------------------------------------------------
//	CDROM ����N���X
//
class CDROM
{
public:
	struct Track
	{
		uint32 addr;
		uint control;		// b2 = data/~audio
	};
	struct MSF
	{
		uint8 min;
		uint8 sec;
		uint8 frame;
	};

public:
	CDROM();
	~CDROM();

	int ReadTOC();
	int GetNumTracks();
	bool Init();
	bool PlayTrack(int tr, bool one = false);
	bool PlayAudio(uint begin, uint stop);
	bool ReadSubCh(uint8* dest, bool msf);
	bool Pause(bool pause);
	bool Stop();
	bool Read(uint sector, uint8* dest, int length);
	bool Read2(uint sector, uint8* dest, int length);
	bool ReadCDDA(uint sector, uint8* dest, int length);
	const Track* GetTrackInfo(int t);
	bool CheckMedia();
	MSF ToMSF(uint lba);
	uint ToLBA(MSF msf);

private:
	bool FindDrive();
	
	ASPI* aspi;
	int ha;			// CD-ROM �h���C�u�̐ڑ�����Ă���z�X�g�A�_�v�^
	int id;			// CD-ROM �h���C�u�� ID
	int ntracks;	// �g���b�N��
	int trstart;	// �g���b�N�̊J�n�ʒu

	Track track[100];
};

// ---------------------------------------------------------------------------
//	LBA ���Ԃ� MSF ���Ԃɕϊ�
//
inline CDROM::MSF CDROM::ToMSF(uint lba)
{
	lba += trstart;
	MSF msf;
	msf.min = NtoBCD(lba / 75 / 60);
	msf.sec = NtoBCD(lba / 75 % 60);
	msf.frame = NtoBCD(lba % 75);
	return msf;
}

// ---------------------------------------------------------------------------
//	LBA ���Ԃ� MSF ���Ԃɕϊ�
//
inline uint CDROM::ToLBA(MSF msf)
{
	return (BCDtoN(msf.min) * 60 + BCDtoN(msf.sec)) * 75 + BCDtoN(msf.frame)
		   - trstart;
}

// ---------------------------------------------------------------------------
//	CD �̃g���b�N�����擾
//	ReadTOC ��ɗL��
//
inline const CDROM::Track* CDROM::GetTrackInfo(int t)
{
	if (t < 0 || t > ntracks+1)
		return 0;
	return &track[t ? t - 1 : ntracks];
}

// ---------------------------------------------------------------------------
//	CD ���̃g���b�N�����擾
//	ReadTOC ��ɗL��
//
inline int CDROM::GetNumTracks()
{
	return ntracks;
}

#endif // incl_cdrom_h
