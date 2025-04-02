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
    NppImp(NppData& npp) : _npp(npp) {}
    std::shared_ptr<Scintilla::ScintillaCall> SciCall();
    std::string GetSelText();
private:
    NppData _npp;
};

