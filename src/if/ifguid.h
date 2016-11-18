//
//  M88 interface GUID
//  $Id: ifguid.h,v 1.4 2002/04/07 05:40:09 cisc Exp $
//

#ifndef incl_ifguid_h
#define incl_ifguid_h

// ----------------------------------------------------------------------------
//  M88 interface GUID
//
// Main 側 I/O Bus                  {328ABDEA-7E10-11d3-BD4F-00A0C90AC9DA}
DEFINE_GUID(M88IID_IOBus1,
            0x328abdea,
            0x7e10,
            0x11d3,
            0xbd,
            0x4f,
            0x0,
            0xa0,
            0xc9,
            0xa,
            0xc9,
            0xda);
// Sub 側 I/O Bus                   {328ABDEB-7E10-11d3-BD4F-00A0C90AC9DA}
DEFINE_GUID(M88IID_IOBus2,
            0x328abdeb,
            0x7e10,
            0x11d3,
            0xbd,
            0x4f,
            0x0,
            0xa0,
            0xc9,
            0xa,
            0xc9,
            0xda);

// Main 側 I/O Bus                  {328ABDEC-7E10-11d3-BD4F-00A0C90AC9DA}
DEFINE_GUID(M88IID_IOAccess1,
            0x328abdec,
            0x7e10,
            0x11d3,
            0xbd,
            0x4f,
            0x0,
            0xa0,
            0xc9,
            0xa,
            0xc9,
            0xda);
// Sub 側 I/O Bus                   {328ABDED-7E10-11d3-BD4F-00A0C90AC9DA}
DEFINE_GUID(M88IID_IOAccess2,
            0x328abded,
            0x7e10,
            0x11d3,
            0xbd,
            0x4f,
            0x0,
            0xa0,
            0xc9,
            0xa,
            0xc9,
            0xda);

// Main 側 メモリ管理               {328ABDEE-7E10-11d3-BD4F-00A0C90AC9DA}
DEFINE_GUID(M88IID_MemoryManager1,
            0x328abdee,
            0x7e10,
            0x11d3,
            0xbd,
            0x4f,
            0x0,
            0xa0,
            0xc9,
            0xa,
            0xc9,
            0xda);
// Sub 側 メモリ管理                {328ABDEF-7E10-11d3-BD4F-00A0C90AC9DA}
DEFINE_GUID(M88IID_MemoryManager2,
            0x328abdef,
            0x7e10,
            0x11d3,
            0xbd,
            0x4f,
            0x0,
            0xa0,
            0xc9,
            0xa,
            0xc9,
            0xda);

// Main 側 メモリアクセス           {328ABDF0-7E10-11d3-BD4F-00A0C90AC9DA}
DEFINE_GUID(M88IID_MemoryAccess1,
            0x328abdf0,
            0x7e10,
            0x11d3,
            0xbd,
            0x4f,
            0x0,
            0xa0,
            0xc9,
            0xa,
            0xc9,
            0xda);
// Sub 側 メモリアクセス            {328ABDF1-7E10-11d3-BD4F-00A0C90AC9DA}
DEFINE_GUID(M88IID_MemoryAccess2,
            0x328abdf1,
            0x7e10,
            0x11d3,
            0xbd,
            0x4f,
            0x0,
            0xa0,
            0xc9,
            0xa,
            0xc9,
            0xda);

// 音管理                           {328ABDF2-7E10-11d3-BD4F-00A0C90AC9DA}
DEFINE_GUID(M88IID_SoundControl,
            0x328abdf2,
            0x7e10,
            0x11d3,
            0xbd,
            0x4f,
            0x0,
            0xa0,
            0xc9,
            0xa,
            0xc9,
            0xda);

// タイマー管理                     {328ABDF3-7E10-11d3-BD4F-00A0C90AC9DA}
DEFINE_GUID(M88IID_Scheduler,
            0x328abdf3,
            0x7e10,
            0x11d3,
            0xbd,
            0x4f,
            0x0,
            0xa0,
            0xc9,
            0xa,
            0xc9,
            0xda);
// システム時間取得                 {328ABDF4-7E10-11d3-BD4F-00A0C90AC9DA}
DEFINE_GUID(M88IID_Time,
            0x328abdf4,
            0x7e10,
            0x11d3,
            0xbd,
            0x4f,
            0x0,
            0xa0,
            0xc9,
            0xa,
            0xc9,
            0xda);
// CPU 時間取得                     {44F8E15C-7E9F-11d3-BD51-00A0C90AC9DA}
DEFINE_GUID(M88IID_CPUTime,
            0x44f8e15c,
            0x7e9f,
            0x11d3,
            0xbd,
            0x51,
            0x0,
            0xa0,
            0xc9,
            0xa,
            0xc9,
            0xda);

// DMA                              {328ABDF5-7E10-11d3-BD4F-00A0C90AC9DA}
DEFINE_GUID(M88IID_DMA,
            0x328abdf5,
            0x7e10,
            0x11d3,
            0xbd,
            0x4f,
            0x0,
            0xa0,
            0xc9,
            0xa,
            0xc9,
            0xda);

// 設定ダイアログ                   {32694F5A-7F25-11d3-BD53-00A0C90AC9DA}
DEFINE_GUID(M88IID_ConfigPropBase,
            0x32694f5a,
            0x7f25,
            0x11d3,
            0xbd,
            0x53,
            0x0,
            0xa0,
            0xc9,
            0xa,
            0xc9,
            0xda);

// UI                               {A63E4B05-8F80-11d3-BD81-00A0C90AC9DA}
DEFINE_GUID(M88IID_WinUI2,
            0xa63e4b05,
            0x8f80,
            0x11d3,
            0xbd,
            0x81,
            0x0,
            0xa0,
            0xc9,
            0xa,
            0xc9,
            0xda);

// Core Lock                        {EDE65456-8F85-11d3-BD81-00A0C90AC9DA}
DEFINE_GUID(M88IID_LockCore,
            0xede65456,
            0x8f85,
            0x11d3,
            0xbd,
            0x81,
            0x0,
            0xa0,
            0xc9,
            0xa,
            0xc9,
            0xda);

// GetMemoryBank                    {6F3544DC-3472-41d9-853D-45F26DB22528}
DEFINE_GUID(M88IID_GetMemoryBank,
            0x6f3544dc,
            0x3472,
            0x41d9,
            0x85,
            0x3d,
            0x45,
            0xf2,
            0x6d,
            0xb2,
            0x25,
            0x28);

// IMouseUI                         {6B126C9B-60BE-49ee-9F12-031001156D78}
DEFINE_GUID(ChIID_MouseUI,
            0x6b126c9b,
            0x60be,
            0x49ee,
            0x9f,
            0x12,
            0x3,
            0x10,
            0x1,
            0x15,
            0x6d,
            0x78);

// IPadInput                        {149E23E5-A0EE-4430-A323-BD6CACAD34E3}
DEFINE_GUID(ChIID_PadInput,
            0x149e23e5,
            0xa0ee,
            0x4430,
            0xa3,
            0x23,
            0xbd,
            0x6c,
            0xac,
            0xad,
            0x34,
            0xe3);

#endif
