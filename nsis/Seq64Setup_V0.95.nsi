;---------------------------------------------------------------------------
;
; File:         Seq64Setup_V0.95.nsi
; Author:       Chris Ahlstrom
; Date:         2018-05-26
; Updated:      2018-05-29
; Version:      0.95.0
;
;       Installation is silent.
;
; Usage of this script:
;
;    -  Obtain and install the NSIS 2.46 installer from
;       http://nsis.sourceforge.net/Download
;    -  Check out the latest branch project from Git.
;    -  In the project directory, on the command-line, run the following
;       command to build the Release version of Seq64 using qmake and make,
;       and to create a 7-Zip "release" package that can be unpacked in
;       the root "sequencer64" directory:
;
;           $ build_release_package.bat
;
;    -  Then run NSIS:
;       -   Windows:
;           -   Click on "Compile NSI scripts".
;           -   Click File / Load Script.
;           -   Navigate to the "nsis" directory and select
;               "Seq64Setup_V0.95.nsi".  The script will take a few minutes
;               to build.  The output goes to "...."
;           -   You can run that executable, or you can instead click the
;               "Test Installer" button in the NSIS window.
;           -   When you get to the "Choose Install Location" window, you can
;               use "C" and test the installation.
;       -   Linux:
;           -   TODO
;    -  The installer package is located at "...."
;       -   Select the defaults and let the installer do its thing.
;    -  To uninstall the application, use Settings /
;           Control Panel / Add and Remove Programs.  The application is
;           Sequencer64, and the executable is qpseq64.exe.
;
; References:
;
;    -  http://nsis.sourceforge.net/Download
;    -  http://www.atomicmpc.com.au/Feature/24263,
;           tutorial-create-a-nsis-install-script.aspx/2
;
;---------------------------------------------------------------------------

;---------------------------------------------------------------------------
;   MUI.nsh provides GUI features as expected for an installer.
;   Sections.nsh provides support for sections and section groups.
;   Seq64Constants.nsh contains names and version numbers.
;---------------------------------------------------------------------------

!include MUI.nsh
!include MUI2.nsh
!include Sections.nsh
!include Seq64Constants.nsh

!define MUI_ICON "..\resources\icons\route64rwb-64x64.ico"
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "..\resources\icons\route64rwb-64x64.bmp"
!define MUI_HEADERIMAGE_RIGHT

;---------------------------------------------------------------------------
; We want:
;
;   -   The description at the bottom.
;   -   A welcome page.
;   -   A license page.
;   -   A components page.
;   -   A directory page to allow changing the installation location of
;       Sequencer64.
;   -   An install-files page.
;   -   A finish page.
;   -   An uninstaller page.
;   -   An abort-warning prompt.
;
;---------------------------------------------------------------------------

!define MUI_COMPONENTSPAGE_SMALLDESC
!insertmacro MUI_PAGE_WELCOME

!define MUI_LICENSEPAGE_CHECKBOX
!insertmacro MUI_PAGE_LICENSE "..\data\license.txt"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

!define MUI_FINISHPAGE_SHOWREADME "..\data\readme.txt"
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_INSTFILES

!define MUI_ABORTWARNING
!insertmacro MUI_LANGUAGE "English"

;---------------------------------------------------------------------------
; Here we set a non-silent install.
;
;   SilentInstall silent
;
;---------------------------------------------------------------------------

SilentInstall normal

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
BrandingText "${PRODUCT_NAME} ${PRODUCT_VERSION} NSIS-based Installer"
OutFile "${EXE_DIRECTORY}\sequencer64_setup_${VER_NUMBER}.${VER_REVISION}.exe"
RequestExecutionLevel admin
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails show
ShowUnInstDetails show
InstallDir "$PROGRAMFILES\${PRODUCT_NAME}"

;---------------------------------------------------------------------------
; The actual installer sections
;---------------------------------------------------------------------------

Section "Application" SEC_APPLIC

    SetOutPath "$INSTDIR"
    SetOverwrite on
    File "..\release\qpseq64.exe"

SectionEnd

SectionGroup "Qt5 Support" SEC_QT5

Section "Mingw DLLs" SEC_MINGW

    SetOutPath "$INSTDIR"
    SetOverwrite on
    File "..\release\D3Dcompiler_47.dll"
    File "..\release\lib*.dll"
    File "..\release\opengl*.dll"

SectionEnd

