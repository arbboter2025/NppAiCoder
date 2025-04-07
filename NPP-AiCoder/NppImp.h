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

    NppImp(NppData& npp) : _npp(npp) {}
    std::shared_ptr<Scintilla::ScintillaCall> SciCall();
    
    void RunUiTask(FnRunUiTask fnTask, const std::string& text);
    void NewLineAfterSelText();
    std::string GetSelText(bool bNewLine = false);
private:
    NppData _npp;
};

