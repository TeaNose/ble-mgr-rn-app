// MagiskDetector.c
// Gabungan: original detector + DetectMagiskHide (is_supath + is_mountpaths) + isolated fork check

#include <jni.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <malloc.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/ptrace.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <errno.h>
#include <time.h>
#include <android/log.h>
#include <stdbool.h>
#include <limits.h>

#define LOG_TAG "MagiskDetector"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

#define TRUE JNI_TRUE
#define FALSE JNI_FALSE

// -----------------------------
// DETECTION: your original checks
// -----------------------------

// --- 1. Cek File Khas Magisk ---
jboolean checkMagiskFiles() {
    const char *paths[] = {
        "/sbin/magisk", "/init.magisk.rc", "/data/adb/magisk",
        "/dev/.magisk_unblock", "/cache/magisk.log", "/metadata/magisk"
    };
    for (int i = 0; i < sizeof(paths)/sizeof(paths[0]); i++) {
        if (access(paths[i], F_OK) == 0) {
            LOGD("Detected Magisk file: %s", paths[i]);
            return TRUE;
        }
    }
    return FALSE;
}

// --- 2. Cek mount point yang mencurigakan ---
jboolean checkMagiskMounts() {
    FILE *fp = fopen("/proc/self/mounts", "r");
    if (!fp) return FALSE;
    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "/dev/magisk") || strstr(line, "magisk.img") ||
            (strstr(line, "tmpfs") && strstr(line, "/dev") && strstr(line, "magisk"))) {
            fclose(fp);
            LOGD("Suspicious mount: %s", line);
            return TRUE;
        }
    }
    fclose(fp);
    return FALSE;
}

// --- 3. Cek loaded libraries ---
jboolean checkLoadedLibraries() {
    FILE *fp = fopen("/proc/self/maps", "r");
    if (!fp) return FALSE;
    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "magisk") || strstr(line, "zygisk") || strstr(line, "libsu")) {
            fclose(fp);
            LOGD("Suspicious library: %s", line);
            return TRUE;
        }
    }
    fclose(fp);
    return FALSE;
}

// --- 4. Cek syscall timing ---
jboolean checkSyscallTiming() {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    getpid();
    clock_gettime(CLOCK_MONOTONIC, &end);
    long diff = (end.tv_nsec - start.tv_nsec);
    if (diff < 0) diff = -diff;
    if (diff > 100000) {
        LOGD("Syscall timing anomaly: %ld ns", diff);
        return TRUE;
    }
    return FALSE;
}

// --- 5. Cek dengan inotify ---
jboolean checkInotify() {
    int fd = inotify_init();
    if (fd < 0) return FALSE;
    int wd = inotify_add_watch(fd, "/data/adb", IN_ALL_EVENTS);
    if (wd >= 0) {
        LOGD("Able to watch /data/adb via inotify");
        close(fd);
        return TRUE;
    }
    close(fd);
    return FALSE;
}

// --- 6. Cek logcat (debug Zygisk info) ---
jboolean checkLogcatZygisk() {
    FILE *fp = popen("logcat -d | grep zygisk", "r");
    if (!fp) return FALSE;
    char buffer[512];
    while (fgets(buffer, sizeof(buffer), fp)) {
        if (strstr(buffer, "zygisk") || strstr(buffer, "denylist")) {
            LOGD("Logcat zygisk: %s", buffer);
            pclose(fp);
            return TRUE;
        }
    }
    pclose(fp);
    return FALSE;
}

jboolean checkWithForkAccess() {
    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        if (access("/sbin/magisk", F_OK) == 0 ||
            access("/data/adb/magisk", F_OK) == 0) {
            _exit(1);
        }
        _exit(0);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 1) {
            LOGD("Detected Magisk with fork+access");
            return TRUE;
        }
    }
    return FALSE;
}

jboolean checkNamespaceIsolation() {
    char self_ns[PATH_MAX], init_ns[PATH_MAX];
    ssize_t len;

    len = readlink("/proc/1/ns/mnt", init_ns, sizeof(init_ns) - 1);
    if (len <= 0) return FALSE;
    init_ns[len] = '\0';

    len = readlink("/proc/self/ns/mnt", self_ns, sizeof(self_ns) - 1);
    if (len <= 0) return FALSE;
    self_ns[len] = '\0';

    if (strcmp(init_ns, self_ns) != 0) {
        LOGD("Namespace mismatch: self=%s init=%s", self_ns, init_ns);
        return TRUE;
    }
    return FALSE;
}

jboolean scanOtherProcMaps() {
    const char* target_proc[] = { "zygote", "system_server", "init" };
    DIR* dir = opendir("/proc");
    if (!dir) return FALSE;

    struct dirent* entry;
    while ((entry = readdir(dir))) {
        if (entry->d_type != DT_DIR) continue;

        int pid = atoi(entry->d_name);
        if (pid <= 0) continue;

        char path[64];
        snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);
        FILE* f = fopen(path, "r");
        if (!f) continue;

        char cmdline[256] = {0};
        fread(cmdline, 1, sizeof(cmdline) - 1, f);
        fclose(f);

        for (int i = 0; i < sizeof(target_proc) / sizeof(target_proc[0]); i++) {
            if (strstr(cmdline, target_proc[i])) {
                snprintf(path, sizeof(path), "/proc/%d/maps", pid);
                FILE* maps = fopen(path, "r");
                if (!maps) continue;

                char line[512];
                while (fgets(line, sizeof(line), maps)) {
                    if (strstr(line, "magisk") || strstr(line, "zygisk")) {
                        LOGD("Found Magisk lib in PID %d (%s): %s", pid, cmdline, line);
                        fclose(maps);
                        closedir(dir);
                        return TRUE;
                    }
                }
                fclose(maps);
            }
        }
    }

    closedir(dir);
    return FALSE;
}

