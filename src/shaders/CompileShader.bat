@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

REM =====================================================================
REM STEP 1: Set the absolute path to glslangValidator.exe below.
REM =====================================================================
set COMPILER="E:\Vulkansdk\Bin\glslangValidator.exe"

REM Get input arguments
set "INPUT=%~1"
set "OUT_DIR=%~2"

REM Check if input is empty
if "%INPUT%"=="" (
    echo ==========================================================
    echo [错误] 请输入要编译的源文件或文件夹路径！
    echo.
    echo 用法 1 ^(默认输出^): %~nx0 ^<文件或文件夹路径^>
    echo 用法 2 ^(自定义输出^): %~nx0 ^<文件或文件夹路径^> ^<自定义输出目录^>
    echo.
    echo 💡 快捷操作: 可以直接把 .vert / .frag / .comp 等文件或文件夹拖拽到此 .bat 上。
    echo ==========================================================
    pause
    exit /b 1
)

REM Handle custom output directory
if not "%OUT_DIR%"=="" (
    if not exist "%OUT_DIR%" (
        echo [信息] 正在创建自定义输出目录...
        mkdir "%OUT_DIR%"
    )
    if not "!OUT_DIR:~-1!"=="\" set "OUT_DIR=!OUT_DIR!\"
)

REM =====================================================================
REM STEP 2: Process file or directory
REM =====================================================================
if exist "%INPUT%\" (
    echo [信息] 识别到文件夹，开始批量编译目录下的所有 GLSL 文件...
    REM 扩展：同时匹配多种 GLSL 着色器后缀
    for %%f in ("%INPUT%\*.vert" "%INPUT%\*.frag" "%INPUT%\*.comp" "%INPUT%\*.geom" "%INPUT%\*.tesc" "%INPUT%\*.tese") do (
        call :CompileShader "%%f" "!OUT_DIR!"
    )
) else (
    if not exist "%INPUT%" (
        echo [错误] 找不到指定的文件: "%INPUT%"
        pause
        exit /b 1
    )
    echo [信息] 识别到单文件，开始编译...
    call :CompileShader "%INPUT%" "!OUT_DIR!"
)

echo.
echo ==========================================================
echo [完成] 所有编译任务已结束。
echo ==========================================================
pause
exit /b 0

REM =====================================================================
REM STEP 3: Compile Function
REM =====================================================================
:CompileShader
set "FILE_IN=%~1"
set "CUSTOM_OUT=%~2"

set "DIR_IN=%~dp1"
set "FULL_NAME=%~nx1"

if "%CUSTOM_OUT%"=="" (
    set "FILE_OUT=%DIR_IN%%FULL_NAME%.spv"
) else (
    set "FILE_OUT=%CUSTOM_OUT%%FULL_NAME%.spv"
)

echo.
echo [编译中] %FULL_NAME%
echo   ├─ 源路径: %FILE_IN%
echo   └─ 目标  : %FILE_OUT%

%COMPILER% -V "%FILE_IN%" -o "%FILE_OUT%"

if %ERRORLEVEL% equ 0 (
    echo   └─ 结果  : [√] 成功
) else (
    echo   └─ 结果  : [X] 失败 (请向上查看报错信息)
)
exit /b 0