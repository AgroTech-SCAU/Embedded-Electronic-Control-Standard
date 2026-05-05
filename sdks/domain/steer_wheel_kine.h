#ifndef _steer_wheel_kine_h_
#define _steer_wheel_kine_h_

// ! ========================= 接 口 变 量 / Typedef 声 明 ========================= ! //

#define swheel steer_wheel_interface

#define STEER_WHEEL_STATUS_TABLE \
    X(OK, "OK") \
    X(INVALID_PARAM, "Invalid Parameter") \
    X(INVALID_MODEL, "Invalid Model") 

#define X(name, str) STEER_WHEEL_##name,
typedef enum {
    STEER_WHEEL_STATUS_TABLE
} SteelWheelErrorCode;
#undef X

typedef struct {
    float wheel_omega;
    float steer_angle;
} WheelModule;

typedef struct {
    float length;
    float width;
    float wheel_radius;
    float max_wheel_linear_speed;
} SteerWheelModel;

typedef struct {
    WheelModule wheels[4];
    float vx;
    float vy;
    float wz;
} SteerWheelControl;

typedef struct {
    WheelModule cur_wheels[4];
    float cur_vx;
    float cur_vy;
    float cur_wz;
} SteerWheelState;

typedef struct {
    SteerWheelModel model;
    SteerWheelControl control;
    SteerWheelState state;
} SteerWheel;

#define X(name, str) SteelWheelErrorCode name;
extern const struct SteerWheelInterface {
    struct {
        STEER_WHEEL_STATUS_TABLE
    };
    SteelWheelErrorCode(*init)(SteerWheel* steer_wheel, SteerWheelModel model);
    SteelWheelErrorCode(*fk)(SteerWheel* steer_wheel);
    SteelWheelErrorCode(*ik)(SteerWheel* steer_wheel);
    const char* (*error_code_to_str)(SteelWheelErrorCode status);
} steer_wheel_interface;
#undef X

// ! ========================= 接 口 函 数 声 明 ========================= ! //

SteelWheelErrorCode steer_wheel_init(SteerWheel* steer_wheel, SteerWheelModel model);
SteelWheelErrorCode steer_wheel_fk(SteerWheel* steer_wheel);
SteelWheelErrorCode steer_wheel_ik(SteerWheel* steer_wheel);
const char* steer_wheel_error_code_to_str(SteelWheelErrorCode status);

#endif
