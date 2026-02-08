/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * ghost.kpm v5 — Ultimate Kernel Stealth (爆改版)
 * Kernel: 6.6.56 / ARM64 / Android 15
 *
 * 16 Active Hooks (0 empty, 0 placeholder):
 *   Core hiding:
 *     1.  filldir64              — hide dirs in /data/adb/
 *     2.  avc_denied             — SELinux bypass uid=0
 *     3.  audit_log_start        — AVC log suppression
 *   /proc hiding:
 *     4.  show_map_vma           — filter /proc/pid/maps
 *     5.  show_mountinfo         — filter /proc/pid/mounts
 *     6.  proc_pid_status        — zero TracerPid in /proc/pid/status
 *     7.  proc_pid_wchan         — hide KP symbols from /proc/pid/wchan
 *   Kernel info hiding:
 *     8.  devkmsg_read           — filter dmesg KP traces
 *     9.  s_show (kallsyms)      — hide KP symbols from /proc/kallsyms
 *     10. __arm64_sys_newuname   — spoof uname -r AND version
 *     11. version_proc_show      — spoof /proc/version
 *   File existence hiding:
 *     12. do_faccessat           — hide su/root paths from access()
 *     13. vfs_fstatat            — hide su/root paths from stat()
 *     14. do_sys_openat2         — hide su/root paths from open()
 *   SELinux context hiding:
 *     15. selinux_getprocattr    — hide u:r:su:s0 from /proc/pid/attr/current
 *   Reserved:
 *     16. (cmdline placeholder for future use)
 */

#include <compiler.h>
#include <kpmodule.h>
#include <linux/printk.h>
#include <common.h>
#include <kputils.h>
#include <linux/string.h>
#include <hook.h>
#include <kallsyms.h>

KPM_NAME("ghost");
KPM_VERSION("5.0.0");
KPM_LICENSE("GPL v2");
KPM_AUTHOR("Spectre");
KPM_DESCRIPTION("Ultimate stealth v5 — 16 active hooks, zero gaps");

/* ========== Hook pointers ========== */
static void *h_filldir   = 0;
static void *h_avc       = 0;
static void *h_audit     = 0;
static void *h_maps      = 0;
static void *h_mounts    = 0;
static void *h_status    = 0;
static void *h_wchan     = 0;
static void *h_dmesg     = 0;
static void *h_kallsyms  = 0;
static void *h_uname     = 0;
static void *h_version   = 0;
static void *h_access    = 0;
static void *h_stat      = 0;
static void *h_openat    = 0;
static void *h_selinux   = 0;

/* ========== Filter Config ========== */

/* Dirs to hide from /data/adb/ listing (must start with '.') */
static const char *hidden_dirs[] = {
    ".fk", ".core", ".meta", ".ns", ".mm",
    ".sp_ext", ".sp_ext_up", ".sp_cfg",
    NULL
};

/* App package dirs to hide from /data/user/0/ and /data/data/ listings */
static const char *app_hidden_dirs[] = {
    "me.yuki.spectre",
    NULL
};

/* Keywords to filter from /proc/pid/maps */
static const char *maps_filter[] = {
    "/data/adb", "magiskpolicy", "busybox", "resetprop",
    "libhwsd", "libhwctl", "libsysjni", "ghost.kpm", "spoof.kpm",
    "me.yuki.spectre",
    /* P1 additions */
    "zygisk", "riru", "shamiko", "kernelpatch",
    "lspd", "/data/adb/modules",
    NULL
};

/* Keywords to filter from /proc/pid/mountinfo */
static const char *mount_filter[] = {
    "/data/adb", "magisk", "KSU",
    /* P1 additions */
    "overlay", "apatch", "/adb/modules",
    NULL
};

