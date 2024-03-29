// SYZYGY Brain 1 SmartVIO controller software
//
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

extern "C" {
#include "syzygy.h"
}

using json = nlohmann::json;

#define I2C_CHECK_COUNT 2000

// Brain 1 SmartVIO Characteristics
// 2 SmartVIO Groups
// Group 1 has 1 port, FPGA range is 1.2 to 3.3 V
// Group 2 has 3 ports, FPGA range is 1.2 to 3.3 V

szgSmartVIOConfig svio = {
	SVIO_NUM_PORTS, SVIO_NUM_GROUPS, {0,0}, {0x1, 0x2}, {
		{
			// Group 1
			0x00, // i2c_addr
			1,    // present
			0,    // group
			0,    // req_ver_major
			0,    // req_ver_minor
			0x00, // attr
			0x00, // port_attr
			0,    // doublewide_mate
			1,    // range_count
			{ {120, 330}, {0,0}, {0,0}, {0,0} } // ranges
		}, {
			0x30, // i2c_addr
			0,    // present
			0,    // group
			0,    // req_ver_major
			0,    // req_ver_minor
			0x00, // attr
			0x00, // port_attr
			0,    // doublewide_mate
			0,    // range_count
			{ {0, 0}, {0,0}, {0,0}, {0,0} } // ranges
		}, {
			// Group 2
			0x00, // i2c_addr
			1,    // present
			1,    // group
			0,    // req_ver_major
			0,    // req_ver_minor
			0x00, // attr
			0x00, // port_attr
			1,    // doublewide_mate
			1,    // range_count
			{ {120, 330}, {0,0}, {0,0}, {0,0} } // ranges
		}, {
			0x31, // i2c_addr
			0,    // present
			1,    // group
			0,    // req_ver_major
			0,    // req_ver_minor
			0x00, // attr
			0x00, // port_attr
			1,    // doublewide_mate
			0,    // range_count
			{ {0, 0}, {0,0}, {0,0}, {0,0} } // ranges
		}, {
			0x32, // i2c_addr
			0,    // present
			1,    // group
			0,    // req_ver_major
			0,    // req_ver_minor
			0x00, // attr
			0x00, // port_attr
			1,    // doublewide_mate
			0,    // range_count
			{ {0, 0}, {0,0}, {0,0}, {0,0} } // ranges
		}, {
			0x33, // i2c_addr
			0,    // present
			1,    // group
			0,    // req_ver_major
			0,    // req_ver_minor
			0x00, // attr
			0x00, // port_attr
			1,    // doublewide_mate
			0,    // range_count
			{ {0, 0}, {0,0}, {0,0}, {0,0} } // ranges
		}
	}
};

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

// Helper function to dump a full DNA, determines the length of
// the DNA and returns it
int dumpDNA (int i2c_file, uint16_t port_addr, uint8_t *data)
{
	uint16_t dna_length;

	if (i2cRead(i2c_file, port_addr, 0x8000, 2, 2, (uint8_t *)&dna_length) != 0) {
		return -1;
	}

	if (dna_length > 1318) {
		printf("Invalid DNA Length\n");
		exit(EXIT_FAILURE);
	}

	if (readMCU(i2c_file, port_addr, 0x8000, data, dna_length) != 0) {
		return -1;
	}

	return dna_length;
}


// Read DNA and determine a SmartVIO solution, stored in 'svio1' and 'svio2'
int readDNA (int i2c_file, uint32_t *svio1, uint32_t *svio2)
{
	uint8_t i;
	int vmin;
	uint8_t dna_buf[64];

	for (i = 0; i < SVIO_NUM_PORTS; i++) {
		// Skip ports referring to the FPGA
		if (0x00 == svio.ports[i].i2c_addr) {
			continue;
		}

		if (i2cDetect(i2c_file, svio.ports[i].i2c_addr) != 0) {
			// Device is not present
			continue;
		}

		// Read the full DNA Header
		if (readMCU(i2c_file, svio.ports[i].i2c_addr, 0x8000, dna_buf,
		            SZG_DNA_HEADER_LENGTH_V1) != 0) {
			return -1;
		}

		if (szgParsePortDNA(i, &svio, dna_buf, SZG_DNA_HEADER_LENGTH_V1) != 0) {
			return -1;
		}

		if (svio.ports[i].attr & SZG_ATTR_LVDS) {
			switch (svio.ports[i].group) {
				case 0:
					svio.ports[0].ranges[0].min = 250;
					svio.ports[0].ranges[0].max = 250;
					break;
				case 1:
					svio.ports[2].ranges[0].min = 250;
					svio.ports[2].ranges[0].max = 250;
					break;
			}
		}
	}

	// Find a solution
	for (i = 0; i < SVIO_NUM_GROUPS; i++) {
		vmin = szgSolveSmartVIOGroup(svio.ports, svio.group_masks[i]);
		if (vmin > 0) {
			svio.svio_results[i] = vmin;
		}
	}

	*svio1 = svio.svio_results[0];
	*svio2 = svio.svio_results[1];

	return 0;
}


