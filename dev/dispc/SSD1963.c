/**
 * @file SSD1963.c
 * 
 */

/*********************
 *      INCLUDES
 *********************/
#include "hw_conf.h"
#if USE_SSD1963 != 0

#include <stdbool.h>
#include "SSD1963.h"
#include "hw/per/par.h"
#include "hw/per/io.h"
#include "hw/per/tick.h"
#include "misc/gfx/color.h"

/*********************
 *      DEFINES
 *********************/
#define SSD1963_CMD_MODE     0
#define SSD1963_DATA_MODE    1

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static inline void ssd1963_cmd_mode(void);
static inline void ssd1963_data_mode(void);
static inline void ssd1963_cmd(uint8_t cmd);
static inline void ssd1963_data(uint8_t data);
static void ssd1963_io_init(void);
static void ssd1963_reset(void);
static void ssd1963_set_clk(void);
static void ssd1963_set_tft_spec(void);
static void ssd1963_init_bl(void);

/**********************
 *  STATIC VARIABLES
 **********************/
static bool cmd_mode = true;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void ssd1963_init(void)
{
    ssd1963_io_init();
    ssd1963_reset();
    ssd1963_set_clk();
    ssd1963_set_tft_spec();
    ssd1963_init_bl();
    
    
    ssd1963_cmd(0x13);		//SET display on


    ssd1963_cmd(0x29);		//SET display on
    tick_wait_ms(30);        
    
}

void ssd1963_fill(int32_t x1, int32_t y1, int32_t x2, int32_t y2, color_t color)
{
    /*Return if the area is out the screen*/
    if(x2 < 0) return;
    if(y2 < 0) return;
    if(x1 > SSD1963_HOR_RES - 1) return;
    if(y1 > SSD1963_VER_RES - 1) return;

    /*Truncate the area to the screen*/
    int32_t act_x1 = x1 < 0 ? 0 : x1;
    int32_t act_y1 = y1 < 0 ? 0 : y1;
    int32_t act_x2 = x2 > SSD1963_HOR_RES - 1 ? SSD1963_HOR_RES - 1 : x2;
    int32_t act_y2 = y2 > SSD1963_VER_RES - 1 ? SSD1963_VER_RES - 1 : y2;
   
    //Set the rectangular area
    ssd1963_cmd(0x002A);
    ssd1963_data(act_x1 >> 8);
    ssd1963_data(0x00FF & act_x1);
    ssd1963_data(act_x2 >> 8);
    ssd1963_data(0x00FF & act_x2);

    ssd1963_cmd(0x002B);
    ssd1963_data(act_y1 >> 8);
    ssd1963_data(0x00FF & act_y1);
    ssd1963_data(act_y2 >> 8);
    ssd1963_data(0x00FF & act_y2);

    ssd1963_cmd(0x2c);
    
    uint16_t color16 = color_to16(color);

    uint32_t size = (act_x2 - act_x1 + 1) * (act_y2 - act_y1 + 1);
    ssd1963_data_mode();
    par_wr_mult(color16, size);
}

