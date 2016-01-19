/* linux/arch/arm/plat/mdnie_ext.h
 *
 * Samsung SoC FIMD Extension Framework Header.
 *
 * InKi Dae <inki.dae@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

struct s5p_fimd_ext_device;

#define fimd_ext_get_drvdata(_dev)	 dev_get_drvdata(&(_dev)->dev)
#define fimd_ext_set_drvdata(_dev, data) dev_set_drvdata(&(_dev)->dev, (data))

struct s5p_fimd_dynamic_refresh {
	void __iomem			*regs;
	unsigned int			dynamic_refresh;
	unsigned int			clkdiv;
};

/**
 * driver structure for fimd extension based driver.
 *
 * this structure should be registered by any extension driver.
 * fimd extension driveer seeks a driver registered through name field
 * and calls these callback functions in appropriate time.
 */
struct s5p_fimd_ext_driver {
	struct	device_driver driver;

	void	(*change_clock)(struct s5p_fimd_dynamic_refresh *fimd_refresh,
				struct s5p_fimd_ext_device *fx_dev);
	void	(*set_clock)(struct s5p_fimd_ext_device *fx_dev);
	int	(*setup)(struct s5p_fimd_ext_device *fx_dev,
				unsigned int enable);
	void	(*power_on)(struct s5p_fimd_ext_device *fx_dev);
	void	(*power_off)(struct s5p_fimd_ext_device *fx_dev);
	int	(*start)(struct s5p_fimd_ext_device *fx_dev);
	void	(*stop)(struct s5p_fimd_ext_device *fx_dev);
	int	(*probe)(struct s5p_fimd_ext_device *fx_dev);
	int	(*remove)(struct s5p_fimd_ext_device *fx_dev);
	int	(*suspend)(struct s5p_fimd_ext_device *fx_dev);
	int	(*resume)(struct s5p_fimd_ext_device *fx_dev);
};

/**
 * device structure for fimd extension based driver.
 *
 * @name: platform device name.
 * @dev: driver model representation of the device.
 * @id: id of device registered and when device is registered
 *	id would be counted.
 * @num_resources: hardware resource count.
 * @resource: a pointer to hardware resource definitions.
 * @modalias: name of the driver to use with the device, or an
 *	alias for that name.
 */
struct s5p_fimd_ext_device {
	char				*name;
	struct device			dev;
	int				id;
	unsigned int			num_resources;
	struct resource			*resource;
	bool				mdnie_enabled;
	bool				enabled;
};

struct mdnie_platform_data {
	unsigned int width;
	unsigned int height;
};


#ifdef CONFIG_MDNIE_SUPPORT
/**
 * register extension driver to fimd extension framework.
 */
int s5p_fimd_ext_register(struct s5p_fimd_ext_driver *fx_drv);
int s5p_fimd_ext_device_register(struct s5p_fimd_ext_device *fx_dev);

/**
 * find a driver object registered to fimd extension framework.
 */
struct s5p_fimd_ext_device *s5p_fimd_ext_find_device(const char *name);

/**
 * convert device driver object to fimd extension device.
 */
struct s5p_fimd_ext_driver *to_fimd_ext_driver(struct device_driver *drv);
#else
#define s5p_fimd_ext_register(dev)		NULL
#define s5p_fimd_ext_device_register(dev)	NULL
#define s5p_fimd_ext_find_device(name)		NULL
#define	to_fimd_ext_driver(drv)			NULL
#endif
