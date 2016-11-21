// $Id: dderr.cpp,v 1.1 2000/02/09 10:47:38 cisc Exp $

#include "win32/headers.h"
#include "win32/types.h"
#include "win32/dderr.h"

#ifdef _DEBUG

#define ERR(x) \
  case x:      \
    err = #x;  \
    break

const char* GetDDERR(HRESULT hr) {
  const char* err = 0;
  switch ((uint32_t)hr) {
    ERR(DDERR_ALREADYINITIALIZED);
    ERR(DDERR_CANNOTATTACHSURFACE);
    ERR(DDERR_CANNOTDETACHSURFACE);
    ERR(DDERR_CURRENTLYNOTAVAIL);
    ERR(DDERR_EXCEPTION);
    ERR(DDERR_GENERIC);
    ERR(DDERR_HEIGHTALIGN);
    ERR(DDERR_INCOMPATIBLEPRIMARY);
    ERR(DDERR_INVALIDCAPS);
    ERR(DDERR_INVALIDCLIPLIST);
    ERR(DDERR_INVALIDMODE);
    ERR(DDERR_INVALIDOBJECT);
    ERR(DDERR_INVALIDPARAMS);
    ERR(DDERR_INVALIDPIXELFORMAT);
    ERR(DDERR_INVALIDRECT);
    ERR(DDERR_LOCKEDSURFACES);
    ERR(DDERR_NO3D);
    ERR(DDERR_NOALPHAHW);
    ERR(DDERR_NOCLIPLIST);
    ERR(DDERR_NOCOLORCONVHW);
    ERR(DDERR_NOCOOPERATIVELEVELSET);
    ERR(DDERR_NOCOLORKEY);
    ERR(DDERR_NOCOLORKEYHW);
    ERR(DDERR_NODIRECTDRAWSUPPORT);
    ERR(DDERR_NOEXCLUSIVEMODE);
    ERR(DDERR_NOFLIPHW);
    ERR(DDERR_NOGDI);
    ERR(DDERR_NOMIRRORHW);
    ERR(DDERR_NOTFOUND);
    ERR(DDERR_NOOVERLAYHW);
    ERR(DDERR_NORASTEROPHW);
    ERR(DDERR_NOROTATIONHW);
    ERR(DDERR_NOSTRETCHHW);
    ERR(DDERR_NOT4BITCOLOR);
    ERR(DDERR_NOT4BITCOLORINDEX);
    ERR(DDERR_NOT8BITCOLOR);
    ERR(DDERR_NOTEXTUREHW);
    ERR(DDERR_NOVSYNCHW);
    ERR(DDERR_NOZBUFFERHW);
    ERR(DDERR_NOZOVERLAYHW);
    ERR(DDERR_OUTOFCAPS);
    ERR(DDERR_OUTOFMEMORY);
    ERR(DDERR_OUTOFVIDEOMEMORY);
    ERR(DDERR_OVERLAYCANTCLIP);
    ERR(DDERR_OVERLAYCOLORKEYONLYONEACTIVE);
    ERR(DDERR_PALETTEBUSY);
    ERR(DDERR_COLORKEYNOTSET);
    ERR(DDERR_SURFACEALREADYATTACHED);
    ERR(DDERR_SURFACEALREADYDEPENDENT);
    ERR(DDERR_SURFACEBUSY);
    ERR(DDERR_CANTLOCKSURFACE);
    ERR(DDERR_SURFACEISOBSCURED);
    ERR(DDERR_SURFACELOST);
    ERR(DDERR_SURFACENOTATTACHED);
    ERR(DDERR_TOOBIGHEIGHT);
    ERR(DDERR_TOOBIGSIZE);
    ERR(DDERR_TOOBIGWIDTH);
    ERR(DDERR_UNSUPPORTED);
    ERR(DDERR_UNSUPPORTEDFORMAT);
    ERR(DDERR_UNSUPPORTEDMASK);
    ERR(DDERR_VERTICALBLANKINPROGRESS);
    ERR(DDERR_WASSTILLDRAWING);
    ERR(DDERR_XALIGN);
    ERR(DDERR_INVALIDDIRECTDRAWGUID);
    ERR(DDERR_DIRECTDRAWALREADYCREATED);
    ERR(DDERR_NODIRECTDRAWHW);
    ERR(DDERR_PRIMARYSURFACEALREADYEXISTS);
    ERR(DDERR_NOEMULATION);
    ERR(DDERR_REGIONTOOSMALL);
    ERR(DDERR_CLIPPERISUSINGHWND);
    ERR(DDERR_NOCLIPPERATTACHED);
    ERR(DDERR_NOHWND);
    ERR(DDERR_HWNDSUBCLASSED);
    ERR(DDERR_HWNDALREADYSET);
    ERR(DDERR_NOPALETTEATTACHED);
    ERR(DDERR_NOPALETTEHW);
    ERR(DDERR_BLTFASTCANTCLIP);
    ERR(DDERR_NOBLTHW);
    ERR(DDERR_NODDROPSHW);
    ERR(DDERR_OVERLAYNOTVISIBLE);
    ERR(DDERR_NOOVERLAYDEST);
    ERR(DDERR_INVALIDPOSITION);
    ERR(DDERR_NOTAOVERLAYSURFACE);
    ERR(DDERR_EXCLUSIVEMODEALREADYSET);
    ERR(DDERR_NOTFLIPPABLE);
    ERR(DDERR_CANTDUPLICATE);
    ERR(DDERR_NOTLOCKED);
    ERR(DDERR_CANTCREATEDC);
    ERR(DDERR_NODC);
    ERR(DDERR_WRONGMODE);
    ERR(DDERR_IMPLICITLYCREATED);
    ERR(DDERR_NOTPALETTIZED);
    ERR(DDERR_UNSUPPORTEDMODE);
    ERR(DDERR_NOMIPMAPHW);
    ERR(DDERR_INVALIDSURFACETYPE);
    ERR(DDERR_NOOPTIMIZEHW);
    ERR(DDERR_NOTLOADED);
    ERR(DDERR_NOFOCUSWINDOW);
    ERR(DDERR_DCALREADYCREATED);
    ERR(DDERR_NONONLOCALVIDMEM);
    ERR(DDERR_CANTPAGELOCK);
    ERR(DDERR_CANTPAGEUNLOCK);
    ERR(DDERR_NOTPAGELOCKED);
    ERR(DDERR_MOREDATA);
    ERR(DDERR_VIDEONOTACTIVE);
    ERR(DDERR_DEVICEDOESNTOWNSURFACE);
    ERR(DDERR_NOTINITIALIZED);
  }
  return err;
}

#endif
