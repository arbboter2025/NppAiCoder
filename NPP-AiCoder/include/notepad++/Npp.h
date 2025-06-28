#pragma once
#include <Windows.h>

// 资源
#define NPPM_GETNPPDATA  (NPPMSG + 30)
#define DWS_DF_CONT_RIGHT  0x02000000
#define DWS_ICONTAB        0x00004000
#define DWS_USEOWNDARKMODE 0x00040000

// 结构体定义应包含在头文件中
typedef struct {
    HWND hClient;
    TCHAR* pszName;
    int dlgID;
    UINT uMask;
    HICON hIconTab;
    TCHAR* pszAddInfo;
} tTbData;