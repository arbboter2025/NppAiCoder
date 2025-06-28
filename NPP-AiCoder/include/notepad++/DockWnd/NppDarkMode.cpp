#include "NppDarkMode.h"

enum class SystemVersion
{
	Unknown,
	Windows10,
	Windows11
};

SystemVersion GetWindowsVersion()
{
	// 使用RtlGetVersion替代已废弃的GetVersionEx
	typedef NTSTATUS(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);

	OSVERSIONINFOW osInfo = { 0 };
	HMODULE hMod = GetModuleHandleW(L"ntdll.dll");
	if (hMod)
	{
		auto RtlGetVersion = reinterpret_cast<RtlGetVersionPtr>(
			GetProcAddress(hMod, "RtlGetVersion"));
		if (RtlGetVersion) {
			osInfo.dwOSVersionInfoSize = sizeof(osInfo);
			if (RtlGetVersion(&osInfo) == 0)
			{ // STATUS_SUCCESS
				// Windows 11的版本号为10.0.22000+
				if (osInfo.dwMajorVersion == 10 &&
					osInfo.dwMinorVersion == 0)
				{
					if (osInfo.dwBuildNumber >= 22000)
					{
						return SystemVersion::Windows11;
					}
					else if (osInfo.dwBuildNumber >= 10240)
					{
						return SystemVersion::Windows10;
					}
				}
			}
		}
	}
	return SystemVersion::Unknown;
}

bool NppDarkMode::isWindows10() { return GetWindowsVersion() == SystemVersion::Windows10; }
bool NppDarkMode::isWindows11() { return GetWindowsVersion() == SystemVersion::Windows11; }
void NppDarkMode::setDarkTitleBar(HWND hwnd) {}