// Apply SmartVIO settings to power IC
int applyVIO (int i2c_file, uint32_t svio1, uint32_t svio2)
{
	uint8_t temp_data[2];
	
	// Bounds check to be sure that everything is good to go
	if ((svio1 != 0) && ((svio1 < 120) || (svio1 > 330))) {
		printf("Invalid SmartVIO solution\n");
		exit(EXIT_FAILURE);
	}
	if ((svio2 != 0) && ((svio2 < 120) || (svio2 > 330))) {
		printf("Invalid SmartVIO solution\n");
		exit(EXIT_FAILURE);
	}

	// Disable write protect on TPS65400
	temp_data[0] = 0x20;
	if (i2cWrite(i2c_file, 0x6a, 0x10, 1, 1, temp_data) != 0) {
		return -1;
	}

	// TPS65400 VREF = VOUT * 531 - 60
	if (svio1 != 0) {
		printf("Setting VIO1 to: %d\n", svio1);
		temp_data[0] = 0x0;
		if (i2cWrite(i2c_file, 0x6a, 0x00, 1, 1, temp_data) != 0) {
			return -1;
		}
		temp_data[0] = svio1 * 531 / 1000 - 60;
		if (i2cWrite(i2c_file, 0x6a, 0xd8, 1, 1, temp_data) != 0) {
			return -1;
		}
	}
	if (svio2 != 0) {
		printf("Setting VIO2 to: %d\n", svio2);
		temp_data[0] = 0x1;
		if (i2cWrite(i2c_file, 0x6a, 0x00, 1, 1, temp_data) != 0) {
			return -1;
		}
		temp_data[0] = svio2 * 531 / 1000 - 60;
		if (i2cWrite(i2c_file, 0x6a, 0xd8, 1, 1, temp_data) != 0) {
			return -1;
		}
	}

	return 0;
}


