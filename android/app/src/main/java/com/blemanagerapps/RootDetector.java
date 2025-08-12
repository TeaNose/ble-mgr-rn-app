package com.blemanagerapps;

import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Build;
import android.util.Log;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class RootDetector {
    private final Context context;

    static {
        try {
            System.loadLibrary("root_detect");
        } catch (UnsatisfiedLinkError e) {
            Log.w("RootDetector", "native lib not found: " + e.getMessage());
        }
    }

    public static class Detection {
        public final String key;
        public final String details;

        public Detection(String key, String details) {
            this.key = key;
            this.details = details;
        }
    }

    public RootDetector(Context context) {
        this.context = context;
    }

    public List<Detection> runAllChecks() {
        List<Detection> results = new ArrayList<>();
        Log.d("RootDetector", "Starting root checks...");

        Detection su = checkSuBinaries();
        if (su != null) {
            results.add(su);
            Log.d("RootDetector", "SU binary detected at: " + su.details);
        }

        Detection magisk = checkMagiskPaths();
        if (magisk != null) {
            results.add(magisk);
            Log.d("RootDetector", "Magisk path detected: " + magisk.details);
        }

        List<Detection> pkgs = checkPackages();
        if (pkgs != null) {
            results.addAll(pkgs);
            for (Detection d : pkgs) {
                Log.d("RootDetector", "Root app package detected: " + d.details);
            }
        }

        Detection hosts = checkHostsFile();
        if (hosts != null) {
            results.add(hosts);
            Log.d("RootDetector", "Modified hosts file detected: " + hosts.details);
        }

        Detection addon = checkAddonOrInstallRecovery();
        if (addon != null) {
            results.add(addon);
            Log.d("RootDetector", "Addon/install-recovery detected: " + addon.details);
        }

        Detection debugFp = checkDebugFingerprint();
        if (debugFp != null) {
            results.add(debugFp);
            Log.d("RootDetector", "Debug fingerprint detected: " + debugFp.details);
        }

        Log.d("RootDetector", "Root checks complete. Total detections: " + results.size());
        return results;
    }


    private Detection checkSuBinaries() {
        List<String> paths = Arrays.asList(
                "/system/bin/su",
                "/system/xbin/su",
                "/sbin/su",
                "/vendor/bin/su",
                "/su/bin/su",
                "/magisk/su",
                "/data/local/xbin/su",
                "/data/local/bin/su",
                "/data/local/su"
        );
        for (String p : paths) {
            try {
                File f = new File(p);
                if (f.exists()) {
                    return new Detection("found_su_binary", p);
                }
            } catch (Exception ignored) {}
        }
        return null;
    }

    private Detection checkMagiskPaths() {
        List<String> magiskPaths = Arrays.asList(
                "/sbin/.magisk",
                "/data/adb/magisk",
                "/data/adb/modules",
                "/magisk",
                "/data/adb/magisk.db"
        );
        for (String p : magiskPaths) {
            try {
                File f = new File(p);
                if (f.exists()) {
                    return new Detection("detected_magisk", p);
                }
            } catch (Exception ignored) {}
        }
        return null;
    }

    private List<Detection> checkPackages() {
        List<Detection> detections = new ArrayList<>();
        List<String> knownRootApps = Arrays.asList(
                "com.topjohnwu.magisk",
                "com.zachspong.temprootremovejb",
                "eu.chainfire.supersu",
                "com.noshufou.android.su",
                "com.thirdparty.superuser",
                "com.koushikdutta.superuser",
                "com.kingouser.com"
        );
        PackageManager pm = context.getPackageManager();
        for (String pkg : knownRootApps) {
            try {
                pm.getPackageInfo(pkg, 0);
                detections.add(new Detection("detected_root_app", pkg));
            } catch (PackageManager.NameNotFoundException ignored) {}
        }
        return detections.isEmpty() ? null : detections;
    }

    private Detection checkHostsFile() {
        List<String> hostsPaths = Arrays.asList("/system/etc/hosts", "/etc/hosts");
        for (String p : hostsPaths) {
            try {
                File f = new File(p);
                if (f.exists() && f.length() > 2048) {
                    return new Detection("detected_modified_hosts_file", p);
                }
            } catch (Exception ignored) {}
        }
        return null;
    }

    private Detection checkAddonOrInstallRecovery() {
        List<String> paths = Arrays.asList(
                "/system/addon.d",
                "/system/etc/install-recovery.sh",
                "/vendor/addon.d"
        );
        for (String p : paths) {
            try {
                File f = new File(p);
                if (f.exists()) {
                    return new Detection("addon_d_or_install_recovery_sh_exists", p);
                }
            } catch (Exception ignored) {}
        }
        return null;
    }

    private Detection checkDebugFingerprint() {
        String fp = Build.FINGERPRINT;
        if (fp != null && (fp.contains("test-keys") || fp.contains("dev-keys") || fp.contains("debug"))) {
            return new Detection("debug_fingerprint_detected", fp);
        }
        return null;
    }
}
