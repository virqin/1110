/*============================================================================

                 Camera Interface Device Driver Source File for 
                    GC0329 VGA CMOS sensor
                    added by yangty LONGSHANG

   DESCRIPTION
     This file contains the definitions needed for the camera interface
     device driver.

   Copyright (c) 2006 by LCT Driver Team.  All Rights Reserved.
============================================================================*/

/*============================================================================

                      EDIT HISTORY FOR FILE

 This section contains comments describing changes made to this file.
 Notice that changes are listed in reverse chronological order.

$Header: //depot/asic/msm6550/drivers/camsensor/camsensor_ SIV121A_ycbcr.c#3 $ $DateTime: 2006/08/11 14:02:49 $ $Author: klee $

 when      who    what, where, why
 --------  -----  ----------------------------------------------------------
  09/30/06  lee    Initial release.
 
============================================================================*/

/*============================================================================
                        INCLUDE FILES
============================================================================*/

#include "camsensor.h"
#if defined (FEATURE_CAMERA) && defined (USE_CAMSENSOR_GC0329)
#include "camerai.h"

#include "camsensor_gc0329.h"
#include "clk.h"


/*============================================================================
                        CONSTANT DEFINITIONS
============================================================================*/

#define CAMSENSOR_GC0329_I2C_SLAVE_ID              0x62 ///SIV121A

#define CAMSENSOR_GC0329_SENSOR_ID              0xC0 ///SIV121A
#ifndef FEATURE_VERSION_K212
#ifndef FEATURE_VERSION_K212_12832

#define CAMSENSOR_GC0329_RESET_PIN         GPIO_OUTPUT_10
#endif
#endif
/* 
 * Maximum number of trials before aborting a register write operation
 *
 */
#define MAX_REGISTER_WRITE_TRIALS   3
#define MAX_REGISTER_READ_TRIALS    3
/*begin\bug6192\yangtingyu\20090226\camera驱动更新，实现亮度调节*/
#define GC0329_MAX_BRIGHTNESS  9
#define GC0329_MIN_BRIGHTNESS  0
/*end\bug6192\yangtingyu\20090226\camera驱动更新，实现亮度调节*/

#define GC0329_MAX_CONTRAST   4
#define GC0329_MIN_CONTRAST   0

/* From the logic analyzer measurements */
#define OV_GC0329_YCBCR_FULL_SIZE_WIDTH           640//640
#define OV_GC0329_YCBCR_FULL_SIZE_HEIGHT          480

#define OV_GC0329_YCBCR_QTR_SIZE_WIDTH   		640//640   
#define OV_GC0329_YCBCR_QTR_SIZE_HEIGHT  		480//240//   

/* Strobe Flash Epoch Interrupt time before the end of line count */
#define OV_SIV121A_YCBCR_EPOCH_LINES_DELAY       1

/*===========================================================================
                          MACRO DEFINITIONS
============================================================================*/


/*============================================================================
                        STATIC VARIABLES 
============================================================================*/
// modified Longcheer3G_liudan_20070403.........................................
const char camsensor_gc0329_ycbcr_sensor_name[]  = "GC0329 VGA";

