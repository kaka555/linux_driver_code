/*
*
*相关函数

字符设备申请设备号函数register_chrdev、register_chrdev_region

register_chrdev注册指定主次设备号的设备，手动在/dev下创建该设备的设备节点

register_chrdev_region 注册一个指定主次设备号的设备，后利用class类在/dev/目录下自动创建一个该设备的节点。

字符设备注册函数cdev_add。

本实例为简洁起见使用了misc_register函数，该函数已包含了上面两个步骤。

字符设备驱动注册函数platform_driver_register。

*
*
* 
 */


#include <linux/init.h>
#include <linux/module.h>
 
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
 
#define DRIVER_NAME "hello"
#define DEVICE_NAME "hello"
 
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Radia");
 
static int hello_open(struct inode *inode, struct file *file){
	printk(KERN_EMERG "hello open\n");
	return 0;
}
 
static int hello_release(struct inode *inode, struct file *file){
	printk(KERN_EMERG "hello release\n");
	return 0;
}
 
static long hello_ioctl(struct file *file, unsigned int cmd, unsigned long arg){
	printk("cmd is %d, arg is %d\n", cmd, arg);
	return 0;
}
 
static struct file_operations hello_fops = {
	.owner = THIS_MODULE,
	.open = hello_open,
	.release = hello_release,
	.unlocked_ioctl = hello_ioctl,
};
 
static struct miscdevice hello_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = DEVICE_NAME,
	.fops = &hello_fops,
};
 
static int hello_probe(struct platform_device *pdv)
{
	printk(KERN_EMERG "hello probe\n");
	misc_register(&hello_dev);
	return 0;
}
 
static int hello_remove(struct platform_device *pdv)
{
	printk(KERN_EMERG "hello remove\n");
	misc_deregister(&hello_dev);
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
 
static struct platform_driver hello_driver = {
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
	driver_state = platform_driver_register(&hello_driver);
	printk(KERN_EMERG "platform_driver_register driver_state is %d\n", driver_state);
	platform_device_register_simple(DRIVER_NAME, -1, NULL, 0);
	printk(KERN_EMERG "platform_device_register_simple end\n");
	return 0;
}
 
static void hello_exit(void)
{
	printk(KERN_EMERG "hello module has been remove!\n");
	platform_driver_unregister(&hello_driver);
}
 
module_init(hello_init);
module_exit(hello_exit);
