//! Drum kit: tilting past the threshold in each direction fires its drum (LED colour), and only again after returning.
mod common;
use std::time::Duration;

const DRUM_KIT: i32 = 7;
const UPRIGHT: [f32; 3] = [0.0, -1.0, 0.0];

fn tilt(a: [f32; 3]) {
    glo_core::set_accel(a[0], a[1], a[2]);
}

#[test]
fn drums_follow_tilt() {
    let glo = common::Glo::boot("drums");
    tilt(UPRIGHT);
    while glo_core::channel() != DRUM_KIT {
        glo.next_channel();
    }
    glo.wait_until(Duration::from_secs(5), "LED off while upright", || glo_core::led() == [false; 3]);
    let quiet = glo.listen(Duration::from_millis(500));

    let drums: [([f32; 3], [bool; 3]); 4] = [
        ([-0.7, -0.7, 0.0], [true, false, false]),
        ([0.7, -0.7, 0.0], [false, true, false]),
        ([0.0, -0.7, -0.7], [false, false, true]),
        ([0.0, -0.7, 0.7], [true, true, true]),
    ];
    for _ in 0..2 {
        for (a, led) in drums {
            tilt(a);
            glo.wait_until(Duration::from_secs(5), &format!("drum LED {led:?} at {a:?}"), || glo_core::led() == led);
            let hit = glo.listen(Duration::from_millis(300));
            eprintln!("tilt {a:?}: quiet {quiet:?}, hit {hit:?}");
            assert!(!hit.non_finite && hit.peak <= 1.0 && hit.blocks > 40, "drum at {a:?}: {hit:?}");
            if std::env::var_os("GLO_TEST_SAMPLES").is_some() {
                assert!(hit.rms > quiet.rms * 2.0 + 1e-4, "drum at {a:?} is silent: {hit:?}");
            }
            tilt(UPRIGHT);
            glo.wait_until(Duration::from_secs(5), "drum released", || glo_core::led() == [false; 3]);
        }
    }
    assert_eq!(glo_core::channel(), DRUM_KIT);
}
