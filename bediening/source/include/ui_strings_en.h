#pragma once

// =============================================================================
// ALL texts shown on the display (English). To translate: copy this file to
// ui_strings_nl.h (or another language), translate the values, and build with
// -D UI_LANG_NL  (see ui_strings.h). Logging and code comments stay English.
//
// Rules:
//  * Keep the maximum lengths noted per group, they are CHECKED AT COMPILE TIME
//    (ui_strings.h). The display is 160 px wide and every character is 8 px.
//  * "%u" / "%s" are printf placeholders: keep them, the order is fixed.
//  * The font has all Latin-1 letters (accents: é è ë ï ö ü ...) and a few symbols.
// =============================================================================

// ---- Screen titles (max 20 characters) ------------------------------------------
#define STR_MENU_TITLE "Menu"
#define STR_SETUP_TITLE "Setup"
#define STR_SCAN_LAUNCHER_TITLE "Scan launcher"
#define STR_SCAN_SWITCHBOARD_TITLE "Scan switchboard"
#define STR_FORGET_LAUNCHER_TITLE "Forget launcher?"
#define STR_FORGET_SWITCHBOARD_TITLE "Forget switchboard?"
#define STR_CHECK_TITLE "! CHECK LAUNCHER !"
#define STR_CHECK_ERROR_TITLE "! LAUNCHER ERROR !"
#define STR_HELP_TITLE "CHECK BY HAND"

// ---- Device names (max 17) -------------------------------------------------------
#define STR_LAUNCHER "Launcher"
#define STR_SWITCHBOARD "Switchboard"

// ---- Menu rows and buttons (max 17) ---------------------------------------------
#define STR_BACK "Back"
#define STR_CANCEL "Cancel"
#define STR_OK "OK"
#define STR_MENU_CHECK "Check launcher"
#define STR_MENU_SETUP "Setup"
#define STR_MENU_MAINTENANCE "Maintenance"
#define STR_SCAN_SELECT "Scan & select"
#define STR_FORGET "Forget"
#define STR_YES_FORGET "Yes, forget"
#define STR_SCAN_AGAIN "Scan again"

// ---- Small info lines under a title (max 20) ------------------------------------
#define STR_SETUP_INFO "select a device"
#define STR_NOT_SET "not set"
#define STR_SAVED "saved"      // right side of a setup row (max 5)
#define STR_NONE "none"        // right side of a setup row (max 5)

// ---- Scan screen (max 20; scanning/found use printf placeholders) ---------------
#define STR_SCAN_STARTING "starting..."
#define STR_SCAN_SCANNING "scanning %us"
#define STR_SCAN_FOUND "found %u"
#define STR_SCAN_NOTHING "nothing found"
#define STR_SCAN_FAILED "scan failed"
#define STR_SAVE_FAILED "save failed"
#define STR_SRC_UUID "UUID"    // how a device was recognised: by its service UUID ...
#define STR_SRC_NAME "name"    // ... or by its name only (max 4)

// ---- Connection state of a device (max 17) --------------------------------------
#define STR_LINK_NOT_SET "not set"
#define STR_LINK_SEARCHING "searching"
#define STR_LINK_CONNECTING "connecting"
#define STR_LINK_CONNECTED "connected"
#define STR_LINK_ERROR "error"
#define STR_ERR_SERVICE_MISSING "service missing"
#define STR_ERR_CHAR_MISSING "characteristic?"
#define STR_ERR_NOT_WRITABLE "not writable"
#define STR_ERR_SUBSCRIBE "subscribe failed"
#define STR_ERR_READ "read failed"
#define STR_ERR_NO_CLIENT "no BLE client"

// ---- Short messages for the driver (max 20) -------------------------------------
#define STR_MSG_LAUNCHED "LAUNCHED"
#define STR_MSG_LAUNCHER_OFFLINE "launcher offline"
#define STR_MSG_LAUNCHER_STATE_UNKNOWN "launcher state ?"
#define STR_MSG_NOT_READY "not ready yet"
#define STR_MSG_STILL_PREPARING "still preparing"
#define STR_MSG_LAUNCHER_ERROR "launcher error"
#define STR_MSG_CHECK_FIRST "check launcher"
#define STR_MSG_LAUNCH_FAILED "LAUNCH FAILED"
#define STR_MSG_PREPARE_REFUSED "prepare refused"
#define STR_MSG_PREPARE_TIMEOUT "prepare timeout"
#define STR_MSG_MAINT_BLOCKS_LAUNCH "in maintenance"
#define STR_MSG_MAINT_BUSY "launcher busy"
#define STR_MSG_RELEASE_OK "bucket released"
#define STR_MSG_RELEASE_FAILED "release failed"
#define STR_MSG_DISPENSE_FAILED "dispense failed"
#define STR_MSG_DISPENSE_STUCK "dispenser stuck?"
#define STR_MSG_MOTOR_FAILED "motor failed"
#define STR_MSG_MOTOR_STUCK "motor stuck?"
#define STR_MSG_STILL_STOPPING "still stopping"
#define STR_MSG_DURATION_SAVED "time saved"
#define STR_MSG_DURATION_FAILED "save failed"
#define STR_MSG_PREPARE_FAILED "PREPARE FAILED"

