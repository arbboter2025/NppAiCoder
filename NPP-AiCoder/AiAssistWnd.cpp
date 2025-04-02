// AiAssistWnd.cpp
#include "AiAssistWnd.h"
#include "resource.h"
#include <richedit.h>
#include <commctrl.h>
#include <format>

AiAssistWnd::AiAssistWnd(HINSTANCE hInst, const NppData& nppData)
    : DockingDlgInterface(IDD_DIALOG_AI_ASSIST), _hInst(hInst), _nppData(nppData)
{
    this->_hParent = _nppData._nppHandle;
}

AiAssistWnd::~AiAssistWnd()
{
    if (_hFont) DeleteObject(_hFont);
    if (_hBoldFont) DeleteObject(_hBoldFont);
}

void AiAssistWnd::init()
{
    DockingDlgInterface::init(_hInst, _hParent);

    // ע��Dock����
    tTbData tbData = { 0 };
    DockingDlgInterface::create(&tbData);
    tbData.uMask = DWS_DF_CONT_RIGHT | DWS_ICONTAB;;
    tbData.pszModuleName = L"AI Assistant";;
    tbData.dlgID = _dlgID;
    ::SendMessage(_nppData._nppHandle, NPPM_DMMREGASDCKDLG, 0, (LPARAM)&tbData);

    // ��ʼ��UI�ؼ�
    initControls();

    // ���ó�ʼ��С
    RECT rc;
    GetClientRect(_hSelf, &rc);
    ::SetWindowPos(_hSelf, nullptr, rc.left, rc.top, 300, rc.bottom, SWP_NOZORDER | SWP_NOMOVE);

    layoutControls(rc.right, rc.bottom);
}

void AiAssistWnd::initControls()
{
    // �����ؼ�
    _hModelCombo = ::CreateWindowExW(0, WC_COMBOBOX, L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST | WS_TABSTOP,
        0, 0, 0, 0, _hSelf, (HMENU)IDC_MODEL_COMBO, _hInst, nullptr);

    _hInputEdit = ::CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL,
        0, 0, 0, 0, _hSelf, (HMENU)IDC_INPUT_EDIT, _hInst, nullptr);

    // ʹ��RichEdit 4.1
    LoadLibraryW(L"Msftedit.dll");
    _hAnswerView = ::CreateWindowExW(WS_EX_CLIENTEDGE, MSFTEDIT_CLASS, L"",
        WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | WS_VSCROLL | WS_HSCROLL,
        0, 0, 0, 0, _hSelf, (HMENU)IDC_ANSWER_VIEW, _hInst, nullptr);

    // ��ʼ������
    _hFont = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH, L"Segoe UI");

    _hBoldFont = CreateFontW(14, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH, L"Segoe UI");

    // Ӧ������
    SendMessageW(_hModelCombo, WM_SETFONT, (WPARAM)_hFont, TRUE);
    SendMessageW(_hInputEdit, WM_SETFONT, (WPARAM)_hFont, TRUE);
    SendMessageW(_hAnswerView, WM_SETFONT, (WPARAM)_hFont, TRUE);
}

INT_PTR AiAssistWnd::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) 
    {
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED) 
        {
            RECT rc;
            GetClientRect(_hSelf, &rc);
            layoutControls(rc.right, rc.bottom);
        }
        return TRUE;

    case WM_COMMAND:
        if (HIWORD(wParam) == EN_MAXTEXT && LOWORD(wParam) == IDC_INPUT_EDIT) 
        {
            handleUserInput();
            return TRUE;
        }
        break;

    case WM_NOTIFY:
        // ��������֪ͨ��Ϣ
        break;
    case WM_CTLCOLORSTATIC:   // ����̬�ı�
    case WM_CTLCOLORLISTBOX:  // ���������б���
    case WM_CTLCOLOREDIT:
        // ��������򱳾�ɫ
        if ((HWND)lParam == _hInputEdit) 
        {
            SetBkColor((HDC)wParam, RGB(255, 255, 255));
            return (INT_PTR)GetStockObject(WHITE_BRUSH);
        }
        // ����Ƿ�ΪĿ����Ͽ�������б��
        if ((HWND)lParam == _hModelCombo)
        {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));   // �����ı���ɫ
            SetBkColor(hdc, GetSysColor(COLOR_WINDOW));         // ���ñ�����ɫ
            return (LRESULT)GetSysColorBrush(COLOR_WINDOW);     // ���ر�����ˢ
        }
        break;
    }

    return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
}