// modified end.................................................................
/*============================================================================
                          INTERNAL API PROTOTYPES
============================================================================*/
static boolean camsensor_gc0329_ycbcr_i2c_write_byte(uint8 reg, uint8 data);
static void camsensor_gc0329_ycbcr_register(camsensor_function_table_type *camsensor_function_table_ptr);
static boolean camsensor_gc0329_ycbcr_i2c_read_byte(uint8 reg, uint8 *data); 
static boolean camsensor_gc0329_ycbcr_start(camsensor_static_params_type *camsensor_params );
static boolean camsensor_gc0329_ycbcr_video_config( camsensor_static_params_type *camsensor_params /* Other config params */ );
static void camsensor_gc0329_ycbcr_power_up(void);
static boolean camsensor_gc0329_ycbcr_snapshot_config( 	camsensor_static_params_type *camsensor_params /* Other config params */);
static camera_ret_code_type camsensor_gc0329_set_effect(int8 effect);
static camera_ret_code_type camsensor_gc0329_set_wb(int8 wb);
static camera_ret_code_type camsensor_gc0329_set_contrast(int8 contrast);
static camera_ret_code_type camsensor_gc0329_set_antibanding(int8 antibanding);
static camera_ret_code_type camsensor_gc0329_set_brightness(int8 value);
static void    camsensor_gc0329_0m3_setup_camctrl_tbl(camctrl_tbl_type *camctrl_tbl_ptr);
/*============================================================================
                          EXTERNAL API DEFINITIONS
============================================================================*/
static  boolean camsensor_gc0329_sensor_init(void)
{
	MSG_FATAL("-----camsensor_gc0329_sensor_init-----enter",0,0,0);

	camsensor_gc0329_ycbcr_i2c_write_byte(0xfe, 0x80);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xfc, 0x12); //[4]Clock_en [2] A25_en [1]D18_en [0]Apwdn
	camsensor_gc0329_ycbcr_i2c_write_byte(0xfc, 0x12); 
	camsensor_gc0329_ycbcr_i2c_write_byte(0xfe, 0x00);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xf0, 0x07); //vsync_en
	camsensor_gc0329_ycbcr_i2c_write_byte(0xf1, 0x01); //data_en
	                               
	camsensor_gc0329_ycbcr_i2c_write_byte(0x73, 0x90); //98//R channle gain
	camsensor_gc0329_ycbcr_i2c_write_byte(0x74, 0x80); //G1 channle gain
	camsensor_gc0329_ycbcr_i2c_write_byte(0x75, 0x80); //G2 channle gain
	camsensor_gc0329_ycbcr_i2c_write_byte(0x76, 0x94); //88//B channle gain
	                               
	camsensor_gc0329_ycbcr_i2c_write_byte(0x42, 0x00);
	camsensor_gc0329_ycbcr_i2c_write_byte(0x77, 0x57);
	camsensor_gc0329_ycbcr_i2c_write_byte(0x78, 0x4d);
	camsensor_gc0329_ycbcr_i2c_write_byte(0x79, 0x45);
	camsensor_gc0329_ycbcr_i2c_write_byte(0x42, 0xfc);

	////////////////////analog////////////////////

	camsensor_gc0329_ycbcr_i2c_write_byte(0xfc, 0x16); //
	                              
	camsensor_gc0329_ycbcr_i2c_write_byte(0x0a, 0x04); //row_start_low
	camsensor_gc0329_ycbcr_i2c_write_byte(0x0c, 0x04); //col_start_low
	camsensor_gc0329_ycbcr_i2c_write_byte(0x17, 0x14); //cisctl_mode1//[7]hsync_always ,[6] NA,[5:4] CFA sequence [3:2]NA,[1]upside_down,[0] mirror 
	camsensor_gc0329_ycbcr_i2c_write_byte(0x19, 0x05); //cisctl_mode3
	camsensor_gc0329_ycbcr_i2c_write_byte(0x1b, 0x24); //04//44//rsh_width
	camsensor_gc0329_ycbcr_i2c_write_byte(0x1c, 0x04); // 1d//Tsp_width
	camsensor_gc0329_ycbcr_i2c_write_byte(0x1e, 0x00); //Analog_mode1//[7:6]rsv1,rsv0[5:3] Column bias(coln_r)[1] clk_delay_en
	camsensor_gc0329_ycbcr_i2c_write_byte(0x1f, 0xc0); //Analog_mode2//[7:6] comv_r
	camsensor_gc0329_ycbcr_i2c_write_byte(0x20, 0x00); //Analog_mode3//[6:4] cap_low_r for MPW [3:2] da18_r [1] rowclk_mode [0]adclk_mode
	camsensor_gc0329_ycbcr_i2c_write_byte(0x21, 0x48); //Hrst_rsg//[7] hrst[6:4] da_rsg[3]txhigh_en
	camsensor_gc0329_ycbcr_i2c_write_byte(0x23, 0x22); //ADC_r//[6:5]opa_r [1:0]sRef
	camsensor_gc0329_ycbcr_i2c_write_byte(0x24, 0x16); //PAD_drv//[7:6]NA,[5:4]sync_drv [3:2]data_drv [1:0]pclk_drv
	                              
	//==================================================
	////////////////////blk////////////////////
	camsensor_gc0329_ycbcr_i2c_write_byte(0x26, 0xf7); 
	camsensor_gc0329_ycbcr_i2c_write_byte(0x32, 0x04);
	camsensor_gc0329_ycbcr_i2c_write_byte(0x33, 0x20);
	camsensor_gc0329_ycbcr_i2c_write_byte(0x34, 0x20);
	camsensor_gc0329_ycbcr_i2c_write_byte(0x35, 0x20);
	camsensor_gc0329_ycbcr_i2c_write_byte(0x36, 0x20);
	                               
	////////////////////ISP BLOCK ENABLE////////////////////
	camsensor_gc0329_ycbcr_i2c_write_byte(0x40, 0xff); //fe //ff
	camsensor_gc0329_ycbcr_i2c_write_byte(0x41, 0x00);
	camsensor_gc0329_ycbcr_i2c_write_byte(0x42, 0xfe); //
	camsensor_gc0329_ycbcr_i2c_write_byte(0x46, 0x03); //sync mode_ 0x02
	camsensor_gc0329_ycbcr_i2c_write_byte(0x4b, 0xcb);
	camsensor_gc0329_ycbcr_i2c_write_byte(0x4d, 0x01); //[1]In_buf
	camsensor_gc0329_ycbcr_i2c_write_byte(0x4f, 0x01);
	camsensor_gc0329_ycbcr_i2c_write_byte(0x70, 0x48); //global gain 0x40, 0x1X
	                              
	//camsensor_gc0329_ycbcr_i2c_write_byte(0xb0, 0x00); 
	//camsensor_gc0329_ycbcr_i2c_write_byte(0xbc, 0x00); 
	//camsensor_gc0329_ycbcr_i2c_write_byte(0xbd, 0x00); 
	//camsensor_gc0329_ycbcr_i2c_write_byte(0xbe, 0x00);

	////////////////////DNDD////////////////////
	camsensor_gc0329_ycbcr_i2c_write_byte(0x80, 0xe7); //87 //[7]auto_en [6]one_pixel [5]two_pixel
	camsensor_gc0329_ycbcr_i2c_write_byte(0x82, 0x55); // DN_inc
	camsensor_gc0329_ycbcr_i2c_write_byte(0x87, 0x4a); //
	                               
	////////////////////ASDE////////////////////
	camsensor_gc0329_ycbcr_i2c_write_byte(0xfe, 0x01);
	camsensor_gc0329_ycbcr_i2c_write_byte(0x18, 0x22); //[7:4]AWB LUMA X, 0x[3:0]ASDE LUMA X
	camsensor_gc0329_ycbcr_i2c_write_byte(0xfe, 0x00);
	camsensor_gc0329_ycbcr_i2c_write_byte(0x9c, 0x0a); //ASDE dn b slope
	camsensor_gc0329_ycbcr_i2c_write_byte(0xa4, 0x50); //40 //90// Auto Sa slope
	camsensor_gc0329_ycbcr_i2c_write_byte(0xa5, 0x21); // [7:4]Saturation limit x10
	camsensor_gc0329_ycbcr_i2c_write_byte(0xa7, 0x35); //low luma value th
	camsensor_gc0329_ycbcr_i2c_write_byte(0xdd, 0x54); //44//edge dec sat enable & slopes
	camsensor_gc0329_ycbcr_i2c_write_byte(0x95, 0x35); //Edge effect 
	                               
	////////////////////RGB gamma////////////////////
	//s_gamma                         
	camsensor_gc0329_ycbcr_i2c_write_byte(0xbf, 0x08);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xc0, 0x13);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xc1, 0x1f);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xc2, 0x31);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xc3, 0x48);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xc4, 0x5f);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xc5, 0x71);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xc6, 0x90);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xc7, 0xa9);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xc8, 0xbc);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xc9, 0xca);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xca, 0xd7);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xcb, 0xe0);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xcc, 0xe8);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xcd, 0xf4);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xce, 0xfb);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xcf, 0xff);
	                               
	//////////////////CC///////////////////
	camsensor_gc0329_ycbcr_i2c_write_byte(0xfe, 0x00);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xb3, 0x3d);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xb4, 0xfd);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xb5, 0x02);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xb6, 0xfa);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xb7, 0x40);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xb8, 0xf0);

	//skin
	//camsensor_gc0329_ycbcr_i2c_write_byte(0xfe, 0x00);
	//camsensor_gc0329_ycbcr_i2c_write_byte(0xb3, 0x3c);
	//camsensor_gc0329_ycbcr_i2c_write_byte(0xb4, 0xFF);
	//camsensor_gc0329_ycbcr_i2c_write_byte(0xb5, 0x03);
	//camsensor_gc0329_ycbcr_i2c_write_byte(0xb6, 0x01);
	//camsensor_gc0329_ycbcr_i2c_write_byte(0xb7, 0x3f);
	//camsensor_gc0329_ycbcr_i2c_write_byte(0xb8, 0xF3);

	////default CC
	//camsensor_gc0329_ycbcr_i2c_write_byte(0xfe, 0x00);
	//camsensor_gc0329_ycbcr_i2c_write_byte(0xb3, 0x45);
	//camsensor_gc0329_ycbcr_i2c_write_byte(0xb4, 0x00);
	//camsensor_gc0329_ycbcr_i2c_write_byte(0xb5, 0x00);
	//camsensor_gc0329_ycbcr_i2c_write_byte(0xb6, 0x00);
	//camsensor_gc0329_ycbcr_i2c_write_byte(0xb7, 0x45);
	//camsensor_gc0329_ycbcr_i2c_write_byte(0xb8, 0xF0);

	// crop 
	camsensor_gc0329_ycbcr_i2c_write_byte(0x50, 0x01);
	camsensor_gc0329_ycbcr_i2c_write_byte(0x19, 0x05);
	camsensor_gc0329_ycbcr_i2c_write_byte(0x20, 0x01);
	camsensor_gc0329_ycbcr_i2c_write_byte(0x22, 0xba);
	camsensor_gc0329_ycbcr_i2c_write_byte(0x21, 0x48);

	////////////////////YCP////////////////////
	camsensor_gc0329_ycbcr_i2c_write_byte(0xfe, 0x00); 
	camsensor_gc0329_ycbcr_i2c_write_byte(0xd1, 0x34); //38//saturation Cb
	camsensor_gc0329_ycbcr_i2c_write_byte(0xd2, 0x34); //38//saturation Cr

	////////////////////AEC////////////////////
	camsensor_gc0329_ycbcr_i2c_write_byte(0xfe, 0x01);
	camsensor_gc0329_ycbcr_i2c_write_byte(0x10, 0x40);
	camsensor_gc0329_ycbcr_i2c_write_byte(0x11, 0x21); //a1
	camsensor_gc0329_ycbcr_i2c_write_byte(0x12, 0x07); //17 //27 center weight un
	camsensor_gc0329_ycbcr_i2c_write_byte(0x13, 0x50); //Y target
	camsensor_gc0329_ycbcr_i2c_write_byte(0x17, 0x88); //AEC ignore mode
	camsensor_gc0329_ycbcr_i2c_write_byte(0x21, 0xb0);
	camsensor_gc0329_ycbcr_i2c_write_byte(0x22, 0x48);
	camsensor_gc0329_ycbcr_i2c_write_byte(0x3c, 0x95);
	camsensor_gc0329_ycbcr_i2c_write_byte(0x3d, 0x50);
	camsensor_gc0329_ycbcr_i2c_write_byte(0x3e, 0x48);

	////////////////////AWB////////////////////
	camsensor_gc0329_ycbcr_i2c_write_byte(0xfe, 0x01);
	camsensor_gc0329_ycbcr_i2c_write_byte(0x06, 0x16);
	camsensor_gc0329_ycbcr_i2c_write_byte(0x07, 0x06);
	camsensor_gc0329_ycbcr_i2c_write_byte(0x08, 0x98);
	camsensor_gc0329_ycbcr_i2c_write_byte(0x09, 0xee);
	camsensor_gc0329_ycbcr_i2c_write_byte(0x50, 0xfc); //RGB high
	camsensor_gc0329_ycbcr_i2c_write_byte(0x51, 0x20); //Y2C diff
	camsensor_gc0329_ycbcr_i2c_write_byte(0x53, 0x08); //20 
	camsensor_gc0329_ycbcr_i2c_write_byte(0x54, 0x20); //30//C inter
	camsensor_gc0329_ycbcr_i2c_write_byte(0x55, 0x16);
	camsensor_gc0329_ycbcr_i2c_write_byte(0x56, 0x10);
	//camsensor_gc0329_ycbcr_i2c_write_byte(0x57, 0x40);
	camsensor_gc0329_ycbcr_i2c_write_byte(0x58, 0x70); //number limit, 0xX4
	camsensor_gc0329_ycbcr_i2c_write_byte(0x59, 0x08); //AWB adjust temp curve
	camsensor_gc0329_ycbcr_i2c_write_byte(0x5a, 0x03); //25//[3:0]light gain range x10
	camsensor_gc0329_ycbcr_i2c_write_byte(0x5b, 0x63); //62
	camsensor_gc0329_ycbcr_i2c_write_byte(0x5c, 0x33); //37 //show and mode [2]big C mode [1]dark mode [0] block move mode
	camsensor_gc0329_ycbcr_i2c_write_byte(0x5d, 0x73); //52//AWB margin
	camsensor_gc0329_ycbcr_i2c_write_byte(0x5e, 0x11); //19//temp curve_enable
	camsensor_gc0329_ycbcr_i2c_write_byte(0x5f, 0x40); //5K gain
	camsensor_gc0329_ycbcr_i2c_write_byte(0x60, 0x40); //5K gain
	camsensor_gc0329_ycbcr_i2c_write_byte(0x61, 0xc8); //sinT
	camsensor_gc0329_ycbcr_i2c_write_byte(0x62, 0xa0); //cosT
	camsensor_gc0329_ycbcr_i2c_write_byte(0x63, 0x40); //30//AWB X1 cut
	camsensor_gc0329_ycbcr_i2c_write_byte(0x64, 0x50); //60//AWB X2 cut
	camsensor_gc0329_ycbcr_i2c_write_byte(0x65, 0x98); //a0//AWB Y1 cut
	camsensor_gc0329_ycbcr_i2c_write_byte(0x66, 0xfa); //ea//AWB Y2 cut
	camsensor_gc0329_ycbcr_i2c_write_byte(0x67, 0x70); //AWB R gain limit
	camsensor_gc0329_ycbcr_i2c_write_byte(0x68, 0x58); //58 //AWB G gain Limit
	camsensor_gc0329_ycbcr_i2c_write_byte(0x69, 0x85); //7d //AWB B gain limit
	camsensor_gc0329_ycbcr_i2c_write_byte(0x6a, 0x40);
	camsensor_gc0329_ycbcr_i2c_write_byte(0x6b, 0x39);
	camsensor_gc0329_ycbcr_i2c_write_byte(0x6c, 0x40);
	camsensor_gc0329_ycbcr_i2c_write_byte(0x6d, 0x80);
	camsensor_gc0329_ycbcr_i2c_write_byte(0x6e, 0x00); //41 //outdoor gain limit enable [7]use exp or luma value to adjust outdoor 
	camsensor_gc0329_ycbcr_i2c_write_byte(0x70, 0x50);
	camsensor_gc0329_ycbcr_i2c_write_byte(0x71, 0x00); //when outdoor , add high luma gray pixel weight
	camsensor_gc0329_ycbcr_i2c_write_byte(0x72, 0x10);
	camsensor_gc0329_ycbcr_i2c_write_byte(0x73, 0x40); //95// when exp < th, outdoor mode open
	camsensor_gc0329_ycbcr_i2c_write_byte(0x74, 0x32);
	camsensor_gc0329_ycbcr_i2c_write_byte(0x75, 0x40);
	camsensor_gc0329_ycbcr_i2c_write_byte(0x76, 0x30);
	camsensor_gc0329_ycbcr_i2c_write_byte(0x77, 0x48);
	camsensor_gc0329_ycbcr_i2c_write_byte(0x7a, 0x50);
	camsensor_gc0329_ycbcr_i2c_write_byte(0x7b, 0x20); // Yellow R2B, 0xB2G limit, >it, as Yellow
	camsensor_gc0329_ycbcr_i2c_write_byte(0x80, 0x58); //R gain high limit
	camsensor_gc0329_ycbcr_i2c_write_byte(0x81, 0x50); //G gain high limit
	camsensor_gc0329_ycbcr_i2c_write_byte(0x82, 0x44); //B gain high limit
	camsensor_gc0329_ycbcr_i2c_write_byte(0x83, 0x40); //R gain low limit
	camsensor_gc0329_ycbcr_i2c_write_byte(0x84, 0x40); //G gain low limit
	camsensor_gc0329_ycbcr_i2c_write_byte(0x85, 0x40); //B gain low limit

	////////////////////CC-AWB////////////////////
	camsensor_gc0329_ycbcr_i2c_write_byte(0xd0, 0x00);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xd2, 0x2c); //D Xn
	camsensor_gc0329_ycbcr_i2c_write_byte(0xd3, 0x80);

	///////////////////ABS///////////////////////
	camsensor_gc0329_ycbcr_i2c_write_byte(0x9c, 0x02);
	camsensor_gc0329_ycbcr_i2c_write_byte(0x9d, 0x10); //08//20

	///////////////////LSC //////////////////////
	//// for xuye062d lens setting
	camsensor_gc0329_ycbcr_i2c_write_byte(0xfe, 0x01);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xa0, 0x00);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xa1, 0x3c);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xa2, 0x50);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xa3, 0x00);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xa8, 0x0f);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xa9, 0x08);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xaa, 0x00);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xab, 0x04);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xac, 0x00);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xad, 0x07);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xae, 0x0e);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xaf, 0x00);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xb0, 0x00);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xb1, 0x09);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xb2, 0x00);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xb3, 0x00);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xb4, 0x31);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xb5, 0x19);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xb6, 0x24);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xba, 0x3a);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xbb, 0x24);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xbc, 0x2a);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xc0, 0x17);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xc1, 0x13);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xc2, 0x17);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xc6, 0x21);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xc7, 0x1c);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xc8, 0x1c);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xb7, 0x00);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xb8, 0x00);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xb9, 0x00);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xbd, 0x00);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xbe, 0x00);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xbf, 0x00);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xc3, 0x00);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xc4, 0x00);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xc5, 0x00);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xc9, 0x00);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xca, 0x00);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xcb, 0x00);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xa4, 0x00);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xa5, 0x00);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xa6, 0x00);
	camsensor_gc0329_ycbcr_i2c_write_byte(0xa7, 0x00);

	camsensor_gc0329_ycbcr_i2c_write_byte(0xfe, 0x00);
	////////////////////asde ///////////////////
	camsensor_gc0329_ycbcr_i2c_write_byte(0xa0, 0xaf); //[7:4]bright_slope for special point
	camsensor_gc0329_ycbcr_i2c_write_byte(0xa2, 0xff); //[for special point

	camsensor_gc0329_ycbcr_i2c_write_byte(0x44, 0xa0); //23//output format 0xa2

	return  TRUE;
} /* camsensor_gc0329_sensor_init */


