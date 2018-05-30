;---------------------------------------------------------------------------
;
; File:         Seq64Constants.nsh
; Author:       Chris Ahlstrom
; Date:         2018-05-26
; Updated:      2018-05-29
; Version:      0.95.0
;
;   Provides constants commonly used by the installer for Sequencer64 for
;   Windows.
;
;---------------------------------------------------------------------------

;============================================================================
; Product Registry keys.
;============================================================================

!define PRODUCT_NAME        "Sequencer64"
!define PRODUCT_DIR_REGKEY  "Software\Microsoft\Windows\CurrentVersion\App Paths\qpseq64.exe"
!define PRODUCT_UNINST_KEY  "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY     "HKLM"

;============================================================================
; Informational settings
;============================================================================

!define VER_MAIN_PURPOSE    "Sequencer64 for Windows"
!define VER_NUMBER          "0.95"
!define VER_REVISION        "0"
!define VER_VARIANT         "Windows"
!define PRODUCT_VERSION     "${VER_NUMBER} ${VER_VARIANT} (rev ${VER_REVISION})"
!define PRODUCT_PUBLISHER   "Chris Ahlstrom"
!define PRODUCT_WEB_SITE    "https://github.com/ahlstromcj/sequencer64-packages"

;============================================================================
; Directory to place the installer.
;============================================================================

!define EXE_DIRECTORY       "..\release"

; vim: ts=4 sw=4 wm=3 et ft=nsis