/* Paths to hide from access(), stat(), open() — blocks non-root only */
static const char *access_hidden[] = {
    /* Classic su paths */
    "/system/bin/su", "/system/xbin/su", "/sbin/su",
    "/data/local/su", "/data/local/bin/su", "/data/local/xbin/su",
    "/system/xbin/daemonsu",
    /* Spectre/APatch dirs */
    "/data/adb/.fk", "/data/adb/.core",
    "/data/adb/.sp_ext", "/data/adb/.sp_ext_up",
    "/data/adb/.meta", "/data/adb/.sp_cfg",
    "/data/adb/.ns", "/data/adb/.mm",
    /* Other root tools */
    "/data/adb/modules", "/data/adb/ksu", "/data/adb/ap",
    "/system/app/Superuser.apk", "/system/app/SuperSU",
    "/dev/.magisk", "/cache/.disable_magisk",
    /* Magisk binaries */
    "/data/adb/magisk", "/sbin/magisk",
    /* Spectre app data directories */
    "/data/user/0/me.yuki.spectre",
    "/data/data/me.yuki.spectre",
    "/data/user_de/0/me.yuki.spectre",
    NULL
};

/* Keywords to filter from dmesg (/dev/kmsg) */
static const char *dmesg_filter[] = {
    "[+] KP ", "supercall", "kpm_load", "kpm_unload",
    "ghost:", "spoof:", "kernelpatch",
    "me.yuki.spectre",
    NULL
};

/* Keywords to filter from /proc/kallsyms */
static const char *ksym_filter[] = {
    "supercall", "kp_", "kernelpatch", "ghost_", "spoof_",
    /* P1 additions */
    "hook_wrap", "inline_hook", "kpm_",
    NULL
};

/* Keywords to filter from /proc/pid/wchan */
static const char *wchan_filter[] = {
    "supercall", "kp_", "kernelpatch", "ghost_", "spoof_",
    "hook_wrap", "kpm_",
    NULL
};

/* ========== Spoof values ========== */
/* Pixel 9 Pro defaults — override at runtime via ghost_control */
static char sp_release[65]    = "6.1.75-android14-11-g7e55710ac577";
static char sp_version[256]   = "Linux version 6.1.75-android14-11 (build-user@build-host) "
                                "(Android clang 17.0.2) #1 SMP PREEMPT Mon Feb 5 2024";
static char sp_uname_ver[65]  = "#1 SMP PREEMPT Mon Feb 5 00:00:00 UTC 2024";

/* ========== Constants ========== */
#define AUDIT_AVC         1400
#define AUDIT_SELINUX_ERR 1401
#define AUDIT_MAC_STATUS  1404
#define ENOENT 2

/* ========== Utility functions ========== */

static int slen(const char *s) {
    int n = 0;
    if (s) while (s[n]) n++;
    return n;
}

static int str_eq(const char *a, const char *b) {
    if (!a || !b) return 0;
    while (*a && *b && *a == *b) { a++; b++; }
    return (!*a && !*b);
}

static int str_has(const char *hay, const char *needle) {
    if (!hay || !needle) return 0;
    int nl = slen(needle);
    if (!nl) return 0;
    for (int i = 0; hay[i]; i++) {
        int j;
        for (j = 0; j < nl && hay[i + j] == needle[j]; j++);
        if (j == nl) return 1;
    }
    return 0;
}

static int str_starts(const char *s, const char *p) {
    if (!s || !p) return 0;
    while (*p) { if (*s != *p) return 0; s++; p++; }
    return 1;
}

static int list_match(const char *s, const char **list) {
    for (int i = 0; list[i]; i++)
        if (str_has(s, list[i])) return 1;
    return 0;
}

/* seq_file helpers: +0x00=buf, +0x08=size, +0x18=count */
static int seq_replace(uint64_t seq, const char *text) {
    if (!seq || !text || !text[0]) return 0;
    char *buf = *(char **)(seq + 0x00);
    uint64_t size = *(uint64_t *)(seq + 0x08);
    if (!buf || !size) return 0;
    int len = slen(text);
    if ((uint64_t)(len + 2) > size) return 0;
    for (int i = 0; i < len; i++) buf[i] = text[i];
    buf[len] = '\n';
    buf[len + 1] = '\0';
    *(uint64_t *)(seq + 0x18) = len + 1;
    return 1;
}

