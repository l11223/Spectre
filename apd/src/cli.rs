#[cfg(target_os = "android")]
use android_logger::Config;
use anyhow::Result;
use clap::Parser;
#[cfg(target_os = "android")]
use log::LevelFilter;

use crate::{defs, event, module, supercall, utils};

/// System service daemon
#[derive(Parser, Debug)]
#[command(author = "", version = defs::VERSION_CODE, about = "system daemon", long_about = None)]
struct Args {
    // SECURITY: SuperKey is NO LONGER passed via command line (VULN-01).
    // cmdline args are visible to ALL processes via /proc/<pid>/cmdline.
    // Instead, read from env var or a temp file descriptor.
    #[arg(
        short,
        long,
        value_name = "KEY",
        help = "Authentication key (DEPRECATED: use SKEY env var instead)"
    )]
    superkey: Option<String>,
    #[command(subcommand)]
    command: Commands,
}

/// Read SuperKey securely: prefer env var over cmdline argument.
/// The env var is only inherited by child processes (not visible in /proc/cmdline).
/// After reading, immediately clear it from the environment.
fn read_superkey_secure(cli_key: Option<String>) -> Option<String> {
    // Priority 1: Environment variable (safer - not in /proc/cmdline)
    let env_key = std::env::var("SKEY").ok().filter(|k| !k.is_empty());
    if let Some(key) = env_key {
        // SAFETY: remove_var is unsafe in Rust 2024+ due to race conditions.
        // Acceptable here: runs early in single-threaded daemon startup.
        unsafe { std::env::remove_var("SKEY"); }
        return Some(key);
    }
    // Priority 2: Temp file (safest - only accessible by owner)
    let file_key = std::fs::read_to_string("/data/adb/.fk/.skey")
        .ok()
        .map(|k| k.trim().to_string())
        .filter(|k| !k.is_empty());
    if let Some(key) = file_key {
        let _ = std::fs::remove_file("/data/adb/.fk/.skey");
        return Some(key);
    }
    // Fallback: cmdline (legacy, insecure - will be removed in future)
    cli_key
}

#[derive(clap::Subcommand, Debug)]
enum Commands {
    /// Manage system modules
    Module {
        #[command(subcommand)]
        command: Module,
    },

    /// Trigger `post-fs-data` event
    PostFsData,

    /// Trigger `service` event
    Services,

    /// Trigger `boot-complete` event
    BootCompleted,

    /// Start uid listener for synchronizing root list
    UidListener,

    /// SELinux policy Patch tool
    Sepolicy {
        #[command(subcommand)]
        command: Sepolicy,
    },
}

#[derive(clap::Subcommand, Debug)]
enum Module {
    /// Install module <ZIP>
    Install {
        /// module zip file path
        zip: String,
    },

    /// Uninstall module <id>
    Uninstall {
        /// module id
        id: String,
    },

    /// enable module <id>
    Enable {
        /// module id
        id: String,
    },

    /// disable module <id>
    Disable {
        // module id
        id: String,
    },

    /// run action for module <id>
    Action {
        // module id
        id: String,
    },
    /// module lua runner
    Lua {
        // module id
        id: String,
        // lua function
        function: String,
    },
    /// list all modules
    List,
}

#[derive(clap::Subcommand, Debug)]
enum Sepolicy {
    /// Check if sepolicy statement is supported/valid
    Check {
        /// sepolicy statements
        sepolicy: String,
    },
}

pub fn run() -> Result<()> {
    #[cfg(target_os = "android")]
    android_logger::init_once(
        Config::default()
            .with_max_level(LevelFilter::Trace)
            // Stealth: Use a generic Android system tag instead of "APatchD"
            .with_tag("ServiceManager")
            .with_filter(
                android_logger::FilterBuilder::new()
                    .filter_level(LevelFilter::Trace)
                    .filter_module("notify", LevelFilter::Warn)
                    .build(),
            ),
    );

    // Stealth: Disguise process name in /proc/self/comm and /proc/<pid>/cmdline
    #[cfg(target_os = "android")]
    {
        let fake_name = std::ffi::CString::new("servicemanager").unwrap();
        unsafe {
            libc::prctl(libc::PR_SET_NAME, fake_name.as_ptr(), 0, 0, 0);
        }
    }

    #[cfg(not(target_os = "android"))]
    env_logger::init();

    // the kernel executes su with argv[0] = "/system/bin/kp" or "/system/bin/su" or "su" or "kp" and replace it with us
    let arg0 = std::env::args().next().unwrap_or_default();
    if arg0.ends_with("kp") || arg0.ends_with("su") {
        return crate::apd::root_shell();
    }

    let cli = Args::parse();

    // SECURITY (VULN-01): Read SuperKey via secure channel, not cmdline
    let superkey = read_superkey_secure(cli.superkey);

    log::info!("command: {:?}", cli.command);

    if superkey.is_some() {
        supercall::privilege_apd_profile(&superkey);
    }

    let result = match cli.command {
        Commands::PostFsData => event::on_post_data_fs(superkey),

        Commands::BootCompleted => event::on_boot_completed(superkey),

        Commands::UidListener => event::start_uid_listener(),

        Commands::Module { command } => {
            #[cfg(any(target_os = "linux", target_os = "android"))]
            {
                utils::switch_mnt_ns(1)?;
            }
            match command {
                Module::Install { zip } => module::install_module(&zip),
                Module::Uninstall { id } => module::uninstall_module(&id),
                Module::Action { id } => module::run_action(&id),
                Module::Lua { id, function } => module::run_lua(&id, &function, false, true)
                    .map_err(|e| anyhow::anyhow!("{}", e)),
                Module::Enable { id } => module::enable_module(&id),
                Module::Disable { id } => module::disable_module(&id),
                Module::List => module::list_modules(),
            }
        }

        Commands::Sepolicy { command } => match command {
            Sepolicy::Check { sepolicy } => crate::sepolicy::check_rule(&sepolicy),
        },

        Commands::Services => event::on_services(superkey.clone()),
    };

    if let Err(e) = &result {
        log::error!("Error: {:?}", e);
    }
    result
}
