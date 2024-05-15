#ifndef __APPLICATION_H__
#define __APPLICATION_H__

#include "pch.h"

#include <iostream>
#include <vector>
#include <mutex>

#include "util/usb_utils.h"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"

#include "util/usb_hid_utils.h"

extern float dpiScale();

class AbstructApplication
{
public:
    virtual bool main_init(int argc, char* argv[]) = 0;
    virtual void main_shutdown(void) = 0;
    virtual int main_gui() = 0;
    virtual ~AbstructApplication() {}
	virtual float DPI(float value) { return dpiScale() * value; }
};

class ApplicationBase : public AbstructApplication
{
public:
    virtual bool main_init(int argc, char* argv[]) 
    {
        return true;
    }
    virtual void main_shutdown(void)
    {
    }
    virtual int main_gui()
    {
        return 0;
    }
};

struct combo_item_t
{
	char name[24];
	int value;
};

enum class ResultCode {
	SUCCESS = 0,
	FAILED_FIND_DEV = 1,
	INVALID_DEVICE = 2,
	UNEXPECTED_RSP = 3,
	EXCEPTION = 4,
	CMD_RET_CODE_UNEXPECTED_ERRPR = 5,
	GET_REPORT_TIMEOUT = 6,
};

enum TX_RX_MODE {
	MODE_TX = 0,
	MODE_RX = 1,
};
enum SINGLE_SWEEP_MODE {
	MODE_SINGLE = 0,
	MODE_SWEEP = 1,
};
enum CARRIER_MODE {
	CARRIER = 0x00,
};

#define RET_HEADER                0xBB
#define RET_PACKET_CODE           0xb0

class RFToolApplication : public ApplicationBase
{
private:
	RFToolApplication();
public:
	virtual bool main_init(int argc, char* argv[]);
	virtual void main_shutdown(void);
	virtual int main_gui();
public:

	void ShowRFWindow(bool* p_open);

public:

	void ParseArg(int argc, char* argv[]);

	void ShowRootWindowMenu();

	void ShowRootWindow(bool* p_open);

	ResultCode BtnScanClicked();

	std::vector<uint8_t> GenCommand(bool is_start);

	ResultCode BtnStartClicked();

	ResultCode BtnStopClicked(uint16_t* rx_packet_num);

	void LoadUserConfig();

	void SaveUserConfig();

public:
	static RFToolApplication* getInstance();
	static void deleteInstance();
	static RFToolApplication* s_Instance;
	static std::mutex s_Mutex;

public:
	bool opt_show_root_window;
	bool opt_show_rf_window;
	bool opt_showdemowindow;
	bool opt_fullscreen;

	char vid_buff[5];
	char pid_buff[5];
	char rid_buff[5];
	uint16_t vid;
	uint16_t pid;
	uint8_t rid;
	char result_find_dev_buff[8];
	int combo_tx_power_current_idx = 0;
	int combo_phy_current_idx = 0;

	int test_mode_setting = 0;
	int combo_single_mode_channel_current_idx = 0;
	int combo_sweep_mode_channel_min_current_idx = 0;
	int combo_sweep_mode_channel_max_current_idx = 39;
	int combo_sweep_time_current_idx = 0;

	int combo_payload_model_current_idx = 0;
	int combo_payload_len_current_idx = 0;
	int tx_rx_mode = 0;	//0:TX 1:RX
	char result_action_buff[64];

	combo_item_t combo_items_tx_power[15];
	combo_item_t combo_items_phy[4];
	combo_item_t combo_items_channel[40];
	combo_item_t combo_items_seconds[255];
	combo_item_t combo_items_payload_model[4];
	combo_item_t combo_items_payload_len[255];

	HIDDevice device;

	std::shared_ptr<spdlog::logger> logger;
};

#endif