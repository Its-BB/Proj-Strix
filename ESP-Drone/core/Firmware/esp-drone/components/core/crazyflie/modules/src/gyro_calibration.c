/**
 * Basic Gyroscope Calibration System Implementation
 * 
 * This module provides basic gyroscope calibration with features:
 * - Simple bias calculation
 * - Manual recalibration triggers
 * - Quality metrics and monitoring
 */

#include "gyro_calibration.h"
#include "FreeRTOS.h"
#include "task.h"
#include "math.h"
#include "string.h"
#include "ledseq.h"
#include "sound.h"

#define DEBUG_MODULE "GYRO_CALIB"
#include "debug_cf.h"

// Global calibration data
static GyroCalibData_t calibData;
static Axis3f sampleBuffer[GYRO_ENHANCED_BIAS_SAMPLES];
static uint32_t bufferIndex = 0;
static bool calibrationActive = false;

// Temperature tracking
static float tempHistory[10];
static uint8_t tempHistoryIndex = 0;

// Statistics tracking
static float lastVariance[3];
// static uint32_t stabilityCounter = 0; // Unused variable

/**
 * Initialize the enhanced gyroscope calibration system
 */
void gyroCalibInit(void)
{
    // Initialize calibration data
    memset(&calibData, 0, sizeof(GyroCalibData_t));
    
    // Set default values
    calibData.autoCalibEnabled = true;
    calibData.tempCompEnabled = true;
    calibData.state = GYRO_CALIB_IDLE;
    calibData.lastCalibTime = xTaskGetTickCount();
    
    // Initialize temperature coefficients to neutral
    calibData.tempCoeff[0] = 0.0f;
    calibData.tempCoeff[1] = 0.0f;
    calibData.tempCoeff[2] = 0.0f;
    
    DEBUG_PRINT("Enhanced gyro calibration initialized\n");
}

/**
 * Calculate stability of gyroscope readings
 */
static float calculateStability(Axis3f* samples, uint32_t count)
{
    if (count < 10) return 0.0f;
    
    float sum_x = 0, sum_y = 0, sum_z = 0;
    float var_x = 0, var_y = 0, var_z = 0;
    
    // Calculate means
    for (uint32_t i = 0; i < count; i++) {
        sum_x += samples[i].x;
        sum_y += samples[i].y;
        sum_z += samples[i].z;
    }
    
    float mean_x = sum_x / count;
    float mean_y = sum_y / count;
    float mean_z = sum_z / count;
    
    // Calculate variances
    for (uint32_t i = 0; i < count; i++) {
        var_x += (samples[i].x - mean_x) * (samples[i].x - mean_x);
        var_y += (samples[i].y - mean_y) * (samples[i].y - mean_y);
        var_z += (samples[i].z - mean_z) * (samples[i].z - mean_z);
    }
    
    var_x /= count;
    var_y /= count;
    var_z /= count;
    
    // Store variances for monitoring
    lastVariance[0] = var_x;
    lastVariance[1] = var_y;
    lastVariance[2] = var_z;
    
    // Calculate stability score (inverse of total variance)
    float totalVariance = var_x + var_y + var_z;
    float stability = 1000.0f / (1.0f + totalVariance);
    
    return fminf(stability, 100.0f);
}

/**
 * Process collected samples and calculate enhanced bias
 */
static void processCalibrationSamples(void)
{
    if (calibData.sampleCount < 100) {
        calibData.state = GYRO_CALIB_FAILED;
        return;
    }
    
    float sum_x = 0, sum_y = 0, sum_z = 0;
    
    // Calculate mean (bias)
    for (uint32_t i = 0; i < calibData.sampleCount; i++) {
        sum_x += sampleBuffer[i].x;
        sum_y += sampleBuffer[i].y;
        sum_z += sampleBuffer[i].z;
    }
    
    calibData.bias.x = sum_x / calibData.sampleCount;
    calibData.bias.y = sum_y / calibData.sampleCount;
    calibData.bias.z = sum_z / calibData.sampleCount;
    
    // Calculate variance for quality assessment
    float var_x = 0, var_y = 0, var_z = 0;
    for (uint32_t i = 0; i < calibData.sampleCount; i++) {
        var_x += (sampleBuffer[i].x - calibData.bias.x) * (sampleBuffer[i].x - calibData.bias.x);
        var_y += (sampleBuffer[i].y - calibData.bias.y) * (sampleBuffer[i].y - calibData.bias.y);
        var_z += (sampleBuffer[i].z - calibData.bias.z) * (sampleBuffer[i].z - calibData.bias.z);
    }
    
    calibData.variance.x = var_x / calibData.sampleCount;
    calibData.variance.y = var_y / calibData.sampleCount;
    calibData.variance.z = var_z / calibData.sampleCount;
    
    // Calculate quality metric
    float totalVariance = calibData.variance.x + calibData.variance.y + calibData.variance.z;
    calibData.calibQuality = fmaxf(0.0f, 100.0f - (totalVariance / 100.0f));
    
    // Check if calibration is acceptable
    if (calibData.calibQuality > 70.0f) {
        calibData.isBiasFound = true;
        calibData.state = GYRO_CALIB_COMPLETE;
        calibData.calibCount++;
        calibData.lastCalibTime = xTaskGetTickCount();
        
        DEBUG_PRINT("Gyro calibration complete - Quality: %.1f%%\n", calibData.calibQuality);
        DEBUG_PRINT("Bias: X=%.3f Y=%.3f Z=%.3f\n", 
                   calibData.bias.x, calibData.bias.y, calibData.bias.z);
        
        // Signal successful calibration
        ledseqRun(&seq_calibrated);
        soundSetEffect(SND_CALIB);
    } else {
        calibData.state = GYRO_CALIB_FAILED;
        DEBUG_PRINT("Gyro calibration failed - Quality too low: %.1f%%\n", calibData.calibQuality);
    }
}

