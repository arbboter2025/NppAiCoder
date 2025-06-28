// AiAssistWnd.cpp
#include "AiAssistWnd.h"
#include "resource.h"
#include "Utils.h"
#include "Exui.h"
#include <richedit.h>
#include <commctrl.h>
#include <format>

AiAssistWnd::AiAssistWnd(HINSTANCE hInst, const NppData& nppData, const Scintilla::PluginConfig& plugConf)
    : DockingDlgInterface(IDD_DIALOG_AI_ASSIST), _hInst(hInst), _nppData(nppData), _plugConf(plugConf)
{
    this->_hParent = _nppData._nppHandle;
}


void AiAssistWnd::init()
{
    DockingDlgInterface::init(_hInst, _hParent);

    // 注册Dock窗口
    tTbData tbData = { 0 };
    DockingDlgInterface::create(&tbData);
    tbData.uMask = DWS_DF_CONT_RIGHT | DWS_ICONTAB;;
    tbData.pszModuleName = L"AI Assistant";;
    tbData.dlgID = _dlgID;
    ::SendMessage(_nppData._nppHandle, NPPM_DMMREGASDCKDLG, 0, (LPARAM)&tbData);

    // 初始化UI控件
    initControls();

    // 设置初始大小
    RECT rc;
    GetClientRect(_hSelf, &rc);
    ::SetWindowPos(_hSelf, nullptr, rc.left, rc.top, 300, rc.bottom, SWP_NOZORDER | SWP_NOMOVE);

    layoutControls(rc.right, rc.bottom);
    SetTaskStatus(g_bRun.load() ? TASK_STATUS_BEGIN : TASK_STATUS_FINISH);
    SendMessageW(_hActionBtn, BM_SETIMAGE, IMAGE_ICON, (LPARAM)(g_bRun.load() ?  _hStopIcon : _hSendIcon));
}

void AiAssistWnd::display(bool toShow/* = true*/)
{
    DockingDlgInterface::display(toShow);
    if (toShow)
    {
        ReloadConfig();
    }
}

LRESULT CALLBACK InputEditSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    auto pThis = (AiAssistWnd*)dwRefData;
    if (uMsg == WM_KEYDOWN && wParam == VK_RETURN) 
    {
        if (GetKeyState(VK_CONTROL) & 0x8000) 
        {
            if (pThis->OnInputFinished())
            {
                // 阻止默认行为
                return 0;
            }
        }
    }
    else if (uMsg == WM_CHAR)
    {
        if (GetKeyState(VK_CONTROL) & 0x8000)
        {
            // 屏蔽回车换行的文本输入
            if (wParam == VK_RETURN || wParam == 0x0A)
            {
                return 0;
            }
        }
        if (wParam == VK_RETURN)
        {
            if (GetKeyState(VK_CONTROL) & 0x8000)
            {
                return 0;
            }
            const wchar_t* pLR = L"\r\n";
            SendMessageW(hWnd, EM_REPLACESEL, FALSE, (LPARAM)pLR); 
        }
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

void AiAssistWnd::initControls()
{
    // 创建控件
    _hModelCombo = ::CreateWindowExW(0, WC_COMBOBOX, L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST | WS_TABSTOP,
        0, 0, 0, 0, _hSelf, (HMENU)IDC_MODEL_COMBO, _hInst, nullptr);

    _hPromtCombo = ::CreateWindowExW(0, WC_COMBOBOX, L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST | WS_TABSTOP,
        0, 0, 0, 0, _hSelf, (HMENU)IDC_PROMT_COMBO, _hInst, nullptr);

    _hInputEdit = ::CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL,
        0, 0, 0, 0, _hSelf, (HMENU)IDC_INPUT_EDIT, _hInst, nullptr);
    SetWindowSubclass(_hInputEdit, InputEditSubclassProc, 0, (DWORD_PTR)this);

    // 使用RichEdit 4.1
    LoadLibraryW(L"Msftedit.dll");
    _hAnswerView = ::CreateWindowExW(WS_EX_CLIENTEDGE, MSFTEDIT_CLASS, L"",
        WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | WS_VSCROLL,
        0, 0, 0, 0, _hSelf, (HMENU)IDC_ANSWER_VIEW, _hInst, nullptr);
    // 只读浅灰色背景
    ::SendMessage(_hAnswerView, EM_SETBKGNDCOLOR, 0, (LPARAM)GetSysColor(COLOR_BTNFACE));

    // 创建操作按钮
    _hActionBtn = ::CreateWindowExW(WS_EX_CLIENTEDGE, L"BUTTON", L"",
        WS_CHILD | WS_VISIBLE | BS_ICON | BS_PUSHBUTTON,
        0, 0, 28, 28, _hSelf, (HMENU)IDC_ACTION_BUTTON, _hInst, nullptr);
    SetWindowText(_hActionBtn, _T("发送"));
    // 加载图标资源
    _hSendIcon = (HICON)LoadImageW(_hInst, MAKEINTRESOURCEW(IDI_ICON_SEND), 
        IMAGE_ICON, 24, 24, LR_DEFAULTCOLOR);
    _hStopIcon = (HICON)LoadImageW(_hInst, MAKEINTRESOURCEW(IDI_ICON_STOP),
        IMAGE_ICON, 24, 24, LR_DEFAULTCOLOR);
    SendMessageW(_hActionBtn, BM_SETIMAGE, IMAGE_ICON, (LPARAM)_hSendIcon);
}

