
tty_struct 结构体被 tty 核心用来保存当前 tty 端口的状态
struct tty_struct {
	int	magic;
	struct kref kref;
	struct tty_driver *driver;
	const struct tty_operations *ops;
	int index;
	/* The ldisc objects are protected by tty_ldisc_lock at the moment */
	struct tty_ldisc ldisc;
	struct mutex termios_mutex;
	spinlock_t ctrl_lock;
	/* Termios values are protected by the termios mutex */
	struct ktermios *termios, *termios_locked;
	struct termiox *termiox;	/* May be NULL for unsupported */
	char name[64];
	struct pid *pgrp;		/* Protected by ctrl lock */
	struct pid *session;
	unsigned long flags;
	int count;
	struct winsize winsize;		/* termios mutex */
	unsigned char stopped:1, hw_stopped:1, flow_stopped:1, packet:1;
	unsigned char low_latency:1, warned:1;
	unsigned char ctrl_status;	/* ctrl_lock */
	unsigned int receive_room;	/* Bytes free for queue */

	struct tty_struct *link;
	struct fasync_struct *fasync;
	struct tty_bufhead buf;		/* Locked internally */
	int alt_speed;		/* For magic substitution of 38400 bps */
	wait_queue_head_t write_wait;
	wait_queue_head_t read_wait;
	struct work_struct hangup_work;
	void *disc_data;
	void *driver_data;
	struct list_head tty_files;

#define N_TTY_BUF_SIZE 4096

	/*
	 * The following is data for the N_TTY line discipline.  For
	 * historical reasons, this is included in the tty structure.
	 * Mostly locked by the BKL.
	 */
	unsigned int column;
	unsigned char lnext:1, erasing:1, raw:1, real_raw:1, icanon:1;
	unsigned char closing:1;
	unsigned short minimum_to_wake;
	unsigned long overrun_time;
	int num_overrun;
	unsigned long process_char_map[256/(8*sizeof(unsigned long))];
	char *read_buf;
	int read_head;
	int read_tail;
	int read_cnt;
	unsigned long read_flags[N_TTY_BUF_SIZE/(8*sizeof(unsigned long))];
	int canon_data;
	unsigned long canon_head;
	unsigned int canon_column;
	struct mutex atomic_read_lock;
	struct mutex atomic_write_lock;
	unsigned char *write_buf;
	int write_cnt;
	spinlock_t read_lock;
	/* If the tty has a pending do_SAK, queue it here - akpm */
	struct work_struct SAK_work;
	struct tty_port *port;
};

struct tty_driver {
        int     magic;          /* magic number for this structure */
        struct cdev cdev;
        struct module   *owner;
        const char      *driver_name;
        const char      *name;
        int     name_base;      /* offset of printed name */
        int     major;          /* major device number */
        int     minor_start;    /* start of minor device number */
        int     minor_num;      /* number of *possible* devices */
        int     num;            /* number of devices allocated */
        short   type;           /* type of tty driver */
        short   subtype;        /* subtype of tty driver */
        struct ktermios init_termios; /* Initial termios */
        int     flags;          /* tty driver flags */
        int     refcount;       /* for loadable tty drivers */
        struct proc_dir_entry *proc_entry; /* /proc fs entry */
        struct tty_driver *other; /* only used for the PTY driver */

        /*
         * Pointer to the tty data structures
         */
        struct tty_struct **ttys;
        struct ktermios **termios;
        struct ktermios **termios_locked;
        void *driver_state;     /* only used for the PTY driver */

