# Y700 四代 (TB322FC) AVB 签名完全指南

## 设备信息

- 型号: Lenovo Legion Y700 4th Gen (TB322FC)
- 内核: 6.6.56-android15-8
- SoC: Snapdragon 8 Elite (SM8650)
- Android 15 / ZUI OS / ZUXOS_1.1.11.044
- Bootloader: **锁定**（免解锁 BL root，通过 AVB 签名密钥 + 9008 刷入）
- 签名算法: SHA256_RSA4096
- 签名密钥公钥 SHA1: `2597c218aae470a130f61162feaae70afd97f011`

---

## 一、AVB 分区类型（最重要的概念）

```
┌─────────────────────────────────────────────────┐
│                  vbmeta.img                      │
│            (SHA256_RSA4096 签名)                  │
│                                                  │
│  ┌──────────────┐  ┌──────────────────────────┐ │
│  │ Chain 分区    │  │ Hash 分区                 │ │
│  │ (自签名)      │  │ (哈希存在 vbmeta 里)      │ │
│  │              │  │                          │ │
│  │ boot.img     │  │ init_boot.img            │ │
│  │ recovery.img │  │ dtbo.img                 │ │
│  │              │  │ vendor_boot.img          │ │
│  │              │  │ odm, vendor, system_dlkm │ │
│  └──────────────┘  └──────────────────────────┘ │
└─────────────────────────────────────────────────┘
```

### Chain 分区（boot.img）
- **自己签名自己**，AVB footer 里包含完整签名
- vbmeta 只存公钥，不存哈希
- **修改后只需重新签名 boot 本身，不需要动 vbmeta**
- 签名命令用 `--algorithm SHA256_RSA4096 --key <密钥>`

### Hash 分区（init_boot.img 等）
- **不签名**，Algorithm = NONE
- 哈希值存在 **vbmeta.img** 里
- **修改后必须同时重建 vbmeta**（更新哈希 + 重新签名 vbmeta）
- 不重建 vbmeta = AVB 验证失败 = 红字 = 变砖

---

## 二、签名密钥

### 密钥来源
从 `鸡鸡拯救者AVB签名工具.exe` 中提取（PyInstaller 解包）：
```
鸡鸡拯救者AVB签名工具.exe_extracted/tools/pem/testkey_rsa4096.pem  (私钥)
鸡鸡拯救者AVB签名工具.exe_extracted/tools/pem/testkey_rsa2048.pem  (备用)
```

### 注意
`联想签名钥.zip` 里的密钥和 exe 里的密钥**内容不同**（行尾格式差异），但**公钥相同**。两个都能用，推荐用 exe 里提取的那个。

### 验证密钥匹配
```bash
python3 avbtool.py extract_public_key --key testkey_rsa4096.pem --output pub.bin
shasum -a 1 pub.bin
# 输出应为: 2597c218aae470a130f61162feaae70afd97f011
```

---

## 三、正确签名 boot.img（Chain 分区）

### 读取原版参数
```bash
python3 avbtool.py info_image --image boot_a_working.img
```

关键参数记录：
```
Algorithm:       SHA256_RSA4096
Rollback Index:  1738713600
Partition Size:  100663296 bytes
Salt:            2919b99b88ac41218ee8054ad02c4e1fb8c631c0b79c541c530d6bb743ea7c09
Partition Name:  boot
```

### 签名步骤
```bash
# 1. 擦除旧 AVB footer
python3 avbtool.py erase_footer --image patched_boot.img

# 2. 添加新的签名 footer
python3 avbtool.py add_hash_footer \
  --image patched_boot.img \
  --partition_name boot \
  --partition_size 100663296 \
  --algorithm SHA256_RSA4096 \
  --key testkey_rsa4096.pem \
  --rollback_index 1738713600 \
  --salt 2919b99b88ac41218ee8054ad02c4e1fb8c631c0b79c541c530d6bb743ea7c09 \
  --prop "com.android.build.boot.os_version:15" \
  --prop "com.android.build.boot.fingerprint:Lenovo/TB322FC_PRC/TB322FC:15/AQ3A.250129.001/ZUXOS_1.1.11.044_250524_PRC:user/release-keys" \
  --prop "com.android.build.boot.security_patch:2025-02-05"

# 3. 验证
python3 avbtool.py info_image --image patched_boot.img
# 确认 Public key sha1 = 2597c218...
```

