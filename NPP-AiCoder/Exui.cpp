#include "Exui.h"
#include "resource.h"
#include "Utils.h"
#include <windowsx.h>

using namespace Scintilla;

void Ui::Util::Show(HWND hWnd, bool bShow, HWND hParent/* = nullptr*/) 
{
    if (!hWnd) return;
    if (bShow) 
    {
        // 获取父窗口句柄（Notepad++主窗口）
        hParent = hParent == nullptr ? GetParent(hWnd) : hParent;
        if (!hParent) 
        {
            hParent = GetDesktopWindow();
        }

        // 获取父窗口和对话框的尺寸
        RECT rcParent, rcDlg;
        GetWindowRect(hParent, &rcParent);
        GetWindowRect(hWnd, &rcDlg);

        // 计算居中位置
        int dialogWidth = rcDlg.right - rcDlg.left;
        int dialogHeight = rcDlg.bottom - rcDlg.top;

        int x = rcParent.left + (rcParent.right - rcParent.left - dialogWidth) / 2;
        int y = rcParent.top + (rcParent.bottom - rcParent.top - dialogHeight) / 2;

        // 设置窗口位置并显示
        SetWindowPos(
            hWnd, 
            nullptr,
            x, y, 
            0, 0,
            SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW
        );

        // 激活窗口
        SetForegroundWindow(hWnd);
    } 
    else 
    {
        ShowWindow(hWnd, SW_HIDE);
    }
}

std::string Ui::Util::GetText(HWND hWnd)
{
    LRESULT textLength = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    if (textLength <= 0)
    {
        return "";
    }
    const size_t bufferSize = static_cast<size_t>(textLength) + 1;
    char* buffer = new char[bufferSize] { 0 };
    SendMessageA( hWnd, WM_GETTEXT, bufferSize, reinterpret_cast<LPARAM>(buffer));
    buffer[textLength] = '\0';
    std::string text = buffer;
    delete[] buffer;
    return text;
}

void Ui::Util::ListViewHeader(HWND hList, std::vector<std::string>& headers)
{
    // 参数校验
    if (!hList || !IsWindow(hList)) return;
    const int nTotal = ListView_GetItemCount(hList);

    // 获取列数
    HWND hHeader = ListView_GetHeader(hList);
    const int nCols = Header_GetItemCount(hHeader);
    headers.clear();
    for (int nCol = 0; nCol < nCols; ++nCol)
    {
        // 获取列标题
        LVCOLUMNW lvCol = {0};
        wchar_t szColName[256] = {0};
        lvCol.mask = LVCF_TEXT;
        lvCol.pszText = szColName;
        lvCol.cchTextMax = _countof(szColName);

        if (!ListView_GetColumn(hList, nCol, &lvCol)) 
        {
            continue;
        }
        headers.push_back(String::wstring2s(lvCol.pszText, false));
    }
}

int Ui::Util::ListViewGetSelRow(HWND hList)
{
    if (!hList || !IsWindow(hList)) return -1;
    return ListView_GetNextItem(hList, -1, LVNI_SELECTED);
}

int Ui::Util::ComboSelText(HWND hWnd, const std::string& text)
{
    if (!hWnd || !IsWindow(hWnd)) return -1;
    auto wtext = String::string2w(text, false);

    // 查找精确匹配的项
    LRESULT nIdx = SendMessageW(hWnd, CB_FINDSTRINGEXACT, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(wtext.c_str()));
    if (nIdx == CB_ERR) return -1;

    // 设置选中项并返回索引
    SendMessageW(hWnd, CB_SETCURSEL, static_cast<WPARAM>(nIdx), 0);
    return static_cast<int>(nIdx);
}

