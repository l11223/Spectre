/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * spoof.kpm v4 — Full kernel device spoofer for Y700 4th Gen
 * Kernel: 6.6.56 / ARM64 / Android 15
 *
 * Hooks:
 *   1. version_proc_show     — /proc/version
 *   2. cmdline_proc_show     — /proc/cmdline
 *   3. __arm64_sys_newuname   — uname -r (ARM64 pt_regs)
 *   4. soc_info_show          — /sys/devices/soc0/{machine,soc_id,serial_number,family,...}
 *   5. c_show (cpuinfo)       — /proc/cpuinfo CPU implementer/part
 */

#include <compiler.h>
#include <kpmodule.h>
#include <linux/printk.h>
#include <common.h>
#include <kputils.h>
#include <linux/string.h>
#include <hook.h>
#include <kallsyms.h>

KPM_NAME("spoof");
KPM_VERSION("4.0.0");
KPM_LICENSE("GPL v2");
KPM_AUTHOR("Spectre");
KPM_DESCRIPTION("Full device spoofer v4");

/* ========== Hook pointers ========== */
static void *h_version = 0;
static void *h_cmdline = 0;
static void *h_uname = 0;
static void *h_soc_info = 0;
static void *h_cpuinfo = 0;

/* ========== Spoof config — Pixel 9 Pro defaults ========== */
/* All values hardcoded for zero-delay boot spoofing.
 * Can still be overridden via control commands at runtime. */
static char sp_release[65] = "6.1.75-android14-11-g7e55710ac577";
static char sp_version[512] = "Linux version 6.1.75-android14-11 (build-user@build-host) (Android clang 17.0.2) #1 SMP PREEMPT Mon Feb 5 2024";
static char sp_cmdline[2048] = {0};  /* will use original unless set */
static char sp_soc_machine[64] = "Pixel 9 Pro";
static char sp_soc_id[16] = "576";
static char sp_soc_serial[32] = "100000001";
static char sp_soc_family[64] = "Snapdragon";
static char sp_cpu_impl[8] = "0x41";   /* ARM */
static char sp_cpu_part[8] = "0xd09";  /* Cortex-A73 */

/* ========== Utility ========== */

static int slen(const char *s) {
    int n = 0; if (s) while (s[n]) n++; return n;
}

static void scopy(char *d, const char *s, int max) {
    int i; for (i = 0; i < max - 1 && s[i]; i++) d[i] = s[i]; d[i] = '\0';
}

static int smatch(const char *hay, const char *needle) {
    if (!hay || !needle) return 0;
    int nlen = slen(needle);
    for (int i = 0; hay[i]; i++) {
        int j; for (j = 0; j < nlen && hay[i+j] == needle[j]; j++);
        if (j == nlen) return 1;
    }
    return 0;
}

/* seq_file replace helper */
static int seq_replace(uint64_t seq, const char *text) {
    if (!seq || !text || !text[0]) return 0;
    char *buf = *(char **)(seq + 0x00);
    uint64_t size = *(uint64_t *)(seq + 0x08);
    if (!buf || !size) return 0;
    int len = slen(text);
    if ((uint64_t)(len + 2) > size) return 0;
    int i; for (i = 0; i < len; i++) buf[i] = text[i];
    buf[i++] = '\n'; buf[i] = '\0';
    *(uint64_t *)(seq + 0x18) = i;
    return 1;
}

/* sysfs buf replace: write string + newline, return new length */
static int sysfs_replace(char *buf, const char *val) {
    if (!buf || !val || !val[0]) return 0;
    int len = slen(val);
    int i; for (i = 0; i < len; i++) buf[i] = val[i];
    buf[i++] = '\n'; buf[i] = '\0';
    return i;
}

/* ========== Hook 1: /proc/version ========== */
static void version_after(hook_fargs4_t *f, void *u) {
    if (sp_version[0] && f->ret == 0) seq_replace(f->arg0, sp_version);
}

/* ========== Hook 2: /proc/cmdline ========== */
static void cmdline_after(hook_fargs4_t *f, void *u) {
    if (sp_cmdline[0] && f->ret == 0) seq_replace(f->arg0, sp_cmdline);
}

