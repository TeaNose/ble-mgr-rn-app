#include <jni.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <android/log.h>

#define LOG_TAG "NativeRootDetection"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Helper: append result into an array-like vector (C-style)
#define MAX_RESULTS 128
#define MAX_STR_LEN 1024

static char results[MAX_RESULTS][MAX_STR_LEN];
static int results_count = 0;

static void push_result(const char* key, const char* details) {
    if (results_count >= MAX_RESULTS) return;
    if (!key) return;
    if (!details) details = "";
    snprintf(results[results_count], MAX_STR_LEN, "%s|%s", key, details);
    results_count++;
}

// ----- Checks -----

// 1) Look for su binary in common locations (native version)
static void check_su_paths() {
    const char* paths[] = {
        "/system/bin/su",
        "/system/xbin/su",
        "/sbin/su",
        "/vendor/bin/su",
        "/su/bin/su",
        "/data/local/xbin/su",
        "/data/local/bin/su",
        "/data/local/su",
        NULL
    };
    for (int i=0; paths[i]; ++i) {
        struct stat st;
        if (stat(paths[i], &st) == 0) {
            push_result("found_su_binary", paths[i]);
        }
    }
}

// 2) Check Magisk specific files and directories
static void check_magisk_paths() {
    const char* paths[] = {
        "/sbin/.magisk",
        "/data/adb/magisk",
        "/data/adb/modules",
        "/magisk",
        "/data/adb/magisk.db",
        NULL
    };
    for (int i=0; paths[i]; ++i) {
        struct stat st;
        if (stat(paths[i], &st) == 0) {
            push_result("detected_magisk", paths[i]);
        }
    }
}

// 3) Detect Zygisk-like anonymous RWX mappings by parsing /proc/self/maps
static void check_proc_maps_for_zygisk() {
    FILE* f = fopen("/proc/self/maps", "r");
    if (!f) return;
    char line[1024];
    int anon_rwx_count = 0;
    while (fgets(line, sizeof(line), f)) {
        // line example: 7f6c1e0000-7f6c1e3000 rwxp 00000000 00:00 0                          [anon:zygisk]
        if (strstr(line, "rwx") && (strstr(line, "anon") || strstr(line, "[anon]"))) {
            anon_rwx_count++;
            if (anon_rwx_count <= 5) {
                // include a sample line as detail
                char sample[512];
                snprintf(sample, sizeof(sample), "%s", line);
                push_result("detected_zygisk", sample);
            }
        }
        // also check for library names that hint zygisk/ksu
        if (strstr(line, "zygisk") || strstr(line, "ksu") || strstr(line, "magisk")) {
            push_result("detected_zygisk_assistant", line);
        }
    }
    if (anon_rwx_count > 0) {
        char detail[64];
        snprintf(detail, sizeof(detail), "anon_rwx=%d", anon_rwx_count);
        push_result("detected_zygisk_next", detail);
    }
    fclose(f);
}

// 4) Compare /proc/self/mountinfo vs /proc/1/mountinfo to detect hidden mounts
static void check_mount_inconsistency() {
    FILE* f1 = fopen("/proc/self/mountinfo", "r");
    FILE* f2 = fopen("/proc/1/mountinfo", "r");
    if (!f1 || !f2) {
        if (f1) fclose(f1);
        if (f2) fclose(f2);
        return;
    }
    // Build a quick set of mountpoints from proc/1
    char buf[1024];
    // store /proc/1 mount points into a simple list
    char mounts1[1024][256];
    int m1 = 0;
    while (fgets(buf, sizeof(buf), f2) && m1 < 1024) {
        // parse mount point (field 5). Simplest approach: find first space after ID fields
        // We'll search for ' - ' which separates optional fields if present; fallback to full line
        char *p = strchr(buf, '/');
        if (p) {
            // trim newline
            char *nl = strchr(p, '\n'); if (nl) *nl = '\0';
            strncpy(mounts1[m1++], p, 255);
        }
    }
    // Now check proc/self and see if any mount doesn't exist in proc/1
    while (fgets(buf, sizeof(buf), f1)) {
        char *p = strchr(buf, '/');
        if (p) {
            char mp[256];
            char *nl = strchr(p, '\n'); if (nl) *nl = '\0';
            strncpy(mp, p, 255);
            int found = 0;
            for (int i=0;i<m1;i++) {
                if (strstr(mounts1[i], mp) != NULL || strstr(mp, mounts1[i]) != NULL) { found = 1; break; }
            }
            if (!found) {
                // Found mount that appears in self but not in init — suspicious
                push_result("detected_mount_inconsistency", mp);
            }
        }
    }
    fclose(f1); fclose(f2);
}

