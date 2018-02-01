# Brain Tools

## Overview

The Brain Tools package provides a suite of applications to assist users
of the [SYZYGY Brain-1](http://syzygyfpga.io/hub/) development platform
as well as provide a reference design for creators of other SYZYGY
compatible platforms.

These tools include:

- syzygy.(h,c) - A library that can be used to parse SYZYGY DNA data and
determine an appropriate VIO voltage for a set of peripherals.

- smartvio-brain - An application implementing SmartVIO functionality for
the Brain-1.

- dna-writer - An application for generating SYZYGY DNA blobs that can be
written to peripheral MCUs.

- szg\_i2cread/i2cwrite - A pair of helper applications to allow for basic
i2c communication with devices that use 16-bit addresses.

### SmartVIO Library

The SmartVIO library consists of a C header file (`syzygy.h`) along with
C source code (`syzygy.c`). The library includes a set of useful definitions
and structures for working with DNA data. It also includes functions for
parsing DNA data, solving for a VIO voltage from a group of pods, and
calculating the CRC-16 used in the DNA.

A test application `smartvio-test` is provided as a test suite for the
SmartVIO solver algorithm. This test is based on the
[Catch](https://github.com/catchorg/Catch2) library.

### SmartVIO Brain Application

The `smartvio-brain` application is an implementation of the SmartVIO library
for the SYZYGY Brain-1 development board by Opal Kelly. This application
handles all communication with peripheral MCU's and the on-board power
supplies to set the appropriate VIO voltages. This application can also be
used to read/write binary DNA blob files from/to a peripheral MCU.

Usage information is available by running `smartvio -h`

### DNA Writer Application

The `dna-writer` application can be used to generate a binary DNA blob from
a JSON-format input. This application uses the
[JSON for Modern C++](https://github.com/nlohmann/json) library. The DNA
writer conforms to the SYZYGY DNA Specification v1.0. Please see the
[SYZYGY](http://syzygyfpga.io/) website for more information.

Please see the examples in the `json` folder for peripheral DNA examples.

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
build all provided applications. Note that the dna-writer and smartvio-test
applications require C++ 11 support. The JSON library used by dna-writer
requires GCC 4.9 or later.

Individual applications can be built by running `make <application>`.

This build has been tested on a machine running Ubuntu 16.04 LTS with
GCC 5.4.0.