Section "Qt5 Main DLLs" SEC_QTDLLS

    SetOutPath "$INSTDIR"
    SetOverwrite on
    File "..\release\Qt*.dll"

SectionEnd

Section "Qt5 Icon Engine" SEC_QTICON

    SetOutPath "$INSTDIR\iconengines"
    SetOverwrite on
    File /r "..\release\iconengines\*.*"

SectionEnd

Section "Qt5 Imaging" SEC_QTIMG

    SetOutPath "$INSTDIR\imageformats"
    SetOverwrite on
    File /r "..\release\imageformats\*.*"

SectionEnd

Section "Qt5 Platform Support" SEC_QTPLAT

    SetOutPath "$INSTDIR\platforms"
    SetOverwrite on
    File /r "..\release\platforms\*.*"

SectionEnd

Section "Qt5 Style Engine" SEC_QTSTYLE

    SetOutPath "$INSTDIR\styles"
    SetOverwrite on
    File /r "..\release\styles\*.*"

SectionEnd

Section "Qt5 Translations" SEC_QTTRANS

    SetOutPath "$INSTDIR\translations"
    SetOverwrite on
    File /r "..\release\translations\*.*"

SectionEnd

SectionGroupEnd

Section "Licensing and Sample Files" SEC_LIC

    SetOutPath "$INSTDIR\data"
    SetOverwrite on
    File /r "..\release\data\*"

SectionEnd

;--------------------------------------------------------------------------
; Section "Registry Entries"
;
;   Sequencer64 is completely configured via qpseq64.rc and qpseq64.usr
;   in the user-directory C:/Users/username/AppData/Local/sequencer64.
;
;--------------------------------------------------------------------------

; Section "Registry Entries"
; SectionEnd

;--------------------------------------------------------------------------
; Post
;--------------------------------------------------------------------------
;
;   In this section, uninstallation Registry keys are added.

;   We are not sure if they are needed.
;
;--------------------------------------------------------------------------

Section -Post

    WriteUninstaller "$INSTDIR\uninst.exe"
    WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\qpseq64.exe"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"

    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"

    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\qpseq64.exe"

    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"

    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"

    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"

SectionEnd

;--------------------------------------------------------------------------
; Section Descriptions
;--------------------------------------------------------------------------

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
!insertmacro MUI_DESCRIPTION_TEXT ${SEC_APPLIC}  "Application 32-bit executable."
!insertmacro MUI_DESCRIPTION_TEXT ${SEC_QT5}     "Qt 5 DLLs."
!insertmacro MUI_DESCRIPTION_TEXT ${SEC_MINGW}   "MingW 32-bit DLLs."
!insertmacro MUI_DESCRIPTION_TEXT ${SEC_QTDLLS}  "Qt 5 32-bit DLLs."
!insertmacro MUI_DESCRIPTION_TEXT ${SEC_QTICON}  "Qt 5 icon-engine DLLs."
!insertmacro MUI_DESCRIPTION_TEXT ${SEC_QTIMG}   "Qt 5 image-format DLLs."
!insertmacro MUI_DESCRIPTION_TEXT ${SEC_QTPLAT}  "Qt 5 platform DLLs."
!insertmacro MUI_DESCRIPTION_TEXT ${SEC_QTSTYLE} "Qt 5 style DLLs."
!insertmacro MUI_DESCRIPTION_TEXT ${SEC_QTTRANS} "Qt 5 translation files."
!insertmacro MUI_DESCRIPTION_TEXT ${SEC_LIC}     "Licenses and sample files."
!insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------------------------------------------------
; Uninstallation operations
;--------------------------------------------------------------------------

Function un.onUninstSuccess

  HideWindow
  MessageBox MB_ICONINFORMATION|MB_OK "$(^Name) was successfully removed from your computer."

FunctionEnd

Function un.onInit

  MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "Are you sure you want to completely remove $(^Name) and all of its components?" IDYES +2
  Abort

FunctionEnd


; The Uninstall section is layed out as much as possible to match the
; sections listed above.
;
; Set up to call a few batch files

Section Uninstall

    ExpandEnvStrings $0 %COMSPEC%

;   Delete "$INSTDIR\license.txt"
;   Delete "$INSTDIR\readme.txt"


    Delete "$INSTDIR\uninst.exe"
    RMDir /r "$INSTDIR"

    DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
    DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"

SectionEnd

; vim: ts=4 sw=4 wm=3 et ft=nsis
