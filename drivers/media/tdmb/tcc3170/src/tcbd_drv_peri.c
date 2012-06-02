/*
 * tcbd_drv_peri.c
 *
 * Author:  <linux@telechips.com>
 * Description: Telechips broadcast driver
 *
 * Copyright (c) Telechips, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "tcpal_os.h"
#include "tcpal_debug.h"

#include "tcbd_feature.h"
#include "tcbd_api_common.h"
#include "tcbd_drv_io.h"

#define TCBD_PERI_SPI_READY_TIME 0x00
#define TCBD_PERI_SPI_WAIT_TIME 0x00

static inline void tcbd_peri_spi_slave(
	u8 *_peri_mode, u8 _interface_speed)
{
	_peri_mode[0] = TCBD_PERI_SPI_MOTOROLA_SSP |
				   TCBD_PERI_SPI_SLAVE |
				   TCBD_PERI_SPI_SIZE32BIT |
				   _interface_speed;
	_peri_mode[1] = TCBD_PERI_SPI_CLKINIT_LOW |
				   TCBD_PERI_SPI_CLKPOL_POS |
				   TCBD_PERI_SPI_BITMSB1 |
				   (TCBD_PERI_SPI_READY_TIME << 2) |
				   TCBD_PERI_SPI_WAIT_TIME;
}

static inline void tcbd_peri_spi_master(
	u8 *_peri_mode, u8 _interface_speed)
{
	_peri_mode[0] = TCBD_PERI_SPI_MOTOROLA_SSP |
				   TCBD_PERI_SPI_MASTER |
				   TCBD_PERI_SPI_SIZE32BIT |
				   _interface_speed;
	_peri_mode[1] = TCBD_PERI_SPI_CLKINIT_LOW |
				   TCBD_PERI_SPI_CLKPOL_POS |
				   TCBD_PERI_SPI_BITMSB1 |
				   (TCBD_PERI_SPI_READY_TIME << 2) |
				   TCBD_PERI_SPI_WAIT_TIME;
}

#define TCBD_PERI_TS_STREAM_WAIT_TIME 0x02
#define TCBD_PERI_TS_MSM_SYNC_HIGH_TIME 0x00
#define TCBD_PERI_TS_MSM_SYNC_LOW_TIME 0x00
static inline void tcbd_peri_sts(
	u8 *_peri_mode, u8 _interface_speed)
{
	_peri_mode[0] = TCBD_PERI_TS_NORMAL_SLAVE |
				   TCBD_PERI_TS_STS |
				   TCBD_PERI_TS_FASTON |
				   _interface_speed;

	_peri_mode[1] = TCBD_PERI_TS_CLKPHASE_NEG |
				   TCBD_PERI_TS_SYNC_ACTIVEHIGH |
				   TCBD_PERI_TS_ENPOL_ACTIVEHIGH |
				   TCBD_PERI_TS_STREAM_WAIT_ON |
				   TCBD_PERI_TS_STREAM_WAIT_TIME;
	_peri_mode[2] = TCBD_PERI_TS_BITMSB |
				   (TCBD_PERI_TS_MSM_SYNC_HIGH_TIME << 3) |
				   TCBD_PERI_TS_MSM_SYNC_LOW_TIME;
	_peri_mode[3] = TCBD_PERI_TS_ERR_SIG_OFF;
}

static inline void tcbd_peri_pts(
	u8 *_peri_mode, u8 _interface_speed)
{
	_peri_mode[0] = TCBD_PERI_TS_NORMAL_SLAVE |
				   TCBD_PERI_TS_PTS |
				   TCBD_PERI_TS_FASTON |
				   TCBD_PERI_TS_TSCLKMASKON |
				   _interface_speed;
	_peri_mode[1] = TCBD_PERI_TS_CLKPHASE_POS |
				   TCBD_PERI_TS_SYNC_ACTIVEHIGH |
				   TCBD_PERI_TS_ENPOL_ACTIVEHIGH |
				   TCBD_PERI_TS_STREAM_WAIT_ON |
				   TCBD_PERI_TS_STREAM_WAIT_TIME;
	_peri_mode[2] = (TCBD_PERI_TS_MSM_SYNC_HIGH_TIME << 3) |
				   TCBD_PERI_TS_MSM_SYNC_LOW_TIME;
	_peri_mode[3] = TCBD_PERI_TS_ERR_SIG_OFF;
}

static inline void tcbd_peri_hpi(u8 *_peri_mode)
{
	_peri_mode[0] = TCBD_PERI_HPI_INTEL;
}

static inline u8 tcbd_calc_clock(
	struct tcbd_device *_device, s32 _min_khz, s32 _max_khz)
{
	u32 min_dlr = 0, max_dlr = 0, minv = -1, maxv = -1;

	min_dlr = (_device->main_clock / (2 * _min_khz)) - 1;
	max_dlr = (_device->main_clock / (2 * _max_khz)) - 1;

	if (min_dlr >= 0 && min_dlr < 8)
		minv = min_dlr;
	else if (min_dlr >= 8)
		minv = 7;

	if (max_dlr >= 0 && max_dlr < 8)
		maxv = max_dlr;
	else if (max_dlr >= 8)
		maxv = 7;

	if (minv == -1 && maxv == -1) {
		tcbd_debug(DEBUG_DRV_PERI,
			" # Can't find DLR value, DLR will set to zero\n");
		return 0;
	}

	if (maxv == -1)
		maxv = minv;
	else if (minv == -1)
		minv = maxv;

	return minv;
}

s32 tcbd_init_peri_gpio(
	struct tcbd_device *_device, enum tcbd_peri_type _peri_type)
{
	u8 cfg_value[10] = {0, };
	/*u8 drv_strength[] = {0xFF, 0xFF};*/

	tcbd_debug(DEBUG_DRV_PERI, "peri type:%d\n", (s32)(_peri_type));
	memset(cfg_value, 0, sizeof(cfg_value));

	switch (_peri_type) {
	case PERI_TYPE_SPI_SLAVE:
	case PERI_TYPE_PTS:
	case PERI_TYPE_HPI:
	case PERI_TYPE_SPI_ONLY:
		return 0;

	case PERI_TYPE_SPI_MASTER:
		cfg_value[0] = 0x0E;
		cfg_value[1] = 0x34;
		break;
	case PERI_TYPE_STS:
		cfg_value[0] = 0x0E;
		cfg_value[1] = 0x3C;
		break;
	default:
		break;
	}

	tcbd_reg_write_burst_cont(_device, TCBD_GPIO_ALT, cfg_value, 10);
	/*tcbd_reg_write_burst_cont(_device, TCBD_GPIO_DRV, drv_strength, 2);*/
	return 0;
}

