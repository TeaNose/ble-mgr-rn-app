import React from 'react';
import {Button, Text, View, TouchableOpacity} from 'react-native';
import useBle from '../../hooks/useBle';

import styles from './styles';

const HomeScreen = () => {
  const {scanForDevices, allDevices, connectToDevice} = useBle();

  return (
    <View style={styles.container}>
      <Button title="Scan Devices" onPress={scanForDevices} />
      {allDevices?.map(deviceItem => (
        <TouchableOpacity
          style={{
            marginVertical: 8,
            padding: 16,
            borderBottomWidth: 1,
            flexDirection: 'row',
            justifyContent: 'space-between',
            alignItems: 'center'
          }}
          onPress={() => connectToDevice(deviceItem?.id)}>
          <Text>{`${deviceItem?.name}: ${deviceItem?.id}`}</Text>
          <Text>Connect</Text>
        </TouchableOpacity>
      ))}
    </View>
  );
};

export default HomeScreen;
