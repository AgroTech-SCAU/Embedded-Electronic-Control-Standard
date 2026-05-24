#ifndef _imu_attitude_h_
#define _imu_attitude_h_

#include <stdint.h>

#include "imu.h"

// ! ========================= 接 口 变 量 / Typedef 声 明 ========================= ! //

typedef enum {
    IMU_ATTITUDE_STATUS_OK = 0,
    IMU_ATTITUDE_STATUS_ERROR,
    IMU_ATTITUDE_STATUS_INVALID_PARAM,
    IMU_ATTITUDE_STATUS_CALIBRATING,
    IMU_ATTITUDE_STATUS_NOT_READY,
} ImuAttitudeStatus;

typedef struct {
    float w;
    float x;
    float y;
    float z;
} ImuQuat;

typedef struct {
    ImuAttitudeConfig config;
    ImuQuat quat;
    ImuAngle angle;
    ImuGyro gyro_bias;
    ImuGyro gyro_bias_sum;
    ImuGyro gyro_filtered;
    ImuAcc acc_filtered;
    uint32_t last_update_us;
    uint16_t calib_count;
    uint8_t calibrated;
    uint8_t has_angle;
} ImuAttitude;

// ! ========================= 接 口 函 数 声 明 ========================= ! //

ImuAttitudeStatus imu_attitude_init(ImuAttitude* attitude, const ImuAttitudeConfig* config);
ImuAttitudeStatus imu_attitude_update(ImuAttitude* attitude, const ImuSample* sample);
ImuAttitudeStatus imu_attitude_get_angle(const ImuAttitude* attitude, ImuAngle* angle);
ImuAttitudeStatus imu_attitude_get_quat(const ImuAttitude* attitude, ImuQuat* quat);
ImuAttitudeStatus imu_attitude_reset_yaw(ImuAttitude* attitude, float yaw);

#endif
