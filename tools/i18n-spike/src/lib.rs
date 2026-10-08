use fluent_bundle::{FluentArgs, FluentBundle, FluentResource, FluentValue};
use std::collections::HashMap;
use std::ffi::{CStr, CString, c_char};
use std::slice;
use std::sync::{Arc, Mutex};
use unic_langid::LanguageIdentifier;

#[repr(C)]
pub struct FormEntry {
    role: *const c_char,
    form: *const c_char,
    value: *const c_char,
}

struct Catalog {
    bundle: FluentBundle<FluentResource>,
    resolution: Arc<Mutex<Resolution>>,
}

struct Resolution {
    forms: HashMap<(String, String), String>,
    missing: bool,
}

unsafe fn read_string(pointer: *const c_char) -> Option<String> {
    if pointer.is_null() {
        return None;
    }
    unsafe { CStr::from_ptr(pointer) }
        .to_str()
        .ok()
        .map(str::to_owned)
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn fluent_catalog_new(
    locale: *const c_char,
    resource: *const c_char,
) -> *mut std::ffi::c_void {
    let Some(locale) = (unsafe { read_string(locale) }) else {
        return std::ptr::null_mut();
    };
    let Some(resource) = (unsafe { read_string(resource) }) else {
        return std::ptr::null_mut();
    };
    let Ok(language) = locale.parse::<LanguageIdentifier>() else {
        return std::ptr::null_mut();
    };
    let Ok(resource) = FluentResource::try_new(resource) else {
        return std::ptr::null_mut();
    };
    let resolution = Arc::new(Mutex::new(Resolution {
        forms: HashMap::new(),
        missing: false,
    }));
    let resolver = Arc::clone(&resolution);
    let mut bundle = FluentBundle::new(vec![language]);
    bundle.set_use_isolating(false);
    if bundle.add_resource(resource).is_err() || bundle.add_builtins().is_err() {
        return std::ptr::null_mut();
    }
    if bundle
        .add_function("FORM", move |positional, named| {
            let Some(FluentValue::String(role)) = positional.first() else {
                return FluentValue::Error;
            };
            let Some(FluentValue::String(form)) = named.get("form") else {
                return FluentValue::Error;
            };
            let Ok(mut resolution) = resolver.lock() else {
                return FluentValue::Error;
            };
            if let Some(value) = resolution
                .forms
                .get(&(role.to_string(), form.to_string()))
                .cloned()
            {
                FluentValue::from(value)
            } else {
                resolution.missing = true;
                FluentValue::Error
            }
        })
        .is_err()
    {
        return std::ptr::null_mut();
    }
    Box::into_raw(Box::new(Catalog { bundle, resolution })).cast()
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn fluent_catalog_render(
    catalog: *mut std::ffi::c_void,
    event: *const c_char,
    entries: *const FormEntry,
    entry_count: usize,
    count: *const c_char,
    count_display: *const c_char,
    damage: i32,
) -> *mut c_char {
    if catalog.is_null() || (entry_count != 0 && entries.is_null()) {
        return std::ptr::null_mut();
    }
    let Some(event) = (unsafe { read_string(event) }) else {
        return std::ptr::null_mut();
    };
    let catalog = unsafe { &mut *catalog.cast::<Catalog>() };
    let entries = if entry_count == 0 {
        &[][..]
    } else {
        unsafe { slice::from_raw_parts(entries, entry_count) }
    };
    let mut forms = HashMap::new();
    let mut args = FluentArgs::new();
    for entry in entries {
        let (Some(role), Some(form), Some(value)) = (
            unsafe { read_string(entry.role) },
            unsafe { read_string(entry.form) },
            unsafe { read_string(entry.value) },
        ) else {
            return std::ptr::null_mut();
        };
        args.set(role.clone(), role.as_str().to_owned());
        if form == "gender" {
            args.set(format!("{role}_gender"), value);
        } else {
            forms.insert((role, form), value);
        }
    }
    let Ok(mut resolution) = catalog.resolution.lock() else {
        return std::ptr::null_mut();
    };
    resolution.forms = forms;
    resolution.missing = false;
    drop(resolution);
    if let Some(count) = unsafe { read_string(count) } {
        args.set("count", FluentValue::try_number(&count).into_owned());
    }
    if let Some(count_display) = unsafe { read_string(count_display) } {
        args.set("count_display", count_display);
    }
    if damage >= 0 {
        args.set("damage", damage);
    }
    let Some(message) = catalog.bundle.get_message(&event) else {
        return std::ptr::null_mut();
    };
    let Some(pattern) = message.value() else {
        return std::ptr::null_mut();
    };
    let mut errors = Vec::new();
    let result = catalog.bundle.format_pattern(pattern, Some(&args), &mut errors);
    let Ok(resolution) = catalog.resolution.lock() else {
        return std::ptr::null_mut();
    };
    if !errors.is_empty() || resolution.missing {
        return std::ptr::null_mut();
    }
    CString::new(result.as_ref())
        .map(CString::into_raw)
        .unwrap_or(std::ptr::null_mut())
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn fluent_catalog_free(catalog: *mut std::ffi::c_void) {
    if !catalog.is_null() {
        drop(unsafe { Box::from_raw(catalog.cast::<Catalog>()) });
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn fluent_string_free(value: *mut c_char) {
    if !value.is_null() {
        drop(unsafe { CString::from_raw(value) });
    }
}
