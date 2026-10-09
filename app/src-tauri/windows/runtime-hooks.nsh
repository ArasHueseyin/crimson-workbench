!include nsDialogs.nsh
!include LogicLib.nsh
!include TextFunc.nsh
!define CW_HOOK_DIR "${__FILEDIR__}"

Var CWRuntimeChoice
Var CWGameDirectory
Var CWRuntimeCheckbox
Var CWGameInput
Var CWBrowseButton
Var CWPowerShell
Var CWInitialized
Var CWShowErrors

LangString CWTitle 1031 "Spielmodule mitinstallieren"
LangString CWTitle 1033 "Install game modules"
LangString CWDescription 1031 "Live-Items und Zusatzsockel sind in diesem Setup enthalten."
LangString CWDescription 1033 "Live Items and Extra Sockets are included in this setup."
LangString CWCheckbox 1031 "Live-Items und Zusatzsockel installieren (empfohlen)"
LangString CWCheckbox 1033 "Install Live Items and Extra Sockets (recommended)"
LangString CWInfo 1031 "Spiel und Workbench vorher schliessen. Unterstuetzt wird EXE 1.0.0.2976. Vorhandene fremde Mods werden nicht ersetzt. Windows kann Administratorrechte anfordern."
LangString CWInfo 1033 "Close the game and Workbench first. Supports EXE 1.0.0.2976. Existing third-party mods will not be replaced. Windows may request administrator permission."
LangString CWFolder 1031 "Crimson-Desert-Spielordner (enthaelt bin64):"
LangString CWFolder 1033 "Crimson Desert game folder (contains bin64):"
LangString CWBrowse 1031 "Auswaehlen..."
LangString CWBrowse 1033 "Browse..."
LangString CWInvalid 1031 "Bitte den Spielordner mit bin64\CrimsonDesert.exe auswaehlen oder die Spielmodule abwaehlen."
LangString CWInvalid 1033 "Choose the game folder containing bin64\CrimsonDesert.exe, or deselect the game modules."
LangString CWFailed 1031 "Die Workbench wurde installiert, aber die Spielmodule konnten nicht eingerichtet werden. Die konkrete Ursache steht im Fehlerdialog. Nach Behebung dieses Setup erneut ausfuehren."
LangString CWFailed 1033 "Workbench was installed, but game module setup failed. See the error dialog for the cause. Resolve it and run this setup again."

; Tauri includes hooks before its standard pages. This is a page of the same
; installer, not a second application or a script the user must launch.
Page custom CWRuntimePage CWRuntimeLeave

Function CWRuntimePage
  IfSilent cw_skip_page
  ; Tauri declares its variables after including this file. Inspect /P here
  ; instead of referring to its not-yet-declared PassiveMode variable.
  ClearErrors
  ${GetOptions} $CMDLINE "/P" $0
  ${IfNot} ${Errors}
    Goto cw_skip_page
  ${EndIf}
  ${If} $CWInitialized != 1
    StrCpy $CWInitialized 1
    StrCpy $CWRuntimeChoice ${BST_CHECKED}
    StrCpy $CWPowerShell "$WINDIR\System32\WindowsPowerShell\v1.0\powershell.exe"
    InitPluginsDir
    File /oname=$PLUGINSDIR\Find-Game.ps1 "${CW_HOOK_DIR}\Find-Game.ps1"
    nsExec::ExecToStack '"$CWPowerShell" -NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File "$PLUGINSDIR\Find-Game.ps1"'
    Pop $0
    Pop $1
    ${If} $0 == 0
      ${TrimNewLines} $1 $CWGameDirectory
    ${EndIf}
  ${EndIf}
  !insertmacro MUI_HEADER_TEXT "$(CWTitle)" "$(CWDescription)"
  nsDialogs::Create 1018
  Pop $0
  ${If} $0 == error
    Abort
  ${EndIf}
  ${NSD_CreateCheckbox} 0 0 100% 20u "$(CWCheckbox)"
  Pop $CWRuntimeCheckbox
  ${NSD_SetState} $CWRuntimeCheckbox $CWRuntimeChoice
  ${NSD_OnClick} $CWRuntimeCheckbox CWRuntimeToggle
  ${NSD_CreateLabel} 0 26u 100% 42u "$(CWInfo)"
  Pop $0
  ${NSD_CreateLabel} 0 75u 100% 12u "$(CWFolder)"
  Pop $0
  ${NSD_CreateDirRequest} 0 92u 75% 14u "$CWGameDirectory"
  Pop $CWGameInput
  ${NSD_CreateBrowseButton} 77% 91u 23% 16u "$(CWBrowse)"
  Pop $CWBrowseButton
  ${NSD_OnClick} $CWBrowseButton CWRuntimeBrowse
  Call CWRuntimeEnable
  nsDialogs::Show
  Return
  cw_skip_page:
    StrCpy $CWRuntimeChoice ${BST_UNCHECKED}
    Abort
