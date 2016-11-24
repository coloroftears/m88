//
//  Windows 系 includes
//  すべての標準ヘッダーを含む
//
//  $Id: headers.h,v 1.13 2003/05/12 22:26:35 cisc Exp $
//

#pragma once

// --- STL 関係

#ifdef _MSC_VER
#undef max
#define max _MAX
#undef min
#define min _MIN
#endif