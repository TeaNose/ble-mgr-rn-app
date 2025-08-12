// EnhancedRootDetectionModule.java - PASSIVE DETECTION ONLY
package com.blemanagerapps;

import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.os.Build;
import com.facebook.react.bridge.Promise;
import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.bridge.ReactContextBaseJavaModule;
import com.facebook.react.bridge.ReactMethod;
import com.facebook.react.bridge.WritableMap;
import com.facebook.react.bridge.Arguments;
import java.io.File;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.util.Arrays;
import java.util.List;

import com.scottyab.rootbeer.RootBeer;

import android.util.Log;

public class EnhancedRootDetectionModule extends ReactContextBaseJavaModule {
    private final ReactApplicationContext reactContext;

    // SU binary paths - CHECK ONLY, NEVER EXECUTE
    private static final String[] SU_PATHS = {
        "/system/app/Superuser.apk",
        "/sbin/su",
        "/system/bin/su", 
        "/system/xbin/su",
        "/data/local/xbin/su",
        "/data/local/bin/su",
        "/system/sd/xbin/su",
        "/system/bin/failsafe/su",
        "/data/local/su",
        "/su/bin/su",
        "/system/xbin/daemonsu"
    };

    // Root management apps
    private static final String[] ROOT_PACKAGES = {
        "com.noshufou.android.su",
        "com.noshufou.android.su.elite", 
        "eu.chainfire.supersu",
        "com.koushikdutta.superuser",
        "com.thirdparty.superuser",
        "com.yellowes.su",
        "com.topjohnwu.magisk",
        "com.kingroot.kinguser",
        "com.kingo.root",
        "com.smedialink.oneclickroot",
        "com.zhiqupk.root.global",
        "com.alephzain.framaroot",
        "com.android.vending.billing.InAppBillingService.COIN",
        "com.chelpus.lackypatch",
        "com.ramdroid.appquarantine",
        "com.topjohnwu.magisk"
    };

    // Potentially dangerous apps that modify system
    private static final String[] DANGEROUS_PACKAGES = {
        "com.koushikdutta.rommanager",
        "com.koushikdutta.rommanager.license",
        "com.dimonvideo.luckypatcher",
        "com.chelpus.lackypatch",
        "com.android.vending.billing.InAppBillingService.COIN",
        "uret.jasi2169.patcher",
        "com.forpda.luckypatcherinstaller",
        "com.android.vending.billing.InAppBillingService.LACK"
    };

    public EnhancedRootDetectionModule(ReactApplicationContext reactContext) {
        super(reactContext);
        this.reactContext = reactContext;
    }

    @Override
    public String getName() {
        return "EnhancedRootDetectionModule";
    }

    @ReactMethod
    public void isRooted(Promise promise) {
        try {
            RootBeer rootBeer = new RootBeer(this.reactContext);
            boolean rooted = checkSuExists() || 
                           checkPackages() || 
                           checkBuildTags() || 
                           checkProps() ||
                           checkPaths() ||
                           checkRWPaths() || rootBeer.isRooted();
            promise.resolve(rooted);
        } catch (Exception e) {
            promise.reject("ROOT_DETECTION_ERROR", e.getMessage());
        }
    }

    @ReactMethod
    public void getDetailedRootInfo(Promise promise) {
        try {
            WritableMap result = Arguments.createMap();
            WritableMap checks = Arguments.createMap();
            RootBeer rootBeer = new RootBeer(this.reactContext);

            NativeRootDetection nativeCheck = new NativeRootDetection();
            boolean nativeRooted = nativeCheck.nativeIsRooted();
            nativeCheck.checkAllRootPossibility();

            RootDetector rootDetector = new RootDetector(this.reactContext);
            List<RootDetector.Detection> detections = rootDetector.runAllChecks();

            if (detections.isEmpty()) {
                Log.i("EnhancedRootDetection", "No root detected.");
            } else {
                for (RootDetector.Detection detection : detections) {
                    Log.w("EnhancedRootDetection", "Root indicator: " +
                            detection.key + " (" + detection.details + ")");
                }
            }

            Log.d("Native Root Detection: ", String.valueOf(nativeRooted));

            boolean suExists = checkSuExists();
            Log.d("suExists", String.valueOf(suExists));

            boolean packagesFound = checkPackages();
            Log.d("packagesFound", String.valueOf(packagesFound));

            boolean buildTags = checkBuildTags();
            Log.d("buildTags", String.valueOf(buildTags));

            boolean props = checkProps();
            Log.d("props", String.valueOf(props));

            boolean paths = checkPaths();
            Log.d("paths", String.valueOf(paths));

            boolean rwPaths = checkRWPaths();
            Log.d("rwPaths", String.valueOf(rwPaths));

            boolean dangerousApps = checkDangerousApps();
            Log.d("dangerousApps", String.valueOf(dangerousApps));

            boolean isDeviceRooted = rootBeer.isRooted();
            Log.d("isDeviceRooted", String.valueOf(isDeviceRooted));

            boolean isSuRunning = isSuProcessRunning();
            Log.d("isSuRunning", String.valueOf(isSuRunning));

            checks.putBoolean("suBinaryExists", suExists);
            checks.putBoolean("rootPackagesFound", packagesFound);
            checks.putBoolean("testKeys", buildTags);
            checks.putBoolean("dangerousProps", props);
            checks.putBoolean("suspiciousPaths", paths);
            checks.putBoolean("rwSystemPartition", rwPaths);
            checks.putBoolean("dangerousApps", dangerousApps);
            checks.putBoolean("isDeviceRootedRootBeer", isDeviceRooted);
            checks.putBoolean("nativeRooted", nativeRooted);

            result.putMap("checks", checks);
            result.putBoolean("isRooted", suExists || packagesFound || buildTags || props || paths || rwPaths || isDeviceRooted || nativeRooted);
            result.putInt("riskScore", calculateRiskScore(suExists, packagesFound, buildTags, props, paths, rwPaths, dangerousApps, isDeviceRooted, nativeRooted));
            
            promise.resolve(result);
        } catch (Exception e) {
            promise.reject("DETAILED_ROOT_ERROR", e.getMessage());
        }
    }

