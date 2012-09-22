/*
 *  wacom_i2c_func.c - Wacom G5 Digitizer Controller (I2C bus)
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/wacom_i2c.h>
#include "wacom_i2c_flash.h"

#ifdef WACOM_IMPORT_FW_ALGO
#include "wacom_i2c_coord_table.h"

/* For firmware algorithm */
#define CAL_PITCH 100
#define LATTICE_SIZE_X ((WACOM_MAX_COORD_X / CAL_PITCH)+2)
#define LATTICE_SIZE_Y ((WACOM_MAX_COORD_Y / CAL_PITCH)+2)
#endif

#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
#define CONFIG_SAMSUNG_KERNEL_DEBUG_USER
#endif

/* block wacom coordinate print */
#ifdef CONFIG_SEC_TOUCHSCREEN_DVFS_LOCK
#if defined(CONFIG_MACH_P4NOTE)
void free_dvfs_lock(struct work_struct *work)
{
	struct wacom_i2c *wac_i2c =
	    container_of(work, struct wacom_i2c, dvfs_work.work);

	if (wac_i2c->dvfs_lock_status)
		dev_lock(wac_i2c->bus_dev,
			wac_i2c->dev, SEC_BUS_LOCK_FREQ);
	else {
		dev_unlock(wac_i2c->bus_dev, wac_i2c->dev);
		exynos_cpufreq_lock_free(DVFS_LOCK_ID_PEN);
	}
}

static void set_dvfs_lock(struct wacom_i2c *wac_i2c, bool on)
{
	if (on) {
		if (!wac_i2c->dvfs_lock_status) {
			cancel_delayed_work(&wac_i2c->dvfs_work);
			dev_lock(wac_i2c->bus_dev,
				wac_i2c->dev, SEC_BUS_LOCK_FREQ2);
			exynos_cpufreq_lock(DVFS_LOCK_ID_PEN,
					    wac_i2c->cpufreq_level);
			wac_i2c->dvfs_lock_status = true;
			schedule_delayed_work(&wac_i2c->dvfs_work,
				msecs_to_jiffies(SEC_DVFS_LOCK_TIMEOUT_MS));
		}
	} else {
		if (wac_i2c->dvfs_lock_status) {
			schedule_delayed_work(&wac_i2c->dvfs_work,
				msecs_to_jiffies(SEC_DVFS_LOCK_TIMEOUT_MS));
			wac_i2c->dvfs_lock_status = false;
		}
	}
}
#else	/* CONFIG_MACH_P4NOTE */
void free_dvfs_lock(struct work_struct *work)
{
	struct wacom_i2c *wac_i2c =
	    container_of(work, struct wacom_i2c, dvfs_work.work);

	exynos_cpufreq_lock_free(DVFS_LOCK_ID_PEN);
	wac_i2c->dvfs_lock_status = false;
}

static void set_dvfs_lock(struct wacom_i2c *wac_i2c, bool on)
{
	if (on) {
		cancel_delayed_work(&wac_i2c->dvfs_work);
		if (!wac_i2c->dvfs_lock_status) {
			exynos_cpufreq_lock(DVFS_LOCK_ID_PEN,
					    wac_i2c->cpufreq_level);
			wac_i2c->dvfs_lock_status = true;
		}
	} else {
		if (wac_i2c->dvfs_lock_status)
			schedule_delayed_work(&wac_i2c->dvfs_work,
			      SEC_DVFS_LOCK_TIMEOUT * HZ);
	}
}
#endif
#endif	/* CONFIG_SEC_TOUCHSCREEN_DVFS_LOCK */

