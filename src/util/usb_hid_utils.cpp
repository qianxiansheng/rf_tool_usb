#include "usb_hid_utils.h"

#include <assert.h>

#include "util/utils.h"

#include "app/Application.h"

#include "spdlog/spdlog.h"
#include "spdlog/fmt/bin_to_hex.h"

#define logger ::RFToolApplication::getInstance()->logger

bool HIDInit()
{
	return hid_init();
}

void HIDExit()
{
	hid_exit();
}

bool ValidateVPID(char* path, uint16_t vid, uint16_t pid)
{
	std::string p(path);
	size_t iv = p.find("VID_");
	size_t ip = p.find("PID_");

	if (iv == std::numeric_limits<size_t>::max() ||
		ip == std::numeric_limits<size_t>::max())
		return false;

	uint16_t tvid = utils::htoi_16(path + iv + 4);
	uint16_t tpid = utils::htoi_16(path + ip + 4);

	if (tvid == vid && tpid == pid)
		return true;
	else
		return false;
}

bool ValidateUsage(uint8_t* desc, uint32_t size, HIDDevice* hid)
{
	uint16_t i = 0;
	while (i < size)
	{
		if ((desc[i] & 0x0F) == 0x06)
		{
			if (desc[i] == 0x06)
			{
				hid->usage = desc[i + 1] | (desc[i + 2] << 8);
				return true;
			}
			i += 3;
		}
		else
		{
			i += 2;
		}
	}
	return false;
}

bool ValidateReportID(uint8_t* desc, uint32_t size, HIDDevice* hid)
{
	uint16_t i = 0;
	while (i < size)
	{
		if ((desc[i] & 0x0F) == 0x06)
		{
			i += 3;
		}
		else
		{
			if (desc[i] == 0x85)
			{
				hid->reportId = desc[i + 1];
				return true;
			}
			i += 2;
		}
	}
	return false;
}


static uint8_t hid_report_buf[USB_HID_REPORT_SIZE] = { USB_HID_REPORT_ID };

int hid_set_report(hid_device* dev, uint8_t* data, uint32_t size)
{
	assert(size <= USB_HID_REPORT_CONTENT_SIZE);
	if (hid_report_buf + 1 != data)
		memcpy(hid_report_buf + 1, data, size);

	logger->debug("hid_set_report {:n}", spdlog::to_hex(hid_report_buf, hid_report_buf + USB_HID_REPORT_SIZE));
	return hid_write(dev, hid_report_buf, USB_HID_REPORT_SIZE);
}
int hid_get_report(hid_device* dev, uint8_t* data, uint32_t size, int timeout)
{
	assert(size <= USB_HID_REPORT_CONTENT_SIZE);

	int r = hid_read_timeout(dev, hid_report_buf + 1, USB_HID_REPORT_SIZE, timeout);
	if (data != hid_report_buf + 1)
		memcpy(data, hid_report_buf + 1, size);
	logger->debug("ret:{}", r);
	logger->debug("hid_get_report {:n}", spdlog::to_hex(hid_report_buf, hid_report_buf + USB_HID_REPORT_SIZE));
	return r;
}

HIDFindResultEnum OpenHIDInterface(uint16_t vid, uint16_t pid, HIDDevice* hid)
{
	hid_device_info* hid_device_list = hid_enumerate(0, 0);
	uint8_t descriptor[HID_API_MAX_REPORT_DESCRIPTOR_SIZE];
	int res = 0;
	for (hid_device_info* p = hid_device_list; p != NULL; p = p->next)
	{
		logger->info("DEV: VID:{:x} PID:{:x}==", vid, pid);
		if (ValidateVPID(p->path, vid, pid))
		{
			hid_device* dev = hid_open_path(p->path);

			res = hid_get_report_descriptor(dev, descriptor, sizeof(descriptor));
			if (res < 0)
				continue;
			logger->info("Report Descriptor: {:n}", spdlog::to_hex(descriptor, descriptor + res));
			if (!ValidateUsage(descriptor, res, hid))
				continue;
			logger->info("usage: {:x}", hid->usage);

			if (hid->usage != 0xFF60)
				continue;

			logger->info("OK!");
			hid->phandle = dev;
			return HID_FIND_SUCCESS;
		}
	}

	hid_free_enumeration(hid_device_list);
	return HID_OPEN_FAILED;
}

void CloseHIDInterface(HIDDevice hid)
{
	hid_close(hid.phandle);
}