uint8 camsensor_gc0329_ycbcr_active(void)
{
	return 0;
}

uint8 camsensor_gc0329_ycbcr_unactive(void)
{
	return 1;
}

/*===========================================================================

FUNCTION      camsensor_gc0329_init

DESCRIPTION
              Initialize the camera sensor;

DEPENDENCIES
  None

RETURN VALUE
  if successful
    TRUE
  else
    FALSE

SIDE EFFECTS
  None

===========================================================================*/
static void gc0329_0p1mp_H_V_Switch(uint8 direction)
{
    switch(direction) 
    {
        case 1:  // normal
            camsensor_gc0329_ycbcr_i2c_write_byte(0x17, 0x14);    
            break;
        case 2:  // IMAGE_H_MIRROR
            camsensor_gc0329_ycbcr_i2c_write_byte(0x17, 0x15);    
            break;        
        case 3:  // IMAGE_V_MIRROR
            camsensor_gc0329_ycbcr_i2c_write_byte(0x17, 0x16);    
            break;  
        case 4:  // IMAGE_HV_MIRROR
            camsensor_gc0329_ycbcr_i2c_write_byte(0x17, 0x17);    
            break;  
        default:
            break;
    } 
}

void gc0329_0p1mp_write_more_registers(uint8 direction)
{
	//  TODO: FAE Modify the Init Regs here!!!
	/*Customer can adjust GAMMA, MIRROR & UPSIDEDOWN here!*/
	//gc0329_0p1mp_gamma_select(2);
	if(direction)
  	gc0329_0p1mp_H_V_Switch(1);
    else
    gc0329_0p1mp_H_V_Switch(2);
}