        /*
         * Interface routines from the upper tty layer to the tty
         * driver.      Will be replaced with struct tty_operations.
         */
        int  (*open)(struct tty_struct * tty, struct file * filp);
        void (*close)(struct tty_struct * tty, struct file * filp);
        int  (*write)(struct tty_struct * tty,
                      const unsigned char *buf, int count);
        void (*put_char)(struct tty_struct *tty, unsigned char ch);
        void (*flush_chars)(struct tty_struct *tty);
        int  (*write_room)(struct tty_struct *tty);
        int  (*chars_in_buffer)(struct tty_struct *tty);
        int  (*ioctl)(struct tty_struct *tty, struct file * file,
                    unsigned int cmd, unsigned long arg);
        long (*compat_ioctl)(struct tty_struct *tty, struct file * file,
                             unsigned int cmd, unsigned long arg);
        void (*set_termios)(struct tty_struct *tty, struct ktermios * old);
        void (*throttle)(struct tty_struct * tty);
        void (*unthrottle)(struct tty_struct * tty);
        void (*stop)(struct tty_struct *tty);
        void (*start)(struct tty_struct *tty);
        void (*hangup)(struct tty_struct *tty);
        void (*break_ctl)(struct tty_struct *tty, int state);
        void (*flush_buffer)(struct tty_struct *tty);
        void (*set_ldisc)(struct tty_struct *tty);
        void (*wait_until_sent)(struct tty_struct *tty, int timeout);
        void (*send_xchar)(struct tty_struct *tty, char ch);
        int (*read_proc)(char *page, char **start, off_t off,
                          int count, int *eof, void *data);
        int (*write_proc)(struct file *file, const char __user *buffer,
                          unsigned long count, void *data);
        int (*tiocmget)(struct tty_struct *tty, struct file *file);
        int (*tiocmset)(struct tty_struct *tty, struct file *file,
                        unsigned int set, unsigned int clear);

        struct list_head tty_drivers;

};

struct ktermios {
        tcflag_t c_iflag;               /* input mode flags */
        tcflag_t c_oflag;               /* output mode flags */
        tcflag_t c_cflag;               /* control mode flags */
        tcflag_t c_lflag;               /* local mode flags */
        cc_t c_line;                    /* line discipline */
        cc_t c_cc[NCCS];                /* control characters */
        speed_t c_ispeed;               /* input speed */
        speed_t c_ospeed;               /* output speed */

};

struct tty_operations {
        int  (*open)(struct tty_struct * tty, struct file * filp);
        void (*close)(struct tty_struct * tty, struct file * filp);
        int  (*write)(struct tty_struct * tty, const unsigned char *buf, int count);
        void (*put_char)(struct tty_struct *tty, unsigned char ch);
        void (*flush_chars)(struct tty_struct *tty);
        int  (*write_room)(struct tty_struct *tty);
        int  (*chars_in_buffer)(struct tty_struct *tty);
        int  (*ioctl)(struct tty_struct *tty, struct file * file, unsigned int cmd, unsigned long arg);
        long (*compat_ioctl)(struct tty_struct *tty, struct file * file, unsigned int cmd, unsigned long arg);
        void (*set_termios)(struct tty_struct *tty, struct ktermios * old);
        void (*throttle)(struct tty_struct * tty);
        void (*unthrottle)(struct tty_struct * tty);
        void (*stop)(struct tty_struct *tty);
        void (*start)(struct tty_struct *tty);
        void (*hangup)(struct tty_struct *tty);
        void (*break_ctl)(struct tty_struct *tty, int state);
        void (*flush_buffer)(struct tty_struct *tty);
        void (*set_ldisc)(struct tty_struct *tty);
        void (*wait_until_sent)(struct tty_struct *tty, int timeout);
        void (*send_xchar)(struct tty_struct *tty, char ch);
        int (*read_proc)(char *page, char **start, off_t off, int count, int *eof, void *data);
        int (*write_proc)(struct file *file, const char __user *buffer, unsigned long count, void *data);
        int (*tiocmget)(struct tty_struct *tty, struct file *file);
        int (*tiocmset)(struct tty_struct *tty, struct file *file, unsigned int set, unsigned int clear);

};



Linux 内核提供一组函数用于操作 tty_driver 结构体及 tty 设备，如下所示。

（1）分配 tty 驱动

struct tty_driver *alloc_tty_driver(int lines);

这个函数返回tty_driver 指针，其参数为要分配的设备数量。

