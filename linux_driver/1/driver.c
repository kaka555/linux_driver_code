/**
  ************************************************************************
* @file: platform driver file
* @author: chenheng
* @date:  11-12-2017
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


/*module_param的定义必须放在任何函数之前，一般在源文件头部*/
/*所有的参数都应该给一个默认的值，可以用来判断用户是否显式地设置了参数的值*/
/*大多数情况下，模块参数都不应该是可写的*/
static int dev_major_num = 0;
static int dev_minor_num = 0;
static dev_t dev_num = 0;

module_param(dev_major_num, int, S_IRUSR);
module_param(dev_minor_num, int, S_IRUSR);

//生成的设备节点的名称
#define CHAR_DEVICE_NAME "kaka_driver"
//申请的设备编号的数目
#define REQUEST_DEV_NUM (1)

/*字符设备结构体*/
#define REQUEST_SIZE (500) //申请的内存空间的大小
typedef struct 
{
	char * data;
	unsigned long size;
	struct cdev my_cdev; //字符设备驱动结构体
} MyCharDev_Typedef;

static MyCharDev_Typedef * pMyCharDev = NULL;

/*设备类*/
static struct class * myclass;
/*PWM控制相关的寄存器的定义*/
struct {  
    unsigned int    TCFG0;  
    unsigned int    TCFG1;  
    unsigned int    TCON;  
    unsigned int    TCNTB0;  
    unsigned int    TCMPB0;  
    unsigned int    TCNTO0;  
    unsigned int    TCNTB1;  
    unsigned int    TCMPB1;  
    unsigned int    TCNTO1;  
    unsigned int    TCNTB2;  
    unsigned int    TCMPB2;  
    unsigned int    TCNTO2;  
    unsigned int    TCNTB3;  
    unsigned int    TCMPB3;  
    unsigned int    TCNTO3;  
    unsigned int    TCNTB4;  
    unsigned int    TCNTO4;  
    unsigned int    TINT_CSTAT;  
}*PWM;

/*用于存放虚拟地址和物理地址*/
volatile unsigned long virt_addr,virt_addr_gpio,phys_addr,phys_addr_gpio;
volatile unsigned long *pgpd0con, *pgpd0pud;
#define BEEP_ON (1)
#define BEEP_OFF (0)



/***************函数声明****************/
static int beep_open(struct inode * p_inode, struct file * p_file);
static int beep_release(struct inode * p_inode, struct file * p_file);
static long beep_unlocked_ioctl(struct file * p_file, unsigned int cmd, unsigned long arg);
static ssize_t beep_read(struct file * p_file, char __user *buffer, size_t size, loff_t * f_pos);
static ssize_t beep_write(struct file * p_file, const char __user *buffer, size_t size, loff_t * f_pos);
static loff_t beep_llseek(struct file * p_file, loff_t f_pos, int orig);


static void fs4412_beep_off(void)
{
	/*关闭PWM*/
	(*PWM).TCON = ((*PWM).TCON & (~(0xf))) | 0x0;
	/*定时器结束之后，输出输出低电平，不让蜂鸣器继续响*/
	*pgpd0con =  (*pgpd0con&(~(0xf)))|0x0;

}

static void fs4412_beep_on(void)
{
	*pgpd0con =  (*pgpd0con&(~(0xf)))|0x2;
	/*开启自动重载*/
	(*PWM).TCON = ((*PWM).TCON & (~(0xf))) | 0x1 | 0x8;

}

static void fs4412_beep_init(void)
{
	*pgpd0con = (*pgpd0con&(~(0xf)))|0x2;
	*pgpd0pud =  *pgpd0pud&(~(0xf));

	/*预分频1-254 + 1*/
	(*PWM).TCFG0 = ((*PWM).TCFG0 & (~(0xff)))|0xf9;
	/*再分频1.2.4.8.16*/
	(*PWM).TCFG1 = ((*PWM).TCFG1 & (~(0xf)))|0x2;
	/*设置占空比*/
	(*PWM).TCMPB0 = 50;
	(*PWM).TCNTB0 = 100;

	/*设置手动加载,开启定时器，手动更新TCMPB0和TCNTB0*/
	(*PWM).TCON = ((*PWM).TCON & (~(0xf))) | 0x1 | 0x2;

	fs4412_beep_off();
}

static void fs4412_beep_unmap(void)
{
	iounmap((void *)virt_addr);
	iounmap((void *)virt_addr_gpio);
}



