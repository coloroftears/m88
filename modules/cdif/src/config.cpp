// $Id: config.cpp,v 1.1 1999/12/28 11:25:53 cisc Exp $

#include "cdif/src/headers.h"
#include "cdif/src/config.h"
#include "resource.h"

// ---------------------------------------------------------------------------

ConfigCDIF::ConfigCDIF() {}

bool ConfigCDIF::Init(HINSTANCE _hinst) {
  hinst = _hinst;
  return true;
}

bool IFCALL ConfigCDIF::Setup(IConfigPropBase* _base, PROPSHEETPAGE* psp) {
  base = _base;

  memset(psp, 0, sizeof(PROPSHEETPAGE));
  psp->dwSize = sizeof(PROPSHEETPAGE);
  psp->dwFlags = 0;
  psp->hInstance = hinst;
  psp->pszTemplate = MAKEINTRESOURCE(IDD_CONFIG);
  psp->pszIcon = 0;
  psp->pfnDlgProc = (DLGPROC)(void*)PageGate;
  psp->lParam = (LPARAM) this;
  return true;
}

INT_PTR ConfigCDIF::PageProc(HWND hdlg, UINT msg, WPARAM wp, LPARAM lp) {
  switch (msg) {
    case WM_INITDIALOG:
      return TRUE;

    case WM_NOTIFY:
      switch (((NMHDR*)lp)->code) {
        case PSN_SETACTIVE:
          base->PageSelected(this);
          break;

        case PSN_APPLY:
          base->Apply();
          return PSNRET_NOERROR;
      }
      return TRUE;
  }
  return FALSE;
}

// static
INT_PTR ConfigCDIF::PageGate(HWND hwnd, UINT m, WPARAM w, LPARAM l) {
  ConfigCDIF* config = nullptr;
  if (m == WM_INITDIALOG) {
    PROPSHEETPAGE* pPage = (PROPSHEETPAGE*)l;
    if (config = reinterpret_cast<ConfigCDIF*>(pPage->lParam))
      ::SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG)config);
  } else {
    config = (ConfigCDIF*)::GetWindowLongPtr(hwnd, GWLP_USERDATA);
  }
  if (!config)
    return FALSE;
  return config->PageProc(hwnd, m, w, l);
}