boolean camsensor_gc0329_init(camsensor_function_table_type *camsensor_function_table_ptr, camctrl_tbl_type *camctrl_tbl_ptr)
{
	uint8 sensor_id = 0;
	MSG_FATAL("camsensor_gc0329_init()\n",0,0,0);///yty 


	camsensor_camclk_po_hz =24000000;
#ifndef FEATURE_VERSION_K212
#ifndef FEATURE_VERSION_K212_12832

	CAMERA_CONFIG_GPIO(CAMSENSOR_GC0329_RESET_PIN);
    
    gpio_out(CAMSENSOR_GC0329_RESET_PIN,1);
    clk_busy_wait(2*1000);
    gpio_out(CAMSENSOR_GC0329_RESET_PIN,0);
    clk_busy_wait(50*1000);
    gpio_out(CAMSENSOR_GC0329_RESET_PIN,1);
    clk_busy_wait(2*1000);
#endif
#endif
	camsensor_preview_resolution  = CAMSENSOR_FULL_SIZE;
	camsensor_snapshot_resolution = CAMSENSOR_FULL_SIZE;


	/* 1. Initiate I2C:  */
	camsensor_i2c_command.bus_id     = I2C_BUS_HW_CTRL;
	camsensor_i2c_command.slave_addr = CAMSENSOR_GC0329_I2C_SLAVE_ID;
	camsensor_i2c_command.options    = (i2c_options_type) (I2C_REG_DEV | I2C_START_BEFORE_READ); 

	camsensor_gc0329_ycbcr_i2c_write_byte(0xfc, 0x16);

	clk_busy_wait(50*1000);
	camsensor_gc0329_ycbcr_i2c_read_byte(0x00,&sensor_id);
	MSG_FATAL("Sensor ID = 0x%x",sensor_id,0,0);
	if ( sensor_id != CAMSENSOR_GC0329_SENSOR_ID ) 
	{
		return FALSE;
	}
	/* Initialize the sensor_function_table */
	camsensor_gc0329_ycbcr_register (camsensor_function_table_ptr);

	camsensor_gc0329_0m3_setup_camctrl_tbl(camctrl_tbl_ptr);
	return TRUE;
} /* camsensor_gc0329_init */


