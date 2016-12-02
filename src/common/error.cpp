// ----------------------------------------------------------------------------
//  M88 - PC-8801 series emulator
//  Copyright (C) cisc 1999.
// ----------------------------------------------------------------------------
//  $Id: error.cpp,v 1.6 2002/04/07 05:40:08 cisc Exp $

#include "common/error.h"

Errno Error::err = Errno::unknown;

const char* Error::ErrorText[] = {
    "原因不明のエラーが発生しました.",
    "PC88 の ROM ファイルが見つかりません.\nファイル名を確認してください.",
    "メモリの割り当てに失敗しました.", "画面の初期化に失敗しました.",
    "スレッドを作成できません.", "テキストフォントが見つかりません.",
    "実行ファイルが書き換えられた恐れがあります.\n"
    "故意でなければウィルス感染が疑われます.",
};

// static
const char* Error::GetErrorText() {
  return ErrorText[static_cast<int>(err)];
}

// static
void Error::SetError(Errno e) {
  err = e;
}