s32 tcbd_select_peri(
	struct tcbd_device *_device, enum tcbd_peri_type _peri_type)
{
	s32 ret = 0;
	u8 periMode[4] = {0, };
	u8 interface_speed = tcbd_calc_clock(_device, 3000, 10000);

	tcbd_debug(DEBUG_DRV_PERI, "peri type:%d, clock div:%d\n",
		(s32)(_peri_type), interface_speed);
	switch (_peri_type) {
	case PERI_TYPE_SPI_SLAVE:
		tcbd_peri_spi_slave(periMode, interface_speed);
		ret |= tcbd_reg_write_burst_cont(
				_device, TCBD_PERI_MODE0, periMode, 2);
		ret |= tcbd_reg_write(_device, TCBD_PERI_CTRL, 0x90);
		ret |= tcbd_init_peri_gpio(_device, PERI_TYPE_SPI_SLAVE);
		break;
	case PERI_TYPE_SPI_MASTER:
		tcbd_peri_spi_master(periMode, interface_speed);
		ret |= tcbd_reg_write_burst_cont(
				_device, TCBD_PERI_MODE0, periMode, 2);
		ret |= tcbd_reg_write(_device, TCBD_PERI_CTRL, 0x90);
		ret |= tcbd_init_peri_gpio(_device, PERI_TYPE_SPI_MASTER);
		break;
	case PERI_TYPE_PTS:
		tcbd_peri_pts(periMode, interface_speed);
		ret |= tcbd_reg_write_burst_cont(
				_device, TCBD_PERI_MODE0, periMode, 4);
		ret |= tcbd_init_peri_gpio(_device, PERI_TYPE_PTS);
		break;
	case PERI_TYPE_STS:
		tcbd_peri_sts(periMode, interface_speed);
		ret |= tcbd_reg_write_burst_cont(
				_device, TCBD_PERI_MODE0, periMode, 4);
		ret |= tcbd_init_peri_gpio(_device, PERI_TYPE_STS);
		break;
	case PERI_TYPE_HPI:
		tcbd_peri_hpi(periMode);
		ret |= tcbd_reg_write(_device, TCBD_PERI_MODE0, periMode[0]);
		break;
	case PERI_TYPE_SPI_ONLY:
		ret |= tcbd_reg_write(_device, TCBD_PERI_CTRL, 0x80);
		ret |= tcbd_init_peri_gpio(_device, PERI_TYPE_SPI_ONLY);
	default:
		break;
	}
	_device->peri_type = _peri_type;
	return ret;
}

s32 tcbd_init_gpio_for_slave(struct tcbd_device *_device)
{
	u8 cfg_value[10] = {0, };

	cfg_value[0] = 0x01;
	cfg_value[1] = 0xC0;
	return tcbd_reg_write_burst_cont(
			_device, TCBD_GPIO_ALT, cfg_value, 10);
}

s32 tcbd_enable_peri(struct tcbd_device *_device)
{
	u8 data = TCBD_PERI_EN | TCBD_PERI_INIT_AUTOCLR;

	switch (_device->peri_type) {
	case PERI_TYPE_SPI_ONLY:
		data = 0x80;
		break;
	case PERI_TYPE_SPI_SLAVE:
	case PERI_TYPE_SPI_MASTER:
		data |= TCBD_PERI_SEL_SPI | TCBD_PERI_HEADER_ON;
		break;
	case PERI_TYPE_PTS:
	case PERI_TYPE_STS:
		data |= TCBD_PERI_SEL_TS | TCBD_PERI_HEADER_ON;
		break;
	case PERI_TYPE_HPI:
		data |= TCBD_PERI_SEL_HPI | TCBD_PERI_HEADER_ON;
		break;
	default:
		break;
	}

	return tcbd_reg_write(_device, TCBD_PERI_CTRL, data);
}

s32 tcbd_disable_peri(struct tcbd_device *_device)
{
	s32 ret = 0;
	u8 data;

	ret |= tcbd_reg_read(_device, TCBD_PERI_CTRL, &data);
	data &= ~TCBD_PERI_EN;
	ret |= tcbd_reg_write(_device, TCBD_PERI_CTRL, data);

	return ret;
}
