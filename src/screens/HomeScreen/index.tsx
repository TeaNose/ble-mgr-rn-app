import React from 'react';
import {Button, Text, View} from 'react-native';
import useBle from '../../hooks/useBle';

import styles from './styles';

const HomeScreen = () => {
  const {scanForDevices} = useBle();

  return (
    <View style={styles.container}>
      <Button title="Scan Devices" onPress={scanForDevices} />
    </View>
  );
};

export default HomeScreen;
