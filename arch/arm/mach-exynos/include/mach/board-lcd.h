/*
 * board-lcd.h - lcd Driver of MIDAS Project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
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

#ifndef __BOARD_LCD_H
#define __BOARD_LCD_H __FILE__

extern struct s3c_platform_fb fb_platform_data;
#ifdef CONFIG_FB_S5P_MDNIE
extern struct platform_device mdnie_device;
#endif
#ifdef CONFIG_LCD_FREQ_SWITCH
extern struct platform_device lcdfreq_device;
#endif
#ifdef CONFIG_FB_S5P_S6C1372
extern struct platform_device lcd_s6c1372;
#endif
#ifdef CONFIG_FB_S5P_GD2EVF
extern struct platform_device gd2evf_spi_gpio;
extern struct s3c_platform_fb *mipi_fb_platdata_addr;
extern struct s3c_platform_fb *evf_fb_platdata_addr;
#endif
extern struct ld9040_panel_data s2plus_panel_data;
extern struct samsung_bl_gpio_info smdk4212_bl_gpio_info;
extern struct platform_pwm_backlight_data smdk4212_bl_data;
extern unsigned int lcdtype;

void mipi_fb_init(void);
#ifdef CONFIG_FB_S5P_GD2EVF
void gd2evf_fb_init(void);
#endif

#endif /* __BOARD_LCD_H */
