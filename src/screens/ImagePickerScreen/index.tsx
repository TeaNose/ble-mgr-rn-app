import React from 'react';
import {View, Button, Image} from 'react-native';

import RNFS from 'react-native-fs';

const sharedCacheDir = `${RNFS.CachesDirectoryPath}/shared`;

const ImagePickerScreen = () => {
  const [imageUrl, setImageUrl] = React.useState('');
  // const ensureSharedCacheDirExists = async () => {
  //   const exists = await RNFS.exists(sharedCacheDir);
  //   if (!exists) {
  //     await RNFS.mkdir(sharedCacheDir);
  //   }
  // };

  // const clearSharedCacheDir = async () => {
  //   const exists = await RNFS.exists(sharedCacheDir);
  //   if (exists) {
  //     const files = await RNFS.readDir(sharedCacheDir);
  //     for (const file of files) {
  //       if (file.isFile()) {
  //         await RNFS.unlink(file.path);
  //       }
  //     }
  //   }
  // };

  // const moveCapturedMediaToShared = async asset => {
  //   if (!asset?.uri || !asset?.fileName) {
  //     throw new Error('Invalid asset data');
  //   }
  //   console.log('asset: ', JSON.stringify(asset));

  //   const sourcePath = asset.uri.replace('file://', '');
  //   const destPath = `${sharedCacheDir}/${Date.now()}_${asset.fileName}`;

  //   await RNFS.copyFile(sourcePath, destPath);
  //   console.log('Moved file to shared:', destPath);

  //   return destPath; // Return the new safe path
  // };

  // async function captureMedia(type = 'photo') {
  //   await ensureSharedCacheDirExists(); // Make sure shared/ exists

  //   const options = {mediaType: type}; // 'photo' or 'video'

  //   const result = await launchCamera(options);

  //   if (result.didCancel) {
  //     console.log('User cancelled capture');
  //     return;
  //   } else if (result.errorCode) {
  //     console.error('Camera Error: ', result.errorMessage);
  //     return;
  //   }

  //   if (result.assets && result.assets.length > 0) {
  //     const asset = result.assets[0];

  //     // Move the captured media to shared/ directory
  //     const safePath = asset.uri;
  //     setImageUrl(safePath);

  //     // You can now share or upload the file safely from safePath
  //     return safePath;
  //   }
  // }

  console.log('imageUrl: ', imageUrl);
  return (
    <View style={{flex: 1}}>
    </View>
  );
};

export default ImagePickerScreen;
