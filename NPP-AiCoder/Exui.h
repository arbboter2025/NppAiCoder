#pragma once
#include <windows.h>
#include "PluginConf.h"
#include <commctrl.h>

namespace Ui
{
    class Util
    {
    public:
        // 局长显示窗口
        static void Show(HWND hWnd, bool bShow, HWND hParent = nullptr);
        static std::string GetText(HWND hWnd);
        static void ListViewHeader(HWND hWnd, std::vector<std::string>& headers);
        static int ListViewGetSelRow(HWND hWnd);
        static int ComboSelText(HWND hWnd, const std::string& text);
        static int ComboGetSelText(HWND hWnd, std::string& text);
        static void ComboRemove(HWND hWnd, int nSel = -1);
        static void ComboSelClear(HWND hWnd);
    };
}

class FieldEditDlg
{
public:
    enum class FieldType
    {
        Edit,
        Combo,
    };

    struct Field
    {
        std::string name;
        std::string val;
        std::vector<std::string> options;
        FieldType type = FieldType::Edit;
        bool readonly = false;
    };
    FieldEditDlg(HINSTANCE hInstance, HWND hParent);
    ~FieldEditDlg();

    static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    INT_PTR DoModal();

private:
    INT_PTR RealDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void CreateDynamicControls();
    void OnInitDialog();
    void OnSave();

public:
    std::vector<Field> m_fields;
    int m_nLabelWidth = 100;
    int m_nBoxWidth = 300;
    std::string m_strTitle = "字段设置";

private:
    HINSTANCE m_hInstance = nullptr;
    HWND m_hDlg = nullptr;
    HWND m_hParent = nullptr;
    HFONT m_hFont = nullptr;
    std::vector<HWND> m_filedsHwnd;
};
