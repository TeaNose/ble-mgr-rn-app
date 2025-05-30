package com.blemanagerapps;

import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.bridge.ReactContextBaseJavaModule;
import com.facebook.react.bridge.ReactMethod;
import com.facebook.react.bridge.Promise;

import java.io.File;

public class FileCheckModule extends ReactContextBaseJavaModule {
    public FileCheckModule(ReactApplicationContext reactContext) {
        super(reactContext);
    }

    @Override
    public String getName() {
        return "FileCheckModule";
    }

    @ReactMethod
    public void doesFileExist(String filePath, Promise promise) {
        try {
            File file = new File(filePath);
            promise.resolve(file.exists());
        } catch (Exception e) {
            promise.reject("FILE_CHECK_ERROR", e);
        }
    }
}