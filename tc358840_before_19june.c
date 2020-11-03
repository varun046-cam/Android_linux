/******************************************************************************
 *
 *			INFORCE COMPUTING INC
 *
 * Copyright (c) 2017, InforceComputing Inc.,
 *
 * Kamalnath Manezhi <kamalnath@inforcecomputing.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *****************************************************************************/
/******************************************************************************

 *****************************************************************************/
#include "tc358840.h"
#include <linux/sched.h>
#include <linux/gpio.h>
#include <media/msm_vidc.h>

#define CDBG(fmt, args...) pr_err(fmt, ##args)

#define TC358840_V4L2_DRIVER_VERSION      "0.1"


static struct workqueue_struct *isr_wq = NULL;
static struct work_struct isr_work;
//static struct timer_list h2c_timer;
extern struct hdmi_in_data *hdmidata;
static int h2c_state;
static int hpd_status = -1;
static int unsupported = -1;
int flag_edid =1;
int disconnect = 0;
int tc35_resolution = 1080;
int have_supported_resolution = 0;
static uint16_t vi_reg_val;
static uint16_t horz_reg1;
static uint16_t horz_reg0;
static uint16_t htotal_reg_val;
static uint16_t pclk_reg_val;
static int stop_skip;
int power_up_skip = 0;
static struct msm_sensor_ctrl_t *tc358840_s_ctrl;
 extern int gCameraopen;
uint16_t vi_status_reg, horz_lin_count_reg0, horz_lin_count_reg1, interrupt_register;

extern int camera_resolution,resolution_changeee;

static uint32_t mflag = 0;

static uint32_t gBoot=0;

uint16_t vi_status_reg_temp=50;  //pkj added


uint16_t gvi_status_reg=0;   //pkj made global


DEFINE_MSM_MUTEX(tc358840_mut);
DEFINE_MSM_MUTEX(tc3588401_mut);
DEFINE_MSM_MUTEX(tc358440_write_lock);
DEFINE_MSM_MUTEX(tc358440_read_lock);

#if 0
static struct msm_camera_i2c_reg_conf tc358840_576p_settings[] = {
	/* YCbCr422, Type=0x1E, I2C addr inc. */
	{0x0004, 0x8004, H2C_DATA_16BIT},
	{0x0002, 0x0000, H2C_DATA_16BIT},
	{0x0006, 0x0008, H2C_DATA_16BIT},	/* FIFO Level[8:0] */
	/* CSI Tx0 (32-bit Registers) */
	{0x0108, 0x00000001, H2C_DATA_32BIT},
	{0x02AC, 0x000080BD, H2C_DATA_32BIT},
	{0x02A0, 0x00000000, H2C_DATA_32BIT},
	{0xffff, 0x0001, H2C_DATA_16BIT},//10msec delay
	{0x02A0, 0x00000003, H2C_DATA_32BIT},
	{0x010C, 0x00000001, H2C_DATA_32BIT},
	{0x0110, 0x00000007, H2C_DATA_32BIT},
	{0x0118, 0x00000014, H2C_DATA_32BIT},
	{0x0120, 0x00001388, H2C_DATA_32BIT},
	{0x0124, 0x00000000, H2C_DATA_32BIT},
	{0x0128, 0x007F0101, H2C_DATA_32BIT},
	{0x0140, 0x00010000, H2C_DATA_32BIT},
	{0x0144, 0x00010000, H2C_DATA_32BIT},
	{0x0148, 0x00001000, H2C_DATA_32BIT},
	{0x0150, 0x00000160, H2C_DATA_32BIT},
	{0x0158, 0x000000C8, H2C_DATA_32BIT},
	{0x0168, 0x0000002A, H2C_DATA_32BIT},
	{0x0170, 0x00000FB9, H2C_DATA_32BIT},
	{0x0214, 0x00000000, H2C_DATA_32BIT},
	{0x0254, 0x00000005, H2C_DATA_32BIT},
	{0x0258, 0x00230205, H2C_DATA_32BIT},
	{0x025C, 0x000C0007, H2C_DATA_32BIT},
	{0x0260, 0x00120006, H2C_DATA_32BIT},
	{0x0264, 0x00005A00, H2C_DATA_32BIT},
	{0x0268, 0x0000000E, H2C_DATA_32BIT},
	{0x026C, 0x000C0008, H2C_DATA_32BIT},
	{0x0270, 0x00000020, H2C_DATA_32BIT},
	{0x0274, 0x0000001F, H2C_DATA_32BIT},
	{0x011C, 0x00000001, H2C_DATA_32BIT},//CSI Tx0 START
	/* CSI Tx1 (32-bit Registers) */
	{0x04A0, 0x00000001, H2C_DATA_32BIT},
	{0xffff, 0x0001, H2C_DATA_16BIT},//10msec delay
	{0x04A0, 0x00000003, H2C_DATA_32BIT},
	/* SPLIT CONTROL */
	{0x5000, 0x0100, H2C_DATA_16BIT},
	{0x5002, 0x3424, H2C_DATA_16BIT},
	{0x500C, 0x8000, H2C_DATA_16BIT},
	/* END OF HDMI Rx INIT */
	{0x8544, 0x11, H2C_DATA_8BIT},
	{0x0004, 0x81D5, H2C_DATA_16BIT},
	{0x0006, 0x0000, H2C_DATA_16BIT},

};
#endif
static struct msm_camera_i2c_reg_conf tc358840_stop_settings[] = {
	{0x011C, 0x00000000, H2C_DATA_32BIT},//CSI Tx0 START
	{0x031C, 0x00000000, H2C_DATA_32BIT},//CSI Tx1 START
};

static struct msm_camera_i2c_reg_conf tc358840_reset_settings[] = {
       /*Soft Reset*/
       {0x0002, 0x3E00, H2C_DATA_16BIT},// Disabled HDMI-Rx Soft Reset
       {0x0004, 0x0000, H2C_DATA_16BIT},
       //{0xffff, 0x0001, H2C_DATA_16BIT},//10msec delay
       {0x0002, 0x0000, H2C_DATA_16BIT},
       {0x0004, 0x8004, H2C_DATA_16BIT},
};

#if 1
static struct msm_camera_i2c_reg_conf tc358840_interrupt_mask[] = {
         {0x0016, 0x0D3F, H2C_DATA_16BIT},
         {0x8512, 0xFE, H2C_DATA_8BIT},
         {0x8513, 0xDF, H2C_DATA_8BIT},
         {0x8514, 0xFF, H2C_DATA_8BIT},
         {0x8515, 0xFF, H2C_DATA_8BIT},
         {0x8516, 0xFF, H2C_DATA_8BIT},
         {0x8517, 0xFF, H2C_DATA_8BIT},
         {0x8518, 0xFF, H2C_DATA_8BIT},
         {0x8519, 0xFF, H2C_DATA_8BIT},
         {0x851B, 0xFF, H2C_DATA_8BIT},
};


#endif
static struct msm_camera_i2c_reg_conf tc358840_interrupt_reset[] = {
         {0x0014, 0xFFFF, H2C_DATA_16BIT},
         {0x8502, 0xFF, H2C_DATA_8BIT},
         {0x8503, 0xFF, H2C_DATA_8BIT},
         {0x8504, 0xFF, H2C_DATA_8BIT},
         {0x8505, 0xFF, H2C_DATA_8BIT},
         {0x8506, 0xFF, H2C_DATA_8BIT},
         {0x8507, 0xFF, H2C_DATA_8BIT},
         {0x8508, 0xFF, H2C_DATA_8BIT},
         {0x8509, 0xFF, H2C_DATA_8BIT},
         {0x0014, 0xFFFF, H2C_DATA_16BIT},
};

static struct msm_camera_i2c_reg_conf tc358840_init_settings_without_EDID[] = {
	/*Soft Reset*/
	{0x0002, 0x3F00, H2C_DATA_16BIT},
	{0x0004, 0x0000, H2C_DATA_16BIT},
	//{0xffff, 0x0001, H2C_DATA_16BIT},//10msec delay
	{0x0002, 0x0000, H2C_DATA_16BIT},
	{0x0004, 0x8004, H2C_DATA_16BIT},
	/*Interrupt Registers*/
	{0x0014, 0x0000, H2C_DATA_16BIT},
	{0x0016, 0x05FF, H2C_DATA_16BIT},
	/*Audio Global*/
	{0x0084, 0x1500, H2C_DATA_16BIT},
	/* HDMI PHY */
	{0x8410, 0x03, H2C_DATA_8BIT},
	{0x84F0, 0x31, H2C_DATA_8BIT},
	{0x84F4, 0x01, H2C_DATA_8BIT},
	/* HDMI CLOCK */
	{0x8540, 0x12C0, H2C_DATA_16BIT},
	{0x8630, 0x00, H2C_DATA_8BIT},
	{0x8631, 0x0753, H2C_DATA_16BIT},
	{0x8670, 0x02, H2C_DATA_8BIT},
	{0x8A0C, 0x12C0, H2C_DATA_16BIT},
	/* HDMI Interrupt Mask */
	{0x8502, 0x01, H2C_DATA_8BIT},
	{0x8512, 0xFE, H2C_DATA_8BIT},
	/* EDID */
	{0x85E0, 0x01, H2C_DATA_8BIT},
	{0x85E4, 0x01, H2C_DATA_8BIT},

	/* HDCP SETTING	 */
	{0x85EC, 0x01, H2C_DATA_8BIT},
	/* VIDEO OUTPUT FORMAT */
	{0x8A00, 0x01, H2C_DATA_8BIT},
	{0x8A01, 0x14, H2C_DATA_8BIT},
	{0x8A02, 0x43, H2C_DATA_8BIT},
	{0x8A08, 0x31, H2C_DATA_8BIT},
	/* HDMI AUDIO REFCLK */
	{0x8544, 0x10, H2C_DATA_8BIT},
	{0x8600, 0x00, H2C_DATA_8BIT},
	{0x8607, 0x00, H2C_DATA_8BIT},
	{0x8652, 0x61, H2C_DATA_8BIT},
	{0x8671, 0x020C49BA, H2C_DATA_32BIT},
	{0x8675, 0x01E1B08A, H2C_DATA_32BIT},
	{0x8680, 0x00, H2C_DATA_8BIT},
	{0x854A, 0x01, H2C_DATA_8BIT},
};


static struct msm_camera_i2c_reg_conf tc358840_init_settings[] = {