void AiAssistWnd::updatelList(HWND hCombo, const std::vector<std::string>& options, int sel/* = -1*/, const char* seltext/* = nullptr*/)
{
    if (hCombo == nullptr || !IsWindow(hCombo)) return;
    SendMessageA(hCombo, CB_RESETCONTENT, 0, 0);
    for (const auto& opt : options)
    {
        SendMessageA(hCombo, CB_ADDSTRING, 0, (LPARAM)opt.c_str());
    }
    if (!options.empty())
    {
        if(sel >= 0) SendMessageA(hCombo, CB_SETCURSEL, sel, 0);
    }
    if (seltext != nullptr && seltext[0] != '\0')
    {
        Ui::Util::ComboSelText(hCombo, seltext);
    }
}

void AiAssistWnd::ReloadConfig()
{
    if(!_plugConf.platform.empty())
    { 
        auto& p = _plugConf.Platform();
        updatelList(_hModelCombo, p.models, -1, p.model_name.c_str());
    }
    std::string seltext;
    Ui::Util::ComboGetSelText(_hPromtCombo, seltext);
    std::vector<std::string> promts;
    for (auto& [k, v] : _plugConf.promt.exts)
    {
        promts.push_back(Scintilla::String::UTF8ToGBK(k.c_str()));
    }
    updatelList(_hPromtCombo, promts, -1, seltext.c_str());
}

void AiAssistWnd::OnModelComboSelChange()
{
    if (fnOnModelSelChange == nullptr) return;
    int selIndex = (int)SendMessage(_hModelCombo, CB_GETCURSEL, 0, 0);
    if(selIndex != CB_ERR)
    {
        // 或者获取文本内容处理
        TCHAR buffer[256] = { L'\0'};
        memset(buffer, 0, sizeof(buffer));
        SendMessage(_hModelCombo, CB_GETLBTEXT, selIndex, (LPARAM)buffer);
        auto model = Scintilla::String::wstring2s(buffer);
        appendNotify("已切换模型到" + model);
        fnOnModelSelChange(model);
    }
}

void AiAssistWnd::SetTitle(const char* title)
{
    std::string text = "AiCoder";
    if (title == nullptr || title[0] == '\0')
    {
        // 获取版本信息
        std::string ver = Scintilla::Util::GetBuildDate();
        std::string author = Scintilla::Util::GetVersionInfo("CompanyName");
        if (!ver.empty() && !author.empty())
        {
            text = "AiCoder By " + author + " V" + ver;
        }
    }
    else
    {
        text = title;
    }
    SetWindowTextA(_hSelf, text.c_str());
}

