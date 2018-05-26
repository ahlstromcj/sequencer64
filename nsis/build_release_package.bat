:: **************************************************************************
:: Build Release Package
:: --------------------------------------------------------------------------
::
:: \file        build_release_package.bat
:: \library     Sequencer64 for Windows
:: \author      Chris Ahlstrom
:: \date        2018-05-26
:: \update      2018-05-26
:: \license     $XPC_SUITE_GPL_LICENSE$
::
::      This script sets up and creates a release build of Sequencer64 for
::      Windows and creates a 7-Zip package that can be unpacked in the root
::      of the project, ready to made into an NSIS installer either in Linux
::      or in Windows.
::
:: Requirements:
::
::       1. Runs in Windows only.
::       2. Requires QtCreator to be installed, configured to provide
::          the 32-bit Mingw tools, including mingw32-make.exe, and
::          qmake.exe.  The PATH must included the path to both executables.
::       3. Requires 7-Zip to be installed and accessible from the DOS
::          command-line, as 7z.exe.
::
:: Instructions:
::
::      Before running this script, modify the environment variables below
::      for your specific setup.
::
::---------------------------------------------------------------------------
 
set PROJECT_DRIVE=C:
set PROJECT_BASE=\Users\Chris\Documents\Home
set PROJECT_ROOT=..\sequencer64
set PROJECT_FILE=qplseq64.pro
set SHADOW_DIR=seq64-release
set APP_DIR=Seq64qt5
set OUTPUT_DIR=%APP_DIR%\release
set CONFIG_SET="CONFIG += release"
set AUX_DIR=data

:: C:

%PROJECT_DRIVE%

:: cd \Users\Chris\Documents\Home

cd %PROJECT_BASE%

:: mkdir seq64-release
:: cd seq64-release

mkdir %SHADOW_DIR%
cd %SHADOW_DIR%

:: qmake -makefile -recursive "CONFIG += release" ..\sequencer64\qplseq64.pro

qmake -makefile -recursive %CONFIG_SET% %PROJECT_ROOT%\%PROJECT_FILE%
mingw32-make 2> make.log

:: windeployqt Seq64qt5\release

windeployqt %OUTPUT_DIR%

:: mkdir Seq64qt5\release\data
:: copy ..\sequencer64\data\b4uacuse-gm-patchless.midi Seq64qt5\release\data

mkdir %OUTPUT_DIR%\%AUX_DIR%
copy %PROJECT_ROOT%\%AUX_DIR%\b4uacuse-gm-patchless.midi %OUTPUT_DIR%\%AUX_DIR%

REM copy %PROJECT_ROOT%\%AUX_DIR%\qpseq64.* %OUTPUT_DIR%\%AUX_DIR%
REM copy %PROJECT_ROOT%\%AUX_DIR%\*.pdf %OUTPUT_DIR%\%AUX_DIR%

:: vim: ts=4 sw=4 ft=dosbatch
