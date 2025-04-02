#include "pch.h"
#include "NppImp.h"
#include "Utils.h"

std::shared_ptr<Scintilla::ScintillaCall> NppImp::SciCall()
{
    CHECK_OR_RETURN(_npp._scintillaMainHandle, nullptr);
    auto pCall = std::shared_ptr<Scintilla::ScintillaCall>(new Scintilla::ScintillaCall());
    pCall->SetFnPtr((intptr_t)_npp._scintillaMainHandle);
    return pCall;
}

std::string NppImp::GetSelText()
{
    CHECK_OR_RETURN(_npp._scintillaMainHandle, "");

    // 获取当前文本内容
    auto pCall = SciCall();
    auto text = pCall->GetSelText();
    auto code_page = pCall->CodePage();
    if (code_page > 0 && code_page != CP_ACP)
    {
        text = Scintilla::String::ConvEncoding(text.c_str(), text.size(), code_page, CP_ACP);
    }
    return text;
}