bool AiAssistWnd::OnInputFinished()
{
    // 获取文本
    int len = GetWindowTextLengthW(_hInputEdit);
    if (len <= 0)
    {
        return false;
    }

    std::wstring buf((size_t)len + 1, L'\0');
    GetWindowTextW(_hInputEdit, &buf[0], (int)buf.size());
    auto text = Scintilla::String::TrimAll(Scintilla::String::wstring2s(&buf[0], false));
    if (text.empty())
    {
        return false;
    }

    // 模板
    std::string promtName;
    if (Ui::Util::ComboGetSelText(_hPromtCombo, promtName) >= 0 && !promtName.empty())
    {
        text = _plugConf.promt.Format(Scintilla::String::GBKToUTF8(promtName.c_str()), text);
    }

    // 清空输入内容
    SetWindowTextW(_hInputEdit, L"");
    SendMessageW(_hInputEdit, EM_SETSEL, 0, 0);
    SendMessageW(_hInputEdit, EM_SCROLLCARET, 0, 0);
    
    // 处理输入
    appendAnswer("【问】\r\n" + text);
    SetTaskStatus(TASK_STATUS_BEGIN);
    if (fnOnInputFinished != nullptr) fnOnInputFinished(text);
    return true;
}

void AiAssistWnd::OnOutputFinished(const std::string& end)
{
    if(!end.empty()) appendAnswer(end, false);
    SetTaskStatus(TASK_STATUS_FINISH);
}

void AiAssistWnd::SetTaskStatus(int stat)
{
    if (stat == TASK_STATUS_BEGIN)
    {
        ::EnableWindow(_hAnswerView, FALSE);
        SendMessageW(_hActionBtn, BM_SETIMAGE, IMAGE_ICON, (LPARAM)_hStopIcon);
    }
    else if (stat == TASK_STATUS_FINISH)
    {
        ::EnableWindow(_hAnswerView, TRUE);
        SendMessageW(_hActionBtn, BM_SETIMAGE, IMAGE_ICON, (LPARAM)_hSendIcon);
    }
}

INT_PTR AiAssistWnd::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) 
    {
    case WM_INITDIALOG:
        SetTitle();
        return TRUE;

    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED) 
        {
            RECT rc;
            GetClientRect(_hSelf, &rc);
            layoutControls(rc.right, rc.bottom);
        }
        return TRUE;

    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            int wmEvent = HIWORD(wParam);
            if (wmId == IDC_MODEL_COMBO && wmEvent == CBN_SELCHANGE)
            {
                OnModelComboSelChange();
                return TRUE;
            }
            if (wmId == IDC_ACTION_BUTTON && wmEvent == BN_CLICKED)
            {
                if (g_bRun.load())
                {
                    g_bRun.store(false);
                }
                else
                {
                    OnInputFinished();
                }
                return TRUE;
            }
        }
        break;
    case WM_NOTIFY:
        // 处理其他通知消息
        break;
    case WM_CTLCOLORSTATIC:   // 处理静态文本
    case WM_CTLCOLORLISTBOX:  // 处理下拉列表背景
    case WM_CTLCOLOREDIT:
        break;
    }

    return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
}

