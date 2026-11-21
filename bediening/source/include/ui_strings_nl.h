#pragma once

// =============================================================================
// ALL texts shown on the display (Dutch). Selected by building with -D UI_LANG_NL
// (PlatformIO environment arduino_nano_esp32_nl, see platformio.ini). Same keys and
// maximum lengths as ui_strings_en.h; the comments in that file explain the rules.
// Technical words that drivers and builders use anyway stay English (launcher, check,
// setup, prepare, relais...). Logging and code comments stay English.
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
#define STR_SCAN_LAUNCHER_TITLE "Zoek launcher"
#define STR_SCAN_SWITCHBOARD_TITLE "Zoek schakelbord"
#define STR_FORGET_LAUNCHER_TITLE "Wis launcher?"
#define STR_FORGET_SWITCHBOARD_TITLE "Wis schakelbord?"
#define STR_CHECK_TITLE "! CHECK LAUNCHER !"
#define STR_CHECK_ERROR_TITLE "! LAUNCHERFOUT !"
#define STR_HELP_TITLE "CONTROLEER HANDMATIG"

// ---- Device names (max 17) -------------------------------------------------------
#define STR_LAUNCHER "Launcher"
#define STR_SWITCHBOARD "Schakelbord"

// ---- Menu rows and buttons (max 17) ---------------------------------------------
#define STR_BACK "Terug"
#define STR_CANCEL "Annuleer"
#define STR_OK "OK"
#define STR_MENU_CHECK "Check launcher"
#define STR_MENU_SETUP "Setup"
#define STR_MENU_MAINTENANCE "Onderhoud"
#define STR_SCAN_SELECT "Zoek & kies"
#define STR_FORGET "Wis"
#define STR_YES_FORGET "Ja, wis"
#define STR_SCAN_AGAIN "Opnieuw zoeken"

// ---- Small info lines under a title (max 20) ------------------------------------
#define STR_SETUP_INFO "kies een apparaat"
#define STR_NOT_SET "niet ingesteld"
#define STR_SAVED "ok"      // right side of a setup row (max 5)
#define STR_NONE "geen"        // right side of a setup row (max 5)

// ---- Scan screen (max 20; scanning/found use printf placeholders) ---------------
#define STR_SCAN_STARTING "starten..."
#define STR_SCAN_SCANNING "zoeken %us"
#define STR_SCAN_FOUND "gevonden %u"
#define STR_SCAN_NOTHING "niets gevonden"
#define STR_SCAN_FAILED "zoeken mislukt"
#define STR_SAVE_FAILED "opslaan mislukt"
#define STR_SRC_UUID "UUID"    // how a device was recognised: by its service UUID ...
#define STR_SRC_NAME "naam"    // ... or by its name only (max 4)

// ---- Connection state of a device (max 17) --------------------------------------
#define STR_LINK_NOT_SET "niet ingesteld"
#define STR_LINK_SEARCHING "zoeken"
#define STR_LINK_CONNECTING "verbinden"
#define STR_LINK_CONNECTED "verbonden"
#define STR_LINK_ERROR "fout"
#define STR_ERR_SERVICE_MISSING "service ontbreekt"
#define STR_ERR_CHAR_MISSING "characteristic?"
#define STR_ERR_NOT_WRITABLE "niet schrijfbaar"
#define STR_ERR_SUBSCRIBE "abonneren mislukt"
#define STR_ERR_READ "lezen mislukt"
#define STR_ERR_NO_CLIENT "geen BLE-client"

// ---- Short messages for the driver (max 20) -------------------------------------
#define STR_MSG_LAUNCHED "AFGEVUURD"
#define STR_MSG_LAUNCHER_OFFLINE "launcher offline"
#define STR_MSG_LAUNCHER_STATE_UNKNOWN "launcherstatus ?"
#define STR_MSG_NOT_READY "nog niet klaar"
#define STR_MSG_STILL_PREPARING "nog bezig"
#define STR_MSG_LAUNCHER_ERROR "launcherfout"
#define STR_MSG_CHECK_FIRST "check launcher"
#define STR_MSG_LAUNCH_FAILED "AFVUREN MISLUKT"
#define STR_MSG_PREPARE_REFUSED "prepare geweigerd"
#define STR_MSG_PREPARE_TIMEOUT "prepare timeout"
#define STR_MSG_MAINT_BLOCKS_LAUNCH "in onderhoud"
#define STR_MSG_MAINT_BUSY "launcher bezig"
#define STR_MSG_RELEASE_OK "emmer losgelaten"
#define STR_MSG_RELEASE_FAILED "loslaten mislukt"
#define STR_MSG_DISPENSE_FAILED "doseren mislukt"
#define STR_MSG_DISPENSE_STUCK "dispenser vast?"
#define STR_MSG_MOTOR_FAILED "motorfout"
#define STR_MSG_MOTOR_STUCK "motor vast?"
#define STR_MSG_STILL_STOPPING "stopt nog..."
#define STR_MSG_DURATION_SAVED "tijd opgeslagen"
#define STR_MSG_DURATION_FAILED "opslaan mislukt"
#define STR_MSG_PREPARE_FAILED "PREPARE MISLUKT"