// ---- Maintenance (pauses all automation) ----
#define STR_MAINT_TITLE "Maintenance"
#define STR_MAINT_INFO "automation paused"
#define STR_MAINT_RELEASE "Release bucket"
#define STR_MAINT_DISPENSE "Dispense"
#define STR_MAINT_MOTOR "Motor control"
#define STR_MAINT_DURATION "Dispenser time"
#define STR_DURATION_TITLE "Dispenser time"
#define STR_DURATION_HINT "turn, push saves"
#define STR_DURATION_SAVE "Push to save"
#define STR_DURATION_UNIT "seconds"
#define STR_DURATION_UNSAVED "seconds (unsaved)"
#define STR_DURATION_SAVING "saving..."
#define STR_DURATION_READING "reading..."
#define STR_DURATION_NONE "no reading"
#define STR_DISPENSE_LAST "last"
#define STR_MAINT_SWITCH "Bottom switch"
#define STR_SWITCH_TITLE "Bottom switch"
#define STR_SWITCH_HINT "bucket at bottom?"
#define STR_SWITCH_PRESSED "PRESSED"
#define STR_SWITCH_RELEASED "RELEASED"
#define STR_SWITCH_NO_READING "NO READING"
#define STR_SWITCH_PRESSED_INFO "bucket is down"
#define STR_SWITCH_RELEASED_INFO "bucket not down"
#define STR_SWITCH_NONE_INFO "launcher offline?"
#define STR_RELEASE_TITLE "Release bucket?"
#define STR_RELEASE_INFO "bucket may fire!"
#define STR_YES_RELEASE "Yes, release"
#define STR_DISPENSE_TITLE "Dispense"
#define STR_DISPENSE_HINT "hold knob to run"
#define STR_DISPENSE_ROW "Hold to dispense"
#define STR_DISPENSE_REVERSE "Hold to reverse"
#define STR_DISPENSE_REVERSING "REVERSING"
#define STR_DISPENSE_STARTING "starting..."
#define STR_DISPENSE_RUNNING "DISPENSING"
#define STR_DISPENSE_STOPPING "stopping..."
#define STR_MOTOR_TITLE "Motor control"
#define STR_MOTOR_HINT "hold knob to run"
#define STR_MOTOR_UP "Hold to go up"
#define STR_MOTOR_DOWN "Hold to go down"
#define STR_MOTOR_RUNNING "MOTOR RUNNING"

// ---- Home screen: launcher band words (max 10; "fix by hand" max 11) and toast problems (max 13) ----
#define STR_BAND_OFFLINE "OFFLINE"
#define STR_BAND_SEARCHING "searching"
#define STR_BAND_CHECK "CHECK"
#define STR_BAND_ANSWER "answer?"
#define STR_BAND_ERROR "ERROR"
#define STR_BAND_FIX "fix by hand"
#define STR_BAND_PREPARING "PREPARING"
#define STR_BAND_STARTING "starting"
#define STR_BAND_LOWERING "lowering"
#define STR_BAND_LOCKING "locking"
#define STR_BAND_UNWINDING "unwinding"
#define STR_BAND_READY "READY"
#define STR_BAND_TO_LAUNCH "to launch"
#define STR_BAND_LAUNCHED "LAUNCHED"
#define STR_BAND_REARM "RE-ARM"
#define STR_BAND_STAND_CLEAR "stand clear"
#define STR_TOAST_BOARD_OFFLINE "board offline"
#define STR_TOAST_FOG_PROBLEM "fog problem"
#define STR_TOAST_RELAY_PROBLEM "relay problem"

// ---- Launcher check overlay (question + answers max 17/20, help lines max 17) ----
#define STR_CHECK_QUESTION "Anything in bucket?"
#define STR_ANS_EMPTY "Bucket empty"
#define STR_ANS_FULL "Bucket full"
#define STR_ANS_UNSURE "Not sure?"
#define STR_HELP_1 "Look inside the"
#define STR_HELP_2 "bucket: anything"
#define STR_HELP_3 "in it = Full."

// ---- Start-up splash screen (title max 11 characters, subtitle max 16, continue offer max 17) ----
// The subtitle is written in capitals, like the owner's example picture. "push = continue" is shown at the top of the screen, over the picture.
#define STR_SPLASH_TITLE "nut-o-matic"
#define STR_SPLASH_SUBTITLE "PEPPERNUT MAKER"
#define STR_SPLASH_CONTINUE "push = continue"
