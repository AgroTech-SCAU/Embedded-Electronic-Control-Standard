# Include this manifest from the restored board project
BUS_MOTOR_STANDARD_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../..)
BUS_MOTOR_DM_SOURCES := \
 $(BUS_MOTOR_STANDARD_ROOT)/sdks/device/bus_motor/bus_motor.c \
 $(BUS_MOTOR_STANDARD_ROOT)/sdks/device/bus_motor/dm_motor.c \
 $(BUS_MOTOR_STANDARD_ROOT)/sdks/device/bus_motor/dm_motor/dm_motor_core.c \
 $(BUS_MOTOR_STANDARD_ROOT)/sdks/device/bus_motor/dm_motor/dm_motor_protocol_v3.c \
 $(BUS_MOTOR_STANDARD_ROOT)/sdks/device/bus_motor/dm_motor/dm_motor_protocol_v4.c
BUS_MOTOR_DM_INCLUDES := -I$(BUS_MOTOR_STANDARD_ROOT)/sdks/device

# All shared dependencies are compiled from the formal SDK
BUS_MOTOR_DM_SOURCES += \
 $(BUS_MOTOR_STANDARD_ROOT)/sdks/device/rgb_led/rgb_led.c \
 $(BUS_MOTOR_STANDARD_ROOT)/sdks/device/rgb_led/ws2812_rgb_led.c \
 $(BUS_MOTOR_STANDARD_ROOT)/sdks/infra/delay.c \
 $(BUS_MOTOR_STANDARD_ROOT)/sdks/infra/log.c
BUS_MOTOR_DM_INCLUDES += -I$(BUS_MOTOR_STANDARD_ROOT)/sdks/infra
# Board project must also include src/app src/service src/service/assemble src/platform
