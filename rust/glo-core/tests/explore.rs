mod common;
use std::thread::sleep;
use std::time::{Duration, Instant};

fn trace(glo: &common::Glo, seq: &str) {
    glo.tap(seq);
    glo.wait_decoded();
    let t0 = Instant::now();
    let mut log = String::new();
    while t0.elapsed() < Duration::from_secs(3) {
        log.push_str(&glo_core::pending_commands().to_string());
        sleep(Duration::from_millis(100));
    }
    eprintln!("## id {} seq {seq}: pending every 100 ms {log}", glo_core::channel_id());
}

#[test]
#[ignore]
fn explore() {
    let glo = common::Glo::boot("explore");
    trace(&glo, "bbb");
    trace(&glo, "b");
    for _ in 0..3 {
        glo.next_channel();
    }
    trace(&glo, "bbb");
    trace(&glo, "b");
    trace(&glo, "bb");
    for _ in 0..5 {
        glo.next_channel();
    }
    trace(&glo, "bbb");
    trace(&glo, "b");
}