	/*Soft Reset*/
	{0x0002, 0x3F00, H2C_DATA_16BIT},
	{0x0004, 0x0000, H2C_DATA_16BIT},
	//{0xffff, 0x0001, H2C_DATA_16BIT},//10msec delay
	{0x0002, 0x0000, H2C_DATA_16BIT},
	{0x0004, 0x8004, H2C_DATA_16BIT},
	/*Interrupt Registers*/
	{0x0014, 0x0000, H2C_DATA_16BIT},
	{0x0016, 0x05FF, H2C_DATA_16BIT},
	/*Audio Global*/
	{0x0084, 0x1500, H2C_DATA_16BIT},
	/* HDMI PHY */
	{0x8410, 0x03, H2C_DATA_8BIT},
	{0x84F0, 0x31, H2C_DATA_8BIT},
	{0x84F4, 0x01, H2C_DATA_8BIT},
	/* HDMI CLOCK */
	{0x8540, 0x12C0, H2C_DATA_16BIT},
	{0x8630, 0x00, H2C_DATA_8BIT},
	{0x8631, 0x0753, H2C_DATA_16BIT},
	{0x8670, 0x02, H2C_DATA_8BIT},
	{0x8A0C, 0x12C0, H2C_DATA_16BIT},
	/* HDMI Interrupt Mask */
	{0x8502, 0x01, H2C_DATA_8BIT},
	{0x8512, 0xFE, H2C_DATA_8BIT},
	/* EDID */
	{0x85E0, 0x01, H2C_DATA_8BIT},
	{0x85E4, 0x01, H2C_DATA_8BIT},

	{0x8c00, 0xFFFFFF00,  H2C_DATA_32BIT},
	{0x8c04, 0x00FFFFFF,  H2C_DATA_32BIT},
	{0x8c08, 0x6640C324,  H2C_DATA_32BIT},
	{0x8c0c, 0x0124FFDE,  H2C_DATA_32BIT},
	{0x8c10, 0x03011C20,  H2C_DATA_32BIT},
	{0x8c14, 0x78000080,  H2C_DATA_32BIT},
	{0x8c18, 0xA2D5840A,  H2C_DATA_32BIT},
	{0x8c1c, 0x26A2525A,  H2C_DATA_32BIT},
	{0x8c20, 0x2054500C,  H2C_DATA_32BIT},
	{0x8c24, 0x01010000,  H2C_DATA_32BIT},
	{0x8c28, 0x01010101,  H2C_DATA_32BIT},
	{0x8c2c, 0x01010101,  H2C_DATA_32BIT},
	{0x8c30, 0x01010101,  H2C_DATA_32BIT},
	{0x8c34, 0x74040101,  H2C_DATA_32BIT},
	{0x8c38, 0x70F23000,  H2C_DATA_32BIT},
	{0x8c3c, 0x58B0805A,  H2C_DATA_32BIT},
	{0x8c40, 0xC220008A,  H2C_DATA_32BIT},
	{0x8c44, 0x5E000031,  H2C_DATA_32BIT},
	{0x8c48, 0x72001D01,  H2C_DATA_32BIT},
	{0x8c4c, 0x201ED051,  H2C_DATA_32BIT},
	{0x8c50, 0x0055286E,  H2C_DATA_32BIT},
	{0x8c54, 0x00000000,  H2C_DATA_32BIT},
	{0x8c58, 0x1D015E00,  H2C_DATA_32BIT},
	{0x8c5c, 0x38711880,  H2C_DATA_32BIT},
	{0x8c60, 0x2C58402D,  H2C_DATA_32BIT},
	{0x8c64, 0xC2200045,  H2C_DATA_32BIT},
	{0x8c68, 0x5E000031,  H2C_DATA_32BIT},
	{0x8c6c, 0xFC000000,  H2C_DATA_32BIT},
	{0x8c70, 0x43464900,  H2C_DATA_32BIT},
	{0x8c74, 0x2020200A,  H2C_DATA_32BIT},
	{0x8c78, 0x20202020,  H2C_DATA_32BIT},
	{0x8c7c, 0xC8012020,  H2C_DATA_32BIT},
	{0x8c80, 0x70140302,  H2C_DATA_32BIT},
	{0x8c84, 0x11620243,  H2C_DATA_32BIT},
	{0x8c88, 0x000C0367,  H2C_DATA_32BIT},
	{0x8c8c, 0x3C000000,  H2C_DATA_32BIT},
	{0x8c90, 0x01068923,  H2C_DATA_32BIT},
	{0x8c94, 0x30007404,  H2C_DATA_32BIT},
	{0x8c98, 0x805A70F2,  H2C_DATA_32BIT},
	{0x8c9c, 0x008A58B0,  H2C_DATA_32BIT},
	{0x8ca0, 0x0031C220,  H2C_DATA_32BIT},
	{0x8ca4, 0x00005E00,  H2C_DATA_32BIT},
	{0x8ca8, 0x00000000,  H2C_DATA_32BIT},
	{0x8cac, 0x00000000,  H2C_DATA_32BIT},
	{0x8cb0, 0x00000000,  H2C_DATA_32BIT},
	{0x8cb4, 0x00000000,  H2C_DATA_32BIT},
	{0x8cb8, 0x00000000,  H2C_DATA_32BIT},
	{0x8cbc, 0x00000000,  H2C_DATA_32BIT},
	{0x8cc0, 0x00000000,  H2C_DATA_32BIT},
	{0x8cc4, 0x00000000,  H2C_DATA_32BIT},
	{0x8cc8, 0x00000000,  H2C_DATA_32BIT},
	{0x8ccc, 0x00000000,  H2C_DATA_32BIT},
	{0x8cd0, 0x00000000,  H2C_DATA_32BIT},
	{0x8cd4, 0x00000000,  H2C_DATA_32BIT},
	{0x8cd8, 0x00000000,  H2C_DATA_32BIT},
	{0x8cdc, 0x00000000,  H2C_DATA_32BIT},
	{0x8ce0, 0x00000000,  H2C_DATA_32BIT},
	{0x8ce4, 0x00000000,  H2C_DATA_32BIT},
	{0x8ce8, 0x00000000,  H2C_DATA_32BIT},
	{0x8cec, 0x00000000,  H2C_DATA_32BIT},
	{0x8cf0, 0x00000000,  H2C_DATA_32BIT},
	{0x8cf4, 0x00000000,  H2C_DATA_32BIT},
	{0x8cf8, 0x00000000,  H2C_DATA_32BIT},
	{0x8cfc, 0x73000000,  H2C_DATA_32BIT},
	/* HDCP SETTING	 */
	{0x85EC, 0x01, H2C_DATA_8BIT},
	/* VIDEO OUTPUT FORMAT */
	{0x8A00, 0x01, H2C_DATA_8BIT},
	{0x8A01, 0x14, H2C_DATA_8BIT},
	{0x8A02, 0x43, H2C_DATA_8BIT},
	{0x8A08, 0x31, H2C_DATA_8BIT},
	/* HDMI AUDIO REFCLK */
	{0x8544, 0x10, H2C_DATA_8BIT},
	{0x8600, 0x00, H2C_DATA_8BIT},
	{0x8607, 0x00, H2C_DATA_8BIT},
	{0x8652, 0x61, H2C_DATA_8BIT},
	{0x8671, 0x020C49BA, H2C_DATA_32BIT},
	{0x8675, 0x01E1B08A, H2C_DATA_32BIT},
	{0x8680, 0x00, H2C_DATA_8BIT},
	{0x854A, 0x01, H2C_DATA_8BIT},
};

static struct msm_camera_i2c_reg_conf tc358840_720p_settings[] = {

	/* YCbCr422, Type=0x1E, I2C addr inc. */
	{0x0004, 0x8004, H2C_DATA_16BIT},
	{0x0002, 0x0000, H2C_DATA_16BIT},
	{0x0006, 0x0008, H2C_DATA_16BIT},	/* FIFO Level[8:0] */
	/* CSI Tx0 (32-bit Registers) */
	{0x0108, 0x00000001, H2C_DATA_32BIT},
	{0x02AC, 0x000080BD, H2C_DATA_32BIT},
	{0x02A0, 0x00000000, H2C_DATA_32BIT},
	{0xffff, 0x0001, H2C_DATA_16BIT},//10msec delay
	{0x02A0, 0x00000003, H2C_DATA_32BIT},
	{0x010C, 0x00000001, H2C_DATA_32BIT},
	{0x0110, 0x00000007, H2C_DATA_32BIT},
	{0x0118, 0x00000014, H2C_DATA_32BIT},
	{0x0120, 0x00001388, H2C_DATA_32BIT},
	{0x0124, 0x00000000, H2C_DATA_32BIT},
	{0x0128, 0x007F0101, H2C_DATA_32BIT},
	{0x0140, 0x00010000, H2C_DATA_32BIT},
	{0x0144, 0x00010000, H2C_DATA_32BIT},
	{0x0148, 0x00001000, H2C_DATA_32BIT},
	{0x0150, 0x00000160, H2C_DATA_32BIT},
	{0x0158, 0x000000C8, H2C_DATA_32BIT},
	{0x0168, 0x0000002A, H2C_DATA_32BIT},
	{0x0170, 0x00001A92, H2C_DATA_32BIT},
	{0x0214, 0x00000000, H2C_DATA_32BIT},
	{0x0254, 0x00000005, H2C_DATA_32BIT},
	{0x0258, 0x00230205, H2C_DATA_32BIT},
	{0x025C, 0x000C0007, H2C_DATA_32BIT},
	{0x0260, 0x00120006, H2C_DATA_32BIT},
	{0x0264, 0x00005A00, H2C_DATA_32BIT},
	{0x0268, 0x0000000E, H2C_DATA_32BIT},
	{0x026C, 0x000C0008, H2C_DATA_32BIT},
	{0x0270, 0x00000020, H2C_DATA_32BIT},
	{0x0274, 0x0000001F, H2C_DATA_32BIT},
	{0x011C, 0x00000001, H2C_DATA_32BIT},//CSI Tx0 START
	/* CSI Tx1 (32-bit Registers) */
	{0x04A0, 0x00000001, H2C_DATA_32BIT},
	{0xffff, 0x0001, H2C_DATA_16BIT},//10msec delay
	{0x04A0, 0x00000003, H2C_DATA_32BIT},
	{0xffff, 0x0001, H2C_DATA_16BIT},//10msec delay
	{0xffff, 0x00C8, H2C_DATA_16BIT},//2000msec delay
	/* SPLIT CONTROL */
	{0x5000, 0x0100, H2C_DATA_16BIT},
	{0x5002, 0x3424, H2C_DATA_16BIT},
	{0x500C, 0x8000, H2C_DATA_16BIT},
	/* END OF HDMI Rx INIT */
	{0x8544, 0x11, H2C_DATA_8BIT},
	{0x0004, 0x81D5, H2C_DATA_16BIT},
	{0x0006, 0x0000, H2C_DATA_16BIT},

};

