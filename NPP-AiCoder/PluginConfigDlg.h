// PluginConfigDlg.h
#pragma once
#include <windows.h>
#include "PluginConf.h"
#include <commctrl.h>

class PluginConfigDlg 
{
public:
    PluginConfigDlg(HINSTANCE hInstance, Scintilla::PluginConfig& plugConf);
    ~PluginConfigDlg() {}

    void Show(bool bShow = true);
    HWND GetHandle() const { return m_hDlg; }

public:
    using FnOnConfigChange = std::function<void()>;
    FnOnConfigChange fnOnConfigChange = nullptr;

private:
    static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    INT_PTR RealDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

    void InitControls();
    bool LoadConfig();
    bool SaveConfig();
    void OnPlatformChange();
    void OnPromtChange();
    bool OnSavePlatform();
    void OnRemovePlatform();
    void OnSaveMode();
    void OnRemoveModel();
    void OnEndpointListViewDBClick(LPNMITEMACTIVATE& pNmItem);
    void OnEndpointListViewRClick();
    void OnEndpointListViewCreate();
    void OnEndpointListViewModify();
    void OnEndpointListViewDelete();
    void OnSavePromt();
    void OnRemovePromt();
    void OnSetPromt(bool bSave);

    // UIº¯Êý
    std::string GetComboSelectedText(int nItemId);
    std::string GetItemText(int nItemId);
    void GetComboData(int nItemId, std::vector<std::string>& datas);
    int  ListViewSetRow(HWND hList, int nRow, const std::vector<std::string>& fields);
    int  ListViewAddRow(HWND hList, const std::vector<std::string>& fields);
    int  ListViewGetRow(HWND hList, int nRow, std::vector<std::string>& fields);
    int  ListViewGetRow(HWND hList, int nRow, std::map<std::string, std::string>& fields);
    int  ListViewGetColumnCount(HWND hList);
    bool ListViewGetEndpoint(HWND hList, int nRow, Scintilla::EndpointConfig& ep);
    bool ListViewGetEndpoint(HWND hList, Scintilla::PlatformConfig& platform);
    bool ListViewCurCell(HWND hList, int& nRow, int& nCol);
    bool GetComboSelectedText(HWND hCombo, std::string& text);
    bool SetEndpointListView(int nRow);
    void EnableAutoHscroll(int nItemId);

    // ½çÃæ
    void ShowConfigError(const std::string& err);
    void Load(const Scintilla::PlatformConfig& platform);
    bool Save(Scintilla::PlatformConfig& platform);
    Scintilla::PlatformConfig* GetCurSelPlatform(std::string& name);

    HWND m_hDlg = nullptr;
    HINSTANCE m_hInstance = nullptr;
    Scintilla::PluginConfig& m_plugConfig;
};