jboolean checkHidepidProc() {
    FILE* f = fopen("/proc/mounts", "r");
    if (!f) return JNI_FALSE;

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "proc") && strstr(line, "hidepid=")) {
            if (strstr(line, "hidepid=2")) {
                LOGD("Possible Magisk/Zygisk hiding detected via hidepid=2: %s", line);
                fclose(f);
                return JNI_TRUE;
            }
        }
    }
    fclose(f);
    return JNI_FALSE;
}

jboolean checkUidNamespaceMismatch() {
    struct stat st_self, st_init;
    if (stat("/proc/self/ns/mnt", &st_self) != 0 ||
        stat("/proc/1/ns/mnt", &st_init) != 0)
        return FALSE;

    if (st_self.st_ino != st_init.st_ino) {
        LOGD("Mount namespace mismatch (self vs init)");
        return TRUE;
    }
    return FALSE;
}

// -----------------------------
// DETECTION: functions from DetectMagiskHide (native-lib.c)
// -----------------------------

static const char *dmh_tag = "DetectMagiskNative"; // alternate tag if needed

static char *blacklistedMountPaths[] = {
    "magisk",
    "core/mirror",
    "core/img"
};

static const char *suPaths[] = {
    "/data/local/su",
    "/data/local/bin/su",
    "/data/local/xbin/su",
    "/sbin/su",
    "/su/bin/su",
    "/system/bin/su",
    "/system/bin/.ext/su",
    "/system/bin/failsafe/su",
    "/system/sd/xbin/su",
    "/system/usr/we-need-root/su",
    "/system/xbin/su",
    "/cache/su",
    "/data/su",
    "/dev/su"
};

// is_supath_detected_local - cek keberadaan file 'su' pada banyak path
static inline jboolean is_supath_detected_local() {
    int len = sizeof(suPaths) / sizeof(suPaths[0]);
    for (int i = 0; i < len; i++) {
        LOGD("Checking SU Path : %s", suPaths[i]);
        if (open(suPaths[i], O_RDONLY) >= 0) {
            LOGD("Found SU Path (open): %s", suPaths[i]);
            return TRUE;
        }
        if (access(suPaths[i], R_OK) == 0) {
            LOGD("Found SU Path (access): %s", suPaths[i]);
            return TRUE;
        }
    }
    return FALSE;
}

// is_mountpaths_detected_local - baca /proc/self/mounts keseluruhan lalu cari kata kunci
static inline jboolean is_mountpaths_detected_local() {
    int len = sizeof(blacklistedMountPaths) / sizeof(blacklistedMountPaths[0]);
    FILE *fp = fopen("/proc/self/mounts", "r");
    if (!fp) return FALSE;

    // coba dapatkan ukuran. jika 0, pakai default buffer besar
    fseek(fp, 0L, SEEK_END);
    long size = ftell(fp);
    if (size <= 0) size = 20000;
    rewind(fp);

    char *buffer = calloc(size + 1, sizeof(char));
    if (!buffer) {
        fclose(fp);
        return FALSE;
    }
    size_t read = fread(buffer, 1, size, fp);
    (void)read; // tidak dipakai secara eksplisit

    for (int i = 0; i < len; i++) {
        LOGD("Checking Mount Path  : %s", blacklistedMountPaths[i]);
        if (strstr(buffer, blacklistedMountPaths[i]) != NULL) {
            LOGD("Found Mount Path : %s", blacklistedMountPaths[i]);
            free(buffer);
            fclose(fp);
            return TRUE;
        }
    }

    free(buffer);
    fclose(fp);
    return FALSE;
}

// -----------------------------
// Isolated check: jalankan is_supath_detected_local + is_mountpaths_detected_local di child (fork)
// -----------------------------
jboolean detectMagiskHideIsolated() {
    pid_t pid = fork();
    if (pid == -1) {
        // fork gagal
        LOGD("fork() failed in detectMagiskHideIsolated: %s", strerror(errno));
        return FALSE;
    } else if (pid == 0) {
        // CHILD: jalankan pemeriksaan "native-lib style"
        if (is_supath_detected_local() || is_mountpaths_detected_local()) {
            // exit code 1 -> terdeteksi
            _exit(1);
        }
        // exit code 0 -> tidak terdeteksi
        _exit(0);
    } else {
        // PARENT: tunggu hasil child
        int status = 0;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 1) {
            LOGD("[Parent] detectMagiskHideIsolated -> DETECTED");
            return TRUE;
        }
    }
    return FALSE;
}

// -----------------------------
// Kombinasi utama: satukan semua lapisan deteksi
// -----------------------------
jboolean isMagiskDetected() {
    return (
        checkMagiskFiles() ||
        checkMagiskMounts() ||
        checkLoadedLibraries() ||
        checkSyscallTiming() ||
        checkInotify() ||
        checkLogcatZygisk() ||
        checkWithForkAccess() ||
        checkNamespaceIsolation() ||
        scanOtherProcMaps() ||
        checkHidepidProc() ||
        checkUidNamespaceMismatch() ||
        detectMagiskHideIsolated()   // <-- tambahan DetectMagiskHide (isolated)
    );
}

// -----------------------------
// JNI ENTRY
// -----------------------------
JNIEXPORT jboolean JNICALL
Java_com_blemanagerapps_NativeRootDetection_nativeIsRooted(JNIEnv* env, jobject thiz) {
    LOGD("Magisk detection started");
    jboolean ret = isMagiskDetected();
    LOGD("Magisk detection finished: %d", ret == JNI_TRUE ? 1 : 0);
    return ret;
}
