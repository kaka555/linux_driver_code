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
 
#define GPIO_KEY	72
 
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Radia");
 
static struct input_dev *input;
static struct hello_platform_data *pdata;
 
struct hello_platform_data {//设备结构体中的用户自定义结构体 定义
    int gpio;
    int num;
    int type;
};
 
static struct hello_platform_data hello_pdata = {   //设备结构体中的用户自定义结构体 填入初始数据
    .gpio = GPIO_KEY,
    .num = 17,
    .type= 3,
};
 
static struct platform_device hello_device = {//设备结构体填入初始数据
	.name = DRIVER_NAME,
	.id = -1,
	.dev = {
		.platform_data = &hello_pdata,//用户自定义结构体 加入到设备结构体中
	}
};
 
static irqreturn_t  button_interrupt(int irq, void *dev_id)
{
    int state = (gpio_get_value_cansleep(pdata->gpio) ? 1 : 0) ^ 1;
    printk(KERN_EMERG "<0>hello report event val:%d\n", state);
	input_report_key(input, KEY_VOLUMEUP, !!state);
	input_sync(input);
	return IRQ_HANDLED;
}
 
static int hello_probe(struct platform_device *pdv)//在probe时，设备结构体被以参数的形式传递了进来，同时自定义的hello_platform_data也在其中了
{
    pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
    pdata = pdv->dev.platform_data;//获取到hello_platform_data结构体指针
	int irq = gpio_to_irq(pdata->gpio);	
	printk(KERN_EMERG "hello key probe\n");
    gpio_request(pdata->gpio, "gpio_key_test_button");
    gpio_direction_input(pdata->gpio);
	
	request_irq(irq, button_interrupt, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "button_irq", NULL);
	input = input_allocate_device();
	set_bit(EV_KEY, input->evbit);
	input->name = "hello_gpio_key";
	input->id.bustype = BUS_HOST;
	set_bit(KEY_VOLUMEUP, input->keybit);
	input_register_device(input);
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
