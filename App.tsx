/**
 * Sample React Native App
 * https://github.com/facebook/react-native
 *
 * @format
 */

import React from 'react';

import HomeScreen from './src/screens/HomeScreen';
import ImagePickerScreen from './src/screens/ImagePickerScreen';
import RequestApiScreen from './src/screens/RequestApiScreen';
import RootDetectionScreen from './src/screens/RootDetectionScreen';
import {useFreeRasp} from 'freerasp-react-native';

// app configuration
const config = {
  androidConfig: {
    packageName: 'com.blemanagerapps',
    certificateHashes: ['+sYXRdwJA3hvue3mKpYrOZ9zSPC7b4mbgzJmdZEDO5w='], // Replace with your release (!) signing certificate hash(es)
    supportedAlternativeStores: ['com.sec.android.app.samsungapps'],
  },
  iosConfig: {
    appBundleId: 'com.blemanagerapps',
    appTeamId: 'your_team_ID',
  },
  watcherMail: 'augustineflame@gmail.com',
  isProd: true,
};

function App(): JSX.Element {
  const actions = {
    // Android & iOS
    privilegedAccess: () => {
      console.log('privilegedAccess');
    },
    // Android & iOS
    debug: () => {
      console.log('debug');
    },
    // Android & iOS
    simulator: () => {
      console.log('simulator');
    },
    // Android & iOS
    appIntegrity: () => {
      console.log('appIntegrity');
    },
    // Android & iOS
    unofficialStore: () => {
      console.log('unofficialStore');
    },
    // Android & iOS
    hooks: () => {
      console.log('hooks');
    },
    // Android & iOS
    deviceBinding: () => {
      console.log('deviceBinding');
    },
    // Android & iOS
    secureHardwareNotAvailable: () => {
      console.log('secureHardwareNotAvailable');
    },
    // Android & iOS
    systemVPN: () => {
      console.log('systemVPN');
    },
    // Android & iOS
    passcode: () => {
      console.log('passcode');
    },
    // iOS only
    deviceID: () => {
      console.log('deviceID');
    },
    // Android only
    obfuscationIssues: () => {
      console.log('obfuscationIssues');
    },
    // Android only
    devMode: () => {
      console.log('devMode');
    },
    // Android only
    adbEnabled: () => {
      console.log('adbEnabled');
    },
    // Android & iOS
    screenshot: () => {
      console.log('screenshot');
    },
    // Android & iOS
    screenRecording: () => {
      console.log('screenRecording');
    },
    // Android only
    multiInstance: () => {
      console.log('multiInstance');
    },
  };

  useFreeRasp(config, actions);

  return <RootDetectionScreen />;
}

export default App;
