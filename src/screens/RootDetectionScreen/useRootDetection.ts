// useRootDetection.js - Safe React Native Hook
import { useState, useEffect } from 'react';
import { NativeModules, Platform } from 'react-native';

const { EnhancedRootDetectionModule } = NativeModules;

const useRootDetection = (options = {}) => {
  const [rootStatus, setRootStatus] = useState({
    isRooted: false,
    isLoading: true,
    riskScore: 0,
    checks: {},
    error: null
  });

  const { 
    enableAutoCheck = true,
    checkInterval = null, // Set to number of ms for periodic checks
    onRootDetected = null // Callback when root is detected
  } = options;

  const performRootCheck = async () => {
    if (Platform.OS !== 'android') {
      setRootStatus(prev => ({
        ...prev,
        isLoading: false,
        error: 'Root detection only available on Android'
      }));
      return;
    }

    if (!EnhancedRootDetectionModule) {
      setRootStatus(prev => ({
        ...prev,
        isLoading: false,
        error: 'Root detection module not available'
      }));
      return;
    }

    try {
      setRootStatus(prev => ({ ...prev, isLoading: true, error: null }));
      
      // Get detailed root information using PASSIVE methods only
      const result = await EnhancedRootDetectionModule.getDetailedRootInfo();
      
      const newStatus = {
        isRooted: result.isRooted,
        riskScore: result.riskScore,
        checks: result.checks,
        isLoading: false,
        error: null
      };

      setRootStatus(newStatus);

      // Call callback if root detected
      if (result.isRooted && onRootDetected) {
        onRootDetected(newStatus);
      }

      return newStatus;
    } catch (error) {
      console.error('Root detection error:', error);
      setRootStatus(prev => ({
        ...prev,
        isLoading: false,
        error: error.message
      }));
      return null;
    }
  };

  useEffect(() => {
    if (enableAutoCheck) {
      performRootCheck();
    }

    // Set up periodic checks if requested
    let intervalId;
    if (checkInterval && checkInterval > 0) {
      intervalId = setInterval(performRootCheck, checkInterval);
    }

    return () => {
      if (intervalId) {
        clearInterval(intervalId);
      }
    };
  }, [enableAutoCheck, checkInterval]);

  return {
    ...rootStatus,
    checkNow: performRootCheck,
    getRiskLevel: () => {
      if (rootStatus.riskScore >= 70) return 'HIGH';
      if (rootStatus.riskScore >= 40) return 'MEDIUM';
      if (rootStatus.riskScore >= 20) return 'LOW';
      return 'MINIMAL';
    }
  };
};

export default useRootDetection;
