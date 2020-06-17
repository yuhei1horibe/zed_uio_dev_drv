/*
 * zed_uio_dev_driver.c
 * Copyright (C) 2019 Yuhei Horibe
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This is a device driver for my device on Zedboard
 *
 */

#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/uio_driver.h>
#include <linux/mod_devicetable.h>
#include <linux/uaccess.h>
#include <linux/types.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/uaccess.h>


// Device name
#define ZED_UIO_MODULES "zed_uio_module"

// Device class and version numbers
static struct device* zed_uio_dev  = NULL;

// My device on Zynq Zedboard
struct zed_uio_dev_data
{
	struct uio_info* info;
    int              zed_uio_dev_major;
    void __iomem*    addr_base;
    unsigned long    size;
};

// Mutex for this device (file)
static DEFINE_MUTEX(zed_uio_dev_mutex);

// ******************************************
// Probe and Remove
// ******************************************
static int zed_uio_dev_probe(struct platform_device *pdev)
{
    int                      retval = 0;
    struct zed_uio_dev_data* data   = NULL;
    struct resource*         res    = platform_get_resource(pdev, IORESOURCE_MEM, 0);

    printk("zed_uio_dev: %s\n", __FUNCTION__);

    // Allocate memory for device data
    data = devm_kzalloc(&pdev->dev, sizeof(struct zed_uio_dev_data), GFP_KERNEL);

    if(data == NULL){
        printk("%s: Failed to allocate memory for device data.", ZED_UIO_MODULES);
        return -ENOMEM;
    }

	data->info = kzalloc(sizeof(struct uio_info), GFP_KERNEL);
	if (!data->info) {
        retval = -ENOMEM;
        goto unreg_class;
    }

    // Get base address
    if(res->start <= 0){
        printk("%s: Failed to get device address from device tree.", ZED_UIO_MODULES);
        retval = -EINVAL;
        goto unreg_class;
    }
    else{
        printk("%s: Register base address is loaded from device tree ... %lx", ZED_UIO_MODULES, (unsigned long)res->start);
        data->size      = (unsigned long)(resource_size(res));
        data->addr_base = (void __iomem*)ioremap(res->start, data->size);
    }

    // UIO info
    data->info->name           = ZED_UIO_MODULES;
	data->info->mem[0].memtype = UIO_MEM_PHYS;
	//data->info->mem[0].addr    = (phys_addr_t)data->addr_base;
	//data->info->mem[0].size    = data->size;
	data->info->mem[0].addr    = res->start;
	data->info->mem[0].size    = data->size;

	data->info->version   = "0.0.1";
	data->info->irq       = 0;
	data->info->irq_flags = 0;
	data->info->handler   = NULL;

    // Register device as UIO device
	if (uio_register_device(&pdev->dev, data->info)) {
        retval = -EINVAL;
        goto unreg_class;
    }

    // Set driver data
    platform_set_drvdata(pdev, data);

    zed_uio_dev = &pdev->dev;

    // Initialize the mutex for /dev fops clients
    mutex_init(&zed_uio_dev_mutex);

    return 0;

// Cleanup for failed operation
unreg_class:
    printk("%s: Driver initialization failed\n", __FUNCTION__);
    kfree(data->info);
    kfree(data);
//out:
    return retval;
}

static int zed_uio_dev_remove(struct platform_device* pdev)
{
    struct zed_uio_dev_data *data = platform_get_drvdata(pdev);

    printk("zed_uio_dev: %s\n", __FUNCTION__);

	uio_unregister_device(data->info);
	iounmap(data->addr_base);

    // Release platform device data
    kfree(data->info);
    kfree(data);
 
    return 0;
}

// ******************************************
// Power management
// ******************************************
// Suspend the device.
static int zed_uio_dev_pm_suspend(struct device *dev)
{
    //struct zed_uio_dev_data* data = dev_get_drvdata(dev);

    printk("%s: PM SUSPEND\n", __FUNCTION__);

    // Suspend the device here

    return 0;
}

// Resume the device.
static int zed_uio_dev_pm_resume(struct device *dev)
{
    //struct zed_uio_dev_data* data = dev_get_drvdata(dev);

    printk("%s: PM RESUME\n", __FUNCTION__);

    // Resume the device here

    return 0;
}

static const struct dev_pm_ops zed_uio_dev_pm_ops = 
{
    .suspend = zed_uio_dev_pm_suspend,
    .resume  = zed_uio_dev_pm_resume,
};


// ******************************************
// Platform device driver
// ******************************************
// Device match table
static const struct of_device_id zed_uio_dev_of_ids[] = 
{
    { .compatible = "xlnx,zed_uio_dev" },
    { }
};
MODULE_DEVICE_TABLE(of, zed_uio_dev_of_ids);

// Platform device driver
static struct platform_driver zed_uio_dev_driver = {
    .probe              = zed_uio_dev_probe,
    .remove             = zed_uio_dev_remove,
    .driver             = {
        .name	        = "zed_uio_dev_drv",
        .of_match_table = zed_uio_dev_of_ids,
        .owner	        = THIS_MODULE,
        .pm	            = &zed_uio_dev_pm_ops,
    },
};

// ******************************************
// init and exit
// ******************************************
static int __init zed_uio_dev_init_test(void)
{
    printk("%s: init\n", ZED_UIO_MODULES);
 
    // Probe this device in init
    return  platform_driver_probe(&zed_uio_dev_driver, zed_uio_dev_probe);
}
module_init(zed_uio_dev_init_test);

static void __exit zed_uio_dev_cleanup(void)
{
    printk("%s: exit.\n", ZED_UIO_MODULES);
    platform_driver_unregister(&zed_uio_dev_driver);
}
module_exit(zed_uio_dev_cleanup);


// ******************************************
// License
// ******************************************
MODULE_AUTHOR("Yuhei Horibe <yuhei1.horibe@gmail.com>");
MODULE_DESCRIPTION("Driver for UIO device on Zynq Zedboard");
MODULE_LICENSE("GPL v2");

