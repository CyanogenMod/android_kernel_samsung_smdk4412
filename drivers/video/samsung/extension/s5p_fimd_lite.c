/* /linux/driver/video/samsung/s5p_fimd_lite.c
 *
 * Samsung SoC FIMD Lite driver.
 *
 * Author: InKi Dae  <inki.dae@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/fb.h>

#include <plat/clock.h>
#include <plat/fb.h>
#include <plat/cpu.h>
#include <plat/fimd_lite_ext.h>
#include <plat/regs-fb.h>

#include <mach/map.h>

#include "s5p_fimd_ext.h"
#include "s5p_fimd_lite.h"
#include "regs_fimd_lite.h"

#ifdef CONFIG_SLP_DISP_DEBUG
#define FIMD_LITE_MAX_REG	128
#define FIMD_LITE_BASE_REG	0x11C40000
#endif

static void *to_fimd_lite_platform_data(struct s5p_fimd_ext_device *fx_dev)
{
	return fx_dev->dev.platform_data ? (void *)fx_dev->dev.platform_data :
						NULL;
}

static void s5p_fimd_lite_set_par(struct s5p_fimd_lite *fimd_lite,
	unsigned int win_id)
{
	struct exynos_drm_fimd_pdata *lcd;
	struct fb_videomode timing;
	unsigned int cfg;

	lcd = fimd_lite->lcd;
	timing = lcd->panel.timing;

	/* set window control */
	cfg = readl(fimd_lite->iomem_base + S5P_WINCON(win_id));

	cfg &= ~(S5P_WINCON_BITSWP_ENABLE | S5P_WINCON_BYTESWP_ENABLE | \
		S5P_WINCON_HAWSWP_ENABLE | S5P_WINCON_WSWP_ENABLE | \
		S5P_WINCON_BURSTLEN_MASK | S5P_WINCON_BPPMODE_MASK | \
		S5P_WINCON_INRGB_MASK | S5P_WINCON_DATAPATH_MASK);

	/* DATAPATH is LOCAL */
	cfg |= S5P_WINCON_DATAPATH_LOCAL;

	/* pixel format is unpacked RGB888 */
	cfg |= S5P_WINCON_INRGB_RGB | S5P_WINCON_BPPMODE_32BPP;

	writel(cfg, fimd_lite->iomem_base + S5P_WINCON(win_id));

	/* set window position to x=0, y=0*/
	cfg = S5P_VIDOSD_LEFT_X(0) | S5P_VIDOSD_TOP_Y(0);
	writel(cfg, fimd_lite->iomem_base + S5P_VIDOSD_A(win_id));

	cfg = S5P_VIDOSD_RIGHT_X(timing.xres - 1) |
		S5P_VIDOSD_BOTTOM_Y(timing.yres - 1);
	writel(cfg, fimd_lite->iomem_base + S5P_VIDOSD_B(win_id));

	/* set window size for window0*/
	cfg = S5P_VIDOSD_SIZE(timing.xres * timing.yres);
	writel(cfg, fimd_lite->iomem_base + S5P_VIDOSD_C(win_id));

	return;
}

static void s5p_fimd_lite_set_clock(struct s5p_fimd_lite *fimd_lite)
{
	unsigned int cfg = 0, div = 0;
	unsigned int pixel_clock, src_clock, max_clock;
	struct clk *clk;
	struct exynos_drm_fimd_pdata *lcd;
	struct fb_videomode timing;

	lcd = fimd_lite->lcd;
	timing = lcd->panel.timing;

	clk = fimd_lite->clk;

	max_clock = 86 * 1000000;

	pixel_clock = timing.refresh *
		(timing.left_margin + timing.right_margin +
		timing.hsync_len + timing.xres) * (timing.upper_margin +
		timing.lower_margin + timing.vsync_len + timing.yres);

	src_clock = clk_get_rate(clk->parent);

	cfg = readl(fimd_lite->iomem_base + S5P_VIDCON0);
	cfg &= ~(S5P_VIDCON0_VCLKEN_MASK | S5P_VIDCON0_CLKVALUP_MASK |
		 S5P_VIDCON0_CLKVAL_F(0xFF));
	cfg |= (S5P_VIDCON0_CLKVALUP_ALWAYS | S5P_VIDCON0_VCLKEN_NORMAL);

	cfg |= S5P_VIDCON0_CLKSEL_HCLK;

	if (pixel_clock > max_clock)
		pixel_clock = max_clock;

	div = (unsigned int)(src_clock / pixel_clock);
	if (src_clock % pixel_clock)
		div++;

	cfg |= S5P_VIDCON0_CLKVAL_F(div - 1);
	writel(cfg, fimd_lite->iomem_base + S5P_VIDCON0);

	return;
}