/*文件操作结构体*/
static struct file_operations file_ops =
{
	/*定义在linux/module.h中，系统使用这个字段是为了避免模块在使用过程中被卸载*/
	.owner = THIS_MODULE, 
	.open = beep_open, 
	.release = beep_release, 
	.unlocked_ioctl = beep_unlocked_ioctl, 
	.read = beep_read, 
	.write = beep_write, 
	.llseek = beep_llseek, 
};

/**********************************
注：
1、loff_t *f_pos：这个参数代表着文件中的当前位置，可以读取这个值，但是不要去修改这个值
	但是llseek是个例外，因为这个函数的目的就是修改文件当前位置
									*/
/**
  *@ brief:  打开设备文件,这常常是对设备文件进行的第一个操作
  *@ param: p_inode: 文件节点,这个节点只有一个，无论用户打开多少个文件，都只是对应着一个inode结构
			p_file:只要打开一个文件，就对应着一个file结构体，file结构体通常用来追踪文件在运行时的状态信息
  *@ retval: 正确：0 
			 错误：-1
  *@attention 如果这个项是 NULL, 设备打开一直成功, 但是你的驱动不会得到通知
  */
static int beep_open(struct inode * p_inode, struct file * p_file)
{
	printk(KERN_EMERG "beep open!\n");
	fs4412_beep_off();

	return 0;
}

/**
  *@ brief:  当最后一个打开设备的用户进程执行close()系统调用的时候,执行这个函数
			 主要任务是清理未结束的输入输出操作，释放资源，用户自定义排他标志的复位等
  *@ param: p_inode: 文件节点,这个节点只有一个，无论用户打开多少个文件，都只是对应着一个inode结构
			p_file:只要打开一个文件，就对应着一个file结构体，file结构体通常用来追踪文件在运行时的状态信息
  *@ retval: 正确：0 
			 错误：-1
  *@attention 如果这个项是 NULL, 设备关闭一直成功, 但是你的驱动不会得到通知
  */
static int beep_release(struct inode * p_inode, struct file * p_file)
{
	printk(KERN_EMERG "beep release!");
	fs4412_beep_off();

	return 0;
}

/**
  *@ brief:  不使用BLK文件系统，将使用此种函数指针代替ioctl，是针对IO的操作，占用内存小
  *@ param: 
			p_file:只要打开一个文件，就对应着一个file结构体，file结构体通常用来追踪文件在运行时的状态信息
			cmd:从用户那里不改变地传下来
			arg:以一个 unsigned long 的形式传递, 不管它是否由用户给定为一个整数或一个指针.
			如果调用程序不传递第 3 个参数, 被驱动操作收到的 arg 值是无定义的.
			因为类型检查在这个额外参数上被关闭, 编译器不能警告你如果一个无效的参数被传递给 ioctl, 
			并且任何关联的错误将难以查找.
  *@ retval: 正确：0 
			 错误：-1
  *@attention 如果这个项是 NULL, 设备关闭一直成功, 但是你的驱动不会得到通知
  */
static long beep_unlocked_ioctl(struct file * p_file, unsigned int cmd, unsigned long arg)
{
	printk(KERN_EMERG "beep unlocked ioctl!");


	if (cmd != BEEP_ON && cmd != BEEP_OFF)
	{
		printk(KERN_EMERG "unlocked ioctrl comand error!\n");
	}

	if (cmd == BEEP_ON)
	{
		fs4412_beep_on();
	}
	else 
	{
		fs4412_beep_off();
	}

	return 0;
}

/**
  *@ brief:  从设备中读取数据 __user代表是用户空间的数据，不可以直接使用
  *@ param: p_file: 读取信息的目标文件结构体指针
			buffer：放置读取信息的缓冲区，是用户空间地址
			size：要读取信息的长度
			f_pos：读的位置相对于文件开头的偏移，在读取信息后，这个指针一般都会移动，移动的值为要读取信�
	�的长度值
  *@ retval: 正确：成功读取的字节数 
			 错误：-EINVAL
  */
static ssize_t beep_read(struct file * p_file, char __user *buffer, size_t size, loff_t * f_pos)
{
	printk(KERN_EMERG "beep read!");
	return 0;
}

/**
  *@ brief:  向设备中写入数据 __user代表是用户空间的数据，不可以直接使用
  *@ param: p_file: 要写入信息的目标文件结构体指针
			buffer：要写入文件的信息缓冲区
			size：要写的信息长度
			f_pos:当前的偏移位置，这个值通常是用来判断写文件是否越界
  *@ retval: 正确：成功写入的字节数 错误：-EINVAL
  *@attention 这个操作和上面的对文件进行读的操作均为阻塞操作
  */
