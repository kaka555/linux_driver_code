/*
 * Tiny Serial driver
 *
 * Copyright (C) 2002-2004 Greg Kroah-Hartman (greg@kroah.com)
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, version 2 of the License.
 *
 * This driver shows how to create a minimal serial driver.  It does not rely on
 * any backing hardware, but creates a timer that emulates data being received
 * from some kind of hardware.
 */

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial.h>
#include <linux/serial_core.h>
#include <linux/module.h>


#define DRIVER_AUTHOR "Greg Kroah-Hartman <greg@kroah.com>"
#define DRIVER_DESC "Tiny serial driver"

/* Module information */
MODULE_AUTHOR( DRIVER_AUTHOR );
MODULE_DESCRIPTION( DRIVER_DESC );
MODULE_LICENSE("GPL");

#define DELAY_TIME		HZ * 2	/* 2 seconds per character */
#define TINY_DATA_CHARACTER	't'

#define TINY_SERIAL_MAJOR	240	/* experimental range */
#define TINY_SERIAL_MINORS	1	/* only have one minor */
#define UART_NR			1	/* only use one port */

#define TINY_SERIAL_NAME	"ttytinykaka"

#define MY_NAME			TINY_SERIAL_NAME

static struct timer_list *timer;

static void tiny_stop_tx(struct uart_port *port)
{
	printk("go into tiny_stop_tx()\n");
}

static void tiny_stop_rx(struct uart_port *port)
{
	printk("go into tiny_stop_rx()\n");
}

static void tiny_enable_ms(struct uart_port *port)
{
	printk("go into tiny_enable_ms()\n");
}

static void tiny_tx_chars(struct uart_port *port)
{
	printk("go into tiny_tx_chars()\n");
	struct circ_buf *xmit = &port->state->xmit;
	int count;

	if (port->x_char) {
		pr_debug("wrote %2x", port->x_char);
		port->icount.tx++;
		port->x_char = 0;
		return;
	}
	if (uart_circ_empty(xmit) || uart_tx_stopped(port)) {
		tiny_stop_tx(port);
		return;
	}

	count = port->fifosize >> 1;
	do {
		pr_debug("wrote %2x", xmit->buf[xmit->tail]);
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		port->icount.tx++;
		if (uart_circ_empty(xmit))
			break;
	} while (--count > 0);

	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(port);

	if (uart_circ_empty(xmit))
		tiny_stop_tx(port);
}

static void tiny_start_tx(struct uart_port *port)
{
	printk("go into tiny_start_tx()\n");
}

static void tiny_timer(struct timer_list * data)
{
	printk("go into tiny_timer()\n");
	struct uart_port *port;
	struct tty_struct *tty;


	port = (struct uart_port *)data;
	if (!port)
		return;
	if (!port->state)
		return;
	tty = port->state->port.tty;
	if (!tty)
		return;

	/* add one character to the tty port */
	/* this doesn't actually push the data through unless tty->low_latency is set */
	tty_insert_flip_char(tty->port, TINY_DATA_CHARACTER, 0);

	tty_flip_buffer_push(tty->port);

	/* resubmit the timer again */
	timer->expires = jiffies + DELAY_TIME;
	add_timer(timer);

	/* see if we have any data to transmit */
	tiny_tx_chars(port);
}

static unsigned int tiny_tx_empty(struct uart_port *port)
{
	printk("go into tiny_tx_empty()\n");
	return 0;
}

static unsigned int tiny_get_mctrl(struct uart_port *port)
{
	printk("go into tiny_get_mctrl()\n");
	return 0;
}

static void tiny_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
	printk("go into tiny_set_mctrl()\n");
}

static void tiny_break_ctl(struct uart_port *port, int break_state)
{
	printk("go into tiny_break_ctl()\n");
}

static void tiny_set_termios(struct uart_port *port,
			     struct ktermios *new, struct ktermios *old)
{
	printk("go into tiny_set_termios()\n");
	int baud, quot, cflag = new->c_cflag;
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

static int tiny_startup(struct uart_port *port)
{
	printk("go into tiny_startup()\n");
	/* this is the first time this port is opened */
	/* do any hardware initialization needed here */

	/* create our timer and submit it */
	if (!timer) {
		timer = kmalloc(sizeof(*timer), GFP_KERNEL);
		if (!timer)
			return -ENOMEM;
	}
	timer->flags = (unsigned long)port;
	timer->expires = jiffies + DELAY_TIME;
	timer->function = tiny_timer;
	add_timer(timer);
	return 0;
}

static void tiny_shutdown(struct uart_port *port)
{
	printk("go into tiny_shutdown()\n");
	/* The port is being closed by the last user. */
	/* Do any hardware specific stuff here */

	/* shut down our timer */
	del_timer(timer);
}

static const char *tiny_type(struct uart_port *port)
{
	printk("go into tiny_type()\n");
	return "tinytty";
}

static void tiny_release_port(struct uart_port *port)
{
	printk("go into tiny_release_port()\n");
}

static int tiny_request_port(struct uart_port *port)
{
	printk("go into tiny_request_port()\n");
	return 0;
}

static void tiny_config_port(struct uart_port *port, int flags)
{
	printk("go into tiny_config_port()\n");
}

static int tiny_verify_port(struct uart_port *port, struct serial_struct *ser)
{
	printk("go into tiny_verify_port()\n");
	return 0;
}

static struct uart_ops tiny_ops = {
	.tx_empty	= tiny_tx_empty,
	.set_mctrl	= tiny_set_mctrl,
	.get_mctrl	= tiny_get_mctrl,
	.stop_tx	= tiny_stop_tx, 
	.stop_rx	= tiny_stop_rx,
	.start_tx	= tiny_start_tx,  //?
	.enable_ms	= tiny_enable_ms,
	.break_ctl	= tiny_break_ctl,
	.startup	= tiny_startup,
	.shutdown	= tiny_shutdown,
	.set_termios	= tiny_set_termios,
	.type		= tiny_type,
	.release_port	= tiny_release_port,
	.request_port	= tiny_request_port,
	.config_port	= tiny_config_port,
	.verify_port	= tiny_verify_port,
};

static struct uart_port tiny_port = {
	.ops		= &tiny_ops,
	//.lock		= SPIN_LOCK_UNLOCKED,
	.iotype		= UPIO_MEM,
	.uartclk	= 0,
	.fifosize	= 16,
	.flags		= UPF_BOOT_AUTOCONF,
	.line		= 0,
};

static struct uart_driver tiny_reg = {
	.owner		= THIS_MODULE,
	.driver_name	= TINY_SERIAL_NAME,
	.dev_name	= TINY_SERIAL_NAME,
	.major		= TINY_SERIAL_MAJOR,
	.minor		= TINY_SERIAL_MINORS,
	.nr		= UART_NR,
};

static int __init tiny_init(void)
{
	int result;

	printk(KERN_INFO "Tiny serial driver loaded\n");

	result = uart_register_driver(&tiny_reg);
	if (result)
	{
		printk(KERN_INFO "uart_register_driver() fail\n");
		return result;
	}

	result = uart_add_one_port(&tiny_reg, &tiny_port);
	if (result)
	{
		printk(KERN_INFO "uart_add_one_port() fail\n");
		uart_unregister_driver(&tiny_reg);
	}

	if(0 == result)
	{
		printk(KERN_INFO "tiny_init() succeed\n");
	}
	return result;
}

module_init(tiny_init);

static void __exit tiny_serial_exit(void)
{
	uart_unregister_driver(&tiny_reg);
	printk("now going to exit\n");
}
module_exit(tiny_serial_exit);

MODULE_LICENSE("GPL");