static struct msm_camera_i2c_reg_conf tc358840_1080p_settings[] = {
	/* YCbCr422, Type=0x1E, I2C addr inc. */
	{0x0004, 0x8004, H2C_DATA_16BIT},
	{0x0002, 0x0000, H2C_DATA_16BIT},
	{0x0006, 0x0008, H2C_DATA_16BIT},	/* FIFO Level[8:0] */
	/* CSI Tx0 (32-bit Registers) */
	{0x0108, 0x00000001, H2C_DATA_32BIT},
	{0x02AC, 0x00003264, H2C_DATA_32BIT},
	{0x02A0, 0x00000001, H2C_DATA_32BIT},
	{0xffff, 0x0001, H2C_DATA_16BIT},//10msec delay
	{0x02A0, 0x00000003, H2C_DATA_32BIT},
	{0x010C, 0x00000001, H2C_DATA_32BIT},
	{0x0110, 0x00000007, H2C_DATA_32BIT},
	{0x0118, 0x00000014, H2C_DATA_32BIT},
	{0x0120, 0x00001388, H2C_DATA_32BIT},
	{0x0124, 0x00000000, H2C_DATA_32BIT},
	{0x0128, 0x007F0101, H2C_DATA_32BIT},
	{0x0140, 0x00010000, H2C_DATA_32BIT},
	{0x0144, 0x00010000, H2C_DATA_32BIT},
	{0x0148, 0x00001000, H2C_DATA_32BIT},
	{0x0150, 0x00000160, H2C_DATA_32BIT},
	{0x0158, 0x000000C8, H2C_DATA_32BIT},
	{0x0168, 0x0000002A, H2C_DATA_32BIT},
	{0x0170, 0x000011F6, H2C_DATA_32BIT},
	{0x0214, 0x00000000, H2C_DATA_32BIT},
	{0x0254, 0x00000005, H2C_DATA_32BIT},
	{0x0258, 0x00230205, H2C_DATA_32BIT},
	{0x025C, 0x000C0007, H2C_DATA_32BIT},
	{0x0260, 0x00120006, H2C_DATA_32BIT},
	{0x0264, 0x00005A00, H2C_DATA_32BIT},
	{0x0268, 0x0000000E, H2C_DATA_32BIT},
	{0x026C, 0x000C0008, H2C_DATA_32BIT},
	{0x0270, 0x00000020, H2C_DATA_32BIT},
	{0x0274, 0x0000001F, H2C_DATA_32BIT},
	{0x011C, 0x00000001, H2C_DATA_32BIT},//CSI Tx0 START
	/* CSI Tx1 (32-bit Registers) */
	{0x04A0, 0x00000001, H2C_DATA_32BIT},
	{0xffff, 0x0001, H2C_DATA_16BIT},//10msec delay
	{0x04A0, 0x00000003, H2C_DATA_32BIT},
	{0xffff, 0x0001, H2C_DATA_16BIT},//10msec delay
	{0xffff, 0x00C8, H2C_DATA_16BIT},//2000msec delay
	/* SPLIT CONTROL */
	{0x5000, 0x0100, H2C_DATA_16BIT},
	{0x5002, 0x3424, H2C_DATA_16BIT},
	{0x500C, 0x8000, H2C_DATA_16BIT},
	/* END OF HDMI Rx INIT */
	{0x8544, 0x11, H2C_DATA_8BIT},
	{0x0004, 0x81D5, H2C_DATA_16BIT},
	{0x0006, 0x0000, H2C_DATA_16BIT},

};

static struct msm_camera_i2c_reg_conf tc358840_VGA_settings[] = {

	/* YCbCr422, Type=0x1E, I2C addr inc. */
	{0x0004, 0x8004, H2C_DATA_16BIT},
	{0x0002, 0x0000, H2C_DATA_16BIT},
	{0x0006, 0x0008, H2C_DATA_16BIT},	/* FIFO Level[8:0] */
	/* CSI Tx0 (32-bit Registers) */
	{0x0108, 0x00000001, H2C_DATA_32BIT},
	{0x02AC, 0x000080BD, H2C_DATA_32BIT},
	{0x02A0, 0x00000000, H2C_DATA_32BIT},
	{0xffff, 0x0001, H2C_DATA_16BIT},//10msec delay
	{0x02A0, 0x00000003, H2C_DATA_32BIT},
	{0x010C, 0x00000001, H2C_DATA_32BIT},
	{0x0110, 0x00000007, H2C_DATA_32BIT},
	{0x0118, 0x00000014, H2C_DATA_32BIT},
	{0x0120, 0x00001388, H2C_DATA_32BIT},
	{0x0124, 0x00000000, H2C_DATA_32BIT},
	{0x0128, 0x007F0101, H2C_DATA_32BIT},
	{0x0140, 0x00010000, H2C_DATA_32BIT},
	{0x0144, 0x00010000, H2C_DATA_32BIT},
	{0x0148, 0x00001000, H2C_DATA_32BIT},
	{0x0150, 0x00000160, H2C_DATA_32BIT},
	{0x0158, 0x000000C8, H2C_DATA_32BIT},
	{0x0168, 0x0000002A, H2C_DATA_32BIT},
	{0x0170, 0x0000107C, H2C_DATA_32BIT},
	{0x0214, 0x00000000, H2C_DATA_32BIT},
	{0x0254, 0x00000005, H2C_DATA_32BIT},
	{0x0258, 0x00230205, H2C_DATA_32BIT},
	{0x025C, 0x000C0007, H2C_DATA_32BIT},
	{0x0260, 0x00120006, H2C_DATA_32BIT},
	{0x0264, 0x00005A00, H2C_DATA_32BIT},
	{0x0268, 0x0000000E, H2C_DATA_32BIT},
	{0x026C, 0x000C0008, H2C_DATA_32BIT},
	{0x0270, 0x00000020, H2C_DATA_32BIT},
	{0x0274, 0x0000001F, H2C_DATA_32BIT},
	{0x011C, 0x00000001, H2C_DATA_32BIT},//CSI Tx0 START
	/* CSI Tx1 (32-bit Registers) */
	{0x04A0, 0x00000001, H2C_DATA_32BIT},
	{0xffff, 0x0001, H2C_DATA_16BIT},//10msec delay
	{0x04A0, 0x00000003, H2C_DATA_32BIT},
	/* SPLIT CONTROL */
	{0x5000, 0x0100, H2C_DATA_16BIT},
	{0x5002, 0x3424, H2C_DATA_16BIT},
	{0x500C, 0x8000, H2C_DATA_16BIT},
	/* END OF HDMI Rx INIT */
	{0x8544, 0x11, H2C_DATA_8BIT},
	{0x0004, 0x81D5, H2C_DATA_16BIT},
	{0x0006, 0x0000, H2C_DATA_16BIT},

};

static struct msm_camera_i2c_reg_conf tc358840_4K_UHD_split_settings[] = {
	/* YCbCr422, Type=0x1E, I2C addr inc. */
	{0x0004, 0x8004, H2C_DATA_16BIT},
	{0x0002, 0x0000, H2C_DATA_16BIT},
	{0x0006, 0x0008, H2C_DATA_16BIT},	/* FIFO Level[8:0] */
	/* CSI Tx0 (32-bit Registers) */
	{0x0108, 0x00000001, H2C_DATA_32BIT},
	{0x02AC, 0x000080BD, H2C_DATA_32BIT},
	{0x02A0, 0x00000001, H2C_DATA_32BIT},

	{0xffff, 0x0001, H2C_DATA_16BIT},//10msec delay

	{0x02A0, 0x00000003, H2C_DATA_32BIT},
	{0x010C, 0x00000001, H2C_DATA_32BIT},
	{0x0110, 0x00000007, H2C_DATA_32BIT},
	{0x0118, 0x00000014, H2C_DATA_32BIT},
	{0x0120, 0x00001388, H2C_DATA_32BIT},
	{0x0124, 0x00000000, H2C_DATA_32BIT},
	{0x0128, 0x007F0101, H2C_DATA_32BIT},
	{0x0140, 0x00010000, H2C_DATA_32BIT},
	{0x0144, 0x00010000, H2C_DATA_32BIT},
	{0x0148, 0x00001000, H2C_DATA_32BIT},
	{0x0150, 0x00000160, H2C_DATA_32BIT},
	{0x0158, 0x000000C8, H2C_DATA_32BIT},
	{0x0168, 0x0000002A, H2C_DATA_32BIT},
	{0x0170, 0x00000767, H2C_DATA_32BIT},
	{0x0214, 0x00000000, H2C_DATA_32BIT},
	{0x0254, 0x00000005, H2C_DATA_32BIT},
	{0x0258, 0x00230205, H2C_DATA_32BIT},
	{0x025C, 0x000C0007, H2C_DATA_32BIT},
	{0x0260, 0x00120006, H2C_DATA_32BIT},
	{0x0264, 0x00005A00, H2C_DATA_32BIT},
	{0x0268, 0x0000000E, H2C_DATA_32BIT},
	{0x026C, 0x000C0008, H2C_DATA_32BIT},
	{0x0270, 0x00000020, H2C_DATA_32BIT},
	{0x0274, 0x0000001F, H2C_DATA_32BIT},
	{0x011C, 0x00000001, H2C_DATA_32BIT},//CSI Tx0 START

	/* CSI Tx1 (32-bit Registers) */
	{0x0308, 0x00000001, H2C_DATA_32BIT},
	{0x04AC, 0x000080BD, H2C_DATA_32BIT},
	{0x04A0, 0x00000001, H2C_DATA_32BIT},

	{0xffff, 0x0001, H2C_DATA_16BIT},//10msec delay

	{0x04A0, 0x00000003, H2C_DATA_32BIT},
	{0x030C, 0x00000001, H2C_DATA_32BIT},
	{0x0310, 0x00000007, H2C_DATA_32BIT},
	{0x0318, 0x00000014, H2C_DATA_32BIT},
	{0x0320, 0x00001388, H2C_DATA_32BIT},
	{0x0324, 0x00000000, H2C_DATA_32BIT},
	{0x0328, 0x007F0101, H2C_DATA_32BIT},
	{0x0340, 0x00010000, H2C_DATA_32BIT},
	{0x0344, 0x00010000, H2C_DATA_32BIT},
	{0x0348, 0x00001000, H2C_DATA_32BIT},
	{0x0350, 0x00000160, H2C_DATA_32BIT},
	{0x0358, 0x000000C8, H2C_DATA_32BIT},
	{0x0368, 0x0000002A, H2C_DATA_32BIT},
	{0x0370, 0x00000767, H2C_DATA_32BIT},
	{0x0414, 0x00000000, H2C_DATA_32BIT},
	{0x0454, 0x00000005, H2C_DATA_32BIT},
	{0x0458, 0x00230205, H2C_DATA_32BIT},
	{0x045C, 0x000C0007, H2C_DATA_32BIT},
	{0x0460, 0x00120006, H2C_DATA_32BIT},
	{0x0464, 0x00005A00, H2C_DATA_32BIT},
	{0x0468, 0x0000000E, H2C_DATA_32BIT},
	{0x046C, 0x000C0008, H2C_DATA_32BIT},
	{0x0470, 0x00000020, H2C_DATA_32BIT},
	{0x0474, 0x0000001F, H2C_DATA_32BIT},
	{0x031C, 0x00000001, H2C_DATA_32BIT},//CSI Tx0 START
	/* SPLIT CONTROL */
	{0x5000, 0x0000, H2C_DATA_16BIT},
	{0x5002, 0x3424, H2C_DATA_16BIT},
	{0x500C, 0x8000, H2C_DATA_16BIT},
	{0x5080, 0x0000, H2C_DATA_16BIT},
	{0x508C, 0x0000, H2C_DATA_16BIT},
	/* END OF HDMI Rx INIT */
	{0x8544, 0x11, H2C_DATA_8BIT},
	{0x0004, 0x81D7, H2C_DATA_16BIT},
	{0x0006, 0x0000, H2C_DATA_16BIT},
};