void forced_release(struct wacom_i2c *wac_i2c)
{
#if defined(CONFIG_SAMSUNG_KERNEL_DEBUG_USER)
	printk(KERN_DEBUG "[E-PEN] %s\n", __func__);
#endif
	input_report_abs(wac_i2c->input_dev, ABS_X, wac_i2c->last_x);
	input_report_abs(wac_i2c->input_dev, ABS_Y, wac_i2c->last_y);
	input_report_abs(wac_i2c->input_dev, ABS_PRESSURE, 0);
	input_report_key(wac_i2c->input_dev, BTN_STYLUS, 0);
	input_report_key(wac_i2c->input_dev, BTN_TOUCH, 0);
#if defined(WACOM_IRQ_WORK_AROUND) || defined(WACOM_PDCT_WORK_AROUND)
	input_report_key(wac_i2c->input_dev, BTN_TOOL_RUBBER, 0);
	input_report_key(wac_i2c->input_dev, BTN_TOOL_PEN, 0);
	input_report_key(wac_i2c->input_dev, KEY_PEN_PDCT, 0);
#else
	input_report_key(wac_i2c->input_dev, wac_i2c->tool, 0);
#endif
	input_sync(wac_i2c->input_dev);

	wac_i2c->last_x = 0;
	wac_i2c->last_y = 0;
	wac_i2c->pen_prox = 0;
	wac_i2c->pen_pressed = 0;
	wac_i2c->side_pressed = 0;
	wac_i2c->pen_pdct = PDCT_NOSIGNAL;

#ifdef CONFIG_SEC_TOUCHSCREEN_DVFS_LOCK
	set_dvfs_lock(wac_i2c, false);
#endif

}

#ifdef WACOM_PDCT_WORK_AROUND
void forced_hover(struct wacom_i2c *wac_i2c)
{
	/* To distinguish hover and pdct area, release */
	if (wac_i2c->last_x != 0 || wac_i2c->last_y != 0) {
		printk(KERN_DEBUG "[E-PEN] release hover\n");
		forced_release(wac_i2c);
	}
	wac_i2c->rdy_pdct = true;
#if defined(CONFIG_SAMSUNG_KERNEL_DEBUG_USER)
	printk(KERN_DEBUG "[E-PEN] %s\n", __func__);
#endif
	input_report_key(wac_i2c->input_dev, KEY_PEN_PDCT, 1);
	input_sync(wac_i2c->input_dev);

#ifdef CONFIG_SEC_TOUCHSCREEN_DVFS_LOCK
	set_dvfs_lock(wac_i2c, true);
#endif
}
#endif

#ifdef WACOM_IRQ_WORK_AROUND
void wacom_i2c_pendct_work(struct work_struct *work)
{
	struct wacom_i2c *wac_i2c =
	    container_of(work, struct wacom_i2c, pendct_dwork.work);

	printk(KERN_DEBUG "[E-PEN] %s , %d\n",
	       __func__, gpio_get_value(wac_i2c->wac_pdata->gpio_pendct));

	if (gpio_get_value(wac_i2c->wac_pdata->gpio_pendct))
		forced_release(wac_i2c);
}
#endif

int wacom_i2c_send(struct wacom_i2c *wac_i2c,
			  const char *buf, int count, bool mode)
{
	struct i2c_client *client = mode ?
		wac_i2c->client_boot : wac_i2c->client;

	if (wac_i2c->boot_mode && !mode) {
		printk(KERN_DEBUG
			"[E-PEN] failed to send\n");
		return 0;
	}

	return i2c_master_send(client, buf, count);
}

int wacom_i2c_recv(struct wacom_i2c *wac_i2c,
			char *buf, int count, bool mode)
{
	struct i2c_client *client = mode ?
		wac_i2c->client_boot : wac_i2c->client;

	if (wac_i2c->boot_mode && !mode) {
		printk(KERN_DEBUG
			"[E-PEN] failed to send\n");
		return 0;
	}

	return i2c_master_recv(client, buf, count);
}

int wacom_i2c_test(struct wacom_i2c *wac_i2c)
{
	int ret, i;
	char buf, test[10];
	buf = COM_QUERY;

	ret = wacom_i2c_send(wac_i2c, &buf, sizeof(buf), false);
	if (ret > 0)
		printk(KERN_INFO "[E-PEN] buf:%d, sent:%d\n", buf, ret);
	else {
		printk(KERN_ERR "[E-PEN] Digitizer is not active\n");
		return -1;
	}

	ret = wacom_i2c_recv(wac_i2c, test, sizeof(test), false);
	if (ret >= 0) {
		for (i = 0; i < 8; i++)
			printk(KERN_INFO "[E-PEN] %d\n", test[i]);
	} else {
		printk(KERN_ERR "[E-PEN] Digitizer does not reply\n");
		return -1;
	}

	return 0;
}

