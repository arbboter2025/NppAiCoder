// PluginConfigDlg.cpp
#include "pch.h"
#include "PluginConfigDlg.h"
#include "Resource.h"
#include "Exui.h"

#pragma comment(lib, "ComCtl32.lib")
using namespace Scintilla;

void PluginConfigDlg::ShowConfigError(const std::string& err)
{
    MessageBoxA(GetHandle(), err.c_str(), "���ô���", MB_OK | MB_ICONERROR);
}

PluginConfigDlg::PluginConfigDlg(HINSTANCE hInstance, Scintilla::PluginConfig& plugConf) 
    : m_hInstance(hInstance), m_plugConfig(plugConf)
{
    // ������ģʽ�Ի���
    m_hDlg = CreateDialogParam(
        m_hInstance,
        MAKEINTRESOURCE(IDD_DIALOG_PLUG_CONFIG),
        nullptr,
        DlgProc,
        reinterpret_cast<LPARAM>(this)
    );
}

INT_PTR CALLBACK PluginConfigDlg::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_INITDIALOG) 
    {
        // ������ʵ��ָ�뵽����
        SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
    }

    // ��ȡ��ʵ��ָ��
    PluginConfigDlg* pThis = reinterpret_cast<PluginConfigDlg*>(
        GetWindowLongPtr(hDlg, GWLP_USERDATA)
        );

    if (pThis) 
    {
        return pThis->RealDlgProc(hDlg, uMsg, wParam, lParam);
    }
    return FALSE;
}


#define OnDlgItemEvent(nItemId, nEventId, fnCall) if(LOWORD(wParam) == nItemId && HIWORD(wParam) == nEventId) { fnCall(); return TRUE; }
INT_PTR PluginConfigDlg::RealDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
    // ��ʽ��������Ϣ
    std::string debugMsg = std::format(
        "RealDlgProc - HWND: {:p}, uMsg: 0x{:08X}, wParam: {:p}, lParam: {:p}\r\n",
        reinterpret_cast<void*>(hDlg),
        uMsg,
        reinterpret_cast<void*>(wParam),
        reinterpret_cast<void*>(lParam)
    );

    // ���������Ϣ
    OutputDebugStringA(debugMsg.c_str());

    switch (uMsg)
    {
    case WM_CLOSE:
        DestroyWindow(m_hDlg);
        return TRUE;
    case WM_INITDIALOG:
        m_hDlg = hDlg;
        if (m_hDlg)
        {
            InitControls();
            LoadConfig();
        }
        return TRUE;

    case WM_COMMAND:
        {
            auto nItemId = LOWORD(wParam);
            auto nEventId = HIWORD(wParam);
            OutputDebugStringA(String::Format("WM_COMMAND:%d %d\r\n", nItemId, nEventId).c_str());
            OnDlgItemEvent(IDC_COMBO_PLATFORM, CBN_SELCHANGE, OnPlatformChange);
            OnDlgItemEvent(IDC_COMBO_PROMT, CBN_SELCHANGE, OnPromtChange);
            OnDlgItemEvent(IDC_BUTTON_MODEL_SAVE, BN_CLICKED, OnSaveMode);
            OnDlgItemEvent(IDC_BUTTON_MODEL_DEL, BN_CLICKED, OnRemoveModel);
            OnDlgItemEvent(IDC_BUTTON_PROMT_SAVE, BN_CLICKED, OnSavePromt);
            OnDlgItemEvent(IDC_BUTTON_PROMT_DEL, BN_CLICKED, OnRemovePromt);
            OnDlgItemEvent(IDC_BUTTON_PLATFORM_SAVE, BN_CLICKED, OnSavePlatform);
            OnDlgItemEvent(IDC_BUTTON_PLATFORM_DEL, BN_CLICKED, OnRemovePlatform);
            OnDlgItemEvent(IDCANCEL, BN_CLICKED, LoadConfig);
            OnDlgItemEvent(IDOK, BN_CLICKED, SaveConfig);
            OnDlgItemEvent(ID_CLOSE, BN_CLICKED, [this]() { DestroyWindow(m_hDlg); return true; });
            OnDlgItemEvent(ID_CONF_ENDPOINT_CREATE, LVNI_ALL, OnEndpointListViewCreate);
            OnDlgItemEvent(ID_CONF_ENDPOINT_MODIFY, LVNI_ALL, OnEndpointListViewModify);
            OnDlgItemEvent(ID_CONF_ENDPOINT_DELETE, LVNI_ALL, OnEndpointListViewDelete);
        }
        break;
    case WM_NOTIFY:
        {
            LPNMITEMACTIVATE pNmItem = (LPNMITEMACTIVATE)lParam;
            UINT nCode = pNmItem->hdr.code;
            if (pNmItem->hdr.idFrom == IDC_LIST_ENDPOINT && nCode == NM_DBLCLK) 
            {
                OnEndpointListViewDBClick(pNmItem);
                return TRUE;
            }
            if (pNmItem->hdr.idFrom == IDC_LIST_ENDPOINT && nCode == NM_RCLICK) 
            {
                OnEndpointListViewRClick();
                return TRUE;
            }
        }
        break;
    }
    return FALSE;
}

