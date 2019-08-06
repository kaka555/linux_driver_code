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
/*包含杂项设备的头文件*/
#include <linux/miscdevice.h>
#include <linux/input.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/regulator/consumer.h>


static struct i2c_client *this_client;

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
            .flags = 0,
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

static int i2c_touch_screen_open_func(struct file *filp)
{
	return 0;
}

static int i2c_touch_screen_release_func(struct file *filp)
{
	return 0;
}

static ssize_t i2c_touch_screen_read_func(struct file *filp, char __user *buffer, size_t count, loff_t *ppos){
	u8 reg_data;
	int ret;
	ret = copy_from_user(&reg_data,buffer,1);
	struct i2c_msg msgs[] = {
		{
			.addr	= this_client->addr,	//0x38
			.flags	= 0,	//写
			.len	= 1,	//要写的数据的长度
			.buf	= &reg_data,
		},
		{
			.addr	= this_client->addr,
			.flags	= I2C_M_RD,
			.len	= 1,
			.buf	= &reg_data,
		},
	};
	ret = i2c_transfer(this_client->adapter, msgs, 2);
	if (ret < 0) {
		pr_err("read reg error!\n");
	}
	ret = copy_to_user(buffer,&reg_data,1);
	return ret;
}

static ssize_t i2c_touch_screen_write_func(struct file *filp, char __user *buffer, size_t count, loff_t *ppos){
	int ret;
	u8 buf[2];
	struct i2c_msg msgs[1];
	ret = copy_from_user(&buf,buffer,2);
	msgs[0].addr	= this_client->addr;	//0x38
	msgs[0].flags	= 0;	//写
	msgs[0].len	= 2;	//第一个是要写的寄存器地址，第二个是要写的内容
	msgs[0].buf	= buf;

	ret = i2c_transfer(this_client->adapter, msgs, 1);
	if (ret < 0) {
		pr_err("write reg 0x%02x error!\n",buf[0]);
	}
	ret = copy_to_user(buffer,buf,1);
	return ret;
}


static struct file_operations i2c_touch_screen_ops = {
	.owner 	= THIS_MODULE,
	.open 	= i2c_touch_screen_open_func,
	.release= i2c_touch_screen_release_func,
	.write  = i2c_touch_screen_write_func,
	.read 	= i2c_touch_screen_read_func,
};

static struct miscdevice i2c_touch_screen_dev = {
	.minor	= MISC_DYNAMIC_MINOR,
	.fops	= &i2c_touch_screen_ops,
	.name	= "i2c_touch_screen", //这个是在/DEV目录下显示的设备节点名称
};


static int i2c_test_probe(struct i2c_client *client, const struct i2c_device_id *device_id)
{
	unsigned char read_byte = 0xff;


	printk("==%s:\n", __FUNCTION__);
	read_byte = touch_screen_read_byte(client,0x68);
	printk(KERN_EMERG "the value of the register 0x68 is %d\n",read_byte);
	this_client = client;
	if(misc_register(&i2c_touch_screen_dev))
	{
		printk(KERN_EMERG "misc_register error!\n");
		return -1;
	}

	return 0;
}


static int i2c_test_remove(struct i2c_client *client)
{
	printk(KERN_EMERG "i2c_test_remove\r\n");
	return 0;
}

static const struct i2c_device_id i2c_test_id[] = {
	{ "i2c_test", 0 },  /* 该名称必须与BOARD_INFO的匹配才会调用probe函数 */
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
	printk("==%s:\n", __FUNCTION__);
	screen_io_init();

	printk("==%s:\n", __FUNCTION__);
	return i2c_add_driver(&i2c_test_driver);
}

static void __exit i2c_test_exit(void)
{
	printk("==%s:\n", __FUNCTION__);
	i2c_del_driver(&i2c_test_driver);
}

late_initcall(i2c_test_init);
module_exit(i2c_test_exit);

MODULE_AUTHOR("chenheng");
MODULE_DESCRIPTION("TsI2CTest");
MODULE_LICENSE("GPL");