static ssize_t beep_write(struct file * p_file, const char __user *buffer, size_t size, loff_t * f_pos)
{

	int beep_command; //对蜂鸣器的命令
	printk(KERN_EMERG "beep write!");

	if (size != 4)
	{
		return - EINVAL;
	}

	if (copy_from_user(&beep_command, buffer, size))
	{
		return - EFAULT;
	}

	if(beep_command == BEEP_ON)
	{
		fs4412_beep_on();
		printk("beep on! \n");
	}
	else if(beep_command == BEEP_OFF)
	{
		fs4412_beep_off();
		printk("beep off! \n");
	}
	else
	{
		printk("error command! \n");
	}

	return 0;
}

/**
  *@ brief:  这个函数是修改文件的当前读写位置的
  *@ param: p_file: 读取信息的目标文件结构体指针	
			f_pos：文件定位的目标偏移量
			orig：文件定位的起始地址，这个值可以为文件开头(SEEK_SET,0),当前位置(SEEK_CUR,1),文件末尾(SEEK_END,2)
  *@ retval: 正确：新位置(正的) 错误：负数
  */
static loff_t beep_llseek(struct file * p_file, loff_t f_pos, int orig)
{
	printk(KERN_EMERG "beep llseek!");
	return 0;
}

/**
  *@ brief:  将设备注册到系统
  *@ param:
  *@ retval: 0:success 1:failure
  */
static int reg_init_cdev(MyCharDev_Typedef * dev, int index)
{
	int err;
	int devno = MKDEV(dev_major_num, dev_minor_num + index);

	/*对struct cdev结构体初始化,建立cdev与file_operations之间的连接*/
	/*
		(1) 将整个结构体清零；
		(2) 初始化list成员使其指向自身；
		(3) 初始化kobj成员；
		(4) 初始化ops成员；
	*/
	cdev_init(&dev->my_cdev, &file_ops);
	dev->my_cdev.owner = THIS_MODULE;
	//dev->my_cdev.ops = &file_ops;


	/*注册驱动到系统,告诉内核该结构的信息*/
	/*向系统添加一个cdev以完成注册*/
	err = cdev_add(&dev->my_cdev, devno, 1);
	if (err)
	{
		printk(KERN_EMERG "cdev_add %d is fail! %d\n", index, err);
		return - 1;
	}
	else 
	{
		printk(KERN_EMERG "cdev_add %d is success!\n", index);
		return 0;
	}
}