/**
 * Update temperature compensation coefficients
 */
static void updateTemperatureCompensation(float temperature)
{
    if (!calibData.tempCompEnabled) return;
    
    // Store temperature history
    tempHistory[tempHistoryIndex] = temperature;
    tempHistoryIndex = (tempHistoryIndex + 1) % 10;
    
    // Simple temperature drift compensation
    // (In a real implementation, you'd collect data over temperature ranges)
    static float lastTemp = 25.0f;
    float tempDelta = temperature - lastTemp;
    
    if (fabs(tempDelta) > 1.0f && calibData.isBiasFound) {
        // Apply small compensation based on temperature change
        calibData.driftCompensation.x += tempDelta * calibData.tempCoeff[0];
        calibData.driftCompensation.y += tempDelta * calibData.tempCoeff[1];
        calibData.driftCompensation.z += tempDelta * calibData.tempCoeff[2];
        
        lastTemp = temperature;
    }
    
    calibData.temperature = temperature;
}

/**
 * Main calibration update function
 */
void gyroCalibUpdate(Axis3f* gyroRaw, float temperature)
{
    uint32_t currentTime = xTaskGetTickCount();
    
    // Update temperature compensation
    updateTemperatureCompensation(temperature);
    
    // Check for manual calibration trigger
    if (calibData.manualCalibTrigger) {
        calibData.manualCalibTrigger = false;
        calibData.state = GYRO_CALIB_COLLECTING;
        calibData.sampleCount = 0;
        bufferIndex = 0;
        calibrationActive = true;
        
        DEBUG_PRINT("Manual gyro calibration started\n");
        ledseqRun(&seq_alive);
    }
    
    // Check for automatic recalibration
    if (calibData.autoCalibEnabled && 
        (currentTime - calibData.lastCalibTime) > GYRO_RECALIB_INTERVAL &&
        calibData.state == GYRO_CALIB_COMPLETE) {
        
        // Check if drone has been stable for a while
        float stability = calculateStability(sampleBuffer, 
                                           fminf(bufferIndex, GYRO_ENHANCED_BIAS_SAMPLES));
        
        if (stability > 80.0f) {
            calibData.state = GYRO_CALIB_COLLECTING;
            calibData.sampleCount = 0;
            bufferIndex = 0;
            calibrationActive = true;
            
            DEBUG_PRINT("Auto gyro recalibration started (stability: %.1f)\n", stability);
        }
    }
    
    // Collect calibration samples
    if (calibData.state == GYRO_CALIB_COLLECTING) {
        // Check if readings are stable enough for calibration
        if (bufferIndex >= 50) {
            float stability = calculateStability(sampleBuffer, bufferIndex);
            if (stability < 50.0f) {
                // Too much movement, reset collection
                calibData.sampleCount = 0;
                bufferIndex = 0;
                DEBUG_PRINT("Gyro calibration reset - too much movement (stability: %.1f)\n", stability);
                return;
            }
        }
        
        // Add sample to buffer
        if (calibData.sampleCount < GYRO_ENHANCED_BIAS_SAMPLES) {
            sampleBuffer[bufferIndex] = *gyroRaw;
            bufferIndex = (bufferIndex + 1) % GYRO_ENHANCED_BIAS_SAMPLES;
            calibData.sampleCount++;
        }
        
        // Check if we have enough samples
        if (calibData.sampleCount >= GYRO_ENHANCED_BIAS_SAMPLES) {
            calibData.state = GYRO_CALIB_PROCESSING;
            calibrationActive = false;
            
            DEBUG_PRINT("Processing gyro calibration with %d samples\n", calibData.sampleCount);
            processCalibrationSamples();
        }
    }
    
    // Apply ongoing drift compensation
    if (calibData.isBiasFound && calibData.state == GYRO_CALIB_COMPLETE) {
        // Very slow drift compensation to prevent overcorrection
        static uint32_t driftUpdateCounter = 0;
        driftUpdateCounter++;
        
        if (driftUpdateCounter >= 1000) {  // Update every 1000 samples (~1 second at 1kHz)
            driftUpdateCounter = 0;
            
            // Check current stability
            float stability = calculateStability(sampleBuffer, 
                                               fminf(bufferIndex, GYRO_ENHANCED_BIAS_SAMPLES));
            
            if (stability > 90.0f) {
                // Very stable - apply small drift correction
                float correctionRate = GYRO_DRIFT_COMPENSATION_RATE;
                calibData.driftCompensation.x += correctionRate * (gyroRaw->x - calibData.bias.x);
                calibData.driftCompensation.y += correctionRate * (gyroRaw->y - calibData.bias.y);
                calibData.driftCompensation.z += correctionRate * (gyroRaw->z - calibData.bias.z);
                
                // Limit drift compensation to prevent runaway
                calibData.driftCompensation.x = fmaxf(-100.0f, fminf(100.0f, calibData.driftCompensation.x));
                calibData.driftCompensation.y = fmaxf(-100.0f, fminf(100.0f, calibData.driftCompensation.y));
                calibData.driftCompensation.z = fmaxf(-100.0f, fminf(100.0f, calibData.driftCompensation.z));
            }
        }
    }
}

