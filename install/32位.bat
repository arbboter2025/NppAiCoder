@echo off
setlocal enabledelayedexpansion

:: �������ԱȨ��
echo ����ʾ���������ʧ�ܣ��볢���Թ���ԱȨ�����У�

:start
set choice=
set /p "choice=��ѡ����Ҫ��Notepad++��AiCode������еĲ�����Y-��װ N-ж�� [Y/N] "
if /i "%choice%"=="Y" goto install
if /i "%choice%"=="N" goto uninstall
echo ������Ч������������Y��N��
goto start

:install
call :get_npp_dir
if not defined nppDir exit /b 1

set "sourceDir=%~dp032"
set "targetDir=%nppDir%\plugins\AiCoder"

echo ���ڰ�װ�� %targetDir%...
mkdir "%targetDir%" 2>nul
xcopy /y /e "%sourceDir%\*" "%targetDir%\" >nul
if errorlevel 1 (
    echo ��װʧ�ܣ���ر�Notepad++���򣬲����Ȩ�޻�·���Ƿ���ȷ��
) else (
    echo ��װ�ɹ���������Notepad++��
)
goto end

:uninstall
call :get_npp_dir
if not defined nppDir exit /b 1

set "targetDir=%nppDir%\plugins\AiCoder"

echo ����ж�أ�ɾ��Ŀ¼ %targetDir%...
rd /s /q "%targetDir%" 2>nul
if exist "%targetDir%" (
    echo ж��ʧ�ܣ�����Ȩ�޻�·���Ƿ���ȷ��
) else (
    echo ж�سɹ���
)
goto end

:get_npp_dir
set "nppDir="

:: ͨ����;�����Ұ�װĿ¼
for /f "tokens=2,*" %%a in ('reg query "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\notepad++.exe" /ve 2^>nul') do (
    set "nppExePath=%%b"
)
if defined nppExePath (
    for %%i in ("!nppExePath!") do set "nppDir=%%~dpi"
    if "!nppDir:~-1!"=="\" set "nppDir=!nppDir:~0,-1!"
)

if not defined nppDir (
    for /f "tokens=2,*" %%a in ('reg query "HKLM\SOFTWARE\Notepad++" /v "Path" 2^>nul') do (
        set "nppDir=%%b"
    )
)

if not defined nppDir (
    for /f "tokens=2,*" %%a in ('reg query "HKLM\SOFTWARE\WOW6432Node\Notepad++" /v "Path" 2^>nul') do (
        set "nppDir=%%b"
    )
)

:: ��֤·����Ч��
if defined nppDir (
    if not exist "!nppDir!\notepad++.exe" (
        set "nppDir="
    )
)

:: �ֶ����봦��
if not defined nppDir (
    :input_npp_dir
    set /p "nppDir=�޷��Զ��ҵ�Notepad++��װĿ¼�����ֶ����밲װ·����"
    set "nppDir=!nppDir:"=!"
    if "!nppDir!"=="" (
        echo ���벻��Ϊ��
        goto input_npp_dir
    )
    if not exist "!nppDir!\notepad++.exe" (
        echo ����·����Ч��δ�ҵ�notepad++.exe
        goto input_npp_dir
    )
)
exit /b 0

:end
pause
endlocal