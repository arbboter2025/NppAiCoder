// AiAssistkWnd.h
#pragma once
#include "DockingDlgInterface.h"
#include "PluginInterface.h"

class AiAssistWnd : public DockingDlgInterface {
public:
    AiAssistWnd(HINSTANCE hInst, const NppData& nppData);
    ~AiAssistWnd();

    // ����ʵ�ֵ��麯��
    virtual void init();
    virtual INT_PTR run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;

    // ���ܽӿ�
    void updateModelList(const std::vector<std::string>& models);
    void appendAnswer(const std::string& answer);
    void clearConversation();

    // void display(bool toShow = true) const override {
    //     DockingDlgInterface::display(toShow);
    // }

private:
    void initControls();
    void layoutControls(int width, int height);
    void handleUserInput();

    // Npp
    HINSTANCE _hInst;
    NppData _nppData;

    // �ؼ����
    HWND _hModelCombo = nullptr;
    HWND _hInputEdit = nullptr;
    HWND _hAnswerView = nullptr;

    // ���ò���
    const int CONTROL_MARGIN = 5;
    const int COMBO_HEIGHT = 25;
    const int INPUT_HEIGHT = 80;

    // ������Դ
    HFONT _hFont = nullptr;
    HFONT _hBoldFont = nullptr;

    // �ؼ�ID����
    enum ControlID {
        IDC_MODEL_COMBO = 2000,
        IDC_INPUT_EDIT,
        IDC_ANSWER_VIEW
    };
};