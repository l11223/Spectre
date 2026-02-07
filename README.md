<div align="center">

<img src="https://img.shields.io/badge/ghost.kpm-v5.0-00E5CC?style=for-the-badge&logo=ghost&logoColor=white" />
<img src="https://img.shields.io/badge/Hooks-15_Active-00E5CC?style=for-the-badge" />
<img src="https://img.shields.io/badge/Hidden_Paths-22-00E5CC?style=for-the-badge" />

<br/><br/>

```
   ███████╗██████╗ ███████╗ ██████╗████████╗██████╗ ███████╗
   ██╔════╝██╔══██╗██╔════╝██╔════╝╚══██╔══╝██╔══██╗██╔════╝
   ███████╗██████╔╝█████╗  ██║        ██║   ██████╔╝█████╗  
   ╚════██║██╔═══╝ ██╔══╝  ██║        ██║   ██╔══██╗██╔══╝  
   ███████║██║     ███████╗╚██████╗   ██║   ██║  ██║███████╗
   ╚══════╝╚═╝     ╚══════╝ ╚═════╝   ╚═╝   ╚═╝  ╚═╝╚══════╝
```

### Kernel-Level Stealth Root Framework

*What they can't see, they can't detect.*

<br/>

[![Build](https://github.com/l11223/Spectre/actions/workflows/build.yml/badge.svg)](https://github.com/l11223/Spectre/actions)
[![Release](https://img.shields.io/github/v/release/l11223/Spectre?style=flat-square&color=00E5CC&label=Latest)](https://github.com/l11223/Spectre/releases)
[![Downloads](https://img.shields.io/github/downloads/l11223/Spectre/total?style=flat-square&color=00E5CC&label=Downloads)](https://github.com/l11223/Spectre/releases)
[![License](https://img.shields.io/badge/License-GPL%20v2-blue?style=flat-square)](LICENSE)

</div>

---

## 🔮 What is Spectre?

**Spectre** is not just another root tool — it's a **kernel-level invisibility engine**.

While traditional root solutions (Magisk, KernelSU) fight detection at the application layer, Spectre operates where anti-cheat can't reach: **inside the kernel itself**. Every file access, every proc read, every system call passes through our hooks before reaching the detection code.

The result? Your device appears completely stock to any inspection — games, banking apps, SafetyNet, Play Integrity — while you have full root access underneath.

> **15 kernel hooks. 22 hidden paths. Zero detection surface.**

---

## ⚡ Architecture

```
╔══════════════════════════════════════════════════════════╗
║                    USER SPACE                            ║
║                                                          ║
║   🎮 Game Anti-Cheat          🏦 Banking App             ║
║   ┌──────────────────┐       ┌──────────────────┐       ║
║   │ access("/bin/su") │       │ stat("/data/adb")│       ║
║   │ → ENOENT ✗        │       │ → ENOENT ✗       │       ║
║   │                    │       │                   │       ║
║   │ read /proc/maps   │       │ read /proc/attr   │       ║
║   │ → [clean output]  │       │ → u:r:sh:s0       │       ║
║   │                    │       │                   │       ║
║   │ uname -r           │       │ TracerPid check   │       ║
║   │ → 6.1.75-pixel    │       │ → TracerPid: 0    │       ║
║   └──────────────────┘       └──────────────────┘       ║
║              │                          │                ║
╠══════════════╪══════════════════════════╪════════════════╣
║              ▼                          ▼                ║
║   ┌──────────────────────────────────────────────┐      ║
║   │            👻  ghost.kpm v5                   │      ║
║   │                                               │      ║
║   │   15 Kernel Hooks  ×  22 Hidden Paths         │      ║
║   │                                               │      ║
║   │   Intercept → Filter → Spoof → Return         │      ║
║   │                                               │      ║
║   │   "You see what we want you to see."          │      ║
║   └──────────────────────────────────────────────┘      ║
║                       KERNEL SPACE                       ║
╚══════════════════════════════════════════════════════════╝
```

---

## 🛡️ ghost.kpm — 15 Active Hooks

The ghost kernel module intercepts every known detection vector at the syscall / VFS level.

### Core Hiding

| Hook | Target | Effect |
|:-----|:-------|:-------|
| `filldir64` | `ls`, `readdir()` | Hides 8 obfuscated dirs under `/data/adb/` |
| `avc_denied` | SELinux enforcement | Transparent bypass for uid=0 processes |
| `audit_log_start` | AVC audit subsystem | Suppresses SELinux violation logs from logcat/dmesg |

### /proc Hiding

| Hook | Target | Effect |
|:-----|:-------|:-------|
| `show_map_vma` | `/proc/pid/maps` | Filters root libraries, zygisk, riru, shamiko traces |
| `show_mountinfo` | `/proc/pid/mounts` | Filters overlay, magisk, KSU mount entries |
| `proc_pid_status` | `/proc/pid/status` | Forces `TracerPid: 0` — defeats anti-debug checks |
| `proc_pid_wchan` | `/proc/pid/wchan` | Replaces KP wait symbols with `ep_poll` |

### Kernel Info Hiding

| Hook | Target | Effect |
|:-----|:-------|:-------|
| `devkmsg_read` | `/dev/kmsg`, `dmesg` | Filters KernelPatch boot/load traces |
| `s_show` | `/proc/kallsyms` | Hides `supercall`, `kp_*`, `ghost_*` symbols |
| `newuname` | `uname -r` | Spoofs both release AND version fields (consistent) |
| `version_proc_show` | `/proc/version` | Spoofs full kernel version string |

### File Existence Hiding

| Hook | Target | Effect |
|:-----|:-------|:-------|
| `do_faccessat` | `access()` syscall | Returns `-ENOENT` for 22 root-related paths |
| `vfs_fstatat` | `stat()` syscall | Same — blocks `stat("/system/bin/su")` |
| `do_sys_openat2` | `open()` syscall | Same — blocks `open("/sbin/su")` |

### SELinux Context Hiding

| Hook | Target | Effect |
|:-----|:-------|:-------|
| `selinux_getprocattr` | `/proc/pid/attr/current` | Masks `u:r:su:s0` → `u:r:sh:s0` in-place |

---

## 📱 Spectre Manager

<table>
<tr>
<td width="50%">

**Root Management**
- One-click boot image patching
- AVB chain signature preservation (locked BL safe)
- SuperUser permission control per-app
- Custom SU path — hide it anywhere

</td>
<td width="50%">

**Stealth Features**
- Obfuscated directory structure (`.fk`, `.core`, `.sp_ext`)
- Process disguised as system service
- Log tags blend with Android system logs
- SELinux context: `u:r:su:s0` (not `magisk`)

</td>
</tr>
<tr>
<td>

**Module System**
- Magisk-compatible module support
- KPM (Kernel Patch Module) loader
- ghost.kpm auto-loaded on boot
- Meta-module support

</td>
<td>

**Customization**
- 12 app title presets
- Custom desktop app name
- Multiple theme options
- Navigation layout settings

</td>
</tr>
</table>

---

## 🎯 Target Device

<table>
<tr>
<td>

| Spec | Value |
|:-----|:------|
| **Device** | Lenovo Legion Y700 4th Gen |
| **Model** | TB322FC |
| **SoC** | Snapdragon 8 Elite (SM8650) |
| **Kernel** | 6.6.56-android15-8 |
| **OS** | Android 15 / ZUI OS |
| **BL Status** | Locked (EDL/9008 supported) |

</td>
<td>

> **Why locked bootloader?**
>
> Spectre is designed to work with **locked bootloaders** via EDL (Qualcomm 9008 mode) flashing. Boot images are re-signed with the correct AVB chain keys, so the device boots normally without tripping verification.
>
> No bootloader unlock required. No orange warning screen.

</td>
</tr>
</table>

---

## 🚀 Quick Start

```bash
# 1. Download latest APK + ghost.kpm from Releases
# 2. Install Spectre APK on device
# 3. Patch boot image via Spectre App
# 4. Flash patched boot (fastboot or EDL for locked BL)
# 5. Reboot → Open Spectre → Load ghost.kpm
# 6. Done. You're invisible.
```

---

## 🔧 Build from Source

```bash
# Prerequisites: JDK 17+, Android NDK r25c+, Rust + cargo-ndk

git clone https://github.com/l11223/Spectre.git
cd Spectre
./gradlew assembleRelease    # Builds APK + KPM
```

CI/CD: GitHub Actions automatically builds on every push.

---

## 📂 Project Structure

```
Spectre/
├── app/                          # Manager App (Kotlin/Compose)
│   ├── src/main/cpp/             # JNI → KernelPatch supercall bridge
│   └── src/main/java/            # UI, patching, SU management
├── apd/                          # Root Daemon (Rust)
│   └── src/
│       ├── cli.rs                # Entry + process disguise
│       ├── apd.rs                # Root shell handler
│       ├── supercall.rs          # Kernel interface + SU path mgmt
│       └── defs.rs               # Obfuscated path definitions
├── kpm/ghost/                    # Kernel Stealth Module (C)
│   └── src/ghost_main.c          # 15 kernel hooks, ~600 lines
└── .github/workflows/            # CI/CD pipeline
```

---

## 🙏 Credits

| Project | Role |
|:--------|:-----|
| [KernelPatch](https://github.com/bmax121/KernelPatch) | Kernel patching framework |
| [APatch](https://github.com/bmax121/APatch) | Android patch architecture |
| [FolkPatch](https://github.com/pomelohan/FolkPatch) | Fork base |

---

<div align="center">

<br/>

```
"The best root is the one nobody knows exists."
```

<br/>

**GPL v2** &nbsp;·&nbsp; Spectre v5 &nbsp;·&nbsp; ghost.kpm v5

<sub>Made with persistence, caffeine, and an unhealthy obsession with kernel internals.</sub>

</div>
