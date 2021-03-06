#include "camsensor.h"

#if defined (FEATURE_CAMERA) && defined (USE_CAMSENSOR_SP0828)
#include "camsensor_sp0828.h"
#include "camerai.h"
#include "ipl.h"
#include "msg.h"
#include "i2c.h"
#include "clk.h"
#include "chromatix_6550.h"
#include "disp_options.h"
#include "camera_svcs.h"

/*============================================================================
    TYPE DEFINITIONS 
============================================================================*/
#if defined(FEATURE_VERSION_C180) || defined(FEATURE_VERSION_C11)
#define T_QSC1110
#endif

#if defined(FEATURE_VERSION_W516)
#ifdef T_QSC1110
#ifndef FEATURE_VERSION_K212
#ifndef FEATURE_VERSION_K212_12832

#define CAMSENSOR_SP0828_RESET_PIN         GPIO_OUTPUT_10
#endif
#endif
#else
#define CAMSENSOR_SP0828_RESET_PIN         GPIO_OUTPUT_53
#endif
#else
#ifdef T_QSC1110
#ifndef FEATURE_VERSION_K212
#ifndef FEATURE_VERSION_K212_12832

#define CAMSENSOR_SP0828_RESET_PIN        GPIO_OUTPUT_10
#endif
#endif
#else
#define CAMSENSOR_SP0828_RESET_PIN         GPIO_OUTPUT_62
#endif
#endif
#ifndef T_QSC1110
#define SP0828_OUTFORMAT_RGB565
#endif


// sensor's chip ID and version
#define SP0828_SENSOR_ID                   (0x0C)
//#define SIC110A_SENSOR_VERSION              (0x02)

#define SP0828_I2C_WRITE_ID                (0x30)
#define SP0828_I2C_READ_ID                 (0x31)

/* SENSOR REGISTER DEFINE */
#define SP0828_ID_REG                      (0x02)
#define SP0828_INFO_REG                    (0x02)


#define CAMSENSOR_SP0828_FULL_SIZE_WIDTH   288 //640
#define CAMSENSOR_SP0828_FULL_SIZE_HEIGHT  352 //480

#define MAX_EV_COMP                          25

/* FPS supported by this sensor */
#define CAMSENSOR_30_FPS                    (30*Q8)
#define CAMSENSOR_15_FPS                    (15*Q8)

/*============================================================================
    LOCAL Variables 
============================================================================*/
static camera_effect_type   camsensor_sp0828_effect             = CAMERA_EFFECT_OFF;
static char                 camsensor_sp0828_sensor_name[32]    = "SP0828 0.1MP RGB565\0\0\0\0\0\0\0";
static const char           camsensor_sp0828_sensor_suffix[]    = "SP0828";

/*============================================================================
    Function Declares
============================================================================*/
static void    camsensor_sp0828_register(camsensor_function_table_type *camsensor_function_table_ptr);
static void    camsensor_sp0828_setup_camctrl_tbl(camctrl_tbl_type *camctrl_tbl_ptr);
static boolean initialize_sp0828_registers(uint16 dx, uint16 dy);
static uint16  camsensor_sp0828_get_snapshot_fps(uint16 fps);
static boolean sp0828_i2c_write_byte(uint8 offset, uint8 data);
static boolean sp0828_i2c_read_byte(uint8 offset, uint8* data);

/*============================================================================
    Function Body
============================================================================*/
static camera_ret_code_type camsensor_sp0828_set_exposure_mode(int8 mode)
{
    return CAMERA_SUCCESS;
}

static camera_ret_code_type camsensor_sp0828_set_frame_rate(uint16 fps)
{
    if (fps > CAMSENSOR_30_FPS)
    {
        return CAMERA_INVALID_PARM;
    } 

    return CAMERA_SUCCESS;
} /* camsensor_sp0828_set_frame_rate() */


uint8 camsensor_sp0828_ycbcr_active(void)
{
	return 0;
}

uint8 camsensor_sp0828_ycbcr_unactive(void)
{
	return 1;
}

boolean camsensor_sp0828_init(camsensor_function_table_type *camsensor_function_table_ptr,
                               camctrl_tbl_type              *camctrl_tbl_ptr)
{
    uint8   sensor_id;
    MSG_FATAL("camsensor_sp0828_init!",0,0,0);
    
    /* Input MCLK = 24MHz */
    camsensor_camclk_po_hz = 24000000;
    camsensor_camclk_po_hz = camsensor_config_camclk_po(camsensor_camclk_po_hz);
    
    /* Preview must aways be se to quater size */
    camsensor_preview_resolution  = CAMSENSOR_QTR_SIZE;
    camsensor_snapshot_resolution = CAMSENSOR_FULL_SIZE;
    
    /* Configure I2C parameters */
    camsensor_i2c_command.bus_id     = I2C_BUS_HW_CTRL;
    camsensor_i2c_command.slave_addr = SP0828_I2C_WRITE_ID;
    
    /*lint -save -e655 */
    camsensor_i2c_command.options    = (i2c_options_type) (I2C_REG_DEV | I2C_START_BEFORE_READ); 
    
	i2c_operation_fs_set(25);
	camera_timed_wait(10);  //ovt

    
	if( !sp0828_i2c_write_byte(0xfd,0x00)) 
	{
		MSG_FATAL("Block Select Error!",0,0,0);
		return FALSE;
	}

	if( !sp0828_i2c_read_byte(SP0828_ID_REG,&sensor_id)) 
	{
	 	MSG_FATAL("read sensor_id failed!",0,0,0);
	 	return FALSE;
	}

	MSG_FATAL("sensor_id 1 = %x",sensor_id,0,0);

	/* Check if it matches it with the value in Datasheet */
	if ( sensor_id != SP0828_SENSOR_ID)
	{
	 	MSG_FATAL("read sensor_id do not matches!",0,0,0);
	 	return FALSE;
	}

    initialize_sp0828_registers(CAMSENSOR_SP0828_FULL_SIZE_WIDTH, CAMSENSOR_SP0828_FULL_SIZE_HEIGHT);
    
    /* Register function table: */
    camsensor_sp0828_register(camsensor_function_table_ptr);

    /* Setup camctrl_tbl */
    camsensor_sp0828_setup_camctrl_tbl(camctrl_tbl_ptr);
    
    MSG_FATAL("camsensor_sic110a_init Exit",0,0,0);
    
    return TRUE;
} /* camsensor_sp0828_init */