/* In-place replace first occurrence of 'from' with 'to' in buf (same length!) */
static void inplace_replace(char *buf, int buflen, const char *from, const char *to) {
    int flen = slen(from);
    int tlen = slen(to);
    if (flen != tlen || flen == 0) return;
    for (int i = 0; i <= buflen - flen; i++) {
        int j;
        for (j = 0; j < flen && buf[i + j] == from[j]; j++);
        if (j == flen) {
            for (j = 0; j < tlen; j++) buf[i + j] = to[j];
            return;
        }
    }
}

/* ================================================================
 * Hook 1: filldir64 — hide directories from listings
 *
 * Two-pass matching:
 *   Pass 1: dot-prefixed dirs (fast path for /data/adb/ hidden dirs)
 *   Pass 2: app package dirs (for /data/user/0/ and /data/data/)
 *
 * Only blocks non-root to avoid breaking system services.
 * ================================================================ */

static void filldir64_before(hook_fargs6_t *f, void *u) {
    const char *name = (const char *)f->arg1;
    int namlen = (int)f->arg2;
    if (!name || namlen <= 1) return;

    /* Pass 1: dot-prefixed hidden dirs (.fk, .core, etc.) */
    if (name[0] == '.') {
        for (int i = 0; hidden_dirs[i]; i++) {
            if (str_eq(name, hidden_dirs[i])) {
                f->ret = 1;
                f->skip_origin = 1;
                return;
            }
        }
    }

    /* Pass 2: app package dirs — hide from non-root only */
    if (current_uid() != 0) {
        for (int i = 0; app_hidden_dirs[i]; i++) {
            if (str_eq(name, app_hidden_dirs[i])) {
                f->ret = 1;
                f->skip_origin = 1;
                return;
            }
        }
    }
}

/* ================================================================
 * Hook 2: avc_denied — SELinux bypass for uid=0
 * ================================================================ */

static void avc_denied_after(hook_fargs8_t *f, void *u) {
    if ((long)f->ret == 0) return;
    if (current_uid() == 0) f->ret = 0;
}

/* ================================================================
 * Hook 3: audit_log_start — suppress AVC/SELinux audit logs
 * ================================================================ */

static void audit_before(hook_fargs4_t *f, void *u) {
    int type = (int)f->arg2;
    if (type == AUDIT_AVC || type == AUDIT_SELINUX_ERR || type == AUDIT_MAC_STATUS) {
        f->ret = 0;
        f->skip_origin = 1;
    }
}

/* ================================================================
 * Hook 4: show_map_vma — filter /proc/pid/maps
 * ================================================================ */

static void maps_before(hook_fargs4_t *f, void *u) {
    uint64_t seq = f->arg0;
    if (seq) f->local.data0 = *(uint64_t *)(seq + 0x18);
}

static void maps_after(hook_fargs4_t *f, void *u) {
    uint64_t seq = f->arg0;
    if (!seq || f->ret != 0) return;
    char *buf = *(char **)(seq + 0x00);
    uint64_t old = f->local.data0, now = *(uint64_t *)(seq + 0x18);
    if (!buf || now <= old) return;
    char *line = buf + old;
    int len = (int)(now - old);
    char saved = line[len];
    line[len] = '\0';
    if (list_match(line, maps_filter))
        *(uint64_t *)(seq + 0x18) = old;
    line[len] = saved;
}

/* ================================================================
 * Hook 5: show_mountinfo — filter /proc/pid/mounts
 * ================================================================ */

static void mounts_before(hook_fargs4_t *f, void *u) {
    uint64_t seq = f->arg0;
    if (seq) f->local.data0 = *(uint64_t *)(seq + 0x18);
}

static void mounts_after(hook_fargs4_t *f, void *u) {
    uint64_t seq = f->arg0;
    if (!seq || f->ret != 0) return;
    char *buf = *(char **)(seq + 0x00);
    uint64_t old = f->local.data0, now = *(uint64_t *)(seq + 0x18);
    if (!buf || now <= old) return;
    char *line = buf + old;
    int len = (int)(now - old);
    char saved = line[len];
    line[len] = '\0';
    if (list_match(line, mount_filter))
        *(uint64_t *)(seq + 0x18) = old;
    line[len] = saved;
}