static void s5p_fimd_lite_window_on(struct s5p_fimd_lite *fimd_lite,
		unsigned int win_id, unsigned int enable)
{
	unsigned int cfg;

	/* enable window */
	cfg = readl(fimd_lite->iomem_base + S5P_WINCON(win_id));

	cfg &= ~S5P_WINCON_ENWIN_ENABLE;

	if (enable)
		cfg |= S5P_WINCON_ENWIN_ENABLE;

	writel(cfg, fimd_lite->iomem_base + S5P_WINCON(win_id));
}

static void s5p_fimd_lite_lcd_on(struct s5p_fimd_lite *fimd_lite,
		unsigned int enable)
{
	unsigned int cfg;

	cfg = readl(fimd_lite->iomem_base + S5P_VIDCON0);

	cfg &= ~(S5P_VIDCON0_ENVID_ENABLE | S5P_VIDCON0_ENVID_F_ENABLE);

	if (enable)
		cfg |= (S5P_VIDCON0_ENVID_ENABLE | S5P_VIDCON0_ENVID_F_ENABLE);

	writel(cfg, fimd_lite->iomem_base + S5P_VIDCON0);
}

void s5p_fimd_lite_get_vsync_interrupt(struct s5p_fimd_lite *fimd_lite,
	unsigned int enable)
{
	unsigned int cfg;

	cfg = readl(fimd_lite->iomem_base + S5P_VIDINTCON0);
	cfg &= ~(S5P_VIDINTCON0_INTFRMEN_ENABLE | S5P_VIDINTCON0_INT_ENABLE |
		S5P_VIDINTCON0_FRAMESEL0_VSYNC);

	if (enable) {
		cfg |= (S5P_VIDINTCON0_INTFRMEN_ENABLE |
			S5P_VIDINTCON0_INT_ENABLE |
			S5P_VIDINTCON0_FRAMESEL0_VSYNC);
	} else {
		cfg |= (S5P_VIDINTCON0_INTFRMEN_DISABLE |
			S5P_VIDINTCON0_INT_DISABLE);

		cfg &= ~S5P_VIDINTCON0_FRAMESEL0_VSYNC;
	}

	writel(cfg, fimd_lite->iomem_base + S5P_VIDINTCON0);
}