#ifdef WACOM_CONNECTION_CHECK
static void wacom_open_test(struct wacom_i2c *wac_i2c)
{
	u8 cmd = 0;
	u8 buf[2] = {0,};
	int ret = 0, cnt = 30;

	cmd = WACOM_I2C_STOP;
	ret = wacom_i2c_send(wac_i2c, &cmd, 1, false);
	if (ret <= 0) {
		printk(KERN_ERR "[E-PEN] failed to send stop command\n");
		return ;
	}

	cmd = WACOM_I2C_GRID_CHECK;
	ret = wacom_i2c_send(wac_i2c, &cmd, 1, false);
	if (ret <= 0) {
		printk(KERN_ERR "[E-PEN] failed to send stop command\n");
		goto grid_check_error;
	}

	cmd = WACOM_STATUS;
	do {
		msleep(50);
		if (1 == wacom_i2c_send(wac_i2c, &cmd, 1, false)) {
			if (2 == wacom_i2c_recv(wac_i2c,
						buf, 2, false)) {
				switch (buf[0]) {
				/*
				*	status value
				*	0 : data is not ready
				*	1 : PASS
				*	2 : Fail (coil function error)
				*	3 : Fail (All coil function error)
				*/
				case 1:
				case 2:
				case 3:
					cnt = 0;
					break;

				default:
					break;
				}
			}
		}
	} while (cnt--);

	wac_i2c->connection_check = (1 == buf[0]);
	printk(KERN_DEBUG
	       "[E-PEN] epen_connection : %s %d\n",
	       (1 == buf[0]) ? "Pass" : "Fail", buf[1]);

grid_check_error:
	cmd = WACOM_I2C_STOP;
	wacom_i2c_send(wac_i2c, &cmd, 1, false);

	cmd = WACOM_I2C_START;
	wacom_i2c_send(wac_i2c, &cmd, 1, false);

}
#endif

int wacom_checksum(struct wacom_i2c *wac_i2c)
{
	int ret = 0, retry = 10;
	int i = 0;
	u8 buf[5] = {0, };

	buf[0] = COM_CHECKSUM;

	while (retry--) {
		ret = wacom_i2c_send(wac_i2c, &buf[0], 1, false);
		if (ret < 0) {
			printk(KERN_DEBUG
			       "[E-PEN] i2c fail, retry, %d\n",
			       __LINE__);
			continue;
		}

		msleep(200);
		ret = wacom_i2c_recv(wac_i2c, buf, 5, false);
		if (ret < 0) {
			printk(KERN_DEBUG
			       "[E-PEN] i2c fail, retry, %d\n",
			       __LINE__);
			continue;
		} else if (buf[0] == 0x1f)
			break;
		printk(KERN_DEBUG "[E-PEN] checksum retry\n");
	}

	if (ret >= 0) {
		printk(KERN_DEBUG
		       "[E-PEN] received checksum %x, %x, %x, %x, %x\n",
		       buf[0], buf[1], buf[2], buf[3], buf[4]);
	}

	for (i = 0; i < 5; ++i) {
		if (buf[i] != Firmware_checksum[i]) {
			printk(KERN_DEBUG
			       "[E-PEN] checksum fail %dth %x %x\n", i,
			       buf[i], Firmware_checksum[i]);
			break;
		}
	}

	wac_i2c->checksum_result = (5 == i);

#ifdef WACOM_CONNECTION_CHECK
	if (!wac_i2c->connection_check)
		wacom_open_test(wac_i2c);
#endif

	return ret;
}