void PluginConfigDlg::InitControls() 
{
    // ��Ȩ����
    HWND hWnd = GetDlgItem(m_hDlg, IDC_COMBO_AUTH_TYPE);
    SendMessage(hWnd, CB_RESETCONTENT, 0, 0);
    SendMessageW(hWnd, CB_ADDSTRING, 0, (LPARAM)L"��");
    SendMessageW(hWnd, CB_ADDSTRING, 0, (LPARAM)L"Basic");
    SendMessageW(hWnd, CB_ADDSTRING, 0, (LPARAM)L"Bearer");
    SendMessageW(hWnd, CB_ADDSTRING, 0, (LPARAM)L"ApiKey");

    // �ӿ��б�
    HWND hList = GetDlgItem(m_hDlg, IDC_LIST_ENDPOINT);

    // 1. ���û�����ʽ��������� LVS_REPORT��
    SetWindowLongW(hList, GWL_STYLE, 
        GetWindowLongW(hList, GWL_STYLE) | 
        LVS_REPORT |      // ������ͼ
        LVS_SINGLESEL     // ��ֹ��ѡ
    );

    // 2. ������չ��ʽ
    ListView_SetExtendedListViewStyle(hList, 
        LVS_EX_GRIDLINES |    // ��ʾ������
        LVS_EX_FULLROWSELECT  // ����ѡ��
    );

    // 3. ��ʼ����ͷ
    LVCOLUMNW lvc = {0};
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
    // ���������
    const struct 
    {
        int width;
        const wchar_t* title;
    } columns[] = 
    {
        {50, L"����"},
        {50, L"����"},
        {300, L"�ӿ�"},
        {150, L"����"}
    };
    for (size_t i = 0; i < _countof(columns); ++i) 
    {
        lvc.fmt = LVCFMT_CENTER;
        lvc.cx = columns[i].width;
        lvc.pszText = const_cast<LPWSTR>(columns[i].title);
        ListView_InsertColumn(hList, i, &lvc);
    }
}

void PluginConfigDlg::OnPlatformChange()
{
    std::string name;
    if (!GetComboSelectedText(GetDlgItem(m_hDlg, IDC_COMBO_PLATFORM), name) || name.empty())
    {
        return;
    }
    auto e = m_plugConfig.platforms.find(name);
    if (e == m_plugConfig.platforms.end())
    {
        return;
    }
    Load(e->second);
}