static void s5p_change_dynamic_refresh(struct s5p_fimd_dynamic_refresh
		*fimd_refresh, struct s5p_fimd_ext_device *fx_dev)
{
	unsigned int cfg = 0, ret = 0;
	struct s5p_fimd_lite *fimd_lite = fimd_ext_get_drvdata(fx_dev);
	struct exynos_drm_fimd_pdata *lcd;
	struct fb_videomode timing;
	unsigned long flags;
	u32 vclk, src_clk, refresh;

	lcd = fimd_lite->lcd;
	timing = lcd->panel.timing;

	cfg = readl(fimd_lite->iomem_base + S5P_VIDCON0);
	cfg &= ~(S5P_VIDCON0_CLKVALUP_START_FRAME | S5P_VIDCON0_CLKVAL_F(0xFF));
	cfg |= (S5P_VIDCON0_CLKVALUP_ALWAYS | S5P_VIDCON0_VCLKEN_NORMAL);
	cfg |= S5P_VIDCON0_CLKVAL_F(fimd_refresh->clkdiv - 1);

	if (!irqs_disabled())
		local_irq_save(flags);

	if (timing.refresh == 60) {
		while (1) {
			ret = (__raw_readl(fimd_lite->iomem_base +
						S5P_VIDCON1) >> 13) &
						S5P_VIDCON1_VSTATUS_MASK;
			if (ret == S5P_VIDCON1_VSTATUS_BACKPORCH) {
				__raw_writel(cfg, fimd_lite->iomem_base +
						S5P_VIDCON0);
				ret = (__raw_readl(fimd_refresh->regs +
						VIDCON1) >> 13) &
						S5P_VIDCON1_VSTATUS_MASK;
				if (ret == S5P_VIDCON1_VSTATUS_ACTIVE) {
					__raw_writel(cfg,
						fimd_refresh->regs + VIDCON0);
					break;
				}
			}
		}
	} else {
		while (1) {
			ret = (__raw_readl(fimd_refresh->regs + VIDCON1) >> 13)
					& S5P_VIDCON1_VSTATUS_MASK;
			if (ret == S5P_VIDCON1_VSTATUS_ACTIVE) {
				ret = (__raw_readl(fimd_lite->iomem_base +
						S5P_VIDCON1) >> 13) &
						S5P_VIDCON1_VSTATUS_MASK;
				if (ret == S5P_VIDCON1_VSTATUS_FRONTPORCH) {
					__raw_writel(cfg,
						fimd_refresh->regs + VIDCON0);
					__raw_writel(cfg,
						fimd_lite->iomem_base +
						S5P_VIDCON0);
					break;
				}
			}
		}
	}
	if (irqs_disabled())
		local_irq_restore(flags);

	src_clk = clk_get_rate(fimd_lite->clk->parent);
	vclk = timing.refresh * (timing.left_margin + timing.hsync_len +
		timing.right_margin + timing.xres) *
		(timing.upper_margin + timing.vsync_len +
		 timing.lower_margin + timing.yres);

	refresh = timing.refresh -
			((vclk - (src_clk / fimd_refresh->clkdiv)) / MHZ);
	dev_dbg(fimd_lite->dev, "expected refresh rate: %d fps\n", refresh);
}

static irqreturn_t s5p_fimd_lite_irq_frame(int irq, void *dev_id)
{
	struct s5p_fimd_lite *fimd_lite;

	fimd_lite = (struct s5p_fimd_lite *)dev_id;

	disable_irq_nosync(fimd_lite->irq);

	enable_irq(fimd_lite->irq);

	return IRQ_HANDLED;
}

static void s5p_fimd_lite_logic_start(struct s5p_fimd_lite *fimd_lite,
			unsigned int enable)
{
	unsigned int cfg;

	cfg = 0x2ff47;

	if (enable)
		writel(cfg, fimd_lite->iomem_base + S5P_GPOUTCON0);
	else
		writel(0, fimd_lite->iomem_base + S5P_GPOUTCON0);
}

static void s5p_fimd_lite_lcd_init(struct s5p_fimd_lite *fimd_lite)
{
	unsigned int cfg, rgb_mode, win_id = 0;
	struct exynos_drm_fimd_pdata *lcd;
	struct fb_videomode timing;

	lcd = fimd_lite->lcd;
	timing = lcd->panel.timing;

	cfg = 0;
	cfg |= lcd->vidcon1;

	writel(cfg, fimd_lite->iomem_base + S5P_VIDCON1);

	/* set timing */
	cfg = 0;
	cfg |= S5P_VIDTCON0_VBPD(timing.upper_margin - 1);
	cfg |= S5P_VIDTCON0_VFPD(timing.lower_margin - 1);
	cfg |= S5P_VIDTCON0_VSPW(timing.vsync_len - 1);
	writel(cfg, fimd_lite->iomem_base + S5P_VIDTCON0);

	cfg = 0;
	cfg |= S5P_VIDTCON1_HBPD(timing.left_margin - 1);
	cfg |= S5P_VIDTCON1_HFPD(timing.right_margin - 1);
	cfg |= S5P_VIDTCON1_HSPW(timing.hsync_len - 1);

	writel(cfg, fimd_lite->iomem_base + S5P_VIDTCON1);

	/* set lcd size */
	cfg = 0;
	cfg |= S5P_VIDTCON2_HOZVAL(timing.xres - 1);
	cfg |= S5P_VIDTCON2_LINEVAL(timing.yres - 1);

	writel(cfg, fimd_lite->iomem_base + S5P_VIDTCON2);

	writel(0, fimd_lite->iomem_base + S5P_DITHMODE);

	/* set output to RGB */
	rgb_mode = 0; /* MODE_RGB_P */
	cfg = readl(fimd_lite->iomem_base + S5P_VIDCON0);
	cfg &= ~S5P_VIDCON0_VIDOUT_MASK;

	cfg |= S5P_VIDCON0_VIDOUT_RGB;
	writel(cfg, fimd_lite->iomem_base + S5P_VIDCON0);

	/* set display mode */
	cfg = readl(fimd_lite->iomem_base + S5P_VIDCON0);
	cfg &= ~S5P_VIDCON0_PNRMODE_MASK;
	cfg |= (rgb_mode << S5P_VIDCON0_PNRMODE_SHIFT);
	writel(cfg, fimd_lite->iomem_base + S5P_VIDCON0);

	s5p_fimd_lite_get_vsync_interrupt(fimd_lite, 0);

	/* set par */
	s5p_fimd_lite_set_par(fimd_lite, win_id);

	/* set buffer size */
	cfg = S5P_VIDADDR_PAGEWIDTH(timing.xres * lcd->bpp / 8);
	writel(cfg, fimd_lite->iomem_base + S5P_VIDADDR_SIZE(win_id));

	/* set clock */
	s5p_fimd_lite_set_clock(fimd_lite);

	return;
}