int Ui::Util::ComboGetSelText(HWND hCombo, std::string& text)
{
    if (!hCombo || !IsWindow(hCombo)) return -1;
    // 获取选中项索引
    int nIndex = (int)SendMessageA(hCombo, CB_GETCURSEL, 0, 0);
    if (nIndex == CB_ERR)
    {
        return -1;
    }

    // 获取文本长度（包含终止null）
    int len = (int)SendMessageA(hCombo, CB_GETLBTEXTLEN, nIndex, 0);
    if (len == CB_ERR)
    {
        return -1;
    }

    // 获取选中项文本
    std::vector<char> buf((size_t)len + 1, '\0');
    if (SendMessageA(hCombo, CB_GETLBTEXT, nIndex, (LPARAM)buf.data()) == CB_ERR)
    {
        return -1;
    }
    text.assign(buf.data());
    return nIndex;
}

void Ui::Util::ComboSelClear(HWND hCombo)
{
    if (!hCombo || !IsWindow(hCombo)) return;
    SendMessage(hCombo, CB_SETCURSEL, (WPARAM)-1, 0);
}

void Ui::Util::ComboRemove(HWND hWnd, int nSel/* = -1*/)
{
    if (!::IsWindow(hWnd)) 
    {
        return;
    }

    if (nSel == -1) 
    {
        nSel = (int)::SendMessage(hWnd, CB_GETCURSEL, 0, 0);
        if (nSel == CB_ERR)
        {
            return;
        }
    }
    else
    {
        int nTotal = (int)::SendMessage(hWnd, CB_GETCOUNT, 0, 0);
        if (nSel >= nTotal) 
        {
            return;
        }
    }
    ::SendMessage(hWnd, CB_DELETESTRING, (WPARAM)nSel, 0);
    ComboSelClear(hWnd);
}

INT_PTR FieldEditDlg::DoModal()
{
    // 创建模态对话框（需提前定义对话框模板ID，假设为IDD_FIELD_EDIT_DLG）
    return DialogBoxParam(
        m_hInstance, 
        MAKEINTRESOURCE(IDD_DIALOG_EDIT_FIELD),
        m_hParent,
        FieldEditDlg::DlgProc,
        reinterpret_cast<LPARAM>(this)
    );
}

FieldEditDlg::FieldEditDlg(HINSTANCE hInstance, HWND hParent)
    :m_hInstance(hInstance), m_hParent(hParent), m_hDlg(nullptr)
{
    // 初始化字体
    m_hFont = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH, L"新宋体");
}

FieldEditDlg::~FieldEditDlg()
{
    if (m_hFont) DeleteObject(m_hFont);
}

// 对话框消息处理
INT_PTR CALLBACK FieldEditDlg::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_INITDIALOG) 
    {
        // 关联类实例指针到窗口
        SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
        auto* pThis = reinterpret_cast<FieldEditDlg*>(lParam);
        pThis->m_hDlg = hDlg;
        pThis->OnInitDialog();
        Ui::Util::Show(hDlg, true);
        return TRUE;
    }

    // 获取类实例指针
    auto pThis = reinterpret_cast<FieldEditDlg*>(
        GetWindowLongPtr(hDlg, GWLP_USERDATA)
        );

    if (pThis) 
    {
        return pThis->RealDlgProc(hDlg, uMsg, wParam, lParam);
    }
    return FALSE;
}

INT_PTR FieldEditDlg::RealDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) 
    {
    case WM_INITDIALOG: 
        {
            SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)this);
            OnInitDialog();
            return TRUE;
        }
    case WM_COMMAND: 
        {
            int wmId = LOWORD(wParam);
            int wmEvent = HIWORD(wParam);
            if (wmId == IDOK && wmEvent == BN_CLICKED) 
            {
                OnSave();
                EndDialog(m_hDlg, IDOK);
                return TRUE;
            } 
            else if (wmId == IDCANCEL && wmEvent == BN_CLICKED) 
            {
                EndDialog(m_hDlg, IDCANCEL);
                return TRUE;
            }
            break;
        }
    case WM_CLOSE:
        EndDialog(m_hDlg, IDCLOSE);
        return TRUE;
    }
    return FALSE;
}