/**
 * Check if calibration is ready
 */
bool gyroCalibIsReady(void)
{
    return calibData.isBiasFound && (calibData.state == GYRO_CALIB_COMPLETE);
}

/**
 * Trigger manual calibration
 */
void gyroCalibTriggerManual(void)
{
    calibData.manualCalibTrigger = true;
    DEBUG_PRINT("Manual gyro calibration triggered\n");
}

/**
 * Reset calibration
 */
void gyroCalibReset(void)
{
    calibData.isBiasFound = false;
    calibData.state = GYRO_CALIB_IDLE;
    calibData.sampleCount = 0;
    calibData.calibQuality = 0.0f;
    
    // Reset drift compensation
    calibData.driftCompensation.x = 0.0f;
    calibData.driftCompensation.y = 0.0f;
    calibData.driftCompensation.z = 0.0f;
    
    bufferIndex = 0;
    calibrationActive = false;
    
    DEBUG_PRINT("Gyro calibration reset\n");
}

/**
 * Get current bias with drift compensation
 */
Axis3f gyroCalibGetBias(void)
{
    Axis3f compensatedBias;
    
    compensatedBias.x = calibData.bias.x + calibData.driftCompensation.x;
    compensatedBias.y = calibData.bias.y + calibData.driftCompensation.y;
    compensatedBias.z = calibData.bias.z + calibData.driftCompensation.z;
    
    return compensatedBias;
}

/**
 * Get calibration quality
 */
float gyroCalibGetQuality(void)
{
    return calibData.calibQuality;
}

/**
 * Get current calibration state
 */
GyroCalibState_t gyroCalibGetState(void)
{
    return calibData.state;
}

// Parameter group for cfclient control
PARAM_GROUP_START(gyroCalib)
PARAM_ADD(PARAM_UINT8, manualTrigger, &calibData.manualCalibTrigger)
PARAM_ADD(PARAM_UINT8, autoEnable, &calibData.autoCalibEnabled)
PARAM_ADD(PARAM_UINT8, tempCompEnable, &calibData.tempCompEnabled)
PARAM_ADD(PARAM_UINT8, reset, &calibData.manualCalibTrigger)  // Trigger reset via parameter
PARAM_ADD(PARAM_FLOAT, tempCoeffX, &calibData.tempCoeff[0])
PARAM_ADD(PARAM_FLOAT, tempCoeffY, &calibData.tempCoeff[1])
PARAM_ADD(PARAM_FLOAT, tempCoeffZ, &calibData.tempCoeff[2])
PARAM_GROUP_STOP(gyroCalib)

// Log group for cfclient monitoring
LOG_GROUP_START(gyroCalib)
LOG_ADD(LOG_UINT8, state, &calibData.state)
LOG_ADD(LOG_FLOAT, quality, &calibData.calibQuality)
LOG_ADD(LOG_UINT32, sampleCount, &calibData.sampleCount)
LOG_ADD(LOG_UINT32, calibCount, &calibData.calibCount)
LOG_ADD(LOG_FLOAT, biasX, &calibData.bias.x)
LOG_ADD(LOG_FLOAT, biasY, &calibData.bias.y)
LOG_ADD(LOG_FLOAT, biasZ, &calibData.bias.z)
LOG_ADD(LOG_FLOAT, driftX, &calibData.driftCompensation.x)
LOG_ADD(LOG_FLOAT, driftY, &calibData.driftCompensation.y)
LOG_ADD(LOG_FLOAT, driftZ, &calibData.driftCompensation.z)
LOG_ADD(LOG_FLOAT, varX, &calibData.variance.x)
LOG_ADD(LOG_FLOAT, varY, &calibData.variance.y)
LOG_ADD(LOG_FLOAT, varZ, &calibData.variance.z)
LOG_ADD(LOG_FLOAT, temperature, &calibData.temperature)
LOG_GROUP_STOP(gyroCalib)
