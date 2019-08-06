这个文件夹里主要有3个部分的东西，device.c编译成模块用于给驱动匹配，driver.c是主要驱动的东西，细看；test.c只是一个测试用的，非常简单；
调试过程为：
1.insmod device.ko
2.insmod driver.ko
3.    ./test 1
现象为蜂鸣器叫

主要学习：platform总线设备、驱动的注册，struct file_operations，struct platform_driver