// 5) Detect overlayfs
static void check_overlayfs() {
    FILE* f = fopen("/proc/filesystems", "r");
    if (!f) return;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "overlay")) {
            push_result("detected_overlayfs", "overlay supported");
            break;
        }
    }
    fclose(f);
    // also check mounts for overlay keyword
    f = fopen("/proc/mounts", "r");
    if (!f) return;
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "overlay")) {
            push_result("detected_overlay_mounted", line);
            break;
        }
    }
    fclose(f);
}

// 6) Detect resetprop
static void check_resetprop() {
    const char* paths[] = {"/system/bin/resetprop","/system/xbin/resetprop","/sbin/resetprop","/vendor/bin/resetprop","/system/bin/.ext/.resetprop","/data/local/tmp/resetprop", NULL};
    for (int i=0; paths[i]; ++i) {
        struct stat st;
        if (stat(paths[i], &st) == 0) {
            push_result("detected_resetprop", paths[i]);
        }
    }
}

// 7) Detect modified hosts file
static void check_hosts_file() {
    const char* hosts[] = {"/system/etc/hosts", "/etc/hosts", NULL};
    for (int i=0; hosts[i]; ++i) {
        struct stat st;
        if (stat(hosts[i], &st) == 0) {
            if (st.st_size > 2048) {
                push_result("detected_modified_hosts_file", hosts[i]);
            }
        }
    }
}

// 8) Detect common Magisk module markers like 'hide' or 'ksu' in /data/adb/modules
static void check_magisk_modules() {
    const char* dir = "/data/adb/modules";
    DIR* d = opendir(dir);
    if (!d) return;
    struct dirent* entry;
    while ((entry = readdir(d))) {
        if (entry->d_name[0] == '.') continue;
        char path[512];
        snprintf(path, sizeof(path), "%s/%s", dir, entry->d_name);
        push_result("detected_magisk_module", path);
    }
    closedir(d);
}

// 9) Detect suspicious apps by reading /data/app (note: might require permissions on non-rooted devices, but we'll attempt)
static void check_installed_packages_dir() {
    const char* dir = "/data/app";
    DIR* d = opendir(dir);
    if (!d) return;
    struct dirent* entry;
    while ((entry = readdir(d))) {
        if (entry->d_name[0] == '.') continue;
        // heuristic: packages under data/app commonly contain package-name strings
        if (strstr(entry->d_name, "magisk") || strstr(entry->d_name, "superuser") || strstr(entry->d_name, "lsposed")) {
            push_result("detected_risky_app", entry->d_name);
        }
    }
    closedir(d);
}

// 10) Check for KSU/AP modules image (ksu-specific hints)
static void check_ksu_ap_modules_img() {
    const char* paths[] = {"/apex/com.kernelsu", "/data/ksu/modules.img", NULL};
    for (int i=0; paths[i]; ++i) {
        struct stat st;
        if (stat(paths[i], &st) == 0) {
            push_result("detected_ksu_ap_modules_img", paths[i]);
        }
    }
}

// 11) Bootloader/unlock checks (best effort via ro properties)
static void check_bootloader_and_oem() {
    // read /proc/cmdline or /sys/class/android_usb/android0/state? limited. Use getprop via reading /system/build.prop fallback
    FILE* f = popen("getprop ro.boot.verifiedbootstate 2>/dev/null", "r");
    if (f) {
        char buf[128];
        if (fgets(buf, sizeof(buf), f)) {
            if (strstr(buf, "orange") || strstr(buf, "unlocked") || strstr(buf, "unknown")) {
                push_result("bootloader_unlocked", buf);
            }
        }
        pclose(f);
    }
    f = popen("getprop ro.oem_unlock_supported 2>/dev/null", "r");
    if (f) {
        char buf[128];
        if (fgets(buf, sizeof(buf), f)) {
            if (strstr(buf, "1")) push_result("oem_unlocking_enabled", buf);
        }
        pclose(f);
    }
}

// 12) TEE / keybox checks (best effort placeholder — real attestation must use Android APIs)
static void check_keybox_and_tee() {
    // Attempt to read keymaster info via getprop
    FILE* f = popen("getprop ro.boot.verifiedbootstate 2>/dev/null", "r");
    if (!f) return;
    char buf[128];
    if (fgets(buf, sizeof(buf), f)) {
        if (strstr(buf, "orange") || strstr(buf, "failed")) {
            push_result("tee_is_broken", buf);
        }
    }
    pclose(f);
}

