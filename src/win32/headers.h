//
//  Windows 系 includes
//  すべての標準ヘッダーを含む
//
//  $Id: headers.h,v 1.13 2003/05/12 22:26:35 cisc Exp $
//

#pragma once

#define STRICT
#define WIN32_LEAN_AND_MEAN

#define DIRECTSOUND_VERSION 0x500  // for pre-DirectX7 environment

#define FORW2K  // W2K 用の SDK を使用

#ifdef FORW2K
#define WINVER 0x500  // for Win2000
#define _WIN32_WINNT 0x500
#endif

#pragma warning(disable : 4786)

#include <windows.h>
#include <commdlg.h>
#include <mmsystem.h>
#include <ddraw.h>
#include <dsound.h>
#include <commctrl.h>

// --- STL 関係

#ifdef _MSC_VER
#undef max
#define max _MAX
#undef min
#define min _MIN
#endif