static struct msm_camera_i2c_reg_conf tc358840_4K_DCI_split_settings[] = {
	/* YCbCr422, Type=0x1E, I2C addr inc. */
	{0x0004, 0x8004, H2C_DATA_16BIT},
	{0x0002, 0x0000, H2C_DATA_16BIT},
	{0x0006, 0x0008, H2C_DATA_16BIT},	/* FIFO Level[8:0] */
	/* CSI Tx0 (32-bit Registers) */
	{0x0108, 0x00000001, H2C_DATA_32BIT},
	{0x02AC, 0x000080BD, H2C_DATA_32BIT},
	{0x02A0, 0x00000001, H2C_DATA_32BIT},

	{0xffff, 0x0001, H2C_DATA_16BIT},//10msec delay

	{0x02A0, 0x00000003, H2C_DATA_32BIT},
	{0x010C, 0x00000001, H2C_DATA_32BIT},
	{0x0110, 0x00000007, H2C_DATA_32BIT},
	{0x0118, 0x00000014, H2C_DATA_32BIT},
	{0x0120, 0x00001388, H2C_DATA_32BIT},
	{0x0124, 0x00000000, H2C_DATA_32BIT},
	{0x0128, 0x007F0101, H2C_DATA_32BIT},
	{0x0140, 0x00010000, H2C_DATA_32BIT},
	{0x0144, 0x00010000, H2C_DATA_32BIT},
	{0x0148, 0x00001000, H2C_DATA_32BIT},
	{0x0150, 0x00000160, H2C_DATA_32BIT},
	{0x0158, 0x000000C8, H2C_DATA_32BIT},
	{0x0168, 0x0000002A, H2C_DATA_32BIT},
	{0x0170, 0x00000767, H2C_DATA_32BIT},
	{0x0214, 0x00000000, H2C_DATA_32BIT},
	{0x0254, 0x00000005, H2C_DATA_32BIT},
	{0x0258, 0x00230205, H2C_DATA_32BIT},
	{0x025C, 0x000C0007, H2C_DATA_32BIT},
	{0x0260, 0x00120006, H2C_DATA_32BIT},
	{0x0264, 0x00005A00, H2C_DATA_32BIT},
	{0x0268, 0x0000000E, H2C_DATA_32BIT},
	{0x026C, 0x000C0008, H2C_DATA_32BIT},
	{0x0270, 0x00000020, H2C_DATA_32BIT},
	{0x0274, 0x0000001F, H2C_DATA_32BIT},
	{0x011C, 0x00000001, H2C_DATA_32BIT},//CSI Tx0 START

	/* CSI Tx1 (32-bit Registers) */
	{0x0308, 0x00000001, H2C_DATA_32BIT},
	{0x04AC, 0x000080BD, H2C_DATA_32BIT},
	{0x04A0, 0x00000001, H2C_DATA_32BIT},

	{0xffff, 0x0001, H2C_DATA_16BIT},//10msec delay

	{0x04A0, 0x00000003, H2C_DATA_32BIT},
	{0x030C, 0x00000001, H2C_DATA_32BIT},
	{0x0310, 0x00000007, H2C_DATA_32BIT},
	{0x0318, 0x00000014, H2C_DATA_32BIT},
	{0x0320, 0x00001388, H2C_DATA_32BIT},
	{0x0324, 0x00000000, H2C_DATA_32BIT},
	{0x0328, 0x007F0101, H2C_DATA_32BIT},
	{0x0340, 0x00010000, H2C_DATA_32BIT},
	{0x0344, 0x00010000, H2C_DATA_32BIT},
	{0x0348, 0x00001000, H2C_DATA_32BIT},
	{0x0350, 0x00000160, H2C_DATA_32BIT},
	{0x0358, 0x000000C8, H2C_DATA_32BIT},
	{0x0368, 0x0000002A, H2C_DATA_32BIT},
	{0x0370, 0x00000767, H2C_DATA_32BIT},
	{0x0414, 0x00000000, H2C_DATA_32BIT},
	{0x0454, 0x00000005, H2C_DATA_32BIT},
	{0x0458, 0x00230205, H2C_DATA_32BIT},
	{0x045C, 0x000C0007, H2C_DATA_32BIT},
	{0x0460, 0x00120006, H2C_DATA_32BIT},
	{0x0464, 0x00005A00, H2C_DATA_32BIT},
	{0x0468, 0x0000000E, H2C_DATA_32BIT},
	{0x046C, 0x000C0008, H2C_DATA_32BIT},
	{0x0470, 0x00000020, H2C_DATA_32BIT},
	{0x0474, 0x0000001F, H2C_DATA_32BIT},
	{0x031C, 0x00000001, H2C_DATA_32BIT},//CSI Tx0 START
	/* SPLIT CONTROL */
	{0x5000, 0x0000, H2C_DATA_16BIT},
	{0x5002, 0x3424, H2C_DATA_16BIT},
	{0x500C, 0x8000, H2C_DATA_16BIT},
	{0x5080, 0x0000, H2C_DATA_16BIT},
	{0x508C, 0x0000, H2C_DATA_16BIT},
	/* END OF HDMI Rx INIT */
	{0x8544, 0x11, H2C_DATA_8BIT},
	{0x0004, 0x81D7, H2C_DATA_16BIT},
	{0x0006, 0x0000, H2C_DATA_16BIT},
};

static void isr_work_handler (struct work_struct *work);
static irqreturn_t tc358840_irq_handler (int irq, void *dev_id);

int is_supported_res(int res)
{
	int rc = 0;
	pr_debug("%s res=%d \n",__func__,res);

	switch (res) {
		case H2C_RES_1080P:
		case H2C_RES_720P:
		//case H2C_RES_576P:
		//case H2C_RES_480P:
		case H2C_RES_VGA:
			rc=1;
			break;

		break;
	}

	return rc;
}

int interrupt_request(struct msm_sensor_ctrl_t *s_ctrl)
{
         int ret;
         //writting 1.reset 2.maksing 3.resting
         tc358840_write_conf_tbl(s_ctrl, tc358840_interrupt_reset, sizeof(tc358840_interrupt_reset)/sizeof(tc358840_interrupt_reset[0]));
         tc358840_write_conf_tbl(s_ctrl, tc358840_interrupt_mask, sizeof(tc358840_interrupt_mask)/sizeof(tc358840_interrupt_mask[0]));
         tc358840_write_conf_tbl(s_ctrl, tc358840_interrupt_reset,sizeof(tc358840_interrupt_reset)/sizeof(tc358840_interrupt_reset[0]));

         /*Request IRQ regestring */
         ret = request_threaded_irq(s_ctrl->irq_gpio, tc358840_irq_handler, NULL, IRQF_TRIGGER_RISING, "tc358840_interrupt", s_ctrl);
         if(ret < 0){
           pr_err ("Error: Request_threaded_irq can not register \n");
           return ret;
	 }
         isr_wq = create_workqueue("wq_ISR");
         INIT_WORK(&isr_work, isr_work_handler);
         return 0;
}

int32_t tc358840_write_conf_tbl(
	struct msm_sensor_ctrl_t *s_ctrl,
	struct msm_camera_i2c_reg_conf *reg_conf_tbl, uint16_t size)
{
	int i;
	int32_t rc = -EFAULT;
	for (i = 0; i < size; i++) {
		if(reg_conf_tbl->reg_addr == 0xffff) {
			msleep(10 * reg_conf_tbl->reg_data);
			rc = 0;
		} else {
		rc = tc358840_cci_write(s_ctrl, reg_conf_tbl->reg_addr,\
				reg_conf_tbl->reg_data, reg_conf_tbl->dt);
			if (rc < 0)
				break;
		}
		reg_conf_tbl++;
	}
	return rc;
}

int tc358840_write_stop_settings(){
	int rc = 0;

	rc = tc358840_write_conf_tbl(tc358840_s_ctrl, tc358840_stop_settings, sizeof (tc358840_stop_settings)/sizeof (tc358840_stop_settings[0]));
	rc = tc358840_write_conf_tbl(tc358840_s_ctrl, tc358840_reset_settings, sizeof (tc358840_reset_settings)/sizeof (tc358840_reset_settings[0]));
	pr_err("%sExit",__func__);
	return rc;
}

static int32_t tc358440_cci_read(struct msm_sensor_ctrl_t *s_ctrl,
					uint32_t addr, uint16_t *data,
					uint16_t len)   //pkj added
{
	int32_t rc = -EFAULT;

	mutex_lock(&tc358440_read_lock);

	switch (len) {
	case H2C_DATA_8BIT:
			rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_read(
						s_ctrl->sensor_i2c_client, addr,
						data, MSM_CAMERA_I2C_BYTE_DATA);
		if (rc < 0)
			pr_err("%s: Line %d: error i2c read BYTE failed\n",
							__func__, __LINE__);
	break;

	case H2C_DATA_16BIT:
			rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_read(
						s_ctrl->sensor_i2c_client, addr,
						data, MSM_CAMERA_I2C_WORD_DATA);
		if (rc < 0)
			pr_err("%s: Line %d: error i2c read WORD failed\n",
							__func__, __LINE__);
	break;

	case H2C_DATA_32BIT:
		{
			uint8_t read_data[4] = {0};
			rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_read_seq(
				s_ctrl->sensor_i2c_client, addr, (uint8_t *)read_data, 4);

			if (rc < 0)
				pr_err("%s: Line %d: error i2c read 32-bit failed\n",
								__func__, __LINE__);
		}
	break;
	default:
		pr_err("%s: ERROR\n", __func__);
	break;
	}

	mutex_unlock(&tc358440_read_lock);

	return rc;
}