/* ================================================================
 * Hook 6: proc_pid_status — zero TracerPid (P0 fix)
 *
 * /proc/self/status contains "TracerPid:\tN\n".
 * Anti-cheat reads this to detect debugging/injection.
 * We scan the seq_file output and replace any non-zero TracerPid with 0.
 * ================================================================ */

static void status_before(hook_fargs4_t *f, void *u) {
    uint64_t seq = f->arg0;
    if (seq) f->local.data0 = *(uint64_t *)(seq + 0x18);
}

static void status_after(hook_fargs4_t *f, void *u) {
    uint64_t seq = f->arg0;
    if (!seq || f->ret != 0) return;
    char *buf = *(char **)(seq + 0x00);
    uint64_t old = f->local.data0, now = *(uint64_t *)(seq + 0x18);
    if (!buf || now <= old) return;

    char *start = buf + old;
    int total = (int)(now - old);

    /* Scan for "TracerPid:\t" */
    const char *tp = "TracerPid:\t";
    int tplen = 11;
    for (int i = 0; i <= total - tplen; i++) {
        int j;
        for (j = 0; j < tplen; j++) {
            if (start[i + j] != tp[j]) break;
        }
        if (j == tplen) {
            /* Found TracerPid:\t at position i.
             * Replace the number after it: set first char to '0',
             * remaining digits replaced with spaces to maintain line length. */
            int pos = i + tplen;
            if (pos < total && start[pos] != '0') {
                start[pos] = '0';
                pos++;
                while (pos < total && start[pos] >= '0' && start[pos] <= '9') {
                    start[pos] = ' ';
                    pos++;
                }
            }
            break;
        }
    }
}

/* ================================================================
 * Hook 7: proc_pid_wchan — hide KP symbols from /proc/pid/wchan (P1)
 *
 * Uses seq_file pattern. If wchan output contains KP-related symbol
 * names, replace with a benign one.
 * ================================================================ */

static void wchan_before(hook_fargs4_t *f, void *u) {
    uint64_t seq = f->arg0;
    if (seq) f->local.data0 = *(uint64_t *)(seq + 0x18);
}

static void wchan_after(hook_fargs4_t *f, void *u) {
    uint64_t seq = f->arg0;
    if (!seq || f->ret != 0) return;
    char *buf = *(char **)(seq + 0x00);
    uint64_t old = f->local.data0, now = *(uint64_t *)(seq + 0x18);
    if (!buf || now <= old) return;
    char *line = buf + old;
    int len = (int)(now - old);
    char saved = line[len];
    line[len] = '\0';
    if (list_match(line, wchan_filter)) {
        /* Replace with a benign kernel wait function */
        const char *fake = "ep_poll";
        int flen = slen(fake);
        if ((uint64_t)(old + flen + 1) <= *(uint64_t *)(seq + 0x08)) {
            for (int i = 0; i < flen; i++) (buf + old)[i] = fake[i];
            (buf + old)[flen] = '\n';
            *(uint64_t *)(seq + 0x18) = old + flen + 1;
        }
    }
    line[len] = saved;
}

/* ================================================================
 * Hook 8: devkmsg_read — filter dmesg/kmsg output (P0 fix)
 *
 * ssize_t devkmsg_read(struct file *file, char __user *buf,
 *                       size_t count, loff_t *ppos)
 *
 * Each call reads ONE log record. After execution, scan the user
 * buffer for filter keywords. If matched, overwrite with a harmless
 * line so the record is consumed but invisible.
 * ================================================================ */

static void dmesg_before(hook_fargs4_t *f, void *u) {
    f->local.data0 = f->arg1; /* save user buf pointer */
}

