#pragma once
#include "PluginInterface.h"
#include "ScintillaTypes.h"
#include "ScintillaStructures.h"
#include "ScintillaMessages.h"
#include "ScintillaCall.h"
#include "Utils.h"
#include <string>
#include <memory>

class NppImp
{
public:
    using FnRunUiTask = std::function<void(const std::string&)>;
    using FnRunAiRequest = std::function<void(const std::string&, const std::string&)>;

    NppImp(NppData& npp) : _npp(npp) {}
    std::shared_ptr<Scintilla::ScintillaCall> SciCall();
    
    void RunUiTask(FnRunUiTask fnTask, const std::string& text);
    void RunAiRequest(FnRunAiRequest fnTask, const std::string& name, const std::string& para);
    void NewLineAfterSelText();
    std::string GetSelText(bool bNewLine = false, bool doc_if_empty = false);
    std::string GetDocText();
private:
    NppData _npp;
};