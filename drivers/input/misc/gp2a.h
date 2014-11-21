#ifndef __GP2A_H__
#define __GP2A_H__

#define I2C_M_WR 0 /* for i2c */
#define I2c_M_RD 1 /* for i2c */


//#define DELAY_PRX 1

#define IRQ_GP2A_INT IRQ_EINT2  /*s3c64xx int number */

#define I2C_DF_NOTIFY			0x01 /* for i2c */

//ADDSEL is LOW
#define GP2A_ADDR		0x88 /* slave addr for i2c */

#define REGS_PROX 	    0x0 // Read  Only
#define REGS_GAIN    	0x1 // Write Only
#define REGS_HYS		0x2 // Write Only
#define REGS_CYCLE		0x3 // Write Only
#define REGS_OPMOD		0x4 // Write Only
#if defined(CONFIG_GP2A_MODE_B)
#define REGS_CON	0x6 // Write Only
#endif

/* sensor type */
#define LIGHT           0
#define PROXIMITY		1
#define ALL				2

/* power control */
#define ON              1
#define OFF				0

/* IOCTL for proximity sensor */
#define SHARP_GP2AP_IOC_MAGIC   'C'                                 
#define SHARP_GP2AP_OPEN    _IO(SHARP_GP2AP_IOC_MAGIC,1)            
#define SHARP_GP2AP_CLOSE   _IO(SHARP_GP2AP_IOC_MAGIC,2)      

/* input device for proximity sensor */
#define USE_INPUT_DEVICE 	0  /* 0 : No Use  ,  1: Use  */


#define INT_CLEAR    1 /* 0 = by polling operation, 1 = by interrupt operation */
#define LIGHT_PERIOD 1 /* per sec */
#define ADC_CHANNEL  9 /* index for s5pC110 9번 channel adc */ 

/*for light sensor */
#define STATE_NUM			3   /* number of states */
#define LIGHT_LOW_LEVEL     1    /* brightness of lcd */
#define LIGHT_MID_LEVEL 	 11    
#define LIGHT_HIGH_LEVEL    23

#define ADC_BUFFER_NUM	6


#define GUARDBAND_BOTTOM_ADC    700
#define GUARDBAND_TOP_ADC    800




/*
 * STATE0 : 30 lux below
 * STATE1 : 31~ 3000 lux
 * STATE2 : 3000 lux over
 */




#define ADC_CUT_HIGH 1100            /* boundary line between STATE_0 and STATE_1 */
#define ADC_CUT_LOW  220            /* boundary line between STATE_1 and STATE_2 */
#define ADC_CUT_GAP  50            /* in order to prevent chattering condition */

#define LIGHT_FOR_16LEVEL 


/* 16 level for premium model*/
typedef enum t_light_state
{
	LIGHT_DIM   = 0,
	LIGHT_LEVEL1   = 1,
	LIGHT_LEVEL2   = 2,
	LIGHT_LEVEL3   = 3,
	LIGHT_LEVEL4   = 4,
	LIGHT_LEVEL5   = 5,
	LIGHT_LEVEL6   = 6,
	LIGHT_LEVEL7   = 7,
	LIGHT_LEVEL8   = 8,
	LIGHT_LEVEL9   = 9,
	LIGHT_LEVEL10   = 10,
	LIGHT_LEVEL11   = 11,
	LIGHT_LEVEL12   = 12,
	LIGHT_LEVEL13   = 13,	
	LIGHT_LEVEL14   = 14,		
	LIGHT_LEVEL15   = 15,	
	LIGHT_LEVEL16   = 16,	
	LIGHT_INIT  = 17,	
}state_type;

/* initial value for sensor register */
static u8 gp2a_original_image[8] =
{
#if defined(CONFIG_GP2A_MODE_B)
	0x00,	// REGS_PROX
	0x08,	// REGS_GAIN
	0x40,	// REGS_HYS
	0x04,	// REGS_CYCLE
	0x03,	// REGS_OPMOD
#else
	0x00,  
	0x08,  
	0xC2,  
	0x04,
	0x01,
#endif
};

/* for state transition */
struct _light_state {
	state_type type;
	int adc_bottom_limit;
	int adc_top_limit;
	int brightness;

};

/* driver data */
struct gp2a_data {
	struct input_dev *input_dev;
	struct work_struct work_prox;  /* for proximity sensor */
	struct work_struct work_light; /* for light_sensor     */
	int             irq;
    struct hrtimer timer;
	struct timer_list light_init_timer;

};


struct workqueue_struct *gp2a_wq;
#if defined(CONFIG_GP2A_MODE_B)
struct workqueue_struct *gp2a_wq_prox;
#endif

/* prototype */
extern short gp2a_get_proximity_value(void);
extern bool gp2a_get_lightsensor_status(void);
int opt_i2c_read(u8 reg, u8 *val, unsigned int len );
int opt_i2c_write( u8 reg, u8 *val );
extern int s3c_adc_get_adc_data(int channel);
void lightsensor_adjust_brightness(int level);
extern int lightsensor_backlight_level_ctrl(int value);
static int proximity_open(struct inode *ip, struct file *fp);
static int proximity_release(struct inode *ip, struct file *fp);
static int light_open(struct inode *ip, struct file *fp);
static int light_release(struct inode *ip, struct file *fp);

int lightsensor_get_adcvalue(void);


#endif