void hdmi_in_send_uevent(int hdmi_in_state_val, int event_reason)
{
	char *envp[3];

	 /*Default values*/
	envp[0] = "HDMI_IN_STATE=RUNNING";
	envp[1] = "HDMI_IN_EVENT_REASON=NONE";
	envp[2] =  NULL;
	pr_err("hdmi_in sending uevent called - %d\n", hdmi_in_state_val);

	switch (hdmi_in_state_val) {
	case HDMI_IN_PORT_CONNECTED:
		envp[0] = "HDMI_IN_STATE=3";
		pr_err("HDMI_IN_STATE: send CONNECTED\n");
	break;
	case HDMI_IN_PORT_DISCONNECTED:
		envp[0] = "HDMI_IN_STATE=2";
		pr_err("HDMI_IN_STATE: send DISCONNECTED\n");
	break;
	case HDMI_IN_STOP:
		envp[0] = "HDMI_IN_STATE=0";
		pr_err("HDMI_IN_STATE: send STOPPED\n");
		tc358840_write_conf_tbl(tc358840_s_ctrl, tc358840_reset_settings, sizeof(tc358840_reset_settings)/sizeof(tc358840_reset_settings[0]));

		stop_skip=0;
	break;
	case HDMI_IN_START:
		envp[0] = "HDMI_IN_STATE=1";
		pr_err("HDMI_IN_STATE: send STARTED\n");
		break;
	}

	switch (event_reason) {
	case HDMI_IN_RESOLUTION_SWITCH:
		envp[1] = "HDMI_IN_EVENT_REASON=1";
		pr_err("HDMI_IN_REASON: RESOLUTION_SWITCH\n");
		break;
	case HDMI_IN_SLEEP:
		envp[1] = "HDMI_IN_EVENT_REASON=2";
		pr_err("HDMI_IN_REASON: SLEEP\n");
		break;
	case HDMI_IN_WAKE:
		envp[1] = "HDMI_IN_EVENT_REASON=3";
		pr_err("HDMI_IN_REASON: WAKE\n");
		break;
	case HDMI_IN_UNKNOWN_RESOLUTION:
		envp[1] = "HDMI_IN_EVENT_REASON=4";
		pr_err("HDMI_IN_REASON: UNKNOWN_RESOLUTION\n");
		tc358840_write_conf_tbl(tc358840_s_ctrl, tc358840_reset_settings, sizeof(tc358840_reset_settings)/sizeof(tc358840_reset_settings[0]));
		break;
	default :
		envp[1] = "HDMI_IN_EVENT_REASON=0";
		pr_err("HDMI_IN_REASON: UNKNOWN\n");
	break;
	}

	kobject_uevent_env(hdmidata->hdmi_in_uevent_kobj, KOBJ_CHANGE, envp);
}

int32_t tc358840_hdmi_sys_int_handler (struct msm_sensor_ctrl_t * s_ctrl)
{
         uint16_t sys_status_register, clock_interrupt, system_interrupt;

         tc358440_cci_read (tc358840_s_ctrl, (uint32_t) SYSTEM_INTERRUPT, &system_interrupt, H2C_DATA_8BIT);
         tc358440_cci_read (tc358840_s_ctrl, (uint32_t) H2C_SYS_STATUS_REG, &sys_status_register, H2C_DATA_8BIT);
         tc358440_cci_read (tc358840_s_ctrl, (uint32_t) CLOCK_INTERRUPT, &clock_interrupt, H2C_DATA_8BIT);
	 vi_reg_val	= 0;
	 horz_reg0 	= 0;
	 horz_reg1 	= 0;
         h2c_state = H2C_STOPPED;
         disconnect = 1;
         hdmi_in_send_uevent (HDMI_IN_PORT_DISCONNECTED, HDMI_IN_UNKNOWN_REASON);
         return 0;
}

int32_t tc358840_hdmi_clk_int_handler (struct msm_sensor_ctrl_t * s_ctrl)
{
	uint16_t retry = 0;
	msleep(2500);
	have_supported_resolution = 0;
	for (retry=0; retry<50; retry++) {
		hpd_status = gpio_get_value(tc358840_s_ctrl->hpd_gpio);
		if(hpd_status){
			disconnect = 1;
			h2c_state = H2C_STOPPED;
			hdmi_in_send_uevent(HDMI_IN_PORT_DISCONNECTED, HDMI_IN_UNKNOWN_REASON);
			interrupt_request(tc358840_s_ctrl);
			return 0;
		}
		msleep(10);
		tc358440_cci_read(s_ctrl, (uint32_t)H2C_VI_STATUS_REG, &vi_status_reg, H2C_DATA_8BIT);
		tc358440_cci_read(s_ctrl, (uint32_t)HORIZONTAL_LINE_COUNT_REG0, &horz_lin_count_reg0, H2C_DATA_8BIT);
		tc358440_cci_read(s_ctrl, (uint32_t)HORIZONTAL_LINE_COUNT_REG1, &horz_lin_count_reg1, H2C_DATA_8BIT);
	}
	pr_err ("H2C_VI_STATUS_REG- %d\n",vi_status_reg);
	pr_err ("HORIZONTAL_LINE_COUNT_REG0- %d\n",horz_lin_count_reg0);
	pr_err ("HORIZONTAL_LINE_COUNT_REG1- %d\n",horz_lin_count_reg1);

	if(disconnect == 1){
		flag_edid = 1;
	}else{
		flag_edid = 0;
	}
	if (is_supported_res(vi_status_reg) || (horz_lin_count_reg0 == 0x00 && horz_lin_count_reg1 == 0x0F) || (horz_lin_count_reg0 == 0x00 && horz_lin_count_reg1 == 0x10)) {

	     hdmi_in_send_uevent (HDMI_IN_PORT_CONNECTED, HDMI_IN_UNKNOWN_REASON);
	     h2c_state = H2C_RESTARTING;
	     hdmi_in_send_uevent (HDMI_IN_STOP, HDMI_IN_RESOLUTION_SWITCH);
	     have_supported_resolution = 1;
	 }else{
	     h2c_state = H2C_STARTED;
	     hdmi_in_send_uevent (HDMI_IN_PORT_CONNECTED, HDMI_IN_UNKNOWN_RESOLUTION);
	     interrupt_request(tc358840_s_ctrl);
	 }
	 return 0;
}

static void isr_work_handler (struct work_struct *work){

         uint16_t sys_interrupt_status_register, system_interrupt, clock_interrupt, sys_status_register;
         free_irq (tc358840_s_ctrl->irq_gpio, tc358840_s_ctrl);
         tc358840_write_stop_settings();
         tc358840_write_conf_tbl (tc358840_s_ctrl, tc358840_interrupt_reset, sizeof (tc358840_interrupt_reset) / sizeof (tc358840_interrupt_reset[0]));
         pr_debug ("Entry-%s\n", __func__);
         msleep(1000);
         hpd_status = gpio_get_value(tc358840_s_ctrl->hpd_gpio);
         pr_err("__%s__The value of HPD = %d\n",__func__,hpd_status);
         tc358440_cci_read (tc358840_s_ctrl, (uint32_t) SYS_INTERRUPT_STATUS_REG, &sys_interrupt_status_register, H2C_DATA_8BIT);
         tc358440_cci_read (tc358840_s_ctrl, (uint32_t) SYSTEM_INTERRUPT, &system_interrupt, H2C_DATA_8BIT);
         tc358440_cci_read (tc358840_s_ctrl, (uint32_t) CLOCK_INTERRUPT, &clock_interrupt, H2C_DATA_8BIT);
         tc358440_cci_read (tc358840_s_ctrl, (uint32_t) H2C_SYS_STATUS_REG, &sys_status_register, H2C_DATA_8BIT);
         if(hpd_status){
             /*Hot Pulg Detection */
             pr_err ("Hot Pulg Detection-- Disconnected \n");
             tc358840_hdmi_sys_int_handler (tc358840_s_ctrl);
	     interrupt_request(tc358840_s_ctrl);
             }else if ((!((sys_interrupt_status_register & 0x01) && (system_interrupt & 0x01))) || (sys_status_register & 0x01)){
             pr_err ("Resolution Switch or FPS Change \n");
             disconnect = 0;
             tc358840_hdmi_clk_int_handler (tc358840_s_ctrl);
         }
	 pr_debug ("Exit-%s\n", __func__);
}

static irqreturn_t tc358840_irq_handler (int irq, void *dev_id)
{
         int ret = 0;
         pr_err("INTERRUPT REQUEST generated");
         ret = queue_work_on (0, isr_wq, &isr_work);
         return IRQ_RETVAL (1);
}

static void tc358840_stop_stream(struct msm_sensor_ctrl_t *s_ctrl)
{
	  tc358840_write_conf_tbl(s_ctrl, tc358840_stop_settings, sizeof (tc358840_stop_settings)/sizeof (tc358840_stop_settings[0]));
	  if (h2c_state == H2C_RESTARTING)
	    {				// sending resoluton switch start uevent for new resolution
	      CDBG("HDMI input is going to start with new resolution - sending start\n");
	      hdmi_in_send_uevent (HDMI_IN_START, HDMI_IN_RESOLUTION_SWITCH);
	      h2c_state = H2C_STARTING;
	    }
}

