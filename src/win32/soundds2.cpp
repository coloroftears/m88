// ---------------------------------------------------------------------------
//	M88 - PC-88 Emulator
//	Copyright (C) cisc 1999, 2000.
// ---------------------------------------------------------------------------
//	DirectSound based driver - another version
// ---------------------------------------------------------------------------
//	$Id: soundds2.cpp,v 1.5 2002/05/31 09:45:21 cisc Exp $

#include "headers.h"
#include "soundds2.h"

#define LOGNAME "soundds2"
#include "diag.h"

using namespace WinSoundDriver;

// ---------------------------------------------------------------------------
//	�\�z�E�j�� ---------------------------------------------------------------

DriverDS2::DriverDS2()
{
	playing = false;
	mixalways = false;
	lpds = 0;
	lpdsb = 0;
	lpdsb_primary = 0;
	lpnotify = 0;

	hthread = 0;
	hevent = 0;
}

DriverDS2::~DriverDS2()
{
	Cleanup();
}

// ---------------------------------------------------------------------------
//  ������ -------------------------------------------------------------------

bool DriverDS2::Init(SoundSource* s, HWND hwnd, uint rate, uint ch, uint buflen)
{
	int i;
	HRESULT hr;

	if (playing)
		return false;

	src = s;
	buffer_length = buflen;
	sampleshift = 1 + (ch == 2 ? 1 : 0);

	// �v�Z
	buffersize = (rate * ch * sizeof(Sample) * buffer_length / 1000) & ~7;

	// DirectSound object �쐬
	if (FAILED(CoCreateInstance(CLSID_DirectSound, 0, CLSCTX_ALL, IID_IDirectSound, (void**) &lpds)))
		return false;
	if (FAILED(lpds->Initialize(0)))
		return false;
//	if (FAILED(DirectSoundCreate(0, &lpds, 0)))
//		return false;

	// �������x���ݒ�
	hr = lpds->SetCooperativeLevel(hwnd, DSSCL_PRIORITY);
	if (hr != DS_OK)
	{
		hr = lpds->SetCooperativeLevel(hwnd, DSSCL_NORMAL);
		if (hr != DS_OK) 
			return false;
	}
	
	DSBUFFERDESC dsbd;
    memset(&dsbd, 0, sizeof(DSBUFFERDESC));
    dsbd.dwSize = sizeof(DSBUFFERDESC);
    dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
    dsbd.dwBufferBytes = 0; 
    dsbd.lpwfxFormat = 0;
	hr = lpds->CreateSoundBuffer(&dsbd, &lpdsb_primary, 0);
	if (hr != DS_OK)
		return false;

	// Primary buffer �̍Đ��t�H�[�}�b�g�ݒ�
	WAVEFORMATEX wf;
    memset(&wf, 0, sizeof(WAVEFORMATEX));
    wf.wFormatTag = WAVE_FORMAT_PCM;
	wf.nChannels = ch;
    wf.nSamplesPerSec = rate;
	wf.wBitsPerSample = 16;
	wf.nBlockAlign = wf.nChannels * wf.wBitsPerSample / 8;
	wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;

	lpdsb_primary->SetFormat(&wf);

	// �Z�J���_���o�b�t�@�쐬
    memset(&dsbd, 0, sizeof(DSBUFFERDESC));
    dsbd.dwSize = sizeof(DSBUFFERDESC);
    dsbd.dwFlags = DSBCAPS_STICKYFOCUS 
    			 | DSBCAPS_GETCURRENTPOSITION2
				 | DSBCAPS_CTRLPOSITIONNOTIFY;

    dsbd.dwBufferBytes = buffersize; 
    dsbd.lpwfxFormat = &wf;
    
	hr = lpds->CreateSoundBuffer(&dsbd, &lpdsb, NULL); 
	if (hr != DS_OK)
		return false;

	// �ʒm�I�u�W�F�N�g�쐬

	hr = lpdsb->QueryInterface(IID_IDirectSoundNotify, (LPVOID*) &lpnotify);
	if (hr != DS_OK)
		return false;

	if (!hevent)
		hevent = CreateEvent(0, false, false, 0);
	if (!hevent)
		return false;
	
	DSBPOSITIONNOTIFY pn[nblocks];
	
	for (i=0; i<nblocks; i++)
	{
		pn[i].dwOffset = buffersize * i / nblocks;
		pn[i].hEventNotify = hevent;
	}

	hr = lpnotify->SetNotificationPositions(nblocks, pn);
	if (hr != DS_OK)
		return false;

	playing = true;
	nextwrite = 1 << sampleshift;
	
	// �X���b�h�N��
	if (!hthread)
	{
		hthread = HANDLE(_beginthreadex(NULL, 0, ThreadEntry,
							reinterpret_cast<void*>(this), 0, &idthread));
		if (!hthread)
			return false;
		SetThreadPriority(hthread, THREAD_PRIORITY_ABOVE_NORMAL);
	}
	
	// �Đ�
	lpdsb->Play(0, 0, DSBPLAY_LOOPING);

	return true;
}

