#include <linux/init.h>
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
static irqreturn_t  button_interrupt(int irq, void *dev_id)
{
    int state = (gpio_get_value_cansleep(GPIO_KEY) ? 1 : 0) ^ 1;
    printk("hello report event val:%d\n", state);
	input_report_key(input, KEY_VOLUMEUP, !!state);
	input_sync(input);
	return IRQ_HANDLED;
}
 
static int hello_probe(struct platform_device *pdv)
{
	int irq = gpio_to_irq(GPIO_KEY);	
	printk(KERN_EMERG "hello key probe\n");
    gpio_request(GPIO_KEY, "gpio_key_test_button");
    gpio_direction_input(GPIO_KEY);
	
	request_irq(irq, button_interrupt, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "button_irq", NULL);
	input = input_allocate_device();
	set_bit(EV_KEY, input->evbit);
	input->name = "hello_gpio_key";
	input->id.bustype = BUS_HOST;
	set_bit(KEY_VOLUMEUP, input->keybit);
	input_register_device(input);
	//misc_register(&hello_dev);
	return 0;
}
 
static int hello_remove(struct platform_device *pdv)
{
	struct gpio_keys_drvdata *ddata = platform_get_drvdata(pdv);
	printk(KERN_EMERG "hello key remove\n");
	input_unregister_device(input);
	//misc_deregister(&hello_dev);
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