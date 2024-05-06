#pragma once

#include "pch.h"

#include <iostream>
#include <unordered_map>

#include "libusb-1.0/libusb.h"

#define INGCHIPS_KEYBOARD_VID 0xFFF1
#define INGCHIPS_KEYBOARD_PID 0xFA2F

namespace usb
{
	typedef std::unordered_map<std::string, libusb_device*> DevMap;

	int init();
	void exit();

	int create_libusb_context(libusb_context** ctx);
	void destroy_libusb_context(libusb_context* ctx);

	bool is_ingchips_keyboard_device(libusb_device* device);

	int transfer_data(libusb_device* dev, uint8_t _interface, uint8_t ep, uint8_t* data, int data_size, int timeout = 1000);
	libusb_device* find_device(libusb_context* libusb_ctx, uint16_t vid, uint16_t pid);
	libusb_device* find_device(libusb_context* libusb_ctx, uint16_t vid, uint16_t pid, uint8_t address, uint8_t bus, uint8_t portNumber);
	libusb_device* find_device(libusb_context* libusb_ctx, const char* deviceName);
	DevMap enumrate_device_map(libusb_context* libusb_ctx);

	void release_device_map(DevMap& device_map);
}
