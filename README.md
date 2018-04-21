# Brain Tools

## Overview

The Brain Tools package provides a suite of applications to assist users
of the [SYZYGY Brain-1](http://syzygyfpga.io/hub/) development platform
as well as provide a reference design for creators of other SYZYGY
compatible platforms.

These tools include:

- smartvio-brain - An application implementing SmartVIO functionality for
the Brain-1.

- szg\_i2cread/i2cwrite - A pair of helper applications to allow for basic
i2c communication with devices that use 16-bit addresses.

### SmartVIO Brain Application

The `smartvio-brain` application is an implementation of the SmartVIO library
for the SYZYGY Brain-1 development board by Opal Kelly. This application
handles all communication with peripheral MCU's and the on-board power
supplies to set the appropriate VIO voltages. This application can also be
used to read/write binary DNA blob files from/to a peripheral MCU.

Usage information is available by running `smartvio -h`

### i2cread/i2cwrite

These are simple helper applications that can be used to read/write single
byte values to i2c devices in Linux. These applications work with I2C devices
that have 16-bit sub-addresses such as SYZYGY peripheral MCU's and the
image sensor on the camera pod.

**NOTE** Interacting directly with I2C devices can brick a device, use these
commands with caution.

## Building

A Makefile is provided to assist with building this design. Simply run the
"make" command on a system with the necessary development tools installed to
build all provided applications. Note that the smartvio-brain application
requires C++ 11 support. The JSON library used by smartvio-brain requires
GCC 4.9 or later.

Individual applications can be built by running `make <application>`.

This build has been tested on a machine running Ubuntu 16.04 LTS with
GCC 5.4.0.
