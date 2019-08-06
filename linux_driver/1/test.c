#include <stdio.h>

//包含EXIT_SUCCESS等宏定义
#include <stdlib.h>


//包含read函数
#include <unistd.h>

//包含open函数的头文件
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

//包含ioctl函数的头文件
#include <sys/ioctl.h>


int main(int argc, char * argv[])
{
	int fd;
	int ret_ioctl, ret_close;
	char * pathname = "/dev/kaka_driver";
	int repeat_time = 0;

	if(argc < 2)
	{
		printf("arguments are too few!\n");
		return EXIT_FAILURE;
	}

	/*以读写方式打开文件，非阻塞方式打开*/
	fd = open(pathname, O_RDWR|O_NDELAY);
	if (fd == -1)
	{
		perror("open");
		return EXIT_FAILURE;
	}
	else
	{
		printf("\napp open %s success\n",pathname);
		ioctl(fd,atoi(argv[1]));
		sleep(5);
	}

	close(fd);

	return EXIT_SUCCESS;

}