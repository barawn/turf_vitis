/******************************************************************************
*
* Copyright (C) 2009 - 2014 Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/

/*
 * helloworld.c: simple test application
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (Zynq) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
 */

#include <stdio.h>
#include <stdbool.h>

#include "platform.h"
#include "xil_printf.h"

#include "xparameters.h"
#include "xiicps.h"

#define IIC_SOM_DEVICE_ID		XPAR_XIICPS_0_DEVICE_ID
#define IIC_SOM_SCLK_RATE		100000

#define IIC_TURF_DEVICE_ID		XPAR_XIICPS_1_DEVICE_ID
#define IIC_TURF_SCLK_RATE		100000

#define SOM_PMIC_ADDR			0x58
#define TURFCLK0_ADDR			0x62
#define TURFCLK1_ADDR			0x6A

XIicPs som_i2c;
XIicPs turf_i2c;


int i2c_initialize() {
	int st;
	XIicPs_Config *cfg;

	// Set up the SOM-local I2C bus
	cfg = XIicPs_LookupConfig( IIC_SOM_DEVICE_ID );
	if (cfg == NULL) return XST_FAILURE;
	st = XIicPs_CfgInitialize(&som_i2c, cfg, cfg->BaseAddress);
	if (st != XST_SUCCESS) return XST_FAILURE;
	st = XIicPs_SelfTest(&som_i2c);
	if (st != XST_SUCCESS) return XST_FAILURE;
	XIicPs_SetSClk( &som_i2c, IIC_SOM_SCLK_RATE );

	// Here we should set up the TURF I2C as well.
	cfg = XIicPs_LookupConfig( IIC_TURF_DEVICE_ID );
	if (cfg == NULL) return XST_FAILURE;
	st = XIicPs_CfgInitialize(&turf_i2c, cfg, cfg->BaseAddress);
	if (st != XST_SUCCESS) return XST_FAILURE;
	st = XIicPs_SelfTest(&turf_i2c);
	if (st != XST_SUCCESS) return XST_FAILURE;
	XIicPs_SetSClk( &turf_i2c, IIC_TURF_SCLK_RATE);

	return XST_SUCCESS;
}

int som_pmic_write(u16 addr, u8 val) {
	// Buffer needs to be 2 bytes.
	u8 buf[2];
	// Page address
	u8 page;
	// Status
	int st;

	page = (addr & 0x180) >> 7;
	while (XIicPs_BusIsBusy(&som_i2c));
	// set the page
	buf[0] = 0;
	buf[1] = page;
	st = XIicPs_MasterSendPolled(&som_i2c, buf, 2, SOM_PMIC_ADDR);
	if (st != XST_SUCCESS) return XST_FAILURE;
	// now write. PMIC addresses need to be sent *in full*.
	buf[0] = addr & 0xFF;
	buf[1] = val;
	while (XIicPs_BusIsBusy(&som_i2c));
	st = XIicPs_MasterSendPolled(&som_i2c, buf, 2, SOM_PMIC_ADDR);
	return st;
}

int som_pmic_read(u16 addr, u8 *val) {
	// Buffer needs to be 2 bytes
	u8 buf[2];
	// Page address
	u8 page;
	// Status
	int st;

	page = (addr & 0x180) >> 7;
	while (XIicPs_BusIsBusy(&som_i2c));
	buf[0] = 0;
	buf[1] = page;
	st = XIicPs_MasterSendPolled(&som_i2c, buf, 2, SOM_PMIC_ADDR);
	if (st != XST_SUCCESS) return XST_FAILURE;
	// now set address. Must be *in full* even though the top bit
	// is duplicated as page.
	buf[0] = addr & 0xFF;
	while (XIicPs_BusIsBusy(&som_i2c));
	st = XIicPs_MasterSendPolled(&som_i2c, buf, 1, SOM_PMIC_ADDR);
	if (st != XST_SUCCESS) return XST_FAILURE;
	// and read
	while (XIicPs_BusIsBusy(&som_i2c));
	st = XIicPs_MasterRecvPolled(&som_i2c, val, 1, SOM_PMIC_ADDR);
	return st;
}