void AiAssistWnd::layoutControls(int width, int height)
{
    const int BTN_SIZE = 28;
    int yPos = CONTROL_MARGIN;
    int nHeight = CONTROL_MARGIN;

    // 回答区域
    int answerHeight = height - 4*CONTROL_MARGIN - COMBO_HEIGHT - INPUT_HEIGHT - BTN_SIZE;
    ::MoveWindow(_hAnswerView,
        CONTROL_MARGIN, yPos,
        width - 2 * CONTROL_MARGIN, answerHeight, TRUE);
    yPos += CONTROL_MARGIN + answerHeight;

    // 模型选择框
    int nComboWidth = (width - 2 * CONTROL_MARGIN) / 2;
    ::MoveWindow(_hModelCombo,
        CONTROL_MARGIN, yPos,
        nComboWidth, COMBO_HEIGHT, TRUE);
    // 提示词
    ::MoveWindow(_hPromtCombo,
        2* CONTROL_MARGIN + nComboWidth, yPos,
        nComboWidth, COMBO_HEIGHT, TRUE);
    yPos += COMBO_HEIGHT + CONTROL_MARGIN;

    // 输入框
    ::MoveWindow(_hInputEdit,
        CONTROL_MARGIN, yPos,
        width - 2 * CONTROL_MARGIN, INPUT_HEIGHT, TRUE);
    yPos += INPUT_HEIGHT + CONTROL_MARGIN;

    // 按钮布局
    // 输入框布局
    int btnX = width - BTN_SIZE - CONTROL_MARGIN;
    ::MoveWindow(_hActionBtn, btnX, yPos, BTN_SIZE, BTN_SIZE, TRUE);
    yPos += BTN_SIZE + CONTROL_MARGIN;
}

void AiAssistWnd::gotoEnd(HWND hWnd)
{
    if (hWnd == nullptr) return;
    // 移动光标到尾部
    int textLength = GetWindowTextLength(_hAnswerView);
    SendMessage(hWnd, EM_SETSEL, (WPARAM)textLength, (LPARAM)textLength);
}


void AiAssistWnd::appendAnswer(const std::string& answer, bool bNewLine/* = true*/)
{
    CHARFORMAT2W cf = { sizeof(CHARFORMAT2W) };
    if (bNewLine)
    {
        // 添加时间戳
        SYSTEMTIME st;
        GetLocalTime(&st);
        std::string timestamp = std::format("[{:02}:{:02}:{:02}] ",
            st.wHour, st.wMinute, st.wSecond);

        // 字体颜色+粗体
        cf.dwMask = CFM_COLOR | CFM_BOLD;
        cf.crTextColor = RGB(0, 128, 0);
        cf.dwEffects = CFE_BOLD;
        SendMessageA(_hAnswerView, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
        gotoEnd(_hAnswerView);
        SendMessageA(_hAnswerView, EM_REPLACESEL, FALSE, (LPARAM)timestamp.c_str());
    }
    cf.dwMask = CFM_COLOR | CFM_BOLD;
    cf.crTextColor = RGB(0, 0, 0);
    cf.dwEffects = 0;
    std::string text = answer + (bNewLine ? "\r\n" : "");
    SendMessageA(_hAnswerView, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
    gotoEnd(_hAnswerView);
    SendMessageA(_hAnswerView, EM_REPLACESEL, FALSE, (LPARAM)text.c_str());

    // 自动滚动到底部
    SendMessageA(_hAnswerView, WM_VSCROLL, SB_BOTTOM, 0);
}

void AiAssistWnd::clearConversation()
{
    if (!IsWindow(_hAnswerView))
        return;

    // 使用SETTEXTEX结构清空内容
    SETTEXTEX st = {
        ST_DEFAULT,    // 标志位
        1200           // 使用UTF-16编码
    };

    // 方法1：直接设置空文本（保留格式）
    SendMessageW(_hAnswerView, EM_SETTEXTEX, (WPARAM)&st, (LPARAM)L"");

    // 方法2：通过选择全部删除（更彻底）
    // SendMessageW(_hAnswerView, EM_SETSEL, 0, -1);   // 全选
    // SendMessageW(_hAnswerView, EM_REPLACESEL, 0, (LPARAM)L""); // 替换为空

    // 可选：重置滚动条位置
    SendMessageW(_hAnswerView, WM_VSCROLL, SB_TOP, 0);

    // 可选：清除Undo缓冲区
    SendMessageW(_hAnswerView, EM_EMPTYUNDOBUFFER, 0, 0);
}

AiAssistWnd::~AiAssistWnd()
{
    RemoveWindowSubclass(_hInputEdit, InputEditSubclassProc, 0);
}
