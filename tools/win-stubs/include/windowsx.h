// windowsx.h stub — the control macros FlowScale uses.
#pragma once
#include <windows.h>
#define ComboBox_AddString(hwndCtl, lpsz)   ((int)(DWORD_PTR)SNDMSG((hwndCtl), CB_ADDSTRING_, (WPARAM)0, (LPARAM)(lpsz)))
#define ComboBox_DeleteString(hwndCtl, i)   ((int)(DWORD_PTR)SNDMSG((hwndCtl), CB_DELETESTRING_, (WPARAM)(i), 0))
#define ComboBox_ResetContent(hwndCtl)      ((int)(DWORD_PTR)SNDMSG((hwndCtl), CB_RESETCONTENT_, 0, 0))
#define ComboBox_GetCurSel(hwndCtl)         ((int)(DWORD_PTR)SNDMSG((hwndCtl), CB_GETCURSEL_, 0, 0))
#define ComboBox_SetCurSel(hwndCtl, i)      ((int)(DWORD_PTR)SNDMSG((hwndCtl), CB_SETCURSEL_, (WPARAM)(i), 0))
#define ComboBox_GetCount(hwndCtl)          ((int)(DWORD_PTR)SNDMSG((hwndCtl), CB_GETCOUNT_, 0, 0))
#define ComboBox_GetLBText(hwndCtl, i, b)   ((int)(DWORD_PTR)SNDMSG((hwndCtl), CB_GETLBTEXT_, (WPARAM)(i), (LPARAM)(b)))
#define ComboBox_GetLBTextLen(hwndCtl, i)   ((int)(DWORD_PTR)SNDMSG((hwndCtl), CB_GETLBTEXTLEN_, (WPARAM)(i), 0))
enum : UINT { CB_ADDSTRING_ = 0x0143, CB_DELETESTRING_ = 0x0144, CB_RESETCONTENT_ = 0x014B,
              CB_GETCURSEL_ = 0x0147, CB_SETCURSEL_ = 0x014E, CB_GETCOUNT_ = 0x0146,
              CB_GETLBTEXT_ = 0x0148, CB_GETLBTEXTLEN_ = 0x0149 };
