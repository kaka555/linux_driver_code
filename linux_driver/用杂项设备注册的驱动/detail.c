
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
 
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
 
#define DRIVER_NAME "hello"
#define DEVICE_NAME "hello"
 
#define GPIO_KEY	75
 
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Radia");
 
static struct input_dev *input;
static struct hello_platform_data *pdata;
 
struct hello_platform_data {//设备结构体中的用户自定义结构体 定义
    int gpio;
    struct work_struct		hello_wq_work;
};
 
static struct hello_platform_data hello_pdata = {   //设备结构体中的用户自定义结构体 填入初始数据
    .gpio = GPIO_KEY,
};
 
static struct platform_device hello_device = {//设备结构体填入初始数据
	.name = DRIVER_NAME,
	.id = -1,
	.dev = {
		.platform_data = &hello_pdata,//用户自定义结构体 加入到设备结构体中
	}
};
 
static int hello_open(struct inode *inode, struct file *file){  
    printk(KERN_EMERG "hello open\n");  
    return 0;  
}  
  
static int hello_release(struct inode *inode, struct file *file){  
    printk(KERN_EMERG "hello release\n");  
    return 0;  
}  
 
static ssize_t hello_read(struct file *file,
				 char *buf, size_t count, loff_t *ppos)
 
{
    printk(KERN_EMERG "hello read!\n");
    memcpy(buf, "hi\n", 3);
    return 3;
}
 
static ssize_t hello_write(struct file *file, 
                const char __user *buf, size_t count, loff_t *ppos)
{
    printk(KERN_EMERG "hello write!\n");
    gpio_direction_output(pdata->gpio, simple_strtoul(buf, NULL, 0));
    return 2;
}
 
 
static long hello_ioctl(struct file *file, unsigned int cmd, unsigned long arg){  
    printk("cmd is %d, arg is %d\n", cmd, arg);  
    return 0;  
}  
  
static struct file_operations hello_fops = {  
    .owner = THIS_MODULE,  
    .open = hello_open,  
    .read = hello_read,
    .write = hello_write,
    .release = hello_release,  
    .unlocked_ioctl = hello_ioctl,  
}; 
 
static int hello_probe(struct platform_device *pdv)//在probe时，设备结构体被以参数的形式传递了进来，同时自定义的hello_platform_data也在其中了
{
    static struct class *hello_class;
    int hello_major;
 
    pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
    pdata = pdv->dev.platform_data;//获取到hello_platform_data结构体指针
    printk(KERN_EMERG "hello key probe\n");
 
    gpio_request(pdata->gpio, "gpio_hello_out\n");
    gpio_direction_output(pdata->gpio, 1);
 
    //注册一个设备类，使mdev可以在/dev/目录下建立设备节点  
    hello_class = class_create(THIS_MODULE, "hello");
    //注册字符设备，这里定义DEVICE_MAJOR=0，让系统去分配，注册成功后将返回动态分配的主设备号  
    hello_major = register_chrdev(0,"hello",&hello_fops);
    //创建一个设备节点，取名为hello_radia
    //注意2.6内核较早版本的函数名是class_device_create，现该为device_create  
    device_create(hello_class,NULL,MKDEV(hello_major,0),NULL,"hello_radia");  
    return 0;
}
 
static int hello_remove(struct platform_device *pdv)
{
	struct gpio_keys_drvdata *ddata = platform_get_drvdata(pdv);
	printk(KERN_EMERG "hello key remove\n");
	input_unregister_device(input);
	return 0;
}
 
static void hello_shutdown(struct platform_device *pdv)
{
}
 
static int hello_suspend(struct platform_device *pdv, pm_message_t pmt)
{
	return 0;
}
 
static int hello_resume(struct platform_device *pdv)
{
	return 0;
}
 
static struct platform_driver hello_driver = {//驱动结构体填入初始数据
	.probe = hello_probe,
	.remove = hello_remove,
	.shutdown = hello_shutdown,
	.suspend = hello_suspend,
	.resume = hello_resume,
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
	}
};
 
static int hello_init(void)
{
	int driver_state;
	printk(KERN_EMERG "hello module has been mount!\n");
	platform_device_register(&hello_device);//使用设备结构体中的数据将设备进行注册
	printk(KERN_EMERG "platform_device_register end\n");
	driver_state = platform_driver_register(&hello_driver);//使用驱动结构体中的数据将驱动进行注册
	printk(KERN_EMERG "platform_driver_register driver_state is %d\n", driver_state);
	return 0;
}
 
static void hello_exit(void)
{
	printk(KERN_EMERG "hello module has been remove!\n");
	platform_driver_unregister(&hello_driver);
}
 
module_init(hello_init);
module_exit(hello_exit);