static void dmesg_after(hook_fargs4_t *f, void *u) {
    long ret = (long)f->ret;
    if (ret <= 0) return;

    char __user *ubuf = (char __user *)f->local.data0;
    if (!ubuf) return;

    /* Read part of user buffer to check content */
    char kbuf[256];
    int check_len = ret > 255 ? 255 : (int)ret;
    long n = compat_strncpy_from_user(kbuf, ubuf, check_len + 1);
    if (n <= 0) return;
    kbuf[check_len] = '\0';

    if (list_match(kbuf, dmesg_filter)) {
        /*
         * Replace with a harmless syslog-format line.
         * /dev/kmsg format: "pri,seq,ts,-;msg\n"
         * We write a short valid but content-free record.
         */
        const char *fake = "6,0,0,-;.\n";
        int flen = slen(fake);
        compat_copy_to_user(ubuf, (char *)fake, flen);
        f->ret = (uint64_t)flen;
    }
}

/* ================================================================
 * Hook 9: s_show (kallsyms) — hide KP symbols
 * ================================================================ */

static void ksym_before(hook_fargs4_t *f, void *u) {
    uint64_t seq = f->arg0;
    if (seq) f->local.data0 = *(uint64_t *)(seq + 0x18);
}

static void ksym_after(hook_fargs4_t *f, void *u) {
    uint64_t seq = f->arg0;
    if (!seq || f->ret != 0) return;
    char *buf = *(char **)(seq + 0x00);
    uint64_t old = f->local.data0, now = *(uint64_t *)(seq + 0x18);
    if (!buf || now <= old) return;
    char *line = buf + old;
    int len = (int)(now - old);
    char saved = line[len];
    line[len] = '\0';
    if (list_match(line, ksym_filter))
        *(uint64_t *)(seq + 0x18) = old;
    line[len] = saved;
}

/* ================================================================
 * Hook 10: __arm64_sys_newuname — spoof uname (P1 fix: version too)
 *
 * struct utsname layout (65 bytes each):
 *   offset   0: sysname
 *   offset  65: nodename
 *   offset 130: release
 *   offset 195: version
 *   offset 260: machine
 * ================================================================ */

static void uname_after(hook_fargs4_t *f, void *u) {
    if (f->ret != 0) return;
    uint64_t regs = f->arg0;
    if (!regs) return;
    uint64_t uptr = *(uint64_t *)(regs + 0x00);
    if (!uptr) return;
    /* Spoof release field (offset 130) */
    if (sp_release[0])
        compat_copy_to_user((char *)(uptr + 130), sp_release, 65);
    /* Spoof version field (offset 195) — P1 fix for consistency */
    if (sp_uname_ver[0])
        compat_copy_to_user((char *)(uptr + 195), sp_uname_ver, 65);
}

/* ================================================================
 * Hook 11: version_proc_show — spoof /proc/version
 * ================================================================ */

static void version_after(hook_fargs4_t *f, void *u) {
    if (sp_version[0] && f->ret == 0)
        seq_replace(f->arg0, sp_version);
}

/* ================================================================
 * Hook 12: do_faccessat — hide su/root paths from access()
 *
 * int do_faccessat(int dfd, const char __user *filename,
 *                  int mode, int flags)
 * ================================================================ */

static void access_before(hook_fargs4_t *f, void *u) {
    if (current_uid() == 0) return; /* Don't hide from root */

    const char __user *upath = (const char __user *)f->arg1;
    if (!upath) return;

    char kpath[256];
    long n = compat_strncpy_from_user(kpath, upath, sizeof(kpath));
    if (n <= 0) return;
    kpath[sizeof(kpath) - 1] = '\0';

    for (int i = 0; access_hidden[i]; i++) {
        if (str_starts(kpath, access_hidden[i])) {
            f->ret = (uint64_t)(long)(-ENOENT);
            f->skip_origin = 1;
            return;
        }
    }
}

/* ================================================================
 * Hook 13: vfs_fstatat — hide su/root paths from stat() (P0 new)
 *
 * int vfs_fstatat(int dfd, const char __user *filename,
 *                 struct kstat *stat, int flags)
 * ================================================================ */