FunctionEnd

Function CWRuntimeEnable
  ${NSD_GetState} $CWRuntimeCheckbox $CWRuntimeChoice
  EnableWindow $CWGameInput $CWRuntimeChoice
  EnableWindow $CWBrowseButton $CWRuntimeChoice
FunctionEnd
Function CWRuntimeToggle
  Pop $0
  Call CWRuntimeEnable
FunctionEnd
Function CWRuntimeBrowse
  Pop $0
  ${NSD_GetText} $CWGameInput $CWGameDirectory
  nsDialogs::SelectFolderDialog "$(CWFolder)" "$CWGameDirectory"
  Pop $0
  ${If} $0 != error
    ${NSD_SetText} $CWGameInput $0
  ${EndIf}
FunctionEnd
Function CWRuntimeLeave
  ${NSD_GetState} $CWRuntimeCheckbox $CWRuntimeChoice
  ${NSD_GetText} $CWGameInput $CWGameDirectory
  ${If} $CWRuntimeChoice == ${BST_CHECKED}
    IfFileExists "$CWGameDirectory\bin64\CrimsonDesert.exe" cw_valid_folder
    MessageBox MB_OK|MB_ICONEXCLAMATION "$(CWInvalid)"
    Abort
  ${EndIf}
  cw_valid_folder:
FunctionEnd

!macro NSIS_HOOK_PREINSTALL
  StrCpy $CWPowerShell "$WINDIR\System32\WindowsPowerShell\v1.0\powershell.exe"
  StrCpy $CWShowErrors "-ShowErrors"
  ${If} ${Silent}
    StrCpy $CWShowErrors ""
    ; Explicit opt-in for deployment/testing. Plain /S never modifies a game.
    ${GetParameters} $0
    ClearErrors
    ${GetOptions} $0 "/INSTALLMODS" $1
    ${IfNot} ${Errors}
      StrCpy $CWRuntimeChoice ${BST_CHECKED}
      ${GetOptions} $0 "/GAMEDIR=" $CWGameDirectory
    ${EndIf}
  ${EndIf}
!macroend

!macro NSIS_HOOK_POSTINSTALL
  ${If} $CWRuntimeChoice == ${BST_CHECKED}
    DetailPrint "$(CWTitle)"
    ; The native installer launches a hidden helper; no terminal or commands
    ; are shown. UAC stays intact when administrator permission is needed.
    nsExec::ExecToStack '"$CWPowerShell" -NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File "$INSTDIR\runtime-mods\Setup-RuntimeMods.ps1" -GameDirectory "$CWGameDirectory\." -Elevate $CWShowErrors'
    Pop $0
    Pop $1
    ${If} $0 != 0
      DetailPrint "$1"
      IfSilent +2
        MessageBox MB_OK|MB_ICONEXCLAMATION "$(CWFailed)"
      SetErrorLevel 1
      Abort
    ${EndIf}
  ${EndIf}
!macroend
