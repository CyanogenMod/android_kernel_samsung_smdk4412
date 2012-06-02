/*****************************************************************************
	Copyright(c) 2009 FCI Inc. All Rights Reserved

	File name : fc8050_isr.c

	Description : fc8050 interrupt service routine

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA


	History :
	----------------------------------------------------------------------
	2009/08/29	jason		initial
*******************************************************************************/

#include "fci_types.h"
#include "fci_hal.h"
#include "fc8050_regs.h"
#include "fc8050_isr.h"

static u8 fic_buffer[512+4];
static u8 msc_buffer[8192+4];

int (*fic_callback)(u32 userdata, u8 *data, int length) = NULL;
int (*msc_callback)(u32 userdata, u8 subchid, u8 *data, int length) = NULL;

u32 fic_user_data;
u32 msc_user_data;


void fc8050_isr(HANDLE hDevice)
{
	u8	ext_int_status = 0;

	bbm_read(hDevice, BBM_COM_INT_STATUS, &ext_int_status);
	bbm_write(hDevice, BBM_COM_INT_STATUS, ext_int_status);
	bbm_write(hDevice, BBM_COM_INT_STATUS, 0x00);

	if (ext_int_status & BBM_MF_INT) {
		u16	buf_int_status = 0;
		u16	size;
		int	i;

		bbm_word_read(hDevice, BBM_BUF_STATUS, &buf_int_status);
		bbm_word_write(hDevice, BBM_BUF_STATUS, buf_int_status);
		bbm_word_write(hDevice, BBM_BUF_STATUS, 0x0000);

		if (buf_int_status & 0x0100) {
			bbm_word_read(hDevice, BBM_BUF_FIC_THR, &size);
			size += 1;
			if (size-1) {
				bbm_data(hDevice, BBM_COM_FIC_DATA
					, &fic_buffer[4], size);

#ifdef CONFIG_TDMB_SPI
				if (fic_callback)
					(*fic_callback)(fic_user_data
					, &fic_buffer[6], size);
#else
				if (fic_callback)
					(*fic_callback)(fic_user_data
					, &fic_buffer[4], size);
#endif
			}
		}

		for (i = 0; i < 8; i++) {
			if (buf_int_status & (1 << i)) {
				bbm_word_read(hDevice
					, BBM_BUF_CH0_THR+i*2, &size);
				size += 1;

				if (size-1) {
					u8  sub_ch_id;

					bbm_read(hDevice, BBM_BUF_CH0_SUBCH+i
						, &sub_ch_id);
					sub_ch_id = sub_ch_id & 0x3f;

					bbm_data(hDevice, (BBM_COM_CH0_DATA+i)
						, &msc_buffer[4], size);

#ifdef CONFIG_TDMB_SPI
					if (msc_callback)
						(*msc_callback)(
						msc_user_data
						, sub_ch_id
						, &msc_buffer[6]
						, size);
#else
					if (msc_callback)
						(*msc_callback)(
						msc_user_data
						, sub_ch_id
						, &msc_buffer[4]
						, size);
#endif
				}
			}
		}

	}

}
