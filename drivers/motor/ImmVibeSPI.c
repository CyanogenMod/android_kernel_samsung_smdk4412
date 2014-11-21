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
#include <linux/pwm.h>
#include <plat/gpio-cfg.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/mfd/max8997.h>
#include <linux/mfd/max8997-private.h>

#include "tspdrv.h"

#ifdef IMMVIBESPIAPI
#undef IMMVIBESPIAPI
#endif
#define IMMVIBESPIAPI static

/*
** This SPI supports only one actuator.
*/
#define NUM_ACTUATORS	1

#define PWM_DUTY_MAX    579 /* 13MHz / (579 + 1) = 22.4kHz */

static bool g_bAmpEnabled;

extern void vibtonz_en(bool en);
extern void vibtonz_pwm(int nForce);

/*
** Called to disable amp (disable output force)
*/
IMMVIBESPIAPI VibeStatus ImmVibeSPI_ForceOut_AmpDisable(VibeUInt8 nActuatorIndex)
{
    /*printk(KERN_ERR"[VIBRATOR] ImmVibeSPI_ForceOut_AmpDisable is called\n");*/
#if 0
	if (g_bAmpEnabled) {
		DbgOut((KERN_DEBUG "ImmVibeSPI_ForceOut_AmpDisable.\n"));

		g_bAmpEnabled = false;

		/* Disable amp */


		/* Disable PWM CLK */

	}
#endif

	if (g_bAmpEnabled) {

		g_bAmpEnabled = false;
#if !defined(CONFIG_MACH_P4NOTE) && !defined(CONFIG_MACH_TAB3) && !defined(CONFIG_MACH_SP7160LTE)
		vibtonz_pwm(0);
		vibtonz_en(false);
#endif
		if (regulator_hapticmotor_enabled == 1) {
			regulator_hapticmotor_enabled = 0;

			printk(KERN_DEBUG "Motor:tspdrv: %s (%d)\n", __func__,
						regulator_hapticmotor_enabled);

		}
	}

	return VIBE_S_SUCCESS;
}

/*
** Called to enable amp (enable output force)
*/
IMMVIBESPIAPI VibeStatus ImmVibeSPI_ForceOut_AmpEnable(VibeUInt8 nActuatorIndex)
{
    /*printk(KERN_ERR"[VIBRATOR]ImmVibeSPI_ForceOut_AmpEnable is called\n");*/
#if 0
	if (!g_bAmpEnabled)	{
		DbgOut((KERN_DEBUG "ImmVibeSPI_ForceOut_AmpEnable.\n"));

		g_bAmpEnabled = true;

		/* Generate PWM CLK with proper frequency(ex. 22400Hz) and 50% duty cycle.*/


		/* Enable amp */

	}
#endif

	if (!g_bAmpEnabled) {
#if !defined(CONFIG_MACH_P4NOTE) && !defined(CONFIG_MACH_TAB3) && !defined(CONFIG_MACH_SP7160LTE)
		vibtonz_en(true);
#endif

		g_bAmpEnabled = true;
		regulator_hapticmotor_enabled = 1;

		printk(KERN_DEBUG "Motor:tspdrv: %s (%d)\n", __func__,
					regulator_hapticmotor_enabled);

	}

	return VIBE_S_SUCCESS;
}

/*
** Called at initialization time to set PWM freq, disable amp, etc...
*/
IMMVIBESPIAPI VibeStatus ImmVibeSPI_ForceOut_Initialize(void)
{
    /*printk(KERN_ERR"[VIBRATOR]ImmVibeSPI_ForceOut_Initialize is called\n");*/
	DbgOut((KERN_DEBUG "ImmVibeSPI_ForceOut_Initialize.\n"));

	g_bAmpEnabled = true;   /* to force ImmVibeSPI_ForceOut_AmpDisable disabling the amp */

	/*
	** Disable amp.
	** If multiple actuators are supported, please make sure to call
	** ImmVibeSPI_ForceOut_AmpDisable for each actuator (provide the actuator index as
	** input argument).
	*/

	ImmVibeSPI_ForceOut_AmpDisable(0);

	regulator_hapticmotor_enabled = 0;

	return VIBE_S_SUCCESS;
}

