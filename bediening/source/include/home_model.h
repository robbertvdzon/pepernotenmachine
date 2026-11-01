#pragma once

// Pure model of the home screen: turns what the controller knows (launcher status, raw preparation
// state, LED-like states of the functions, messages) into WHAT to show: which band animation, how
// far the progress bar is, which tiles are lit and which toast. No LVGL, no Arduino.
//
// Truth rules: the band follows the real launcher state. The padlock animation plays when the
// launcher is SEEN to switch from "moving down" to "up"; the rocket flies only after the launch
// write was reported OK (until then the band keeps showing READY). Progress bars are time based
// (home_config.h) and only decorate: nothing is decided from them.

#include <stdint.h>

#include "control_logic.h"

enum class BandState : uint8_t { Offline, Check, Error, Starting, Down, Lock, Up, Ready, Launched, Rearm };
enum class TileState : uint8_t { Off, On, Active, Pending, Problem };
enum class ToastLevel : uint8_t { None, Warn, Error };

enum TileId : uint8_t { TILE_FOG, TILE_PANIC, TILE_FLASH, TILE_LAMP, TILE_SIREN, TILE_AUTO, TILE_DISPENSER, TILE_COUNT };

struct HomeInputs {
    uint32_t now = 0;

    LauncherStatus launcher = LauncherStatus::Offline;
    bool prepValid = false;
    uint8_t prep = 0;  // 0 idle, 1 moving down, 2 up slowly, 3 up quickly, 4 error
    uint32_t rearmInMs = 0;
    bool launchPending = false;  // launch sent, outcome not known yet
    uint8_t launchOkCount = 0;   // grows by one whenever a launch write was reported OK

    bool switchboardConnected = false;
    led_state_t fogLed = LED_OFF;  // the same states as the LEDs (L8, L6, L7, L4) and the siren relay
    led_state_t panicLed = LED_OFF;
    led_state_t flashLed = LED_OFF;
    led_state_t lampLed = LED_OFF;
    led_state_t sirenLed = LED_OFF;
    bool dispenserValid = false;
    bool dispenserOn = false;

    Msg message = Msg::None;  // latest message and when it appeared
    uint32_t messageAt = 0;
};

struct HomeView {
    BandState band = BandState::Offline;
    uint32_t bandSince = 0;  // when the current band state began (animation clock)
    uint16_t progress = 0;   // 0..1000, only for Starting / Down / Lock / Up
    uint8_t secsLeft = 0;    // Rearm countdown
    TileState tile[TILE_COUNT] = {TileState::Off, TileState::Off, TileState::Off, TileState::Off,
                                  TileState::Off, TileState::Off, TileState::Off};
    char toast[24] = {0};
    ToastLevel toastLevel = ToastLevel::None;
    uint32_t toastSince = 0;
};

class HomeModel {
   public:
    void update(const HomeInputs& in, HomeView& out);

   private:
    enum class Phase : uint8_t { None, Down, Up };
    void updateTiles(const HomeInputs& in, HomeView& out);
    void updateToast(const HomeInputs& in, HomeView& out);

    Phase phase_ = Phase::None;
    uint32_t phaseSince_ = 0;
    bool lockPlayed_ = false;
    uint8_t lastOkCount_ = 0;
    bool launchedValid_ = false;
    uint32_t launchedAt_ = 0;
    BandState lastBand_ = BandState::Offline;
    uint32_t bandSince_ = 0;
    bool boardOff_ = false;
    uint32_t boardOffSince_ = 0;
    char lastToast_[24] = {0};
    ToastLevel lastToastLevel_ = ToastLevel::None;
    uint32_t toastSince_ = 0;
};
