#include "pch.h"
#include "NppImp.h"
#include "Utils.h"
#include <thread>


std::shared_ptr<Scintilla::ScintillaCall> NppImp::SciCall()
{
    CHECK_OR_RETURN(_npp._scintillaMainHandle, nullptr);
    auto pCall = std::shared_ptr<Scintilla::ScintillaCall>(new Scintilla::ScintillaCall());
    pCall->SetFnPtr((intptr_t)_npp._scintillaMainHandle);
    return pCall;
}

void NppImp::RunUiTask(FnRunUiTask fnTask, const std::string& text)
{
    // �������������߳�
    std::shared_ptr<char[]> pText(new char[text.size()]);
    memcpy(pText.get(), text.c_str(), text.size());
    std::thread worker([pText, fnTask, text]() {
        try
        {
            g_bRun.store(true);
            fnTask(text);
        }
        catch (const std::exception& e)
        {
            ShowMsgBox(e.what(), "�쳣");
        }
        catch (...)
        {
            ShowMsgBox("δ֪�쳣", "�쳣");
        }
        g_bRun.store(false);
    });

    // �������̣߳��������߳�����
    worker.detach();
}

void NppImp::RunAiRequest(FnRunAiRequest fnTask, const std::string& name, const std::string& para)
{
    // �������������߳�
    std::thread worker([fnTask, name, para]() {
        try
        {
            g_bRun.store(true);
            fnTask(name, para);
        }
        catch (const std::exception& e)
        {
            ShowMsgBox(e.what(), "�쳣");
        }
        catch (...)
        {
            ShowMsgBox("δ֪�쳣", "�쳣");
        }
        g_bRun.store(false);
    });

    // �������̣߳��������߳�����
    worker.detach();
}

void NppImp::NewLineAfterSelText()
{
    // �������λ�ã�ѡ�в���β���½�һ��
    auto pCall = SciCall();
    auto pEnd = pCall->SelectionEnd();
    pCall->ClearSelections();
    pCall->GotoPos(pEnd + 1);
}

std::string NppImp::GetSelText(bool bNewLine/* = false*/, bool doc_if_empty/* = false*/)
{
    CHECK_OR_RETURN(_npp._scintillaMainHandle, "");

    // ��ȡ��ǰ�ı�����
    auto pCall = SciCall();
    auto text = pCall->GetSelText();
    if (doc_if_empty && (text.empty() || text[0]=='\0' || Scintilla::String::Trim(text).empty()))
    {
        text = GetDocText();
        pCall->GotoPos(text.length());
    }
    auto code_page = pCall->CodePage();
    if (code_page > 0 && code_page != CP_ACP)
    {
        text = Scintilla::String::ConvEncoding(text.c_str(), text.size(), code_page, CP_ACP);
    }

    // �½�һ��
    if (bNewLine) NewLineAfterSelText();
    return text;
}

std::string NppImp::GetDocText()
{
    CHECK_OR_RETURN(_npp._scintillaMainHandle, "");

    // ��ȡ��ǰ�ı�����
    auto pCall = SciCall();
    auto nLen = pCall->TextLength();
    if (nLen <= 0)
    {
        return "";
    }

    auto doc = pCall->GetText(nLen);
    return doc;
}