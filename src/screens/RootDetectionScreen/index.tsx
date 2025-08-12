import React from 'react';
import {View, Text, TouchableOpacity} from 'react-native';

import useRootDetection from './useRootDetection';

const RootDetectionScreen = () => {
  const rootInfo = useRootDetection({
    enableAutoCheck: true,
    onRootDetected: status => {
      console.log('Root detected with risk level:', status.riskScore);
      // Handle root detection (e.g., show warning, disable features)
    },
  });

  if (rootInfo.isLoading) {
    return <Text>Checking device security...</Text>;
  }

  if (rootInfo.error) {
    return <Text>Error: {rootInfo.error}</Text>;
  }

  return (
    <View style={{padding: 20}}>
      <Text
        style={{
          fontSize: 18,
          fontWeight: 'bold',
          color: rootInfo.isRooted ? 'red' : 'green',
        }}>
        Device Status: {rootInfo.isRooted ? 'ROOTED' : 'SECURE'}
      </Text>

      <Text>Risk Level: {rootInfo.getRiskLevel()}</Text>
      <Text>Risk Score: {rootInfo.riskScore}/100</Text>

      <View style={{marginTop: 10}}>
        <Text style={{fontWeight: 'bold'}}>Security Checks:</Text>
        <Text>
          • SU Binary:{' '}
          {rootInfo.checks.suBinaryExists ? '⚠️ Found' : '✅ Not Found'}
        </Text>
        <Text>
          • Root Apps:{' '}
          {rootInfo.checks.rootPackagesFound ? '⚠️ Found' : '✅ Not Found'}
        </Text>
        <Text>
          • Test Keys: {rootInfo.checks.testKeys ? '⚠️ Found' : '✅ Not Found'}
        </Text>
        <Text>
          • Dangerous Props:{' '}
          {rootInfo.checks.dangerousProps ? '⚠️ Found' : '✅ Not Found'}
        </Text>
        <Text>
          • RW System:{' '}
          {rootInfo.checks.rwSystemPartition ? '⚠️ Found' : '✅ Not Found'}
        </Text>
        <Text>
          • Root Beer Root Detection:{' '}
          {rootInfo.checks.isDeviceRootedRootBeer ? '⚠️ Found' : '✅ Not Found'}
        </Text>
        <Text>
          • Native C Root Detection:{' '}
          {rootInfo.checks.nativeRooted ? '⚠️ Found' : '✅ Not Found'}
        </Text>
      </View>

      <TouchableOpacity
        style={{
          backgroundColor: '#007AFF',
          padding: 10,
          borderRadius: 5,
          marginTop: 15,
        }}
        onPress={rootInfo.checkNow}>
        <Text style={{color: 'white', textAlign: 'center'}}>Check Again</Text>
      </TouchableOpacity>
    </View>
  );
};

export default RootDetectionScreen;
