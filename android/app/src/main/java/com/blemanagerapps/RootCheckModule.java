// RootCheckModule.java
package com.blemanagerapps;

import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.bridge.ReactContextBaseJavaModule;
import com.facebook.react.bridge.ReactMethod;
import com.facebook.react.bridge.Promise;
import com.scottyab.rootbeer.RootBeer;

public class RootCheckModule extends ReactContextBaseJavaModule {

   public RootCheckModule(ReactApplicationContext reactContext) {
       super(reactContext);
   }

   @Override
   public String getName() {
       return "RootCheckModule";
   }

   @ReactMethod
   public void isDeviceRooted(Promise promise) {
       try {
           RootBeer rootBeer = new RootBeer(getReactApplicationContext());
           boolean isRooted = rootBeer.isRooted();
           promise.resolve(isRooted);
       } catch (Exception e) {
           promise.reject("ROOT_CHECK_ERROR", e.getMessage());
       }
   }
}