void ssd1963_map(int32_t x1, int32_t y1, int32_t x2, int32_t y2, color_t * color_p)
{
    
    /*Return if the area is out the screen*/
    if(x2 < 0) return;
    if(y2 < 0) return;
    if(x1 > SSD1963_HOR_RES - 1) return;
    if(y1 > SSD1963_VER_RES - 1) return;

    /*Truncate the area to the screen*/
    int32_t act_x1 = x1 < 0 ? 0 : x1;
    int32_t act_y1 = y1 < 0 ? 0 : y1;
    int32_t act_x2 = x2 > SSD1963_HOR_RES - 1 ? SSD1963_HOR_RES - 1 : x2;
    int32_t act_y2 = y2 > SSD1963_VER_RES - 1 ? SSD1963_VER_RES - 1 : y2;
   
    //Set the rectangular area
    ssd1963_cmd(0x002A);
    ssd1963_data(act_x1 >> 8);
    ssd1963_data(0x00FF & act_x1);
    ssd1963_data(act_x2 >> 8);
    ssd1963_data(0x00FF & act_x2);

    ssd1963_cmd(0x002B);
    ssd1963_data(act_y1 >> 8);
    ssd1963_data(0x00FF & act_y1);
    ssd1963_data(act_y2 >> 8);
    ssd1963_data(0x00FF & act_y2);

    ssd1963_cmd(0x2c);
     int16_t i;
    uint16_t act_w = act_x2 - act_x1 + 1;
    uint16_t last_w = x2 - x1 + 1;
    
    ssd1963_data_mode();
    
#if COLOR_DEPTH == 16
    for(i = act_y1; i <= act_y2; i++) {
        par_wr_array((uint16_t*)color_p, act_w);
        color_p += last_w;
    }
#else
    int16_t j;
    for(i = act_y1; i <= act_y2; i++) {
        for(j = 0; j <= act_x2 - act_x1 + 1; j++) {
            par_wr(color_to16(color_p[j]));
            color_p += last_w;
        }
    }
#endif
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void ssd1963_io_init(void)
{
    io_set_pin_dir(SSD1963_RST_PORT, SSD1963_RST_PIN, IO_DIR_OUT);   
    io_set_pin_dir(SSD1963_BL_PORT, SSD1963_BL_PIN, IO_DIR_OUT);
    io_set_pin_dir(SSD1963_RS_PORT, SSD1963_RS_PIN, IO_DIR_OUT);
    io_set_pin(SSD1963_RST_PORT, SSD1963_RST_PIN, 1);
    io_set_pin(SSD1963_BL_PORT, SSD1963_BL_PIN, 0);
    io_set_pin(SSD1963_RS_PORT, SSD1963_RS_PIN, SSD1963_CMD_MODE);
    cmd_mode = true;
}

static void ssd1963_reset(void)
{
    /*Hardware reset*/
    io_set_pin(SSD1963_RST_PORT, SSD1963_RST_PIN, 1);
    tick_wait_ms(50);
    io_set_pin(SSD1963_RST_PORT, SSD1963_RST_PIN, 0);
    tick_wait_ms(50);
    io_set_pin(SSD1963_RST_PORT, SSD1963_RST_PIN, 1);
    tick_wait_ms(50);

    /*Chip enable*/
    par_cs_dis(SSD1963_PAR_CS);
    tick_wait_ms(10);
    par_cs_en(SSD1963_PAR_CS);
    tick_wait_ms(5);
    
    /*Software reset*/
    ssd1963_cmd(0x01);
    tick_wait_ms(20);

    ssd1963_cmd(0x01);
    tick_wait_ms(20);

    ssd1963_cmd(0x01);
    tick_wait_ms(20);
    
}

static void ssd1963_set_clk(void)
{
    /* Set PLL*/    
    ssd1963_cmd(0xe2);          
    ssd1963_data(0x23);
    ssd1963_data(0x05);
    ssd1963_data(0x54);

    /*Enable PLL*/
    ssd1963_cmd(0xe0);     
    ssd1963_data(0x01);
    tick_wait_ms(20);

    /*Lock PLL*/
    ssd1963_cmd(0xe0);        
    ssd1963_data(0x03);

    /*Software reset*/
    ssd1963_cmd(0x01);    
    tick_wait_ms(20);

    /*Set PCLK freq*/
    ssd1963_cmd(0xe6); 
    ssd1963_data(0x04);
    ssd1963_data(0x93);
    ssd1963_data(0xe0);
}

static void ssd1963_set_tft_spec(void)
{
    
    ssd1963_cmd(0xB0);	//LCD SPECIFICATION
    ssd1963_data(0x20);

    ssd1963_data(0x00);
    ssd1963_data(((SSD1963_HOR_RES - 1) >> 8) & 0XFF);  //Set HDP
    ssd1963_data((SSD1963_HOR_RES - 1) & 0XFF);
    ssd1963_data(((SSD1963_VER_RES - 1) >> 8) & 0XFF);  //Set VDP
    ssd1963_data((SSD1963_VER_RES - 1) & 0XFF);
    ssd1963_data(0x00);

    ssd1963_cmd(0xB4);	//HSYNC
    ssd1963_data((SSD1963_HT >> 8) & 0XFF);  //Set HT
    ssd1963_data(SSD1963_HT & 0XFF);
    ssd1963_data((SSD1963_HPS >> 8) & 0XFF);  //Set HPS
    ssd1963_data(SSD1963_HPS & 0XFF);
    ssd1963_data(SSD1963_HPW);			   //Set HPW
    ssd1963_data((SSD1963_LPS >> 8) & 0XFF);  //Set HPS
    ssd1963_data(SSD1963_LPS & 0XFF);
    ssd1963_data(0x00);

    ssd1963_cmd(0xB6);	//VSYNC
    ssd1963_data((SSD1963_VT >> 8) & 0XFF);   //Set VT
    ssd1963_data(SSD1963_VT & 0XFF);
    ssd1963_data((SSD1963_VPS >> 8) & 0XFF);  //Set VPS
    ssd1963_data(SSD1963_VPS & 0XFF);
    ssd1963_data(SSD1963_VPW);			   //Set VPW
    ssd1963_data((SSD1963_FPS >> 8) & 0XFF);  //Set FPS
    ssd1963_data(SSD1963_FPS & 0XFF);
    
    ssd1963_cmd(0xf0);            //SET pixel data I/F format=16bit
    ssd1963_data(0x03);
//    ssd1963_cmd(0x3a);      //RGB pixel format
//    ssd1963_data(0x66);      //24bpp (0x60: 18bpp)

    ssd1963_cmd(0x36);     	 //SET address mode=flip vertical
#if SSD1963_ORI == 0
    ssd1963_data(0x03);
#else
    ssd1963_data(0x00);
#endif


//     ssd1963_cmd(0x26);      //gamma curve
//    ssd1963_data(0x08);

}

static void ssd1963_init_bl(void)
{

    ssd1963_cmd(0xBE);// Set PWM configuration for backlight control

    ssd1963_data(0x02);			// PWMF[7:0] = 2, PWM base freq = PLL/(256*(1+5))/256 =
                                                            // 300Hz for a PLL freq = 120MHz
    ssd1963_data(0x20);		// Set duty cycle, from 0x00 (total pull-down) to 0xFF
                                                            // (99% pull-up , 255/256)
    ssd1963_data(0x01);			// PWM enabled and controlled by host (mcu)
    ssd1963_data(0x00);
    ssd1963_data(0x00);
    ssd1963_data(0x00);

    io_set_pin(SSD1963_BL_PORT, SSD1963_BL_PIN, 1);
    
}


/**
 * Command mode
 */
static inline void ssd1963_cmd_mode(void)
{
    if(cmd_mode == false) {
        io_set_pin(SSD1963_RS_PORT, SSD1963_RS_PIN, SSD1963_CMD_MODE);
        cmd_mode = true;
    }
}

/**
 * Data mode
 */
static inline void ssd1963_data_mode(void)
{
    if(cmd_mode != false) {
        io_set_pin(SSD1963_RS_PORT, SSD1963_RS_PIN, SSD1963_DATA_MODE);
        cmd_mode = false;
    }
}

/**
 * Write command
 * @param cmd the command
 */
static inline void ssd1963_cmd(uint8_t cmd)
{    
    ssd1963_cmd_mode();
    par_wr(cmd);    
}

/**
 * Write data
 * @param data the data
 */
static inline void ssd1963_data(uint8_t data)
{    
    ssd1963_data_mode();
    par_wr(data);    
}

#endif