int turf_clock_read(u8 chip_addr, u8 addr, u16 *val) {
	u8 buf[2];
	int st;

	while (XIicPs_BusIsBusy(&turf_i2c));
	st = XIicPs_MasterSendPolled(&turf_i2c, &addr, 1, chip_addr);
	if (st != XST_SUCCESS) return XST_FAILURE;
	while (XIicPs_BusIsBusy(&turf_i2c));
	st = XIicPs_MasterRecvPolled(&turf_i2c, buf, 2, chip_addr);
	if (st != XST_SUCCESS) return XST_FAILURE;
	*val = (buf[0] << 8) | buf[1];
	return XST_SUCCESS;
}

int turf_clock_write(u8 chip_addr, u8 addr, u16 val) {
	u8 buf[3];
	int st;

	buf[0] = addr;
	buf[1] = (val >> 8) & 0xFF;
	buf[2] = (val >> 0) & 0xFF;
	while (XIicPs_BusIsBusy(&turf_i2c));
	st = XIicPs_MasterSendPolled(&turf_i2c, buf, 3, chip_addr);
	return st;
}

int main()
{
	bool have_clk0;
	bool have_clk1;
	u8 clk_addr;
	u8 val;
	u16 v2;
	int st;
    init_platform();

    print("TURF Startup Process\n\r");

    i2c_initialize();
    // Check the Dialog PMIC.
    st = som_pmic_read(0x181, &val);
    if (st != XST_SUCCESS) {
    	xil_printf("Failed reading DA9062: st %d\n\r", st, val);
    } else {
    	xil_printf("Dialog PMIC found: ID %x, checking lock\n\r", val);
    	som_pmic_read(0x012, &val);
    	xil_printf("CONTROL_E = %x (lock=%d)\n\r", val, (val>>7) & 0x1);
    	if (!(val & 0x80)) {
    		xil_printf("LOCK not set, raising voltages\n\r");
    		// Raise VLDO4_A to 2.5V. This may need to go to 3.3V, we'll see.
    		som_pmic_write(0xAC, 34);
    		som_pmic_read(0xAC, &val);
    		xil_printf("VLDO4_A is now: %d mV\n\r", val*50 + 800);
    		// Raise VLDO1_A to 1.8V.
    		som_pmic_write(0xA9, 20);
    		som_pmic_read(0xA9, &val);
    		xil_printf("VLDO1_A is now: %d mV\n\r", val*50 + 800);
    		// Set VLOCK
    		som_pmic_read(0x12, &val);
    		val |= 0x80;
    		som_pmic_write(0x12, val);
    		som_pmic_read(0x12, &val);
    		xil_printf("CONTROL_E = %x (lock=%d)\n\r", val, (val>>7) & 0x1);
    	} else {
    		xil_printf("LOCK is already set, voltages should be OK\n\r");
    	}
    }
    // Next we have to actually turn on the clocks, because OF COURSE WE DO
    // Check if they ack.
    st = turf_clock_read(TURFCLK0_ADDR, 0x02, &v2);
    if (st != XST_SUCCESS) {
    	xil_printf("Failed locating SiT5157 #0: st %d\n\r", st);
    	have_clk0 = false;
    } else {
    	xil_printf("Located SiT5157 #0: read %x\n\r", v2);
    	have_clk0 = true;
    }
    st = turf_clock_read(TURFCLK1_ADDR, 0x02, &v2);
    if (st != XST_SUCCESS) {
    	xil_printf("Failed locating SiT5157 #1: st %d\n\r", st);
    	have_clk1 = false;
    } else {
    	xil_printf("Located SiT5157 #1: read %x\n\r", v2);
    	have_clk1 = true;
    }
    // i dunno, do something smarter later
    if (have_clk0) clk_addr = TURFCLK0_ADDR;
    else if (have_clk1) clk_addr = TURFCLK1_ADDR;
    else clk_addr = 0;

    xil_printf("Turning on clock at %x\n\r", TURFCLK0_ADDR);
    turf_clock_write(TURFCLK0_ADDR, 0x1, (1<<10));
    st = turf_clock_read(TURFCLK0_ADDR, 0x1, &v2);
    xil_printf("Register 1 now reads: %x\n\r", v2);

    xil_printf("Turning on clock at %x\n\r", TURFCLK1_ADDR);
    turf_clock_write(TURFCLK1_ADDR, 0x1, (1<<10));
    st = turf_clock_read(TURFCLK1_ADDR, 0x1, &v2);
    xil_printf("Register 1 now reads: %x\n\r", v2);


    cleanup_platform();
    return 0;
}