static boolean initialize_sp0828_registers(uint16 dx, uint16 dy)
{
#ifndef T_QSC1110
    uint16 x,y;
    uint8 internel_resize = FALSE;
    if(dx > CAMSENSOR_SP0828_FULL_SIZE_WIDTH)
    {
        dx = CAMSENSOR_SP0828_FULL_SIZE_WIDTH;
    }
    
    if(dy > CAMSENSOR_SP0828_FULL_SIZE_HEIGHT)
    {
        dy = CAMSENSOR_SP0828_FULL_SIZE_HEIGHT;
    }
    
    x = (CAMSENSOR_SP0828_FULL_SIZE_WIDTH-dx)>>1;
    y = (CAMSENSOR_SP0828_FULL_SIZE_HEIGHT-dy)>>1;

    if(x<60)
	{
		x = 60;
	}
	
	if((dx<128)&&(dy<228)) //sensor internel resize for inprove sharpness by xuegang  
	{
		dx=dx*1.4;
		dy=dy*1.4;
		internel_resize = TRUE;
	}

#endif
    //Sensor Block Setting  ###Don't Change###
    
	sp0828_i2c_write_byte(0xfd,0x00);
	sp0828_i2c_write_byte(0x1c,0x00);//08
	sp0828_i2c_write_byte(0x30,0x00);//00:不分频 02:2分频
	sp0828_i2c_write_byte(0x0f,0x2f);//;analog
	sp0828_i2c_write_byte(0x10,0x2f);
	//sp0828_i2c_write_byte(0x12,0x6f);
	sp0828_i2c_write_byte(0x13,0x2f);
	//sp0828_i2c_write_byte(0x15,0x7f);
	//sp0828_i2c_write_byte(0x16,0x0f);
	sp0828_i2c_write_byte(0x22,0xe0);
	sp0828_i2c_write_byte(0x26,0x08);
	sp0828_i2c_write_byte(0x27,0xe8);
	sp0828_i2c_write_byte(0x28,0x03);
	sp0828_i2c_write_byte(0x32,0x00);

	sp0828_i2c_write_byte(0xfd,0x00);
	sp0828_i2c_write_byte(0x31,0x00);	 //Upside/mirr/Pclk inv/sub
	sp0828_i2c_write_byte(0xd8,0x62);  //0x58 pzt 20111102
	sp0828_i2c_write_byte(0xd9,0x62);  //0x58 pzt 20111102
	sp0828_i2c_write_byte(0xda,0x58);
	sp0828_i2c_write_byte(0xdb,0x48);

	sp0828_i2c_write_byte(0x5f,0x11);


	sp0828_i2c_write_byte(0xe0,0x00);//;resize
	sp0828_i2c_write_byte(0xe1,0xdc);
	sp0828_i2c_write_byte(0xe2,0xb0);
	sp0828_i2c_write_byte(0xe3,0x00);
	sp0828_i2c_write_byte(0xe4,0x2e);
	sp0828_i2c_write_byte(0xe5,0x00);
	sp0828_i2c_write_byte(0xe6,0x2b);
	sp0828_i2c_write_byte(0xb7,0x3c);//;LSC
	sp0828_i2c_write_byte(0xb8,0x50);
	sp0828_i2c_write_byte(0xfd,0x01);
	sp0828_i2c_write_byte(0x25,0x1a);//AWB
	sp0828_i2c_write_byte(0x26,0xfb);
	sp0828_i2c_write_byte(0x28,0x61);
	sp0828_i2c_write_byte(0x29,0x49);
	sp0828_i2c_write_byte(0x31,0x64);
	sp0828_i2c_write_byte(0x32,0x18);
	sp0828_i2c_write_byte(0x4d,0xdc);
	sp0828_i2c_write_byte(0x4e,0x6b);
	sp0828_i2c_write_byte(0x41,0x8c);
	sp0828_i2c_write_byte(0x42,0x66);
	sp0828_i2c_write_byte(0x55,0xff);
	sp0828_i2c_write_byte(0x56,0x00);
	sp0828_i2c_write_byte(0x59,0x82);
	sp0828_i2c_write_byte(0x5a,0x00);
	sp0828_i2c_write_byte(0x5d,0xff);
	sp0828_i2c_write_byte(0x5e,0x6f);
	sp0828_i2c_write_byte(0x57,0xff);
	sp0828_i2c_write_byte(0x58,0x00);
	sp0828_i2c_write_byte(0x5b,0xff);
	sp0828_i2c_write_byte(0x5c,0xa8);
	sp0828_i2c_write_byte(0x5f,0x75);
	sp0828_i2c_write_byte(0x60,0x00);
	sp0828_i2c_write_byte(0x2d,0x00);
	sp0828_i2c_write_byte(0x2e,0x00);
	sp0828_i2c_write_byte(0x2f,0x00);
	sp0828_i2c_write_byte(0x30,0x00);
	sp0828_i2c_write_byte(0x33,0x00);
	sp0828_i2c_write_byte(0x34,0x00);
	sp0828_i2c_write_byte(0x37,0x00);
	sp0828_i2c_write_byte(0x38,0x00);
	sp0828_i2c_write_byte(0x39,0x01);
	sp0828_i2c_write_byte(0x3a,0x07);
	sp0828_i2c_write_byte(0xfd,0x00);//;BPC
	sp0828_i2c_write_byte(0x33,0x0f);
	sp0828_i2c_write_byte(0x51,0x3f);
	sp0828_i2c_write_byte(0x52,0x09);
	sp0828_i2c_write_byte(0x53,0x00);
	sp0828_i2c_write_byte(0x54,0x00);// 4
	sp0828_i2c_write_byte(0x55,0x10);
	sp0828_i2c_write_byte(0x4f,0xFF);;//blueedge
	sp0828_i2c_write_byte(0x50,0xFF);
	sp0828_i2c_write_byte(0x56,0x70);;//smooth
	sp0828_i2c_write_byte(0x57,0x40);
	sp0828_i2c_write_byte(0x58,0x40);
	sp0828_i2c_write_byte(0x59,0x20);
	sp0828_i2c_write_byte(0x5a,0x02);
	sp0828_i2c_write_byte(0x5b,0x05);
	sp0828_i2c_write_byte(0x5c,0x30);
	sp0828_i2c_write_byte(0x65,0x03);//sharpness
	sp0828_i2c_write_byte(0x66,0x01);
	sp0828_i2c_write_byte(0x67,0x03);
	sp0828_i2c_write_byte(0x68,0x43);
	sp0828_i2c_write_byte(0x69,0x7f);
	sp0828_i2c_write_byte(0x6a,0x01);
	sp0828_i2c_write_byte(0x6b,0x03);    
	sp0828_i2c_write_byte(0x6c,0x01);
	sp0828_i2c_write_byte(0x6d,0x06);  //0x03  pzt 20111102
	sp0828_i2c_write_byte(0x6e,0x46);  //0x43 pzt 20111102
	sp0828_i2c_write_byte(0x6f,0x7f);
	sp0828_i2c_write_byte(0x70,0x01);
	sp0828_i2c_write_byte(0x71,0x03);
	sp0828_i2c_write_byte(0x72,0x10);
	sp0828_i2c_write_byte(0x73,0x03);//3
	sp0828_i2c_write_byte(0x74,0x43);
	sp0828_i2c_write_byte(0x75,0x7f);
	sp0828_i2c_write_byte(0x76,0x01);
	sp0828_i2c_write_byte(0x7f,0x96);//;colorcorrection
	sp0828_i2c_write_byte(0x80,0xf2);
	sp0828_i2c_write_byte(0x81,0xfe); 
	sp0828_i2c_write_byte(0x82,0xde);
	sp0828_i2c_write_byte(0x83,0xa3);
	sp0828_i2c_write_byte(0x84,0xff);
	sp0828_i2c_write_byte(0x85,0xea);
	sp0828_i2c_write_byte(0x86,0x81);
	sp0828_i2c_write_byte(0x87,0x14);
	sp0828_i2c_write_byte(0x88,0x3c); 
	sp0828_i2c_write_byte(0x89,0x33);
	sp0828_i2c_write_byte(0x8a,0x1f); 
	sp0828_i2c_write_byte(0x8b,0x00);// ;gamma
	sp0828_i2c_write_byte(0x8c,0x1a);
	sp0828_i2c_write_byte(0x8d,0x29);
	sp0828_i2c_write_byte(0x8e,0x41);
	sp0828_i2c_write_byte(0x8f,0x62);
	sp0828_i2c_write_byte(0x90,0x7c);
	sp0828_i2c_write_byte(0x91,0x90);
	sp0828_i2c_write_byte(0x92,0xa2);
	sp0828_i2c_write_byte(0x93,0xaf);
	sp0828_i2c_write_byte(0x94,0xba);
	sp0828_i2c_write_byte(0x95,0xc4);
	sp0828_i2c_write_byte(0x96,0xce);
	sp0828_i2c_write_byte(0x97,0xd6);
	sp0828_i2c_write_byte(0x98,0xdd);
	sp0828_i2c_write_byte(0x99,0xe4);
	sp0828_i2c_write_byte(0x9a,0xea);
	sp0828_i2c_write_byte(0x9b,0xf1);
	sp0828_i2c_write_byte(0xfd,0x01);
	sp0828_i2c_write_byte(0x8d,0xf8);
	sp0828_i2c_write_byte(0x8e,0xff);
	sp0828_i2c_write_byte(0xfd,0x00);
	sp0828_i2c_write_byte(0xca,0xcf);//;saturation
	sp0828_i2c_write_byte(0xcb,0x07);//;hist-expand
	sp0828_i2c_write_byte(0xcc,0x04);
	sp0828_i2c_write_byte(0xce,0xff);
	sp0828_i2c_write_byte(0xcf,0x10);
	sp0828_i2c_write_byte(0xd0,0x20);
	sp0828_i2c_write_byte(0xd1,0x00);
	sp0828_i2c_write_byte(0xd2,0x1c);
	sp0828_i2c_write_byte(0xd3,0x16);
	sp0828_i2c_write_byte(0xd4,0x00);
	sp0828_i2c_write_byte(0xd6,0x1c);
	sp0828_i2c_write_byte(0xd7,0x16);
	sp0828_i2c_write_byte(0xdd,0x70);// ;heq  
	sp0828_i2c_write_byte(0xde,0x90);
	sp0828_i2c_write_byte(0xb9,0x00);//;Ygamma 
	sp0828_i2c_write_byte(0xba,0x04); 
	sp0828_i2c_write_byte(0xbb,0x08); 
	sp0828_i2c_write_byte(0xbc,0x10);
	sp0828_i2c_write_byte(0xbd,0x20);
	sp0828_i2c_write_byte(0xbe,0x30);
	sp0828_i2c_write_byte(0xbf,0x40);
	sp0828_i2c_write_byte(0xc0,0x50);
	sp0828_i2c_write_byte(0xc1,0x60);
	sp0828_i2c_write_byte(0xc2,0x70);
	sp0828_i2c_write_byte(0xc3,0x80);
	sp0828_i2c_write_byte(0xc4,0x90);
	sp0828_i2c_write_byte(0xc5,0xA0);
	sp0828_i2c_write_byte(0xc6,0xB0);
	sp0828_i2c_write_byte(0xc7,0xC0);
	sp0828_i2c_write_byte(0xc8,0xD0);
	sp0828_i2c_write_byte(0xc9,0xE0);
	sp0828_i2c_write_byte(0xfd,0x01);
	sp0828_i2c_write_byte(0x89,0xf0);
	sp0828_i2c_write_byte(0x8a,0xff);
	sp0828_i2c_write_byte(0xfd,0x00);
	sp0828_i2c_write_byte(0xe8,0x30);//;AE
	sp0828_i2c_write_byte(0xe9,0x30);
	sp0828_i2c_write_byte(0xea,0x40);
	sp0828_i2c_write_byte(0xf4,0x1b);
	sp0828_i2c_write_byte(0xf5,0x97);
	sp0828_i2c_write_byte(0xec,0x4B);
	sp0828_i2c_write_byte(0xed,0x90);
	sp0828_i2c_write_byte(0xee,0x3B);
	sp0828_i2c_write_byte(0xef,0x80);
	sp0828_i2c_write_byte(0xf7,0x78);//AEtarget
	sp0828_i2c_write_byte(0xf8,0x63);//AEtarget
	sp0828_i2c_write_byte(0xf9,0x68);//AEtarget
	sp0828_i2c_write_byte(0xfa,0x53);//AEtarget
	sp0828_i2c_write_byte(0xfd,0x01);
	sp0828_i2c_write_byte(0x09,0x31);	
	sp0828_i2c_write_byte(0x0a,0x85);
	sp0828_i2c_write_byte(0x0b,0x0b);	
	sp0828_i2c_write_byte(0x14,0x20);
	sp0828_i2c_write_byte(0x15,0x0f); 

 // MCLK 2.4M fix5fps maxgain:0x70
	sp0828_i2c_write_byte(0xfd,0x00);
	sp0828_i2c_write_byte(0x05,0x0 );
	sp0828_i2c_write_byte(0x06,0x0 );
	sp0828_i2c_write_byte(0x09,0x1 );
	sp0828_i2c_write_byte(0x0a,0xe0);
	sp0828_i2c_write_byte(0xf0,0x10);
	sp0828_i2c_write_byte(0xf1,0x0 );
	sp0828_i2c_write_byte(0xf2,0x38);
	sp0828_i2c_write_byte(0xf5,0x51);
	sp0828_i2c_write_byte(0xfd,0x01);
	sp0828_i2c_write_byte(0x00,0x95);
	sp0828_i2c_write_byte(0x0f,0x39);
	sp0828_i2c_write_byte(0x16,0x39);
	sp0828_i2c_write_byte(0x17,0x85);
	sp0828_i2c_write_byte(0x18,0x8d);
	sp0828_i2c_write_byte(0x1b,0x39);
	sp0828_i2c_write_byte(0x1c,0x8d);
	sp0828_i2c_write_byte(0xb4,0xf );
	sp0828_i2c_write_byte(0xb5,0xf );
	sp0828_i2c_write_byte(0xb6,0xf );
	sp0828_i2c_write_byte(0xb9,0x40);
	sp0828_i2c_write_byte(0xba,0x4f);
	sp0828_i2c_write_byte(0xbb,0x47);
	sp0828_i2c_write_byte(0xbc,0x45);
	sp0828_i2c_write_byte(0xbd,0x43);
	sp0828_i2c_write_byte(0xbe,0x42);
	sp0828_i2c_write_byte(0xbf,0x42);
	sp0828_i2c_write_byte(0xc0,0x42);
	sp0828_i2c_write_byte(0xc1,0x41);
	sp0828_i2c_write_byte(0xc2,0x41);
	sp0828_i2c_write_byte(0xc3,0x41);
	sp0828_i2c_write_byte(0xc4,0x41);
	sp0828_i2c_write_byte(0xc5,0x41);
	sp0828_i2c_write_byte(0xc6,0x41);
	sp0828_i2c_write_byte(0xca,0x70);
	sp0828_i2c_write_byte(0xcb,0x14);
	sp0828_i2c_write_byte(0xfd,0x00);



	sp0828_i2c_write_byte(0xfd,0x00);
	sp0828_i2c_write_byte(0x32,0x15);
	sp0828_i2c_write_byte(0x34,0x66);
	sp0828_i2c_write_byte(0x35,0x04);//out format RGB565
#ifndef T_QSC1110
	sp0828_i2c_write_byte(0x47,(byte)((y>>8)&0xff));		
	sp0828_i2c_write_byte(0x48,(byte)(y&0xff) );	   
	sp0828_i2c_write_byte(0x49,(byte)((dy>>8)&0xff)); 	 
	sp0828_i2c_write_byte(0x4a,(byte)(dy&0xff));	   
	sp0828_i2c_write_byte(0x4b,(byte)(x&0xff) );	   
	sp0828_i2c_write_byte(0x4c,(byte)(dx&0xff));	  
    MSG_FATAL("sp0828_i2c_write_byte y=%d dy=%d",y,dy,0);
    MSG_FATAL("sp0828_i2c_write_byte x=%d dx=%d",x,dx,0);
	//if(internel_resize == TRUE)
	{
		sp0828_i2c_write_byte(0xe0,0x00);     //02 00 
		sp0828_i2c_write_byte(0xe1,0x60);
		sp0828_i2c_write_byte(0xe2,0x80);
		sp0828_i2c_write_byte(0xe3,0x00);
		sp0828_i2c_write_byte(0xe4,0x2c);
		sp0828_i2c_write_byte(0xe5,0x00);
		sp0828_i2c_write_byte(0xe6,0x2c);

	}

#else
	
	//sp0828_i2c_write_byte(0x47,0x0);	   
	//sp0828_i2c_write_byte(0x48,0x00);		
	//sp0828_i2c_write_byte(0x49,0x0);	   
	//sp0828_i2c_write_byte(0x4a,0x60);		
	//sp0828_i2c_write_byte(0x4b,0x5C);		
	//sp0828_i2c_write_byte(0x4c,0x80);
	sp0828_i2c_write_byte(0x47,0x0 );      
   	sp0828_i2c_write_byte(0x48,0x0 );      
   	sp0828_i2c_write_byte(0x49,0x0 );      
   	sp0828_i2c_write_byte(0x4a,0x90);      
   	sp0828_i2c_write_byte(0x4b,0x30);      
   	sp0828_i2c_write_byte(0x4c,0xc0);
   
   
   
   	sp0828_i2c_write_byte(0xe0,0x01);      
   	sp0828_i2c_write_byte(0xe1,0x60);
   	sp0828_i2c_write_byte(0xe2,0x80);
   	sp0828_i2c_write_byte(0xe3,0x00);
   	sp0828_i2c_write_byte(0xe4,0x30);
   	sp0828_i2c_write_byte(0xe5,0x00);
   	sp0828_i2c_write_byte(0xe6,0x30);
 	
#endif

	sp0828_i2c_write_byte(0x36,0x80);		
	sp0828_i2c_write_byte(0x30,0x00);			 
	
    /*Customer can adjust GAMMA, MIRROR & UPSIDEDOWN here!*/
    return TRUE;
} /* end of initialize_sp0828_registers. */

