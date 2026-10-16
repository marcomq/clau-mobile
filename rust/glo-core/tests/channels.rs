//! Every default channel starts in order, plays and stays finite; U1 wraps after the 12 default channels.
mod common;
use std::time::Duration;

const DEFAULT_CHANNELS: i32 = 12;
/// Channels that only process the microphone or wait for a gesture (reverb, granular, drum kit, Clouds).
const NEEDS_INPUT: &[i32] = &[4, 5, 7, 9];
/// Starts frozen with an empty buffer (v1.0.118).
const GRANULAR: i32 = 5;

#[test]
fn default_channels_play_in_order() {
    let glo = common::Glo::boot("channels");
    assert_eq!((glo_core::channel(), glo_core::channel_id()), (0, 0), "boots into channel 1");
    assert!(!glo_core::all_channels_unlocked());

    for ch in 0..DEFAULT_CHANNELS {
        assert_eq!((glo_core::channel(), glo_core::channel_id()), (ch, ch), "default channel order");
        let dry = glo.listen(Duration::from_millis(1500));
        if ch == GRANULAR {
            glo.tap("a");
            glo.wait_decoded();
            assert_eq!(glo_core::channel(), ch, "the first U1 in Granular starts sampling instead of switching");
        }
        glo.mic(true);
        let wet = glo.listen(Duration::from_millis(1500));
        glo.mic(false);
        eprintln!("channel {ch}: dry {dry:?}, mic {wet:?}, volume {:?} dB", glo_core::volume_db());
        for w in [dry, wet] {
            assert!(!w.non_finite, "channel {ch} produced NaN/inf");
            assert!(w.peak <= 1.0, "channel {ch} clips: {w:?}");
            assert!(w.blocks > 250, "render stalled in channel {ch}: {w:?}");
        }
        let audible = if NEEDS_INPUT.contains(&ch) && ch != 7 { wet } else { dry };
        if ch != 7 {
            assert!(audible.rms > 1e-3, "channel {ch} is silent: {audible:?}");
        }
        glo.next_channel();
    }
    assert_eq!(glo_core::channel(), 0, "U1 wraps to channel 1 after the default channels");
}
