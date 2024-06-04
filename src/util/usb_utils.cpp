#include "usb_utils.h"
#include "util/utils.h"

#include "app/Application.h"

#define logger ::RFToolApplication::getInstance()->logger

int usb::init()
{
	return libusb_init(NULL);
}
void usb::exit()
{
	libusb_exit(NULL);
}

int usb::create_libusb_context(libusb_context** ctx)
{
	return libusb_init(ctx);
}
void usb::destroy_libusb_context(libusb_context* ctx)
{
	libusb_exit(ctx);
}


bool usb::is_ingchips_keyboard_device(libusb_device* device)
{
	struct libusb_device_descriptor desc;

	if (libusb_get_device_descriptor(device, &desc)) {
		printf("failed to get device descriptor");
		return false;
	}

	if (INGCHIPS_KEYBOARD_VID != desc.idVendor ||
		INGCHIPS_KEYBOARD_PID != desc.idProduct)
		return false;

	return true;
}


int usb::transfer_data(libusb_device* dev, uint8_t _interface, uint8_t ep, uint8_t* data, int data_size, int timeout)
{
	logger->trace("usb::transfer_data({0:x}, {1}, {2}, {3:x}, {4}, {5})", (uint64_t)dev, (int)_interface, (int)ep, (uint64_t)data, data_size, timeout);
	int actual_length = 0;
	libusb_device_handle* handler = NULL;

	logger->trace("libusb_open({0:x}, {1:x})", (uint64_t)dev, (uint64_t)&handler);
	int ret = libusb_open(dev, &handler);
	if (ret == LIBUSB_SUCCESS)
	{
		logger->trace("libusb_claim_interface({0:x}, {1})", (uint64_t)handler, _interface);
		ret = libusb_claim_interface(handler, _interface);
		if (ret != LIBUSB_SUCCESS) {
			logger->trace("libusb_close({0:x})", (uint64_t)handler);
			libusb_close(handler);
			throw std::exception(libusb_strerror(ret));
		}

		logger->trace("libusb_bulk_transfer({0:x}, {1:x}, {2:x}, {3}, {4:x}, {5})", (uint64_t)handler, ep, (uint64_t)data, data_size, (uint64_t)&actual_length, timeout);
		ret = libusb_bulk_transfer(handler, ep, data, data_size, &actual_length, timeout);
		if (ret != LIBUSB_SUCCESS) {
			logger->trace("libusb_close({0:x})", (uint64_t)handler);
			libusb_close(handler);
			throw std::exception(libusb_strerror(ret));
		}
		logger->trace("libusb_close({0:x})", (uint64_t)handler);
		libusb_close(handler);
	}
	else {
		throw std::exception(libusb_strerror(ret));
	}
	return actual_length;
}


libusb_device* usb::find_device(libusb_context* libusb_ctx, uint16_t vid, uint16_t pid)
{
	logger->trace("usb::find_device({0:x}, {1:x}, {2:x})", (uint64_t)libusb_ctx, vid, pid);
	size_t cnt;
	libusb_device** libusb_device_list = NULL;

	// enumerate all device
	cnt = libusb_get_device_list(libusb_ctx, &libusb_device_list);

	for (size_t i = 0; i < cnt; ++i)
	{
		struct libusb_device_descriptor desc;
		if (LIBUSB_SUCCESS != libusb_get_device_descriptor(libusb_device_list[i], &desc))
			continue;

		logger->debug("enum device: VID:{0:x}, PID:{1:x}", desc.idVendor, desc.idProduct);
		if (vid != desc.idVendor || pid != desc.idProduct)
		{
			continue;
		}
		else
		{
			libusb_device* devRef = libusb_ref_device(libusb_device_list[i]);
			libusb_free_device_list(libusb_device_list, 1);

			logger->info("Successfully found the device: VID:{0:x}, PID:{1:x}", vid, pid);
			return devRef;
		}
	}

	// free
	libusb_free_device_list(libusb_device_list, 1);
	logger->info("No suitable device found", vid, pid);
	return NULL;
}

libusb_device* usb::find_device(libusb_context* libusb_ctx, uint16_t vid, uint16_t pid, uint8_t address, uint8_t bus, uint8_t portNumber)
{
	size_t cnt;
	libusb_device** libusb_device_list = NULL;

	// enumerate all device
	cnt = libusb_get_device_list(libusb_ctx, &libusb_device_list);

	for (size_t i = 0; i < cnt; ++i)
	{
		struct libusb_device_descriptor desc;
		if (LIBUSB_SUCCESS != libusb_get_device_descriptor(libusb_device_list[i], &desc))
			continue;

		if (vid != desc.idVendor || pid != desc.idProduct ||
			address != libusb_get_device_address(libusb_device_list[i]) ||
			bus != libusb_get_bus_number(libusb_device_list[i]) ||
			portNumber != libusb_get_port_number(libusb_device_list[i]))
		{
			continue;
		}
		else
		{
			libusb_ref_device(libusb_device_list[i]);
			libusb_free_device_list(libusb_device_list, 1);
			return libusb_device_list[i];
		}
	}

	// free
	libusb_free_device_list(libusb_device_list, 1);
	return NULL;
}

libusb_device* usb::find_device(libusb_context* libusb_ctx, const char* deviceName)
{
	return find_device(libusb_ctx,
		utils::htoi_16(deviceName + 8),
		utils::htoi_16(deviceName + 17),
		utils::htoi_8(deviceName + 22),
		utils::htoi_8(deviceName + 25),
		utils::htoi_8(deviceName + 28));
}

usb::DevMap usb::enumrate_device_map(libusb_context* libusb_ctx)
{
	DevMap map;
	size_t cnt;
	char buf[32] = { 0 };

	libusb_device** libusb_device_list = NULL;


	// enumerate all device
	cnt = libusb_get_device_list(libusb_ctx, &libusb_device_list);

	for (size_t i = 0; i < cnt; ++i)
	{
		if (is_ingchips_keyboard_device(libusb_device_list[i]))
		{
			struct libusb_device_descriptor desc;
			libusb_get_device_descriptor(libusb_device_list[i], &desc);

			uint8_t bus = libusb_get_bus_number(libusb_device_list[i]);
			uint8_t address = libusb_get_device_address(libusb_device_list[i]);
			uint8_t portNumber = libusb_get_port_number(libusb_device_list[i]);

			sprintf_s(buf, 32, "USB#VID_%04X#PID_%04X#%02X#%02X#%02X", desc.idVendor, desc.idProduct, address, bus, portNumber);

			map[buf] = libusb_device_list[i];
			libusb_ref_device(libusb_device_list[i]);
		}
	}

	// free
	libusb_free_device_list(libusb_device_list, 1);

	return map;
}

void usb::release_device_map(DevMap& device_map)
{
	for (const auto& dev : device_map)
		libusb_unref_device(dev.second);
}