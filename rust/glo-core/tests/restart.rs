//! Firmware restarts: moving a channel to the front, unlocking all channels, factory reset.
mod common;
use std::thread::sleep;
use std::time::Duration;

const ALL_CHANNELS: i32 = 25;

fn assert_plays(glo: &common::Glo, what: &str) {
    let w = glo.listen(Duration::from_millis(1000));
    assert!(!w.non_finite && w.peak <= 1.0 && w.blocks > 150, "{what}: {w:?}");
}

fn wait_restarted(glo: &common::Glo, what: &str) {
    sleep(Duration::from_millis(500));
    glo.wait_until(Duration::from_secs(15), what, || glo_core::is_powered() && glo_core::channel() == 0);
    sleep(Duration::from_secs(1));
}

/// Taps U1 until the order wraps back to channel 1 and returns the channel ids seen.
fn order(glo: &common::Glo) -> Vec<i32> {
    let mut ids = vec![glo_core::channel_id()];
    while glo.next_channel() != 0 {
        ids.push(glo_core::channel_id());
        assert!(ids.len() <= ALL_CHANNELS as usize, "order does not wrap: {ids:?}");
    }
    ids
}

#[test]
fn restarts() {
    let glo = common::Glo::boot("restart");
    let default_order: Vec<i32> = (0..12).collect();
    assert_eq!(order(&glo), default_order);

    for _ in 0..3 {
        glo.next_channel();
    }
    glo_core::set_button(glo_core::BUTTON_PWR, true);
    glo_core::set_button(glo_core::BUTTON_U2, true);
    sleep(Duration::from_millis(2500));
    glo_core::set_button(glo_core::BUTTON_U2, false);
    glo_core::set_button(glo_core::BUTTON_PWR, false);
    glo.wait_until(Duration::from_secs(5), "Fire moved to the front", || glo_core::channel() == 0);
    sleep(Duration::from_secs(2));
    assert_eq!(glo_core::channel_id(), 3, "Fire plays as channel 1");
    assert_plays(&glo, "after moving Fire to the front");
    assert_eq!(order(&glo), [3, 0, 1, 2, 4, 5, 6, 7, 8, 9, 10, 11]);

    glo_core::set_all_channels_unlocked(true);
    wait_restarted(&glo, "restart after unlocking");
    assert!(glo_core::all_channels_unlocked());
    assert_plays(&glo, "after unlocking");
    let unlocked = order(&glo);
    assert_eq!(unlocked.len(), ALL_CHANNELS as usize, "unlocked order {unlocked:?}");
    assert_eq!(unlocked[..12], default_order, "unlocking resets the order: {unlocked:?}");

    let v0 = glo.settled_volume().expect("muted");
    glo.tap("ccccc");
    glo.wait_decoded();
    assert_eq!(glo.settled_volume(), Some(v0 - 6.0));

    glo.tap("abbbccca");
    wait_restarted(&glo, "restart after factory reset");
    assert!(!glo_core::all_channels_unlocked(), "factory reset locks the extra channels");
    assert_eq!(glo.settled_volume(), Some(v0), "factory reset restores the volume");
    assert_plays(&glo, "after factory reset");
    assert_eq!(order(&glo), default_order);

    glo_core::set_all_channels_unlocked(false);
    wait_restarted(&glo, "plain restart");
    assert_plays(&glo, "after plain restart");
    assert_eq!(order(&glo), default_order);
}