/* ========== Hook 3: uname (ARM64 pt_regs) ========== */
static void uname_after(hook_fargs4_t *f, void *u) {
    if (!sp_release[0] || f->ret != 0) return;
    uint64_t regs = f->arg0;
    if (!regs) return;
    uint64_t user_ptr = *(uint64_t *)(regs + 0x00);
    if (user_ptr) compat_copy_to_user((char *)(user_ptr + 130), sp_release, 65);
}

/* ========== Hook 4: soc_info_show ========== */
/*
 * soc_info_show(struct device *dev, struct device_attribute *attr, char *buf)
 *
 * This is the SHARED show function for most /sys/devices/soc0/ attributes
 * including: machine, family, soc_id, revision, serial_number, etc.
 *
 * We identify which attribute by reading attr->attr.name and replace output.
 */
static void soc_info_after(hook_fargs4_t *f, void *u) {
    if ((long)f->ret <= 0) return;

    uint64_t attr_ptr = f->arg1;
    if (!attr_ptr) return;
    const char *name = *(const char **)(attr_ptr + 0x00);
    if (!name) return;

    char *buf = (char *)f->arg2;
    if (!buf) return;

    int newlen = 0;

    if (sp_soc_machine[0] && smatch(name, "machine")) {
        newlen = sysfs_replace(buf, sp_soc_machine);
    } else if (sp_soc_id[0] && smatch(name, "soc_id")) {
        newlen = sysfs_replace(buf, sp_soc_id);
    } else if (sp_soc_serial[0] && smatch(name, "serial_number")) {
        newlen = sysfs_replace(buf, sp_soc_serial);
    } else if (sp_soc_family[0] && smatch(name, "family")) {
        newlen = sysfs_replace(buf, sp_soc_family);
    }

    if (newlen > 0) f->ret = newlen;
}

/* ========== Hook 5: c_show (cpuinfo) ========== */
/*
 * c_show(struct seq_file *m, void *v) writes each CPU's info block.
 * After original writes, we scan the seq_file buffer and replace
 * CPU implementer and CPU part lines.
 *
 * This is tricky because c_show is called once per CPU core.
 * We do a simple find-and-replace in the buffer.
 */
static void cpuinfo_after(hook_fargs4_t *f, void *u) {
    if ((!sp_cpu_impl[0] && !sp_cpu_part[0]) || f->ret != 0) return;

    uint64_t seq = f->arg0;
    if (!seq) return;

    char *buf = *(char **)(seq + 0x00);
    uint64_t count = *(uint64_t *)(seq + 0x18);
    if (!buf || !count) return;

    /* Scan buffer for "CPU implementer" and "CPU part" lines.
     * Replace the hex value after ": 0x" with our spoofed value.
     * This is a simple in-place replacement. */

    for (uint64_t i = 0; i < count - 20; i++) {
        /* Look for "CPU implementer\t: " */
        if (sp_cpu_impl[0] && buf[i] == 'C' && buf[i+1] == 'P' && buf[i+2] == 'U' &&
            buf[i+3] == ' ' && buf[i+4] == 'i' && buf[i+5] == 'm') {
            /* Find the ": " part */
            uint64_t j = i;
            while (j < count && buf[j] != ':') j++;
            if (j + 2 < count) {
                j += 2; /* skip ": " */
                /* Overwrite the value until newline */
                int k;
                for (k = 0; sp_cpu_impl[k] && j + k < count && buf[j+k] != '\n'; k++)
                    buf[j+k] = sp_cpu_impl[k];
                /* Pad with spaces if new value is shorter */
                while (j + k < count && buf[j+k] != '\n') { buf[j+k] = ' '; k++; }
            }
        }

        /* Look for "CPU part" */
        if (sp_cpu_part[0] && buf[i] == 'C' && buf[i+1] == 'P' && buf[i+2] == 'U' &&
            buf[i+3] == ' ' && buf[i+4] == 'p' && buf[i+5] == 'a') {
            uint64_t j = i;
            while (j < count && buf[j] != ':') j++;
            if (j + 2 < count) {
                j += 2;
                int k;
                for (k = 0; sp_cpu_part[k] && j + k < count && buf[j+k] != '\n'; k++)
                    buf[j+k] = sp_cpu_part[k];
                while (j + k < count && buf[j+k] != '\n') { buf[j+k] = ' '; k++; }
            }
        }
    }
}

/* ========== Init ========== */