int wacom_i2c_query(struct wacom_i2c *wac_i2c)
{
	struct wacom_features *wac_feature = wac_i2c->wac_feature;
	int ret;
	u8 buf;
	u8 data[9] = {0, };
	int i = 0;
	int query_limit = 10;

	buf = COM_QUERY;

	for (i = 0; i < query_limit; i++) {
		ret = wacom_i2c_send(wac_i2c, &buf, 1, false);
		if (ret < 0) {
			printk(KERN_ERR"[E-PEN] I2C send failed(%d)\n", ret);
			continue;
		}
		msleep(100);
		ret = wacom_i2c_recv(wac_i2c, data, COM_QUERY_NUM, false);
		if (ret < 0) {
			printk(KERN_ERR"[E-PEN] I2C recv failed(%d)\n", ret);
			continue;
		}
		printk(KERN_INFO "[E-PEN] %s: %dth ret of wacom query=%d\n",
		       __func__, i, ret);
		if (COM_QUERY_NUM == ret) {
			if (0x0f == data[0]) {
				wac_feature->fw_version =
					((u16) data[7] << 8) + (u16) data[8];
				break;
			} else {
				printk(KERN_NOTICE
				       "[E-PEN] %X, %X, %X, %X, %X, %X, %X, fw=0x%x\n",
				       data[0], data[1], data[2], data[3],
				       data[4], data[5], data[6],
				       wac_feature->fw_version);
			}
		}
	}

#if defined(CONFIG_MACH_Q1_BD)\
	|| defined(CONFIG_MACH_P4NOTE)\
	|| defined(CONFIG_MACH_T0)
	wac_feature->x_max = (u16) WACOM_MAX_COORD_X;
	wac_feature->y_max = (u16) WACOM_MAX_COORD_Y;
#else
	wac_feature->x_max = ((u16) data[1] << 8) + (u16) data[2];
	wac_feature->y_max = ((u16) data[3] << 8) + (u16) data[4];
#endif
	wac_feature->pressure_max = (u16) data[6] + ((u16) data[5] << 8);

#if defined(COOR_WORK_AROUND)
	if (i == 10 || ret < 0) {
		printk(KERN_NOTICE "[E-PEN] COOR_WORK_AROUND is applied\n");
		printk(KERN_NOTICE
		       "[E-PEN] %X, %X, %X, %X, %X, %X, %X, %X, %X\n", data[0],
		       data[1], data[2], data[3], data[4], data[5], data[6],
		       data[7], data[8]);
		wac_feature->x_max = (u16) WACOM_MAX_COORD_X;
		wac_feature->y_max = (u16) WACOM_MAX_COORD_Y;
		wac_feature->pressure_max = (u16) WACOM_MAX_PRESSURE;
#ifdef CONFIG_MACH_T0
		wac_feature->fw_version = 0;
#else
		wac_feature->fw_version = 0xFF;
#endif
	}
#endif

	printk(KERN_NOTICE "[E-PEN] x_max=0x%X\n", wac_feature->x_max);
	printk(KERN_NOTICE "[E-PEN] y_max=0x%X\n", wac_feature->y_max);
	printk(KERN_NOTICE "[E-PEN] pressure_max=0x%X\n",
	       wac_feature->pressure_max);
	printk(KERN_NOTICE "[E-PEN] fw_version=0x%X (d7:0x%X,d8:0x%X)\n",
	       wac_feature->fw_version, data[7], data[8]);
	printk(KERN_NOTICE "[E-PEN] %X, %X, %X, %X, %X, %X, %X, %X, %X\n",
	       data[0], data[1], data[2], data[3], data[4], data[5], data[6],
	       data[7], data[8]);

	if ((i == 10) && (ret < 0)) {
		printk(KERN_DEBUG"[E-PEN] %s, failed\n", __func__);
		wac_i2c->query_status = false;
		return ret;
	}
	wac_i2c->query_status = true;

#if defined(CONFIG_MACH_P4NOTE)
	wacom_checksum(wac_i2c);
#endif

	return 0;
}