static int s5p_fimd_lite_setup(struct s5p_fimd_ext_device *fx_dev,
				unsigned int enable)
{
	struct s5p_fimd_lite *fimd_lite = fimd_ext_get_drvdata(fx_dev);

	s5p_fimd_lite_logic_start(fimd_lite, enable);

	s5p_fimd_lite_lcd_init(fimd_lite);


	s5p_fimd_lite_window_on(fimd_lite, 0, 1);

	return 0;
}

static int s5p_fimd_lite_start(struct s5p_fimd_ext_device *fx_dev)
{
	struct s5p_fimd_lite *fimd_lite = fimd_ext_get_drvdata(fx_dev);

	s5p_fimd_lite_lcd_on(fimd_lite, 1);

	return 0;
}

static void s5p_fimd_lite_stop(struct s5p_fimd_ext_device *fx_dev)
{
	struct s5p_fimd_lite *fimd_lite = fimd_ext_get_drvdata(fx_dev);

	s5p_fimd_lite_lcd_on(fimd_lite, 0);
}

static void s5p_fimd_lite_power_on(struct s5p_fimd_ext_device *fx_dev)
{
	struct s5p_fimd_lite *fimd_lite = fimd_ext_get_drvdata(fx_dev);

	clk_enable(fimd_lite->clk);
}

static void s5p_fimd_lite_power_off(struct s5p_fimd_ext_device *fx_dev)
{
	struct s5p_fimd_lite *fimd_lite = fimd_ext_get_drvdata(fx_dev);

	clk_disable(fimd_lite->clk);
}

#ifdef CONFIG_SLP_DISP_DEBUG
static int s5p_fimd_lite_read_reg(struct s5p_fimd_lite *fimd_lite, char *buf)
{
	u32 cfg;
	int i;
	int pos = 0;

	pos += sprintf(buf+pos, "0x%.8x | ", FIMD_LITE_BASE_REG);
	for (i = 1; i < FIMD_LITE_MAX_REG + 1; i++) {
		cfg = readl(fimd_lite->iomem_base + ((i-1) * sizeof(u32)));
		pos += sprintf(buf+pos, "0x%.8x ", cfg);
		if (i % 4 == 0)
			pos += sprintf(buf+pos, "\n0x%.8x | ",
				FIMD_LITE_BASE_REG + (i * sizeof(u32)));
	}

	return pos;
}

static ssize_t show_read_reg(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct s5p_fimd_ext_device *fx_dev = to_fimd_ext_device(dev);
	struct s5p_fimd_lite *fimd_lite = fimd_ext_get_drvdata(fx_dev);

	if (!fimd_lite->iomem_base) {
		dev_err(dev, "failed to get current register.\n");
		return -EINVAL;
	}

	return s5p_fimd_lite_read_reg(fimd_lite, buf);
}

