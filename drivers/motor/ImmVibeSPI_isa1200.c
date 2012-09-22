/*
** =========================================================================
** File:
**     ImmVibeSPI.c
**
** Description:
**     Device-dependent functions called by Immersion TSP API
**     to control PWM duty cycle, amp enable/disable, save IVT file, etc...
**
** Portions Copyright (c) 2008-2010 Immersion Corporation. All Rights Reserved.
**
** This file contains Original Code and/or Modifications of Original Code
** as defined in and that are subject to the GNU Public License v2 -
** (the 'License'). You may not use this file except in compliance with the
** License. You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software Foundation, Inc.,
** 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA or contact
** TouchSenseSales@immersion.com.
**
** The Original Code and all software distributed under the License are
** distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
** EXPRESS OR IMPLIED, AND IMMERSION HEREBY DISCLAIMS ALL SUCH WARRANTIES,
** INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS
** FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. Please see
** the License for the specific language governing rights and limitations
** under the License.
** =========================================================================
*/

#ifdef IMMVIBESPIAPI
#undef IMMVIBESPIAPI
#endif
#define IMMVIBESPIAPI static

#include <linux/isa1200_vibrator.h>

/*
** This SPI supports only one actuator.
*/
#define NUM_ACTUATORS 1

#define ISA1200_I2C_ADDRESS	0x90
#define SCTRL	(0)
#define HCTRL0	(0x30)
#define HCTRL1	(0x31)
#define HCTRL2	(0x32)
#define HCTRL3	(0x33)
#define HCTRL4	(0x34)
#define HCTRL5	(0x35)
#define HCTRL6	(0x36)
#define HCTRL7	(0x37)
#define HCTRL8	(0x38)
#define HCTRL9	(0x39)
#define HCTRLA	(0x3A)
#define HCTRLB	(0x3B)
#define HCTRLC	(0x3C)
#define HCTRLD	(0x3D)

#define LDO_VOLTAGE_23V	0x08
#define LDO_VOLTAGE_24V	0x09
#define LDO_VOLTAGE_25V	0x0A
#define LDO_VOLTAGE_26V	0x0B
#define LDO_VOLTAGE_27V	0x0C
#define LDO_VOLTAGE_28V	0x0D
#define LDO_VOLTAGE_29V	0x0E
#define LDO_VOLTAGE_30V	0x0F
#define LDO_VOLTAGE_31V	0x00
#define LDO_VOLTAGE_32V	0x01
#define LDO_VOLTAGE_33V	0x02
#define LDO_VOLTAGE_34V	0x03
#define LDO_VOLTAGE_35V	0x04
#define LDO_VOLTAGE_36V	0x05
#define LDO_VOLTAGE_37V	0x06
#define LDO_VOLTAGE_38V	0x07

static bool g_bAmpEnabled;

/* Variable defined to allow for tuning of the handset. */
/* For temporary section for Tuning Work */
#if 0
#define VIBETONZ_TUNING
#define ISA1200_GEN_MODE
#endif
#define MOTOR_TYPE_LRA

#ifdef VIBETONZ_TUNING
extern VibeStatus ImmVibeGetDeviceKernelParameter(VibeInt32 nDeviceIndex, VibeInt32 nDeviceKernelParamID, VibeInt32 *pnDeviceKernelParamValue);
#endif /* VIBETONZ_TUNING */

/* PWM mode selection */
#ifndef ISA1200_GEN_MODE
#define ISA1200_PWM_MODE
#if 0
#define ISA1200_PWM_256DIV_MODE
#else
#define ISA1200_PWM_128DIV_MODE
#endif
#endif

/* Actuator selection */
#ifndef MOTOR_TYPE_LRA
#define MOTOR_TYPE_ERM
#endif

#define ISA1200_HEN_ENABLE
#define RETRY_CNT 10

