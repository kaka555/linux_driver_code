#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

int main(int argc, char * argv[])
{
	int fd, ret;
	char * i2c_device = "/dev/i2c_touch_screen";

	unsigned char buffer[2];

	printf("open %s!\r\n", i2c_device);
	if ((fd = open(i2c_device, O_RDWR | O_NDELAY)) < 0)
	{
		printf("APP open %s failed", i2c_device);
		return -1;
	}
	else 
		printf("APP open %s success!\n", i2c_device);

	/*0xA6寄存器地址中是存放固件ID*/
	buffer[0] = 0xa6;
	ret = read(fd, buffer, 1);
	if (ret < 0)
	{
		printf("i2c read firmware id failed!\n");
		goto exit;
	}
	else 
	{
		printf("firmware id is 0x%02x!\n", buffer[0]);
	}

	/*read device mode register*/
	buffer[0] = 0x00;
	ret = read(fd, buffer, 1);
	if (ret < 0)
	{
		printf("i2c read device mode failed!\n");
		goto exit;
	}
	printf("the current device mode is 0x%02x!\n", buffer[0]);
	/*change device mode register*/
	printf("change device mode!\n");
	buffer[0] = 0x00;
	buffer[1] = 0x04;
	ret = write(fd, buffer, 2);
	if (ret < 0)
	{
		printf("i2c write failed!\n");
		goto exit;
	}

	/*read device mode register*/
	buffer[0] = 0x00;
	ret = read(fd, buffer, 1);
	if (ret < 0)
	{
		printf("i2c read failed!\n");
		goto exit;
	}
	printf("the current device mode is 0x%02x!\n", buffer[0]);
	close(fd);
	return 0;
exit:
	close(fd);
	return - 1;
}