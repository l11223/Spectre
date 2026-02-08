use const_format::concatcp;

// Stealth: Obfuscated directory names to avoid game anti-cheat filesystem scanning.
// Games commonly scan for /data/adb/, /data/adb/modules/, /data/adb/ap/ etc.
// Using generic-looking directory names that blend with normal Android data.
pub const ADB_DIR: &str = "/data/adb/";
pub const WORKING_DIR: &str = concatcp!(ADB_DIR, ".fk/");          // was "ap/"
pub const BINARY_DIR: &str = concatcp!(WORKING_DIR, "lib/");       // was "bin/"
pub const LOG_FOLDER: &str = concatcp!(WORKING_DIR, "tmp/"); // was "log/"

pub const AP_RC_PATH: &str = concatcp!(WORKING_DIR, ".rc");        // was ".aprc"
pub const GLOBAL_NAMESPACE_FILE: &str = concatcp!(ADB_DIR, ".ns");  // was ".global_namespace_enable"
pub const MAGIC_MOUNT_FILE: &str = concatcp!(ADB_DIR, ".mm");       // was ".magic_mount_enable"
pub const DAEMON_PATH: &str = concatcp!(ADB_DIR, ".core");          // was "apd"

pub const MODULE_DIR: &str = concatcp!(ADB_DIR, ".sp_ext/");        // hidden dir, synced with ghost.kpm
pub const AP_MAGIC_MOUNT_SOURCE: &str = concatcp!(WORKING_DIR, "mnt");

// warning: this directory should not change, or you need to change the code in module_installer.sh!!!
pub const MODULE_UPDATE_DIR: &str = concatcp!(ADB_DIR, ".sp_ext_up/"); // hidden dir, synced with ghost.kpm

pub const TEMP_DIR: &str = "/debug_ramdisk";
pub const TEMP_DIR_LEGACY: &str = "/sbin";

pub const MODULE_WEB_DIR: &str = "webroot";
pub const MODULE_ACTION_SH: &str = "action.sh";
pub const DISABLE_FILE_NAME: &str = "disable";
pub const SKIP_MOUNT_FILE_NAME: &str = "skip_mount";
pub const UPDATE_FILE_NAME: &str = "update";
pub const REMOVE_FILE_NAME: &str = "remove";

// Metamodule support
pub const METAMODULE_MOUNT_SCRIPT: &str = "metamount.sh";
pub const METAMODULE_METAINSTALL_SCRIPT: &str = "metainstall.sh";
pub const METAMODULE_METAUNINSTALL_SCRIPT: &str = "metauninstall.sh";
pub const METAMODULE_DIR: &str = concatcp!(ADB_DIR, ".meta/");     // synced with ghost.kpm
pub const CONFIG_DIR: &str = concatcp!(ADB_DIR, ".sp_cfg/");       // synced with ghost.kpm

pub const PTS_NAME: &str = "pts";

pub const VERSION_CODE: &str = include_str!(concat!(env!("OUT_DIR"), "/VERSION_CODE"));
pub const VERSION_NAME: &str = include_str!(concat!(env!("OUT_DIR"), "/VERSION_NAME"));
