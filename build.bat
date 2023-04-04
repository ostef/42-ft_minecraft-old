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

set output_name=minecraft.exe

set compiler_flags= /nologo /Oi /Od /Zi
set compiler_defines=
set compiler_includes= /Isource\ /Ithird_party\ /Ithird_party\glad\include\ /Ithird_party\glfw-3.3.8\include\ /Ithird_party\imgui-1.89.4\
set compiler_options= %compiler_flags% %compiler_defines% %compiler_includes%

set libs= Shell32.lib Kernel32.lib DbgHelp.lib Opengl32.lib User32.lib Gdi32.lib ^
	third_party\glad\glad.lib third_party\glfw-3.3.8\lib-vc2019\glfw3_mt.lib

set linker_flags= /incremental:no /opt:ref /subsystem:console
set linker_options= %libs% %linker_flags%

if not exist third_party\glad\glad.lib (
	pushd third_party\glad\
	call build
	popd
)

cl %compiler_options% -c "source\Core.cpp"
cl %compiler_options% -c "source\Linalg.cpp"
cl %compiler_options% -c "source\ImGui.cpp"
cl %compiler_options% -c "source\main.cpp"

cl %compiler_flags% main.obj Core.obj ImGui.obj /link %linker_options% -OUT:%output_name%