static boolean camsensor_sp0828_start( camsensor_static_params_type *camsensor_params)
{
    /* Initialize CAMIF operation mode */
    camsensor_params->camif_config.SyncMode    = CAMIF_APS;
    camsensor_params->camif_config.HSyncEdge   = CAMIF_High;
    camsensor_params->camif_config.VSyncEdge   = CAMIF_Low;
    camsensor_params->camif_efs_config.EFS_EOL = 0x0000;
    camsensor_params->camif_efs_config.EFS_SOL = 0x0000;
    camsensor_params->camif_efs_config.EFS_EOF = 0x0000;
    camsensor_params->camif_efs_config.EFS_SOF = 0x0000;


    /* ------------------  Sensor-specific Config -------------- */
    /* Make/model of sensor */
    camsensor_params->sensor_model  = CAMSENSOR_ID_SP0828;
    //use_camsensor_sic110a = TRUE;
    //use_camsensor_siv121a = FALSE;
    /* CCD or CMOS */
    camsensor_params->sensor_type   = CAMSENSOR_CMOS;
    #ifdef T_QSC1110
	camsensor_params->format = CAMIF_YCbCr_Cr_Y_Cb_Y;
	#endif
    /* BAYER or YCbCr */
    camsensor_params->output_format = CAMSENSOR_YCBCR;

    /* A pointer to the sensor name for EXIF tags */
    camsensor_params->sensor_name = camsensor_sp0828_sensor_name;

    /* Pointer to sensor suffix */

    camsensor_params->sensor_suffix=camsensor_sp0828_sensor_suffix;

    /* What is the maximum FPS that can be supported by this sensor in video mode? */
    camsensor_params->max_video_fps   = 15 * Q8;

    /* Application assigned FPS in video mode */
    camsensor_params->video_fps       = 15 * Q8;

    /* Snapshot mode operation */
    camsensor_params->max_preview_fps = CAMSENSOR_15_FPS;


    /* May be assigned with max_preview_fps or nightshot_fps. */
    camsensor_params->preview_fps     = camsensor_params->max_preview_fps;

    /* AEC is not required for YUV sensor from VFE side */
    camsensor_params->aec_enable=FALSE;
    /* AWB is not required for YUV sensor from VFE side */
    camsensor_params->awb_enable=FALSE;
    /* AF is not required for YUV sensor from VFE side */
    camsensor_params->af_enable=FALSE;
    
    camsensor_params->num_possible_frame_rates = 1;
    /* Define these frame rates */
    /* By convention, the highest frame rate will be first in the
    array (zeroth index) and the lowest last (in order). */
    camsensor_params->frame_rate_array[0].fps = (uint16) (15*256.0); /* Q8 */
    camsensor_params->frame_rate_array[0].use_in_auto_frame_rate = TRUE;
    
    camsensor_params->frame_rate_array[1].fps = (uint16) (30.0*256.0); /* Q8 */
    camsensor_params->frame_rate_array[1].use_in_auto_frame_rate = FALSE;

    /* Sensor output capability */
    camsensor_params->full_size_width  = CAMSENSOR_SP0828_FULL_SIZE_WIDTH;
    camsensor_params->full_size_height = CAMSENSOR_SP0828_FULL_SIZE_HEIGHT;
    
    camsensor_params->qtr_size_width  =   CAMSENSOR_SP0828_FULL_SIZE_WIDTH;
    camsensor_params->qtr_size_height =  CAMSENSOR_SP0828_FULL_SIZE_HEIGHT;
    camsensor_params->pclk_invert     =  camsensor_info.pclk_invert;
    
#if defined FEATURE_WHITE_LED_FLASH || defined FEATURE_STROBE_FLASH
    camsensor_params->support_auto_flash = FALSE;
#else
    camsensor_params->support_auto_flash = FALSE;
#endif

    return TRUE;
} /*camsensor_sp0828_start*/

