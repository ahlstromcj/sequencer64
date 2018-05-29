@echo off

:: **************************************************************************
:: Build Release Package
:: --------------------------------------------------------------------------
::
:: \file        build_release_package.bat
:: \library     Sequencer64 for Windows
:: \author      Chris Ahlstrom
:: \date        2018-05-26
:: \update      2018-05-27
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
:: It completely removes the old release directory and re-does everything.
::
::---------------------------------------------------------------------------
 
set PROJECT_VERSION=0.95.0
set PROJECT_DRIVE=C:
set PROJECT_BASE=\Users\Chris\Documents\Home
set PROJECT_ROOT=..\sequencer64
set PROJECT_FILE=qplseq64.pro
set PROJECT_7ZIP="qpseq64-release-package-%PROJECT_VERSION%.7z"
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

del /S /Q %SHADOW_DIR% > NUL
mkdir %SHADOW_DIR%
cd %SHADOW_DIR%

:: qmake -makefile -recursive "CONFIG += release" ..\sequencer64\qplseq64.pro

cd
echo qmake -makefile -recursive %CONFIG_SET% %PROJECT_ROOT%\%PROJECT_FILE%
echo mingw32-make (output to make.log)
qmake -makefile -recursive %CONFIG_SET% %PROJECT_ROOT%\%PROJECT_FILE%
mingw32-make > make.log 2>&1

:: windeployqt Seq64qt5\release

echo windeployqt %OUTPUT_DIR%
windeployqt %OUTPUT_DIR%

:: mkdir Seq64qt5\release\data
:: copy ..\sequencer64\data\*.midi Seq64qt5\release\data
:: copy ..\sequencer64\data\qpseq64.* Seq64qt5\release\data
:: copy ..\sequencer64\data\*.pdf Seq64qt5\release\data
:: copy ..\sequencer64\data\*.txt Seq64qt5\release\data

echo mkdir %OUTPUT_DIR%\%AUX_DIR%
echo copy %PROJECT_ROOT%\%AUX_DIR%\qpseq64.* %OUTPUT_DIR%\%AUX_DIR%
echo copy %PROJECT_ROOT%\%AUX_DIR%\*.midi %OUTPUT_DIR%\%AUX_DIR%
echo copy %PROJECT_ROOT%\%AUX_DIR%\*.pdf %OUTPUT_DIR%\%AUX_DIR%
echo copy %PROJECT_ROOT%\%AUX_DIR%\*.txt %OUTPUT_DIR%\%AUX_DIR%

mkdir %OUTPUT_DIR%\%AUX_DIR%
copy %PROJECT_ROOT%\%AUX_DIR%\qpseq64.* %OUTPUT_DIR%\%AUX_DIR%
copy %PROJECT_ROOT%\%AUX_DIR%\*.midi %OUTPUT_DIR%\%AUX_DIR%
copy %PROJECT_ROOT%\%AUX_DIR%\*.pdf %OUTPUT_DIR%\%AUX_DIR%
copy %PROJECT_ROOT%\%AUX_DIR%\*.txt %OUTPUT_DIR%\%AUX_DIR%

:: This section takes the generated build and data files and packs them
:: up into a 7-zip archive.  This archive should be copied to the root
:: directory (sequencer64) and extracted (the contents go into the release
:: directory.
::
:: Then, in Linux, "cd" to the "nsis" directory and run
::
::      makensis Seq64Setup_V0.95.nsi
::
:: pushd Seq64qt5
:: 7z a -r qppseq64-nsis-ready-package-DATE.7z release\*

pushd %APP_DIR%
cd
echo 7z a -r %PROJECT_7ZIP% release\*
7z a -r %PROJECT_7ZIP% release\*
popd

:: vim: ts=4 sw=4 ft=dosbatch fileformat=dos
