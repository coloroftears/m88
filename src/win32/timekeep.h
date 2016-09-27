// ---------------------------------------------------------------------------
//	M88 - PC-8801 Emulator.
//	Copyright (C) cisc 1998, 2001.
// ---------------------------------------------------------------------------
//	$Id: timekeep.h,v 1.1 2002/04/07 05:40:11 cisc Exp $

#if !defined(win32_timekeep_h)
#define win32_timekeep_h

#include "types.h"

// ---------------------------------------------------------------------------
//	TimeKeeper
//	現在の時間を数値(1/unit ミリ秒単位)で与えるクラス．
//	GetTime() で与えられる値自体は特別な意味を持っておらず，
//	連続した呼び出しを行うとき，返される値の差分が，
//	その呼び出しの間に経過した時間を示す．
//	即ち，GetTime() を呼んでから N (1/unit ミリ秒) 後に GetTime() を呼ぶと，
//	2度目に返される値は最初に返される値より N 増える．  
//
class TimeKeeper
{
public:
	enum
	{
		unit = 100,			// 最低 1 ということで．
	};

public:
	TimeKeeper();
	~TimeKeeper();

	uint32 GetTime();

private:
	uint32 freq;			// ソースクロックの周期
	uint32 base;			// 最後の呼び出しの際の元クロックの値
	uint32 time;			// 最後の呼び出しに返した値
};

#endif // !defined(win32_timekeep_h)
