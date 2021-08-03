use block::{Block, ConcreteBlock};
use clap::{AppSettings, Clap};
use libloading::{Library, Symbol};
use std::ffi::CString;
use std::os::raw::c_char;
use std::path::PathBuf;

const VERSION: &'static str = env!("CARGO_PKG_VERSION");
const AUTHOR: &'static str = env!("CARGO_PKG_AUTHORS");

#[derive(Clap)]
#[clap(version = VERSION, author = AUTHOR)]
#[clap(setting = AppSettings::ColoredHelp)]
struct Opts {
    /// The shared cache file to extract from
    #[clap(parse(from_os_str))]
    shared_cache_path: PathBuf,
    /// The root output directory for the extracted libraries
    #[clap(parse(from_os_str))]
    output_path: PathBuf,
}

fn fail<S: Into<String>>(message: S) -> ! {
    eprintln!("{}", message.into());
    std::process::exit(1)
}

fn path_to_cstring(path: PathBuf) -> CString {
    use std::os::unix::ffi::OsStrExt;
    CString::new(path.as_os_str().as_bytes()).unwrap()
}

fn get_library_path() -> PathBuf {
    let output = std::process::Command::new("xcrun")
        .arg("--sdk")
        .arg("iphoneos")
        .arg("--show-sdk-platform-path")
        .output()
        .unwrap();
    if output.status.success() {
        let mut path = PathBuf::from(String::from_utf8(output.stdout).unwrap().trim());
        path.push("usr/lib/dsc_extractor.bundle");
        path
    } else {
        fail("error: failed to fetch platform path from xcrun")
    }
}

fn extract_shared_cache(library_path: PathBuf, input_path: CString, output_path: CString) {
    let progress_block = ConcreteBlock::new(|x, y| println!("extracted {}/{}", x, y));
    unsafe {
        let library = Library::new(library_path).unwrap();
        let func: Symbol<
            unsafe extern "C" fn(
                input_path: *const c_char,
                output_path: *const c_char,
                progress: &Block<(usize, usize), ()>,
            ),
        > = library
            .get(b"dyld_shared_cache_extract_dylibs_progress")
            .unwrap();
        func(input_path.as_ptr(), output_path.as_ptr(), &progress_block);
    }
}

fn main() {
    let opts: Opts = Opts::parse();

    if !opts.shared_cache_path.exists() {
        fail(format!(
            "error: shared cache path doesn't exist: {}",
            opts.shared_cache_path.to_str().unwrap()
        ));
    }

    let library_path = get_library_path();
    if !library_path.exists() {
        fail(format!(
            "error: dsc_extractor.bundle wasn't found at expected path, Xcode might have changed this location: {}",
            library_path.to_str().unwrap()
        ));
    }

    extract_shared_cache(
        library_path,
        path_to_cstring(opts.shared_cache_path),
        path_to_cstring(opts.output_path),
    );
}