static boolean sp0828_i2c_write_byte(uint8 offset, uint8 data) 
{
    static uint8 writebyte;
    uint8 i;

    writebyte  = data;
    
    camsensor_i2c_command.addr.reg = offset;
    camsensor_i2c_command.buf_ptr  = (byte *)(&writebyte);
    camsensor_i2c_command.len      = 1;

    for (i = 0; i < 3; ++i)
    {
        if (i2c_write(&camsensor_i2c_command) == I2C_SUCCESS)
        {
            return TRUE;
        }
    }
    return FALSE;
}

static boolean sp0828_i2c_read_byte(uint8 offset, uint8 *data) 
{
    static uint8 readbyte; 
    uint8  i;

    if (data == NULL)
    {
        return FALSE;
    }
    
    camsensor_i2c_command.addr.reg = offset;
    camsensor_i2c_command.buf_ptr  = (byte *)(&readbyte);
    camsensor_i2c_command.len      = 1;

    for (i =0; i < 3; ++i)
    {
        if (i2c_read(&camsensor_i2c_command) == I2C_SUCCESS)
        {
            *data  = readbyte;
            return TRUE;
        }
    }
    return FALSE;
} /* end of sic110a_i2c_read_word. */

static camera_ret_code_type camsensor_sp0828_set_effect(int8 effect)
{
    return CAMERA_NOT_SUPPORTED;
} /* camsensor_sic110a_set_effect() */