    // Method 1: Check if SU binary exists (FILE EXISTENCE ONLY)
    private boolean checkSuExists() {
        for (String path : SU_PATHS) {
            try {
                File file = new File(path);
                if (file.exists()) {
                    return true;
                }
            } catch (Exception e) {
                // File access exception - continue checking
            }
        }
        return false;
    }

    // Method 2: Check for root packages (PASSIVE)
    private boolean checkPackages() {
        PackageManager pm = reactContext.getPackageManager();
        for (String packageName : ROOT_PACKAGES) {
            try {
                pm.getPackageInfo(packageName, 0);
                return true;
            } catch (PackageManager.NameNotFoundException e) {
                // Continue
            }
        }
        return false;
    }

    // Method 3: Check build tags
    private boolean checkBuildTags() {
        String buildTags = Build.TAGS;
        return buildTags != null && buildTags.contains("test-keys");
    }

    // Method 4: Check system properties (SAFE METHODS ONLY)
    private String getSystemProperty(String propName) {
        try {
            Process process = Runtime.getRuntime().exec("getprop " + propName);
            BufferedReader reader = new BufferedReader(new InputStreamReader(process.getInputStream()));
            String line = reader.readLine();
            reader.close();
            return line != null ? line : "";
        } catch (Exception e) {
            return "";
        }
    }
    private boolean checkProps() {
        try {
            // Check ro.debuggable (should be 0 in production)
            String debuggable = getSystemProperty("ro.debuggable");
            if ("1".equals(debuggable)) {
                return true;
            }

            // Check ro.secure (should be 1 in production)  
            String secure = getSystemProperty("ro.secure");
            if ("0".equals(secure)) {
                return true;
            }
        } catch (Exception e) {
            // Property access failed
        }
        return false;
    }

    // Method 5: Check for suspicious paths
    private boolean checkPaths() {
        String[] suspiciousPaths = {
            "/system/recovery-from-boot.p",
            "/system/etc/init.d/99SuperSUDaemon",
            "/dev/com.koushikdutta.superuser.daemon/",
            "/system/xbin/daemonsu"
        };
        
        for (String path : suspiciousPaths) {
            try {
                if (new File(path).exists()) {
                    return true;
                }
            } catch (Exception e) {
                // Continue
            }
        }
        return false;
    }

    // Method 6: Check if system is mounted as RW (SAFE CHECK)
    private boolean checkRWPaths() {
        try {
            // Try to read mount information
            Process proc = Runtime.getRuntime().exec("cat /proc/mounts");
            BufferedReader reader = new BufferedReader(new InputStreamReader(proc.getInputStream()));
            String line;
            
            while ((line = reader.readLine()) != null) {
                if (line.contains(" /system ") && line.contains("rw")) {
                    reader.close();
                    return true;
                }
            }
            reader.close();
        } catch (Exception e) {
            // Failed to read mounts
        }
        return false;
    }

    // Method 7: Check for dangerous apps
    private boolean checkDangerousApps() {
        PackageManager pm = reactContext.getPackageManager();
        for (String packageName : DANGEROUS_PACKAGES) {
            try {
                pm.getPackageInfo(packageName, 0);
                return true;
            } catch (PackageManager.NameNotFoundException e) {
                // Continue
            }
        }
        return false;
    }
        
    // Method 7: Check for running SU
    private boolean isSuProcessRunning() {
    try {
        Process process = Runtime.getRuntime().exec("ps");
        BufferedReader in = new BufferedReader(new InputStreamReader(process.getInputStream()));
        String line;
        while ((line = in.readLine()) != null) {
            if (line.contains("su")) return true;
        }
        in.close();
    } catch (Exception e) {
        // Ignore
    }
    return false;
}

    // Calculate risk score (0-100)
    private int calculateRiskScore(boolean suExists, boolean packages, boolean buildTags, 
                                 boolean props, boolean paths, boolean rwPaths, boolean dangerous, boolean isDeviceRooted, boolean nativeRooted) {
        int score = 0;
        if (suExists) score += 30;      // High risk
        if (packages) score += 25;      // High risk  
        if (buildTags) score += 15;     // Medium risk
        if (props) score += 10;         // Medium risk
        if (paths) score += 10;         // Medium risk
        if (rwPaths) score += 5;        // Low risk
        if (dangerous) score += 5; 
        if (isDeviceRooted) score += 30;     // Low risk
        if (nativeRooted) score += 50;
        
        return Math.min(score, 100);
    }
}