//! Android: stdout/stderr go nowhere by default, so the firmware's printf output is piped to logcat.

use std::ffi::CString;
use std::io::{BufRead, BufReader};
use std::os::fd::FromRawFd;
use std::os::raw::{c_char, c_int};
use std::sync::Once;

#[link(name = "log")]
extern "C" {
    fn __android_log_write(prio: c_int, tag: *const c_char, text: *const c_char) -> c_int;
}

const ANDROID_LOG_INFO: c_int = 4;

pub fn redirect_stdio() {
    static ONCE: Once = Once::new();
    ONCE.call_once(|| unsafe {
        let mut fds = [0 as c_int; 2];
        if libc::pipe(fds.as_mut_ptr()) != 0 {
            return;
        }
        libc::setvbuf(libc_stdout(), std::ptr::null_mut(), libc::_IOLBF, 0);
        libc::dup2(fds[1], 1);
        libc::dup2(fds[1], 2);
        let reader = std::fs::File::from_raw_fd(fds[0]);
        std::thread::spawn(move || {
            let tag = CString::new("glo").unwrap();
            for line in BufReader::new(reader).split(b'\n').map_while(Result::ok) {
                if let Ok(text) = CString::new(line) {
                    __android_log_write(ANDROID_LOG_INFO, tag.as_ptr(), text.as_ptr());
                }
            }
        });
    });
}

unsafe fn libc_stdout() -> *mut libc::FILE {
    extern "C" {
        static mut stdout: *mut libc::FILE;
    }
    stdout
}
