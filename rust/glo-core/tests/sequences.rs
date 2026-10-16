//! Button sequences from the manual: volume, mic gain limit, nature-channel commands, commands in other channels.
mod common;
use std::thread::sleep;
use std::time::Duration;

fn type_seq(glo: &common::Glo, seq: &str) {
    glo.tap(seq);
    glo.wait_decoded();
    sleep(Duration::from_millis(500));
}

fn assert_plays(glo: &common::Glo, what: &str) {
    let w = glo.listen(Duration::from_millis(400));
    assert!(!w.non_finite && w.peak <= 1.0 && w.blocks > 60, "{what}: {w:?}");
}

#[test]
fn sequences() {
    let glo = common::Glo::boot("sequences");

    let v0 = glo.settled_volume().expect("muted at boot");
    eprintln!("volume at boot {v0} dB");
    type_seq(&glo, "ccccc");
    assert_eq!(glo.settled_volume(), Some(v0 - 6.0), "5x U3 lowers the volume by 6 dB");
    type_seq(&glo, "bbbbb");
    assert_eq!(glo.settled_volume(), Some(v0), "5x U2 raises it again");

    let agc = glo_core::mic_agc_db().expect("mic AGC off at boot");
    eprintln!("mic AGC limit at boot {agc} dB");
    type_seq(&glo, "bbbbbb");
    assert_eq!(glo_core::mic_agc_db(), Some(agc + 12), "6x U2 raises the mic gain limit");
    type_seq(&glo, "cccccc");
    assert_eq!(glo_core::mic_agc_db(), Some(agc), "6x U3 lowers it again");

    for seq in ["c", "b", "bb", "ccc", "bbb", "cccc", "bbbb"] {
        type_seq(&glo, seq);
        glo.wait_until(Duration::from_secs(3), &format!("Earth picks up {seq}"), || glo_core::pending_commands() == 0);
        assert_plays(&glo, &format!("Earth after {seq}"));
    }

    type_seq(&glo, "bcbcbc");
    type_seq(&glo, "bcbcbc");
    assert_plays(&glo, "key beeps toggled twice");

    for ch in 1..12 {
        glo.next_channel();
        for seq in ["bb", "ccc", "bc"] {
            type_seq(&glo, seq);
            assert_eq!(glo_core::channel(), ch, "{seq} must not change the channel");
            assert_plays(&glo, &format!("channel {ch} after {seq}"));
        }
    }
}