/*===========================================================================

FUNCTION      camsensor_gc0329_ycbcr_start

DESCRIPTION
              Initialize the camsensor parameters.

DEPENDENCIES
  None

RETURN VALUE
  if successful
    TRUE
  else
    FALSE

SIDE EFFECTS
  None

===========================================================================*/
static boolean camsensor_gc0329_ycbcr_start( camsensor_static_params_type *camsensor_params)
{

	MSG_FATAL("camsensor_gc0329_ycbcr_start begin ", 0,0,0);///yty 

	/* Initialize CAMIF operation mode */
	camsensor_params->camif_config.SyncMode         = CAMIF_APS;
	camsensor_params->camif_config.HSyncEdge        = CAMIF_High;
	camsensor_params->camif_config.VSyncEdge        = CAMIF_High;
	camsensor_params->camif_efs_config.EFS_EOL      = 0x0000;
	camsensor_params->camif_efs_config.EFS_SOL      = 0x0000;
	camsensor_params->camif_efs_config.EFS_EOF      = 0x0000;
	camsensor_params->camif_efs_config.EFS_SOF      = 0x0000;

	/* ------------------  Sensor-specific Config -------------- */
	/* Make/model of sensor */
	camsensor_params->sensor_model = CAMSENSOR_ID_GC0329;

	/* CCD or CMOS */
	camsensor_params->sensor_type = CAMSENSOR_CMOS;

	camsensor_params->format = CAMIF_YCbCr_Cr_Y_Cb_Y;
	/* BAYER or YCbCr */
	camsensor_params->output_format = CAMSENSOR_YCBCR;

	/* A pointer to the sensor name for EXIF tags                */
	camsensor_params->sensor_name = camsensor_gc0329_ycbcr_sensor_name;

	/* What is the maximum FPS that can be supported by this sensor
	 in video mode */
	camsensor_params->max_video_fps = 15 * Q8;

	/* Application assigned FPS in video mode */
	camsensor_params->video_fps = 15 * Q8;

	/* Snapshot mode operation */
	camsensor_params->max_preview_fps = 15 * Q8;
	camsensor_params->nightshot_fps = 15 * Q8;

	/* May be assigned with max_preview_fps or nightshot_fps. */
	camsensor_params->preview_fps = 15 * Q8;

	/* PCLK Invert */
	// camsensor_params->pclk_invert = TRUE;

	/* VFE's perception of Sensor output capability */

	/* Snapshot dimensions */
	camsensor_params->full_size_width  = OV_GC0329_YCBCR_FULL_SIZE_WIDTH;
	camsensor_params->full_size_height = OV_GC0329_YCBCR_FULL_SIZE_HEIGHT;

	/* Preview dimensions */
	camsensor_params->qtr_size_width = OV_GC0329_YCBCR_QTR_SIZE_WIDTH;
	camsensor_params->qtr_size_height = OV_GC0329_YCBCR_QTR_SIZE_HEIGHT;

	switch (camsensor_preview_resolution)
	{
		case CAMSENSOR_QTR_SIZE:
			camsensor_params->preview_dx_decimation = camsensor_params->full_size_width * Q12 / camsensor_params->qtr_size_width;
			camsensor_params->preview_dy_decimation = camsensor_params->full_size_height * Q12 / camsensor_params->qtr_size_height;

			/* Set the current dimensions */
			camsensor_params->camsensor_width  = camsensor_params->qtr_size_width;
			camsensor_params->camsensor_height = camsensor_params->qtr_size_height;
			break;

		case CAMSENSOR_FULL_SIZE:
			camsensor_params->preview_dx_decimation = Q12;
			camsensor_params->preview_dy_decimation = Q12;

			/* Set the current dimensions */
			camsensor_params->camsensor_width  = camsensor_params->full_size_width;
			camsensor_params->camsensor_height = camsensor_params->full_size_height;
			break;

		default:
			camsensor_params->preview_dx_decimation = Q12;
			camsensor_params->preview_dy_decimation = Q12;
			break;
	}

	/* This tells camera service the minimum decimation that is supported
	* by the sensor. 
	* Ex: if preview is in quarter size mode, then there is a 
	* sensor decimation of 2, so the minimum is 2 
	*/
	camsensor_params->preview_dx_decimation = (camsensor_params->preview_dx_decimation < Q12) ? Q12 : camsensor_params->preview_dx_decimation;
	camsensor_params->preview_dy_decimation = (camsensor_params->preview_dy_decimation < Q12) ? Q12 : camsensor_params->preview_dy_decimation;

	/* ------------  Auto Exposure Control Config -------------- */
	camsensor_params->aec_enable = FALSE;

	/* ------------  Auto White Balance Config ----------------- */
	/* AWB -Auto White Balance Parameters */
	camsensor_params->awb_enable = FALSE;

	/* ------------  Auto Focus Config ------------------------- */
	/* AF -Auto Focus Parameters */
	camsensor_params->af_enable = FALSE; //TRUE;

	camsensor_params->num_possible_frame_rates = 1;
	/* Define these frame rates */
	/* By convention, the highest frame rate will be first in the
	array (zeroth index) and the lowest last (in order). */
	camsensor_params->frame_rate_array[0].fps = (uint16) (15*256.0); /* Q8 */
	camsensor_params->frame_rate_array[0].use_in_auto_frame_rate = TRUE;

	camsensor_params->frame_rate_array[1].fps = (uint16) (15*256.0); /* Q8 */
	camsensor_params->frame_rate_array[1].use_in_auto_frame_rate = FALSE;

	camsensor_params->pclk_invert = camsensor_info.pclk_invert;
	/* ------------  Default Misc Parmas Config ---------------- */
	/* Does the sensor need/use a flash  */
#if defined FEATURE_WHITE_LED_FLASH || defined FEATURE_STROBE_FLASH
	camsensor_params->support_auto_flash = FALSE;//TRUE;   ///lee test 1004
#else
	camsensor_params->support_auto_flash = FALSE;
#endif

	return TRUE;
} /* camsensor_SIV121A_ycbcr_start */


