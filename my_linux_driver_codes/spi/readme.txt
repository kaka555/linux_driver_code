.c文件是4412开发板的例程，里面只使用了spi_register_driver（）注册驱动设备
这是因为spi_master已经由bsp做好了
更多细节参考/drivers/spi/spi.c 以及 /drivers/spi/spidev.c ，还有我自己写的笔记