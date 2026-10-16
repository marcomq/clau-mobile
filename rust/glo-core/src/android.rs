//! Android: cpal needs the JavaVM and a Context (via ndk-context) to list audio devices. A GDExtension is
//! loaded with dlopen, so nothing sets them; fetch them ourselves.

use std::ffi::c_void;
use std::sync::OnceLock;

use jni_sys::{jsize, JNIEnv, JavaVM, JNI_OK};

type GetCreatedJavaVMs = unsafe extern "system" fn(*mut *mut JavaVM, jsize, *mut jsize) -> i32;

static READY: OnceLock<bool> = OnceLock::new();

/// True once device enumeration is safe. Needs `JNI_GetCreatedJavaVMs` from libnativehelper (Android 12+).
pub fn init_context() -> bool {
    *READY.get_or_init(|| {
        let ok = unsafe { try_init() }.is_some();
        if !ok {
            eprintln!("glo: no Android context, using default audio routing");
        }
        ok
    })
}

unsafe fn try_init() -> Option<()> {
    let lib = libc::dlopen(c"libnativehelper.so".as_ptr(), libc::RTLD_NOW);
    let sym = if lib.is_null() { return None } else { libc::dlsym(lib, c"JNI_GetCreatedJavaVMs".as_ptr()) };
    if sym.is_null() {
        return None;
    }
    let get_vms: GetCreatedJavaVMs = std::mem::transmute(sym);
    let mut vm: *mut JavaVM = std::ptr::null_mut();
    let mut n: jsize = 0;
    if get_vms(&mut vm, 1, &mut n) != JNI_OK || n < 1 {
        return None;
    }
    let mut env: *mut JNIEnv = std::ptr::null_mut();
    if ((**vm).v1_1.AttachCurrentThread)(vm, (&mut env as *mut *mut JNIEnv).cast(), std::ptr::null_mut()) != JNI_OK {
        return None;
    }
    let f = &(**env).v1_1;
    let cls = (f.FindClass)(env, c"android/app/ActivityThread".as_ptr());
    let mid = if cls.is_null() {
        std::ptr::null_mut()
    } else {
        (f.GetStaticMethodID)(env, cls, c"currentApplication".as_ptr(), c"()Landroid/app/Application;".as_ptr())
    };
    let app = if mid.is_null() { std::ptr::null_mut() } else { (f.CallStaticObjectMethodA)(env, cls, mid, std::ptr::null()) };
    if app.is_null() {
        (f.ExceptionClear)(env);
        return None;
    }
    let global = (f.NewGlobalRef)(env, app);
    (f.DeleteLocalRef)(env, app);
    (f.DeleteLocalRef)(env, cls);
    ndk_context::initialize_android_context(vm as *mut c_void, global as *mut c_void);
    Some(())
}