/*===========================================================================

FUNCTION      camsensor_sic110a_snapshot_config

DESCRIPTION
              Configure the camera sensor for Snapshot 

DEPENDENCIES
  None

RETURN VALUE
  TRUE if successful
  FALSE otherwise

SIDE EFFECTS
  None

===========================================================================*/

static boolean camsensor_sp0828_snapshot_config( camsensor_static_params_type  *camsensor_params)
{
	uint16 x,y;
    uint8 internel_resize = FALSE;
    /* Set the current dimensions */
    camsensor_params->camsensor_width  = camera_dx;
    camsensor_params->camsensor_height = camera_dy;
    
    /* CAMIF frame */
    camsensor_params->camif_frame_config.pixelsPerLine = camera_dx*2;
    camsensor_params->camif_frame_config.linesPerFrame = camera_dy;
    
    /* CAMIF Window */
    camsensor_params->camif_window_width_config.firstPixel = 0; //按CLK
    camsensor_params->camif_window_width_config.lastPixel  = camsensor_params->camif_window_width_config.firstPixel+(camera_dx*2);
    
    camsensor_params->camif_window_height_config.firstLine = 0;
    camsensor_params->camif_window_height_config.lastLine  = camsensor_params->camif_window_height_config.firstLine+camera_dy;
    camsensor_params->pixel_clock = 1;

    if(camera_dx > CAMSENSOR_SP0828_FULL_SIZE_WIDTH)
    {
        camera_dx = CAMSENSOR_SP0828_FULL_SIZE_WIDTH;
    }
    
    if(camera_dy > CAMSENSOR_SP0828_FULL_SIZE_HEIGHT)
    {
        camera_dy = CAMSENSOR_SP0828_FULL_SIZE_HEIGHT;
    }
    
    x = (CAMSENSOR_SP0828_FULL_SIZE_WIDTH-camera_dx)>>1;
    y = (CAMSENSOR_SP0828_FULL_SIZE_HEIGHT-camera_dy)>>1;

    MSG_FATAL("sp0828_i2c_write_byte y=%d dy=%d",y,camera_dy,0);
    MSG_FATAL("sp0828_i2c_write_byte x=%d dx=%d",x,camera_dx,0);
    if(x<60)
	{
		x = 60;
	}
	if((camera_dx<128)&&(camera_dy<228)) //sensor internel resize for inprove sharpness by xuegang  
	{
		camera_dx=camera_dx*1.4;
		camera_dy=camera_dy*1.4;
		internel_resize = TRUE;
	}

    MSG_FATAL("sp0828_i2c_write_byte y=%d dy=%d",y,camera_dy,0);
    MSG_FATAL("sp0828_i2c_write_byte x=%d dx=%d",x,camera_dx,0);


    if (camera_dy==320||camera_dx==240)
    {
        sp0828_i2c_write_byte(0x47,0x0);		
    	sp0828_i2c_write_byte(0x48,0x0);	   
    	sp0828_i2c_write_byte(0x49,0x1 ); 	 
    	sp0828_i2c_write_byte(0x4a,0x40);	   
    	sp0828_i2c_write_byte(0x4b,0x00);	   
    	sp0828_i2c_write_byte(0x4c,0xf0);
    }
    else
    {
        sp0828_i2c_write_byte(0x47,(byte)((y>>8)&0xff) );		
    	sp0828_i2c_write_byte(0x48,(byte)(y&0xff) );	   
    	sp0828_i2c_write_byte(0x49,(byte)((camera_dy>>8)&0xff) ); 	 
    	sp0828_i2c_write_byte(0x4a,(byte)(camera_dy&0xff));	   
    	sp0828_i2c_write_byte(0x4b,(byte)(x&0xff));	   
    	sp0828_i2c_write_byte(0x4c,(byte)(camera_dx&0xff));	
    }
 
    return TRUE;
}
/*===========================================================================

FUNCTION      camsensor_sic110a_raw_snapshot_config

DESCRIPTION
              Configure the camera sensor for RAW Snapshot

DEPENDENCIES
  None

RETURN VALUE
  TRUE if successful
  FALSE otherwise

SIDE EFFECTS
  None

===========================================================================*/
static boolean camsensor_sp0828_raw_snapshot_config(camsensor_static_params_type  *camsensor_params)
{
    MSG_FATAL("camsensor_sp0828_raw_snapshot_config!",0,0,0);
    return TRUE;
}

