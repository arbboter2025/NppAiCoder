@echo off
setlocal enabledelayedexpansion

:: 请求管理员权限
echo 【提示】如果操作失败，请尝试以管理员权限运行！

:start
set choice=
set /p "choice=请选择需要对Notepad++的AiCode插件进行的操作：Y-安装 N-卸载 [Y/N] "
if /i "%choice%"=="Y" goto install
if /i "%choice%"=="N" goto uninstall
echo 输入无效，请重新输入Y或N。
goto start

:install
call :get_npp_dir
if not defined nppDir exit /b 1

set "sourceDir=%~dp032"
set "targetDir=%nppDir%\plugins\AiCoder"

echo 正在安装到 %targetDir%...
mkdir "%targetDir%" 2>nul
xcopy /y /e "%sourceDir%\*" "%targetDir%\" >nul
if errorlevel 1 (
    echo 安装失败，请关闭Notepad++程序，并检查权限或路径是否正确。
) else (
    echo 安装成功，请重启Notepad++！
)
goto end

:uninstall
call :get_npp_dir
if not defined nppDir exit /b 1

set "targetDir=%nppDir%\plugins\AiCoder"

echo 正在卸载，删除目录 %targetDir%...
rd /s /q "%targetDir%" 2>nul
if exist "%targetDir%" (
    echo 卸载失败，请检查权限或路径是否正确。
) else (
    echo 卸载成功！
)
goto end

:get_npp_dir
set "nppDir="

:: 通过多途径查找安装目录
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

:: 验证路径有效性
if defined nppDir (
    if not exist "!nppDir!\notepad++.exe" (
        set "nppDir="
    )
)

:: 手动输入处理
if not defined nppDir (
    :input_npp_dir
    set /p "nppDir=无法自动找到Notepad++安装目录，请手动输入安装路径："
    set "nppDir=!nppDir:"=!"
    if "!nppDir!"=="" (
        echo 输入不能为空
        goto input_npp_dir
    )
    if not exist "!nppDir!\notepad++.exe" (
        echo 错误：路径无效，未找到notepad++.exe
        goto input_npp_dir
    )
)
exit /b 0

:end
pause
endlocal