### 也可以用 rebuild_avb.py（自动化）
```bash
# 把 patched_boot.img 改名为 boot.img 放在工作目录
cp patched_boot.img boot.img
python3 rebuild_avb.py --chained-mode
# 自动检测参数并签名
```

---

## 四、正确签名 init_boot.img（Hash 分区）—— 需要同时更新 vbmeta

### 绝对不能做的事
- ❌ 不能给 init_boot 用 SHA256_RSA4096 签名（Algorithm 必须是 NONE）
- ❌ 不能只改 init_boot 不改 vbmeta
- ❌ 不能用小于 64KB 的 vbmeta 覆盖原版（原版是 64KB）

### 正确步骤
```bash
# 1. 修改 init_boot 内容（解包、改 ramdisk、重新打包）

# 2. 添加 NONE 算法的 hash footer（不签名！）
python3 avbtool.py add_hash_footer \
  --image init_boot_modified.img \
  --partition_name init_boot \
  --partition_size 8388608 \
  --algorithm NONE \
  --salt 4734f4370403f387182e2d7b9d25c8547b05073a6841f4d2382082acf8e7788e \
  --prop "com.android.build.init_boot.os_version:15" \
  --prop "com.android.build.init_boot.fingerprint:Lenovo/TB322FC_PRC/TB322FC:15/AQ3A.250129.001/ZUXOS_1.1.11.044_250524_PRC:user/release-keys" \
  --prop "com.android.build.init_boot.security_patch:2025-02-05"

# 3. 重建 vbmeta（包含新的 init_boot 哈希 + 保留其他分区哈希）
python3 avbtool.py make_vbmeta_image \
  --output vbmeta_new.img \
  --algorithm SHA256_RSA4096 \
  --key testkey_rsa4096.pem \
  --rollback_index 0 \
  --flags 0 \
  --padding_size 65536 \
  --include_descriptors_from_image vbmeta_original.img \
  --include_descriptors_from_image init_boot_modified.img
# 注意：padding_size 必须是 65536（64KB），和原版一致！

# 4. 刷入 BOTH（两个都要刷！）
sudo edl w init_boot_a init_boot_modified.img --loader=firehose_fixed.elf
# 重启进 fastboot，再进 9008
sudo edl w vbmeta_a vbmeta_new.img --loader=firehose_fixed.elf
```

---

## 五、9008 刷入方法（Mac）

### 工具
- `edl`：`pip3 install edl`（bkerler/edl）
- `firehose_fixed.elf`：从 `四代引导.melf` 修复 ELF 头得到
  - 原文件 e_machine = 0xf3 (RISC-V)，需改为 0x28 (ARM)
  - 修复脚本：
    ```python
    data = bytearray(open('四代引导.melf', 'rb').read())
    import struct
    struct.pack_into('<H', data, 18, 0x28)
    open('firehose_fixed.elf', 'wb').write(data)
    ```

### 进入 9008
```bash
# 从 fastboot 进入
fastboot oem edl

# 或物理方式：关机 → 按住音量+ → 插 USB
```

### 刷入命令
```bash
# 写入分区（每次 9008 session 只能执行一条命令）
sudo edl w <分区名> <文件路径> --loader=firehose_fixed.elf

# 读取分区（验证写入）
sudo edl r <分区名> <输出路径> --loader=firehose_fixed.elf

# 写原始扇区（GPT 修复等）
sudo edl ws <起始扇区> <文件路径> --lun=<LUN号> --loader=firehose_fixed.elf

# 读原始扇区
sudo edl rs <起始扇区> <扇区数> <输出路径> --lun=<LUN号> --loader=firehose_fixed.elf
```