// ---------------------------------------------------------------------------
//  ��Еt�� -----------------------------------------------------------------

bool DriverDS2::Cleanup()
{
	playing = false;
	
	if (hthread)
	{
		SetEvent(hevent);
		if (WAIT_TIMEOUT == WaitForSingleObject(hthread, 3000))
			TerminateThread(hthread, 0);
		CloseHandle(hthread);
		hthread = 0;
	}
	
	if (lpdsb)
		lpdsb->Stop();
	if (lpnotify)
		lpnotify->Release(), lpnotify = 0;
	if (lpdsb)
		lpdsb->Release(), lpdsb = 0;
	if (lpdsb_primary)
		lpdsb_primary->Release(), lpdsb_primary = 0;
	if (lpds)
		lpds->Release(), lpds = 0;

	if (hevent)
		CloseHandle(hevent), hevent = 0;
	return true;
}

// ---------------------------------------------------------------------------
//  Thread -------------------------------------------------------------------

uint __stdcall DriverDS2::ThreadEntry(LPVOID arg)
{
	DriverDS2* dd = reinterpret_cast<DriverDS2*>(arg);

	while (dd->playing)
	{
		static int p;
		int t = GetTickCount();
		LOG1("%d ", t-p); p=t;

		dd->Send();
		WaitForSingleObject(dd->hevent, INFINITE);
	}
	return 0;
}

// ---------------------------------------------------------------------------
//  �u���b�N���� -------------------------------------------------------------

void DriverDS2::Send()
{
	bool restored = false;
	if (!playing)
		return;
	
	// Buffer Lost ?
	DWORD status;
	lpdsb->GetStatus(&status);
	if (DSBSTATUS_BUFFERLOST & status)
	{
		if (DS_OK != lpdsb->Restore())
			return;
		restored = true;
	}

	// �ʒu�擾
	DWORD cplay, cwrite;
	lpdsb->GetCurrentPosition(&cplay, &cwrite);

	if (cplay == nextwrite && !restored)
		return;

	// �������݃T�C�Y�v�Z
	int writelength;
	if (cplay < nextwrite)
		writelength = cplay + buffersize - nextwrite;
	else
		writelength = cplay - nextwrite;

	writelength &= -1 << sampleshift;

	if (writelength <= 0)
		return;
		
	void* a1, * a2;
	DWORD al1, al2;
	
	// Lock
	if (DS_OK == lpdsb->Lock(nextwrite, writelength,
							 (void**) &a1, &al1, (void**) &a2, &al2, 0))
	{
		// ��������
//		if (mixalways || !src->IsEmpty())
		{
			if (a1)
				src->Get((Sample*) a1, al1 >> sampleshift);
			if (a2)
				src->Get((Sample*) a2, al2 >> sampleshift);
		}
		
		// Unlock
		lpdsb->Unlock(a1, al1, a2, al2);

		nextwrite += writelength;
		if (nextwrite >= buffersize)
			nextwrite -= buffersize;
		
		if (restored)
			lpdsb->Play(0, 0, DSBPLAY_LOOPING);
	}
}
