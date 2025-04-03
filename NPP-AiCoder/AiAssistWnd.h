// AiAssistkWnd.h
#pragma once
#include "DockingDlgInterface.h"
#include "PluginInterface.h"

class AiAssistWnd : public DockingDlgInterface {
public:
    AiAssistWnd(HINSTANCE hInst, const NppData& nppData);
    ~AiAssistWnd();

    // 必需实现的虚函数
    virtual void init();
    virtual INT_PTR run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;

    // 功能接口
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

    // 控件句柄
    HWND _hModelCombo = nullptr;
    HWND _hInputEdit = nullptr;
    HWND _hAnswerView = nullptr;

    // 配置参数
    const int CONTROL_MARGIN = 5;
    const int COMBO_HEIGHT = 25;
    const int INPUT_HEIGHT = 80;

    // 字体资源
    HFONT _hFont = nullptr;
    HFONT _hBoldFont = nullptr;

    // 控件ID定义
    enum ControlID {
        IDC_MODEL_COMBO = 2000,
        IDC_INPUT_EDIT,
        IDC_ANSWER_VIEW
    };
};