/*
** Called at termination time to set PWM freq, disable amp, etc...
*/
IMMVIBESPIAPI VibeStatus ImmVibeSPI_ForceOut_Terminate(void)
{
	DbgOut((KERN_DEBUG "ImmVibeSPI_ForceOut_Terminate.\n"));

	/*
	** Disable amp.
	** If multiple actuators are supported, please make sure to call
	** ImmVibeSPI_ForceOut_AmpDisable for each actuator (provide the actuator index as
	** input argument).
	*/
	ImmVibeSPI_ForceOut_AmpDisable(0);

	return VIBE_S_SUCCESS;
}

/*
** Called by the real-time loop to set PWM duty cycle
*/
IMMVIBESPIAPI VibeStatus ImmVibeSPI_ForceOut_SetSamples(VibeUInt8 nActuatorIndex,
							VibeUInt16 nOutputSignalBitDepth,
							VibeUInt16 nBufferSizeInBytes,
							VibeInt8 * pForceOutputBuffer)
{
#if 0
	VibeInt8 nForce;

	switch (nOutputSignalBitDepth) {
	case 8:
		/* pForceOutputBuffer is expected to contain 1 byte */
		if (nBufferSizeInBytes != 1) {
			DbgOut((KERN_ERR "[ImmVibeSPI] ImmVibeSPI_ForceOut_SetSamples nBufferSizeInBytes =  %d\n", nBufferSizeInBytes));
			return VIBE_E_FAIL;
		}
		nForce = pForceOutputBuffer[0];
		break;
	case 16:
		/* pForceOutputBuffer is expected to contain 2 byte */
		if (nBufferSizeInBytes != 2)
			return VIBE_E_FAIL;

		/* Map 16-bit value to 8-bit */
		nForce = ((VibeInt16 *)pForceOutputBuffer)[0] >> 8;
		break;
	default:
		/* Unexpected bit depth */
		return VIBE_E_FAIL;
	}

	if (nForce == 0)
		/* Set 50% duty cycle or disable amp */
	else
		/* Map force from [-127, 127] to [0, PWM_DUTY_MAX] */


#endif

	VibeInt8 nForce;

	switch (nOutputSignalBitDepth) {
	case 8:
		/* pForceOutputBuffer is expected to contain 1 byte */
		if (nBufferSizeInBytes != 1) {
			DbgOut((KERN_ERR "[ImmVibeSPI] ImmVibeSPI_ForceOut_SetSamples nBufferSizeInBytes =  %d\n", nBufferSizeInBytes));
			return VIBE_E_FAIL;
		}
		nForce = pForceOutputBuffer[0];
		break;
	case 16:
		/* pForceOutputBuffer is expected to contain 2 byte */
		if (nBufferSizeInBytes != 2)
			return VIBE_E_FAIL;

		/* Map 16-bit value to 8-bit */
		nForce = ((VibeInt16 *)pForceOutputBuffer)[0] >> 8;
		break;
	default:
		/* Unexpected bit depth */
		return VIBE_E_FAIL;
	}

	if (nForce == 0) {
		/* Set 50% duty cycle or disable amp */
		ImmVibeSPI_ForceOut_AmpDisable(0);
	} else {
		/* Map force from [-127, 127] to [0, PWM_DUTY_MAX] */
		ImmVibeSPI_ForceOut_AmpEnable(0);
#if !defined(CONFIG_MACH_P4NOTE) && !defined(CONFIG_MACH_TAB3) && !defined(CONFIG_MACH_SP7160LTE)
		vibtonz_pwm(nForce);
#endif
	}

	return VIBE_S_SUCCESS;
}

#if 0	/* Unused */
/*
** Called to set force output frequency parameters
*/
IMMVIBESPIAPI VibeStatus ImmVibeSPI_ForceOut_SetFrequency(VibeUInt8 nActuatorIndex, VibeUInt16 nFrequencyParameterID, VibeUInt32 nFrequencyParameterValue)
{
	return VIBE_S_SUCCESS;
}
#endif

/*
** Called to get the device name (device name must be returned as ANSI char)
*/
IMMVIBESPIAPI VibeStatus ImmVibeSPI_Device_GetName(VibeUInt8 nActuatorIndex, char *szDevName, int nSize)
{
	return VIBE_S_SUCCESS;
}