void AiAssistWnd::layoutControls(int width, int height)
{
    int yPos = CONTROL_MARGIN;
    int nHeight = CONTROL_MARGIN;

    // �ش�����
    int answerHeight = height - 3*CONTROL_MARGIN - COMBO_HEIGHT - INPUT_HEIGHT;
    ::MoveWindow(_hAnswerView,
        CONTROL_MARGIN, yPos,
        width - 2 * CONTROL_MARGIN, answerHeight, TRUE);
    yPos += CONTROL_MARGIN + answerHeight;

    // ģ��ѡ���
    ::MoveWindow(_hModelCombo,
        CONTROL_MARGIN, yPos,
        width - 2 * CONTROL_MARGIN, COMBO_HEIGHT, TRUE);
    yPos += COMBO_HEIGHT + CONTROL_MARGIN;

    // �����
    ::MoveWindow(_hInputEdit,
        CONTROL_MARGIN, yPos,
        width - 2 * CONTROL_MARGIN, INPUT_HEIGHT, TRUE);
    yPos += INPUT_HEIGHT + CONTROL_MARGIN;
}

void AiAssistWnd::updateModelList(const std::vector<std::string>& models)
{
    SendMessageA(_hModelCombo, CB_RESETCONTENT, 0, 0);
    for (const auto& model : models) 
    {
        SendMessageA(_hModelCombo, CB_ADDSTRING, 0, (LPARAM)model.c_str());
    }
    if (!models.empty()) 
    {
        SendMessageA(_hModelCombo, CB_SETCURSEL, 0, 0);
    }
}

void AiAssistWnd::appendAnswer(const std::string& answer)
{
    // ���ʱ���
    SYSTEMTIME st;
    GetLocalTime(&st);
    std::string timestamp = std::format("[{:02}:{:02}:{:02}] ",
        st.wHour, st.wMinute, st.wSecond);

    // ���ø��ı���ʽ
    CHARFORMAT2W cf = { sizeof(CHARFORMAT2W) };
    cf.dwMask = CFM_COLOR | CFM_BOLD;
    cf.crTextColor = RGB(0, 128, 0);
    cf.dwEffects = CFE_BOLD;
    SendMessageA(_hAnswerView, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
    SendMessageA(_hAnswerView, EM_REPLACESEL, FALSE, (LPARAM)timestamp.c_str());

    // �ָ�Ĭ�ϸ�ʽ
    cf.dwMask = CFM_COLOR;
    cf.crTextColor = RGB(0, 0, 0);
    cf.dwEffects = 0;
    SendMessageA(_hAnswerView, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
    SendMessageA(_hAnswerView, EM_REPLACESEL, FALSE, (LPARAM)answer.c_str());

    // �Զ��������ײ�
    SendMessageA(_hAnswerView, WM_VSCROLL, SB_BOTTOM, 0);
}

void AiAssistWnd::clearConversation()
{
    if (!IsWindow(_hAnswerView))
        return;

    // ʹ��SETTEXTEX�ṹ�������
    SETTEXTEX st = {
        ST_DEFAULT,    // ��־λ
        1200           // ʹ��UTF-16����
    };

    // ����1��ֱ�����ÿ��ı���������ʽ��
    SendMessageW(_hAnswerView, EM_SETTEXTEX, (WPARAM)&st, (LPARAM)L"");

    // ����2��ͨ��ѡ��ȫ��ɾ���������ף�
    // SendMessageW(_hAnswerView, EM_SETSEL, 0, -1);   // ȫѡ
    // SendMessageW(_hAnswerView, EM_REPLACESEL, 0, (LPARAM)L""); // �滻Ϊ��

    // ��ѡ�����ù�����λ��
    SendMessageW(_hAnswerView, WM_VSCROLL, SB_TOP, 0);

    // ��ѡ�����Undo������
    SendMessageW(_hAnswerView, EM_EMPTYUNDOBUFFER, 0, 0);
}
void AiAssistWnd::handleUserInput()
{
    // ��ȡ�����ı�
    int len = GetWindowTextLengthA(_hInputEdit) + 1;
    std::string input(len, L'\0');
    GetWindowTextA(_hInputEdit, &input[0], len);
    input.resize(len - 1); // �Ƴ���β��null�ַ�

    if (!input.empty()) 
    {
        // TODO: ����AI�����߼�
        appendAnswer("Received: " + input);

        // ��������
        SetWindowText(_hInputEdit, L"");
    }
}