（2）注册 tty 驱动

int tty_register_driver(struct tty_driver *driver);

参数为由 alloc_tty_driver ()分配的 tty_driver 结构体指针，注册 tty 驱动成功时返回 0。

（3）注销 tty 驱动

 int tty_unregister_driver(struct tty_driver *driver);

这个函数与 tty_register_driver ()对应，tty 驱动最终会调用上述函数注销 tty_driver。

（4）注册 tty 设备

struct device *tty_register_device(struct tty_driver *driver,

                                          unsigned index, struct device *dev);

分析：

仅有 tty_driver 是不够的，驱动必须依附于设备，tty_register_device()函数用于注册关联于tty_driver 的设备，index 为设备的索引（范围是 0～driver->num），如：

for (i = 0; i < XXX_TTY_MINORS; ++i)

        tty_register_device(xxx_tty_driver, i, NULL);

（5）注销 tty 设备

    void tty_unregister_device(struct tty_driver *driver, unsigned index);

上述函数与 tty_register_device()对应，用于注销 tty 设备，其使用方法如：

for (i = 0; i < XXX_TTY_MINORS; ++i)

        tty_unregister_device(xxx_tty_driver, i);

（6）设置 tty 驱动操作

void tty_set_operations(struct tty_driver *driver, struct tty_operations *op);

上述函数会将 tty_operations 结构体中的函数指针拷贝给 tty_driver 对应的函数指针。



struct uart_driver {
        struct module           *owner;
        const char              *driver_name;/* 驱动名 */
        const char              *dev_name;/* 设备名 */
        int                      major;/* 主设备号 */
        int                      minor;/* 次设备号 */
        int                      nr;

        struct console          *cons;

        /*
         * these are private; the low level driver should not
         * touch these; they should be initialised to NULL
         */
        struct uart_state       *state;
        struct tty_driver       *tty_driver;
};

注册/注销 uart_driver，使用如下接口：

int uart_register_driver(struct uart_driver *drv);

void uart_unregister_driver(struct uart_driver *drv);

uart_register_driver()和 uart_unregister_driver()中分别包含了 tty_register_driver()和tty_unregister_driver()的操作，

struct uart_port {
        spinlock_t              lock;                   /* port lock */
        unsigned int            iobase;                 /* in/out[bwl] */
        unsigned char __iomem   *membase;               /* read/write[bwl] */
        unsigned int            irq;                    /* irq number */
        unsigned int            uartclk;                /* base uart clock */
        unsigned int            fifosize;               /* tx fifo size */
        unsigned char           x_char;                 /* xon/xoff char */
        unsigned char           regshift;               /* reg offset shift */
        unsigned char           iotype;                 /* io access style */
        unsigned char           unused1;

#define UPIO_PORT               (0)
#define UPIO_HUB6               (1)
#define UPIO_MEM                (2)
#define UPIO_MEM32              (3)
#define UPIO_AU                 (4)                     /* Au1x00 type IO */
#define UPIO_TSI                (5)                     /* Tsi108/109 type IO */
#define UPIO_DWAPB              (6)                     /* DesignWare APB UART */
#define UPIO_RM9000             (7)                     /* RM9000 type IO */

