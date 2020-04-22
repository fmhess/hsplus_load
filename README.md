# hsplus_load
hsplus_load is a utility for initializing NI GPIB-USB-HS+ adapters under Linux.

## Disclaimer
This program is EXPERIMENTAL and is provided WITHOUT WARRANTY.  
USE AT YOUR OWN RISK.  It was
written solely based on information gained by sniffing the USB 
bus to observe how the National
Instruments driver initializes the adapter under Windows.
It is entirely possible that initializing your adapter with
bad firmware could permanently damage or disable your adapter.

## Motivation
If your GPIB-USB-HS+ powers on with a USB product id of 0x761e then
it needs a one-time firmware initialization in order to become usable.  After
initializtion, the GPIB-USB-HS+ will power up with USB product id
0x7618 and may be used normally.

If you do not wish to use this program, you may alternatively initialize
your GPIB-USB-HS+ by plugging it into a Windows computer which has the
National Instruments GPIB driver software installed.

## Requirements
* libusb-1.0
* Stage 1 and stage 2 firmware images, available from the
[Linux-GPIB firmware repository](https://github.com/fmhess/linux_gpib_firmware).


## Build
	make

## Usage
As root:

	hsplus_load STAGE1_IMAGE STAGE2_IMAGE

## Author
Frank Mori Hess fmh6jj@gmail.com