/*===========================================================================

FUNCTION      camsensor_gc0329_ycbcr_snapshot_config

DESCRIPTION
              Configure the camera sensor and the VFE_CAMIF for
              snapshot mode

DEPENDENCIES
  None

RETURN VALUE
  TRUE if successful
  FALSE otherwise

SIDE EFFECTS
  None

===========================================================================*/

static boolean camsensor_gc0329_ycbcr_snapshot_config
(
  camsensor_static_params_type *camsensor_params /* Other config params */
) 
{
	/* Sensor output data format */
	camsensor_params->format = CAMIF_YCbCr_Cr_Y_Cb_Y;

	switch (camsensor_snapshot_resolution)
	{
		case CAMSENSOR_QTR_SIZE:
			/* Set the current dimensions */
			camsensor_params->camsensor_width = camsensor_params->qtr_size_width;
			camsensor_params->camsensor_height = camsensor_params->qtr_size_height;
			/* CAMIF frame */
			camsensor_params->camif_frame_config.pixelsPerLine = camsensor_params->qtr_size_width*2;
			camsensor_params->camif_frame_config.linesPerFrame = camsensor_params->qtr_size_height;
			break;

		case CAMSENSOR_FULL_SIZE:
			/* Set the current dimensions */
			camsensor_params->camsensor_width = camsensor_params->full_size_width;
			camsensor_params->camsensor_height = camsensor_params->full_size_height;
			/* CAMIF frame */
			camsensor_params->camif_frame_config.pixelsPerLine = camsensor_params->full_size_width*2;
			camsensor_params->camif_frame_config.linesPerFrame = camsensor_params->full_size_height;
			break;

		default:
			return FALSE;
	} /* camsensor_preview_resolution */

	/* CAMIF window */
	camsensor_params->camif_window_width_config.firstPixel = 0;
	camsensor_params->camif_window_width_config.lastPixel  = camsensor_params->camif_window_width_config.firstPixel + camsensor_params->camsensor_width*2 - 1;
	camsensor_params->camif_window_height_config.firstLine = 0;
	camsensor_params->camif_window_height_config.lastLine = camsensor_params->camif_window_height_config.firstLine + camsensor_params->camsensor_height - 1;

	clk_busy_wait(100*1000);
	camsensor_current_resolution = camsensor_snapshot_resolution;
	return TRUE;
} /* camsensor_gc0329_ycbcr_snapshot_config */

/*===========================================================================

FUNCTION      camsensor_gc0329_ycbcr_raw_snapshot_config

DESCRIPTION
              Configure the camera sensor and the VFE_CAMIF for
              raw snapshot mode

DEPENDENCIES
  None

RETURN VALUE
  TRUE if successful
  FALSE otherwise

SIDE EFFECTS
  None

===========================================================================*/

static boolean camsensor_gc0329_ycbcr_raw_snapshot_config
(
  camsensor_static_params_type *camsensor_params /* Other config params */
) 
{
	return TRUE;
} /* camsensor_gc0329_ycbcr_raw_snapshot_config */


/*===========================================================================

FUNCTION      camsensor_gc0329_ycbcr_video_config

DESCRIPTION
              Configure the camera sensor and the camera interface
              for Preview mode. Implements all 

DEPENDENCIES
  None

RETURN VALUE
  TRUE if successful
  FALSE otherwise

SIDE EFFECTS
  None

===========================================================================*/