void PluginConfigDlg::Load(const Scintilla::PlatformConfig& platform)
{
    // SSL
    CheckDlgButton(m_hDlg, IDC_CHECK_SSL, platform.enable_ssl ? BST_CHECKED : BST_UNCHECKED);

    // ��Ȩ����
    HWND hWnd = GetDlgItem(m_hDlg, IDC_COMBO_AUTH_TYPE);
    SendMessage(hWnd, CB_SETCURSEL, (WPARAM)(int)platform.authorization.eAuthType, 0);

    // ��Ȩ����
    SetDlgItemTextA(m_hDlg, IDC_EDIT_AUTH_DATA, platform.authorization.auth_data.c_str());

    // ����ַ
    SetDlgItemTextA(m_hDlg, IDC_EDIT_ROOT_URL, platform.base_url.c_str());

    // ģ������
    int nSel = 0;
    int nIdx = 0;
    hWnd = GetDlgItem(m_hDlg, IDC_COMBO_MODEL_NAME);
    SendMessage(hWnd, CB_RESETCONTENT, 0, 0);
    for (auto& m : platform.models)
    {
        SendMessageA(hWnd, CB_ADDSTRING, 0, (LPARAM)m.c_str());
        if (m == platform.model_name)
        {
            nSel = nIdx;
        }
        nIdx++;
    }
    SendMessage(hWnd, CB_SETCURSEL, (WPARAM)nSel, 0);

    hWnd = GetDlgItem(m_hDlg, IDC_LIST_ENDPOINT);
    ListView_DeleteAllItems(hWnd);

    if (!platform.base_url.empty())
    {
        auto pE = &platform.chat_endpoint;
        ListViewAddRow(hWnd, { "�Ի�", pE->method, pE->api, pE->prompt});
        pE = &platform.generate_endpoint;
        ListViewAddRow(hWnd, { "����", pE->method, pE->api, pE->prompt});
        pE = &platform.models_endpoint;
        ListViewAddRow(hWnd, { "ģ��", pE->method, pE->api, pE->prompt});
    }
}
int PluginConfigDlg::ListViewSetRow(HWND hList, int nRow, const std::vector<std::string>& fields)
{
    // ����У��
    if (!hList || !IsWindow(hList) || fields.empty() || nRow < 0 || nRow >= ListView_GetItemCount(hList))
    {
        return -1;
    }

    // �������
    for (size_t col = 0; col < fields.size(); ++col) 
    {
        auto ws = Scintilla::String::string2w(fields[col], false);
        ListView_SetItemText(
            hList, 
            nRow, 
            static_cast<int>(col),
            const_cast<LPWSTR>(ws.c_str())
        );
    }
    return nRow;
}

int PluginConfigDlg::ListViewAddRow(HWND hList, const std::vector<std::string>& fields) 
{
    // ����У��
    if (!hList || !IsWindow(hList) || fields.empty())
    {
        return -1;
    }

    auto ws = Scintilla::String::string2w(fields[0], false);
    // ���������һ�У�
    LVITEMW lvi = {0};
    lvi.mask = LVIF_TEXT;
    lvi.iItem = ListView_GetItemCount(hList);
    lvi.iSubItem = 0;
    lvi.pszText = const_cast<LPWSTR>(ws.c_str());
    const int rowIndex = ListView_InsertItem(hList, &lvi);
    return ListViewSetRow(hList, rowIndex, fields);
}

bool PluginConfigDlg::ListViewGetEndpoint(HWND hList, int nRow, Scintilla::EndpointConfig& ep)
{
    if (!hList || !IsWindow(hList))
    {
        return false;
    }
    std::vector<std::string> fields;
    if (ListViewGetRow(hList, nRow, fields) < 4)
    {
        return false;
    }
    ep.method = fields[1];
    ep.api = fields[2];
    ep.prompt = fields[3];
    return true;
}

bool PluginConfigDlg::ListViewGetEndpoint(HWND hList, Scintilla::PlatformConfig& platform)
{
    if (!hList || !IsWindow(hList))
    {
        return false;
    }
    Scintilla::EndpointConfig* pEnd = nullptr;
    std::vector<std::string> fields;
    const int nCount = ListView_GetItemCount(hList);
    for (int i = 0; i < nCount; i++)
    {
        if (ListViewGetRow(hList, i, fields) < 4)
        {
            return false;
        }
        pEnd = nullptr;
        if (fields[0] == "�Ի�")
        {
            pEnd = &platform.chat_endpoint;
        }
        else if (fields[0] == "����")
        {
            pEnd = &platform.generate_endpoint;
        }
        else if (fields[0] == "ģ��")
        {
            pEnd = &platform.models_endpoint;
        }
        if (pEnd)
        {
            pEnd->method = fields[1];
            pEnd->api = fields[2];
            pEnd->prompt = fields[3];
        }
    }
    return true;
}


