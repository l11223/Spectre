# Spectre

Kernel-level stealth root for Lenovo Legion Y700 4th Gen (TB322FC).

Based on FolkPatch with custom KPM stealth module.

## Components

- **Spectre App** - Root manager (patched FolkPatch with security hardening)
- **ghost.kpm** - Kernel stealth module with 4 active hooks:
  - `filldir64` - Directory hiding in /data/adb/
  - `avc_denied` - SELinux bypass for root processes (uid==0 only)
  - `audit_log_start` - Selective AVC audit suppression
  - `newuname` - Kernel version string spoofing

## Target Device

- Lenovo Legion Y700 4th Gen (TB322FC)
- Kernel: 6.6.56
- Android 15 / ZUI OS
- Snapdragon 8 Elite (SM8650)

## Build

GitHub Actions CI builds both APK and KPM automatically on push.

Manual build:
```bash
# APK
./gradlew assembleRelease

# KPM
cd kpm/ghost && make
```

## License

GPL v2
