#ifndef __GP2A_H__
#define __GP2A_H__

#define I2C_M_WR 0		/* for i2c Write */
#define I2c_M_RD 1		/* for i2c Read */

#define I2C_DF_NOTIFY			0x01	/* for i2c */

/* Registers */
#define COMMAND1    0x00
#define COMMAND2    0x01	/* Read&Write */
#define COMMAND3    0x02	/* Read&Write */
#define COMMAND4    0x03	/* Read&Write */
#define INT_LT_LSB  0x04	/* Read&Write */
#define INT_LT_MSB  0x05	/* Read&Write */
#define INT_HT_LSB  0x06	/* Read&Write */
#define INT_HT_MSB  0x07	/* Read&Write */
#define PS_LT_LSB   0x08	/* Read&Write */
#define PS_LT_MSB   0x09	/* Read&Write */
#define PS_HT_LSB   0x0A	/* Read&Write */
#define PS_HT_MSB   0x0B	/* Read&Write */
#define DATA0_LSB   0x0C	/* Read Only */
#define DATA0_MSB   0x0D	/* Read Only */
#define DATA1_LSB   0x0E	/* Read Only */
#define DATA1_MSB   0x0F	/* Read Only */
#define DATA2_LSB   0x10	/* Read Only */
#define DATA2_MSB   0x11	/* Read Only */

#define ADC_BUFFER_NUM	6

#define DEFAULT_LO_THR	0x07 /* sharp recommand Loff */
#define DEFAULT_HI_THR	0x08 /* sharp recommand Lon */

#define OFFSET_ARRAY_LENGTH		10

/* 16 level for premium model*/
enum {
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
};

/* extern variables */
extern u8 lightsensor_mode; /* 0 = low, 1 = high */
extern int proximity_enable;
extern char proximity_sensor_detection;

/* prototype */
int opt_i2c_read(u8 reg, unsigned char *rbuf, int len);
int opt_i2c_write(u8 reg, u8 *val);
#ifndef CONFIG_MACH_KONA_SENSOR
int lightsensor_get_adcvalue(void);
#endif
int is_gp2a030a(void);

struct gp2a_platform_data {
	int (*gp2a_led_on) (bool);
	int p_out; /* proximity-sensor-output gpio */
#ifdef CONFIG_MACH_KONA_SENSOR
	unsigned long (*gp2a_get_threshold)(u8 *);
#else
	unsigned long (*gp2a_get_threshold)(void);
#endif
};
#endif
