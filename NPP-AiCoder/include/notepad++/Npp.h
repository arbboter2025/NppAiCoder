#pragma once
#include <Windows.h>

// ��Դ
#define NPPM_GETNPPDATA  (NPPMSG + 30)
#define DWS_DF_CONT_RIGHT  0x02000000
#define DWS_ICONTAB        0x00004000
#define DWS_USEOWNDARKMODE 0x00040000

// �ṹ�嶨��Ӧ������ͷ�ļ���
typedef struct {
    HWND hClient;
    TCHAR* pszName;
    int dlgID;
    UINT uMask;
    HICON hIconTab;
    TCHAR* pszAddInfo;
} tTbData;