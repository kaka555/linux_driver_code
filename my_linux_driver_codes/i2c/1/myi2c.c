
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/time.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/cpufreq.h>
#include <linux/slab.h>
#include <linux/io.h>

#include <asm/irq.h>

MODULE_DESCRIPTION("S3C24XX I2C Bus driver");
MODULE_AUTHOR("kaka");
MODULE_LICENSE("GPL");

struct kaka_i2c_struct {
	struct i2c_adapter	adap;
};


/* s3c24xx_i2c_xfer
 *
 * first port of call from the i2c bus code when an message needs
 * transferring across the i2c bus.
*/

static int ka_i2c_xfer(struct i2c_adapter *adap,
			struct i2c_msg *msgs, int num)
{
	struct kaka_i2c_struct *i2c = (struct kaka_i2c_struct *)adap->algo_data;

	printk("going to ka_i2c_xfer()\n");
	return num;
}

u32 s3c24xx_i2c_func(struct i2c_adapter *i2c_adapter_ptr)
{
	return I2C_FUNC_I2C;
}

/* i2c bus registration info */

static const struct i2c_algorithm s3c24xx_i2c_algorithm = {
	.master_xfer		= ka_i2c_xfer,
	.functionality		= s3c24xx_i2c_func,
};


static int ka_i2c_probe(struct platform_device *pdev)
{
	struct kaka_i2c_struct *i2c;
	int ret;

	printk("going to ka_i2c_probe()\n");

	i2c = kzalloc(sizeof(struct kaka_i2c_struct), GFP_KERNEL);
	if (!i2c) {
		dev_err(&pdev->dev, "no memory for state\n");
		return -ENOMEM;
	}

	strlcpy(i2c->adap.name, "kakakakka-i2c", sizeof(i2c->adap.name));
	i2c->adap.owner   = THIS_MODULE;
	i2c->adap.algo    = &s3c24xx_i2c_algorithm;
	i2c->adap.retries = 2;
	i2c->adap.class   = I2C_CLASS_HWMON | I2C_CLASS_SPD;
	//i2c->tx_setup     = 50;

	/* find the clock and enable it */

	//i2c->dev = &pdev->dev;

	/* setup info block for the i2c core */

	i2c->adap.algo_data = i2c;
	i2c->adap.dev.parent = &pdev->dev;

	/* Note, previous versions of the driver used i2c_add_adapter()
	 * to add the bus at any number. We now pass the bus number via
	 * the platform data, so if unset it will now default to always
	 * being bus 0.
	 */

	i2c->adap.nr = 2;

	ret = i2c_add_numbered_adapter(&i2c->adap);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to add bus to i2c core\n");
		return -1;
	}

	platform_set_drvdata(pdev, i2c);

	dev_info(&pdev->dev, "%s: S3C I2C adapter\n", dev_name(&i2c->adap.dev));
	printk("going out ka_i2c_probe()\n");
	return 0;
}

/* s3c24xx_i2c_remove
 *
 * called when device is removed from the bus
*/

static int s3c24xx_i2c_remove(struct platform_device *pdev)
{
	
	struct kaka_i2c_struct *i2c = platform_get_drvdata(pdev);
	printk("going to s3c24xx_i2c_remove()\n");
	i2c_del_adapter(&i2c->adap);

	//release_resource(i2c->ioarea);

	return 0;
}

/* device driver for platform bus bits */
/*
static struct platform_device_id s3c24xx_driver_ids[] = {
	{
		.name		= "ka-i2c",
	}, { },
};
MODULE_DEVICE_TABLE(platform, s3c24xx_driver_ids);
*/
static struct platform_driver s3c24xx_i2c_driver = {
	.probe		= ka_i2c_probe,
	.remove 	= s3c24xx_i2c_remove,
	//.id_table	= s3c24xx_driver_ids,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "lizhe-i2c",
		//.pm	= S3C24XX_DEV_PM_OPS,
	},
};

static void platform_ka_release(struct device * dev)
{
	return ;
}

static struct platform_device i2c_platform_device=  
{  
    .name = "lizhe-i2c",  
    .id = -1,
    .dev = {
        .release = platform_ka_release,
    }
};  

static int __init i2c_init(void)
{
	int ret = platform_device_register(&i2c_platform_device);
	printk("i2c_device_init\n"); 
	if(ret < 0)
	{
		printk("platform_device_register fail\n");
		return ret;
	}
	ret = platform_driver_register(&s3c24xx_i2c_driver);
	if(ret < 0)
	{
		printk("platform_driver_register fail\n");
		platform_device_unregister(&i2c_platform_device); 
		return ret;
	}
	return ret;
}
module_init(i2c_init);

static void __exit i2c_exit(void)
{
	platform_device_unregister(&i2c_platform_device); 
	platform_driver_unregister(&s3c24xx_i2c_driver);
}
module_exit(i2c_exit);