/*===========================================================================

FUNCTION      camsensor_sic110a_video_config

DESCRIPTION
              Configure the camera sensor for Preview

DEPENDENCIES
  None

RETURN VALUE
  TRUE if successful
  FALSE otherwise

SIDE EFFECTS
  None

===========================================================================*/
static boolean camsensor_sp0828_video_config(camsensor_static_params_type *camsensor_params)
{
        /* Set the current dimensions */

    /* Sensor output data format */
    #ifdef T_QSC1110
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
	MSG_FATAL("camsensor_params->camsensor_width=====%d",camsensor_params->camsensor_width,0,0);
	MSG_FATAL("camsensor_params->camsensor_height=====%d",camsensor_params->camsensor_height,0,0);
	camsensor_params->camif_window_width_config.firstPixel = 0;
	camsensor_params->camif_window_width_config.lastPixel  = camsensor_params->camif_window_width_config.firstPixel + camsensor_params->camsensor_width*2 - 1;
	camsensor_params->camif_window_height_config.firstLine = 0;
	camsensor_params->camif_window_height_config.lastLine = camsensor_params->camif_window_height_config.firstLine + camsensor_params->camsensor_height*2 - 1;

	//	camsensor_sp0828_ycbcr_write_sensor (camsensor_preview_resolution);//yty add
	camsensor_current_resolution = camsensor_preview_resolution;
	#else
	 /* Set the current dimensions */
    camsensor_params->camsensor_width  = camera_preview_dx;
    camsensor_params->camsensor_height = camera_preview_dy;

    /* CAMIF frame */
    camsensor_params->camif_frame_config.pixelsPerLine =camera_preview_dx*2;
    camsensor_params->camif_frame_config.linesPerFrame =camera_preview_dy;
    
    /* CAMIF Window */
    camsensor_params->camif_window_width_config.firstPixel = 0; //按CLK
    camsensor_params->camif_window_width_config.lastPixel  = camsensor_params->camif_window_width_config.firstPixel+(camera_preview_dx*2);

    camsensor_params->camif_window_height_config.firstLine = 0;
    camsensor_params->camif_window_height_config.lastLine  = camsensor_params->camif_window_height_config.firstLine+camera_preview_dy;
    camsensor_params->pixel_clock = 1;
    initialize_sp0828_registers(camera_preview_dx, camera_preview_dy);
	#endif
    return TRUE;
}