#ifdef WACOM_IMPORT_FW_ALGO
#ifdef WACOM_USE_OFFSET_TABLE
void wacom_i2c_coord_offset(u16 *coordX, u16 *coordY)
{
	u16 ix, iy;
	u16 dXx_0, dXy_0, dXx_1, dXy_1;
	int D0, D1, D2, D3, D;

	ix = (u16) (((*coordX)) / CAL_PITCH);
	iy = (u16) (((*coordY)) / CAL_PITCH);

	dXx_0 = *coordX - (ix * CAL_PITCH);
	dXx_1 = CAL_PITCH - dXx_0;

	dXy_0 = *coordY - (iy * CAL_PITCH);
	dXy_1 = CAL_PITCH - dXy_0;

	if (*coordX <= WACOM_MAX_COORD_X) {
		D0 = tableX[user_hand][screen_rotate][ix +
						      (iy * LATTICE_SIZE_X)] *
		    (dXx_1 + dXy_1);
		D1 = tableX[user_hand][screen_rotate][ix + 1 +
						      iy * LATTICE_SIZE_X] *
		    (dXx_0 + dXy_1);
		D2 = tableX[user_hand][screen_rotate][ix +
						      (iy +
						       1) * LATTICE_SIZE_X] *
		    (dXx_1 + dXy_0);
		D3 = tableX[user_hand][screen_rotate][ix + 1 +
						      (iy +
						       1) * LATTICE_SIZE_X] *
		    (dXx_0 + dXy_0);
		D = (D0 + D1 + D2 + D3) / (4 * CAL_PITCH);

		if (((int)*coordX + D) > 0)
			*coordX += D;
		else
			*coordX = 0;
	}

	if (*coordY <= WACOM_MAX_COORD_Y) {
		D0 = tableY[user_hand][screen_rotate][ix +
						      (iy * LATTICE_SIZE_X)] *
		    (dXy_1 + dXx_1);
		D1 = tableY[user_hand][screen_rotate][ix + 1 +
						      iy * LATTICE_SIZE_X] *
		    (dXy_1 + dXx_0);
		D2 = tableY[user_hand][screen_rotate][ix +
						      (iy +
						       1) * LATTICE_SIZE_X] *
		    (dXy_0 + dXx_1);
		D3 = tableY[user_hand][screen_rotate][ix + 1 +
						      (iy +
						       1) * LATTICE_SIZE_X] *
		    (dXy_0 + dXx_0);
		D = (D0 + D1 + D2 + D3) / (4 * CAL_PITCH);

		if (((int)*coordY + D) > 0)
			*coordY += D;
		else
			*coordY = 0;
	}
}
#endif

#ifdef WACOM_USE_AVERAGING
void wacom_i2c_coord_average(unsigned short *CoordX, unsigned short *CoordY,
			     int bFirstLscan)
{
	unsigned char i;
	unsigned int work;
	unsigned char ave_step = 4, ave_shift = 2;
	static unsigned short Sum_X, Sum_Y;
	static unsigned short AveBuffX[4], AveBuffY[4];
	static unsigned char AvePtr;
	static unsigned char bResetted;

	if (bFirstLscan == 0)
		bResetted = 0;
	else {
		if (bFirstLscan && (bResetted == 0)) {
			AvePtr = 0;

			ave_step = 4;
			ave_shift = 2;

			for (i = 0; i < ave_step; i++) {
				AveBuffX[i] = *CoordX;
				AveBuffY[i] = *CoordY;
			}
			Sum_X = (unsigned short)*CoordX << ave_shift;
			Sum_Y = (unsigned short)*CoordY << ave_shift;
			bResetted = 1;
		} else if (bFirstLscan) {
			Sum_X = Sum_X - AveBuffX[AvePtr] + (*CoordX);
			AveBuffX[AvePtr] = *CoordX;
			work = Sum_X >> ave_shift;
			*CoordX = (unsigned int)work;

			Sum_Y = Sum_Y - AveBuffY[AvePtr] + (*CoordY);
			AveBuffY[AvePtr] = (*CoordY);
			work = Sum_Y >> ave_shift;
			*CoordY = (unsigned int)work;

			if (++AvePtr >= ave_step)
				AvePtr = 0;
		}
	}

}
#endif

#if defined(WACOM_USE_BOXFILTER)
/*Center*/
int g_boxThreshold_C[] = {0, 0, 0, };
int g_boxThreshold_X[] = {30, 20, 20, };
int g_boxThreshold_Y[] = {50, 20, 20, };
/*Transition*/
int g_boxThreshold_Trs[] = {130, 20, 20, };

