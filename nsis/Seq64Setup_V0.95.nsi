;---------------------------------------------------------------------------
;
; File:         Seq64Setup_V0.95.nsi
; Author:       Chris Ahlstrom
; Date:         2018-05-26
; Version:      0.95.0
;
;       Installation is silent.
;
; Usage of this Script:
;
;    -  Obtain and install the NSIS 2.46 installer from
;       http://nsis.sourceforge.net/Download
;    -  Check out the latest branch project from Git.
;    -  In the project directory, on the command-line, run the following
;       command to build the Release version of Seq64 using qmake and make:
;
;           $ qmake -makefile -recursive "CONFIG += release"
;                   ../sequencer64/qplseq64.pro
;           $ make
;
;    -  If the build succeeds, then run
;
;           $ windeployqt ./Seq64qt
;
;    -  Then run NSIS.  Then do the following steps:
;       -   Click on "Compile NSI scripts".
;       -   Click File / Load Script.
;       -   Navigate to the "nsis" directory and select
;           "Seq64Setup_V0.95.nsi".  The script will take a few minutes
;           to build.  The output goes to "...."
;       -   You can run that executable, or you can instead click the
;           "Test Installer" button in the NSIS window.
;       -   When you get to the "Choose Install Location" window, you can
;           use "C" and test the installation.
;    -  The installer package is located at "...."
;       -   Copy this executable to a CD-ROM drive.
;       -   Bring it to a prospective VIDS server and run it.
;       -   Select the defaults and let the installer do its thing.
;    -  To uninstall the application, use Settings /
;           Control Panel / Add and Remove Programs.
;
; References:
;
;    -  http://nsis.sourceforge.net/Download
;    -  http://www.atomicmpc.com.au/Feature/24263,
;           tutorial-create-a-nsis-install-script.aspx/2
;
;---------------------------------------------------------------------------

!include Sections.nsh
; !include VIDSUtilFunctions.nsh
!include Seq64Constants.nsh

;---------------------------------------------------------------------------
; Here we set silent install.
;---------------------------------------------------------------------------

SilentInstall silent

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"

BrandingText "${PRODUCT_NAME} ${PRODUCT_VERSION} NSIS-based Installer"

OutFile "${EXE_DIRECTORY}\sequencer64_setup_${VER_NUMBER}.exe"

RequestExecutionLevel admin
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails show
ShowUnInstDetails show
InstallDir "$PROGRAMFILES\VIDS"

; SectionGroup /e "Workstation"

Section "Binaries"

    SetOutPath "$INSTDIR"
    SetOverwrite on
;   File "Projects\apps\AFLCSClient\vs10\Release\AFLCSClient.exe"
    File "qpseq64.exe"
          
;   CreateShortCut "$SMPROGRAMS\Startup\seq64start.lnk" "$INSTDIR\seq64start.bat"

SectionEnd

Section "Supporting DLLs" 

    SetOutPath "$INSTDIR"
    SetOverwrite on

;   File "Projects\libs\tcpip\vs10\Release\clientdll.dll"

SectionEnd

; SectionGroupEnd

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

;   File "Projects\license.txt"
;   File "Projects\readme.txt"

SectionEnd

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