// 13) Generic detection of injected libraries in /proc/self/maps
static void check_injected_libraries() {
    FILE* f = fopen("/proc/self/maps", "r");
    if (!f) return;
    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, ".so") && (strstr(line, "/data/adb") || strstr(line, "/magisk") || strstr(line, "lsposed") || strstr(line, "zygisk"))) {
            push_result("found_injection", line);
        }
        if (strstr(line, "lsposed")) {
            push_result("detected_lsposed", line);
        }
    }
    fclose(f);
}

// 14) Detect hide_my_applist module heuristics
static void check_hide_my_applist() {
    // Check common module names
    const char* names[] = {"hmap", "hide_my_applist", "hideapplist", NULL};
    for (int i=0; names[i]; ++i) {
        char path[256];
        snprintf(path, sizeof(path), "/data/adb/modules/%s", names[i]);
        struct stat st;
        if (stat(path, &st) == 0) push_result("detected_hide_my_applist", path);
    }
}

// 15) Detect custom ROM / lineage
static void check_custom_rom() {
    FILE* f = popen("getprop ro.build.fingerprint 2>/dev/null", "r");
    if (!f) return;
    char buf[512];
    if (fgets(buf, sizeof(buf), f)) {
        if (strstr(buf, "lineage") || strstr(buf, "omni") || strstr(buf, "aosp")) {
            push_result("detected_lineageos", buf);
            // vendor sepolicy check
            FILE* v = fopen("/vendor/etc/selinux/vendor_sepolicy.cil", "r");
            if (v) {
                char tmp[256];
                while (fgets(tmp, sizeof(tmp), v)) {
                    if (strstr(tmp, "lineage")) {
                        push_result("vendor_sepolicy_contains_lineage", "vendor_sepolicy.cil");
                        break;
                    }
                }
                fclose(v);
            }
        }
    }
    pclose(f);
}

// 16) Detected custom kernel
static void check_custom_kernel() {
    FILE* f = fopen("/proc/version", "r");
    if (!f) return;
    char buf[512];
    if (fgets(buf, sizeof(buf), f)) {
        if (strstr(buf, "android") == NULL || strstr(buf, "gcc") != NULL) {
            push_result("detected_custom_kernel", buf);
        }
    }
    fclose(f);
}

// 17) Detect GApps (heuristic)
static void check_gapps() {
    // look for package com.google.android.gms directory under /data/app or /system
    const char* paths[] = {"/system/priv-app/GoogleGmsCore","/data/app/com.google.android.gms-","/system/app/GoogleServicesFramework", NULL};
    for (int i=0; paths[i]; ++i) {
        struct stat st;
        if (stat(paths[i], &st) == 0) push_result("detected_gapps", paths[i]);
    }
}

// 18) Detect framework patch (smali or Xposed/LSPosed-like patches)
static void check_framework_patch() {
    // heuristics: /system/framework/XposedBridge.jar or modifications to /system/framework
    struct stat st;
    if (stat("/system/framework/XposedBridge.jar", &st) == 0) push_result("detected_framework_patch", "/system/framework/XposedBridge.jar");
}

// Main aggregator
static void run_all_checks_native() {
    results_count = 0;
    check_su_paths();
    check_magisk_paths();
    check_proc_maps_for_zygisk();
    check_mount_inconsistency();
    check_overlayfs();
    check_resetprop();
    check_hosts_file();
    check_magisk_modules();
    check_installed_packages_dir();
    check_ksu_ap_modules_img();
    check_bootloader_and_oem();
    check_keybox_and_tee();
    check_injected_libraries();
    check_hide_my_applist();
    check_custom_rom();
    check_custom_kernel();
    check_gapps();
    check_framework_patch();
}

// JNI bridge
JNIEXPORT jobjectArray JNICALL
Java_com_blemanagerapps_NativeRootDetection_checkAllRootPossibility(JNIEnv* env, jobject thiz) {
    run_all_checks_native();

    LOGI("Running all root detection checks...");
    LOGI("Total results: %d", results_count);

    jclass stringClass = (*env)->FindClass(env, "java/lang/String");
    jobjectArray arr = (*env)->NewObjectArray(env, results_count, stringClass, NULL);

    for (int i = 0; i < results_count; i++) {
        LOGI("Check #%d result: %s", i + 1, results[i]); // <-- log result
        jstring s = (*env)->NewStringUTF(env, results[i]);
        (*env)->SetObjectArrayElement(env, arr, i, s);
        (*env)->DeleteLocalRef(env, s);
    }

    LOGI("All checks completed, returning results to Java.");
    return arr;
}