#include <iostream>
#include <libusb.h>


int main (int argc, const char * argv[])
{
	if (argc != 3)
	{
		std::cerr << "Usage: " << argv[0] << " STAGE1_IMAGE STAGE2_IMAGE" << std::endl;
		return -1;
	}
	
	const char * const stage1_image = argv[1];
	const char * const stage2_image = argv[2];

	int retval = libusb_init(0);
	if (retval < 0) {
		std::cerr << "libusb_init() failed: " << libusb_error_name(retval) << std::endl;
		return -1;
	}
	
	libusb_set_option (0, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_INFO);

	libusb_device **device_list;
	ssize_t num_devices = libusb_get_device_list(0, &device_list);
	if (num_devices < 0) 
	{
		return -1;
	}
	
	for (int i = 0; i < num_devices; ++i)
	{
		libusb_device_handle *usb_dev;
		
		int retval = libusb_open (device_list[i], &usb_dev);
		if (retval < 0) return -1;

		libusb_device_descriptor usb_desc;
		retval = libusb_get_device_descriptor (device_list[i], &usb_desc);
		if (retval < 0) return -1;
		
		std::cerr << "idVendor: " << std::hex << usb_desc.idVendor << " idProduct: " << usb_desc.idProduct << std::endl; 
		
		libusb_close (usb_dev);
	}
	
	
	libusb_free_device_list(device_list, 1);

	libusb_exit(0);
	return 0;
}
