@echo off

setlocal EnableDelayedExpansion

:: Find cl.exe and if not found call vcvarsall to setup the environment variables

where /Q cl.exe || (
    set __VSCMD_ARG_NO_LOGO=1
    for /f "tokens=*" %%i in ('"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" -latest -requires Microsoft.VisualStudio.Workload.NativeDesktop -property installationPath') do set VS=%%i
    if "!VS!" equ "" (
        echo ERROR: Visual Studio installation not found
        exit /b 1
    )
    echo INFO: Calling vcvarsall.bat for amd64
    echo INFO: Do this manually before calling the script to prevent this call every time you build and save time
    echo INFO: You can call !VS!\VC\Auxiliary\Build\vcvarsall.bat amd64
    call "!VS!\VC\Auxiliary\Build\vcvarsall.bat" amd64 || exit /b 1
)

set always_compile_third_party=1

set output_name=minecraft.exe

set compiler_flags= /nologo /Oi /Od /Zi
set compiler_defines=
set compiler_includes= /Isource\ /Ithird_party\ /Ithird_party\glad\include\ /Ithird_party\glfw-3.3.8\include\ /Ithird_party\imgui\
set compiler_options= %compiler_flags% %compiler_defines% %compiler_includes%

set libs= Shell32.lib Kernel32.lib DbgHelp.lib Opengl32.lib User32.lib Gdi32.lib ^
    third_party\glfw-3.3.8\lib-vc2019\glfw3_mt.lib

set linker_flags= /incremental:no /opt:ref /subsystem:console
set linker_options= %libs% %linker_flags%

if not exist gl.obj (
    cl %compiler_flags% /Ithird_party\glad\include\ -c "third_party\glad\src\gl.c"
) else if %always_compile_third_party% == 1 (
    cl %compiler_flags% /Ithird_party\glad\include\ -c "third_party\glad\src\gl.c"
)

if %errorlevel% neq 0 goto error

if not exist ImGui.obj (
    cl %compiler_options% -c "source\ImGui.cpp"
) else if %always_compile_third_party% == 1 (
    cl %compiler_options% -c "source\ImGui.cpp"
)

if %errorlevel% neq 0 goto error

cl %compiler_options% "source\Linalg\gen_Linalg.cpp" /link %linker_options% -OUT:gen_Linalg.exe

if %errorlevel% neq 0 goto error

echo Generating source/Linalg/generated.tpp
call gen_Linalg > source\Linalg\generated.tpp

if %errorlevel% neq 0 goto error

del gen_Linalg.exe
del gen_Linalg.obj
del gen_Linalg.pdb

cl %compiler_options% -c "source\Core.cpp"

if %errorlevel% neq 0 goto error

cl %compiler_options% -c "source\Minecraft.cpp"

if %errorlevel% neq 0 goto error

cl %compiler_options% /D_WIN32 -c "source\Cubiomes.cpp"

if %errorlevel% neq 0 goto error

cl %compiler_flags% Minecraft.obj Core.obj Cubiomes.obj ImGui.obj gl.obj /link %linker_options% -OUT:%output_name%

if %errorlevel% neq 0 goto error

goto:eof

:error
    echo Build failed. Exitting.
    cmd /c exit /b 1
