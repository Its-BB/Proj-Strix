/**
 * Basic Gyroscope Calibration System for ESP-Drone
 * 
 * Provides simple gyroscope bias calibration
 * to eliminate the gyro drift visible in cfclient even when on steady ground.
 */

#ifndef __GYRO_CALIBRATION_H__
#define __GYRO_CALIBRATION_H__

#include "stabilizer_types.h"
#include "param.h"
#include "log.h"

// Basic calibration parameters
#define GYRO_ENHANCED_BIAS_SAMPLES    2000    // Sample count for calibration
#define GYRO_DRIFT_COMPENSATION_RATE  0.001f  // Rate of ongoing drift compensation
#define GYRO_TEMP_COMPENSATION        0       // Disable temperature compensation
#define GYRO_STABILITY_THRESHOLD      100     // Threshold for detecting stable state
#define GYRO_RECALIB_INTERVAL         30000   // Auto-recalibration interval (ms)

// Calibration states
typedef enum {
    GYRO_CALIB_IDLE = 0,
    GYRO_CALIB_COLLECTING,
    GYRO_CALIB_PROCESSING,
    GYRO_CALIB_COMPLETE,
    GYRO_CALIB_FAILED
} GyroCalibState_t;

// Enhanced calibration structure
typedef struct {
    // Basic bias calibration
    Axis3f bias;
    Axis3f variance;
    Axis3f mean;
    bool isBiasFound;
    
    // Enhanced features
    Axis3f driftCompensation;    // Ongoing drift compensation
    float temperature;           // Current temperature
    float tempCoeff[3];         // Temperature coefficients for each axis
    
    // Calibration state
    GyroCalibState_t state;
    uint32_t sampleCount;
    uint32_t lastCalibTime;
    
    // Manual calibration control
    bool manualCalibTrigger;
    bool autoCalibEnabled;
    bool tempCompEnabled;
    
    // Statistics
    float calibQuality;         // Quality metric (0-100)
    uint32_t calibCount;        // Number of successful calibrations
    
} GyroCalibData_t;

// Function declarations
void gyroCalibInit(void);
void gyroCalibUpdate(Axis3f* gyroRaw, float temperature);
bool gyroCalibIsReady(void);
void gyroCalibTriggerManual(void);
void gyroCalibReset(void);
Axis3f gyroCalibGetBias(void);
float gyroCalibGetQuality(void);
GyroCalibState_t gyroCalibGetState(void);

// Parameter and logging support
void gyroCalibRegisterParams(void);
void gyroCalibRegisterLogs(void);

#endif // __GYRO_CALIBRATION_H__
