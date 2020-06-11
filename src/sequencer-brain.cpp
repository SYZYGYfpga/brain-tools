// SYZYGY Brain 1 Supply Sequencer programmer software
//
// Tool used to read and write supply sequencing configuration data from a
// SYZYGY AVR MCU running the official firmware.
//
//------------------------------------------------------------------------
// Copyright (c) 2014-2018 Opal Kelly Incorporated
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
// 
//------------------------------------------------------------------------

#include "json.hpp"

#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <argp.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

using json = nlohmann::json;

#define I2C_CHECK_COUNT 2000
#define SEQ_LENGTH 9


// Detect if a device is on a given I2C address, returns 0 if present
int i2cDetect (int i2c_file, int i2c_addr)
{
	uint8_t data[2];

	// Set I2C address
	if (ioctl(i2c_file, I2C_SLAVE, i2c_addr) < 0) {
		return -1;
	}

	data[0] = 0x00;
	data[1] = 0x00;

	if (write(i2c_file, data, 2) != 2) {
		return 1; // I2C device not present
	}

	return 0; // I2C device present
}


// Write to I2C with either a 16- or 8-bit address
int i2cWrite (int i2c_file, int i2c_addr, uint16_t sub_addr,
              int sub_addr_length, int length, uint8_t data[32])
{
	uint8_t *buffer = (uint8_t *)malloc((length * sizeof(uint8_t)) + sub_addr_length);
	int i;

	memcpy(buffer + sub_addr_length, data, length * sizeof(uint8_t));

	// Set I2C address
	if (ioctl(i2c_file, I2C_SLAVE, i2c_addr) < 0) {
		return -1;
	}

	if (sub_addr_length == 2) {
		buffer[0] = (sub_addr >> 8) & 0xFF;
		buffer[1] = sub_addr & 0xFF;
	} else {
		buffer[0] = sub_addr & 0xFF;
	}

	for (i = 0; i < I2C_CHECK_COUNT; i++) {
		// The DNA Spec allows an MCU to NAK subsequent writes when multiple
		// writes are performed, keep trying for I2C_CHECK_COUNT tries before
		// giving up.
		if (write(i2c_file, buffer, sub_addr_length + length)
		      == (length + sub_addr_length)) {
			return 0;
		}
	}

	// We gave up trying to write
	return -1;
}


// Read from I2C with either a 16- or 8-bit address
int i2cRead (int i2c_file, int i2c_addr, uint16_t sub_addr,
             int sub_addr_length, int length, uint8_t data[32])
{
	uint8_t temp_buf[2];

	// Set I2C address
	if (ioctl(i2c_file, I2C_SLAVE, i2c_addr) < 0) {
		return -1;
	}

	if (sub_addr_length == 2) {
		temp_buf[0] = (sub_addr >> 8) & 0xFF;
		temp_buf[1] = sub_addr & 0xFF;
	} else {
		temp_buf[0] = sub_addr & 0xFF;
	}

	if (write(i2c_file, temp_buf, sub_addr_length) != sub_addr_length) {
		return -1;
	}

	if (read(i2c_file, data, length) != length) {
		return -1;
	}

	return 0;
}

// Helper function that allows for writes > 32 bytes to a SYZYGY MCU
int writeMCU (int i2c_file, uint16_t port_addr, int sub_addr, uint8_t *data,
              int length)
{
	int temp_length, current_sub_addr;

	// Useful for debug
	//printf("Writing %d bytes to 0x%X, sub-address 0x%X\n", length, port_addr, sub_addr);

	current_sub_addr = sub_addr;

	while (length > 0) {
		temp_length = (length > 32) ? 32 : length;

		if (i2cWrite(i2c_file, port_addr, current_sub_addr, 2, temp_length,
		             &data[(current_sub_addr - sub_addr)]) != 0) {
			return -1;
		}

		current_sub_addr += temp_length;
		length -= temp_length;
	}

	return 0;
}

// Helper function that allows for reads > 32 bytes from a SYZYGY MCU
int readMCU (int i2c_file, uint16_t port_addr, int sub_addr, uint8_t *data,
             int length)
{
	int current_sub_addr, temp_length;

	// Useful for debug
	//printf("Reading %d bytes from 0x%X, sub-address 0x%X\n", length, port_addr, sub_addr);

	current_sub_addr = sub_addr;

	while (length > 0) {
		temp_length = (length > 32) ? 32 : length;

		if (i2cRead(i2c_file, port_addr, current_sub_addr, 2, temp_length,
		            &data[(current_sub_addr - sub_addr)]) != 0) {
			return -1;
		}

		current_sub_addr += temp_length;
		length -= temp_length;
	}

	return 0;
}

// Helper function to dump a full sequencer register set, determines the length
// of the DNA and returns it
int dumpSeq (int i2c_file, uint16_t port_addr, uint8_t *data)
{
	if (readMCU(i2c_file, port_addr, 0x9000, data, SEQ_LENGTH) != 0) {
		return -1;
	}

	return SEQ_LENGTH;
}