static long spoof_init(const char *args, const char *event, void *__user reserved) {
    int err;

    h_version = (void *)kallsyms_lookup_name("version_proc_show");
    if (h_version) { err = hook_wrap4(h_version, NULL, (void *)version_after, NULL); if (err) h_version = 0; }

    h_cmdline = (void *)kallsyms_lookup_name("cmdline_proc_show");
    if (h_cmdline) { err = hook_wrap4(h_cmdline, NULL, (void *)cmdline_after, NULL); if (err) h_cmdline = 0; }

    h_uname = (void *)kallsyms_lookup_name("__arm64_sys_newuname");
    if (h_uname) { err = hook_wrap4(h_uname, NULL, (void *)uname_after, NULL); if (err) h_uname = 0; }

    h_soc_info = (void *)kallsyms_lookup_name("soc_info_show");
    if (h_soc_info) { err = hook_wrap4(h_soc_info, NULL, (void *)soc_info_after, NULL); if (err) h_soc_info = 0; }

    /* c_show for cpuinfo — there are multiple c_show symbols, we want the one in kernel (not modules) */
    h_cpuinfo = (void *)kallsyms_lookup_name("c_show");
    if (h_cpuinfo) { err = hook_wrap4(h_cpuinfo, NULL, (void *)cpuinfo_after, NULL); if (err) h_cpuinfo = 0; }

    return 0;
}

/* ========== Control ========== */

static long spoof_control(const char *args, char *__user out_msg, int outlen) {
    if (!args) return -1;

    if (strncmp(args, "release:", 8) == 0)  { scopy(sp_release, args+8, sizeof(sp_release)); return 0; }
    if (strncmp(args, "version:", 8) == 0)  { scopy(sp_version, args+8, sizeof(sp_version)); return 0; }
    if (strncmp(args, "cmdline:", 8) == 0)  { scopy(sp_cmdline, args+8, sizeof(sp_cmdline)); return 0; }
    if (strncmp(args, "machine:", 8) == 0)  { scopy(sp_soc_machine, args+8, sizeof(sp_soc_machine)); return 0; }
    if (strncmp(args, "socid:", 6) == 0)    { scopy(sp_soc_id, args+6, sizeof(sp_soc_id)); return 0; }
    if (strncmp(args, "serial:", 7) == 0)   { scopy(sp_soc_serial, args+7, sizeof(sp_soc_serial)); return 0; }
    if (strncmp(args, "family:", 7) == 0)   { scopy(sp_soc_family, args+7, sizeof(sp_soc_family)); return 0; }
    if (strncmp(args, "cpuimpl:", 8) == 0)  { scopy(sp_cpu_impl, args+8, sizeof(sp_cpu_impl)); return 0; }
    if (strncmp(args, "cpupart:", 8) == 0)  { scopy(sp_cpu_part, args+8, sizeof(sp_cpu_part)); return 0; }

    if (strncmp(args, "status", 6) == 0) {
        char st[128];
        char *p = st;
        *p++='v'; *p++='4'; *p++=' ';
        *p++='V'; *p++=h_version?'1':'0'; *p++=' ';
        *p++='C'; *p++=h_cmdline?'1':'0'; *p++=' ';
        *p++='U'; *p++=h_uname?'1':'0'; *p++=' ';
        *p++='S'; *p++=h_soc_info?'1':'0'; *p++=' ';
        *p++='I'; *p++=h_cpuinfo?'1':'0';
        *p='\0';
        if (out_msg && outlen > 0) compat_copy_to_user(out_msg, st, p-st+1);
        return 0;
    }

    return -1;
}

/* ========== Exit ========== */

static long spoof_exit(void *__user reserved) {
    if (h_cpuinfo)  hook_unwrap(h_cpuinfo, NULL, (void *)cpuinfo_after);
    if (h_soc_info) hook_unwrap(h_soc_info, NULL, (void *)soc_info_after);
    if (h_uname)    hook_unwrap(h_uname, NULL, (void *)uname_after);
    if (h_cmdline)  hook_unwrap(h_cmdline, NULL, (void *)cmdline_after);
    if (h_version)  hook_unwrap(h_version, NULL, (void *)version_after);
    return 0;
}

KPM_INIT(spoof_init);
KPM_CTL0(spoof_control);
KPM_EXIT(spoof_exit);