bool PluginConfigDlg::ListViewCurCell(HWND hList, int& nRow, int& nCol)
{
    // ��֤�ؼ���Ч��
    if (!IsWindow(hList) || !ListView_GetItemCount(hList)) 
    {
        return false;
    }

    // ��ȡ�����Ļ���겢ת��Ϊ�ͻ�������
    POINT ptScreen;
    GetCursorPos(&ptScreen);
    ScreenToClient(hList, &ptScreen);

    // ִ�����в���
    LVHITTESTINFO hitTestInfo = {0};
    hitTestInfo.pt = ptScreen;
    nRow = ListView_HitTest(hList, &hitTestInfo);

    // �������
    if (nRow == -1 || !(hitTestInfo.flags & LVHT_ONITEM)) 
    {
        return false;
    }

    // ��ȡ������
    LVITEM lvItem = {0};
    lvItem.mask = LVIF_PARAM;
    lvItem.iItem = nRow;
    lvItem.iSubItem = 0;
    if (!ListView_GetItem(hList, &lvItem)) 
    {
        return false;
    }

    // ��ȡ��ǰ�У�������������жϣ�
    nCol = 0;
    RECT rcItem;
    for (int i = 0; i < Header_GetItemCount(ListView_GetHeader(hList)); ++i) 
    {
        ListView_GetSubItemRect(hList, nRow, i, LVIR_BOUNDS, &rcItem);
        if (PtInRect(&rcItem, ptScreen)) 
        {
            nCol = i;
            break;
        }
    }
    return true;
}

std::string PluginConfigDlg::GetComboSelectedText(int nItemId)
{
    HWND hCombo = GetDlgItem(m_hDlg, nItemId);
    std::string text;
    GetComboSelectedText(hCombo, text);
    return text;
}

bool PluginConfigDlg::GetComboSelectedText(HWND hCombo, std::string& text)
{
    if (!hCombo || !IsWindow(hCombo)) 
    {
        return false;
    }

    LRESULT index = SendMessageW(hCombo, CB_GETCURSEL, 0, 0);
    if (index == CB_ERR) 
    {
        // ����༭�����ͣ��� CBS_DROPDOWN��
        int len = GetWindowTextLengthW(hCombo) + 1;
        wchar_t* buffer = new wchar_t[len];
        GetWindowTextW(hCombo, buffer, len);
        std::wstring wtext(buffer);
        delete[] buffer;
        text = String::wstring2s(wtext, false);
        return true;
    }

    // ��ȡ�����б��е��ı�
    int len = (int)SendMessageW(hCombo, CB_GETLBTEXTLEN, index, 0);
    wchar_t* buffer = new wchar_t[(size_t)len + 1];
    SendMessageW(hCombo, CB_GETLBTEXT, index, (LPARAM)buffer);
    std::wstring wtext(buffer);
    delete[] buffer;
    text = String::wstring2s(wtext, false);
    return true;
}

int PluginConfigDlg::ListViewGetRow(HWND hList, int nRow, std::vector<std::string>& fields)
{
    // ����У��
    if (!hList || !IsWindow(hList))
    {
        return -1;
    }
    int nTotal = ListView_GetItemCount(hList);
    if (nRow < 0 || nRow >= nTotal)
    {
        return -1;
    }
    const int nCols = ListViewGetColumnCount(hList);
    // ����������
    for (int nCol = 0; nCol < nCols; ++nCol) 
    {
        LVITEMW lvi = { 0 };
        wchar_t szBuffer[1024] = { 0 };

        lvi.mask = LVIF_TEXT;
        lvi.iItem = nRow;
        lvi.iSubItem = nCol;
        lvi.pszText = szBuffer;
        lvi.cchTextMax = sizeof(szBuffer);

        // ��ȡ��ǰ��Ԫ������
        if (ListView_GetItem(hList, &lvi) && lvi.pszText[0] != L'\0')
        {
            fields.push_back(String::wstring2s(lvi.pszText, false));
        } 
        else 
        {
            fields.push_back("");
        }
    }
    return (int)fields.size();
}