        unsigned int            read_status_mask;       /* driver specific */
        unsigned int            ignore_status_mask;     /* driver specific */
        struct uart_info        *info;                  /* pointer to parent info */
        struct uart_icount      icount;                 /* statistics */
        struct console          *cons;                  /* struct console, if any */
#ifdef CONFIG_SERIAL_CORE_CONSOLE
        unsigned long           sysrq;                  /* sysrq timeout */
#endif
        upf_t                   flags;

#define UPF_FOURPORT            ((__force upf_t) (1 << 1))
#define UPF_SAK                 ((__force upf_t) (1 << 2))
#define UPF_SPD_MASK            ((__force upf_t) (0x1030))
#define UPF_SPD_HI              ((__force upf_t) (0x0010))
#define UPF_SPD_VHI             ((__force upf_t) (0x0020))
#define UPF_SPD_CUST            ((__force upf_t) (0x0030))
#define UPF_SPD_SHI             ((__force upf_t) (0x1000))
#define UPF_SPD_WARP            ((__force upf_t) (0x1010))
#define UPF_SKIP_TEST           ((__force upf_t) (1 << 6))
#define UPF_AUTO_IRQ            ((__force upf_t) (1 << 7))
#define UPF_HARDPPS_CD          ((__force upf_t) (1 << 11))
#define UPF_LOW_LATENCY         ((__force upf_t) (1 << 13))
#define UPF_BUGGY_UART          ((__force upf_t) (1 << 14))
#define UPF_MAGIC_MULTIPLIER    ((__force upf_t) (1 << 16))
#define UPF_CONS_FLOW           ((__force upf_t) (1 << 23))
#define UPF_SHARE_IRQ           ((__force upf_t) (1 << 24))
#define UPF_BOOT_AUTOCONF       ((__force upf_t) (1 << 28))
#define UPF_FIXED_PORT          ((__force upf_t) (1 << 29))
#define UPF_DEAD                ((__force upf_t) (1 << 30))
#define UPF_IOREMAP             ((__force upf_t) (1 << 31))

#define UPF_CHANGE_MASK         ((__force upf_t) (0x17fff))
#define UPF_USR_MASK            ((__force upf_t) (UPF_SPD_MASK|UPF_LOW_LATENCY))

        unsigned int            mctrl;                  /* current modem ctrl settings */
        unsigned int            timeout;                /* character-based timeout */
        unsigned int            type;                   /* port type */
        const struct uart_ops   *ops;
        unsigned int            custom_divisor;
        unsigned int            line;                   /* port index */
        unsigned long           mapbase;                /* for ioremap */
        struct device           *dev;                   /* parent device */
        unsigned char           hub6;                   /* this should be in the 8250 driver */
        unsigned char           unused[3];
        void                    *private_data;          /* generic platform data pointer */
};

struct uart_ops {
	unsigned int	(*tx_empty)(struct uart_port *);
	void		(*set_mctrl)(struct uart_port *, unsigned int mctrl);
	unsigned int	(*get_mctrl)(struct uart_port *);
	void		(*stop_tx)(struct uart_port *);
	void		(*start_tx)(struct uart_port *);
	void		(*send_xchar)(struct uart_port *, char ch);
	void		(*stop_rx)(struct uart_port *);
	void		(*enable_ms)(struct uart_port *);
	void		(*break_ctl)(struct uart_port *, int ctl);
	int		(*startup)(struct uart_port *);
	void		(*shutdown)(struct uart_port *);
	void		(*flush_buffer)(struct uart_port *);
	void		(*set_termios)(struct uart_port *, struct ktermios *new,
				       struct ktermios *old);
	void		(*set_ldisc)(struct uart_port *, int new);
	void		(*pm)(struct uart_port *, unsigned int state,
			      unsigned int oldstate);
	int		(*set_wake)(struct uart_port *, unsigned int state);
	void		(*wake_peer)(struct uart_port *);

	/*
	 * Return a string describing the type of the port
	 */
	const char *(*type)(struct uart_port *);

	/*
	 * Release IO and memory resources used by the port.
	 * This includes iounmap if necessary.
	 */
	void		(*release_port)(struct uart_port *);

	/*
	 * Request IO and memory resources used by the port.
	 * This includes iomapping the port if necessary.
	 */
	int		(*request_port)(struct uart_port *);
	void		(*config_port)(struct uart_port *, int);
	int		(*verify_port)(struct uart_port *, struct serial_struct *);
	int		(*ioctl)(struct uart_port *, unsigned int, unsigned long);
#ifdef CONFIG_CONSOLE_POLL
	void	(*poll_put_char)(struct uart_port *, unsigned char);
	int		(*poll_get_char)(struct uart_port *);
#endif
};