#define SYS_API_LEN_HIGH	vibtonz_chip_enable(true)
#define SYS_API_LEN_LOW	vibtonz_chip_enable(false)
#define SYS_API_HEN_HIGH
#define SYS_API_HEN_LOW
#define SYS_API_VDDP_ON
#define SYS_API_VDDP_OFF
#define SYS_API__I2C__Write( _addr, _dataLength, _data)	\
	vibtonz_i2c_write(_addr, _dataLength, _data)
#define SLEEP(_us_time)

#define PWM_CLK_ENABLE	vibtonz_clk_enable(true)
#define PWM_CLK_DISABLE	vibtonz_clk_enable(false)

#define DEBUG_MSG(fmt, ...) \
	printk(KERN_DEBUG pr_fmt(fmt), ##__VA_ARGS__)

#define SYS_API_SET_PWM_FREQ(freq)
#define SYS_API_SET_PWM_DUTY(ratio)	vibtonz_clk_config(ratio)

#ifdef ISA1200_GEN_MODE
#define PWM_PLLDIV_DEFAULT	0x02
#define PWM_FREQ_DEFAULT		0x00
#define PWM_PERIOD_DEFAULT	0x74
#define PWM_DUTY_DEFAULT		0x3a
VibeUInt32 g_nPWM_PLLDiv = PWM_PLLDIV_DEFAULT;
VibeUInt32 g_nPWM_Freq = PWM_FREQ_DEFAULT;
VibeUInt32 g_nPWM_Period = PWM_PERIOD_DEFAULT;
VibeUInt32 g_nPWM_Duty = PWM_DUTY_DEFAULT;
#else
#ifdef ISA1200_PWM_256DIV_MODE
#define PWM_PERIOD_DEFAULT	44800
#else
#define PWM_PERIOD_DEFAULT	22400
#endif
VibeUInt32 g_nPWM_Freq = PWM_PERIOD_DEFAULT;
#endif

#define LDO_VOLTAGE_DEFAULT LDO_VOLTAGE_30V
VibeUInt32 g_nLDO_Voltage = LDO_VOLTAGE_DEFAULT;

#define DEVICE_NAME "Generic"

/*
** Called to disable amp (disable output force)
*/
IMMVIBESPIAPI VibeStatus ImmVibeSPI_ForceOut_AmpDisable(VibeUInt8 nActuatorIndex)
{
    int cnt = 0;
    unsigned char I2C_data[2];
    int ret = VIBE_S_SUCCESS;

    if (g_bAmpEnabled)
    {
        DbgOut((KERN_DEBUG "ImmVibeSPI_ForceOut_AmpDisable.\n"));

        g_bAmpEnabled = false;


#ifdef ISA1200_HEN_ENABLE
	    SYS_API_HEN_LOW;
#endif

	    I2C_data[1] = 0x00; // Haptic drive disable
	    I2C_data[0]= HCTRL0;
	    do
	    {
	        ret = SYS_API__I2C__Write(ISA1200_I2C_ADDRESS,  2 /* data length*/,  I2C_data);
	        cnt++;
	    }while(VIBE_S_SUCCESS != ret && cnt < RETRY_CNT);
	    if( VIBE_S_SUCCESS != ret) DEBUG_MSG("[ImmVibeSPI_ForceOut_AmpDisable] I2C_Write Error,  Slave Address = [%d], ret = [%d]\n", HCTRL0, ret);

	    PWM_CLK_DISABLE;
    }

    return VIBE_S_SUCCESS;
}

/*
** Called to enable amp (enable output force)
*/
IMMVIBESPIAPI VibeStatus ImmVibeSPI_ForceOut_AmpEnable(VibeUInt8 nActuatorIndex)
{
    int cnt = 0;
    unsigned char I2C_data[2];
    int ret = VIBE_S_SUCCESS;
#ifdef VIBETONZ_TUNING
    VibeUInt32 nPWM_PLLDiv_KernelParam = 0;
    VibeUInt32 nPWM_Freq_KernelParam = 0;
    VibeUInt32 nPWM_Period_KernelParam = 0;
    VibeUInt32 nPWM_Duty_KernelParam = 0;
    VibeUInt32 nPWM_Update_KernelParam = 0;
#endif

    if (!g_bAmpEnabled)
    {
        DbgOut((KERN_DEBUG "ImmVibeSPI_ForceOut_AmpEnable.\n"));
        g_bAmpEnabled = true;

#ifdef VIBETONZ_TUNING /* For Resonant Frequency Test */
	    ImmVibeGetDeviceKernelParameter(0, 85, &nPWM_PLLDiv_KernelParam); /* use for PLL variable */
	    ImmVibeGetDeviceKernelParameter(0, 86, &nPWM_Freq_KernelParam); /* use for Freq variable */
	    ImmVibeGetDeviceKernelParameter(0, 87, &nPWM_Period_KernelParam); /* use for Period variable */
	    ImmVibeGetDeviceKernelParameter(0, 88, &nPWM_Duty_KernelParam); /* use for Duty variable */
	    ImmVibeGetDeviceKernelParameter(0, 89, &nPWM_Update_KernelParam); /* use for Update variable */

	    /* Update current Period and Duty values if those from the Kernel Parameters table are acceptables (value != 0) */
#ifdef ISA1200_GEN_MODE
	    if(nPWM_Update_KernelParam)
	    {
	        g_nPWM_PLLDiv = nPWM_PLLDiv_KernelParam;
	        g_nPWM_Freq = nPWM_Freq_KernelParam;
	        g_nPWM_Period = nPWM_Period_KernelParam;
	        g_nPWM_Duty = nPWM_Duty_KernelParam;

#ifdef MOTOR_TYPE_LRA
	        I2C_data[1] = 0xC0 ; // EXT clock + DAC inversion + LRA
#else
	        I2C_data[1] = 0xE0; // EXT clock + DAC inversion + ERM
#endif
	        I2C_data[0]= HCTRL1;
	        do
	        {
	            ret = SYS_API__I2C__Write(ISA1200_I2C_ADDRESS,  2 /* data length*/,  I2C_data);
	            cnt++;
	        }while(VIBE_S_SUCCESS != ret && cnt < RETRY_CNT);
	        if( VIBE_S_SUCCESS != ret) DEBUG_MSG("[ImmVibeSPI_ForceOut_AmpEnable] I2C_Write Error,  Slave Address = [%d], ret = [%d]\n", I2C_data[0], ret);

	        I2C_data[1] = 0x00 ; // Disable Software Reset
	        I2C_data[0]= HCTRL2;
	        do
	        {
	            ret = SYS_API__I2C__Write(ISA1200_I2C_ADDRESS,  2 /* data length*/,  I2C_data);
	            cnt++;
	        }while(VIBE_S_SUCCESS != ret && cnt < RETRY_CNT);
	        if( VIBE_S_SUCCESS != ret) DEBUG_MSG("[ImmVibeSPI_ForceOut_AmpEnable] I2C_Write Error,  Slave Address = [%d], ret = [%d]\n", I2C_data[0], ret);

	        I2C_data[1] = 0x03 + ( g_nPWM_PLLDiv<<4); // PLLLDIV
	        I2C_data[0]= HCTRL3;
	        do
	        {
	            ret = SYS_API__I2C__Write(ISA1200_I2C_ADDRESS,  2 /* data length*/,  I2C_data);
	            cnt++;
	        }while(VIBE_S_SUCCESS != ret && cnt < RETRY_CNT);
	        if( VIBE_S_SUCCESS != ret) DEBUG_MSG("[ImmVibeSPI_ForceOut_AmpEnable] I2C_Write Error,  Slave Address = [%d], ret = [%d]\n", I2C_data[0], ret);

	        I2C_data[1] = g_nPWM_Freq; // PWM control
	        I2C_data[0]= HCTRL4;
	        do
	        {
	            ret = SYS_API__I2C__Write(ISA1200_I2C_ADDRESS,  2 /* data length*/,  I2C_data);
	            cnt++;
	        }while(VIBE_S_SUCCESS != ret && cnt < RETRY_CNT);
	        if( VIBE_S_SUCCESS != ret) DEBUG_MSG("[ImmVibeSPI_ForceOut_AmpEnable] I2C_Write Error,  Slave Address = [%d], ret = [%d]\n", I2C_data[0], ret);

	        I2C_data[1] = g_nPWM_Duty; // PWM High Duty
	        I2C_data[0]= HCTRL5;
	        do
	        {
	            ret = SYS_API__I2C__Write(ISA1200_I2C_ADDRESS,  2 /* data length*/,  I2C_data);
	            cnt++;
	        }while(VIBE_S_SUCCESS != ret && cnt < RETRY_CNT);
	        if( VIBE_S_SUCCESS != ret) DEBUG_MSG("[ImmVibeSPI_ForceOut_AmpEnable] I2C_Write Error,  Slave Address = [%d], ret = [%d]\n", I2C_data[0], ret);

	        I2C_data[1] = g_nPWM_Period; // PWM Period
	        I2C_data[0]= HCTRL6;
	        do
	        {
	            ret = SYS_API__I2C__Write(ISA1200_I2C_ADDRESS,  2 /* data length*/,  I2C_data);
	            cnt++;
	        }while(VIBE_S_SUCCESS != ret && cnt < RETRY_CNT);
	        if( VIBE_S_SUCCESS != ret) DEBUG_MSG("[ImmVibeSPI_ForceOut_AmpEnable] I2C_Write Error,  Slave Address = [%d], ret = [%d]\n", I2C_data[0], ret);
	    }
#else
	    if(nPWM_Update_KernelParam)
	    {
	        g_nPWM_Freq = nPWM_Freq_KernelParam;
	    }
	    SYS_API_SET_PWM_FREQ(g_nPWM_Freq); // 22.4Khz
	    SYS_API_SET_PWM_DUTY(500); // 50% Duty Cycle, 1000 ~ 0
#endif
#endif /* VIBETONZ_TUNING */

	    PWM_CLK_ENABLE;

#ifdef ISA1200_HEN_ENABLE
	    SYS_API_HEN_HIGH;
#endif

#ifdef ISA1200_GEN_MODE
	    I2C_data[1] = 0x91; //Haptic Enable + PWM generation mode
#else
#ifdef ISA1200_PWM_256DIV_MODE
	    I2C_data[1] = 0x89; // Haptic Drive Disable + PWM Input mode + 44.8Khz mode
#else
	    I2C_data[1] = 0x88; // Haptic Drive Disable + PWM Input mode + 22.4Khz mode
#endif
#endif
	    I2C_data[0]= HCTRL0;
	    do
	    {
	        ret = SYS_API__I2C__Write(ISA1200_I2C_ADDRESS,  2 /* data length*/,  I2C_data);
	        cnt++;
	    }while(VIBE_S_SUCCESS != ret && cnt < RETRY_CNT);
	    if( VIBE_S_SUCCESS != ret) DEBUG_MSG("[ImmVibeSPI_ForceOut_AmpEnable] I2C_Write Error,  Slave Address = [%d], ret = [%d]\n", I2C_data[0], ret);

    }

    return VIBE_S_SUCCESS;
}

/*
** Called at initialization time to set PWM frequencies, disable amp, etc...
*/
IMMVIBESPIAPI VibeStatus ImmVibeSPI_ForceOut_Initialize(void)
{
    int cnt = 0;
    unsigned char I2C_data[2];
    int ret = VIBE_S_SUCCESS;

    DbgOut((KERN_DEBUG "ImmVibeSPI_ForceOut_Initialize.\n"));

    SYS_API_VDDP_ON;
    SYS_API_LEN_HIGH;

    SLEEP(200 /*us*/);

    I2C_data[1] = g_nLDO_Voltage; // LDO Voltage
    I2C_data[0]= SCTRL;
    do
    {
        ret = SYS_API__I2C__Write(ISA1200_I2C_ADDRESS,  2 /* data length*/,  I2C_data);
        cnt++;
    }while(VIBE_S_SUCCESS != ret && cnt < RETRY_CNT);
    if( VIBE_S_SUCCESS != ret) DEBUG_MSG("[ImmVibeSPI_ForceOut_Initialize] I2C_Write Error,  Slave Address = [%d], ret = [%d]\n", I2C_data[0], ret);

#ifdef ISA1200_GEN_MODE
    I2C_data[1] = 0x11; //Haptic Drive Disable + PWM generation mode + 44.8Khz mode
    I2C_data[0]= HCTRL0;
    do
    {
        ret = SYS_API__I2C__Write(ISA1200_I2C_ADDRESS,  2 /* data length*/,  I2C_data);
        cnt++;
    }while(VIBE_S_SUCCESS != ret && cnt < RETRY_CNT);
    if( VIBE_S_SUCCESS != ret) DEBUG_MSG("[ImmVibeSPI_ForceOut_Initialize] I2C_Write Error,  Slave Address = [%d], ret = [%d]\n", I2C_data[0], ret);

#ifdef MOTOR_TYPE_LRA
    I2C_data[1] = 0xC0 ; // EXT clock + DAC inversion + LRA
#else
    I2C_data[1] = 0xE0; // EXT clock + DAC inversion + ERM
#endif
    I2C_data[0]= HCTRL1;
    do
    {
        ret = SYS_API__I2C__Write(ISA1200_I2C_ADDRESS,  2 /* data length*/,  I2C_data);
        cnt++;
    }while(VIBE_S_SUCCESS != ret && cnt < RETRY_CNT);
    if( VIBE_S_SUCCESS != ret) DEBUG_MSG("[ImmVibeSPI_ForceOut_Initialize] I2C_Write Error,  Slave Address = [%d], ret = [%d]\n", I2C_data[0], ret);

    I2C_data[1] = 0x00 ; // Disable Software Reset
    I2C_data[0]= HCTRL2;
    do
    {
        ret = SYS_API__I2C__Write(ISA1200_I2C_ADDRESS,  2 /* data length*/,  I2C_data);
        cnt++;
    }while(VIBE_S_SUCCESS != ret && cnt < RETRY_CNT);
    if( VIBE_S_SUCCESS != ret) DEBUG_MSG("[ImmVibeSPI_ForceOut_Initialize] I2C_Write Error,  Slave Address = [%d], ret = [%d]\n", I2C_data[0], ret);

    I2C_data[1] = 0x03 + ( g_nPWM_PLLDiv<<4); // PLLLDIV
    I2C_data[0]= HCTRL3;
    do
    {
        ret = SYS_API__I2C__Write(ISA1200_I2C_ADDRESS,  2 /* data length*/,  I2C_data);
        cnt++;
    }while(VIBE_S_SUCCESS != ret && cnt < RETRY_CNT);
    if( VIBE_S_SUCCESS != ret) DEBUG_MSG("[ImmVibeSPI_ForceOut_Initialize] I2C_Write Error,  Slave Address = [%d], ret = [%d]\n", I2C_data[0], ret);

    I2C_data[1] = g_nPWM_Freq; // PWM control
    I2C_data[0]= HCTRL4;
    do
    {
        ret = SYS_API__I2C__Write(ISA1200_I2C_ADDRESS,  2 /* data length*/,  I2C_data);
        cnt++;
    }while(VIBE_S_SUCCESS != ret && cnt < RETRY_CNT);
    if( VIBE_S_SUCCESS != ret) DEBUG_MSG("[ImmVibeSPI_ForceOut_Initialize] I2C_Write Error,  Slave Address = [%d], ret = [%d]\n", I2C_data[0], ret);

    I2C_data[1] = g_nPWM_Duty; // PWM High Duty
    I2C_data[0]= HCTRL5;
    do
    {
        ret = SYS_API__I2C__Write(ISA1200_I2C_ADDRESS,  2 /* data length*/,  I2C_data);
        cnt++;
    }while(VIBE_S_SUCCESS != ret && cnt < RETRY_CNT);
    if( VIBE_S_SUCCESS != ret) DEBUG_MSG("[ImmVibeSPI_ForceOut_Initialize] I2C_Write Error,  Slave Address = [%d], ret = [%d]\n", I2C_data[0], ret);

    I2C_data[1] = g_nPWM_Period; // PWM Period
    I2C_data[0]= HCTRL6;
    do
    {
        ret = SYS_API__I2C__Write(ISA1200_I2C_ADDRESS,  2 /* data length*/,  I2C_data);
        cnt++;
    }while(VIBE_S_SUCCESS != ret && cnt < RETRY_CNT);
    if( VIBE_S_SUCCESS != ret) DEBUG_MSG("[ImmVibeSPI_ForceOut_Initialize] I2C_Write Error,  Slave Address = [%d], ret = [%d]\n", I2C_data[0], ret);

#else // PWM Input Mode

#ifdef ISA1200_PWM_256DIV_MODE
    I2C_data[1] = 0x09; // Haptic Drive Disable + PWM Input mode + 44.8Khz mode
#else
    I2C_data[1] = 0x08; // Haptic Drive Disable + PWM Input mode + 22.4Khz mode
#endif
    I2C_data[0]= HCTRL0;
    do
    {
        ret = SYS_API__I2C__Write(ISA1200_I2C_ADDRESS,  2 /* data length*/,  I2C_data);
        cnt++;
    }while(VIBE_S_SUCCESS != ret && cnt < RETRY_CNT);
    if( VIBE_S_SUCCESS != ret) DEBUG_MSG("[ImmVibeSPI_ForceOut_Initialize] I2C_Write Error,  Slave Address = [%d], ret = [%d]\n", I2C_data[0], ret);

#ifdef MOTOR_TYPE_LRA
    I2C_data[1] = 0x43; // EXT clock + DAC inversion + LRA
#else
    I2C_data[1] = 0x63; // EXT clock + DAC inversion + ERM
#endif
    I2C_data[0]= HCTRL1;
    do
    {
        ret = SYS_API__I2C__Write(ISA1200_I2C_ADDRESS,  2 /* data length*/,  I2C_data);
        cnt++;
    }while(VIBE_S_SUCCESS != ret && cnt < RETRY_CNT);
    if( VIBE_S_SUCCESS != ret) DEBUG_MSG("[ImmVibeSPI_ForceOut_Initialize] I2C_Write Error,  Slave Address = [%d], ret = [%d]\n", I2C_data[0], ret);

    I2C_data[1] = 0x00; // Disable Software Reset
    I2C_data[0]= HCTRL2;
    do
    {
        ret = SYS_API__I2C__Write(ISA1200_I2C_ADDRESS,  2 /* data length*/,  I2C_data);
        cnt++;
    }while(VIBE_S_SUCCESS != ret && cnt < RETRY_CNT);
    if( VIBE_S_SUCCESS != ret) DEBUG_MSG("[ImmVibeSPI_ForceOut_Initialize] I2C_Write Error,  Slave Address = [%d], ret = [%d]\n", I2C_data[0], ret);

    I2C_data[1] = 0x00; // Default
    I2C_data[0]= HCTRL4;
    do
    {
        ret = SYS_API__I2C__Write(ISA1200_I2C_ADDRESS,  2 /* data length*/,  I2C_data);
        cnt++;
    }while(VIBE_S_SUCCESS != ret && cnt < RETRY_CNT);
    if( VIBE_S_SUCCESS != ret) DEBUG_MSG("[ImmVibeSPI_ForceOut_Initialize] I2C_Write Error,  Slave Address = [%d], ret = [%d]\n", I2C_data[0], ret);
#endif
    return VIBE_S_SUCCESS;
}

/*
** Called at termination time to set PWM freq, disable amp, etc...
*/
IMMVIBESPIAPI VibeStatus ImmVibeSPI_ForceOut_Terminate(void)
{
    DbgOut((KERN_DEBUG "ImmVibeSPI_ForceOut_Terminate.\n"));
    SYS_API_LEN_LOW;
#ifdef ISA1200_HEN_ENABLE
    SYS_API_HEN_LOW;
#endif
    SYS_API_VDDP_OFF;
    return VIBE_S_SUCCESS;
}

/*
** Called by the real-time loop to set PWM_MAG duty cycle
*/
IMMVIBESPIAPI VibeStatus ImmVibeSPI_ForceOut_SetSamples(VibeUInt8 nActuatorIndex, VibeUInt16 nOutputSignalBitDepth, VibeUInt16 nBufferSizeInBytes, VibeInt8* pForceOutputBuffer)
{
    VibeInt8 nForce;
    int cnt = 0;
    unsigned char I2C_data[2];
    int ret = VIBE_S_SUCCESS;

    switch (nOutputSignalBitDepth)
    {
        case 8:
            /* pForceOutputBuffer is expected to contain 1 byte */
            if (nBufferSizeInBytes != 1)
			{
				DbgOut((KERN_ERR "[ImmVibeSPI] ImmVibeSPI_ForceOut_SetSamples nBufferSizeInBytes =  %d \n", nBufferSizeInBytes ));
				return VIBE_E_FAIL;
            }
            nForce = pForceOutputBuffer[0];
            break;
        case 16:
            /* pForceOutputBuffer is expected to contain 2 byte */
            if (nBufferSizeInBytes != 2) return VIBE_E_FAIL;

            /* Map 16-bit value to 8-bit */
            nForce = ((VibeInt16*)pForceOutputBuffer)[0] >> 8;
            break;
        default:
            /* Unexpected bit depth */
            return VIBE_E_FAIL;
    }

    if(nForce == 0)
    {
#ifdef ISA1200_GEN_MODE
        I2C_data[1] = g_nPWM_Duty; // PWM High Duty 50%
        I2C_data[0]= HCTRL5;
        do
        {
            ret = SYS_API__I2C__Write(ISA1200_I2C_ADDRESS,  2 /* data length*/,  I2C_data);
            cnt++;
        }while(VIBE_S_SUCCESS != ret && cnt < RETRY_CNT);
        if( VIBE_S_SUCCESS != ret) DEBUG_MSG("[ImmVibeSPI_ForceOut_Set] I2C_Write Error,  Slave Address = [%d], ret = [%d]\n", I2C_data[0], ret);
#else
        SYS_API_SET_PWM_DUTY(500); //50%
#endif
        if (g_bAmpEnabled == true)
        {
            ImmVibeSPI_ForceOut_AmpDisable(0);
        }
    }
    else
    {
        if (g_bAmpEnabled != true)
        {
            ImmVibeSPI_ForceOut_AmpEnable(0);
        }
#ifdef ISA1200_GEN_MODE
        I2C_data[1] = g_nPWM_Duty + ((g_nPWM_Duty-1)*nForce)/127;
        I2C_data[0]= HCTRL5;
        do
        {
            ret = SYS_API__I2C__Write(ISA1200_I2C_ADDRESS,  2 /* data length*/,  I2C_data);
            cnt++;
        }while(VIBE_S_SUCCESS != ret && cnt < RETRY_CNT);
        if( VIBE_S_SUCCESS != ret) DEBUG_MSG("[ImmVibeSPI_ForceOut_Set] I2C_Write Error,  Slave Address = [%d], ret = [%d]\n", I2C_data[0], ret);
#else
        SYS_API_SET_PWM_DUTY(500 - (490 * nForce)/127);
#endif
    }
    return VIBE_S_SUCCESS;
}
/*
** Called to set force output frequency parameters
*/
IMMVIBESPIAPI VibeStatus ImmVibeSPI_ForceOut_SetFrequency(VibeUInt8 nActuatorIndex, VibeUInt16 nFrequencyParameterID, VibeUInt32 nFrequencyParameterValue)
{
    return VIBE_S_SUCCESS;
}

/*
** Called to get the device name (device name must be returned as ANSI char)
*/
IMMVIBESPIAPI VibeStatus ImmVibeSPI_Device_GetName(VibeUInt8 nActuatorIndex, char *szDevName, int nSize)
{
    return VIBE_S_SUCCESS;
}
