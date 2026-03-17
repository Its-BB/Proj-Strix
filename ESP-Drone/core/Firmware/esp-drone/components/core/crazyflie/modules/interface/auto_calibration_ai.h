/**
 * AI-Powered Automatic Calibration System for ESP-Drone
 * 
 * This system automatically calibrates ALL sensors without any manual intervention:
 * - Gyroscope bias and drift compensation
 * - Accelerometer scaling and alignment
 * - Magnetometer hard/soft iron calibration
 * - Motor trim and balance optimization
 * - Flight dynamics parameter tuning
 * 
 * Uses AI algorithms to:
 * - Detect optimal calibration conditions
 * - Learn from flight patterns and sensor data
 * - Adapt to environmental changes
 * - Optimize performance continuously
 */

#ifndef __AUTO_CALIBRATION_AI_H__
#define __AUTO_CALIBRATION_AI_H__

#include "stabilizer_types.h"
#include "param.h"
#include "log.h"

// AI Calibration States
typedef enum {
    AI_CALIB_STARTUP = 0,
    AI_CALIB_LEARNING,
    AI_CALIB_GYRO_AUTO,
    AI_CALIB_ACCEL_AUTO,
    AI_CALIB_MAG_AUTO,
    AI_CALIB_MOTOR_AUTO,
    AI_CALIB_FLIGHT_TUNE,
    AI_CALIB_COMPLETE,
    AI_CALIB_MONITORING
} AICalibState_t;

// AI Learning Data Structure
typedef struct {
    // Sensor calibration data
    Axis3f gyroBias;
    Axis3f gyroScale;
    Axis3f accelBias;
    Axis3f accelScale;
    Axis3f magBias;
    Axis3f magScale;
    
    // Motor calibration
    float motorTrim[4];
    float motorBalance[4];
    
    // Flight dynamics
    float pidGains[9]; // P, I, D for roll, pitch, yaw
    float attitudeTrim[3];
    
    // AI learning parameters
    float confidence;
    float learningRate;
    uint32_t sampleCount;
    uint32_t flightTime;
    
    // Environmental adaptation
    float temperature;
    float pressure;
    float humidity;
    
    // Performance metrics
    float stabilityScore;
    float responseScore;
    float efficiencyScore;
    
    // AI state
    AICalibState_t state;
    bool isLearning;
    bool autoTuneEnabled;
    
} AICalibData_t;

// AI Calibration Functions
void aiCalibInit(void);
void aiCalibUpdate(sensorData_t* sensors, const control_t* control, const setpoint_t* setpoint);
void aiCalibForceRecalibration(void);
bool aiCalibIsReady(void);
float aiCalibGetConfidence(void);
AICalibState_t aiCalibGetState(void);

// Automatic sensor calibration functions
void aiCalibAutoGyro(Axis3f* gyroData, float temperature);
void aiCalibAutoAccel(Axis3f* accelData);
void aiCalibAutoMag(Axis3f* magData);
void aiCalibAutoMotors(float motorOutputs[4], float actualThrust);

// AI learning and adaptation
void aiCalibLearnFromFlight(const sensorData_t* sensors, const control_t* control);
void aiCalibAdaptToEnvironment(float temp, float pressure, float humidity);
void aiCalibOptimizePID(float rollError, float pitchError, float yawError);

// Performance analysis
float aiCalibAnalyzeStability(const sensorData_t* sensors);
float aiCalibAnalyzeResponse(const control_t* control, const setpoint_t* setpoint);
float aiCalibAnalyzeEfficiency(float motorOutputs[4], float actualPerformance);

#endif // __AUTO_CALIBRATION_AI_H__