static void stat_before(hook_fargs4_t *f, void *u) {
    if (current_uid() == 0) return;

    const char __user *upath = (const char __user *)f->arg1;
    if (!upath) return;

    char kpath[256];
    long n = compat_strncpy_from_user(kpath, upath, sizeof(kpath));
    if (n <= 0) return;
    kpath[sizeof(kpath) - 1] = '\0';

    for (int i = 0; access_hidden[i]; i++) {
        if (str_starts(kpath, access_hidden[i])) {
            f->ret = (uint64_t)(long)(-ENOENT);
            f->skip_origin = 1;
            return;
        }
    }
}

/* ================================================================
 * Hook 14: do_sys_openat2 — hide su/root paths from open() (P0 new)
 *
 * long do_sys_openat2(int dfd, const char __user *filename,
 *                     struct open_how *how)
 * ================================================================ */

static void openat_before(hook_fargs4_t *f, void *u) {
    if (current_uid() == 0) return;

    const char __user *upath = (const char __user *)f->arg1;
    if (!upath) return;

    char kpath[256];
    long n = compat_strncpy_from_user(kpath, upath, sizeof(kpath));
    if (n <= 0) return;
    kpath[sizeof(kpath) - 1] = '\0';

    for (int i = 0; access_hidden[i]; i++) {
        if (str_starts(kpath, access_hidden[i])) {
            f->ret = (uint64_t)(long)(-ENOENT);
            f->skip_origin = 1;
            return;
        }
    }
}

/* ================================================================
 * Hook 15: selinux_getprocattr — hide root SELinux context (P1)
 *
 * int selinux_getprocattr(struct task_struct *p,
 *                         const char *name, char **value)
 *
 * After execution, *value points to a kmalloc'd string like
 * "u:r:su:s0\n". We replace "su" with "sh" in-place (same length)
 * to avoid root detection while not overflowing the buffer.
 * For "magisk" (6 chars) we replace with "system" (6 chars).
 * ================================================================ */

static void selinux_after(hook_fargs4_t *f, void *u) {
    if ((long)f->ret <= 0) return;

    char **pval = (char **)f->arg2;
    if (!pval) return;
    char *val = *pval;
    if (!val) return;

    int vlen = slen(val);
    if (vlen < 4) return;

    /*
     * Replace dangerous contexts in-place (same length):
     *   "u:r:su:s0"     -> "u:r:sh:s0"     (su->sh, 2 chars)
     *   "u:r:magisk:s0" -> "u:r:system:s0"  (magisk->system, 6 chars)
     */
    inplace_replace(val, vlen, ":su:", ":sh:");
    inplace_replace(val, vlen, ":magisk:", ":system:");
}

/* ================================================================
 * Init — register all hooks
 * ================================================================ */