void boxfilt(unsigned short *CoordX, unsigned short *CoordY,
			int height, int bFirstLscan)
{
	bool isMoved = false;
	static bool bFirst = true;
	static unsigned short lastX_loc, lastY_loc;
	static unsigned char bResetted;
	int threshold = 0;
	int distance = 0;
	bool transition = false;
	bool isXMoved = false;

	/*Reset filter*/
	if (bFirstLscan == 0) {
		bResetted = 0;
		return ;
	}

	if (bFirstLscan && (bResetted == 0)) {
		lastX_loc = *CoordX;
		lastY_loc = *CoordY;
		bResetted = 1;
	}

	if (bFirst) {
		lastX_loc = *CoordX;
		lastY_loc = *CoordY;
		bFirst = false;
	}

	/*Start Filtering*/
	threshold = g_boxThreshold_C[height];

	if (*CoordX > X_INC_E1)
		threshold = g_boxThreshold_X[height];
	else if (*CoordX < X_INC_S1)
		threshold = g_boxThreshold_X[height];

	/*Right*/
	if (*CoordY > Y_INC_E1) {
		/*Transition*/
		if (*CoordY < Y_INC_E2 && *CoordY > Y_INC_E3) {
			transition = true;
			threshold += g_boxThreshold_Trs[height];
		} else
			threshold += g_boxThreshold_Y[height];
	}
	/*Left*/
	else if (*CoordY < Y_INC_S1) {
		/*Transition*/
		if (*CoordY > Y_INC_S2 && *CoordY < Y_INC_S3) {
			transition = true;
			threshold += g_boxThreshold_Trs[height];
		} else {
			threshold += g_boxThreshold_Y[height];
		}
	}

	/*Check Stop condition*/
	if (threshold == 0)
		return ;

	/*X*/
	distance = abs(*CoordX - lastX_loc);
	if (transition) {
		if (distance >= 70) {
			isMoved = true;
			isXMoved = true;
		}
	} else if (distance >= threshold)
		isMoved = true;

	/*Y*/
	if (isMoved == false) {
		distance = abs(*CoordY - lastY_loc);
		if (distance >= threshold)
			isMoved = true;
	}

	/*Update position*/
	if (isMoved) {
		if (isXMoved)
			lastX_loc = *CoordX;
		else {
			lastX_loc = *CoordX;
			lastY_loc = *CoordY;
		}
	} else {
		*CoordX = lastX_loc;
		*CoordY = lastY_loc;
	}
}
#endif

#endif /*WACOM_IMPORT_FW_ALGO*/

static bool wacom_i2c_coord_range(u16 *x, u16 *y)
{
#if defined(CONFIG_MACH_P4NOTE)
	if ((*x <= WACOM_POSX_OFFSET) || (*y <= WACOM_POSY_OFFSET))
		return false;
#endif
	if ((*x <= WACOM_POSX_MAX) && (*y <= WACOM_POSY_MAX))
		return true;

	return false;
}

