//  $Id: winvars.cpp,v 1.1 2000/11/02 12:43:52 cisc Exp $

#include "win32/headers.h"
#include "win32/winvars.h"

static WinVars winvars;

int WinVars::var[nparam];

void WinVars::Init() {
  // Windows �̃o�[�W����
  OSVERSIONINFO vi;
  memset(&vi, 0, sizeof(vi));
  vi.dwOSVersionInfoSize = sizeof(vi);
  GetVersionEx(&vi);
  var[MajorVer] = vi.dwMajorVersion;
  var[MinorVer] = vi.dwMinorVersion;

  // �\���̂Ƃ�
  var[OFNSIZE] = Var(MajorVer) <= 4 ? OFNV4SIZE : sizeof(OPENFILENAME);
  var[MIISIZE] = Var(MajorVer) <= 4 ? 44 : sizeof(MENUITEMINFO);
}
