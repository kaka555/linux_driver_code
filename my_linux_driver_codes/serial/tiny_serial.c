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

#define TINY_SERIAL_NAME	"ttytiny"

#define MY_NAME			TINY_SERIAL_NAME

static struct timer_list *timer;

unsigned int ka_count;

static void tiny_stop_tx(struct uart_port *port)
{
	printk(KERN_INFO "going into tiny_stop_tx()\n");
}

static void tiny_stop_rx(struct uart_port *port)
{
	printk(KERN_INFO "going into tiny_stop_rx()\n");
}

static void tiny_enable_ms(struct uart_port *port)
{
	printk(KERN_INFO "going into tiny_enable_ms()\n");
}

static void tiny_tx_chars(struct uart_port *port)
{
	struct circ_buf *xmit = &port->state->xmit;
	int count;
	printk(KERN_INFO "going into tiny_tx_chars()\n");
	if (port->x_char) {
		printk("wrote %2x", port->x_char);
		port->icount.tx++;
		port->x_char = 0;
		return;
	}
	printk(KERN_INFO "1\n");
	if (uart_circ_empty(xmit) || uart_tx_stopped(port)) {
		tiny_stop_tx(port);
		return;
	}
	printk(KERN_INFO "2\n");
	printk(KERN_INFO "fifosize is %d\n",port->fifosize);
	//count = port->fifosize >> 1;
	count = port->fifosize;
	do {
		printk("wrote %2x", xmit->buf[xmit->tail]);
		printk("head is %d,tail is %d\n",xmit->head,xmit->tail);
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		port->icount.tx++;
		if (uart_circ_empty(xmit))
		{
			printk("break\n");
			break;
		}
	} while (--count > 0);
	printk(KERN_INFO "3\n");
	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(port);
	printk(KERN_INFO "4\n");
	if (uart_circ_empty(xmit))
		tiny_stop_tx(port);
}

static void tiny_start_tx(struct uart_port *port)
{
	struct circ_buf *xmit = &port->state->xmit;
	unsigned int i = 0;
	printk(KERN_INFO "going into tiny_start_tx()\n");
	while(!uart_circ_empty(xmit)) /* if we do not complete this step, write operation will sleep */
	{
		printk("i is %u\n",++i);
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
	}
	printk(KERN_INFO "going out tiny_start_tx()\n");
}

static void tiny_timer(unsigned long data)
{
	struct uart_port *port;
	struct tty_struct *tty;

	printk(KERN_INFO "going into tiny_timer()\n");
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
	tty_insert_flip_char(tty, TINY_DATA_CHARACTER, 0);
	if ((ka_count % 5) == 0)
		tty_insert_flip_char(tty, '\n', TTY_NORMAL);

	tty_flip_buffer_push(tty);

	/* resubmit the timer again */
	timer->expires = jiffies + DELAY_TIME;
	add_timer(timer);

	/* see if we have any data to transmit */
	tiny_tx_chars(port);
}

static unsigned int tiny_tx_empty(struct uart_port *port) /* return 0 means not empty, else means empty */
{
	printk(KERN_INFO "going into tiny_tx_empty()\n");
	return 1;
}

static unsigned int tiny_get_mctrl(struct uart_port *port)
{
	printk(KERN_INFO "going into tiny_get_mctrl()\n");
	return 0;
}

static void tiny_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
	printk(KERN_INFO "going into tiny_set_mctrl()\n");
}

static void tiny_break_ctl(struct uart_port *port, int break_state)
{
	printk(KERN_INFO "going into tiny_break_ctl()\n");
}

static void tiny_set_termios(struct uart_port *port,
			     struct ktermios *new, struct ktermios *old)
{
	int baud, quot, cflag = new->c_cflag;
	printk(KERN_INFO "going into tiny_set_termios()\n");
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
			printk(" - parity = odd\n");
		else
			printk(" - parity = even\n");
	else
		printk(" - parity = none\n");