static void tc358840_start_stream(struct msm_sensor_ctrl_t *s_ctrl)
{
	uint16_t h_total_reg0, pixel_clk_freq_reg1;

	gBoot++;
	tc358840_s_ctrl=s_ctrl;

	hpd_status = gpio_get_value(tc358840_s_ctrl->hpd_gpio);
        pr_debug("__%s__The value of HPD = %d\n",__func__,hpd_status);
        if(hpd_status){
		hdmi_in_send_uevent(HDMI_IN_PORT_DISCONNECTED, HDMI_IN_UNKNOWN_REASON);
		h2c_state = H2C_STOPPED;
		vi_reg_val	= 0;
		horz_reg0 	= 0;
		horz_reg1 	= 0;
		goto disconnected;
	}
	if(tc35_resolution == 640){
		pr_err("%s: Writing VGA settings..\n",__func__);
		tc358840_write_conf_tbl(s_ctrl, tc358840_VGA_settings, sizeof(tc358840_VGA_settings)/sizeof(tc358840_VGA_settings[0]));
		vi_reg_val = 1;
		horz_reg1 = 02;
		horz_reg0 = 128;
	}else if(tc35_resolution == 720){
		pr_err("%s: Writing 720p settings..\n",__func__);
		tc358840_write_conf_tbl(s_ctrl, tc358840_720p_settings, sizeof(tc358840_720p_settings)/sizeof(tc358840_720p_settings[0]));
		vi_reg_val = 12;
		horz_reg1 = 05;
		horz_reg0 = 00;
	}else if(tc35_resolution == 1080){
		pr_err("%s: Writing 1080p settings..\n",__func__);
		tc358840_write_conf_tbl(s_ctrl, tc358840_1080p_settings, sizeof(tc358840_1080p_settings)/sizeof(tc358840_1080p_settings[0]));
		pr_err("%s: Writing 1080p settings done\n",__func__);
		vi_reg_val = 15;
		horz_reg1 = 07;
		horz_reg0 = 128;
	}else if (tc35_resolution == 3840 )/*(horz_lin_count_reg0 == 0x00 && horz_lin_count_reg1 == 0x0F)*/{
		pr_err("%s: Writing 4k UHD settings..\n",__func__);
		tc358840_write_conf_tbl(s_ctrl, tc358840_4K_UHD_split_settings, sizeof(tc358840_4K_UHD_split_settings)/sizeof(tc358840_4K_UHD_split_settings[0]));
		vi_reg_val = 0;
		horz_reg1 = 15;
		horz_reg0 = 0;
	}else if (tc35_resolution == 4096)/*(horz_lin_count_reg0 == 0x00 && horz_lin_count_reg1 == 0x10)*/{
		pr_err("%s: Writing 4k DCI settings..\n",__func__);
		tc358840_write_conf_tbl(s_ctrl, tc358840_4K_DCI_split_settings, sizeof(tc358840_4K_DCI_split_settings)/sizeof(tc358840_4K_DCI_split_settings[0]));
		vi_reg_val = 0;
		horz_reg1 = 16;
		horz_reg0 = 0;
	}

	if (tc35_resolution == 640 || tc35_resolution == 720 || tc35_resolution == 3840 || tc35_resolution == 1080 || tc35_resolution == 4096){
                h2c_state = H2C_STARTED;// Successfully started the streaming

		/* reading 2 different registers, it will be useful later to detect change in input signal*/
		tc358440_cci_read(tc358840_s_ctrl, (uint32_t)H2C_HTOTAL_REG0, &h_total_reg0, H2C_DATA_8BIT);
		tc358440_cci_read(tc358840_s_ctrl, (uint32_t)PIXEL_CLCK_FREQ_REG1, &pixel_clk_freq_reg1, H2C_DATA_8BIT);
		htotal_reg_val = h_total_reg0;
		pclk_reg_val = pixel_clk_freq_reg1;
		pr_err("%s: Started Input video streaming for supported resolution\n",__func__);
	} else {
		h2c_state = H2C_STOPPED; // Unsuccessfull, No input signal or in sleep
		unsupported = 1;
		hdmi_in_send_uevent(HDMI_IN_PORT_CONNECTED, HDMI_IN_SLEEP); // sending respective uevent to app
		pr_err("%s: Stopped Input video streaming - unsupported resolution\n",__func__);
	}

disconnected:
	CDBG("%s__in state\n",__func__);
}

int32_t tc358840_cci_write(struct msm_sensor_ctrl_t *s_ctrl,
					uint32_t addr, uint32_t data,
					uint16_t len)
{
	int32_t rc = -EFAULT;


	struct msm_camera_i2c_reg_setting conf_array;
	struct msm_camera_i2c_reg_array *reg_setting;

	mutex_lock(&tc358840_write_lock);

	switch (len) {
	case H2C_DATA_8BIT:
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write(
						s_ctrl->sensor_i2c_client, addr,
						data, MSM_CAMERA_I2C_BYTE_DATA);


		if (rc < 0)
			pr_err("%s: Line %d: error i2c write BYTE failed\n",
							__func__, __LINE__);



	break;

	case H2C_DATA_16BIT:
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write(
						s_ctrl->sensor_i2c_client, addr,
						data, MSM_CAMERA_I2C_WORD_DATA);

		if (rc < 0)
			pr_err("%s: Line %d: error i2c write WORD failed\n",
							__func__, __LINE__);


	break;

	case H2C_DATA_32BIT:
		conf_array.addr_type = MSM_CAMERA_I2C_WORD_ADDR;
		conf_array.data_type = MSM_CAMERA_I2C_WORD_DATA;   //MSM_CAMERA_I2C_WORD_DATA
		conf_array.size = 2;
		conf_array.delay = 10;

		reg_setting = kzalloc((sizeof(struct msm_camera_i2c_reg_array)
						* conf_array.size), GFP_KERNEL);
		if (!reg_setting) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -ENOMEM;
			break;
		}

		reg_setting->reg_addr = addr;
		reg_setting->reg_data = data;
		conf_array.reg_setting = reg_setting;
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write_table(
			s_ctrl->sensor_i2c_client, &conf_array);


		if (rc < 0)
			pr_err("%s: Line %d: error i2c write 32-bit failed\n",
							__func__, __LINE__);

		kfree(reg_setting);
	break;
	default:
		pr_err("%s: ERROR\n", __func__);
	break;
	}

	mutex_unlock(&tc358840_write_lock);

	return rc;
}

