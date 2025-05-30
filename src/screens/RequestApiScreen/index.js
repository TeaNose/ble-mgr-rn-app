import axios from 'axios';
import React from 'react';
import {View, Button, ActivityIndicator, NativeModules} from 'react-native';

const {RootCheckModule, FileCheckModule} = NativeModules;

export const ROOT_DETECTION_PATH = [
  // Magisk and Zygisk-related paths
  '/data/adb/magisk',
  '/data/adb/zygisk',
  '/system/lib/libzygisk.so',
  '/system/lib64/libzygisk.so',
  '/sbin/.magisk/',
  '/dev/.magisk/',
  '/data/adb/magisk/',
  '/cache/magisk.log',
  '/system/app/Superuser.apk',

  // Common superuser binaries and SU paths
  '/sbin/su',
  '/system/bin/su',
  '/system/xbin/su',
  '/data/local/xbin/su',
  '/data/local/bin/su',
  '/system/sd/xbin/su',
  '/system/bin/failsafe/su',
  '/data/local/su',
  '/su/bin/su',

  // Common busybox binaries, often associated with rooted devices
  '/system/xbin/busybox',
  '/system/bin/busybox',
  '/system/sbin/busybox',
  '/vendor/bin/busybox',
  '/data/local/xbin/busybox',
  '/data/local/bin/busybox',

  // Frida-related paths (often used for reverse engineering)
  '/data/local/tmp/frida-server',
  '/data/local/tmp/re.frida.server',
  '/data/local/tmp/frida',
  '/data/local/tmp/frida-inject',
  '/data/local/tmp/frida-agent-32',
  '/data/local/tmp/frida-agent-64',
  '/system/lib/libfrida-gadget.so',
  '/system/lib64/libfrida-gadget.so',

  // Xposed Framework-related paths (another common rooting tool)
  '/system/framework/XposedBridge.jar',
  '/system/lib/libxposed_art.so',
  '/system/lib64/libxposed_art.so',
  '/system/xbin/daemonsu',
  '/system/xbin/supolicy',
  '/data/data/de.robv.android.xposed.installer',
  '/system/bin/daemonsu',
  '/data/app/de.robv.android.xposed.installer',

  // Substrate-related files (jailbreak library often used for injecting code)
  '/usr/libexec/substrate',
  '/Library/MobileSubstrate/MobileSubstrate.dylib',
  '/usr/lib/substitute-inserter.dylib',
  '/usr/lib/libhooker.dylib',

  // Other known files related to jailbreak/root detection
  '/etc/apt',
  '/bin/bash',
  '/usr/sbin/sshd',
  '/private/var/lib/apt/',
  '/Applications/Cydia.app',
  '/Applications/FakeCarrier.app',
  '/Applications/Icy.app',
  '/Applications/IntelliScreen.app',
  '/Applications/SBSettings.app',
  '/Applications/RockApp.app',
];

const RequestApiScreen = () => {
  const [isLoading, setIsLoading] = React.useState(false);

  const API_URL =
    'https://ahm-dev.acedigitalcloudplatforms.com//auth/api/1.0.1/auth/login';
  const params = {
    username: 'ahm_device_admin',
    password: 'admin123',
  };

  const onCheckRoot = async () => {
    console.log('Begin Checking Root...');
    const isContainsZygiskFiles = await checkForZygiskFiles();
    console.log('isContainsZygiskFiles: ', isContainsZygiskFiles);
  };

  const checkForZygiskFiles = async () => {
    try {
      const results = await Promise.all(
        ROOT_DETECTION_PATH.map(path => FileCheckModule.doesFileExist(path)),
      );
      return results.some(result => result === true);
    } catch (error) {
      console.error('Error checking for Zygisk files:', error);
      return false;
    }
  };

  const onAxiosRequest = async () => {
    setIsLoading(true);
    try {
      const apiResponse = await axios.post(API_URL, params);
      console.log('apiResponse: ', JSON.stringify(apiResponse.data));
    } catch (error) {
      console.error(error);
    } finally {
      setIsLoading(false);
    }
  };

  const onSSLPinningRequest = async () => {
    setIsLoading(true);
    try {
      const apiResponse = await axios.post(API_URL, params);
      console.log('apiResponse: ', JSON.stringify(apiResponse.data));
    } catch (error) {
      console.error(error);
    } finally {
      setIsLoading(false);
    }
  };

  return (
    <View style={{flex: 1, backgroundColor: 'white', padding: 16}}>
      <Button title="Request using axios" onPress={onAxiosRequest} />
      <View style={{height: 28}} />
      <Button title="Request using rn-ssl-pinning" />
      <View style={{height: 28}} />
      <Button title="Check Root" onPress={onCheckRoot} />
      <View style={{height: 28}} />
      {isLoading && (
        <View style={{justifyContent: 'center', alignItems: 'center'}}>
          <ActivityIndicator size={'large'} color={'black'} />
        </View>
      )}
    </View>
  );
};

export default RequestApiScreen;
