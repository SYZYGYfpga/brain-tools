// SmartVIO Tester
//
// This is a quick test application to ensure that the SmartVIO solver
// functions correctly for a series of different test ranges.
//
//------------------------------------------------------------------------
// Copyright (c) 2018 Opal Kelly Incorporated
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

#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#define CATCH_CONFIG_MAIN // Have Catch handle our main()
#include "catch.hpp"

extern "C" {
#include "syzygy.h"
}

// Unfortunately we can't used designated initializers in c++...
// Order of parameters: i2c_addr, present, group, attr, doublewide_mate,
// range_count, range array
szgSmartVIOPort ports[3] = {{0x00, 1, 0, 0, 0, 1,
			      { {120, 330}, {0,0}, {0,0}, {0,0} } },
			    {0x30, 1, 0, 0, 1, 1,
			      { {120, 330}, {0,0}, {0,0}, {0,0} } },
			    {0x31, 1, 1, 0, 0, 1,
			      { {120, 330}, {0,0}, {0,0}, {0,0} } }};

TEST_CASE( "Basic SmartVIO Test", "[smartvio]" ) {
	// Basic test
	// - FPGA  = 120 - 330
	// - Port1 = 180 - 330
	ports[0].ranges[0].min = 120;
	ports[0].ranges[0].max = 330;
	ports[0].range_count = 1;
	ports[1].ranges[0].min = 180;
	ports[1].ranges[0].max = 330;
	ports[1].range_count = 1;

	REQUIRE(szgSolveSmartVIOGroup(ports, 0x1) == 180);
}

TEST_CASE( "Advanced SmartVIO Test", "[smartvio]" ) {
	// More advanced test, multiple ranges, less overlap
	// - FPGA  = 090 - 100, 150 - 180, 250 - 330
	// - Port1 = 110 - 130, 180 - 200, 250 - 330
	ports[0].ranges[0].min = 90;
	ports[0].ranges[0].max = 100;
	ports[0].ranges[1].min = 150;
	ports[0].ranges[1].max = 180;
	ports[0].ranges[2].min = 250;
	ports[0].ranges[2].max = 330;
	ports[0].range_count = 3;
	ports[1].ranges[0].min = 110;
	ports[1].ranges[0].max = 130;
	ports[1].ranges[1].min = 180;
	ports[1].ranges[1].max = 200;
	ports[1].ranges[2].min = 250;
	ports[1].ranges[2].max = 330;
	ports[1].range_count = 3;

	REQUIRE(szgSolveSmartVIOGroup(ports, 0x1) == 180);
}

TEST_CASE( "Doublewide group test", "[smartvio]" ) {
	// Doublewide group test
	// - FPGA  = 120 - 330
	// - Port1 = 120 - 250
	// - Port2 = 180 - 330
	ports[0].ranges[0].min = 120;
	ports[0].ranges[0].max = 330;
	ports[0].range_count = 1;
	ports[1].ranges[0].min = 120;
	ports[1].ranges[0].max = 250;
	ports[1].range_count = 1;
	ports[2].ranges[0].min = 180;
	ports[2].ranges[0].max = 330;
	ports[2].range_count = 1;

	REQUIRE(szgSolveSmartVIOGroup(ports, 0x3) == 180);
}

TEST_CASE( "Failing SmartVIO Tests", "[smartvio]" ) {
	// Failing - First Range Lower
	// - FPGA  = 120 - 180
	// - Port1 = 250 - 330
	ports[0].ranges[0].min = 120;
	ports[0].ranges[0].max = 180;
	ports[1].ranges[0].min = 250;
	ports[1].ranges[0].max = 330;

	REQUIRE(szgSolveSmartVIOGroup(ports, 0x1) == -1);

	// Failing - First Range Higher
	// - FPGA  = 250 - 330
	// - Port1 = 120 - 180
	ports[0].ranges[0].min = 250;
	ports[0].ranges[0].max = 330;
	ports[1].ranges[0].min = 120;
	ports[1].ranges[0].max = 180;

	REQUIRE(szgSolveSmartVIOGroup(ports, 0x1) == -1);
}