static boolean camsensor_gc0329_ycbcr_video_config
(
  camsensor_static_params_type *camsensor_params /* Other config params */
)
{
	camsensor_gc0329_sensor_init();
	/* Sensor output data format */
	camsensor_params->discardFirstFrame = TRUE;
	camsensor_params->format = CAMIF_YCbCr_Cr_Y_Cb_Y;

	switch (camsensor_preview_resolution)
	{
		case CAMSENSOR_QTR_SIZE:
			/* Set the current dimensions */
			camsensor_params->camsensor_width = camsensor_params->qtr_size_width;
			camsensor_params->camsensor_height = camsensor_params->qtr_size_height;
			/* CAMIF frame */
			camsensor_params->camif_frame_config.pixelsPerLine = camsensor_params->qtr_size_width*2;
			camsensor_params->camif_frame_config.linesPerFrame = camsensor_params->qtr_size_height;
			break;

		case CAMSENSOR_FULL_SIZE:
			/* Set the current dimensions */
			camsensor_params->camsensor_width = camsensor_params->full_size_width;
			camsensor_params->camsensor_height = camsensor_params->full_size_height;
			/* CAMIF frame */
			camsensor_params->camif_frame_config.pixelsPerLine = camsensor_params->full_size_width*2;
			camsensor_params->camif_frame_config.linesPerFrame = camsensor_params->full_size_height;
			break;

		default:
			return FALSE;
	} /* camsensor_preview_resolution */

	/* CAMIF window */
	camsensor_params->camif_window_width_config.firstPixel = 0;
	camsensor_params->camif_window_width_config.lastPixel  = camsensor_params->camif_window_width_config.firstPixel + camsensor_params->camsensor_width*2 - 1;
	camsensor_params->camif_window_height_config.firstLine = 0;
	camsensor_params->camif_window_height_config.lastLine = camsensor_params->camif_window_height_config.firstLine + camsensor_params->camsensor_height - 1;

	camsensor_current_resolution = camsensor_preview_resolution;

	return TRUE;
} /* camsensor_gc0329_ycbcr_video_config */

/*===========================================================================

FUNCTION      camsensor_gc0329_ycbcr_power_up

DESCRIPTION
              Power up the sensor.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
static void camsensor_gc0329_ycbcr_power_up(void)
{
}


/*===========================================================================

FUNCTION      camsensor_gc0329_ycbcr_power_down

DESCRIPTION
              Power down the sensor.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
static void camsensor_gc0329_ycbcr_power_down(void)
{
    MSG_FATAL ("camsensor_gc0329_ycbcr_power_down begin", 0,0,0);
}


/*===========================================================================

FUNCTION      camsensor_gc0329_ycbcr_i2c_read_byte

DESCRIPTION
              8-bit I2C read.

DEPENDENCIES
  None

RETURN VALUE
  TRUE - I2C read successful
  FALSE - I2C read failed


SIDE EFFECTS
  None

===========================================================================*/
static boolean camsensor_gc0329_ycbcr_i2c_read_byte(uint8 reg, uint8 *data) 
{
    static uint8 readbyte; 
	uint8  i;

	if (data == NULL)
	{
		return FALSE;
	}

	camsensor_i2c_command.addr.reg = reg;
	camsensor_i2c_command.buf_ptr  = (byte *)(&readbyte);
	camsensor_i2c_command.len      = 1;

	for (i =0; i < 3; ++i)
	{
		if (i2c_read(&camsensor_i2c_command) == I2C_SUCCESS)
		{
			*data  = readbyte;
			MSG_FATAL("camsensor_gc0329_ycbcr_i2c_read_byte: OK %x",reg,0,0);
			return TRUE;
		}
	}

	MSG_FATAL("camsensor_gc0329_ycbcr_i2c_read_byte: false %x",reg,0,0);
	return FALSE;
}

/*===========================================================================

FUNCTION      camsensor_gc0329_ycbcr_i2c_write_byte

DESCRIPTION
              8-bit I2C write.

DEPENDENCIES
  None

RETURN VALUE
  TRUE  - I2C write successful
  FALSE - I2C write failed


SIDE EFFECTS
  None

===========================================================================*/
static boolean camsensor_gc0329_ycbcr_i2c_write_byte(uint8 reg, uint8 data) 
{
    static uint8 writebyte;
	uint8 i;

	writebyte  = data;

	camsensor_i2c_command.addr.reg = reg;
	camsensor_i2c_command.buf_ptr  = (byte *)(&writebyte);
	camsensor_i2c_command.len      = 1;

	for (i = 0; i < 3; ++i)
	{
		if (i2c_write(&camsensor_i2c_command) == I2C_SUCCESS)
		{
			MSG_FATAL("camsensor_gc0329_ycbcr_i2c_write_byte: OK %x,%x",reg,data,0);
			return TRUE;
		}
	}
	MSG_FATAL("camsensor_gc0329_ycbcr_i2c_write_byte: false %x,%x",reg,data,0);
	return FALSE;
} /* camsensor_gc0329_ycbcr_i2c_write_byte */


/* The following are stub functions, (unsupported sensor functions) */
static camera_ret_code_type camsensor_gc0329_ycbcr_write_exposure_gain(uint16 gain, uint32 line_count)
{
	return(CAMERA_SUCCESS);
} /* camsensor_gc0329_ycbcr_write_exposure_gain */

//wlr 1114 start
static camera_ret_code_type camsensor_gc0329_set_effect(int8 effect) 
{
	camera_ret_code_type ret_val = CAMERA_SUCCESS;

	MSG_FATAL ("+++++ camsensor_gc0329_set_effect effect = %d",effect,0,0);
	
	switch(effect)
	{
		case CAMERA_EFFECT_OFF://normal
			break;
			
		case CAMERA_EFFECT_MONO:
			break;
			
		case CAMERA_EFFECT_NEGATIVE://负片
			break;
			
		case CAMERA_EFFECT_SEPIA://棕褐色
			break;

		default:
			ret_val = CAMERA_INVALID_PARM;
			break;
	}

  	return ret_val;
}/* camsensor_gc0329_set_effect */