int PluginConfigDlg::ListViewGetRow(HWND hList, int nRow, std::map<std::string, std::string>& fields)
{
    // ����У��
    if (!hList || !IsWindow(hList)) return -1;
    const int nTotal = ListView_GetItemCount(hList);
    if (nRow < 0 || nRow >= nTotal) return -1;

    // ��ȡ����
    HWND hHeader = ListView_GetHeader(hList);
    const int nCols = Header_GetItemCount(hHeader);
    fields.clear();
    for (int nCol = 0; nCol < nCols; ++nCol)
    {
        // ��ȡ�б���
        LVCOLUMNW lvCol = {0};
        wchar_t szColName[256] = {0};
        lvCol.mask = LVCF_TEXT;
        lvCol.pszText = szColName;
        lvCol.cchTextMax = _countof(szColName);

        if (!ListView_GetColumn(hList, nCol, &lvCol)) 
        {
            continue;
        }

        // ��ȡ��Ԫ������
        wchar_t szValue[1024] = {0};
        ListView_GetItemText(hList, nRow, nCol, szValue, _countof(szValue));

        // ת�����벢�����ֵ�
        fields[String::wstring2s(lvCol.pszText, false)] = String::wstring2s(szValue, false);
    }
    return (int)fields.size();
}

bool PluginConfigDlg::LoadConfig()
{
    SetDlgItemTextA(m_hDlg, IDC_EDIT_TIMEOUT, std::to_string(m_plugConfig.timeout).c_str());
    int nSel = 0;
    int nIdx = 0;
    HWND hWnd = GetDlgItem(m_hDlg, IDC_COMBO_PROMT);
    SendMessage(hWnd, CB_RESETCONTENT, 0, 0);
    for (auto& e : m_plugConfig.promt.exts)
    {
        auto g = String::UTF8ToGBK(e.first.c_str());
        SendMessageA(hWnd, CB_ADDSTRING, 0, (LPARAM)g.c_str());
        if (nIdx++ == 0)
        {
            SendMessage(hWnd, CB_SETCURSEL, (WPARAM)0, 0);
            g = String::UTF8ToGBK(e.second.c_str());
            SetDlgItemTextA(m_hDlg, IDC_RICHEDIT_PROMT, g.c_str());
        }
    }

    hWnd = GetDlgItem(m_hDlg, IDC_COMBO_PLATFORM);
    SendMessage(hWnd, CB_RESETCONTENT, 0, 0);
    nIdx = 0;
    for (auto& p : m_plugConfig.platforms)
    {
        SendMessageA(hWnd, CB_ADDSTRING, 0, (LPARAM)p.first.c_str());
        if (!Scintilla::String::icasecompare(p.first, m_plugConfig.platform))
        {
            nSel = nIdx;
        }
        nIdx++;
    }
    SendMessage(hWnd, CB_SETCURSEL, (WPARAM)nSel, 0);

    if (m_plugConfig.platforms.empty()) return true;
    auto& platform = m_plugConfig.Platform();
    Load(platform);
    return true;
}

bool PluginConfigDlg::SaveConfig()
{
    if (!OnSavePlatform())
    {
        return false;
    }
    std::string name;
    auto p = GetCurSelPlatform(name);
    if (p == nullptr)
    {
        ShowConfigError("�������󣬻�ȡ��ǰƽ̨Ϊ��");
        return false;
    }
    if (!m_plugConfig.Save(""))
    {
        return false;
    }
    m_plugConfig.timeout = atoi(GetItemText(IDC_EDIT_TIMEOUT).c_str());
    m_plugConfig.platform = name;
    if (fnOnConfigChange) fnOnConfigChange();
    return true;
}

void PluginConfigDlg::Show(bool bShow) 
{
    Ui::Util::Show(m_hDlg, bShow);
}

bool PluginConfigDlg::OnSavePlatform()
{
    HWND hPlatform = GetDlgItem(m_hDlg, IDC_COMBO_PLATFORM);
    if (hPlatform == nullptr)
    {
        return false;
    }

    std::string name;
    auto pPlat = GetCurSelPlatform(name);
    if (name.empty())
    {
        ShowConfigError("�������ѡ��ǰƽ̨����");
        return false;
    }

    Scintilla::PlatformConfig plat;
    if (!Save(plat))
    {
        return false;
    }
    if (pPlat == nullptr)
    {
        // ����
        SendMessageA(hPlatform, CB_ADDSTRING, 0, (LPARAM)name.c_str());
        m_plugConfig.platforms[name] = plat;
    }
    else
    {
        *pPlat = plat;
    }
    return true;
}

