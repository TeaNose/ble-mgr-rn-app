// RootDetectionModule.java
package com.blemanagerapps;

import android.content.Context;
import android.content.pm.PackageManager;
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

public class RootDetectionModule extends ReactContextBaseJavaModule {
    private final ReactApplicationContext reactContext;

    // Common root detection paths
    private static final String[] ROOT_PATHS = {
        "/system/app/Superuser.apk",
        "/sbin/su",
        "/system/bin/su",
        "/system/xbin/su",
        "/data/local/xbin/su",
        "/data/local/bin/su",
        "/system/sd/xbin/su",
        "/system/bin/failsafe/su",
        "/data/local/su",
        "/su/bin/su"
    };

    // Common root apps package names
    private static final String[] ROOT_APPS = {
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
        "com.alephzain.framaroot"
    };

    // Xposed framework indicators
    private static final String[] XPOSED_INDICATORS = {
        "de.robv.android.xposed.installer",
        "de.robv.android.xposed.installer3",
        "io.va.exposed"
    };

    public RootDetectionModule(ReactApplicationContext reactContext) {
        super(reactContext);
        this.reactContext = reactContext;
    }

    @Override
    public String getName() {
        return "RootDetectionModule";
    }

    @ReactMethod
    public void isRooted(Promise promise) {
        try {
            RootBeer rootBeer = new RootBeer(reactContext);
            boolean rooted = checkRootMethod1() || checkRootMethod2() || checkRootMethod3() || checkRootMethod4() || rootBeer.isRooted();
            promise.resolve(rooted);
        } catch (Exception e) {
            promise.reject("ROOT_DETECTION_ERROR", e.getMessage());
        }
    }

    @ReactMethod
    public void getRootInfo(Promise promise) {
        try {
            WritableMap rootInfo = Arguments.createMap();
            WritableMap checks = Arguments.createMap();

            boolean method1 = checkRootMethod1(); // SU binary
            boolean method2 = checkRootMethod2(); // Root apps
            boolean method3 = checkRootMethod3(); // Build tags
            boolean method4 = checkRootMethod4(); // System properties

            checks.putBoolean("suBinary", method1);
            checks.putBoolean("rootApps", method2);
            checks.putBoolean("buildTags", method3);
            checks.putBoolean("dangerousProps", checkForDangerousProps());
            checks.putBoolean("rwSystem", checkForRWSystem());
            checks.putBoolean("suBinaryExists", checkForSuBinaryExistence());

            rootInfo.putMap("checks", checks);
            rootInfo.putBoolean("isRooted", method1 || method2 || method3 || method4);

            promise.resolve(rootInfo);
        } catch (Exception e) {
            promise.reject("ROOT_INFO_ERROR", e.getMessage());
        }
    }

    @ReactMethod
    public void hasXposed(Promise promise) {
        try {
            boolean hasXposed = checkForXposed();
            promise.resolve(hasXposed);
        } catch (Exception e) {
            promise.reject("XPOSED_DETECTION_ERROR", e.getMessage());
        }
    }

    // Method 1: Check for SU binary
    private boolean checkRootMethod1() {
        for (String path : ROOT_PATHS) {
            if (new File(path).exists()) {
                return true;
            }
        }
        return false;
    }

    // Method 2: Check for root apps
    private boolean checkRootMethod2() {
        PackageManager packageManager = reactContext.getPackageManager();
        for (String packageName : ROOT_APPS) {
            try {
                packageManager.getPackageInfo(packageName, 0);
                return true;
            } catch (PackageManager.NameNotFoundException e) {
                // Package not found, continue checking
            }
        }
        return false;
    }

    // Method 3: Check build tags
    private boolean checkRootMethod3() {
        String buildTags = android.os.Build.TAGS;
        return buildTags != null && buildTags.contains("test-keys");
    }

    // Method 4: Check system properties and dangerous properties (PASSIVE ONLY)
    private boolean checkRootMethod4() {
        return checkForDangerousProps() || checkForRWSystem() || checkForSuBinaryExistence();
    }

    // Check for dangerous system properties that indicate root
    private boolean checkForDangerousProps() {
        String[] dangerousProps = {
            "ro.debuggable",
            "ro.secure"
        };
        
        try {
            // Check if ro.debuggable is 1 (should be 0 on production)
            String debuggable = getSystemProperty("ro.debuggable");
            if ("1".equals(debuggable)) {
                return true;
            }
            
            // Check if ro.secure is 0 (should be 1 on production)
            String secure = getSystemProperty("ro.secure");
            if ("0".equals(secure)) {
                return true;
            }
        } catch (Exception e) {
            // Property access failed
        }
        
        return false;
    }

    // Passive check - only verify file existence, don't execute
    private boolean checkForSuBinaryExistence() {
        String[] paths = {"/sbin/", "/system/bin/", "/system/xbin/", "/data/local/xbin/",
                         "/data/local/bin/", "/system/sd/xbin/", "/system/bin/failsafe/", 
                         "/data/local/"};
        
        for (String path : paths) {
            File suFile = new File(path + "su");
            if (suFile.exists() && suFile.canRead()) {
                return true;
            }
        }
        return false;
    }

    // Check if system partition is mounted as read-write
    private boolean checkForRWSystem() {
        try {
            Process process = Runtime.getRuntime().exec("mount");
            BufferedReader reader = new BufferedReader(new InputStreamReader(process.getInputStream()));
            String line;
            while ((line = reader.readLine()) != null) {
                if (line.contains("/system") && (line.contains("rw,") || line.contains("rw "))) {
                    reader.close();
                    process.destroy();
                    return true;
                }
            }
            reader.close();
            process.destroy();
        } catch (Exception e) {
            // Mount command failed
        }
        return false;
    }

    // Get system property safely
    private String getSystemProperty(String key) {
        try {
            Process process = Runtime.getRuntime().exec("getprop " + key);
            BufferedReader reader = new BufferedReader(new InputStreamReader(process.getInputStream()));
            String value = reader.readLine();
            reader.close();
            process.destroy();
            return value != null ? value.trim() : "";
        } catch (Exception e) {
            return "";
        }
    }

    // Check for Xposed framework
    private boolean checkForXposed() {
        // Method 1: Check for Xposed installer apps
        PackageManager packageManager = reactContext.getPackageManager();
        for (String packageName : XPOSED_INDICATORS) {
            try {
                packageManager.getPackageInfo(packageName, 0);
                return true;
            } catch (PackageManager.NameNotFoundException e) {
                // Continue checking
            }
        }

        // Method 2: Check for Xposed bridge
        try {
            Class.forName("de.robv.android.xposed.XposedBridge");
            return true;
        } catch (ClassNotFoundException e) {
            // Xposed not found
        }

        // Method 3: Check for Xposed helpers
        try {
            Class.forName("de.robv.android.xposed.XposedHelpers");
            return true;
        } catch (ClassNotFoundException e) {
            // Xposed helpers not found
        }

        return false;
    }
}
