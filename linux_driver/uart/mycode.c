#include <linux/serial_core.h>
#include <include/linux/kernel.h>

static unsigned int ka_tx_empty(struct uart_port *port)
{
	printk("tiny_tx_empty()\n");
	return 0;
}

static unsigned int ka_get_mctrl(struct uart_port *port)
{
	printk("tiny_get_mctrl()\n");
	return 0;
}

static void ka_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
	printk("tiny_set_mctrl()\n");
}

static void ka_break_ctl(struct uart_port *port, int break_state)
{
	printk("tiny_break_ctl()\n");
}

static void ka_set_termios(struct uart_port *port,
			     struct ktermios *new, struct ktermios *old)
{
	int baud, quot, cflag = new->c_cflag;
	printk("tiny_set_termios()\n");
	/* get the byte size */
	switch (cflag & CSIZE) {
	case CS5:
		printk(KERN_DEBUG " - data bits = 5\n");
		break;
	case CS6:
		printk(KERN_DEBUG " - data bits = 6\n");
		break;
	case CS7:
		printk(KERN_DEBUG " - data bits = 7\n");
		break;
	default: // CS8
		printk(KERN_DEBUG " - data bits = 8\n");
		break;
	}

	/* determine the parity */
	if (cflag & PARENB)
		if (cflag & PARODD)
			pr_debug(" - parity = odd\n");
		else
			pr_debug(" - parity = even\n");
	else
		pr_debug(" - parity = none\n");

	/* figure out the stop bits requested */
	if (cflag & CSTOPB)
		pr_debug(" - stop bits = 2\n");
	else
		pr_debug(" - stop bits = 1\n");

	/* figure out the flow control settings */
	if (cflag & CRTSCTS)
		pr_debug(" - RTS/CTS is enabled\n");
	else
		pr_debug(" - RTS/CTS is disabled\n");

	/* Set baud rate */
        baud = uart_get_baud_rate(port, new, old, 0, port->uartclk/16);
        quot = uart_get_divisor(port, baud);
	
	//UART_PUT_DIV_LO(port, (quot & 0xff));
	//UART_PUT_DIV_HI(port, ((quot & 0xf00) >> 8));
}

static int ka_startup(struct uart_port *port)
{
	printk("tiny_startup()\n");
	/* this is the first time this port is opened */
	/* do any hardware initialization needed here */

#ifdef TIMER_EN
	/* create our timer and submit it */
	if (!timer) {
		timer = kmalloc(sizeof(*timer), GFP_KERNEL);
		if (!timer)
			return -ENOMEM;
	}
	timer->data = (unsigned long)port;
	timer->expires = jiffies + DELAY_TIME;
	timer->function = tiny_timer;
	add_timer(timer);
#endif
	return 0;
}


static void ka_shutdown(struct uart_port *port)
{
	printk("tiny_shutdown()\n");
	/* The port is being closed by the last user. */
	/* Do any hardware specific stuff here */

	/* shut down our timer */
	del_timer(timer);
}

static const char *ka_type(struct uart_port *port)
{
	printk("tiny_type()\n");
	return "katty";
}

static void ka_release_port(struct uart_port *port)
{
	printk("tiny_release_port()\n");
}

static int ka_request_port(struct uart_port *port)
{
	printk("tiny_request_port()\n");
	return 0;
}

static void ka_config_port(struct uart_port *port, int flags)
{
	printk("tiny_config_port()\n");
}

static int ka_verify_port(struct uart_port *port, struct serial_struct *ser)
{
	printk("tiny_verify_port()\n");
	return 0;
}

static struct uart_ops ka_ops = {
	.tx_empty	= ka_tx_empty,
	.set_mctrl	= ka_set_mctrl,
	.get_mctrl	= ka_get_mctrl,
	.stop_tx	= ka_stop_tx,
	.start_tx	= ka_start_tx,
	.stop_rx	= ka_stop_rx,
	.enable_ms	= ka_enable_ms,
	.break_ctl	= ka_break_ctl,
	.startup	= ka_startup,
	.shutdown	= ka_shutdown,
	.set_termios	= ka_set_termios,
	.type		= ka_type,
	.release_port	= ka_release_port,
	.request_port	= ka_request_port,
	.config_port	= ka_config_port,
	.verify_port	= ka_verify_port,
};

static struct uart_port ka_port = {
	.ops		= &ka_ops,
	.line 		= 0,
	.fifosize	= 16,
	.uartclk	= 0,
	.iotype = UPIO_MEM,
	.line 		= 0,
};

static struct uart_driver ka_driver = {
	.owner		= THIS_MODULE,
	.driver_name	= "ka_uart_driver_name",
	.dev_name	= "ka_uart_dev_name",
	.major		= 289,
	.minor		= 1,
	.nr		= 1,
};

static int __init ka_init(void)
{
	int result;
	printk("ka_init()\n");
	result = uart_register_driver(&ka_driver);
	if (result)
		return result;

	result = uart_add_one_port(&ka_driver, &ka_port);
	if (result)
		uart_unregister_driver(&ka_driver);

	return result;
}
module_init(ka_init);

static void __exit ka_exit(void)
{
	printk("ka_exit()\n");
	uart_unregister_driver(&ka_driver);
	uart_remove_one_port(&ka_port)
}
module_exit(ka_exit);

MODULE_LICENSE("GPL");
