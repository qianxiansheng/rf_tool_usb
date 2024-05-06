#include "utils.h"
#include <sstream>
#include <vector>
#include <fstream>

#include "imgui.h"

// OS Specific sleep
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

void utils::BeginDisable(bool test)
{
	if (test)
		ImGui::BeginDisabled();
}
void utils::EndDisable(bool test)
{
	if (test)
		ImGui::EndDisabled();
}

void utils::Alert(bool show, const char* title, const char* text)
{
	if (show)
		ImGui::OpenPopup(title);

	// Always center this window when appearing
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	if (ImGui::BeginPopupModal(title, NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text(text);
		ImGui::Separator();

		if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
		ImGui::SetItemDefaultFocus();
		ImGui::EndPopup();
	}
}

bool utils::Confirm(bool show, const char* title, const char* text)
{
	if (show)
		ImGui::OpenPopup(title);

	// Always center this window when appearing
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	bool result = false;

	if (ImGui::BeginPopupModal(title, NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text(text);
		ImGui::Separator();

		if (ImGui::Button("OK", ImVec2(120, 0))) 
		{ 
			ImGui::CloseCurrentPopup(); 
			result = true;
		}
		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0))) 
		{ 
			ImGui::CloseCurrentPopup(); 
		}
		ImGui::EndPopup();
	}
	return result;
}

void utils::readFileData(std::filesystem::path path, void* out_data)
{
	std::ifstream inFile;
	inFile.open(path, std::ios::in | std::ios::binary);
	while (inFile.read((char*)out_data, std::filesystem::file_size(path)));
	inFile.close();
}

std::string utils::readFileText(std::filesystem::path path)
{
	std::ifstream inFile;
	std::stringstream ss;
	std::string content;
	inFile.open(path, std::ios::in);

	while (!inFile.eof())
	{
		inFile >> content;
		ss << content;
	}
	inFile.close();

	return ss.str();
}


uint8_t utils::htoi_4(const char c)
{
	if ('0' <= c && c <= '9') return c - '0';
	if ('A' <= c && c <= 'F') return c - 'A' + 10;
	if ('a' <= c && c <= 'f') return c - 'a' + 10;
	return 0;
}
uint8_t utils::htoi_8(const char* c)
{
	return (htoi_4(c[0]) << 4) | htoi_4(c[1]);
}
uint16_t utils::htoi_16(const char* c)
{
	return (htoi_8(c) << 8) | htoi_8(c + 2);
}
uint32_t utils::htoi_32(const char* c)
{
	return (htoi_8(c) << 24) | (htoi_8(c + 2) << 16) | (htoi_8(c + 4) << 8) | htoi_8(c + 6);
}

void utils::ValidateU8Text(char* text, uint8_t& v)
{
	// Load address
	if (text[0] == '\0') {
		v = 0;
		strcpy_s(text, 3, "00");
	}
	else {
		if (strlen(text) == 1) {
			text[2] = '\0';
			text[1] = text[0];
			text[0] = '0';
		}
		v = utils::htoi_8(text);
	}
}
void utils::ValidateU16Text(char* text, uint16_t& v)
{
	// Load address
	if (text[0] == '\0') {
		v = 0;
		strcpy_s(text, 5, "0000");
	}
	else {
		if (strlen(text) == 1) {
			text[4] = '\0';
			text[3] = text[0];
			text[2] = '0';
			text[1] = '0';
			text[0] = '0';
		}
		if (strlen(text) == 2) {
			text[4] = '\0';
			text[3] = text[1];
			text[2] = text[0];
			text[1] = '0';
			text[0] = '0';
		}
		if (strlen(text) == 3) {
			text[4] = '\0';
			text[3] = text[2];
			text[2] = text[1];
			text[1] = text[0];
			text[0] = '0';
		}
		v = utils::htoi_16(text);
	}
}
void utils::ValidateIntText(char* text, uint32_t& v)
{
	// Load address
	if (text[0] == '\0') {
		v = 0;
		strcpy_s(text, 2, "0");
	}
	else {
		v = std::stoi(text);
	}
}
void utils::ValidateDataText(char* text, uint8_t* data, size_t& size)
{
	// Load address
	if (text[0] == '\0') {
		size = 0;
	}
	else {
		if (text[0] == '0' && text[1] == 'x') {
			size = (strlen(text) - 2) / 2;

			for (size_t i = 0; i < size; ++i) {
				data[i] = utils::htoi_8(text + 2 + (i * 2));
			}
		}
		else {
			size = strlen(text);
			strcpy_s((char*)data, size + 1, text);
		}
	}
}
void utils::ValidateHex16ByteText(char* text, uint8_t* data)
{
	uint8_t rl = 32 - (uint8_t)strlen(text);
	std::stringstream ss;
	for (uint8_t i = 0; i < rl; ++i)
		ss << '0';
	ss << text;
	strcpy_s(text, 33, ss.str().c_str());

	utils::String2Hex(text, data, 32);
}

std::string utils::dec2hex(uint32_t i)
{
	std::ostringstream ss;
	ss << "0x" << std::hex << i;
	return ss.str();
}

void utils::printf_hexdump(uint8_t* data, uint16_t size)
{
	std::cout << std::hex;
	for (uint16_t i = 0; i < size; ++i) {
		std::cout << (int)data[i];
	}
	std::cout << std::dec << std::endl;
}