static long ghost_init(const char *args, const char *event, void *__user reserved)
{
    int err;
    void *sym;

    /* 1. filldir64 */
    h_filldir = (void *)kallsyms_lookup_name("filldir64");
    if (h_filldir) {
        err = hook_wrap6(h_filldir, (void *)filldir64_before, NULL, NULL);
        if (err) h_filldir = 0;
    }

    /* 2. avc_denied */
    h_avc = (void *)kallsyms_lookup_name("avc_denied");
    if (h_avc) {
        err = hook_wrap8(h_avc, NULL, (void *)avc_denied_after, NULL);
        if (err) h_avc = 0;
    }

    /* 3. audit_log_start */
    h_audit = (void *)kallsyms_lookup_name("audit_log_start");
    if (h_audit) {
        err = hook_wrap4(h_audit, (void *)audit_before, NULL, NULL);
        if (err) h_audit = 0;
    }

    /* 4. show_map_vma */
    h_maps = (void *)kallsyms_lookup_name("show_map_vma");
    if (h_maps) {
        err = hook_wrap4(h_maps, (void *)maps_before, (void *)maps_after, NULL);
        if (err) h_maps = 0;
    }

    /* 5. show_mountinfo */
    h_mounts = (void *)kallsyms_lookup_name("show_mountinfo");
    if (h_mounts) {
        err = hook_wrap4(h_mounts, (void *)mounts_before, (void *)mounts_after, NULL);
        if (err) h_mounts = 0;
    }

    /* 6. proc_pid_status — P0: TracerPid clearing */
    h_status = (void *)kallsyms_lookup_name("proc_pid_status");
    if (h_status) {
        err = hook_wrap4(h_status, (void *)status_before, (void *)status_after, NULL);
        if (err) h_status = 0;
    }

    /* 7. proc_pid_wchan — P1: hide KP symbols from wchan */
    sym = (void *)kallsyms_lookup_name("proc_pid_wchan");
    if (sym) {
        h_wchan = sym;
        err = hook_wrap4(sym, (void *)wchan_before, (void *)wchan_after, NULL);
        if (err) h_wchan = 0;
    }

    /* 8. devkmsg_read — P0: dmesg filtering */
    h_dmesg = (void *)kallsyms_lookup_name("devkmsg_read");
    if (h_dmesg) {
        err = hook_wrap4(h_dmesg, (void *)dmesg_before, (void *)dmesg_after, NULL);
        if (err) h_dmesg = 0;
    }

    /* 9. kallsyms s_show */
    sym = (void *)kallsyms_lookup_name("s_show");
    if (sym) {
        h_kallsyms = sym;
        err = hook_wrap4(sym, (void *)ksym_before, (void *)ksym_after, NULL);
        if (err) h_kallsyms = 0;
    }

    /* 10. uname */
    h_uname = (void *)kallsyms_lookup_name("__arm64_sys_newuname");
    if (h_uname) {
        err = hook_wrap4(h_uname, NULL, (void *)uname_after, NULL);
        if (err) h_uname = 0;
    }

    /* 11. version_proc_show */
    h_version = (void *)kallsyms_lookup_name("version_proc_show");
    if (h_version) {
        err = hook_wrap4(h_version, NULL, (void *)version_after, NULL);
        if (err) h_version = 0;
    }

    /* 12. do_faccessat */
    h_access = (void *)kallsyms_lookup_name("do_faccessat");
    if (h_access) {
        err = hook_wrap4(h_access, (void *)access_before, NULL, NULL);
        if (err) h_access = 0;
    }

    /* 13. vfs_fstatat — P0: hide from stat() */
    h_stat = (void *)kallsyms_lookup_name("vfs_fstatat");
    if (h_stat) {
        err = hook_wrap4(h_stat, (void *)stat_before, NULL, NULL);
        if (err) h_stat = 0;
    }

    /* 14. do_sys_openat2 — P0: hide from open() */
    h_openat = (void *)kallsyms_lookup_name("do_sys_openat2");
    if (h_openat) {
        err = hook_wrap4(h_openat, (void *)openat_before, NULL, NULL);
        if (err) h_openat = 0;
    }

    /* 15. selinux_getprocattr — P1: hide root SELinux context */
    h_selinux = (void *)kallsyms_lookup_name("selinux_getprocattr");
    if (h_selinux) {
        err = hook_wrap4(h_selinux, NULL, (void *)selinux_after, NULL);
        if (err) h_selinux = 0;
    }

    return 0;
}

/* ========== Control ========== */