void FieldEditDlg::OnInitDialog()
{
    if(!m_fields.empty()) CreateDynamicControls();
    SetWindowTextA(m_hDlg, m_strTitle.c_str());
}

void FieldEditDlg::OnSave()
{
    for(size_t i=0; i<m_fields.size(); i++)
    {
        m_fields[i].val = Ui::Util::GetText(m_filedsHwnd[i]);
    }
}


void FieldEditDlg::CreateDynamicControls()
{
    const int labelWidth = m_nLabelWidth;   // 标签宽度
    const int ctrlHeight = 24;    // 控件高度
    const int margin = 10;        // 边距

    RECT rcClient;
    GetClientRect(m_hDlg, &rcClient);

    int yPos = margin;
    std::wstring clsName;
    for (size_t i=0; i<m_fields.size(); i++)
    {
        auto& field = m_fields[i];
        auto& fieldName = m_fields[i].name;
        // 创建标签
        HWND hLabel = CreateWindowExW(0, L"STATIC", 
            String::string2w(fieldName, false).c_str(),
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            margin, yPos, labelWidth, ctrlHeight,
            m_hDlg, NULL, m_hInstance, NULL);
        SendMessageW(hLabel, WM_SETFONT, (WPARAM)m_hFont, TRUE);

        // 创建输入控件
        DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_TABSTOP;
        if (field.type == FieldType::Combo)
        {
            dwStyle |= CBS_DROPDOWN | CBS_HASSTRINGS;
            clsName = L"COMBOBOX";
        } 
        else
        {
            dwStyle |= ES_LEFT | ES_AUTOHSCROLL;
            clsName = L"EDIT";
        }

        HWND hCtrl = CreateWindowExW(WS_EX_CLIENTEDGE,
            clsName.c_str(),
            L"",
            dwStyle,
            labelWidth + margin*2, 
            yPos,
            m_nBoxWidth,
            ctrlHeight,
            m_hDlg,
            NULL,
            m_hInstance,
            NULL);
        SendMessageW(hCtrl, WM_SETFONT, (WPARAM)m_hFont, TRUE);

        // 初始化控件数据
        if (field.type == FieldType::Combo) 
        {
            for (const auto& opt : field.options) 
            {
                SendMessageW(hCtrl, CB_ADDSTRING, 0, (LPARAM)String::string2w(opt, false).c_str());
            }
            if (!field.val.empty()) 
            {
                SendMessageW(hCtrl, CB_SELECTSTRING, -1, (LPARAM)String::string2w(field.val, false).c_str());
            }
        } 
        else 
        {
            SetWindowTextW(hCtrl, String::string2w(field.val, false).c_str());
        }
        EnableWindow(hCtrl, !field.readonly);
        m_filedsHwnd.push_back(hCtrl);
        yPos += ctrlHeight + margin;
    }

    // 调整对话框尺寸
    int nBtnHeight = 25;
    SetWindowPos(m_hDlg, NULL, 0, 0, 
        m_nBoxWidth + m_nLabelWidth + 4*margin, yPos + margin + ctrlHeight + nBtnHeight + margin,
        SWP_NOMOVE | SWP_NOZORDER);
    GetClientRect(m_hDlg, &rcClient);

    // 调整按钮
    int nBtnWidth = 80;
    HWND hCtrl = GetDlgItem(m_hDlg, IDOK);
    if (hCtrl != nullptr)
    {
        int xPos = rcClient.left + margin;
        SetWindowPos(hCtrl, NULL, xPos, yPos, nBtnWidth, nBtnHeight, NULL);
        hCtrl = GetDlgItem(m_hDlg, IDCANCEL);
        xPos = rcClient.right - nBtnWidth - margin;
        SetWindowPos(hCtrl, NULL, xPos, yPos, nBtnWidth, nBtnHeight, NULL);
    }
}