// Help text
void printHelp (char *progname)
{
	printf("Usage: %s [option [argument]] <i2c device>\n", progname);
	printf("  <i2c device> is required for all commands. It must contain the\n");
	printf("               path to the Linux i2c device\n");
	printf("\n");
	printf("  Exactly one of the following options must be specified:\n");
	printf("    -h - print this text\n");
	printf("    -w <filename> - write a binary DNA to a peripheral, takes the DNA filename\n");
	printf("                    as an argument\n");
	printf("    -d <filename> - dump the DNA from a peripheral to a binary file, takes the\n");
	printf("                    DNA filename as an argument\n");
	printf("\n");
	printf("  Examples:\n");
	printf("    Dump DNA from the MCU on Port 1:\n");
	printf("      %s -d seq_file.bin -p 1 /dev/i2c-1\n", progname);
	printf("    Set VIO1 to 3.3V:\n");
	printf("      %s -s -1 330 /dev/i2c-1\n", progname);
}


int main (int argc, char *argv[])
{
	// Options flags
	int hflag = 0;
	int wflag = 0;
	int dflag = 0;
	char i2c_filename[200];
	char seq_filename[200];
	uint8_t seq_buf[9];
	int i2c_file;
	int seq_file;
	int periph_num = 0;
	int curr_opt;
	int err;
	json json_handler;
	uint16_t peripheral_address[] = {0x30, 0x31, 0x32, 0x33};

	// Parse args
	while ((curr_opt = getopt(argc, argv, "w:d:p:h")) != -1) {
		switch(curr_opt)
		{
			case 'w':
				wflag = 1;
				if (optarg){ 
					strcpy(seq_filename, optarg);
				} else {
					printf("No argument specified for -w\n");
					exit(EXIT_FAILURE);
				}
				break;
			case 'd':
				dflag = 1;
				if (optarg){ 
					strcpy(seq_filename, optarg);
				} else {
					printf("No argument specified for -d\n");
					exit(EXIT_FAILURE);
				}
				break;
			case 'p':
				if (optarg){ 
					periph_num = strtol(optarg, NULL, 0) - 1;
				} else {
					printf("No argument specified for -p\n");
					exit(EXIT_FAILURE);
				}
				break;
			case 'h':
				hflag = 1;
				break;
			case '?':
				printHelp(argv[0]);
				exit(EXIT_FAILURE);
			default:
				printHelp(argv[0]);
				exit(EXIT_FAILURE);
		}
	}

	if ((hflag == 1) || (argc == 1)) {
		printHelp(argv[0]);
		return 0;
	}

	// Extract i2c device
	if (optind < argc) {
		strcpy(i2c_filename, argv[optind]);
	} else {
		printf("I2C Device required.\n");
		exit(EXIT_FAILURE);
	}

	// Open I2C file
	i2c_file = open(i2c_filename, O_RDWR);
	if (i2c_file < 0) {
		printf("Error opening i2c device\n");
		exit(EXIT_FAILURE);
	}

	if ((hflag + wflag + dflag) > 1) {
		printf("Invalid set of options specified.\n");
		printHelp(argv[0]);
		return 0;
	}

	if ((wflag == 1) || (dflag == 1)) {
		if (i2cDetect(i2c_file, peripheral_address[periph_num]) != 0) {
			printf("Peripheral at %X not found\n", peripheral_address[periph_num]);
			exit(EXIT_FAILURE);
		}

		seq_file = open(seq_filename, O_RDWR | O_CREAT, 0666);
		if (seq_file < 0) {
			printf("Error opening sequencing file\n");
			exit(EXIT_FAILURE);
		}
	}

	if (wflag == 1) { // Write sequencer registers from a file to a peripheral
		if (read(seq_file, seq_buf, SEQ_LENGTH) != SEQ_LENGTH) {
			printf("Error reading from sequencer file\n");
			exit(EXIT_FAILURE);
		}

		if (writeMCU(i2c_file, peripheral_address[periph_num], 0x9000,
		             seq_buf, SEQ_LENGTH) != 0) {
			printf("Error writing to the MCU\n");
			exit(EXIT_FAILURE);
		}
	} else if (dflag == 1) { // Dump sequencer registers from a peripheral to a file
		err = dumpSeq(i2c_file, peripheral_address[periph_num], seq_buf);

		if (err < 0) {
			printf("Error reading sequencer registers from device\n");
			exit(EXIT_FAILURE);
		}
		
		if (write(seq_file, seq_buf, SEQ_LENGTH) != SEQ_LENGTH) {
			printf("Error writing to sequencer registers file\n");
			exit(EXIT_FAILURE);
		}
	} else {
		printHelp(argv[0]);
		return 0;
	}

	return 0;
}

