import React from 'react';
import {
  Alert,
  PermissionsAndroid,
  ToastAndroid,
  NativeModules,
  NativeEventEmitter,
  EmitterSubscription,
} from 'react-native';
import DeviceInfo from 'react-native-device-info';
import {PERMISSIONS} from 'react-native-permissions';
import BleManager, {BleState, Peripheral} from 'react-native-ble-manager';

const BleManagerModule = NativeModules.BleManager;
const bleManagerEmitter = new NativeEventEmitter(BleManagerModule);

const SECONDS_TO_SCAN = 6;
const SERVICE_UUIDS = [];
const ALLOW_DUPLICATE = false;

const useBle = () => {
  const MAX_CONNECT_WAITING_PERIOD = 30000;
  const serviceReadinIdentifier = '';
  const charNotificationIdentifier = '';
  const connectedDeviceId = React.useRef('');

  const requestPermissions = async (callback: PermissionCallback) => {
    const apiLevel = await DeviceInfo.getApiLevel();
    console.log('apiLevel: ', apiLevel);
    if (apiLevel < 31) {
      const grantedStatus = await PermissionsAndroid.request(
        PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION,
        {
          title: 'Location Permission',
          message: 'Bluetooth Low Energy Needs Location Permission',
          buttonNegative: 'Cancel',
          buttonPositive: 'Ok',
          buttonNeutral: 'Maybe Later',
        },
      );
      callback(grantedStatus === PermissionsAndroid.RESULTS.GRANTED);
    } else {
      const result = await PermissionsAndroid.requestMultiple([
        PERMISSIONS.ANDROID.BLUETOOTH_SCAN,
        PERMISSIONS.ANDROID.BLUETOOTH_CONNECT,
        PERMISSIONS.ANDROID.ACCESS_FINE_LOCATION,
        PERMISSIONS.ANDROID.BLUETOOTH_ADVERTISE,
      ]);

      const isAllPermissionsGranted =
        result['android.permission.BLUETOOTH_SCAN'] ===
          PermissionsAndroid.RESULTS.GRANTED &&
        result['android.permission.BLUETOOTH_CONNECT'] ===
          PermissionsAndroid.RESULTS.GRANTED &&
        result['android.permission.ACCESS_FINE_LOCATION'] ===
          PermissionsAndroid.RESULTS.GRANTED &&
        result['android.permission.BLUETOOTH_ADVERTISE'] ===
          PermissionsAndroid.RESULTS.GRANTED;

      callback(isAllPermissionsGranted);
    }
  };

  const isDeviceConnected = async (deviceId: string) => {
    return await BleManager.isPeripheralConnected(deviceId, []);
  };

  const connect = (deviceId: string): Promise<boolean> => {
    return new Promise<boolean>(async (resolve, reject) => {
      let failedToConnectTimer;

      let isConnected = await isDeviceConnected(deviceId);

      if (!isConnected) {
        //if not connected already, set the timer such that after some time connection process automatically stops if its failed to connect.
        failedToConnectTimer = setTimeout(() => {
          return resolve(false);
        }, MAX_CONNECT_WAITING_PERIOD);

        await BleManager.connect(deviceId).then(() => {
          //if connected successfully, stop the previous set timer.
          clearTimeout(failedToConnectTimer);
        });
        isConnected = await isDeviceConnected(deviceId);

        if (!isConnected) {
          //now if its not connected somehow, just close the process.
          return resolve(false);
        } else {
          //Connection success
          connectedDeviceId.current = deviceId;
        }

        //get the services and characteristics information for the connected hardware device.
        const peripheralInformation = await BleManager.retrieveServices(
          deviceId,
        );

        /**
         * Check for supported services and characteristics from device info
         */
        const deviceSupportedServices = (
          peripheralInformation.services || []
        ).map(item => item?.uuid?.toUpperCase());
        const deviceSupportedCharacteristics = (
          peripheralInformation.characteristics || []
        ).map(_char => _char.characteristic.toUpperCase());
        if (
          !deviceSupportedServices.includes(serviceReadinIdentifier) ||
          !deviceSupportedCharacteristics.includes(charNotificationIdentifier)
        ) {
          //if required service ID and Char ID is not supported by hardware, close the connection.
          isConnected = false;
          await BleManager.disconnect(connectedDeviceId.current);
          return reject(
            'Connected device does not have required service and characteristic.',
          );
        }

        await BleManager.startNotification(
          deviceId,
          serviceReadinIdentifier,
          charNotificationIdentifier,
        )
          .then(response => {
            console.log(
              'Started notification successfully on ',
              charNotificationIdentifier,
            );
          })
          .catch(async () => {
            isConnected = false;
            await BleManager.disconnect(connectedDeviceId.current);
            return reject(
              'Failed to start notification on required service and characteristic.',
            );
          });

        let disconnectListener = bleManagerEmitter.addListener(
          'BleManagerDisconnectPeripheral',
          async () => {
            //addd the code to execute after hardware disconnects.
            if (connectedDeviceId.current) {
              await BleManager.disconnect(connectedDeviceId.current);
            }
            disconnectListener.remove();
          },
        );

        return resolve(isConnected);
      }
    });
  };

  const scanNearbyDevices = (): Promise<Map<string, Peripheral>> => {
    return new Promise((resolve, reject) => {
      const devicesMap = new Map<string, Peripheral>();

      let listeners: EmitterSubscription[] = [];

      const onBleManagerDiscoverPeripheral = (peripheral: Peripheral) => {
        if (peripheral.id && peripheral.name) {
          devicesMap.set(peripheral.id, peripheral);
        }
      };

      const onBleManagerStopScan = () => {
        for (const listener of listeners) {
          listener.remove();
        }
        resolve(devicesMap);
      };

      try {
        listeners = [
          bleManagerEmitter.addListener(
            'BleManagerDiscoverPeripheral',
            onBleManagerDiscoverPeripheral,
          ),
          bleManagerEmitter.addListener(
            'BleManagerStopScan',
            onBleManagerStopScan,
          ),
        ];

        BleManager.scan(SERVICE_UUIDS, SECONDS_TO_SCAN, ALLOW_DUPLICATE);
      } catch (error) {
        reject(
          new Error(error instanceof Error ? error.message : (error as string)),
        );
      }
    });
  };

  const scanForDevices = async () => {
    try {
      await BleManager.enableBluetooth().then(async () => {
        console.info('Bluetooth is enabled');
        const nearbyDevices = await scanNearbyDevices();
        console.log('nearbyDevices: ', nearbyDevices);
      });
      //go ahead to scan nearby devices
    } catch (e) {
      //prompt user to enable bluetooth manually and also give them the option to navigate to bluetooth settings directly.
      return;
    }
  };

  const connectToDevice = async (deviceId: any) => {
    const isConnected = await connect(deviceId);
    if (isConnected) {
      console.log('Device successfully connected');
    } else {
      console.error('Failed to connect to the device');
    }
  };

  React.useEffect(() => {
    requestPermissions((isGranted: boolean) => {
      if (isGranted) {
        console.log('isGranted: ', isGranted);
        BleManager.start({showAlert: false, forceLegacy: true}).then(() => {});
      }
    });
  }, []);

  return {
    requestPermissions,
    scanForDevices,
    connectToDevice,
  };
};

export default useBle;