static camera_ret_code_type camsensor_gc0329_set_wb(int8 wb) 
{
  	camera_ret_code_type ret_val = CAMERA_SUCCESS;

	MSG_FATAL ("+++++ camsensor_gc0329_set_wb wb = %d",wb,0,0);

  	switch (wb)
  	{
		case  CAMERA_WB_AUTO://自动
		    camsensor_gc0329_ycbcr_i2c_write_byte(0x42, 0x7e);
	  		break;

		case CAMERA_WB_CLOUDY_DAYLIGHT://多云
            camsensor_gc0329_ycbcr_i2c_write_byte(0x42, 0x7d); //WB_manual_gain 
            camsensor_gc0329_ycbcr_i2c_write_byte(0x77, 0x8c); //WB_manual_gain 
            camsensor_gc0329_ycbcr_i2c_write_byte(0x78, 0x50);
            camsensor_gc0329_ycbcr_i2c_write_byte(0x79, 0x40);
	  		break;
	
		case CAMERA_WB_INCANDESCENT://白炽灯
   		
           camsensor_gc0329_ycbcr_i2c_write_byte(0x42, 0x7d); 
           camsensor_gc0329_ycbcr_i2c_write_byte(0x77, 0x48);
           camsensor_gc0329_ycbcr_i2c_write_byte(0x78, 0x40);
           camsensor_gc0329_ycbcr_i2c_write_byte(0x79, 0x5c);
	  		break;

		case CAMERA_WB_FLUORESCENT: //荧光
            camsensor_gc0329_ycbcr_i2c_write_byte(0x42, 0x7d); 
            camsensor_gc0329_ycbcr_i2c_write_byte(0x77, 0x40);
            camsensor_gc0329_ycbcr_i2c_write_byte(0x78, 0x42);
            camsensor_gc0329_ycbcr_i2c_write_byte(0x79, 0x50);
	  		break;	
	  
		case CAMERA_WB_DAYLIGHT:  //日光
            camsensor_gc0329_ycbcr_i2c_write_byte(0x42, 0x7d); 
            camsensor_gc0329_ycbcr_i2c_write_byte(0x77, 0x74); 
            camsensor_gc0329_ycbcr_i2c_write_byte(0x78, 0x52);
            camsensor_gc0329_ycbcr_i2c_write_byte(0x79, 0x40); 
			break;

		default:
	  		ret_val = CAMERA_INVALID_PARM;
	  		break;
  	}

  	return ret_val;
}/* camsensor_gc0329_set_wb */
//wlr 1114 end

/*===========================================================================

FUNCTION	  camsensor_gc0329_set_brightness

DESCRIPTION   
  Set brightness.

DEPENDENCIES
  None

INPUT
  positive or negative value to change birghtness.

RETURN VALUE
  CAMERA_SUCCESS - if we changed brightness successfully
  CAMERA_FAILED  - if brightness change failed

SIDE EFFECTS
  This will change the output brgihtness.

===========================================================================*/
static camera_ret_code_type camsensor_gc0329_set_brightness (int8 value)
{
  	return CAMERA_SUCCESS;
} /* camsensor_gc0329_set_brightness */

/*===========================================================================

FUNCTION	  camsensor_gc0329_set_contrast

DESCRIPTION   
  Set contrast.

DEPENDENCIES
  None

INPUT
  positive or negative value to change contrast.

RETURN VALUE
  CAMERA_SUCCESS - if we changed contrast successfully
  CAMERA_FAILED  - if contrast change failed

SIDE EFFECTS
  This will change the output contrast.

===========================================================================*/
static camera_ret_code_type camsensor_gc0329_set_contrast (int8 contrast)
{
  	return CAMERA_SUCCESS;
} /* camsensor_gc0329_set_contrast */

static camera_ret_code_type camsensor_gc0329_set_frame_rate(uint16 fps)
{
  	return CAMERA_SUCCESS;
} /* camsensor_gc0329_set_frame_rate */

static camera_ret_code_type camsensor_gc0329_set_antibanding(int8 antibanding)
{
	return CAMERA_SUCCESS;
}

static uint16 camsensor_gc0329_0m3_get_snapshot_fps(uint16 fps)
{
	/* In the current driver, the snapshot fps is always 11 */
	fps = 15 * Q8;

	return fps;
}

static void    camsensor_gc0329_0m3_setup_camctrl_tbl(camctrl_tbl_type *camctrl_tbl_ptr)
{
	camctrl_tbl_ptr->asf_5x5_is_supported         = FALSE; 
	camctrl_tbl_ptr->la_is_supported              = FALSE;
	camctrl_tbl_ptr->cs_is_supported              = FALSE;
	camctrl_tbl_ptr->iso_is_supported             = FALSE;
	camctrl_tbl_ptr->enable_rolloff_correction    = FALSE;
	camctrl_tbl_ptr->hjr_bayer_filtering_enable   = FALSE;
	camctrl_tbl_ptr->nightshot_is_supported       = FALSE;

	camctrl_tbl_ptr->get_snapshot_frame_per_sec = camsensor_gc0329_0m3_get_snapshot_fps;
}
/*===========================================================================

FUNCTION   camsensor_gc0329_ycbcr_register 

DESCRIPTION  
  Exports the sensor specific functions to CAMSENSOR

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
static void camsensor_gc0329_ycbcr_register (camsensor_function_table_type *camsensor_function_table_ptr)
{
	camsensor_function_table_ptr->camsensor_start               	= camsensor_gc0329_ycbcr_start;
	camsensor_function_table_ptr->camsensor_video_config        	= camsensor_gc0329_ycbcr_video_config;
	camsensor_function_table_ptr->camsensor_raw_snapshot_config 	= camsensor_gc0329_ycbcr_raw_snapshot_config;
	camsensor_function_table_ptr->camsensor_snapshot_config     	= camsensor_gc0329_ycbcr_snapshot_config;
	camsensor_function_table_ptr->camsensor_power_up            	= camsensor_gc0329_ycbcr_power_up;
	camsensor_function_table_ptr->camsensor_power_down          	= camsensor_gc0329_ycbcr_power_down;
	camsensor_function_table_ptr->camsensor_write_exposure_gain 	= camsensor_gc0329_ycbcr_write_exposure_gain;
	camsensor_function_table_ptr->camsensor_set_frame_rate        	= camsensor_gc0329_set_frame_rate;
	camsensor_function_table_ptr->camsensor_set_effect				= camsensor_gc0329_set_effect; 
	camsensor_function_table_ptr->camsensor_set_wb					= camsensor_gc0329_set_wb;
	camsensor_function_table_ptr->camsensor_set_brightness 			= camsensor_gc0329_set_brightness;	 
	camsensor_function_table_ptr->camsensor_set_contrast 			= camsensor_gc0329_set_contrast;
	camsensor_function_table_ptr->camsensor_set_antibanding 		= camsensor_gc0329_set_antibanding; 
} /* camsensor_SIV121A_ycbcr_register */
#endif /* FEATURE_CAMERA */
