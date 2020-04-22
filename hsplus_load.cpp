#include <cstdio>
#include <functional>
#include <fstream>
#include <iostream>
#include <libusb.h>
#include <memory>
#include <vector>

static const uint16_t ni_vendor_id = 0x3923;
static const uint16_t uninitialized_ni_gpib_usb_hs_plus_product_id = 0x761e;
static const uint16_t ni_gpib_usb_hs_plus_product_id = 0x7618;

std::shared_ptr<libusb_device_handle> find_ni_gpib_usb_hs_plus()
{
	libusb_device **device_list;
	ssize_t num_devices = libusb_get_device_list(0, &device_list);
	if (num_devices < 0) 
	{
		throw std::runtime_error("No USB devices found.");
	}
	std::shared_ptr<libusb_device *> device_list_cleanup(device_list, std::bind(&libusb_free_device_list, std::placeholders::_1, 1));
	
	for (int i = 0; i < num_devices; ++i)
	{

		libusb_device_descriptor usb_desc;
		int retval = libusb_get_device_descriptor (device_list[i], &usb_desc);
		if (retval < 0) 
		{
			continue;
		}
		
		if (usb_desc.idVendor == ni_vendor_id)
		{
			if (usb_desc.idProduct == uninitialized_ni_gpib_usb_hs_plus_product_id)
			{
				std::cerr << "Found uninitialized NI GPIB-USB_HS+." << std::endl;
				libusb_device_handle *usb_dev = 0;
				int retval = libusb_open (device_list[i], &usb_dev);
				if (retval < 0)
				{
					continue;
				}
				return std::shared_ptr<libusb_device_handle>(usb_dev, &libusb_close);
			}
		}
	}
	throw std::runtime_error("No uninitialized NI GPIB-USB-HS+ devices found.");
}

template<typename T, typename OutputIterator> 
void to_little_endian(T value, OutputIterator it)
{
	for(unsigned i = 0; i < sizeof(T); ++i)
	{
		*it = value & 0xff;
		value >>= 8;
	}
}

void load_first_stage(libusb_device_handle *dev, const char *stage1_image)
{
	int retval;
	std::vector<unsigned char> data;
	const unsigned int timeout = 3000;
	
	// check target idProduct
	data.resize(2);
	retval = libusb_control_transfer (dev, 
		LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE, 
		0x90, 
		0x0,
		0x0,
		&data.at(0),
		data.size(), 
		timeout);
	if (retval < static_cast<int>(data.size()))
	{
		throw std::runtime_error("Failed to verify target idProduct.");
	}
	uint16_t response_id = data.at(0) | (data.at(1) << 8);
	if (response_id != ni_gpib_usb_hs_plus_product_id)
	{
		std::fprintf(stderr, "Received unexpected target idProduct of 0x%x\n", response_id); 
		throw std::runtime_error("Adaptor did not response with expected target idProduct.");
	}
	
	std::ifstream stage1_ifstream;
	stage1_ifstream.open(stage1_image, std::ifstream::binary | std::ifstream::in);
	if (stage1_ifstream.good() == false) throw std::runtime_error("Failed to open stage 1 image file.");
	stage1_ifstream.seekg (0, stage1_ifstream.end);
	uint32_t stage1_image_length = stage1_ifstream.tellg();
	stage1_ifstream.seekg (0, stage1_ifstream.beg);
	if (stage1_ifstream.good() == false) throw std::runtime_error("Failed to get file size of stage 1 image.");
	std::cerr << "Stage 1 image size is " << stage1_image_length << std::endl;
	
	// send length of stage 1 image
	data.clear();
	to_little_endian(stage1_image_length, std::back_inserter(data));
	retval = libusb_control_transfer (dev, 
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE, 
		0x91, 
		0x2,
		0x0,
		&data.at(0),
		data.size(), 
		timeout);
	if (retval < static_cast<int>(data.size()))
	{
		throw std::runtime_error("Failed to send first stage data length.");
	}
	
	// send stage 1 image data
	static const unsigned max_chunk_length = 0x1000;
	unsigned remaining = stage1_image_length;
	while (remaining > 0)
	{
		data.clear();
		unsigned i;
		for(i = 0; i < max_chunk_length && i < remaining; ++i)
		{
			char byte = stage1_ifstream.get();
			if (stage1_ifstream.good() == false) break;
			data.push_back(byte);
		}
		
		if (data.empty())
		{
			throw std::runtime_error("Error reading stage 1 image file.");
		}
		
		retval = libusb_control_transfer (dev, 
			LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE, 
			0x92, 
			0x0,
			0x0,
			&data.at(0),
			data.size(), 
			timeout);
		if (retval < static_cast<int>(data.size()))
		{
			throw std::runtime_error("Failed to send first stage data.");
		}
		remaining -= data.size();
	}

	// I think this is checking for errors after writing first stage data
	data.resize(4);
	retval = libusb_control_transfer (dev, 
		LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE, 
		0x93, 
		0x0,
		0x0,
		&data.at(0),
		data.size(), 
		timeout);
	if (retval < static_cast<int>(data.size()))
	{
		throw std::runtime_error("Failed to verify target idProduct.");
	}
	// we expect to get all zeros back
	for(unsigned i = 0; i < data.size(); ++i)
	{
		if (data.at(i) != 0)
		{
			throw std::runtime_error("Received unexpected response after writing first stage data.");
		}
	}
}

void load_second_stage(libusb_device_handle *dev, const char *stage2_image)
{
	//FIXME: finish implementing
}

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
	std::shared_ptr<int> libusb_exiter(0, &libusb_exit);
	
	libusb_set_option (0, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_INFO);

	try
	{
		auto device = find_ni_gpib_usb_hs_plus();
		load_first_stage(device.get(), stage1_image);
		load_second_stage(device.get(), stage2_image);
	}
	catch (std::exception &err)
	{
		std::cerr << "Caught exception: " << err.what() << std::endl;
		return -1;
	}
	return 0;
}