void PluginConfigDlg::OnRemovePlatform()
{
    HWND hPlatform = GetDlgItem(m_hDlg, IDC_COMBO_PLATFORM);
    if (hPlatform == nullptr)
    {
        return;
    }

    std::string name;
    auto pPlat = GetCurSelPlatform(name);
    if (!pPlat)
    {
        ShowConfigError("��ѡ��ǰƽ̨����");
        return;
    }
    auto nRet = ShowMsgBox(String::Format("�Ƿ�Ҫɾ��ƽ̨��%s������?", name.c_str()), "ɾ��ȷ��", MB_YESNO | MB_ICONQUESTION);
    if (nRet != IDYES)
    {
        return;
    }
    SendMessageA(hPlatform, CB_DELETESTRING, 0, (LPARAM)name.c_str());
    m_plugConfig.platforms.erase(name);
    // �������
    Load(PlatformConfig());
}

void PluginConfigDlg::OnSaveMode()
{
    HWND hCombo = GetDlgItem(m_hDlg, IDC_COMBO_MODEL_NAME);
    if (hCombo == nullptr)
    {
        return;
    }
    auto name = String::Trim(GetComboSelectedText(IDC_COMBO_MODEL_NAME));
    if (name.empty())
    {
        ShowConfigError("�������ѡ��ǰģ������");
        return;
    }
    SendMessageA(hCombo, CB_ADDSTRING, 0, (LPARAM)name.c_str());
    Ui::Util::ComboSelClear(hCombo);
}

void PluginConfigDlg::OnRemoveModel()
{
    HWND hCombo = GetDlgItem(m_hDlg, IDC_COMBO_MODEL_NAME);
    if (hCombo == nullptr) return;
    Ui::Util::ComboRemove(hCombo);
}

void PluginConfigDlg::OnEndpointListViewDBClick(LPNMITEMACTIVATE& pNmItem)
{
    if (pNmItem == nullptr)
    {
        return;
    }
    int nRow = pNmItem->iItem;
    if (nRow < 0)
    {
        return;
    }
    SetEndpointListView(nRow);
}

void PluginConfigDlg::OnEndpointListViewRClick()
{
    // ��ȡ�����Ļ����
    POINT pt;
    GetCursorPos(&pt);

    // ���ز˵�����ʾ
    HMENU hMenu = LoadMenu(m_hInstance, MAKEINTRESOURCE(IDR_MENU_CONFIG));
    HMENU hSubMenu = GetSubMenu(hMenu, IDC_MENU_CONFIG_ENDPOINT);
    TrackPopupMenu(hSubMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON, pt.x, pt.y, 0, m_hDlg, NULL);
}

void PluginConfigDlg::OnPromtChange()
{
    auto hCombo = GetDlgItem(m_hDlg, IDC_COMBO_PROMT);
    std::string name;
    if (!GetComboSelectedText(hCombo, name) || name.empty())
    {
        return;
    }
    name = String::GBKToUTF8(name.c_str());
    auto e = m_plugConfig.promt.exts.find(name);
    if (e == m_plugConfig.promt.exts.end())
    {
        return;
    }
    SetWindowTextA(GetDlgItem(m_hDlg, IDC_RICHEDIT_PROMT), String::UTF8ToGBK(e->second.c_str()).c_str());
}

