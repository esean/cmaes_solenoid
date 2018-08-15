
/*
 * $Copyright$
 * Copyright (c) 2016 All Rights Reserved, Sean Harre
 *
 * Sean Harre is the copyright holder of all code below.
 * Do not re-use without permission.
 */

#ifndef _BNO055_DEFS_H
#define _BNO055_DEFS_H

// UART pkt max size
#define USB_CDC_MAX_PKT_SIZE    128

// comm response
#define BNO055_READ_STATUS_OK       0x00
#define BNO055_WRITE_STATUS_OK      0x01

// comm PDUs
#define BNO055_PDU_UART_START     0xAA
#define BNO055_PDU_READ_RESP      0xBB
#define BNO055_PDU_ERROR          0xEE
#define BNO055_UART_READ      0x01
#define BNO055_UART_WRITE     0x00

// registers
#define BNO055_REG_CHIP_ID      0x00
#define BNO055_REG_CHIP_ID_VAL      0xA0
#define BNO055_REG_ACC_ID       0x01
#define BNO055_REG_ACC_ID_VAL       0xFB
#define BNO055_REG_MAG_ID       0x02
#define BNO055_REG_MAG_ID_VAL       0x32
#define BNO055_REG_GYR_ID       0x03
#define BNO055_REG_GYR_ID_VAL       0x0F

#define BNO055_PAGE_ID          0x07

#define BNO055_EUL_HEAD_LSB     0x1A
#define BNO055_EUL_HEAD_MSB     0x1B
#define BNO055_EUL_ROLL_LSB     0x1C
#define BNO055_EUL_ROLL_MSB     0x1D
#define BNO055_EUL_PITCH_LSB    0x1E
#define BNO055_EUL_PITCH_MSB    0x1F

#define BNO055_QUAT_W_LSB       0x20
#define BNO055_QUAT_W_MSB       0x21
#define BNO055_QUAT_X_LSB       0x22
#define BNO055_QUAT_X_MSB       0x23
#define BNO055_QUAT_Y_LSB       0x24
#define BNO055_QUAT_Y_MSB       0x25
#define BNO055_QUAT_Z_LSB       0x26
#define BNO055_QUAT_Z_MSB       0x27

#define BNO055_LINACC_X_LSB     0x28
#define BNO055_LINACC_X_MSB     0x29
#define BNO055_LINACC_Y_LSB     0x2A
#define BNO055_LINACC_Y_MSB     0x2B
#define BNO055_LINACC_Z_LSB     0x2C
#define BNO055_LINACC_Z_MSB     0x2D

#define BNO055_GRAVVEC_X_LSB    0x2E
#define BNO055_GRAVVEC_X_MSB    0x2F
#define BNO055_GRAVVEC_Y_LSB    0x30
#define BNO055_GRAVVEC_Y_MSB    0x31
#define BNO055_GRAVVEC_Z_LSB    0x32
#define BNO055_GRAVVEC_Z_MSB    0x33

#define BNO055_REG_TEMP         0x34
#define BNO055_CALIB_STAT       0x35
#define BNO055_ST_RESULT        0x36
#define BNO055_INT_STA          0x37
#define BNO055_SYS_CLK_STATUS   0x38
#define BNO055_SYS_STATUS       0x39
#define BNO055_SYS_ERR          0x3A    // top register

#define BNO055_REG_ACCXL        0x08    // bottom register

#define BNO055_OPR_MODE         0x3D
#define BNO055_PWR_MODE         0x3E
#define BNO055_SYS_TRIGGER      0x3F
#define BNO055_SYS_TRIG_RST         0x20

#define BNO055_AXISMAP_CFG      0x41
#define BNO055_AXISMAP_SIGN     0x42

#define BNO055_CALIB_REGS_START 0x55
#define BNO055_CALIB_REGS_END   0x6A

typedef enum
{
    PAGE_0  = 0x0,
    PAGE_1  = 0x1,
} bno055_page_id;

typedef enum
{
    POWER_MODE_NORMAL                                       = 0X00,
    POWER_MODE_LOWPOWER                                     = 0X01,
    POWER_MODE_SUSPEND                                      = 0X02
} bno055_powermode_t;

typedef enum
{
    /* Operation mode settings*/
    OPERATION_MODE_CONFIG                                   = 0X00,
    OPERATION_MODE_ACCONLY                                  = 0X01,
    OPERATION_MODE_MAGONLY                                  = 0X02,
    OPERATION_MODE_GYRONLY                                  = 0X03,
    OPERATION_MODE_ACCMAG                                   = 0X04,
    OPERATION_MODE_ACCGYRO                                  = 0X05,
    OPERATION_MODE_MAGGYRO                                  = 0X06,
    OPERATION_MODE_AMG                                      = 0X07,
    OPERATION_MODE_IMUPLUS                                  = 0X08,
    OPERATION_MODE_COMPASS                                  = 0X09,
    OPERATION_MODE_M4G                                      = 0X0A,
    OPERATION_MODE_NDOF_FMC_OFF                             = 0X0B,
    OPERATION_MODE_NDOF                                     = 0X0C
} bno055_opmode_t;

typedef enum
{
    REMAP_CONFIG_P0                                         = 0x21,
    REMAP_CONFIG_P1                                         = 0x24, // default
    REMAP_CONFIG_P2                                         = 0x24,
    REMAP_CONFIG_P3                                         = 0x21,
    REMAP_CONFIG_P4                                         = 0x24,
    REMAP_CONFIG_P5                                         = 0x21,
    REMAP_CONFIG_P6                                         = 0x21,
    REMAP_CONFIG_P7                                         = 0x24
} bno055_axis_remap_config_t;

typedef enum
{
    REMAP_SIGN_P0                                           = 0x04,
    REMAP_SIGN_P1                                           = 0x00, // default
    REMAP_SIGN_P2                                           = 0x06,
    REMAP_SIGN_P3                                           = 0x02,
    REMAP_SIGN_P4                                           = 0x03,
    REMAP_SIGN_P5                                           = 0x01,
    REMAP_SIGN_P6                                           = 0x07,
    REMAP_SIGN_P7                                           = 0x05
} bno055_axis_remap_sign_t;


#endif

