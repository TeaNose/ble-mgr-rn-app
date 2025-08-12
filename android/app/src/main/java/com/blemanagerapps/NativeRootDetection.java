package com.blemanagerapps;

import android.util.Log;

public class NativeRootDetection {
    static {
        try {
            System.loadLibrary("native_root_detection");
            System.loadLibrary("root_detector");
        } catch (UnsatisfiedLinkError e) {
            e.printStackTrace();
        }
    }

    public native boolean nativeIsRooted();
    public native boolean checkAllRootPossibility();
}
