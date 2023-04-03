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

set output_obj_dir=obj
set output_name=minecraft.exe

set compiler_flags= /nologo /Oi /Od /Zi
set compiler_defines=
set compiler_includes= /Isource\ /Ithird_party\
set compiler_options= %compiler_flags% %compiler_defines% %compiler_includes%

set linker_flags= -incremental:no -opt:ref -subsystem:console
set libs=Kernel32.lib DbgHelp.lib
set linker_options= %libs% %linker_flags%

if not exist %output_obj_dir%\ (
	mkdir %output_obj_dir%\
)

cl %compiler_options% -c "source\Core.cpp"   /Fo%output_obj_dir%\
cl %compiler_options% -c "source\main.cpp"   /Fo%output_obj_dir%\

cl %compiler_flags% %output_obj_dir%\main.obj %output_obj_dir%\Linalg.obj %output_obj_dir%\Core.obj /link %linker_options% -OUT:%output_name%
