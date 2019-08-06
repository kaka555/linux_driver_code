#我们是通过bash的脚本编译的
#!/bin/bash
#通知编译器我们要编译模块的哪些源码
#这里是编译minilinux_module.c这个文件编译成中间文件minilinux_module.o
obj-m += device.o driver.o

#如果有多个源文件，那么添加这个
#char_driver_leds-objs := file1.0 file2.o file3.o

#源码目录变量，这里用户需要根据实际情况选择路径
#作者是将Linux的源码拷贝到目录/home/topeet/android4.0下并解压的
KDIR := /home/topeet/android4.0/iTop4412_Kernel_3.0

#当前目录变量
PWD ?= $(shell pwd)

#make命名默认寻找第一个目标
#make -C就是指调用执行的路径
#$(KDIR)Linux源码目录，作者这里指的是/home/topeet/android4.0/iTop4412_Kernel_3.0
#$(PWD)当前目录变量
#modules要执行的操作
all:
	make -C $(KDIR) M=$(PWD) modules
	arm-none-linux-gnueabi-gcc test.c -o test -static      
#make clean执行的操作是删除后缀为o的文件
clean:
	rm -rf *.o *.ko *.order *.symvers *.mod.c test