int tc358840_sensor_config32(struct msm_sensor_ctrl_t *s_ctrl,
	void __user *argp)
{
	struct sensorb_cfg_data32 *cdata = (struct sensorb_cfg_data32 *)argp;
	int32_t rc = 0;
	int32_t i = 0;
	static uint16_t h_total_reg0, v_total_reg0,pixel_clk_freq_reg0, pixel_clk_freq_reg1;
	static int init_retry, start_retry, stop_retry, retry, output_info_retry;
	const char *sensor_name;
	static int start_skip;

	sensor_name = s_ctrl->sensordata->sensor_name;
	mutex_lock(s_ctrl->msm_sensor_mutex);
	pr_debug("%s:%d %s cfgtype = %d\n", __func__, __LINE__,
		s_ctrl->sensordata->sensor_name, cdata->cfgtype);
	switch (cdata->cfgtype) {
	case CFG_GET_SENSOR_INFO:
		memcpy(cdata->cfg.sensor_info.sensor_name,
			s_ctrl->sensordata->sensor_name,
			sizeof(cdata->cfg.sensor_info.sensor_name));
		cdata->cfg.sensor_info.session_id =
			s_ctrl->sensordata->sensor_info->session_id;
		for (i = 0; i < SUB_MODULE_MAX; i++) {
			cdata->cfg.sensor_info.subdev_id[i] =
				s_ctrl->sensordata->sensor_info->subdev_id[i];
			cdata->cfg.sensor_info.subdev_intf[i] =
				s_ctrl->sensordata->sensor_info->subdev_intf[i];
		}
		cdata->cfg.sensor_info.is_mount_angle_valid =
			s_ctrl->sensordata->sensor_info->is_mount_angle_valid;
		cdata->cfg.sensor_info.sensor_mount_angle =
			s_ctrl->sensordata->sensor_info->sensor_mount_angle;
		cdata->cfg.sensor_info.position =
			s_ctrl->sensordata->sensor_info->position;
		cdata->cfg.sensor_info.modes_supported =
			s_ctrl->sensordata->sensor_info->modes_supported;
		pr_err("%s:%d sensor name %s\n", __func__, __LINE__,
			cdata->cfg.sensor_info.sensor_name);
		pr_err("%s:%d session id %d\n", __func__, __LINE__,
			cdata->cfg.sensor_info.session_id);
		for (i = 0; i < SUB_MODULE_MAX; i++) {
			pr_err("%s:%d subdev_id[%d] %d\n", __func__, __LINE__, i,
				cdata->cfg.sensor_info.subdev_id[i]);
			pr_err("%s:%d subdev_intf[%d] %d\n", __func__, __LINE__,
				i, cdata->cfg.sensor_info.subdev_intf[i]);
		}
		pr_err("%s:%d mount angle valid %d value %d\n", __func__,
			__LINE__, cdata->cfg.sensor_info.is_mount_angle_valid,
			cdata->cfg.sensor_info.sensor_mount_angle);

		break;
	case CFG_GET_SENSOR_INIT_PARAMS:
		cdata->cfg.sensor_init_params.modes_supported =
			s_ctrl->sensordata->sensor_info->modes_supported;
		cdata->cfg.sensor_init_params.position =
			s_ctrl->sensordata->sensor_info->position;
		cdata->cfg.sensor_init_params.sensor_mount_angle =
			s_ctrl->sensordata->sensor_info->sensor_mount_angle;
		pr_err("%s:%d init params mode %d pos %d mount %d\n", __func__,
			__LINE__,
			cdata->cfg.sensor_init_params.modes_supported,
			cdata->cfg.sensor_init_params.position,
			cdata->cfg.sensor_init_params.sensor_mount_angle);
		break;

	case CFG_WRITE_I2C_ARRAY:
	case CFG_WRITE_I2C_ARRAY_SYNC:
	case CFG_WRITE_I2C_ARRAY_SYNC_BLOCK:
	case CFG_WRITE_I2C_ARRAY_ASYNC: {
		pr_err("CFG_WRITE_I2C_ARRAY \n");

			mflag++;

		break;
	}
	case CFG_SET_START_STREAM:{
		pr_err("CFG_SET_START_STREAM \n");
		init_retry=0;
		stop_retry=0;
		if (tc35_resolution == 4096 || tc35_resolution == 3840){
			pr_err("Dual Splitting CSI enabled\n");
			if(start_retry==0){ // Skipping unwanted ioctl calls to avoid unexpected behaviour of system in the case of 4k
				start_retry++;
				 break;
			}
			start_retry=0;
			tc358840_start_stream(s_ctrl);
		} else { //when 1 CSI lane is enabled for 1080P and below
			pr_err("single CSI enabled\n");
			tc358840_start_stream(s_ctrl);
		}
		interrupt_request(s_ctrl);
		break;
	}
	case CFG_SLAVE_READ_I2C: {
		struct msm_camera_i2c_read_config read_config;
		struct msm_camera_i2c_read_config *read_config_ptr = NULL;
		uint16_t local_data = 0;
		uint16_t orig_slave_addr = 0, read_slave_addr = 0;
		uint16_t orig_addr_type = 0, read_addr_type = 0;

		if (s_ctrl->is_csid_tg_mode)
			goto DONE;

		read_config_ptr =
			(struct msm_camera_i2c_read_config *)
			compat_ptr(cdata->cfg.setting);

		if (copy_from_user(&read_config, read_config_ptr,
			sizeof(struct msm_camera_i2c_read_config))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}
		read_slave_addr = read_config.slave_addr;
		read_addr_type = read_config.addr_type;

		if (s_ctrl->sensor_i2c_client->cci_client) {
			orig_slave_addr =
				s_ctrl->sensor_i2c_client->cci_client->sid;
			s_ctrl->sensor_i2c_client->cci_client->sid =
				read_slave_addr >> 1;
		} else if (s_ctrl->sensor_i2c_client->client) {
			orig_slave_addr =
				s_ctrl->sensor_i2c_client->client->addr;
			s_ctrl->sensor_i2c_client->client->addr =
				read_slave_addr >> 1;
		} else {
			pr_err("%s: error: no i2c/cci client found.", __func__);
			rc = -EFAULT;
			break;
		}
		pr_err("%s:orig_slave_addr=0x%x, new_slave_addr=0x%x",
				__func__, orig_slave_addr,
				read_slave_addr >> 1);

		orig_addr_type = s_ctrl->sensor_i2c_client->addr_type;
		s_ctrl->sensor_i2c_client->addr_type = read_addr_type;

		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_read(
				s_ctrl->sensor_i2c_client,
				read_config.reg_addr,
				&local_data, read_config.data_type);
		if (s_ctrl->sensor_i2c_client->cci_client) {
			s_ctrl->sensor_i2c_client->cci_client->sid =
				orig_slave_addr;
		} else if (s_ctrl->sensor_i2c_client->client) {
			s_ctrl->sensor_i2c_client->client->addr =
				orig_slave_addr;
		}
		s_ctrl->sensor_i2c_client->addr_type = orig_addr_type;

		if (rc < 0) {
			pr_err("%s:%d: i2c_read failed\n", __func__, __LINE__);
			break;
		}
		read_config_ptr->data = local_data;
		break;
	}
	case CFG_SLAVE_WRITE_I2C_ARRAY: {
		struct msm_camera_i2c_array_write_config32 write_config32;
		struct msm_camera_i2c_array_write_config write_config;
		struct msm_camera_i2c_reg_array *reg_setting = NULL;
		uint16_t orig_slave_addr = 0, write_slave_addr = 0;
		uint16_t orig_addr_type = 0, write_addr_type = 0;

		if (s_ctrl->is_csid_tg_mode)
			goto DONE;

		if (copy_from_user(&write_config32,
				(void *)compat_ptr(cdata->cfg.setting),
				sizeof(
				struct msm_camera_i2c_array_write_config32))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}

		write_config.slave_addr = write_config32.slave_addr;
		write_config.conf_array.addr_type =
			write_config32.conf_array.addr_type;
		write_config.conf_array.data_type =
			write_config32.conf_array.data_type;
		write_config.conf_array.delay =
			write_config32.conf_array.delay;
		write_config.conf_array.size =
			write_config32.conf_array.size;
		write_config.conf_array.reg_setting =
			compat_ptr(write_config32.conf_array.reg_setting);


		if (!write_config.conf_array.size ||
			write_config.conf_array.size > I2C_REG_DATA_MAX) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}

		reg_setting = kzalloc(write_config.conf_array.size *
			(sizeof(struct msm_camera_i2c_reg_array)), GFP_KERNEL);
		if (!reg_setting) {
			rc = -ENOMEM;
			break;
		}
		if (copy_from_user(reg_setting,
				(void *)(write_config.conf_array.reg_setting),
				write_config.conf_array.size *
				sizeof(struct msm_camera_i2c_reg_array))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			kfree(reg_setting);
			rc = -EFAULT;
			break;
		}
		write_config.conf_array.reg_setting = reg_setting;
		write_slave_addr = write_config.slave_addr;
		write_addr_type = write_config.conf_array.addr_type;

		if (s_ctrl->sensor_i2c_client->cci_client) {
			orig_slave_addr =
				s_ctrl->sensor_i2c_client->cci_client->sid;
			s_ctrl->sensor_i2c_client->cci_client->sid =
				write_slave_addr >> 1;
		} else if (s_ctrl->sensor_i2c_client->client) {
			orig_slave_addr =
				s_ctrl->sensor_i2c_client->client->addr;
			s_ctrl->sensor_i2c_client->client->addr =
				write_slave_addr >> 1;
		} else {
			pr_err("%s: error: no i2c/cci client found.",
				__func__);
			kfree(reg_setting);
			rc = -EFAULT;
			break;
		}

		pr_err("%s:orig_slave_addr=0x%x, new_slave_addr=0x%x\n",
				__func__, orig_slave_addr,
				write_slave_addr >> 1);
		orig_addr_type = s_ctrl->sensor_i2c_client->addr_type;
		s_ctrl->sensor_i2c_client->addr_type = write_addr_type;
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write_table(
			s_ctrl->sensor_i2c_client, &(write_config.conf_array));

		s_ctrl->sensor_i2c_client->addr_type = orig_addr_type;
		if (s_ctrl->sensor_i2c_client->cci_client) {
			s_ctrl->sensor_i2c_client->cci_client->sid =
				orig_slave_addr;
		} else if (s_ctrl->sensor_i2c_client->client) {
			s_ctrl->sensor_i2c_client->client->addr =
				orig_slave_addr;
		} else {
			pr_err("%s: error: no i2c/cci client found.\n",
				__func__);
			kfree(reg_setting);
			rc = -EFAULT;
			break;
		}
		kfree(reg_setting);
		break;
	}
	case CFG_WRITE_I2C_SEQ_ARRAY: {
		struct msm_camera_i2c_seq_reg_setting32 conf_array32;
		struct msm_camera_i2c_seq_reg_setting conf_array;
		struct msm_camera_i2c_seq_reg_array *reg_setting = NULL;

		if (s_ctrl->is_csid_tg_mode)
			goto DONE;

		if (s_ctrl->sensor_state != MSM_SENSOR_POWER_UP) {
			pr_err("%s:%d failed: invalid state %d\n", __func__,
				__LINE__, s_ctrl->sensor_state);
			rc = -EFAULT;
			break;
		}

		if (copy_from_user(&conf_array32,
			(void *)compat_ptr(cdata->cfg.setting),
			sizeof(struct msm_camera_i2c_seq_reg_setting32))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}

		conf_array.addr_type = conf_array32.addr_type;
		conf_array.delay = conf_array32.delay;
		conf_array.size = conf_array32.size;
		conf_array.reg_setting = compat_ptr(conf_array32.reg_setting);

		if (!conf_array.size ||
			conf_array.size > I2C_SEQ_REG_DATA_MAX) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}

		reg_setting = kzalloc(conf_array.size *
			(sizeof(struct msm_camera_i2c_seq_reg_array)),
			GFP_KERNEL);
		if (!reg_setting) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -ENOMEM;
			break;
		}
		if (copy_from_user(reg_setting, (void *)conf_array.reg_setting,
			conf_array.size *
			sizeof(struct msm_camera_i2c_seq_reg_array))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			kfree(reg_setting);
			rc = -EFAULT;
			break;
		}

		conf_array.reg_setting = reg_setting;
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
			i2c_write_seq_table(s_ctrl->sensor_i2c_client,
			&conf_array);
		kfree(reg_setting);
		break;
	}



	case CFG_POWER_UP:{
		pr_err("CFG_POWER_UP\n");
		start_skip = 0;
		if(power_up_skip == 1){
			break;
		}
		if (s_ctrl->is_csid_tg_mode)
			goto DONE;

		if (s_ctrl->sensor_state != MSM_SENSOR_POWER_DOWN) {
			pr_err("%s:%d failed: invalid state %d\n", __func__,
			__LINE__, s_ctrl->sensor_state);
			rc = -EFAULT;
			break;
		}

		rc = msm_sensor_power_up(s_ctrl);
		if (rc < 0) {
			pr_err("%s:%d failed rc %d\n", __func__,__LINE__, rc);
		} else {
			s_ctrl->sensor_state = MSM_SENSOR_POWER_UP;
		}

		pr_err("%s:%d sensor state %d\n", __func__, __LINE__,s_ctrl->sensor_state);
		mflag = 0;

		}
		power_up_skip++;
		break;

	case CFG_POWER_DOWN:{
		int cnt = 0;
		free_irq(s_ctrl->irq_gpio, s_ctrl);
		if(!strcmp(sensor_name,"tc358840")) {
			for(cnt=0;cnt<5;cnt++){
		               tc358440_cci_read(s_ctrl, (uint32_t)H2C_VI_STATUS_REG, &vi_status_reg, H2C_DATA_8BIT);
		               camera_resolution=vi_status_reg;
		               msleep(100);
		        }
		}

		init_retry=1;
		if (s_ctrl->is_csid_tg_mode)
			goto DONE;

		if (s_ctrl->sensor_state != MSM_SENSOR_POWER_UP) {
			pr_err("%s:%d failed: invalid state %d\n", __func__,
			__LINE__, s_ctrl->sensor_state);
			rc = -EFAULT;
			break;
		}

		pr_err("%s: poewering down \n",__func__);
		vi_status_reg = horz_lin_count_reg0 = horz_lin_count_reg1 = 0;

		//rc =msm_sensor_power_down(s_ctrl);

	        hpd_status = gpio_get_value(s_ctrl->hpd_gpio);
	        if(hpd_status){
			hdmi_in_send_uevent(HDMI_IN_PORT_DISCONNECTED, HDMI_IN_UNKNOWN_REASON);
			h2c_state = H2C_STOPPED;
	        }
		if (rc < 0) {
			pr_err("%s:%d failed rc %d\n", __func__,
			__LINE__, rc);
		} else {
			s_ctrl->sensor_state = MSM_SENSOR_POWER_DOWN;
		}
		pr_err("%s:%d sensor state %d\n", __func__, __LINE__,
		s_ctrl->sensor_state);
		mflag = 0;
		}
		break;
	case CFG_SET_INIT_SETTING: {
		pr_err("%s:%d CFG_SET_INIT_SETTING\n", __func__, __LINE__);
		start_retry=0;//Initialising start_retry variable used in case CFG_SET_START_STREAM to avoid accidental corruption of value
		if(!strcmp(s_ctrl->sensordata->sensor_name,"tc3588401")) //To avoid doing power up for second dummy sensor in case of tc358840
		{
			pr_err("It is tc3588401, so skip writing init settings \n");
			break;
		}

		pr_err("init retry value :%d \n",init_retry);
		if (init_retry==0){
			pr_err("Skipping ioctl call\n");
			init_retry++;
			break;
		}
		init_retry=0;
		hpd_status = gpio_get_value(s_ctrl->hpd_gpio);

                if(hpd_status){
                        hdmi_in_send_uevent(HDMI_IN_PORT_DISCONNECTED, HDMI_IN_UNKNOWN_REASON);
                        h2c_state = H2C_STOPPED;
                }
		if(flag_edid == 1){
			pr_err("writing INIT setting with EDID Disconnected state\n");
			disconnect =0;
			tc358840_write_conf_tbl(s_ctrl, tc358840_init_settings, sizeof(tc358840_init_settings)/sizeof(tc358840_init_settings[0]));
		}
		else {
			pr_err("writing INIT setting without EDID only for resolution change\n");
			tc358840_write_conf_tbl(s_ctrl, tc358840_init_settings_without_EDID, sizeof(tc358840_init_settings_without_EDID)/sizeof(tc358840_init_settings_without_EDID[0]));
		}
	        if(hpd_status)
                        break;
                if(!have_supported_resolution){
		        msleep(3500);
		        for (retry=0; retry<50; retry++) {
				msleep(10);
				tc358440_cci_read(s_ctrl, (uint32_t)H2C_VI_STATUS_REG, &vi_status_reg, H2C_DATA_8BIT);
				tc358440_cci_read(s_ctrl, (uint32_t)HORIZONTAL_LINE_COUNT_REG0, &horz_lin_count_reg0, H2C_DATA_8BIT);
				tc358440_cci_read(s_ctrl, (uint32_t)HORIZONTAL_LINE_COUNT_REG1, &horz_lin_count_reg1, H2C_DATA_8BIT);
			}
		}
		pr_err ("H2C_VI_STATUS_REG- %d\n",vi_status_reg);
		pr_err ("HORIZONTAL_LINE_COUNT_REG0- %d\n",horz_lin_count_reg0);
		pr_err ("HORIZONTAL_LINE_COUNT_REG1- %d\n",horz_lin_count_reg1);
		tc358840_s_ctrl = s_ctrl;
		}
		have_supported_resolution = 0;
		break;
	case CFG_GET_OUTPUT_INFO: {
			static int in_fps;
			int fps_try;
			pr_err("CFG_GET_OUTPUT_INFO\n");
			if(!strcmp(s_ctrl->sensordata->sensor_name,"tc3588401")) //To avoid doing power up for second dummy sensor in case of tc358840
			{
				pr_err("[%s]It is tc3588401, so skip CFG_GET_OUTPUT_INFO \n",__func__);
				break;
			}
			output_info_retry++;
			hpd_status = gpio_get_value(s_ctrl->hpd_gpio);
			if (vi_status_reg == 0x0F){
				pr_err("resolution to user-space is 0x%d\n",vi_status_reg);
				cdata->input_width = 1920;// copy this info to mct_pipeline to re-initialize Camera Pipeline
				cdata->input_height = 1080;
				tc35_resolution = 1080;
				break;
			}
			else if (vi_status_reg == 0x0C){
				pr_err("resolution to user-space is 0x%d\n",vi_status_reg);
				cdata->input_width = 1280;
				cdata->input_height = 720;
				tc35_resolution = 720;
				break;
			}
			else if (vi_status_reg == 0x01){
				pr_err("resolution to user-space is 0x%d\n",vi_status_reg);
				cdata->input_width = 640;
				cdata->input_height = 480;
				tc35_resolution = 640;
				break;
			}
			else if (vi_status_reg == 0x00 && horz_lin_count_reg0 == 0x00 && horz_lin_count_reg1 == 0x0F ){
				pr_err("resolution to user-space is 4K UHD");
				cdata->input_width = 3840;
				cdata->input_height = 2160;
				tc35_resolution = 3840;
				break;
			}
			else if (vi_status_reg == 0x00 && horz_lin_count_reg0 == 0x00 && horz_lin_count_reg1 == 0x10 ){
				pr_err("resolution to user-space is 4K DCI");
				cdata->input_width = 4096;
				cdata->input_height = 2160;
				tc35_resolution = 4096;
				break;
			}else {
				pr_err("No Input..Check the input !!!"); // No input signal or source is in sleep
				cdata->input_width = 640; // Default Dummy value
				cdata->input_height = 480;
				tc35_resolution = 0;
				hdmi_in_send_uevent(HDMI_IN_PORT_CONNECTED, HDMI_IN_SLEEP); // sending respective uevent to app
				pr_err("%s:Unsupported resolution\n",__func__);
				if(hpd_status){
					cdata->input_width = 640; // Default Dummy value
					cdata->input_height = 480;
					tc35_resolution = 0;
					break;
				}
				if (output_info_retry > 2)
				{
					msleep(500);
				}
			}
			for(fps_try = 0; fps_try < 100; fps_try ++)
			{
				msleep(10);
				tc358440_cci_read(s_ctrl, (uint32_t)H2C_HTOTAL_REG0, &h_total_reg0, H2C_DATA_16BIT);
				tc358440_cci_read(s_ctrl, (uint32_t)H2C_VTOTAL_REG0, &v_total_reg0, H2C_DATA_16BIT);
				tc358440_cci_read(s_ctrl, (uint32_t)PIXEL_CLCK_FREQ_REG0, &pixel_clk_freq_reg0, H2C_DATA_8BIT);
				tc358440_cci_read(s_ctrl, (uint32_t)PIXEL_CLCK_FREQ_REG1, &pixel_clk_freq_reg1, H2C_DATA_8BIT);

				h_total_reg0=((h_total_reg0<<8)&(0xFF00))|((h_total_reg0>>8)&(0x00FF));
				v_total_reg0=((v_total_reg0<<8)&(0xFF00))|((v_total_reg0>>8)&(0x00FF));

				in_fps = ((((pixel_clk_freq_reg1 & 0x1FFF) << 8) | pixel_clk_freq_reg0) * 80000)/((h_total_reg0 & 0x1FFF) * ((v_total_reg0 & 0x3FFF) >> 1));
				if(pixel_clk_freq_reg0 && pixel_clk_freq_reg1)
					break;
			}
			pr_err("FPS ->%d\n",in_fps + 1);
			cdata->fps = in_fps + 1;
			break;
		}
	case CFG_SET_STOP_STREAM_SETTING: {
		struct msm_camera_i2c_reg_setting32 stop_setting32;
		struct msm_camera_i2c_reg_setting *stop_setting =
			&s_ctrl->stop_setting;
		struct msm_camera_i2c_reg_array *reg_setting = NULL;

		if (s_ctrl->is_csid_tg_mode)
			goto DONE;

		if (copy_from_user(&stop_setting32,
				(void *)compat_ptr((cdata->cfg.setting)),
			sizeof(struct msm_camera_i2c_reg_setting32))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}

		stop_setting->addr_type = stop_setting32.addr_type;
		stop_setting->data_type = stop_setting32.data_type;
		stop_setting->delay = stop_setting32.delay;
		stop_setting->size = stop_setting32.size;

		reg_setting = compat_ptr(stop_setting32.reg_setting);

		if (!stop_setting->size) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}
		stop_setting->reg_setting = kzalloc(stop_setting->size *
			(sizeof(struct msm_camera_i2c_reg_array)), GFP_KERNEL);
		if (!stop_setting->reg_setting) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -ENOMEM;
			break;
		}
		if (copy_from_user(stop_setting->reg_setting,
			(void *)reg_setting,
			stop_setting->size *
			sizeof(struct msm_camera_i2c_reg_array))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			kfree(stop_setting->reg_setting);
			stop_setting->reg_setting = NULL;
			stop_setting->size = 0;
			rc = -EFAULT;
			break;
		}
		break;
	}
	case CFG_SET_STOP_STREAM:{
		init_retry=1;
		start_retry=0;
		pr_err("CFG_SET_STOP_STREAM\n");

		if(!strcmp(sensor_name,"tc3588401"))
		{
			break;
		}
		if(stop_skip == 1 ){
			pr_err("Skipping ioctl: CFG_SET_STOP_STREAM call\n");
			stop_skip=0;
			break;
		}
		if ((vi_status_reg == 0x00 && horz_lin_count_reg0 == 0x00 && horz_lin_count_reg1 == 0x0F) || (vi_status_reg == 0x00 &&horz_lin_count_reg0 == 0x00 && horz_lin_count_reg1 == 0x10)){
			pr_err("Dual Splitting CSI disabled\n");
			if(stop_retry==1){
				stop_retry=0;
				pr_err("Skipping ioctl call\n");
				break;
			}
			stop_retry++;
		} else {
			pr_err("single CSI disabled\n");
		}

		tc358840_write_conf_tbl(s_ctrl, tc358840_stop_settings, sizeof(tc358840_stop_settings)/sizeof(tc358840_stop_settings[0]));
		tc358840_write_conf_tbl (tc358840_s_ctrl, tc358840_interrupt_reset, sizeof (tc358840_interrupt_reset) / sizeof (tc358840_interrupt_reset[0]));
		tc358840_stop_stream(s_ctrl);
		stop_skip++;

		break;
	}
	case CFG_SET_I2C_SYNC_PARAM: {
		struct msm_camera_cci_ctrl cci_ctrl;

		s_ctrl->sensor_i2c_client->cci_client->cid =
			cdata->cfg.sensor_i2c_sync_params.cid;
		s_ctrl->sensor_i2c_client->cci_client->id_map =
			cdata->cfg.sensor_i2c_sync_params.csid;

		pr_err("I2C_SYNC_PARAM CID:%d, line:%d delay:%d, cdid:%d\n",
			s_ctrl->sensor_i2c_client->cci_client->cid,
			cdata->cfg.sensor_i2c_sync_params.line,
			cdata->cfg.sensor_i2c_sync_params.delay,
			cdata->cfg.sensor_i2c_sync_params.csid);

		cci_ctrl.cmd = MSM_CCI_SET_SYNC_CID;
		cci_ctrl.cfg.cci_wait_sync_cfg.line =
			cdata->cfg.sensor_i2c_sync_params.line;
		cci_ctrl.cfg.cci_wait_sync_cfg.delay =
			cdata->cfg.sensor_i2c_sync_params.delay;
		cci_ctrl.cfg.cci_wait_sync_cfg.cid =
			cdata->cfg.sensor_i2c_sync_params.cid;
		cci_ctrl.cfg.cci_wait_sync_cfg.csid =
			cdata->cfg.sensor_i2c_sync_params.csid;
		rc = v4l2_subdev_call(s_ctrl->sensor_i2c_client->
				cci_client->cci_subdev,
				core, ioctl, VIDIOC_MSM_CCI_CFG, &cci_ctrl);
		if (rc < 0) {
			pr_err("%s: line %d rc = %d\n", __func__, __LINE__, rc);
			rc = -EFAULT;
			break;
		}
		break;
	}

	default:
		rc = -EFAULT;
		break;
	}

DONE:
	mutex_unlock(s_ctrl->msm_sensor_mutex);

	return rc;
}

MODULE_DESCRIPTION("Toshiba HDMI to CSI2 4K bridge TC358840XBG");
MODULE_LICENSE("GPL v2");
MODULE_VERSION(TC358840_V4L2_DRIVER_VERSION);

/*
 * Overrides for Emacs so that we almost follow Linus's tabbing style.
 * Emacs will notice this stuff at the end of the file and automatically
 * adjust the settings for this buffer only.  This must remain at the end
 * of the file.
 * ---------------------------------------------------------------------------
 * Local variables:
 * c-basic-offset: 8
 * tab-width: 8
 * End:
 */