unsigned char utils::Hex2String(const unsigned char* pSrc, unsigned char* dest, int nL)
{
	unsigned char* buf_hex = 0;
	int i;
	unsigned char hb;
	unsigned char lb;
	buf_hex = dest;
	memset((char*)buf_hex, 0, sizeof(buf_hex));
	hb = 0;
	lb = 0;
	for (i = 0; i < nL; i++)
	{
		hb = (pSrc[i] & 0xf0) >> 4;
		if (hb >= 0 && hb <= 9)
			hb += 0x30;
		else if (hb >= 10 && hb <= 15)
			hb = hb - 10 + 'A';
		else
			return 1;
		lb = pSrc[i] & 0x0f;
		if (lb >= 0 && lb <= 9)
			lb += 0x30;
		else if (lb >= 10 && lb <= 15)
			lb = lb - 10 + 'A';
		else
			return 1;
		buf_hex[i * 2] = hb;
		buf_hex[i * 2 + 1] = lb;
	}
	return 0;
}

void utils::String2Hex(const char* src, unsigned char* dest, int srcL)
{
	int halfSrcL = srcL / 2;
	for (int i = 0; i < halfSrcL; ++i)
		dest[i] = htoi_8(src + (i * 2));
}

uint16_t utils::checksum16(uint8_t* buffer, uint32_t size)
{
	uint16_t sum = 0;
	while (size-- > 0) {
		sum += *buffer++;
	}
	return sum;
}
//
//std::wstring utils::utf8_to_wstr(const std::string& src)
//{
//	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
//	return converter.from_bytes(src);
//}
//std::string utils::wstr_to_utf8(const std::wstring& src)
//{
//	std::wstring_convert<std::codecvt_utf8<wchar_t>> convert;
//	return convert.to_bytes(src);
//}
//std::string utils::utf8_to_gbk(const std::string& str)
//{
//	std::wstring_convert<std::codecvt_utf8<wchar_t> > conv;
//	std::wstring tmp_wstr = conv.from_bytes(str);
//
//	//GBK locale name in windows
//	const char* GBK_LOCALE_NAME = ".936";
//	std::wstring_convert<std::codecvt_byname<wchar_t, char, mbstate_t>> convert(new std::codecvt_byname<wchar_t, char, mbstate_t>(GBK_LOCALE_NAME));
//	return convert.to_bytes(tmp_wstr);
//}
//std::string utils::gbk_to_utf8(const std::string& str)
//{
//	//GBK locale name in windows
//	const char* GBK_LOCALE_NAME = ".936";
//	std::wstring_convert<std::codecvt_byname<wchar_t, char, mbstate_t>> convert(new std::codecvt_byname<wchar_t, char, mbstate_t>(GBK_LOCALE_NAME));
//	std::wstring tmp_wstr = convert.from_bytes(str);
//
//	std::wstring_convert<std::codecvt_utf8<wchar_t>> cv2;
//	return cv2.to_bytes(tmp_wstr);
//}
//std::wstring utils::gbk_to_wstr(const std::string& str)
//{
//	//GBK locale name in windows
//	const char* GBK_LOCALE_NAME = ".936";
//	std::wstring_convert<std::codecvt_byname<wchar_t, char, mbstate_t>> convert(new std::codecvt_byname<wchar_t, char, mbstate_t>(GBK_LOCALE_NAME));
//	return convert.from_bytes(str);
//}

void utils::my_sleep(unsigned long milliseconds) {
#ifdef _WIN32
	Sleep(milliseconds); // 100 ms
#else
	usleep(milliseconds * 1000); // 100 ms
#endif
}

long long utils::get_current_system_time_us()
{
	std::chrono::system_clock::time_point time_point_now = std::chrono::system_clock::now();
	std::chrono::system_clock::duration duration_since_epoch = time_point_now.time_since_epoch();
	return std::chrono::duration_cast<std::chrono::microseconds>(duration_since_epoch).count();
}
long long utils::get_current_system_time_ms()
{
	std::chrono::system_clock::time_point time_point_now = std::chrono::system_clock::now();
	std::chrono::system_clock::duration duration_since_epoch = time_point_now.time_since_epoch();
	return std::chrono::duration_cast<std::chrono::milliseconds>(duration_since_epoch).count();
}
long long utils::get_current_system_time_s()
{
	std::chrono::system_clock::time_point time_point_now = std::chrono::system_clock::now();
	std::chrono::system_clock::duration duration_since_epoch = time_point_now.time_since_epoch();
	return std::chrono::duration_cast<std::chrono::seconds>(duration_since_epoch).count();
}

void utils::logToFile(const std::string& message, const std::string& filename) 
{
	time_t rawtime;
	struct tm timeinfo;
	errno_t result;

	result = localtime_s(&timeinfo, &rawtime);

	std::ofstream logfile(filename, std::ios::app);  // ×·¼ÓÄ£Ê½
	if (logfile.is_open()) {
		char buffer[80];
		strftime(buffer, sizeof(buffer), "[%Y-%m-%d %H:%M:%S] ", &timeinfo);

		logfile << buffer << message << std::endl;

		logfile.close();
	}
	else {
		std::cerr << "Error opening the log file." << std::endl;
	}
}

