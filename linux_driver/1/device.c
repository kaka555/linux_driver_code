/**
  ************************************************************************
* @file: platform device file
* @author: chenheng
* @date:  11-12-2017
* @brief:  
* @attention:
  ************************************************************************
  */
#include <linux/init.h>
#include <linux/module.h>  
#include <linux/device.h>  
#include <linux/platform_device.h>  
#include <linux/ioport.h>  

static struct resource beep_resource[] =  
{
    [0] ={
        .start = 0x114000a0,  
        .end =  0x114000a0 + 0x10,  
        .flags = IORESOURCE_MEM,  
    },  
  
    [1] ={
        .start = 0x139D0000,  
        .end =  0x139D0000 + 0x32,  
        .flags = IORESOURCE_MEM,  
    }  
};
static void beep_device_release(struct device *dev)  
{  
    printk("beep_device_release\n");  
    return ;  
}  
  
  
  
static struct platform_device beep_device=  
{  
    .name = "beep_platform",  
    .id = -1,  
    .dev.release = beep_device_release,  
    .num_resources = ARRAY_SIZE(beep_resource),  
    .resource = beep_resource,  
};  
  
static int beep_device_init(void)  
{  
    printk("beep_device_init\n");  
    return platform_device_register(&beep_device);  
}  

static void beep_device_exit(void)  
{  
    printk("beep_device_exit\n"); 
    platform_device_unregister(&beep_device);  
    return;  
}  

module_init(beep_device_init);
module_exit(beep_device_exit);


MODULE_LICENSE("GPL");  
MODULE_AUTHOR("CHEN HENG");