// Print strings, Read DNA must have been run first to populate the svio struct
int printVIOStrings (json &json_handler, int i2c_file)
{
	uint8_t temp_string[257];
	int i;
	int j = 0;

	for (i = 0; i < SVIO_NUM_PORTS; i++) {
		if (svio.ports[i].i2c_addr == 0x00) {
			// don't print anything for FPGA "ports"
			continue;
		}

		if (svio.ports[i].present == 0) {
			if (!json_handler.is_null()) {
				json_handler["port"][j] = nullptr;
			}

			// just increment the port counter for non-present ports
			j++;
			continue;
		}

		// retrieve manufacturer
		if (readMCU(i2c_file, svio.ports[i].i2c_addr,
		        0x8000 + svio.ports[i].mfr_offset, temp_string,
		        svio.ports[i].mfr_length) != 0) {
			return -1;
		}

		temp_string[svio.ports[i].mfr_length] = '\0';

		if (!json_handler.is_null()) {
			json_handler["port"][j]["manufacturer"] = std::string((char*) temp_string);
		} else {
			printf("Port 0x%X Manufacturer: %s\n", svio.ports[i].i2c_addr, temp_string);
		}

		// retrieve product name
		if (readMCU(i2c_file, svio.ports[i].i2c_addr,
		        0x8000 + svio.ports[i].product_name_offset, temp_string,
		        svio.ports[i].product_name_length) != 0) {
			return -1;
		}

		temp_string[svio.ports[i].product_name_length] = '\0';

		if (!json_handler.is_null()) {
			json_handler["port"][j]["product_name"] = std::string((char*) temp_string);
		} else {
			printf("Product Name: %s\n", temp_string);
		}

		// retrieve product model
		if (readMCU(i2c_file, svio.ports[i].i2c_addr,
		        0x8000 + svio.ports[i].product_model_offset, temp_string,
		        svio.ports[i].product_model_length) != 0) {
			return -1;
		}

		temp_string[svio.ports[i].product_model_length] = '\0';

		if (!json_handler.is_null()) {
			json_handler["port"][j]["product_model"] = std::string((char*) temp_string);
		} else {
			printf("Product Model: %s\n", temp_string);
		}

		// retrieve product version
		if (readMCU(i2c_file, svio.ports[i].i2c_addr,
		        0x8000 + svio.ports[i].product_version_offset, temp_string,
		        svio.ports[i].product_version_length) != 0) {
			return -1;
		}

		temp_string[svio.ports[i].product_version_length] = '\0';

		if (!json_handler.is_null()) {
			json_handler["port"][j]["product_version"] = std::string((char*) temp_string);
		} else {
			printf("Version: %s\n", temp_string);
		}

		// retrieve serial
		if (readMCU(i2c_file, svio.ports[i].i2c_addr,
		        0x8000 + svio.ports[i].serial_number_offset, temp_string,
		        svio.ports[i].serial_number_length) != 0) {
			return -1;
		}

		temp_string[svio.ports[i].serial_number_length] = '\0';

		if (!json_handler.is_null()) {
			json_handler["port"][j]["serial_number"] = std::string((char*) temp_string);
		} else {
			printf("Serial: %s\n", temp_string);
		}
		j++;
	}

	return 0;
}


// Help text
void printHelp (char *progname)
{
	printf("Usage: %s [option [argument]] <i2c device>\n", progname);
	printf("  <i2c device> is required for all commands. It must contain the\n");
	printf("               path to the Linux i2c device\n");
	printf("\n");
	printf("  Exactly one of the following options must be specified:\n");
	printf("    -r - run smartVIO, queries attached MCU's and sets voltages accordingly\n");
	printf("    -s - set VIO voltages to the values provided by -1 and -2 options\n");
	printf("    -j - print out a JSON object with DNA and SmartVIO information\n");
	printf("    -h - print this text\n");
	printf("    -w <filename> - write a binary DNA to a peripheral, takes the DNA filename\n");
	printf("                    as an argument\n");
	printf("    -d <filename> - dump the DNA from a peripheral to a binary file, takes the\n");
	printf("                    DNA filename as an argument\n");
	printf("\n");
	printf("  The following options may be used in conjunction with the above options:\n");
	printf("    -1 <vio1> - Sets the voltage for VIO1\n");
	printf("    -2 <vio2> - Sets the voltage for VIO2\n");
	printf("          <vio1> and <vio2> must be specified as numbers in 10's of mV\n");
	printf("    -p <number> - Specifies the peripheral number for the -w or -d options\n");
	printf("\n");
	printf("  Examples:\n");
	printf("    Run SmartVIO sequence:\n");
	printf("      %s -r /dev/i2c-1\n", progname);
	printf("    Dump DNA from the MCU on Port 1:\n");
	printf("      %s -d dna_file.bin -p 1 /dev/i2c-1\n", progname);
	printf("    Set VIO1 to 3.3V:\n");
	printf("      %s -s -1 330 /dev/i2c-1\n", progname);
}