static void camsensor_sp0828_power_up(void)
{
    ERR("camsensor_sp0828_power_up!",0,0,0);
} /* camsensor_sp0828_power_up */

/*===========================================================================

FUNCTION      camsensor_sic110a_power_down

DESCRIPTION
Power down the sensor, using I2C driver.

DEPENDENCIES
None

RETURN VALUE
None

SIDE EFFECTS
None

===========================================================================*/
static void camsensor_sp0828_power_down(void)
{
	sp0828_i2c_write_byte(0xfd, 0x00);
    sp0828_i2c_write_byte(0x03, 0x3e);
    sp0828_i2c_write_byte(0x1c, 0x0f);//sensor sleep mode
    MSG_FATAL("camsensor_sp0828_power_down!",0,0,0);
} /* camsensor_sic110a_power_down */

static camera_ret_code_type camsensor_sp0828_set_wb(int8 wb)
{
    MSG_FATAL("camsensor_sp0828_set_wb!",0,0,0);
    switch((camera_wb_type)wb)
    {
        case CAMERA_WB_AUTO:
        {
			//         自动
			sp0828_i2c_write_byte(0xfd,0x01);                                                          
			sp0828_i2c_write_byte(0x28,0x61);		                                                       
			sp0828_i2c_write_byte(0x29,0x49);
			sp0828_i2c_write_byte(0xfd,0x00);  // AUTO 3000K~7000K                                     
			sp0828_i2c_write_byte(0x32,0x15);                                                                                                                                                                      
            break;
        }
        
        case CAMERA_WB_INCANDESCENT:
        {
			//  白炽灯 
			sp0828_i2c_write_byte(0xfd,0x00);  //2800K~3000K                                     
			sp0828_i2c_write_byte(0x32,0x05);                                                          
			sp0828_i2c_write_byte(0xfd,0x01);                                                          
			sp0828_i2c_write_byte(0x28,0x41);		                                                       
			sp0828_i2c_write_byte(0x29,0x71);		                                                                                                            
			sp0828_i2c_write_byte(0xfd,0x00);                                                          
            break;
        }
        
        case CAMERA_WB_FLUORESCENT:
        {
			//  荧光灯 
			sp0828_i2c_write_byte(0xfd,0x00);  //4200K~5000K                                     
			sp0828_i2c_write_byte(0x32,0x05);                                                          
			sp0828_i2c_write_byte(0xfd,0x01);                                                          
			sp0828_i2c_write_byte(0x28,0x5a);		                                                       
			sp0828_i2c_write_byte(0x29,0x62);		                                                                                                             
			sp0828_i2c_write_byte(0xfd,0x00);                                                         
            break;
        }
        
        case CAMERA_WB_DAYLIGHT:
        {
			//   白天 
			sp0828_i2c_write_byte(0xfd,0x00);  //6500K                                     
			sp0828_i2c_write_byte(0x32,0x05);                                                          
			sp0828_i2c_write_byte(0xfd,0x01);                                                          
			sp0828_i2c_write_byte(0x28,0x6b);		                                                       
			sp0828_i2c_write_byte(0x29,0x48);		                                                                                                             
			sp0828_i2c_write_byte(0xfd,0x00);                                                         
            break;
        }
        
        case CAMERA_WB_CLOUDY_DAYLIGHT:
        {
			//    阴天
			sp0828_i2c_write_byte(0xfd,0x00);   //7000K                                     
			sp0828_i2c_write_byte(0x32,0x05);                                                          
			sp0828_i2c_write_byte(0xfd,0x01);                                                          
			sp0828_i2c_write_byte(0x28,0x71);		                                                       
			sp0828_i2c_write_byte(0x29,0x41);		                                                                                                 
			sp0828_i2c_write_byte(0xfd,0x00);                                             
            break;
        }
        
        case CAMERA_WB_TWILIGHT:
        case CAMERA_WB_SHADE :
        case CAMERA_WB_CUSTOM:
        default:
            return CAMERA_NOT_SUPPORTED;
    }//end of switch

    return CAMERA_SUCCESS;
}