static struct device_attribute device_attrs[] = {
	__ATTR(read_reg, S_IRUGO, show_read_reg, NULL),
};
#endif

static int s5p_fimd_lite_probe(struct s5p_fimd_ext_device *fx_dev)
{
	struct clk *sclk = NULL;
	struct resource *res;
	struct s5p_fimd_lite *fimd_lite;
	int ret = -1;
#ifdef CONFIG_SLP_DISP_DEBUG
	int i;
#endif

	fimd_lite = kzalloc(sizeof(struct s5p_fimd_lite), GFP_KERNEL);
	if (!fimd_lite) {
		dev_err(&fx_dev->dev, "failed to alloc fimd_lite object.\n");
		return -EFAULT;
	}

	fimd_lite->dev = &fx_dev->dev;
	fimd_lite->lcd = (struct exynos_drm_fimd_pdata *)
			to_fimd_lite_platform_data(fx_dev);

	res = s5p_fimd_ext_get_resource(fx_dev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&fx_dev->dev, "failed to get io memory region.\n");
		ret = -EINVAL;
		goto err0;
	}

	fimd_lite->iomem_base = ioremap(res->start, resource_size(res));
	if (!fimd_lite->iomem_base) {
		dev_err(&fx_dev->dev, "failed to remap io region\n");
		ret = -EFAULT;
		goto err0;
	}

	fimd_lite->clk = clk_get(&fx_dev->dev, "mdnie0");
	if (IS_ERR(fimd_lite->clk)) {
		dev_err(&fx_dev->dev, "failed to get FIMD LITE clock source\n");
		ret = -EINVAL;
		goto err1;
	}

	sclk = clk_get(&fx_dev->dev, "sclk_mdnie");
	if (IS_ERR(sclk)) {
		dev_err(&fx_dev->dev, "failed to get sclk_mdnie clock\n");
		ret = -EINVAL;
		goto err2;
	}
	fimd_lite->clk->parent = sclk;

	fimd_lite->irq = s5p_fimd_ext_get_irq(fx_dev, 0);

	/* register interrupt handler for fimd-lite. */
	if (request_irq(fimd_lite->irq, s5p_fimd_lite_irq_frame, IRQF_DISABLED,
		fx_dev->name, (void *)fimd_lite)) {
		dev_err(&fx_dev->dev, "request_irq failed\n");
		ret = -EINVAL;
		goto err3;
	}

#ifdef CONFIG_SLP_DISP_DEBUG
	for (i = 0; i < ARRAY_SIZE(device_attrs); i++) {
		ret = device_create_file(&(fx_dev->dev),
					&device_attrs[i]);
		if (ret)
			break;
	}

	if (ret < 0)
		dev_err(&fx_dev->dev, "failed to add sysfs entries\n");
#endif

	fimd_ext_set_drvdata(fx_dev, fimd_lite);

	dev_info(&fx_dev->dev, "fimd lite driver has been probed.\n");

	return 0;

err3:
	free_irq(fimd_lite->irq, fx_dev);
err2:
	clk_put(sclk);
err1:
	iounmap(fimd_lite->iomem_base);
	clk_put(fimd_lite->clk);
err0:
	kfree(fimd_lite);

	return ret;

}

static struct s5p_fimd_ext_driver fimd_ext_driver = {
	.driver		= {
		.name	= "fimd_lite",
		.owner	= THIS_MODULE,
	},
	.change_clock	= s5p_change_dynamic_refresh,
	.power_on	= s5p_fimd_lite_power_on,
	.power_off	= s5p_fimd_lite_power_off,
	.setup		= s5p_fimd_lite_setup,
	.start		= s5p_fimd_lite_start,
	.stop		= s5p_fimd_lite_stop,
	.probe		= s5p_fimd_lite_probe,
};

static int __init s5p_fimd_lite_init(void)
{
	return s5p_fimd_ext_driver_register(&fimd_ext_driver);
}

static void __exit s5p_fimd_lite_exit(void)
{
}

arch_initcall(s5p_fimd_lite_init);
module_exit(s5p_fimd_lite_exit);

MODULE_AUTHOR("InKi Dae <inki.dae@samsung.com>");
MODULE_DESCRIPTION("FIMD Lite Driver");
MODULE_LICENSE("GPL");

