<div align="center">

```
   ███████╗██████╗ ███████╗ ██████╗████████╗██████╗ ███████╗
   ██╔════╝██╔══██╗██╔════╝██╔════╝╚══██╔══╝██╔══██╗██╔════╝
   ███████╗██████╔╝█████╗  ██║        ██║   ██████╔╝█████╗  
   ╚════██║██╔═══╝ ██╔══╝  ██║        ██║   ██╔══██╗██╔══╝  
   ███████║██║     ███████╗╚██████╗   ██║   ██║  ██║███████╗
   ╚══════╝╚═╝     ╚══════╝ ╚═════╝   ╚═╝   ╚═╝  ╚═╝╚══════╝
```

**Kernel-Level Stealth Root Framework**

*What can't be seen, can't be detected.*

[![Build](https://github.com/l11223/Spectre/actions/workflows/build.yml/badge.svg)](https://github.com/l11223/Spectre/actions)
[![Release](https://img.shields.io/github/v/release/l11223/Spectre?color=00E5CC&label=Release)](https://github.com/l11223/Spectre/releases)
[![Downloads](https://img.shields.io/github/downloads/l11223/Spectre/total?color=00E5CC&label=Downloads)](https://github.com/l11223/Spectre/releases)
[![License](https://img.shields.io/badge/License-GPL%20v3-blue)](LICENSE)

English · [中文](./README.md)

</div>

---

## Overview

Spectre is an Android root solution built on [KernelPatch](https://github.com/bmax121/KernelPatch), engineered for **kernel-level stealth**.

Traditional root tools (Magisk, KernelSU) fight detection at the application layer. Spectre's ghost.kpm module operates directly in kernel space, intercepting system calls and filtering data before detection code ever reads it. The device presents as completely stock — game anti-cheat engines, banking apps, and Play Integrity cannot perceive root presence.

---

## How It Works

```
┌─────────────────────────────────────────────────┐
│                   Userspace                       │
│                                                   │
│   Game Anti-Cheat        Banking App              │
│   access("/bin/su")      stat("/data/adb")        │
│   → ENOENT               → ENOENT                 │
│   /proc/maps → clean     /proc/attr → u:r:sh:s0  │
│   uname -r → stock       TracerPid → 0            │
├───────────────────────┬───────────────────────────┤
│                       ▼                           │
│              ghost.kpm v5                         │
│                                                   │
│   15 Kernel Hooks  ·  22 Hidden Paths             │
│   Intercept → Filter → Spoof → Return             │
│                                                   │
│                  Kernel Space                      │
└─────────────────────────────────────────────────┘
```

---

## ghost.kpm — Kernel Stealth Module

ghost.kpm is the core of Spectre — approximately 600 lines of pure C, entirely custom-written. It deploys 15 kernel hooks that intercept all known root detection vectors at the syscall level, covering directory enumeration, the /proc filesystem, kernel symbol tables, SELinux auditing, and file existence checks. Every hook is live working code — no placeholders, no stubs.

### Stealth Coverage Summary

| Detection Vector | Hooks | Coverage |
|:-----------------|:------|:---------|
| Directory/file scanning | 4 | `filldir64` + `access()` / `stat()` / `open()` triple interception |
| /proc information leaks | 4 | maps, mounts, status, wchan |
| Kernel info exposure | 4 | dmesg, kallsyms, uname, /proc/version |
| SELinux detection | 2 | AVC bypass + context spoofing |
| Audit logging | 1 | Kernel-level log suppression |
| **Total** | **15** | **Zero blind spots** |

### Core Hiding

| Hook | Target | Effect |
|:-----|:-------|:-------|
| `filldir64` | Directory enumeration | Two-pass filtering: hides 8 obfuscated dirs under `/data/adb/`, plus manager package dirs from `/data/user/0/` and `/data/data/` |
| `avc_denied` | SELinux | Transparent bypass for uid=0 processes, no audit trail |
| `audit_log_start` | Audit logs | Suppresses AVC, SELINUX_ERR, and MAC_STATUS audit records at kernel level |

### /proc Hiding

| Hook | Target | Effect |
|:-----|:-------|:-------|
| `show_map_vma` | `/proc/pid/maps` | Filters 16 keywords: root libraries, zygisk, riru, shamiko, kernelpatch, lspd, module paths, etc. |
| `show_mountinfo` | `/proc/pid/mounts` | Filters 6 keywords: overlay, magisk, KSU, apatch mount entries |
| `proc_pid_status` | `/proc/pid/status` | Forces `TracerPid: 0`, bypassing anti-debug detection |
| `proc_pid_wchan` | `/proc/pid/wchan` | Replaces 7 KP-related symbols with benign `ep_poll` |

### Kernel Info Hiding

| Hook | Target | Effect |
|:-----|:-------|:-------|
| `devkmsg_read` | dmesg/kmsg | Filters 8 keywords, replaces KernelPatch boot traces with harmless records |
| `s_show` | `/proc/kallsyms` | Hides 8 symbol classes: `supercall`, `kp_*`, `kernelpatch`, `ghost_*`, `spoof_*`, `hook_wrap`, `inline_hook`, `kpm_*` |
| `__arm64_sys_newuname` | `uname -r` | Spoofs both release AND version fields (most solutions only patch release) |
| `version_proc_show` | `/proc/version` | Spoofs complete version string, defaults to Pixel 9 Pro kernel signature |

### File Existence Hiding

| Hook | Target | Effect |
|:-----|:-------|:-------|
| `do_faccessat` | `access()` | Returns `-ENOENT` for 22+ root-related paths (non-root processes only) |
| `vfs_fstatat` | `stat()` | Same |
| `do_sys_openat2` | `open()` | Same |

Covered paths include: classic su paths, Spectre directories, APatch directories, Magisk directories, KSU directories, SuperSU paths, and manager app data directories.

### SELinux Context Hiding

| Hook | Target | Effect |
|:-----|:-------|:-------|
| `selinux_getprocattr` | `/proc/pid/attr/current` | In-place replacement: `u:r:su:s0` → `u:r:sh:s0`, `u:r:magisk:s0` → `u:r:system:s0` |

### Runtime Control

ghost.kpm exposes a `ghost_control` interface for live operations:

- **Update spoof values**: Modify uname release, version, and version string fields without rebooting
- **Status query**: Query current ghost.kpm state and loaded spoof values
- **Lifecycle management**: Clean hook registration and teardown via `hook_wrap` / `hook_unwrap`

---

## Improvements over APatch

Spectre builds on APatch / FolkPatch with extensive stealth and security hardening. Every change listed here addresses a real detection vector — nothing is cosmetic.

### 🔒 SuperKey Secure Delivery

Original APatch passes the SuperKey via command-line arguments — meaning **any process** on the system can read it from `/proc/<pid>/cmdline`. This is a real security vulnerability (VULN-01).

Spectre's fix uses a three-tier priority system:

| Priority | Method | Security |
|:---------|:-------|:---------|
| 1 (preferred) | Environment variable `SKEY` | Not visible in `/proc/cmdline`, gone after process exits |
| 2 (fallback) | Temp file `/data/adb/.fk/.skey` | Deleted immediately after read |
| 3 (legacy) | Command-line argument | Kept only for backward compatibility |

### 🗂️ Obfuscated Directory Structure

All working directories renamed from recognizable names to meaningless short names. Anti-cheat engines scanning the filesystem cannot identify root tool presence by path signatures:

| Original Path | Spectre Path | Purpose |
|:--------------|:-------------|:--------|
| `/data/adb/ap/` | `/data/adb/.fk/` | Working directory |
| `/data/adb/ap/bin/` | `/data/adb/.fk/lib/` | Binary directory |
| `/data/adb/ap/log/` | `/data/adb/.fk/tmp/` | Log directory |
| `/data/adb/apd` | `/data/adb/.core` | Daemon |
| `/data/adb/modules/` | `/data/adb/.sp_ext/` | Module directory |
| `/data/adb/modules_update/` | `/data/adb/.sp_ext_up/` | Module update directory |
| — | `/data/adb/.meta/` | Metamodule directory (new) |
| — | `/data/adb/.sp_cfg/` | Config directory (new) |

All obfuscated paths are synced with ghost.kpm's `filldir64` hook — even if userspace hiding is bypassed, the kernel layer blocks directory enumeration.

### 👻 Process Disguise

The daemon sets its process name to `servicemanager` via `PR_SET_NAME`, with the log tag changed to `ServiceManager`. In `ps` output and the `/proc` filesystem, it is indistinguishable from a real Android system service.

### 🧹 Environment Variable Cleanup

Original APatch leaks the following environment variables to child processes:

- `KERNELPATCH`
- `KERNELPATCH_VERSION`
- `KERNEL_VERSION`
- `KP_READY`

Any app can detect these via `/proc/<pid>/environ`. Spectre strips all detectable environment variables when forking child processes.

### 🛡️ SELinux Context Replacement

| | Original APatch | Spectre |
|:--|:----------------|:--------|
| Internal context | `u:r:magisk:s0` | `u:r:su:s0` |
| Externally visible | `u:r:magisk:s0` (directly exposed) | `u:r:sh:s0` (spoofed by ghost.kpm hook) |

`u:r:magisk:s0` is the first thing every root detection tool checks. Spectre eliminates this signature at two levels: `u:r:su:s0` internally, presented as `u:r:sh:s0` externally via ghost.kpm's `selinux_getprocattr` hook.

### 📦 Metamodule System

An entirely new metamodule mechanism with three lifecycle hooks:

| Hook | Trigger | Purpose |
|:-----|:--------|:--------|
| `metamount.sh` | Module mount phase | Control how regular modules are mounted |
| `metainstall.sh` | Module install phase | Custom installation logic |
| `metauninstall.sh` | Module uninstall phase | Cleanup and rollback |

Metamodules support symlink resolution with fallback search, and include safety checks to prevent installation during unstable states.

### 👻 ghost.kpm Kernel Stealth Module

Approximately 600 lines of pure C, entirely custom-developed. 15 active kernel hooks covering every known detection vector, implementing stealth at the syscall level. See the [ghost.kpm section](#ghostkpm--kernel-stealth-module) above for full details.

---

## Manager Features

- One-click boot image patching
- Per-app SuperUser permission management
- Custom SU path configuration
- Magisk-compatible module system (APM)
- KPM kernel module loader with ghost.kpm auto-load on boot
- Obfuscated directory structure, daemon disguised as system service
- Multiple themes, custom app naming

---

## Quick Start

```bash
# 1. Download latest APK + ghost.kpm from Releases
# 2. Install Spectre APK
# 3. Patch boot image via the app
# 4. Flash patched boot via fastboot
# 5. Reboot → Open Spectre → Load ghost.kpm
```

---

## Build from Source

```bash
# Prerequisites: JDK 21+, Android NDK r25c+, Rust + cargo-ndk

git clone https://github.com/l11223/Spectre.git
cd Spectre
./gradlew assembleRelease
```

Build ghost.kpm separately:

```bash
cd kpm/ghost
make NDK_PATH=/path/to/ndk KP_DIR=/path/to/KernelPatch
```

---

## Project Structure

```
Spectre/
├── app/                    # Manager App (Kotlin / Jetpack Compose)
│   ├── src/main/cpp/       # JNI bridge to KernelPatch supercall
│   └── src/main/java/      # UI, patching, SU management
├── apd/                    # Root Daemon (Rust)
│   └── src/
│       ├── apd.rs          # Root shell handler
│       ├── supercall.rs    # Kernel interface
│       └── defs.rs         # Obfuscated path definitions
├── kpm/ghost/              # Kernel Stealth Module (C)
│   └── src/ghost_main.c    # 15 kernel hooks
└── .github/workflows/      # CI/CD
```

---

## Credits

| Project | Role |
|:--------|:-----|
| [KernelPatch](https://github.com/bmax121/KernelPatch) | Kernel patching framework |
| [APatch](https://github.com/bmax121/APatch) | Android patch architecture |
| [Magisk](https://github.com/topjohnwu/Magisk) | magiskboot / magiskpolicy |
| [KernelSU](https://github.com/tiann/KernelSU) | App UI reference |

---

## License

[GPL-3.0](LICENSE)