	/* figure out the stop bits requested */
	if (cflag & CSTOPB)
		printk(" - stop bits = 2\n");
	else
		printk(" - stop bits = 1\n");

	/* figure out the flow control settings */
	if (cflag & CRTSCTS)
		printk(" - RTS/CTS is enabled\n");
	else
		printk(" - RTS/CTS is disabled\n");

	/* Set baud rate */
        baud = uart_get_baud_rate(port, new, old, 0, port->uartclk/16);
        quot = uart_get_divisor(port, baud);
	
	//UART_PUT_DIV_LO(port, (quot & 0xff));
	//UART_PUT_DIV_HI(port, ((quot & 0xf00) >> 8));
}

static int tiny_startup(struct uart_port *port)
{
	/* this is the first time this port is opened */
	/* do any hardware initialization needed here */
	printk(KERN_INFO "going into tiny_startup()\n");
	/* create our timer and submit it */
	if (!timer) {
		timer = kmalloc(sizeof(*timer), GFP_KERNEL);
		if (!timer)
			return -ENOMEM;
		init_timer(timer);
	}
	timer->data = (unsigned long)port;
	timer->expires = jiffies + DELAY_TIME;
	timer->function = tiny_timer;
	add_timer(timer);
	return 0;
}

static void tiny_shutdown(struct uart_port *port)
{
	/* The port is being closed by the last user. */
	/* Do any hardware specific stuff here */
	printk(KERN_INFO "going into tiny_shutdown()\n");
	/* shut down our timer */
	del_timer(timer);
}

static const char *tiny_type(struct uart_port *port)
{
	printk(KERN_INFO "going into tiny_type()\n");
	return "tinytty";
}

static void tiny_release_port(struct uart_port *port)
{
	printk(KERN_INFO "going into tiny_release_port()\n");
}

static int tiny_request_port(struct uart_port *port)
{
	printk(KERN_INFO "going into tiny_request_port()\n");
	return 0;
}

static void tiny_config_port(struct uart_port *port, int flags)
{
	printk(KERN_INFO "going into tiny_config_port()\n");
}

static int tiny_verify_port(struct uart_port *port, struct serial_struct *ser)
{
	printk(KERN_INFO "going into tiny_verify_port()\n");
	return 0;
}

static struct uart_ops tiny_ops = {
	.tx_empty	= tiny_tx_empty,
	.set_mctrl	= tiny_set_mctrl,
	.get_mctrl	= tiny_get_mctrl,
	.stop_tx	= tiny_stop_tx,
	.start_tx	= tiny_start_tx, /* call by write, now the data is stored in uart_state->xmit->buf */
	.stop_rx	= tiny_stop_rx,
	.enable_ms	= tiny_enable_ms,
	.break_ctl	= tiny_break_ctl,
	.startup	= tiny_startup, /* call by open() */
	.shutdown	= tiny_shutdown,
	.set_termios	= tiny_set_termios,/* call by open() */
	.type		= tiny_type,
	.release_port	= tiny_release_port,
	.request_port	= tiny_request_port,
	.config_port	= tiny_config_port,
	.verify_port	= tiny_verify_port,
};

static struct uart_port tiny_port = {
	.ops		= &tiny_ops,
	.type 		= 1000,
	.uartclk 	= 1843200,
	.fifosize	= 8,
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
		return result;

	result = uart_add_one_port(&tiny_reg, &tiny_port);
	if (result)
		uart_unregister_driver(&tiny_reg);

	printk(KERN_INFO "Tiny serial driver loaded end\n");
	return result;
}

static void __exit tiny_exit(void)
{
	printk(KERN_INFO "going into tiny_exit()\n");
	uart_remove_one_port(&tiny_reg, &tiny_port);
	uart_unregister_driver(&tiny_reg);
}

module_init(tiny_init);
module_exit(tiny_exit);
