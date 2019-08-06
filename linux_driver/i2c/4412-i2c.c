/**
  ************************************************************************
* @file: i2c touch screen FT5X
* @author: chenheng
* @date: 3-26-2018
* @brief:  
* @attention:
  ************************************************************************
  */
//module_init和module_exit在此头文件中
#include <linux/init.h>
#include <linux/module.h>
/*包含module_param 的头文件*/
#include <linux/moduleparam.h>
/*包含模块参数访问属性的头文件参数 perm，
参数 perm 表示此参数在 sysfs 文件系统中所对应的文件节点
的属性，其权限在“include/linux/stat.h”*/
#include <linux/stat.h>
/*包含字符类设备申请的头文件*/
#include <linux/fs.h>  //三个字符设备函数，包含分配设备号的函数
#include <linux/types.h> //包含dev_t类型
#include <linux/kdev_t.h> //转换设备号宏定义的头文件,获取主次设备号
/*包含申请内存空间的头文件*/
#include <linux/slab.h>
/*定义字符设备的结构体 struct cdev*/
#include <linux/cdev.h>
/*包含函数device_create 、class结构体的头文件*/
#include <linux/device.h>
/*包含ioremap\readl\writel的头文件*/
#include <asm/io.h>  
/*包含copy_from_user等函数的头文件*/
#include <asm/uaccess.h>
/*platform相关函数的头文件*/
#include <linux/platform_device.h>

#include<linux/kernel.h>

#include<asm/uaccess.h>
#include<linux/kdev_t.h>
#include<linux/slab.h>
#include<linux/uaccess.h>
#include<linux/interrupt.h> //中断注册注销头文件
#include<linux/gpio.h>	 //gpio相关的头文件

/*以后写驱动可以将头文件一股脑的加载代码前面*/
#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#include <linux/miscdevice.h>
#include <mach/regs-gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/delay.h>

/*中断函数头文件*/
#include <linux/irq.h>
/*I2C相关的函数*/
#include <linux/i2c.h>

/**
  *@ brief:此函数是为了唤醒I2C芯片，要不然，此芯片不工作
  *@ param:
  *@ retval:
  */
static void screen_io_init(void)
{
	int ret;
	ret = gpio_request(EXYNOS4_GPL0(2), "TP1_EN");
	if (ret) 
	{
		printk(KERN_ERR "failed to request TP1_EN for I2C control\n");
		return ;
	}
	s3c_gpio_cfgpin(EXYNOS4_GPL0(2), S3C_GPIO_OUTPUT);
	gpio_set_value(EXYNOS4_GPL0(2), 1);
	gpio_free(EXYNOS4_GPL0(2));
	mdelay(5);
	ret = gpio_request(EXYNOS4_GPX0(3), "GPX0_3");
	if (ret) 
	{
		gpio_free(EXYNOS4_GPX0(3));
		ret = gpio_request(EXYNOS4_GPX0(3), "GPX0_3");
		if(ret)
		{
			printk("ft5xox: Failed to request GPX0_3 \n");
		}
	}
	s3c_gpio_cfgpin(EXYNOS4_GPX0(3), S3C_GPIO_OUTPUT);
	gpio_set_value(EXYNOS4_GPX0(3), 1);
	gpio_free(EXYNOS4_GPX0(3));
	msleep(300);

}

static void touch_screen_write_byte(struct i2c_client *client,const unsigned char reg,const unsigned char val)
{ 
    char txbuf[2] = {reg,val};
    struct i2c_msg msg[1] = {
        [0] = {
            .addr = client->addr,
            .flags= 0,
            .len = sizeof(txbuf),
            .buf = txbuf,
        },
    };
    i2c_transfer(client->adapter, msg, ARRAY_SIZE(msg));
}
static unsigned char touch_screen_read_byte(struct i2c_client *client,const unsigned char reg)
{
    char txbuf[1] = {reg};
    char rxbuf[1] = {0};
    struct i2c_msg msg[2] = {
        [0] = {
            .addr = client->addr,
            .flags = 0, //0为写
            .len = sizeof(txbuf),
            .buf = txbuf,
        },
        [1] = {
            .addr = client->addr,
            .flags = I2C_M_RD,
            .len = sizeof(rxbuf),
            .buf = rxbuf,
        },
    };

    i2c_transfer(client->adapter, msg, ARRAY_SIZE(msg));
    return rxbuf[0];
}


static int i2c_test_probe(struct i2c_client *client, const struct i2c_device_id *device_id)
{
	printk(KERN_EMERG "i2c_test_probe\r\n");
	unsigned char read_byte = 0xff;


	printk("==%s:\n", __FUNCTION__);
	read_byte = touch_screen_read_byte(client,0x68);
	printk(KERN_EMERG "the value of the register 0x68 is %d\n",read_byte);

	return 0;
}


static int i2c_test_remove(struct i2c_client *client)
{
	printk(KERN_EMERG "i2c_test_remove\r\n");
	return 0;
}

static const struct i2c_device_id i2c_test_id[] = {
	{ "i2c_test", 0 },  /* 该名称必须与BOARD_INFO的匹配才会调用probe函数 */
	/* 所以bsp初始化的时候会调用i2c_register_board_info()函数注册一个I2C_BOARD_INFO
	   然后这里才能匹配上 */
	{ }
};


static struct i2c_driver i2c_test_driver =
{
	.probe = i2c_test_probe, 
	.remove = i2c_test_remove, 
	.id_table	= i2c_test_id,
	.driver	= {
		.name	= "this_name", /* 该名字不需要与别的匹配 */
		.owner	= THIS_MODULE,
	},
};

static int __init i2c_test_init(void)
{
	printk(KERN_EMERG "i2c_test_init\r\n");

	printk("==%s:\n", __FUNCTION__);
	screen_io_init();

	printk("==%s:\n", __FUNCTION__);
	return i2c_add_driver(&i2c_test_driver);
}

static void __exit i2c_test_exit(void)
{
	printk("i2c_test_exit\r\n");

	printk("==%s:\n", __FUNCTION__);
	i2c_del_driver(&i2c_test_driver);
}

late_initcall(i2c_test_init);
module_exit(i2c_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("CHEN HENG");