int wacom_i2c_coord(struct wacom_i2c *wac_i2c)
{
	bool prox = false;
	int ret = 0;
	u8 *data;
	int rubber, stylus;
	static u16 x, y, pressure;
	static u16 tmp;
	int rdy = 0;

#ifdef WACOM_IRQ_WORK_AROUND
	cancel_delayed_work(&wac_i2c->pendct_dwork);
#endif

	data = wac_i2c->wac_feature->data;
	ret = wacom_i2c_recv(wac_i2c, data, COM_COORD_NUM, false);

	if (ret < 0) {
		printk(KERN_ERR "[E-PEN] %s failed to read i2c.L%d\n", __func__,
		       __LINE__);
		return -1;
	}
#if defined(CONFIG_SAMSUNG_KERNEL_DEBUG_USER)
#if defined(CONFIG_MACH_T0)
	/*printk(KERN_DEBUG"[E-PEN] %x, %x, %x, %x, %x, %x, %x %x\n",
		data[0], data[1], data[2], data[3],
		data[4], data[5], data[6], data[7]);*/
#else
	pr_debug("[E-PEN] %x, %x, %x, %x, %x, %x, %x\n",
		data[0], data[1], data[2], data[3], data[4], data[5], data[6]);
#endif
#endif
	if (data[0] & 0x80) {
		/* enable emr device */
		if (!wac_i2c->pen_prox) {

			wac_i2c->pen_prox = 1;

			if (data[0] & 0x40)
				wac_i2c->tool = BTN_TOOL_RUBBER;
			else
				wac_i2c->tool = BTN_TOOL_PEN;
#if defined(CONFIG_SAMSUNG_KERNEL_DEBUG_USER)
			pr_debug("[E-PEN] is in(%d)\n", wac_i2c->tool);
#endif
		}

		prox = !!(data[0] & 0x10);
		stylus = !!(data[0] & 0x20);
		rubber = !!(data[0] & 0x40);
		rdy = !!(data[0] & 0x80);

		x = ((u16) data[1] << 8) + (u16) data[2];
		y = ((u16) data[3] << 8) + (u16) data[4];
		pressure = ((u16) data[5] << 8) + (u16) data[6];

#ifdef WACOM_IMPORT_FW_ALGO
		/* Change Position to Active Area */
		if (x <= origin_offset[0])
			x = 0;
		else
			x = x - origin_offset[0];
		if (y <= origin_offset[1])
			y = 0;
		else
			y = y - origin_offset[1];

#ifdef WACOM_USE_OFFSET_TABLE
		if (wac_i2c->use_offset_table)
			wacom_i2c_coord_offset(&x, &y);
#endif
#ifdef WACOM_USE_AVERAGING
		wacom_i2c_coord_average(&x, &y, rdy);
#endif
#ifdef WACOM_USE_BOXFILTER
		if (wac_i2c->use_box_filter && pressure == 0)
			boxfilt(&x, &y, 0, rdy);
#endif
#endif
		if (wac_i2c->wac_pdata->x_invert)
			x = wac_i2c->wac_feature->x_max - x;
		if (wac_i2c->wac_pdata->y_invert)
			y = wac_i2c->wac_feature->y_max - y;

		if (wac_i2c->wac_pdata->xy_switch) {
			tmp = x;
			x = y;
			y = tmp;
		}
#ifdef WACOM_USE_TILT_OFFSET
		/* Add offset */
		x = x + tilt_offsetX[user_hand][screen_rotate];
		y = y + tilt_offsetY[user_hand][screen_rotate];
#endif
		if (wacom_i2c_coord_range(&x, &y)) {
			input_report_abs(wac_i2c->input_dev, ABS_X, x);
			input_report_abs(wac_i2c->input_dev, ABS_Y, y);
			input_report_abs(wac_i2c->input_dev,
					 ABS_PRESSURE, pressure);
			input_report_key(wac_i2c->input_dev,
					 BTN_STYLUS, stylus);
			input_report_key(wac_i2c->input_dev, BTN_TOUCH, prox);
			input_report_key(wac_i2c->input_dev, wac_i2c->tool, 1);
			if (wac_i2c->rdy_pdct) {
				wac_i2c->rdy_pdct = false;
				input_report_key(wac_i2c->input_dev,
					KEY_PEN_PDCT, 0);
			}
			input_sync(wac_i2c->input_dev);
			wac_i2c->last_x = x;
			wac_i2c->last_y = y;

			if (prox && !wac_i2c->pen_pressed) {
#ifdef CONFIG_SEC_TOUCHSCREEN_DVFS_LOCK
				set_dvfs_lock(wac_i2c, true);
#endif
#if defined(CONFIG_SAMSUNG_KERNEL_DEBUG_USER)
				printk(KERN_DEBUG
				       "[E-PEN] is pressed(%d,%d,%d)(%d)\n",
				       x, y, pressure, wac_i2c->tool);
#else
				printk(KERN_DEBUG "[E-PEN] pressed\n");
#endif

			} else if (!prox && wac_i2c->pen_pressed) {
#ifdef CONFIG_SEC_TOUCHSCREEN_DVFS_LOCK
				set_dvfs_lock(wac_i2c, false);
#endif
#if defined(CONFIG_SAMSUNG_KERNEL_DEBUG_USER)
				printk(KERN_DEBUG
				       "[E-PEN] is released(%d,%d,%d)(%d)\n",
				       x, y, pressure, wac_i2c->tool);
#else
				printk(KERN_DEBUG "[E-PEN] released\n");
#endif
			}

			wac_i2c->pen_pressed = prox;

			if (stylus && !wac_i2c->side_pressed)
				printk(KERN_DEBUG "[E-PEN] side on\n");
			else if (!stylus && wac_i2c->side_pressed)
				printk(KERN_DEBUG "[E-PEN] side off\n");

			wac_i2c->side_pressed = stylus;
		}
#if defined(CONFIG_SAMSUNG_KERNEL_DEBUG_USER)
		else
			printk(KERN_DEBUG "[E-PEN] raw data x=0x%x, y=0x%x\n",
			x, y);
#endif
	} else {

#ifdef WACOM_IRQ_WORK_AROUND
		if (!gpio_get_value(wac_i2c->wac_pdata->gpio_pendct)) {
			x = ((u16) data[1] << 8) + (u16) data[2];
			y = ((u16) data[3] << 8) + (u16) data[4];

			if (data[0] & 0x40)
				wac_i2c->tool = BTN_TOOL_RUBBER;
			else
				wac_i2c->tool = BTN_TOOL_PEN;

			input_report_abs(wac_i2c->input_dev, ABS_X, x);
			input_report_abs(wac_i2c->input_dev, ABS_Y, y);
			input_report_abs(wac_i2c->input_dev, ABS_PRESSURE, 0);
			input_report_key(wac_i2c->input_dev, BTN_STYLUS, 0);
			input_report_key(wac_i2c->input_dev, BTN_TOUCH, 0);
			input_report_key(wac_i2c->input_dev, wac_i2c->tool, 1);
			input_sync(wac_i2c->input_dev);
		}

		schedule_delayed_work(&wac_i2c->pendct_dwork, HZ / 10);

		return 0;
#else				/* WACOM_IRQ_WORK_AROUND */
#ifdef WACOM_USE_AVERAGING
		/* enable emr device */
		wacom_i2c_coord_average(0, 0, 0);
#endif
#ifdef WACOM_USE_BOXFILTER
		if (wac_i2c->use_box_filter)
			boxfilt(0, 0, 0, 0);
#endif

#ifdef WACOM_PDCT_WORK_AROUND
		if (wac_i2c->pen_pdct == PDCT_DETECT_PEN)
			forced_hover(wac_i2c);
		else
#endif
		if (wac_i2c->pen_prox) {
			/* input_report_abs(wac->input_dev,
			   ABS_X, x); */
			/* input_report_abs(wac->input_dev,
			   ABS_Y, y); */

			input_report_abs(wac_i2c->input_dev, ABS_PRESSURE, 0);
			input_report_key(wac_i2c->input_dev, BTN_STYLUS, 0);
			input_report_key(wac_i2c->input_dev, BTN_TOUCH, 0);
#if defined(WACOM_PDCT_WORK_AROUND)
			input_report_key(wac_i2c->input_dev,
				BTN_TOOL_RUBBER, 0);
			input_report_key(wac_i2c->input_dev, BTN_TOOL_PEN, 0);
			input_report_key(wac_i2c->input_dev, KEY_PEN_PDCT, 0);
#else
			input_report_key(wac_i2c->input_dev, wac_i2c->tool, 0);
#endif
			input_sync(wac_i2c->input_dev);

			printk(KERN_DEBUG "[E-PEN] is out");
		}
		wac_i2c->pen_prox = 0;
		wac_i2c->pen_pressed = 0;
		wac_i2c->side_pressed = 0;
		wac_i2c->last_x = 0;
		wac_i2c->last_y = 0;

#ifdef CONFIG_SEC_TOUCHSCREEN_DVFS_LOCK
		set_dvfs_lock(wac_i2c, false);
#endif
#endif
	}

	return 0;
}
