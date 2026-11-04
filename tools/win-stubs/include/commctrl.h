// commctrl.h stub
#pragma once
#include <windows.h>
enum : UINT {
    TB_GETPOS_ = 0, TBM_GETPOS = 0x0401, TBM_SETPOS = 0x0405, TBM_SETRANGE = 0x0406,
};
enum : UINT { TBS_HORZ = 0x0000, TBS_AUTOTICKS = 0x0001 };
#define SNDMSG SendMessageW
inline BOOL InitCommonControlsEx(const INITCOMMONCONTROLSEX* icc) { (void)icc; return TRUE; }
