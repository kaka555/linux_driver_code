/*
 I2C控制器所在驱动的platform_driver与/arch/arm/mach-xxx中的platform_device通过
 platform总线的match()函数匹配导致platform_driver.probe()函数执行，从而完成I2C
 控制器(i2c_adpater)的注册；而I2C上面挂的触摸屏依附的i2c_driver与/arch/arm/mach-xxx
 中的i2c_board_info指向的设备通过I2C总线的match函数匹配导致i2c_driver.probe()执行，
 从而使触摸屏展开
 */

/* 这是个设备--device */
struct i2c_client {
	unsigned short flags; //标志位
	unsigned short addr; //设备的地址，低7位为芯片地址  
	char name[I2C_NAME_SIZE];//设备的名称，最大为20个字节  
	struct i2c_adapter *adapter;//依附的适配器i2c_adapter，适配器指明所属的总线  
	struct i2c_driver *driver;//指向设备对应的驱动程序  
	struct device dev;//设备结构体  
	int irq;//设备申请的中断号  
	struct list_head detected;//已经被发现的设备链表  
};

设备结构体i2c_client中addr的低8位表示设备地址。设备地址由读写位、器件类型和自定义地址组成，如下图：
第7位是R/W位，0表示写，2表示读，所以I2C设备通常有两个地址，即读地址和写地址；
类型器件由中间4位组成，这是由半导体公司生产的时候就已经固化了；
自定义类型由低3位组成。由用户自己设置；
1、i2c_client数据结构是描述IIC设备的“模板”，驱动程序的设备结构中应包含该结构；
2、adapter指向设备连接的总线适配器，系统可能有多个总线适配器。内核中静态指针数组adapters记录所有已经注册的总线适配器设备；
3、driver是指向设备驱动程序，这个驱动程序是在系统检测到设备存在时赋值的；


struct i2c_driver {
    //驱动的类型
    unsigned int class;
    //当检测到适配器时调用的函数
    int (*attach_adapter)(struct i2c_adapter *) __deprecated;
     //卸载适配器时调用的函数
    int (*detach_adapter)(struct i2c_adapter *) __deprecated;

     //以下是一种新类型驱动需要的函数，这些函数支持IIC设备动态插入和拔出。如果不想支持只实现上面3个。要不实现上面3个。要么实现下面5个。不能同时定义
    //新类型设备探测函数
    int (*probe)(struct i2c_client *, const struct i2c_device_id *);
    //新类型设备移除函数
    int (*remove)(struct i2c_client *);
    //关闭IIC设备
    void (*shutdown)(struct i2c_client *);
    //挂起IIC设备
    int (*suspend)(struct i2c_client *, pm_message_t mesg);
    //恢复IIC设备
    int (*resume)(struct i2c_client *);
    void (*alert)(struct i2c_client *, unsigned int data);
     //使用命令使设备完成特殊的功能。类似ioctl（）函数
    int (*command)(struct i2c_client *client, unsigned int cmd, void *arg);
    //设备驱动结构体
    struct device_driver driver;
    //设备ID表
    const struct i2c_device_id *id_table;
    //自动探测设备的回调函数
    int (*detect)(struct i2c_client *, struct i2c_board_info *);
    //设备所在的地址范围
    const unsigned short *address_list;
    //指向驱动支持的设备
    struct list_head clients;
};


IIC总线适配器就是一个IIC总线控制器，在物理上连接若干个IIC设备。IIC总线适配器本质上是一个物理设备，其主要功能是完成IIC总线控制器相关的数据通信：
struct i2c_adapter
{
	struct module *owner;   //模块计数
        unsigned  int id;  //alogorithm的类型，定义于i2c_id.h中
        unsigned   int  class;   //允许探测的驱动类型
	const struct i2c_algorithm *algo;         //指向适配器的驱动程序
        void *algo_data;   //指向适配器的私有数据，根据不同的情况使用方法不同
        int (*client_register)(struct  i2c_client *);  //设备client注册时调用
        int (*client_unregister(struct  i2c_client *);   //设备client注销时调用
        u8 level;                                                         
	struct  mutex  bus_lock;  //对总线进行操作时，将获得总线锁
        struct  mutex  clist_lock ;   //链表操作的互斥锁
        int timeout;   //超时
	int retries;      //重试次数
	struct device dev;  //指向 适配器的设备结构体
	int  nr ;                                                          
	struct  list_head      clients; //连接总线上的设备的链表
	char name[48];   //适配器名称
	struct completion     dev_released; //用于同步的完成量
};

struct  i2c_algorithm
{
    //传输函数指针，指向实现IIC总线通信协议的函数，用来确定适配器支持那些传输类型
    int  (*master_xfer)(struct  i2c_adapter *adap,  struct  i2c_msg *msg, int num);
    //smbus方式传输函数指针，指向实现SMBus总线通信协议的函数。SMBus和IIC之间可以通过软件方式兼容，所以这里提供了一个函数，但是一般都赋值为NULL            
    int  (*smbus_xfer)(struct  i2c_adapter *adap, u16  addr, unsigned  short flags, char  read_write, u8 command, int size, union  i2c_smbus_data  *data);
    //返回适配器支持的功能
    u32  (*functionality)(struct  i2c_adapter *);
};

