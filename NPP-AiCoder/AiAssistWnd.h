// AiAssistkWnd.h
#pragma once
#include "DockingDlgInterface.h"
#include "PluginInterface.h"
#include "PluginConf.h"
#include <functional>

class AiAssistWnd : public DockingDlgInterface 
{
public:
    using FnOnTextChange = std::function<void(const std::string&)>;
    using FnOnDealText = std::function<void(const std::string&, HWND)>;

    // �ص�����
    FnOnTextChange fnOnModelSelChange = nullptr;
    FnOnTextChange fnOnInputFinished = nullptr;
    FnOnTextChange fnOnOutputFinished = nullptr;
public:
    AiAssistWnd(HINSTANCE hInst, const NppData& nppData, const Scintilla::PluginConfig& plugConf);
    ~AiAssistWnd();

    // ʵ�ֵ��麯��
    void init();
    void display(bool toShow = true);

    // ���ܽӿ�
    void appendAnswer(const std::string& answer, bool bNewLine = true);
    void appendNotify(const std::string& notify) { appendAnswer("����ʾ��" + notify); }
    void clearConversation();

public:
    bool OnInputFinished();
    void OnOutputFinished(const std::string& end);
    void SetTaskStatus(int stat);
    void ReloadConfig();

protected:
    virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;

private:
    void OnModelComboSelChange();
    void SetTitle(const char* title = nullptr);

private:
    void initControls();
    void layoutControls(int width, int height);
    void gotoEnd(HWND hWnd);
    void updatelList(HWND hCombo, const std::vector<std::string>& options, int sel = -1, const char* seltext = nullptr);

    // Npp
    HINSTANCE _hInst;
    NppData _nppData;
    const Scintilla::PluginConfig& _plugConf;

    // �ؼ����
    HWND _hModelCombo = nullptr;
    HWND _hPromtCombo = nullptr;
    HWND _hInputEdit = nullptr;
    HWND _hAnswerView = nullptr;
    HWND _hActionBtn = nullptr;
    HICON _hSendIcon = nullptr;
    HICON _hStopIcon = nullptr;

    // ���ò���
    const int CONTROL_MARGIN = 5;
    const int COMBO_HEIGHT = 25;
    const int INPUT_HEIGHT = 80;

    // �ؼ�ID����
    enum ControlID {
        IDC_MODEL_COMBO = 2000,
        IDC_INPUT_EDIT,
        IDC_ANSWER_VIEW,
        IDC_ACTION_BUTTON,
        IDC_PROMT_COMBO
    };
};