// ---- Maintenance (pauses all automation) ----
#define STR_MAINT_TITLE "Onderhoud"
#define STR_MAINT_INFO "alles gepauzeerd"
#define STR_MAINT_RELEASE "Emmer loslaten"
#define STR_MAINT_DISPENSE "Doseren"
#define STR_MAINT_MOTOR "Motorbesturing"
#define STR_MAINT_DURATION "Doseertijd"
#define STR_DURATION_TITLE "Doseertijd"
#define STR_DURATION_HINT "draai, druk = ok"
#define STR_DURATION_SAVE "Druk = opslaan"
#define STR_DURATION_UNIT "seconden"
#define STR_DURATION_UNSAVED "seconden (nieuw)"
#define STR_DURATION_SAVING "opslaan..."
#define STR_DURATION_READING "lezen..."
#define STR_DURATION_NONE "geen waarde"
#define STR_DISPENSE_LAST "laatst"
#define STR_MAINT_SWITCH "Eindschakelaar"
#define STR_SWITCH_TITLE "Eindschakelaar"
#define STR_SWITCH_HINT "emmer beneden?"
#define STR_SWITCH_PRESSED "INGEDRUKT"
#define STR_SWITCH_RELEASED "LOSGELATEN"
#define STR_SWITCH_NO_READING "GEEN WAARDE"
#define STR_SWITCH_PRESSED_INFO "emmer is onder"
#define STR_SWITCH_RELEASED_INFO "emmer niet onder"
#define STR_SWITCH_NONE_INFO "launcher offline?"
#define STR_RELEASE_TITLE "Emmer loslaten?"
#define STR_RELEASE_INFO "emmer kan vuren!"
#define STR_YES_RELEASE "Ja, loslaten"
#define STR_DISPENSE_TITLE "Doseren"
#define STR_DISPENSE_HINT "knop vasthouden"
#define STR_DISPENSE_ROW "Houd voor doseren"
#define STR_DISPENSE_REVERSE "Houd voor omkeren"
#define STR_DISPENSE_REVERSING "OMKEREN"
#define STR_DISPENSE_STARTING "starten..."
#define STR_DISPENSE_RUNNING "DOSEREN"
#define STR_DISPENSE_STOPPING "stoppen..."
#define STR_MOTOR_TITLE "Motorbesturing"
#define STR_MOTOR_HINT "knop vasthouden"
#define STR_MOTOR_UP "Houd voor omhoog"
#define STR_MOTOR_DOWN "Houd voor omlaag"
#define STR_MOTOR_RUNNING "MOTOR DRAAIT"

// ---- Home screen: launcher band words (max 10; "fix by hand" max 11) and toast problems (max 13) ----
#define STR_BAND_OFFLINE "OFFLINE"
#define STR_BAND_SEARCHING "zoeken"
#define STR_BAND_CHECK "CHECK"
#define STR_BAND_ANSWER "antwoord?"
#define STR_BAND_ERROR "FOUT"
#define STR_BAND_FIX "handmatig"
#define STR_BAND_PREPARING "KLAARMAKEN"
#define STR_BAND_STARTING "starten"
#define STR_BAND_LOWERING "zakken"
#define STR_BAND_LOCKING "borgen"
#define STR_BAND_UNWINDING "afwikkelen"
#define STR_BAND_READY "KLAAR"
#define STR_BAND_TO_LAUNCH "voor launch"
#define STR_BAND_LAUNCHED "AFGEVUURD"
#define STR_BAND_REARM "HERLADEN"
#define STR_BAND_STAND_CLEAR "achteruit!"
#define STR_TOAST_BOARD_OFFLINE "bord offline"
#define STR_TOAST_FOG_PROBLEM "rookprobleem"
#define STR_TOAST_RELAY_PROBLEM "relaisprobleem"

// ---- Launcher check overlay (question + answers max 17/20, help lines max 17) ----
#define STR_CHECK_QUESTION "Iets in de emmer?"
#define STR_ANS_EMPTY "Emmer leeg"
#define STR_ANS_FULL "Emmer vol"
#define STR_ANS_UNSURE "Niet zeker?"
#define STR_HELP_1 "Kijk in de emmer:"
#define STR_HELP_2 "zit er iets in,"
#define STR_HELP_3 "kies dan Vol."

// ---- Start-up splash screen (title max 11 characters, subtitle max 16, continue offer max 17) ----
// The subtitle is written in capitals, like the owner's example picture. "push = continue" is shown at the top of the screen, over the picture.
#define STR_SPLASH_TITLE "noot-o-maat"
#define STR_SPLASH_SUBTITLE "PEPERNOTEN MAKER"
#define STR_SPLASH_CONTINUE "druk = doorgaan"
