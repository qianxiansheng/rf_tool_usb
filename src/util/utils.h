#pragma once
#include <iostream>
#include <string>
#include <codecvt>
#include <filesystem>
#include <mutex>
#include <spdlog/spdlog.h>

namespace utils {

	void BeginDisable(bool test);
	void EndDisable(bool test);
	void Alert(bool show, const char* title, const char* text);
	bool Confirm(bool show, const char* title, const char* text);

	void readFileData(std::filesystem::path path, void* out_data);
	std::string readFileText(std::filesystem::path path);

	uint8_t htoi_4(const char c);
	uint8_t htoi_8(const char* c);
	uint16_t htoi_16(const char* c);
	uint32_t htoi_32(const char* c);
	void ValidateU8Text(char* text, uint8_t& v);
	void ValidateU16Text(char* text, uint16_t& v);
	void ValidateIntText(char* text, uint32_t& v);
	void ValidateDataText(char* text, uint8_t* data, size_t& size);
	void ValidateHex16ByteText(char* text, uint8_t* data);

	std::string dec2hex(uint32_t i);
	void printf_hexdump(uint8_t* data, uint16_t size);
	unsigned char Hex2String(const unsigned char* pSrc, unsigned char* dest, int nL);
	void String2Hex(const char* src, unsigned char* dest, int srcL);

	uint16_t checksum16(uint8_t* buffer, uint32_t size);

	//std::wstring utf8_to_wstr(const std::string& src);
	//std::string wstr_to_utf8(const std::wstring& src);
	//std::string utf8_to_gbk(const std::string& str);
	//std::string gbk_to_utf8(const std::string& str);
	//std::wstring gbk_to_wstr(const std::string& str);

	void my_sleep(unsigned long milliseconds);

	long long get_current_system_time_us();
	long long get_current_system_time_ms();
	long long get_current_system_time_s();

	void logToFile(const std::string& message, const std::string& filename = "log.txt");
}