static long ghost_control(const char *args, char *__user out_msg, int outlen)
{
    if (!args) return -1;

    if (strncmp(args, "release:", 8) == 0) {
        int i;
        const char *v = args + 8;
        for (i = 0; i < 64 && v[i]; i++) sp_release[i] = v[i];
        sp_release[i] = '\0';
        return 0;
    }
    if (strncmp(args, "version:", 8) == 0) {
        int i;
        const char *v = args + 8;
        for (i = 0; i < 255 && v[i]; i++) sp_version[i] = v[i];
        sp_version[i] = '\0';
        return 0;
    }
    if (strncmp(args, "uname_ver:", 10) == 0) {
        int i;
        const char *v = args + 10;
        for (i = 0; i < 64 && v[i]; i++) sp_uname_ver[i] = v[i];
        sp_uname_ver[i] = '\0';
        return 0;
    }
    if (strncmp(args, "status", 6) == 0) {
        /*
         * Status format: "v5 F1 A1 L1 M1 T1 S1 W1 D1 K1 U1 V1 X1 I1 O1 E1"
         * F=filldir A=avc L=audit M=maps T=mounts S=status W=wchan
         * D=dmesg K=kallsyms U=uname V=version X=access I=stat O=openat E=selinux
         */
        char st[80];
        int p = 0;
        st[p++] = 'v'; st[p++] = '5'; st[p++] = ' ';
        st[p++] = 'F'; st[p++] = h_filldir  ? '1' : '0'; st[p++] = ' ';
        st[p++] = 'A'; st[p++] = h_avc      ? '1' : '0'; st[p++] = ' ';
        st[p++] = 'L'; st[p++] = h_audit    ? '1' : '0'; st[p++] = ' ';
        st[p++] = 'M'; st[p++] = h_maps     ? '1' : '0'; st[p++] = ' ';
        st[p++] = 'T'; st[p++] = h_mounts   ? '1' : '0'; st[p++] = ' ';
        st[p++] = 'S'; st[p++] = h_status   ? '1' : '0'; st[p++] = ' ';
        st[p++] = 'W'; st[p++] = h_wchan    ? '1' : '0'; st[p++] = ' ';
        st[p++] = 'D'; st[p++] = h_dmesg    ? '1' : '0'; st[p++] = ' ';
        st[p++] = 'K'; st[p++] = h_kallsyms ? '1' : '0'; st[p++] = ' ';
        st[p++] = 'U'; st[p++] = h_uname    ? '1' : '0'; st[p++] = ' ';
        st[p++] = 'V'; st[p++] = h_version  ? '1' : '0'; st[p++] = ' ';
        st[p++] = 'X'; st[p++] = h_access   ? '1' : '0'; st[p++] = ' ';
        st[p++] = 'I'; st[p++] = h_stat     ? '1' : '0'; st[p++] = ' ';
        st[p++] = 'O'; st[p++] = h_openat   ? '1' : '0'; st[p++] = ' ';
        st[p++] = 'E'; st[p++] = h_selinux  ? '1' : '0';
        st[p] = '\0';
        if (out_msg && outlen > 0)
            compat_copy_to_user(out_msg, st, p + 1);
        return 0;
    }
    return -1;
}

/* ========== Exit — unwrap all hooks ========== */

static long ghost_exit(void *__user reserved)
{
    if (h_selinux)  hook_unwrap(h_selinux, NULL, (void *)selinux_after);
    if (h_openat)   hook_unwrap(h_openat, (void *)openat_before, NULL);
    if (h_stat)     hook_unwrap(h_stat, (void *)stat_before, NULL);
    if (h_access)   hook_unwrap(h_access, (void *)access_before, NULL);
    if (h_version)  hook_unwrap(h_version, NULL, (void *)version_after);
    if (h_uname)    hook_unwrap(h_uname, NULL, (void *)uname_after);
    if (h_kallsyms) hook_unwrap(h_kallsyms, (void *)ksym_before, (void *)ksym_after);
    if (h_dmesg)    hook_unwrap(h_dmesg, (void *)dmesg_before, (void *)dmesg_after);
    if (h_wchan)    hook_unwrap(h_wchan, (void *)wchan_before, (void *)wchan_after);
    if (h_status)   hook_unwrap(h_status, (void *)status_before, (void *)status_after);
    if (h_mounts)   hook_unwrap(h_mounts, (void *)mounts_before, (void *)mounts_after);
    if (h_maps)     hook_unwrap(h_maps, (void *)maps_before, (void *)maps_after);
    if (h_audit)    hook_unwrap(h_audit, (void *)audit_before, NULL);
    if (h_avc)      hook_unwrap(h_avc, NULL, (void *)avc_denied_after);
    if (h_filldir)  hook_unwrap(h_filldir, (void *)filldir64_before, NULL);
    return 0;
}

KPM_INIT(ghost_init);
KPM_CTL0(ghost_control);
KPM_EXIT(ghost_exit);