static int beep_probe(struct platform_device *pdev)  
{  
	int ret_register;
	int ret;
	printk("match ok! beep_probe\n");

	/*PWM寄存器重映射*/
	phys_addr = pdev->resource[1].start;/* use member resource of struct platform_device */
	virt_addr = (unsigned long)ioremap(phys_addr,pdev->resource[1].end - pdev->resource[1].start);
	if((void *)virt_addr == NULL)
	{
		printk("virt_addr ioremap return NULL\n");
		return -1;
	}
	PWM = (unsigned long*)(virt_addr+0x00);
	/*端口IO寄存器重映射*/
	phys_addr_gpio = pdev->resource[0].start;
	virt_addr_gpio = (unsigned long)ioremap(phys_addr_gpio,pdev->resource[0].end-pdev->resource[0].start);
	if((void *)virt_addr == NULL)
	{
		printk("virt_addr_gpio ioremap return NULL\n");
		return -1;
	}
	pgpd0con = (unsigned long*)(virt_addr_gpio+0x00);
	pgpd0pud = (unsigned long*)(virt_addr_gpio+0x00A8-0x00A0);

	printk(KERN_EMERG "dev_major_num:%d\r\n", dev_major_num);
	printk(KERN_EMERG "dev_minor_num:%d\r\n", dev_minor_num);
	/*第一步：申请字符类设备号*/
	if (dev_major_num) //用户显式设置了主设备号,那就静态申请设备号
	{
		dev_num = MKDEV(dev_major_num, dev_minor_num);
		/*
			第一个参数是设备编号的起始值，次设备号通常为0
			第二个参数是连续设备编号的个数
			第三个参数是和该设备编号范围关联的设备节点的名称，会出现在/pro/devices 和sysfs
		*/
		ret_register = register_chrdev_region(dev_num, 0, CHAR_DEVICE_NAME);
		/* then there will be a node call /dev/kaka_driver */
		if (ret_register < 0)
		{
			printk(KERN_EMERG "register_chrdev_region error\r\n");
			return - 1;
		}

		printk(KERN_EMERG "register_chrdev_region success!\r\n");
	}
	else 
	{
		/*动态申请设备号*/
		/*
			第一个参数保存已分配范围的第一个编号
			第二个参数是要使用的第一个次设备号，通常写0即可
			第三个参数、第四个参数和register_chrdev_region函数中两个参数的意义相同
		*/
		ret_register = alloc_chrdev_region(&dev_num, 0, REQUEST_DEV_NUM, CHAR_DEVICE_NAME);
		if (ret_register == -1)
		{
			printk(KERN_EMERG "alloc_chrdev_region error\r\n");
			return - 1;
		}

		printk(KERN_EMERG "alloc_chrdev_region success!\r\n");
		//获取主设备号
		dev_major_num = MAJOR(dev_num);
		printk(KERN_EMERG "the first major num is %d\r\n", dev_major_num);
		//获取次设备号
		dev_minor_num = MINOR(dev_num);
		printk(KERN_EMERG "the first minor num is %d\r\n", dev_minor_num);

	}

	/*第二步：注册设备*/
	/*申请内存空间*/
	pMyCharDev = kmalloc(sizeof(MyCharDev_Typedef), GFP_KERNEL);
	if (!pMyCharDev)
	{
		printk(KERN_EMERG "request memory failed\r\n");
		goto fail_kmalloc;
	}

	//清空数据,这里不是必须的
	memset(pMyCharDev, 0, sizeof(MyCharDev_Typedef));
	/*为数据申请空间*/
	pMyCharDev->data = kmalloc(REQUEST_SIZE, GFP_KERNEL);
	if (pMyCharDev->data == NULL)
	{
		printk(KERN_EMERG "request memory for data failed\r\n");
		goto fail_malloc_data;
	}

	//清空数据,这里不是必须的
	memset(pMyCharDev->data, 0, REQUEST_SIZE);
	/*向系统中注册设备*/
	/*将设备注册到系统*/
	ret = reg_init_cdev(pMyCharDev, 0); /* use cdev_init and cdev_add */
	if (ret == -1)
	{
		printk(KERN_EMERG "register char device failed\r\n");
		goto fail_register_dev;
	}

	/*第三步：生成设备节点*/
	//创建类
	myclass = class_create(THIS_MODULE, CHAR_DEVICE_NAME);
	//创建设备节点
	device_create(myclass, NULL, dev_num, NULL, CHAR_DEVICE_NAME);
/*
extern struct device *device_create(
                                    struct class *cls,
                                    struct device *parent,
				    dev_t devt, void *drvdata,
				    const char *fmt, ...)
				    __attribute__((format(printf, 5, 6)));
 */

	//初始化蜂鸣器设备
	fs4412_beep_init();

	return 0;


fail_register_dev:
	kfree(pMyCharDev->data);


fail_malloc_data:
	/*释放内存*/
	kfree(pMyCharDev);


fail_kmalloc:
	/*释放设备编号*/
	unregister_chrdev_region(dev_num, dev_num);
	return - 1;
}  

  
static int beep_remove(struct platform_device *pdev)  
{  
	printk(KERN_EMERG "beep_remove！\r\n");
	fs4412_beep_off();

	fs4412_beep_unmap();  /* use iounmap */

	/*释放设备节点*/
	device_destroy(myclass, dev_num);
	/*释放设备类*/
	class_destroy(myclass);

	/*从系统中移除字符设备*/
	cdev_del(& (pMyCharDev->my_cdev));
	/*释放内存空间*/
	kfree(pMyCharDev->data);
	kfree(pMyCharDev);
	/*释放设备编号*/
	unregister_chrdev_region(dev_num, REQUEST_DEV_NUM);

    return 0;
}  

static struct platform_driver beep_driver=  
{  
	/*要和平台设备的name保持一致*/
    .driver.name = "beep_platform",  
    .probe = beep_probe,  
    .remove = beep_remove,  
};  


static int __init beep_driver_init(void)
{

	printk(KERN_EMERG "beep_driver_init\r\n");
    return platform_driver_register(&beep_driver);  



}

static void __exit beep_driver_exit(void)
{
	printk("beep_driver_exit");  
    platform_driver_unregister(&beep_driver);  
}



module_init(beep_driver_init);
module_exit(beep_driver_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("CHEN HENG");