bool PluginConfigDlg::SetEndpointListView(int nRow)
{
    HWND hList = GetDlgItem(m_hDlg, IDC_LIST_ENDPOINT);
    std::vector<std::string> fields;
    std::vector<std::string> headers;
    Ui::Util::ListViewHeader(hList, headers);
    if (headers.empty())
    {
        return false;
    }
    if (nRow >= 0)
    {
        if (ListViewGetRow(hList, nRow, fields) <= 0)
        {
            return false;
        }
    }
    else
    {
        fields.resize(10);
    }
    FieldEditDlg dlg(m_hInstance, m_hDlg);
    for(size_t i=0; i<headers.size(); i++)
    {
        dlg.m_fields.push_back({ headers[i], fields[i] });
        auto& f = dlg.m_fields[dlg.m_fields.size() - 1];
        if (f.name == "����")
        {
            f.options = { "post", "get" };
            f.type = FieldEditDlg::FieldType::Combo;
        }
        else if (f.name == "����")
        {
            f.options = { "�Ի�", "����", "ģ��", "�Զ���" };
            f.type = FieldEditDlg::FieldType::Combo;
            int ndis = (int)std::distance(f.options.begin(), std::find(f.options.begin(), f.options.end(), f.val));
            f.readonly = (ndis >= 0 && ndis <= 2);
        }
    }
    if (dlg.DoModal() != IDOK)
    {
        return false;
    }
    for (size_t i=0; i<dlg.m_fields.size(); i++)
    {
        fields[i] = dlg.m_fields[i].val;
    }
    if (nRow >= 0)
    {
        ListViewSetRow(hList, nRow, fields);
    }
    else
    {
        ListViewAddRow(hList, fields);
    }
    return true;
}

void PluginConfigDlg::OnEndpointListViewCreate()
{
    SetEndpointListView(-1);
}

void PluginConfigDlg::OnEndpointListViewModify()
{
    HWND hList = GetDlgItem(m_hDlg, IDC_LIST_ENDPOINT);
    int nRow = Ui::Util::ListViewGetSelRow(hList);
    if (nRow < 0) return;
    SetEndpointListView(nRow);
}

void PluginConfigDlg::OnEndpointListViewDelete()
{
    HWND hList = GetDlgItem(m_hDlg, IDC_LIST_ENDPOINT);
    int nRow = Ui::Util::ListViewGetSelRow(hList);
    if (nRow < 3) return;
    ListView_DeleteItem(hList, nRow);
}

void PluginConfigDlg::OnSavePromt()
{
    OnSetPromt(true);
}

void PluginConfigDlg::OnRemovePromt()
{
    OnSetPromt(false);
}

void PluginConfigDlg::OnSetPromt(bool bSave)
{
    auto hCombo = GetDlgItem(m_hDlg, IDC_COMBO_PROMT);
    auto name = Ui::Util::GetText(hCombo);
    if (name.empty())
    {
        return;
    }
    auto utf8Name = String::GBKToUTF8(name.c_str());
    auto hText = GetDlgItem(m_hDlg, IDC_RICHEDIT_PROMT);
    auto e = m_plugConfig.promt.exts.find(utf8Name);
    bool bExist = (e != m_plugConfig.promt.exts.end());
    if(bSave)
    {
        auto text = Ui::Util::GetText(hText);
        if (text.empty())
        {
            ShowConfigError("������ʾ��ģ��ʱ����д��ʾ�����ݣ�������");
            return;
        }
        m_plugConfig.promt.exts[utf8Name] = String::GBKToUTF8(text.c_str());
        if (!bExist)
        {
            SendMessageA(hCombo, CB_ADDSTRING, 0, (LPARAM)name.c_str());
        }
        Ui::Util::ComboSelClear(hCombo);
        SetWindowText(hText, L"");
    }
    else
    {
        if(bExist)
        {
            m_plugConfig.promt.exts.erase(e);
        }
        Ui::Util::ComboRemove(hCombo);
        SetWindowText(hText, L"");
    }
}

std::string PluginConfigDlg::GetItemText(int nItemId)
{
    HWND hItem = GetDlgItem(m_hDlg, nItemId);
    if (hItem == nullptr)
    {
        return "";
    }
    int nLen = GetWindowTextLengthA(hItem);
    if (nLen <= 0)
    {
        return "";
    }
    char* pBuf = new char[(size_t)nLen + 1];
    GetDlgItemTextA(m_hDlg, nItemId, pBuf, nLen + 1);
    pBuf[nLen] = '\0';
    
    std::string text(pBuf);
    delete[] pBuf;
    return text;
}

