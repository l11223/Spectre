<div align="center">

```
   ███████╗██████╗ ███████╗ ██████╗████████╗██████╗ ███████╗
   ██╔════╝██╔══██╗██╔════╝██╔════╝╚══██╔══╝██╔══██╗██╔════╝
   ███████╗██████╔╝█████╗  ██║        ██║   ██████╔╝█████╗  
   ╚════██║██╔═══╝ ██╔══╝  ██║        ██║   ██╔══██╗██╔══╝  
   ███████║██║     ███████╗╚██████╗   ██║   ██║  ██║███████╗
   ╚══════╝╚═╝     ╚══════╝ ╚═════╝   ╚═╝   ╚═╝  ╚═╝╚══════╝
```

**Kernel-Level Android Root Management Framework**

[![Build](https://github.com/l11223/Spectre/actions/workflows/build.yml/badge.svg)](https://github.com/l11223/Spectre/actions)
[![Release](https://img.shields.io/github/v/release/l11223/Spectre?color=00E5CC&label=Release)](https://github.com/l11223/Spectre/releases)
[![Downloads](https://img.shields.io/github/downloads/l11223/Spectre/total?color=00E5CC&label=Downloads)](https://github.com/l11223/Spectre/releases)
[![License](https://img.shields.io/badge/License-GPL%20v3-blue)](LICENSE)

English · [中文](./README.md)

</div>

---

## Overview

Spectre is an Android root solution built on [KernelPatch](https://github.com/bmax121/KernelPatch). It provides kernel-level root management with a companion KPM (Kernel Patch Module) called ghost.kpm for enhanced privacy.

The ghost.kpm module operates in kernel space, intercepting and filtering system calls to manage root visibility. This allows fine-grained control over which applications can detect root presence on the device.

---

## Features

- One-click boot image patching with AVB chain preservation
- Per-app SuperUser permission control
- Custom SU path configuration
- Magisk-compatible module system (APM)
- KPM kernel module loader with auto-load on boot
- ghost.kpm: 15 kernel hooks for root visibility management
- Multiple themes and custom app naming
- Built with Kotlin/Compose (app), Rust (daemon), and C (kernel module)

---

## Quick Start

```bash
# 1. Download latest APK + ghost.kpm from Releases
# 2. Install Spectre APK
# 3. Patch boot image via the app
# 4. Flash patched boot (fastboot or EDL)
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
│       └── defs.rs         # Path definitions
├── kpm/ghost/              # Kernel Module (C)
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
