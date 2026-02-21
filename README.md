<div align="center">

```
   ███████╗██████╗ ███████╗ ██████╗████████╗██████╗ ███████╗
   ██╔════╝██╔══██╗██╔════╝██╔════╝╚══██╔══╝██╔══██╗██╔════╝
   ███████╗██████╔╝█████╗  ██║        ██║   ██████╔╝█████╗  
   ╚════██║██╔═══╝ ██╔══╝  ██║        ██║   ██╔══██╗██╔══╝  
   ███████║██║     ███████╗╚██████╗   ██║   ██║  ██║███████╗
   ╚══════╝╚═╝     ╚══════╝ ╚═════╝   ╚═╝   ╚═╝  ╚═╝╚══════╝
```

**内核级隐匿 Root 框架**

*看不见的，就检测不到。*

[![Build](https://github.com/l11223/Spectre/actions/workflows/build.yml/badge.svg)](https://github.com/l11223/Spectre/actions)
[![Release](https://img.shields.io/github/v/release/l11223/Spectre?color=00E5CC&label=Release)](https://github.com/l11223/Spectre/releases)
[![Downloads](https://img.shields.io/github/downloads/l11223/Spectre/total?color=00E5CC&label=Downloads)](https://github.com/l11223/Spectre/releases)
[![License](https://img.shields.io/badge/License-GPL%20v3-blue)](LICENSE)

[English](./README_EN.md) · 中文

</div>

---

## 概述

Spectre 是一个基于 [KernelPatch](https://github.com/bmax121/KernelPatch) 的 Android Root 方案，核心特点是**内核级隐匿**。

传统 Root 工具（Magisk、KernelSU）在应用层对抗检测，而 Spectre 的 ghost.kpm 模块直接在内核空间拦截系统调用，在检测代码读取到数据之前完成过滤和伪装。对外表现为完全原厂状态 — 游戏反作弊、银行 App、Play Integrity 均无法感知 Root 存在。

---

## 工作原理

```
┌─────────────────────────────────────────────────┐
│                   用户空间                        │
│                                                   │
│   游戏反作弊            银行 App                   │
│   access("/bin/su")     stat("/data/adb")         │
│   → ENOENT              → ENOENT                  │
│   /proc/maps → 干净     /proc/attr → u:r:sh:s0   │
│   uname -r → 原厂       TracerPid → 0             │
├───────────────────────┬───────────────────────────┤
│                       ▼                           │
│              ghost.kpm v5                         │
│                                                   │
│   15 个内核 Hook  ·  22 条隐藏路径                 │
│   拦截 → 过滤 → 伪装 → 返回                       │
│                                                   │
│                   内核空间                         │
└─────────────────────────────────────────────────┘
```

---

## ghost.kpm — 内核隐匿模块

### 核心隐藏

| Hook | 目标 | 效果 |
|:-----|:-----|:-----|
| `filldir64` | 目录遍历 | 隐藏 `/data/adb/` 下的 8 个混淆目录 |
| `avc_denied` | SELinux | uid=0 进程透明绕过 |
| `audit_log_start` | 审计日志 | 抑制 SELinux 违规日志 |

### /proc 隐藏

| Hook | 目标 | 效果 |
|:-----|:-----|:-----|
| `show_map_vma` | `/proc/pid/maps` | 过滤 root 库、zygisk、riru 痕迹 |
| `show_mountinfo` | `/proc/pid/mounts` | 过滤 overlay、magisk 挂载项 |
| `proc_pid_status` | `/proc/pid/status` | 强制 `TracerPid: 0` |
| `proc_pid_wchan` | `/proc/pid/wchan` | 替换 KP 符号为 `ep_poll` |

### 内核信息隐藏

| Hook | 目标 | 效果 |
|:-----|:-----|:-----|
| `devkmsg_read` | dmesg | 过滤 KernelPatch 启动痕迹 |
| `s_show` | `/proc/kallsyms` | 隐藏 `supercall`、`kp_*`、`ghost_*` 符号 |
| `newuname` | `uname -r` | 伪装内核版本号 |
| `version_proc_show` | `/proc/version` | 伪装完整版本字符串 |

### 文件存在性隐藏

| Hook | 目标 | 效果 |
|:-----|:-----|:-----|
| `do_faccessat` | `access()` | 22 条 root 相关路径返回 `-ENOENT` |
| `vfs_fstatat` | `stat()` | 同上 |
| `do_sys_openat2` | `open()` | 同上 |

### SELinux 上下文隐藏

| Hook | 目标 | 效果 |
|:-----|:-----|:-----|
| `selinux_getprocattr` | `/proc/pid/attr/current` | `u:r:su:s0` → `u:r:sh:s0` |

---

## 管理器功能

- 一键修补 boot 镜像，保留 AVB 签名链（锁定 BL 可用）
- 逐应用 SuperUser 权限管理
- 自定义 SU 路径
- Magisk 兼容模块系统（APM）
- KPM 内核模块加载器，ghost.kpm 开机自动加载
- 混淆目录结构，进程伪装为系统服务
- 多主题、自定义应用名称

---

## 快速开始

```bash
# 1. 从 Releases 下载最新 APK 和 ghost.kpm
# 2. 安装 Spectre APK
# 3. 通过 App 修补 boot 镜像
# 4. 刷入修补后的 boot（fastboot 或 EDL）
# 5. 重启 → 打开 Spectre → 加载 ghost.kpm
```

---

## 从源码构建

```bash
# 环境要求：JDK 21+, Android NDK r25c+, Rust + cargo-ndk

git clone https://github.com/l11223/Spectre.git
cd Spectre
./gradlew assembleRelease
```

ghost.kpm 单独构建：

```bash
cd kpm/ghost
make NDK_PATH=/path/to/ndk KP_DIR=/path/to/KernelPatch
```

---

## 项目结构

```
Spectre/
├── app/                    # 管理器 App（Kotlin / Jetpack Compose）
│   ├── src/main/cpp/       # JNI → KernelPatch supercall 桥接
│   └── src/main/java/      # UI、修补、SU 管理
├── apd/                    # Root 守护进程（Rust）
│   └── src/
│       ├── apd.rs          # Root shell 处理
│       ├── supercall.rs    # 内核接口
│       └── defs.rs         # 混淆路径定义
├── kpm/ghost/              # 内核隐匿模块（C）
│   └── src/ghost_main.c    # 15 个内核 hook
└── .github/workflows/      # CI/CD
```

---

## 致谢

| 项目 | 说明 |
|:-----|:-----|
| [KernelPatch](https://github.com/bmax121/KernelPatch) | 内核修补框架 |
| [APatch](https://github.com/bmax121/APatch) | Android 修补架构 |
| [Magisk](https://github.com/topjohnwu/Magisk) | magiskboot / magiskpolicy |
| [KernelSU](https://github.com/tiann/KernelSU) | App UI 参考 |

---

## 许可证

[GPL-3.0](LICENSE)
