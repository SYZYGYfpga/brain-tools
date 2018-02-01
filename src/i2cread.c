#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

int main (int argc, char *argv[])
{
	int file;
	int error = 0;
	int addr = 0;
	char buf[2];

	char filename[20];

	if (argc < 4) {
		printf("Usage: i2cread [i2c device #] [i2c address] [i2c sub-address]\n");
		exit(1);
	}

	snprintf(filename,19,"/dev/i2c-%d", atoi(argv[1]));
	file = open(filename, O_RDWR);
	if (file < 0) {
		printf("Error opening device\n");
		exit(1);
	}

	addr = strtoul(argv[2], NULL, 16);

	if (ioctl(file, I2C_SLAVE, addr) < 0) {
		printf("Error during address set\n");
		exit(1);
	}

	buf[0] = (strtoul(argv[3], NULL, 16) >> 8) & 0xFF;
	buf[1] = (strtoul(argv[3], NULL, 16)) & 0xFF;

	if (write(file,buf,2) != 2) {
		printf("Error during write\n");
		exit(1);
	}

	if (read(file,buf,1) != 1) {
		printf("Error during read\n");
		exit(1);
	} else {
		printf("Result: %.2X\n", buf[0]);
	}

	return error;
}
