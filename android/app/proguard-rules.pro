# Add project specific ProGuard rules here.
# By default, the flags in this file are appended to flags specified
# in /usr/local/Cellar/android-sdk/24.3.3/tools/proguard/proguard-android.txt
# You can edit the include path and order by changing the proguardFiles
# directive in build.gradle.
#
# For more details, see
#   http://developer.android.com/guide/developing/tools/proguard.html

# Add any project specific keep options here:
# Strip out debug preference XMLs
-assumenosideeffects class com.facebook.react.devsupport.DevSettings { *; }
-assumenosideeffects class com.facebook.react.devsupport.DevInternalSettings { *; }
-assumenosideeffects class com.facebook.react.devsupport.DebugServerHost { *; }