int main (int argc, char *argv[])
{
	// Options flags
	int rflag = 0;
	int sflag = 0;
	int jflag = 0;
	int hflag = 0;
	int wflag = 0;
	int dflag = 0;
	uint32_t svio1 = 0;
	uint32_t svio2 = 0;
	char i2c_filename[200];
	char dna_filename[200];
	uint8_t dna_buf[1320];
	int i2c_file;
	int dna_file;
	int dna_length = 0;
	int periph_num = 0;
	int curr_opt;
	json json_handler;
	uint16_t peripheral_address[] = {0x30, 0x31, 0x32, 0x33};

	// Parse args
	while ((curr_opt = getopt(argc, argv, "rsj1:2:w:d:p:h")) != -1) {
		switch(curr_opt)
		{
			case 'r':
				rflag = 1;
				break;
			case 's':
				sflag = 1;
				break;
			case 'j':
				jflag = 1;
				break;
			case '1':
				if (optarg){ 
					svio1 = atoi(optarg);
				} else {
					printf("No argument specified for -1\n");
					exit(EXIT_FAILURE);
				}
				break;
			case '2':
				if (optarg){ 
					svio2 = atoi(optarg);
				} else {
					printf("No argument specified for -2\n");
					exit(EXIT_FAILURE);
				}
				break;
			case 'w':
				wflag = 1;
				if (optarg){ 
					strcpy(dna_filename, optarg);
				} else {
					printf("No argument specified for -w\n");
					exit(EXIT_FAILURE);
				}
				break;
			case 'd':
				dflag = 1;
				if (optarg){ 
					strcpy(dna_filename, optarg);
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

	if ((rflag + sflag + jflag + hflag + wflag + dflag) > 1) {
		printf("Invalid set of options specified.\n");
		printHelp(argv[0]);
		return 0;
	}

	if ((wflag == 1) || (dflag == 1)) {
		if (i2cDetect(i2c_file, peripheral_address[periph_num]) != 0) {
			printf("Peripheral at %X not found\n", peripheral_address[periph_num]);
			exit(EXIT_FAILURE);
		}

		dna_file = open(dna_filename, O_RDWR | O_CREAT, 0666);
		if (dna_file < 0) {
			printf("Error opening DNA file\n");
			exit(EXIT_FAILURE);
		}
	}

	if (rflag == 1) { // Run the main SmartVIO procedure
		if (readDNA(i2c_file, &svio1, &svio2) != 0) {
			printf("Error obtaining a SmartVIO solution\n");
			exit(EXIT_FAILURE);
		}

		if (applyVIO(i2c_file, svio1, svio2) != 0) {
			printf("Error applying SmartVIO settings to power supplies\n");
			exit(EXIT_FAILURE);
		}

		if (printVIOStrings(json_handler, i2c_file) != 0) {
			printf("Error retrieving DNA strings\n");
			exit(EXIT_FAILURE);
		}
	} else if (sflag == 1) { // Apply a user specified VIO
		if (applyVIO(i2c_file, svio1, svio2) != 0) {
			printf("Error applying SmartVIO settings to power supplies\n");
			exit(EXIT_FAILURE);
		}
	} else if (jflag == 1) {
		readDNA(i2c_file, &svio1, &svio2);

		// Bounds check on the svio ranges
		if ((svio1 < 120) || (svio1 > 330) || (svio2 < 120) || (svio2 > 330)) {
			json_handler["vio"][0] = 0;
			json_handler["vio"][1] = 0;
		} else {
			json_handler["vio"][0] = svio1;
			json_handler["vio"][1] = svio2;
		}

		printVIOStrings(json_handler, i2c_file);

		printf(json_handler.dump().c_str());
		printf("\n");
	} else if (wflag == 1) { // Write DNA from a file to a peripheral
		if (read(dna_file, &dna_length, 2) != 2) {
			printf("Error reading from DNA file\n");
			exit(EXIT_FAILURE);
		}

		if (dna_length > 1318) {
			printf("Invalid DNA Length\n");
			exit(EXIT_FAILURE);
		}

		lseek(dna_file, 0, SEEK_SET);
		if (read(dna_file, dna_buf, dna_length) != dna_length) {
			printf("Error reading from DNA file\n");
			exit(EXIT_FAILURE);
		}

		if (writeMCU(i2c_file, peripheral_address[periph_num], 0x8000,
		             dna_buf, dna_length) != 0) {
			printf("Error writing to the MCU\n");
			exit(EXIT_FAILURE);
		}
	} else if (dflag == 1) { // Dump DNA from a peripheral to a file
		dna_length = dumpDNA(i2c_file, peripheral_address[periph_num], dna_buf);

		if (dna_length < 0) {
			printf("Error reading DNA from device\n");
			exit(EXIT_FAILURE);
		}
		
		if (write(dna_file, dna_buf, dna_length) != dna_length) {
			printf("Error writing to DNA file\n");
			exit(EXIT_FAILURE);
		}
	} else {
		printHelp(argv[0]);
		return 0;
	}

	return 0;
}

