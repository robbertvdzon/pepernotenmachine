#include "home_model.h"

#include <stdio.h>
#include <string.h>

#include "home_config.h"
#include "ui_strings.h"

namespace {

inline uint32_t elapsed(uint32_t now, uint32_t since) { return static_cast<uint32_t>(now - since); }
inline uint32_t min_u32(uint32_t a, uint32_t b) { return a < b ? a : b; }

// LED state -> tile state. `on_is_active`: a steady LED is shown with the animated icon.
TileState from_led(led_state_t s, bool on_is_active) {
    switch (s) {
        case LED_ON: return on_is_active ? TileState::Active : TileState::On;
        case LED_PULSE: return TileState::Active;
        case LED_FLASH: return TileState::Pending;
        case LED_FLASH_FAST: return TileState::Problem;
        default: return TileState::Off;
    }
}

bool is_error_message(Msg m) { return m == Msg::LaunchFailed || m == Msg::PrepareFailed || m == Msg::LaunchError; }

}  // namespace

void HomeModel::update(const HomeInputs& in, HomeView& v) {
    const uint32_t now = in.now;

    // The rocket flies when a launch write was reported OK.
    if (in.launchOkCount != lastOkCount_) {
        lastOkCount_ = in.launchOkCount;
        launchedValid_ = true;
        launchedAt_ = now;
    }
    if (launchedValid_ && elapsed(now, launchedAt_) >= LAUNCHED_ANIM_MS) launchedValid_ = false;

    // Down / up phase of a running preparation. The padlock belongs to the moment the launcher
    // is seen to switch from down to up; when it is first seen already going up there is no lock.
    const bool running = in.launcher == LauncherStatus::Preparing && in.prepValid && in.prep >= 1 && in.prep <= 3;
    if (!running) {
        phase_ = Phase::None;
    } else if (in.prep == 1) {
        if (phase_ != Phase::Down) {
            phase_ = Phase::Down;
            phaseSince_ = now;
        }
    } else if (phase_ != Phase::Up) {
        lockPlayed_ = (phase_ == Phase::Down);
        phase_ = Phase::Up;
        phaseSince_ = now;
    }

    BandState band = BandState::Offline;
    uint32_t progress = 0;
    switch (in.launcher) {
        case LauncherStatus::Offline: band = BandState::Offline; break;
        case LauncherStatus::NeedsCheck: band = BandState::Check; break;
        case LauncherStatus::Error: band = BandState::Error; break;
        case LauncherStatus::Ready: band = BandState::Ready; break;
        case LauncherStatus::Rearming:
            // The countdown starts at the button press; the rocket only leaves once the write is confirmed.
            band = in.launchPending ? BandState::Ready : (launchedValid_ ? BandState::Launched : BandState::Rearm);
            break;
        case LauncherStatus::Preparing:
            if (launchedValid_) {
                band = BandState::Launched;
            } else if (phase_ == Phase::Down) {
                band = BandState::Down;
                progress = min_u32(elapsed(now, phaseSince_), PREP_DOWN_MS) * 1000u / PREP_DOWN_MS;
            } else if (phase_ == Phase::Up) {
                const uint32_t el = elapsed(now, phaseSince_);
                const uint32_t lock = lockPlayed_ ? LOCK_ANIM_MS : 0;
                if (el < lock) {
                    band = BandState::Lock;  // the bar starts after the lock animation
                } else {
                    band = BandState::Up;
                    progress = min_u32(el - lock, PREP_UP_MS - lock) * 1000u / (PREP_UP_MS - lock);
                }
            } else {
                band = BandState::Starting;
            }
            break;
    }

    if (band != lastBand_) {
        lastBand_ = band;
        bandSince_ = (band == BandState::Lock) ? phaseSince_ : now;
    }
    v.band = band;
    v.bandSince = bandSince_;
    v.progress = static_cast<uint16_t>(progress > 1000 ? 1000 : progress);
    v.secsLeft = static_cast<uint8_t>(min_u32((in.rearmInMs + 999) / 1000, 9));

    updateTiles(in, v);
    updateToast(in, v);
}

void HomeModel::updateTiles(const HomeInputs& in, HomeView& v) {
    v.tile[TILE_FOG] = from_led(in.fogLed, false);  // steady = ready, pulsing = fogging
    v.tile[TILE_PANIC] = from_led(in.panicLed, true);
    v.tile[TILE_FLASH] = from_led(in.flashLed, true);
    v.tile[TILE_LAMP] = from_led(in.lampLed, true);
    v.tile[TILE_SIREN] = from_led(in.sirenLed, true);
    v.tile[TILE_AUTO] = TileState::Off;  // SW5 is not defined yet
    v.tile[TILE_DISPENSER] = (in.dispenserValid && in.dispenserOn) ? TileState::Active : TileState::Off;
}

// One slot at the bottom: a fresh message ("event", a few seconds) wins over a standing problem
// ("condition", shown while it is true; the first one plus "+N").
void HomeModel::updateToast(const HomeInputs& in, HomeView& v) {
    const uint32_t now = in.now;
    char text[24] = {0};
    ToastLevel level = ToastLevel::None;

    const bool fresh = in.message != Msg::None && in.message != Msg::LaunchSent &&
                       elapsed(now, in.messageAt) < MESSAGE_SHOW_MS;
    if (fresh) {
        snprintf(text, sizeof(text), "%s", msg_text(in.message));
        level = is_error_message(in.message) ? ToastLevel::Error : ToastLevel::Warn;
    } else {
        if (!in.switchboardConnected) {
            if (!boardOff_) {
                boardOff_ = true;
                boardOffSince_ = now;
            }
        } else {
            boardOff_ = false;
        }
        const bool boardShow = boardOff_ && elapsed(now, boardOffSince_) >= BOARD_OFFLINE_TOAST_MS;

        const char* names[3];
        ToastLevel levels[3];
        int n = 0;
        if (boardShow) {  // everything else on the switchboard is a consequence of this
            names[n] = STR_TOAST_BOARD_OFFLINE;
            levels[n++] = ToastLevel::Error;
        } else if (in.switchboardConnected) {
            if (v.tile[TILE_FOG] == TileState::Problem) {
                names[n] = STR_TOAST_FOG_PROBLEM;
                levels[n++] = ToastLevel::Warn;
            }
            if (v.tile[TILE_PANIC] == TileState::Problem || v.tile[TILE_FLASH] == TileState::Problem ||
                v.tile[TILE_LAMP] == TileState::Problem || v.tile[TILE_SIREN] == TileState::Problem) {
                names[n] = STR_TOAST_RELAY_PROBLEM;
                levels[n++] = ToastLevel::Warn;
            }
        }
        if (n > 0) {
            if (n == 1) snprintf(text, sizeof(text), "%s", names[0]);
            else snprintf(text, sizeof(text), "%s +%d", names[0], n - 1);
            level = levels[0];
        }
    }

    if (level != lastToastLevel_ || strcmp(text, lastToast_) != 0) {
        lastToastLevel_ = level;
        strncpy(lastToast_, text, sizeof(lastToast_) - 1);
        lastToast_[sizeof(lastToast_) - 1] = '\0';
        toastSince_ = now;
    }
    memcpy(v.toast, lastToast_, sizeof(v.toast));
    v.toastLevel = lastToastLevel_;
    v.toastSince = toastSince_;
}