static camera_ret_code_type camsensor_sp0828_set_ev_compensation(int32 compensation)
{
    camera_ret_code_type ret_val = CAMERA_SUCCESS;
    int16  parmCurNum = (int16)(compensation>>16);
    int32 max_parmVal = 12;
    int32 min_parmVal =-12;
    int32  parmVal= parmCurNum;
    uint32 new_luma_target;

    MSG_FATAL("camsensor_sp0828_set_ev_compensation!",0,0,0);
    if ( parmVal > max_parmVal )
    {
        ret_val =  CAMERA_INVALID_PARM;
    }
    else if ( parmVal < min_parmVal )
    {
        ret_val =  CAMERA_INVALID_PARM;
    }
    
    new_luma_target = (uint32) parmVal + max_parmVal;
    
    if( new_luma_target >= MAX_EV_COMP)
    {
        ret_val=CAMERA_INVALID_PARM;
    }
    else
    {
        sp0828_i2c_write_byte(0x00,0x02); //bank 1
        switch(new_luma_target)
        {
            case 8:    // -4 EV
				sp0828_i2c_write_byte(0xfd, 0x00);
				sp0828_i2c_write_byte(0xdc, 0xc0);
                break;

            case 9:    // -3 EV
				sp0828_i2c_write_byte(0xfd, 0x00);
				sp0828_i2c_write_byte(0xdc, 0xd0);
                break;

            case 10:    // -2 EV
				sp0828_i2c_write_byte(0xfd, 0x00);
				sp0828_i2c_write_byte(0xdc, 0xe0);
                break;

            case 11:    // -1 EV
				sp0828_i2c_write_byte(0xfd, 0x00);
				sp0828_i2c_write_byte(0xdc, 0xf0);
                break;

            case 12:   // +0 EV
				sp0828_i2c_write_byte(0xfd, 0x00);
				sp0828_i2c_write_byte(0xdc, 0x00);
                break;

            case 13:    // +1 EV
				sp0828_i2c_write_byte(0xfd, 0x00);
				sp0828_i2c_write_byte(0xdc, 0x10);
                break;

            case 14:    // +2 EV
				sp0828_i2c_write_byte(0xfd, 0x00);
				sp0828_i2c_write_byte(0xdc, 0x20);
                break;

            case 15:    // +3 EV
				sp0828_i2c_write_byte(0xfd, 0x00);
				sp0828_i2c_write_byte(0xdc, 0x30);
                break;

            case 16:    // +4 EV
				sp0828_i2c_write_byte(0xfd, 0x00);
				sp0828_i2c_write_byte(0xdc, 0x40);
                break;

            default:
                break;
        }
    }
    
    return ret_val;
}

static camera_ret_code_type camsensor_sp0828_set_brightness(int8 brightness)
{
    ERR("camsensor_sp0828_set_brightness!",0,0,0);
    return CAMERA_SUCCESS;
}


static camera_ret_code_type camsensor_sp0828_set_contrast(int8 contrast)
{
    MSG_FATAL("camsensor_sp0828_set_contrast!",0,0,0);
    return CAMERA_SUCCESS;
}/* camsensor_sic110a_set_contrast() */

static void camsensor_sp0828_setup_camctrl_tbl(camctrl_tbl_type *camctrl_tbl_ptr)
{
    MSG_FATAL("camsensor_sp0828_setup_camctrl_tbl!",0,0,0);
    camctrl_tbl_ptr->asf_5x5_is_supported         = FALSE; 
    camctrl_tbl_ptr->get_snapshot_frame_per_sec = camsensor_sp0828_get_snapshot_fps;
}

static uint16 camsensor_sp0828_get_snapshot_fps(uint16 fps)
{
    MSG_FATAL("camsensor_sp0828_get_snapshot_fps!",0,0,0);
    /* In the current driver, the snapshot fps is always 11 */
    fps = 15 * Q8;
    return fps;
}

camera_ret_code_type camsensor_sp0828_set_antibanding(int8 antibanding)
{
    MSG_FATAL("camsensor_sp0828_set_antibanding!",0,0,0);
    return CAMERA_SUCCESS;
}

static void camsensor_sp0828_register(camsensor_function_table_type *camsensor_function_table_ptr)
{
    MSG_FATAL("camsensor_sp0828_register!",0,0,0);
    camsensor_function_table_ptr->camsensor_start                 = camsensor_sp0828_start;
    camsensor_function_table_ptr->camsensor_video_config          = camsensor_sp0828_video_config;
    camsensor_function_table_ptr->camsensor_raw_snapshot_config   = camsensor_sp0828_raw_snapshot_config;
    camsensor_function_table_ptr->camsensor_snapshot_config       = camsensor_sp0828_snapshot_config;
    camsensor_function_table_ptr->camsensor_power_up              = camsensor_sp0828_power_up;
    camsensor_function_table_ptr->camsensor_power_down            = camsensor_sp0828_power_down;
    camsensor_function_table_ptr->camsensor_set_frame_rate        = camsensor_sp0828_set_frame_rate;  

    camsensor_function_table_ptr->camsensor_set_effect            = camsensor_sp0828_set_effect;

    camsensor_function_table_ptr->camsensor_set_brightness        = camsensor_sp0828_set_brightness;
    camsensor_function_table_ptr->camsensor_set_contrast          = camsensor_sp0828_set_contrast;
    camsensor_function_table_ptr->camsensor_set_antibanding       = camsensor_sp0828_set_antibanding;
    camsensor_function_table_ptr->camsensor_set_exposure_mode     = camsensor_sp0828_set_exposure_mode;
    camsensor_function_table_ptr->camsensor_set_ev_compensation   = camsensor_sp0828_set_ev_compensation; 
    camsensor_function_table_ptr->camsensor_set_wb                = camsensor_sp0828_set_wb;
} /* camsensor_mt9d112_mu2m0yu_register */
#endif

