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

set output_name=glad.lib

set compiler_flags= /nologo /O2 /Zi
set compiler_defines=
set compiler_includes= /Iinclude\
set compiler_options= %compiler_flags% %compiler_defines% %compiler_includes%

cl %compiler_options% -c "src\glad.c"

lib /nologo glad.obj