### 重要：每次 edl 操作后设备会进入 Sahara error 状态
- 第二条命令会报 "Device is in Sahara error state"
- 必须 **拔线 → 长按电源 15 秒 → 重新进 fastboot → 再进 9008** 才能执行下一条

---

## 六、我们的失败和修复记录

### 失败原因链
```
dd 修改版 init_boot 到设备
  → init_boot 用了 SHA256_RSA4096（错！应该 NONE）
  → 没更新 vbmeta 中的哈希
  → AVB 验证失败 → 红字
  → 多次失败启动 → slot_a 被标记 unbootable (GPT 属性 0xf7)
  → misc 分区写入 "boot-recovery" BCB 命令
  → 即使恢复原版文件也开不了机
```

### 修复步骤
1. **恢复 init_boot_a**：edl w init_boot_a 原版备份
2. **恢复 vbmeta_a**：edl w vbmeta_a 原版 vbmeta（必须 64KB）
3. **修复 GPT unbootable 标志**：
   ```bash
   # 读 GPT
   sudo edl rs 0 34 gpt_full.bin --lun=4 --loader=firehose_fixed.elf
   # Python 修改 boot_a 属性：0xf7 → 0x77
   # 位置：sector 2, entry 11, offset 1462
   # 0x77 = priority=3, active=1, tries=6, successful=1, unbootable=0
   # 重算 GPT CRC
   # 写回
   sudo edl ws 0 gpt_full_fixed.bin --lun=4 --loader=firehose_fixed.elf
   ```
4. **清除 misc BCB**：
   ```bash
   sudo edl r misc misc_dump.bin --loader=firehose_fixed.elf
   # Python: 将前 32 字节清零（清除 "boot-recovery" 命令）
   sudo edl w misc misc_fixed.bin --loader=firehose_fixed.elf
   ```

### GPT 属性值对照表（LUN 4，boot_a 是 entry 11）
```
0x1044 = 正常 slot_a (priority=0, active=1, successful=1, unbootable=0)
0x1077 = 正常 boot_a (priority=3, active=1, tries=6, successful=1, unbootable=0)
0x10f7 = 坏的 boot_a (priority=3, active=1, tries=6, successful=1, unbootable=1)
0x1000 = 正常 slot_b (全零)
```

### 属性字节解读 (bits in byte at offset +6 of GPT attribute)
```
Bit 0-1: priority (0-3)
Bit 2:   active
Bit 3-5: tries_remaining (0-7)
Bit 6:   successful_boot
Bit 7:   unbootable
```

---

## 七、关键分区位置（LUN 4）

| 分区 | Sector | Offset |
|------|--------|--------|
| boot_a | 113318 | 0x1baa6000 |
| vbmeta_a | 137946 | 0x21ada000 |
| init_boot_a | 205962 | 0x3248a000 |
| boot_b | - | 0x4e920000 |
| vbmeta_b | - | 0x54954000 |
| init_boot_b | - | 0x63504000 |

misc 分区在 LUN 0，sector 89992。

---

## 八、备份清单（刷入前必须有）

| 文件 | 用途 | 获取方式 |
|------|------|---------|
| boot_a_working.img | 当前正常 boot | `dd if=/dev/block/by-name/boot_a of=/sdcard/boot_a_working.img` |
| init_boot_a_working.img | 当前正常 init_boot | `dd if=/dev/block/sde27 of=/sdcard/init_boot_a_working.img` |
| vbmeta.img | 原版 vbmeta (64KB) | 从 ROM 包提取 |
| firehose_fixed.elf | 9008 引导 | 从 四代引导.melf 修复 |
| testkey_rsa4096.pem | 签名密钥 | 从 exe 提取 |

---

## 九、黄金法则

1. **改 boot = 只签 boot**（不动 vbmeta）
2. **改 init_boot = 签 init_boot + 重建 vbmeta**（两个都刷）
3. **vbmeta 必须 64KB**（padding_size=65536）
4. **init_boot 的 Algorithm 必须是 NONE**
5. **每次 9008 只能执行一条 edl 命令**
6. **刷入前必须备份原版**
7. **GPT 修改后必须重算 CRC**
