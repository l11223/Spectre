<div align="center">

# 👻 Spectre

**Kernel-Level Stealth Root Solution**

*Invisible. Unstoppable. Undetectable.*

[![Build](https://github.com/l11223/Spectre/actions/workflows/build.yml/badge.svg)](https://github.com/l11223/Spectre/actions)
[![Release](https://img.shields.io/github/v/release/l11223/Spectre?color=00E5CC&label=Latest)](https://github.com/l11223/Spectre/releases)
[![License](https://img.shields.io/badge/License-GPL%20v2-blue.svg)](LICENSE)

---

**Spectre** is a next-generation Android root framework built for maximum stealth.
Based on KernelPatch + APatch architecture, redesigned from the ground up to be invisible to anti-cheat engines, banking apps, and integrity checks.

</div>

---

## Features

### Spectre Manager App

- One-click boot image patching with AVB chain signing
- SuperUser permission management with per-app control
- Module system (Magisk-compatible `.sp_ext` modules)
- KPM (Kernel Patch Module) loader
- Custom SU path — change it to anything you want
- Built-in update checker
- App title & icon customization

### ghost.kpm — Kernel Stealth Module

The core of Spectre's undetectability. **15 active kernel hooks** that intercept and filter every known root detection vector at the lowest level:

| # | Hook | What It Does |
|---|------|-------------|
| 1 | `filldir64` | Hides `/data/adb/` hidden directories from `ls` / `readdir` |
| 2 | `avc_denied` | SELinux bypass for root processes (uid=0 only) |
| 3 | `audit_log_start` | Suppresses AVC / SELinux audit log entries |
| 4 | `show_map_vma` | Filters `/proc/pid/maps` — removes root library traces |
| 5 | `show_mountinfo` | Filters `/proc/pid/mounts` — hides overlay & module mounts |
| 6 | `proc_pid_status` | Zeros `TracerPid` in `/proc/pid/status` — anti-debug evasion |
| 7 | `proc_pid_wchan` | Hides KP symbols from `/proc/pid/wchan` |
| 8 | `devkmsg_read` | Filters KernelPatch traces from `dmesg` / `/dev/kmsg` |
| 9 | `s_show` | Hides KP symbols from `/proc/kallsyms` |
| 10 | `__arm64_sys_newuname` | Spoofs `uname -r` AND version field (consistent) |
| 11 | `version_proc_show` | Spoofs `/proc/version` output |
| 12 | `do_faccessat` | Hides su/root paths from `access()` syscall |
| 13 | `vfs_fstatat` | Hides su/root paths from `stat()` syscall |
| 14 | `do_sys_openat2` | Hides su/root paths from `open()` syscall |
| 15 | `selinux_getprocattr` | Masks root SELinux context in `/proc/pid/attr/current` |

**22 hidden paths** including all known su locations, Magisk/KSU/APatch directories, and Spectre's own obfuscated dirs.

### Stealth Architecture

```
┌─────────────────────────────────────────────────┐
│                 User Space                       │
│  Game / Bank App                                 │
│    ├── access("/system/bin/su") → ENOENT ✗      │
│    ├── stat("/data/adb/.fk")   → ENOENT ✗      │
│    ├── open("/sbin/su")        → ENOENT ✗      │
│    ├── read /proc/self/maps    → [filtered]     │
│    ├── read /proc/self/mounts  → [filtered]     │
│    ├── read /proc/self/status  → TracerPid: 0   │
│    ├── read /proc/kallsyms     → [no KP syms]   │
│    ├── read /proc/self/attr    → u:r:sh:s0      │
│    └── uname -r                → 6.1.75-pixel   │
├─────────────────────────────────────────────────┤
│              ghost.kpm (Kernel)                  │
│         15 hooks × 22 hidden paths              │
│         All detection vectors blocked            │
└─────────────────────────────────────────────────┘
```

---

## Target Device

| | |
|---|---|
| **Device** | Lenovo Legion Y700 4th Gen |
| **Model** | TB322FC |
| **SoC** | Snapdragon 8 Elite (SM8650) |
| **Kernel** | 6.6.56-android15 |
| **OS** | Android 15 / ZUI OS |
| **Bootloader** | Locked (EDL/9008 flash supported) |

---

## Quick Start

### 1. Install Spectre App

Download the latest APK from [Releases](https://github.com/l11223/Spectre/releases) and install it.

### 2. Patch Boot Image

- Extract your stock `boot.img`
- Open Spectre App → Patch → Select boot image
- Flash the patched image (supports locked BL via EDL)

### 3. Load ghost.kpm

- Download `ghost.kpm` from Releases
- Open Spectre App → KPM → Load Module
- All 15 stealth hooks activate automatically

### 4. Configure

- **Change SU path**: Settings → Reset SU Path → Enter custom path
- **Spoof kernel version**: ghost.kpm auto-spoofs to Pixel 9 Pro defaults
- **Exclude apps**: SuperUser → Select app → Exclude from root

---

## Build from Source

### Prerequisites

- JDK 17+
- Android NDK r25c+
- Rust toolchain with `aarch64-linux-android` target
- `cargo-ndk`

### Build

```bash
# Clone
git clone https://github.com/l11223/Spectre.git
cd Spectre

# Build APK + KPM
./gradlew assembleRelease
```

GitHub Actions CI builds both APK and ghost.kpm automatically on every push.

---

## Project Structure

```
Spectre/
├── app/                    # Android manager app (Kotlin/Compose)
│   ├── src/main/cpp/       # JNI bridge to KernelPatch supercall
│   └── src/main/java/      # UI, patching, su management
├── apd/                    # Root daemon (Rust)
│   └── src/
│       ├── cli.rs          # Entry point, process disguise
│       ├── apd.rs          # Root shell handler
│       ├── supercall.rs    # Kernel supercall interface
│       └── defs.rs         # Obfuscated path definitions
├── kpm/ghost/              # Kernel stealth module (C)
│   └── src/ghost_main.c   # 15 kernel hooks
└── .github/workflows/     # CI/CD
```

---

## Acknowledgments

Built on the shoulders of giants:

- [KernelPatch](https://github.com/bmax121/KernelPatch) — Kernel patching framework
- [APatch](https://github.com/bmax121/APatch) — Android patching architecture
- [FolkPatch](https://github.com/pomelohan/FolkPatch) — Fork base

---

<div align="center">

*"The best root is the one nobody knows exists."*

**GPL v2** · Made with ☕ and sleepless nights

</div>