void PluginConfigDlg::GetComboData(int nItemId, std::vector<std::string>& datas)
{
    // ��ȡ��Ͽ���
    HWND hCombo = GetDlgItem(m_hDlg, nItemId);
    if (hCombo == nullptr)
    {
        return;
    }
    int itemCount = (int) SendMessage(hCombo, CB_GETCOUNT, 0, 0);
    if (itemCount <= 0) 
    {
        return;
    }
    for (int i = 0; i < itemCount; i++) 
    {
        LRESULT nLen = SendMessage(hCombo, CB_GETLBTEXTLEN, i, 0);
        if (nLen == CB_ERR) 
        {
            continue;
        }

        char* buf = new char[nLen + 1];
        LRESULT result = SendMessageA(hCombo, CB_GETLBTEXT, i, (LPARAM)buf);
        if (result != CB_ERR) 
        {
            buf[nLen] = '\0';
            datas.push_back(buf);
        }
        delete[] buf;
    }
}

bool PluginConfigDlg::Save(Scintilla::PlatformConfig& platform)
{
    auto p = platform;

    p.enable_ssl = IsDlgButtonChecked(m_hDlg, IDC_CHECK_SSL) == BST_CHECKED;
    p.authorization.eAuthType = Scintilla::AuthorizationConf::GetAuthType(GetComboSelectedText(IDC_COMBO_AUTH_TYPE));
    p.authorization.auth_data = GetItemText(IDC_EDIT_AUTH_DATA);
    if (p.authorization.eAuthType != Scintilla::AuthorizationConf::AuthType::None
        && Scintilla::String::Trim(p.authorization.auth_data).empty())
    {
        ShowConfigError("������Ϸ�����֤���ݣ���ʽ�ο�:\r\n1.Base\t�û���:����\r\n2.Bearer\t��Կ��\r\n3.ApiKey\t��Կ����:��Կֵ");
        return false;
    }
    p.base_url = GetItemText(IDC_EDIT_ROOT_URL);
    if (Scintilla::String::Trim(p.base_url).empty())
    {
        ShowConfigError("������Ϸ���ƽ̨����ַ��ע��ֻ��Ҫ��������:www.ai.com");
        return false;
    }
    if (!GetComboSelectedText(GetDlgItem(m_hDlg, IDC_COMBO_MODEL_NAME), p.model_name) || p.model_name.empty())
    {
        ShowConfigError("��ѡ�������ģ������");
        return false;
    }
    p.models.clear();
    GetComboData(IDC_COMBO_MODEL_NAME, p.models);

    HWND hList = GetDlgItem(m_hDlg, IDC_LIST_ENDPOINT);
    if (!ListViewGetEndpoint(hList, p))
    {
        return false;
    }
    if (p.chat_endpoint.api.empty())
    {
        ShowConfigError("����������Ϊ���Ի����Ľӿڼ����ַ");
        return false;
    }

    platform = p;
    return true;
}

int PluginConfigDlg::ListViewGetColumnCount(HWND hList)
{
    HWND hHeader = ListView_GetHeader(hList);
    return (hHeader) ? Header_GetItemCount(hHeader) : 0;
}

Scintilla::PlatformConfig* PluginConfigDlg::GetCurSelPlatform(std::string& name)
{
    HWND hPlatform = GetDlgItem(m_hDlg, IDC_COMBO_PLATFORM);
    if (hPlatform == nullptr)
    {
        return nullptr;
    }

    if (!GetComboSelectedText(hPlatform, name) || name.empty())
    {
        return nullptr;
    }

    auto e = m_plugConfig.platforms.find(name);
    if (e == m_plugConfig.platforms.end())
    {
        return nullptr;
    }
    return &e->second;
}

void PluginConfigDlg::EnableAutoHscroll(int nItemId)
{
    HWND hCtrl = GetDlgItem(m_hDlg, nItemId);
    if (hCtrl == nullptr) 
    {
        return;
    }
    // ��� CBS_AUTOHSCROLL ��ʽ
    LONG_PTR style = GetWindowLongPtr(hCtrl, GWL_STYLE);
    SetWindowLongPtr(hCtrl, GWL_STYLE, style | CBS_AUTOHSCROLL);

    // ǿ���ػ�ؼ���Ӧ������ʽ
    SetWindowPos(hCtrl, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}