#include "Application.h"

#include <iostream>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <vector>
#include "imgui.h"

#include "util/utils.h"
#include "util/usb_utils.h"
#include "util/INIReader.h"

#define SPDLOG_WCHAR_TO_UTF8_SUPPORT

#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/fmt/bin_to_hex.h"

#include "hidapi.h"
#include "util/usb_hid_utils.h"

static void ImGuiDCXAxisAlign(float v)
{
	ImGui::SetCursorPos(ImVec2(v, ImGui::GetCursorPos().y));
}

static void ImGuiDCYMargin(float v)
{
	ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPos().x, ImGui::GetCursorPos().y + v));
}

RFToolApplication* RFToolApplication::s_Instance = nullptr;
std::mutex RFToolApplication::s_Mutex;

RFToolApplication* RFToolApplication::getInstance()
{
	if (s_Instance == nullptr) {
		std::unique_lock<std::mutex> lock(s_Mutex);	// lock
		if (s_Instance == nullptr) {
			s_Instance = new RFToolApplication();
		}
		// unlock
	}
	return s_Instance;
}
void RFToolApplication::deleteInstance()
{
	std::unique_lock<std::mutex> lock(s_Mutex);	// lock
	if (s_Instance != nullptr) {
		delete s_Instance;
		s_Instance = nullptr;
	}
	// unlock
}

RFToolApplication::RFToolApplication()
	: opt_fullscreen(true)
	, opt_show_root_window(true)
	, opt_show_rf_window(true)
	, opt_showdemowindow(false)
{
	combo_items_tx_power[0]  = { "0db  ", 0 };
	combo_items_tx_power[1]  = { "2db  ", 1 };
	combo_items_tx_power[2]  = { "3db  ", 2 };
	combo_items_tx_power[3]  = { "4db  ", 3 };
	combo_items_tx_power[4]  = { "5db  ", 4 };
	combo_items_tx_power[5]  = { "6db  ", 5 };
	combo_items_tx_power[6]  = { "-2db ", 6 };
	combo_items_tx_power[7]  = { "-4db ", 7 };
	combo_items_tx_power[8]  = { "-8db ", 8 };
	combo_items_tx_power[9]  = { "-12db", 9 };
	combo_items_tx_power[10] = { "-16db", 10 };
	combo_items_tx_power[11] = { "-10db", 11 };
	combo_items_tx_power[12] = { "-20db", 12 };
	combo_items_tx_power[13] = { "-25db", 13 };
	combo_items_tx_power[14] = { "-63db", 14 };

	combo_items_phy[0] = { "1M/bps"  , 0 };
	combo_items_phy[1] = { "2M/bps"  , 1 };
	combo_items_phy[2] = { "Coded S8", 2 };
	combo_items_phy[3] = { "Coded S2", 3 };

	combo_items_payload_model[0] = { "Constant Carrier", 0 };
	combo_items_payload_model[1] = { "PRBS9 sequence", 1 };
	combo_items_payload_model[2] = { "Repeated '11110000'", 2 };
	combo_items_payload_model[3] = { "Repeated '10101010'", 3 };

	for (uint32_t i = 0; i < 40; ++i) {
		auto labelName = (std::stringstream() << (i + 1) * 2).str();
		strcpy_s(combo_items_channel[i].name, labelName.c_str());
		combo_items_channel[i].value = (i + 1) * 2;
	}

	for (uint32_t i = 0; i < 255; ++i) {
		auto labelName = (std::stringstream() << (i + 1) << " s").str();
		strcpy_s(combo_items_seconds[i].name, labelName.c_str());
		combo_items_seconds[i].value = i;
	}

	for (uint32_t i = 0; i < 255; ++i) {
		auto labelName = (std::stringstream() << (i + 1)).str();
		strcpy_s(combo_items_payload_len[i].name, labelName.c_str());
		combo_items_payload_len[i].value = i;
	}

	strcpy_s(result_find_dev_buff, 8, "");
	strcpy_s(result_action_buff, 8, "");

	LoadUserConfig();
}
bool RFToolApplication::main_init(int argc, char* argv[])
{
	ParseArg(argc, argv);

	// log
	logger = spdlog::basic_logger_mt(LOGGER_NAME, LOGGER_FILE);
	logger->set_level(spdlog::level::trace);
	logger->info("main initializing...");
	logger->info("Log system OK");

	// Enable Dock
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	//io.Fonts->Clear();
	//io.Fonts->AddFontFromFileTTF(ofToDataPath("fonts/OpenSans-Light.ttf", true).c_str(), 16);
	//io.Fonts->AddFontFromFileTTF(ofToDataPath("fonts/OpenSans-Regular.ttf", true).c_str(), 16);
	//io.Fonts->AddFontFromFileTTF(ofToDataPath("fonts/OpenSans-Light.ttf", true).c_str(), 32);
	//io.Fonts->AddFontFromFileTTF(ofToDataPath("fonts/OpenSans-Regular.ttf", true).c_str(), 11);
	//io.Fonts->AddFontFromFileTTF(ofToDataPath("fonts/OpenSans-Bold.ttf", true).c_str(), 11);
	//io.Fonts->Build();

	// Theme
	ImGui::StyleColorsLight();
	ImGuiStyle* style = &ImGui::GetStyle();

	style->WindowPadding = ImVec2(15, 15);
	style->WindowRounding = 5.0f;
	style->FramePadding = ImVec2(5, 5);
	style->FrameRounding = 4.0f;
	style->ItemSpacing = ImVec2(12, 8);
	style->ItemInnerSpacing = ImVec2(8, 6);
	style->IndentSpacing = 25.0f;
	style->ScrollbarSize = 15.0f;
	style->ScrollbarRounding = 9.0f;
	style->GrabMinSize = 5.0f;
	style->GrabRounding = 3.0f;

	style->Colors[ImGuiCol_Text] = ImVec4(0.00f, 0.0f, 0.0f, 1.00f);
	style->Colors[ImGuiCol_TextDisabled] = ImVec4(0.40f, 0.39f, 0.38f, 0.77f);
	style->Colors[ImGuiCol_WindowBg] = ImVec4(0.92f, 0.91f, 0.88f, 0.70f);
	//style->Colors[ImGuiCol_ChildWindowBg] = ImVec4(1.00f, 0.98f, 0.95f, 0.58f);
	style->Colors[ImGuiCol_PopupBg] = ImVec4(0.92f, 0.91f, 0.88f, 0.92f);
	style->Colors[ImGuiCol_Border] = ImVec4(0.84f, 0.83f, 0.80f, 0.65f);
	style->Colors[ImGuiCol_BorderShadow] = ImVec4(0.92f, 0.91f, 0.88f, 0.00f);
	style->Colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 0.98f, 0.95f, 1.00f);
	style->Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.99f, 1.00f, 0.40f, 0.78f);
	style->Colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 1.00f, 0.00f, 1.00f);
	style->Colors[ImGuiCol_TitleBg] = ImVec4(1.00f, 0.98f, 0.95f, 1.00f);
	style->Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 0.98f, 0.95f, 0.75f);
	style->Colors[ImGuiCol_TitleBgActive] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
	style->Colors[ImGuiCol_MenuBarBg] = ImVec4(1.00f, 0.98f, 0.95f, 0.47f);
	style->Colors[ImGuiCol_ScrollbarBg] = ImVec4(1.00f, 0.98f, 0.95f, 1.00f);
	style->Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.00f, 0.00f, 0.00f, 0.21f);
	style->Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.90f, 0.91f, 0.00f, 0.78f);
	style->Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
	//style->Colors[ImGuiCol_ComboBg] = ImVec4(1.00f, 0.98f, 0.95f, 1.00f);
	style->Colors[ImGuiCol_CheckMark] = ImVec4(0.25f, 1.00f, 0.00f, 0.80f);
	style->Colors[ImGuiCol_SliderGrab] = ImVec4(0.00f, 0.00f, 0.00f, 0.14f);
	style->Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
	style->Colors[ImGuiCol_Button] = ImVec4(0.00f, 0.00f, 0.00f, 0.14f);
	style->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.99f, 1.00f, 0.22f, 0.86f);
	style->Colors[ImGuiCol_ButtonActive] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
	style->Colors[ImGuiCol_Header] = ImVec4(0.25f, 1.00f, 0.00f, 0.76f);
	style->Colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 1.00f, 0.00f, 0.86f);
	style->Colors[ImGuiCol_HeaderActive] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
	//style->Colors[ImGuiCol_Column] = ImVec4(0.00f, 0.00f, 0.00f, 0.32f);
	//style->Colors[ImGuiCol_ColumnHovered] = ImVec4(0.25f, 1.00f, 0.00f, 0.78f);
	//style->Colors[ImGuiCol_ColumnActive] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
	style->Colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.04f);
	style->Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.25f, 1.00f, 0.00f, 0.78f);
	style->Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
	//style->Colors[ImGuiCol_CloseButton] = ImVec4(0.40f, 0.39f, 0.38f, 0.16f);
	//style->Colors[ImGuiCol_CloseButtonHovered] = ImVec4(0.40f, 0.39f, 0.38f, 0.39f);
	//style->Colors[ImGuiCol_CloseButtonActive] = ImVec4(0.40f, 0.39f, 0.38f, 1.00f);
	style->Colors[ImGuiCol_PlotLines] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
	style->Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
	style->Colors[ImGuiCol_PlotHistogram] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
	style->Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
	style->Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25f, 1.00f, 0.00f, 0.43f);
	//style->Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(1.00f, 0.98f, 0.95f, 0.73f);
	
	logger->info("Theme OK");

	// Font
	io.Fonts->AddFontFromFileTTF(DEFAULT_FONT, 13.0f, NULL, io.Fonts->GetGlyphRangesChineseFull());
	logger->info("Font OK");

	// USB
	HIDInit();
	logger->info("USB HID OK");


	return true;
}
void RFToolApplication::main_shutdown(void)
{
	logger->info("main shutdown");
	HIDExit();
}
int RFToolApplication::main_gui()
{
	ShowRootWindow(&opt_show_root_window);

	static int fc = 0;
	if (fc++ >= CONFIG_SAVE_INTERVAL) {
		fc = 0;
		SaveUserConfig();
	}

	return 0;
}

void RFToolApplication::ShowRFWindow(bool* p_open)
{
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar;
	if (!ImGui::Begin(WINNAME_RF, p_open, window_flags)) {
		ImGui::End();
		return;
	}

	ImGui::SeparatorText("Base");

	ImGui::InputText("VID", vid_buff, 5);
	ImGui::InputText("PID", pid_buff, 5);
	ImGui::InputText("Report ID", rid_buff, 3);
	if (ImGui::Button(u8"设定")) {
		strcpy_s(result_find_dev_buff, 8, "");
		if (ResultCode::SUCCESS == BtnScanClicked()) {
			strcpy_s(result_find_dev_buff, 8, "PASS");
		} else {
			strcpy_s(result_find_dev_buff, 8, "FAIL");
		}
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(DPI(40.0f));
	ImGui::InputText(u8"##RESULT_FIND_DEV", result_find_dev_buff, IM_ARRAYSIZE(result_find_dev_buff));

	utils::ValidateU16Text(vid_buff, vid);
	utils::ValidateU16Text(pid_buff, pid);
	utils::ValidateU8Text(rid_buff, rid);
	sprintf_s(vid_buff, 5, "%04X", vid);
	sprintf_s(pid_buff, 5, "%04X", pid);
	sprintf_s(rid_buff, 3, "%02X", rid);



	ImGui::SeparatorText(u8"RF数据设定");
	utils::BeginDisable((tx_rx_mode == MODE_RX));
	if (ImGui::BeginCombo("TX Power", combo_items_tx_power[combo_tx_power_current_idx].name, 0))
	{
		for (int n = 0; n < IM_ARRAYSIZE(combo_items_tx_power); n++)
		{
			const bool is_selected = (combo_tx_power_current_idx == n);
			if (ImGui::Selectable(combo_items_tx_power[n].name, is_selected)) {
				combo_tx_power_current_idx = n;
				LOG_INFO("tx power:%d", combo_items_tx_power[combo_tx_power_current_idx].value);
			}

			if (is_selected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	utils::EndDisable((tx_rx_mode == MODE_RX));


	utils::BeginDisable((tx_rx_mode == MODE_TX) && 
		(combo_items_payload_model[combo_payload_model_current_idx].value == CARRIER));
	if (ImGui::BeginCombo("Phy:##PHY", combo_items_phy[combo_phy_current_idx].name, 0))
	{
		for (int n = 0; n < IM_ARRAYSIZE(combo_items_phy); n++)
		{
			const bool is_selected = (combo_phy_current_idx == n);
			if (ImGui::Selectable(combo_items_phy[n].name, is_selected)) {
				combo_phy_current_idx = n;
				LOG_INFO("phy:%d", combo_items_phy[combo_phy_current_idx].value);
			}

			if (is_selected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	utils::EndDisable((tx_rx_mode == MODE_TX) && 
		(combo_items_payload_model[combo_payload_model_current_idx].value == CARRIER));

	ImGui::SeparatorText(u8"模式设定");

	ImGuiStyle& style = ImGui::GetStyle();
	ImVec2 region = ImGui::GetContentRegionAvail();
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::BeginChild("##MODE_SINGLE", ImVec2(((region.x - style.WindowPadding.x) / 2.0f), DPI(CHILD_HEIGHT)), true);
	{
		utils::BeginDisable((tx_rx_mode == MODE_RX));
		ImGui::RadioButton("Single", &test_mode_setting, 0);
		utils::EndDisable((tx_rx_mode == MODE_RX));

		utils::BeginDisable((tx_rx_mode == MODE_TX) && (test_mode_setting == MODE_SWEEP));
		if (ImGui::BeginCombo("Channel##SINGLE_MODE_CHANNEL", combo_items_channel[combo_single_mode_channel_current_idx].name, 0))
		{
			for (int n = 0; n < IM_ARRAYSIZE(combo_items_channel); ++n)
			{
				bool is_selected = (combo_single_mode_channel_current_idx == n);
				if (ImGui::Selectable(combo_items_channel[n].name, is_selected)) {
					combo_single_mode_channel_current_idx = n;
					LOG_INFO("channel:%d", combo_items_channel[combo_single_mode_channel_current_idx].value);
				}
				if (is_selected) {
					ImGui::SetItemDefaultFocus();
				}
			}

			ImGui::EndCombo();
		}

		if (ImGui::Button("H", ImVec2(50.0f, 50.0f))) {
			combo_single_mode_channel_current_idx = 39;
		}
		ImGui::SameLine();
		if (ImGui::Button("M", ImVec2(50.0f, 50.0f))) {
			combo_single_mode_channel_current_idx = 19;
		}
		ImGui::SameLine();
		if (ImGui::Button("L", ImVec2(50.0f, 50.0f))) {
			combo_single_mode_channel_current_idx = 0;
		}
		utils::EndDisable((tx_rx_mode == MODE_TX) && (test_mode_setting == MODE_SWEEP));

		ImGui::EndChild();
	}
	ImGui::SameLine();
	ImGui::BeginChild("##MODE_SWEEP", ImVec2(((region.x - style.WindowPadding.x) / 2.0f), DPI(CHILD_HEIGHT)), true);
	{
		utils::BeginDisable((tx_rx_mode == MODE_RX));
		ImGui::RadioButton("Sweep", &test_mode_setting, 1);
		utils::EndDisable((tx_rx_mode == MODE_RX));

		utils::BeginDisable((tx_rx_mode == MODE_RX) || (test_mode_setting == MODE_SINGLE));
		if (ImGui::BeginCombo("Channel Min##SWEEP_MODE_CHANNEL_MIN", combo_items_channel[combo_sweep_mode_channel_min_current_idx].name, 0))
		{
			for (int n = 0; n < IM_ARRAYSIZE(combo_items_channel); ++n)
			{
				bool is_selected = (combo_sweep_mode_channel_min_current_idx == n);
				if (ImGui::Selectable(combo_items_channel[n].name, is_selected)) {

					if (combo_items_channel[combo_sweep_mode_channel_max_current_idx].value < combo_items_channel[n].value) {
						combo_sweep_mode_channel_min_current_idx = combo_sweep_mode_channel_max_current_idx;
					} else {
						combo_sweep_mode_channel_min_current_idx = n;
					}

					LOG_INFO("sweep min channel:%d", combo_items_channel[combo_sweep_mode_channel_min_current_idx].value);
				}

				if (is_selected) {
					ImGui::SetItemDefaultFocus();
				}
			}

			ImGui::EndCombo();
		}

		if (ImGui::BeginCombo("Channel Max##SWEEP_MODE_CHANNEL_MAX", combo_items_channel[combo_sweep_mode_channel_max_current_idx].name, 0))
		{
			for (int n = 0; n < IM_ARRAYSIZE(combo_items_channel); ++n)
			{
				bool is_selected = (combo_sweep_mode_channel_max_current_idx == n);
				if (ImGui::Selectable(combo_items_channel[n].name, is_selected)) {
					if (combo_items_channel[combo_sweep_mode_channel_min_current_idx].value > combo_items_channel[n].value) {
						combo_sweep_mode_channel_max_current_idx = combo_sweep_mode_channel_min_current_idx;
					} else {
						combo_sweep_mode_channel_max_current_idx = n;
					}
					LOG_INFO("sweep max channel:%d", combo_items_channel[combo_sweep_mode_channel_max_current_idx].value);
				}

				if (is_selected) {
					ImGui::SetItemDefaultFocus();
				}
			}

			ImGui::EndCombo();
		}

		if (ImGui::BeginCombo("Sweep Time##SWEEP_TIME", combo_items_seconds[combo_sweep_time_current_idx].name, 0))
		{
			for (int n = 0; n < IM_ARRAYSIZE(combo_items_seconds); ++n)
			{
				bool is_selected = (combo_sweep_time_current_idx == n);
				if (ImGui::Selectable(combo_items_seconds[n].name, is_selected)) {
					combo_sweep_time_current_idx = n;
					LOG_INFO("sweep time:%d", combo_items_seconds[combo_sweep_time_current_idx].value);
				}

				if (is_selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		utils::EndDisable((tx_rx_mode == MODE_RX) || (test_mode_setting == MODE_SINGLE));
		ImGui::EndChild();
	}

	ImGui::SeparatorText(u8"Payload设定");
	ImGui::BeginChild("##PAYLOAD", ImVec2(region.x, DPI(CHILD_HEIGHT)), true);
	{
		utils::BeginDisable((tx_rx_mode == MODE_RX));
		if (ImGui::BeginCombo("Payload Mode##PAYLOAD_MODE", combo_items_payload_model[combo_payload_model_current_idx].name, 0))
		{
			for (int n = 0; n < IM_ARRAYSIZE(combo_items_payload_model); ++n)
			{
				bool is_selected = (combo_payload_model_current_idx == n);
				if (ImGui::Selectable(combo_items_payload_model[n].name, is_selected)) {
					combo_payload_model_current_idx = n;
					LOG_INFO("payload mode:%d", combo_items_payload_model[combo_payload_model_current_idx].value);
				}

				if (is_selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		utils::EndDisable((tx_rx_mode == MODE_RX));

		utils::BeginDisable((tx_rx_mode == MODE_RX) || (combo_items_payload_model[combo_payload_model_current_idx].value == CARRIER));
		if (ImGui::BeginCombo("Payload Length##PAYLOAD_LEN", combo_items_payload_len[combo_payload_len_current_idx].name, 0))
		{
			for (int n = 0; n < IM_ARRAYSIZE(combo_items_payload_len); ++n)
			{
				bool is_selected = (combo_payload_len_current_idx == n);
				if (ImGui::Selectable(combo_items_payload_len[n].name, is_selected)) {
					combo_payload_len_current_idx = n;
					LOG_INFO("payload length:%d", combo_items_payload_len[combo_payload_len_current_idx].value);
				}

				if (is_selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		utils::EndDisable((tx_rx_mode == MODE_RX) || (combo_items_payload_model[combo_payload_model_current_idx].value == CARRIER));

		ImGui::RadioButton("TX", &tx_rx_mode, 0); ImGui::SameLine();
		ImGui::RadioButton("RX", &tx_rx_mode, 1);

		if (ImGui::Button(u8"开始##BTN_START")) {
			ResultCode r = BtnStartClicked();
			if (ResultCode::SUCCESS == r) {
				strcpy_s(result_action_buff, 64, u8"命令执行成功");
			}
			else {
				strcpy_s(result_action_buff, 64, (std::stringstream() << u8"命令执行错误，错误码：" << (int)r).str().c_str());
			}
		}
		ImGui::SameLine();
		if (ImGui::Button(u8"停止##BTN_STOP")) {
			uint16_t rx_packet_num = 0;
			ResultCode r = BtnStopClicked(&rx_packet_num);
			if (ResultCode::SUCCESS == r) {
				if (tx_rx_mode == MODE_RX) {
					strcpy_s(result_action_buff, 64, (std::stringstream() << u8"包数：" << rx_packet_num).str().c_str());
				} else {
					strcpy_s(result_action_buff, 64, u8"命令执行成功");
				}
			} else {
				strcpy_s(result_action_buff, 64, (std::stringstream() << u8"命令执行错误，错误码：" << (int)r).str().c_str());
			}
		}

		ImGui::EndChild();
	}
	ImGui::PopStyleVar();	// WindowPadding

	ImGui::InputText("Result:##RESULT_ACTION", result_action_buff, IM_ARRAYSIZE(result_action_buff));

	ImGui::End();

}


void RFToolApplication::ParseArg(int argc, char* argv[])
{
	for (int i = 1; i < argc; ++i)
	{
		std::string cmd = argv[i];
		logger->info("arg[{0}] {1}", i, cmd.c_str());
	}
}

void RFToolApplication::ShowRootWindowMenu()
{
	if (ImGui::BeginMenuBar()) {

		if (ImGui::BeginMenu("View")) {

			// ImGui::MenuItem("Demo Window", NULL, &opt_showdemowindow);
			ImGui::MenuItem("RF Test", NULL, &opt_show_rf_window);
			ImGui::Separator();
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}
}

void RFToolApplication::ShowRootWindow(bool* p_open)
{
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

	if (opt_fullscreen) {
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::SetNextWindowViewport(viewport->ID);
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	}
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

	ImGui::Begin("-", p_open, window_flags);

	if (opt_fullscreen) {
		ImGui::PopStyleVar();	// WindowBorderSize
		ImGui::PopStyleVar();	// WindowRounding
	}
	ImGui::PopStyleVar();	// WindowPadding

	ImGuiID dockspace_id = ImGui::GetID(DOCKSPACE_ID);
	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_AutoHideTabBar | ImGuiDockNodeFlags_NoUndocking | ImGuiDockNodeFlags_NoResize);

	ShowRootWindowMenu();

	ImGui::End();

	if (opt_showdemowindow) {
		ImGui::ShowDemoWindow(&opt_showdemowindow);
	}

	if (opt_show_rf_window) {
		ShowRFWindow(&opt_show_rf_window);
	}
}

ResultCode RFToolApplication::BtnScanClicked()
{
	HIDDevice dev;
	auto ret = OpenHIDInterface(vid, pid, rid, &dev);
	if (ret != HID_FIND_SUCCESS) {
		return ResultCode::FAILED_FIND_DEV;
	}
	if (nullptr == dev.phandle)
		return ResultCode::FAILED_FIND_DEV;
	if (nullptr != device.phandle) {
		CloseHIDInterface(device);
	}
	device = dev;
	return ResultCode::SUCCESS;
}

std::vector<uint8_t> RFToolApplication::GenCommand(bool is_start)
{
	std::vector<uint8_t> temp(USB_HID_REPORT_CONTENT_SIZE);
	uint8_t* buffer = temp.data();
	memset(buffer, USB_HID_REPORT_CONTENT_SIZE, 0);
	int i = 0;
	buffer[i++] = 0x04; //
	buffer[i++] = 0xFF; // Check sum lsb
	buffer[i++] = 0xFF; // Check sum msb
	buffer[i++] = 0xB0;
	*(uint32_t*)(buffer + i) = 55; i += 4;	// len
	buffer[i++] = is_start   ? 0x01 : 0x00;	// start:0x01, stop:0x00
	buffer[i++] = tx_rx_mode;				// tx:0x00, rx:0x01
	buffer[i++] = combo_items_payload_model[combo_payload_model_current_idx].value;
	buffer[i++] = combo_items_payload_len[combo_payload_len_current_idx].value;
	buffer[i++] = test_mode_setting;		// Single:0x00, Sweep:0x01
	if (tx_rx_mode || test_mode_setting == MODE_SINGLE)
	{
		int channel = combo_items_channel[combo_single_mode_channel_current_idx].value;
		buffer[i++] = channel;
		buffer[i++] = channel;
		buffer[i++] = 0x00;					// Reserve
		buffer[i++] = 0x00;					// Reserve
	}
	else
	{
		buffer[i++] = combo_items_channel[combo_sweep_mode_channel_min_current_idx].value;
		buffer[i++] = combo_items_channel[combo_sweep_mode_channel_max_current_idx].value;
		buffer[i++] = 0x00;					// Reserve
		buffer[i++] = combo_items_seconds[combo_sweep_time_current_idx].value;
	}
	buffer[i++] = combo_items_tx_power[combo_tx_power_current_idx].value;
	buffer[i++] = combo_items_phy[combo_phy_current_idx].value;

	*(uint16_t*)(buffer + 1) = utils::checksum16(buffer + 3, 60);

	if (is_start) 
	{
		if (tx_rx_mode == MODE_TX)
		{
			if (test_mode_setting == MODE_SINGLE)
			{
				if (combo_items_payload_model[combo_payload_model_current_idx].value == CARRIER)
				{
					// 1. single send carrier
					logger->info("Start TX Test (Single)(Carrier)====");
					logger->info("channel       : {}MHz", (2400 + (combo_items_channel[combo_single_mode_channel_current_idx].value + 1) * 2));
					logger->info("tx_power      : {}", combo_items_tx_power[combo_tx_power_current_idx].name);
					logger->info("phy           : {}", combo_items_phy[combo_phy_current_idx].name);
				}
				else
				{
					// 2. single send payload
					logger->info("Start TX Test (Single)(Payload)====");
					logger->info("channel       : {}MHz", (2400 + (combo_items_channel[combo_single_mode_channel_current_idx].value + 1) * 2));
					logger->info("tx_power      : {}", combo_items_tx_power[combo_tx_power_current_idx].name);
					logger->info("phy           : {}", combo_items_phy[combo_phy_current_idx].name);
					logger->info("payload_model : {}", combo_items_payload_model[combo_payload_model_current_idx].name);
					logger->info("payload_len   : {}", combo_items_payload_len[combo_payload_len_current_idx].name);
				}
			}
			else if (test_mode_setting == MODE_SWEEP)
			{
				if (combo_items_payload_model[combo_payload_model_current_idx].value == CARRIER)
				{
					// 3. sweep send carrier
					logger->info("Start TX Test (Sweep)(Carrier)====");
					logger->info("channel_min   : {}MHz", (2400 + combo_items_channel[combo_sweep_mode_channel_min_current_idx].value));
					logger->info("channel_max   : {}MHz", (2400 + combo_items_channel[combo_sweep_mode_channel_max_current_idx].value));
					logger->info("sweep_time    : {}s", combo_items_seconds[combo_sweep_time_current_idx].value + 1);
					logger->info("tx_power      : {}", combo_items_tx_power[combo_tx_power_current_idx].name);
					logger->info("phy           : {}", combo_items_phy[combo_phy_current_idx].name);
				}
				else
				{
					// 3. sweep send payload
					logger->info("Start TX Test (Sweep)(Payload)====");
					logger->info("channel_min   : {}MHz", (2400 + combo_items_channel[combo_sweep_mode_channel_min_current_idx].value));
					logger->info("channel_max   : {}MHz", (2400 + combo_items_channel[combo_sweep_mode_channel_max_current_idx].value));
					logger->info("sweep_time    : {}s", combo_items_seconds[combo_sweep_time_current_idx].value + 1);
					logger->info("tx_power      : {}", combo_items_tx_power[combo_tx_power_current_idx].name);
					logger->info("phy           : {}", combo_items_phy[combo_phy_current_idx].name);
					logger->info("payload_model : {}", combo_items_payload_model[combo_payload_model_current_idx].name);
					logger->info("payload_len   : {}", combo_items_payload_len[combo_payload_len_current_idx].name);
				}
			}
		}
		else if (tx_rx_mode = MODE_RX)
		{
			// 4. rx
			logger->info("Start RX Test====");
			logger->info("channel       : {}MHz", (2400 + combo_items_channel[combo_single_mode_channel_current_idx].value));
			logger->info("phy           : {}", combo_items_phy[combo_phy_current_idx].name);
		}
	}
	else
	{
		// 5. stop
		logger->info("Stop Test====");
	}
	logger->debug("command {:n}", spdlog::to_hex(buffer, buffer + i));

	return temp;
}

ResultCode RFToolApplication::BtnStartClicked()
{
	if (nullptr == device.phandle)
		return ResultCode::INVALID_DEVICE;

	try {
		auto command = GenCommand(true);
		uint8_t rsp[USB_HID_REPORT_CONTENT_SIZE];
		int ret = 0;

		ret = hid_set_report(device.phandle, command.data(), USB_HID_REPORT_CONTENT_SIZE);
		ret = hid_get_report(device.phandle, rsp, USB_HID_REPORT_CONTENT_SIZE, 1000);

		if (ret == 0)
			return ResultCode::GET_REPORT_TIMEOUT;
		if (rsp[0] != 0x04)
			return ResultCode::UNEXPECTED_RSP;
		if (rsp[3] != 0xB0)
			return ResultCode::UNEXPECTED_RSP;
		if (rsp[8] == 0)
			return ResultCode::SUCCESS;
		else
			return ResultCode::CMD_RET_CODE_UNEXPECTED_ERRPR;
	}
	catch (std::exception& e) {
		logger->error("[Exception] {}", e.what());
		return ResultCode::EXCEPTION;
	}
	return ResultCode::SUCCESS;
}

ResultCode RFToolApplication::BtnStopClicked(uint16_t* rx_packet_num)
{
	if (nullptr == device.phandle)
		return ResultCode::INVALID_DEVICE;

	try {
		auto command = GenCommand(false);
		uint8_t rsp[USB_HID_REPORT_CONTENT_SIZE];
		int ret = 0;

		ret = hid_set_report(device.phandle, command.data(), USB_HID_REPORT_CONTENT_SIZE);
		ret = hid_get_report(device.phandle, rsp, USB_HID_REPORT_CONTENT_SIZE, 1000);

		if (ret == 0)
			return ResultCode::GET_REPORT_TIMEOUT;
		if (rsp[0] != 0x04)
			return ResultCode::UNEXPECTED_RSP;
		if (rsp[3] != 0xB0)
			return ResultCode::UNEXPECTED_RSP;

		*rx_packet_num = *(uint16_t*)(rsp + 9);

		if (rsp[8] == 0)
			return ResultCode::SUCCESS;
		else
			return ResultCode::CMD_RET_CODE_UNEXPECTED_ERRPR;
	}
	catch (std::exception& e) {
		logger->error("[Exception] {}", e.what());
		return ResultCode::EXCEPTION;
	}
	return ResultCode::SUCCESS;
}

void RFToolApplication::LoadUserConfig()
{
	auto user_config_file = std::filesystem::current_path() / USER_CONFIG_FILE;
	INIReader reader(user_config_file.generic_string());

	strcpy_s(vid_buff, 5, reader.GetString("option", "vid", "320F").c_str());
	strcpy_s(pid_buff, 5, reader.GetString("option", "pid", "5088").c_str());
	strcpy_s(rid_buff, 3, reader.GetString("option", "rid", "3F").c_str());
	utils::ValidateU16Text(vid_buff, vid);
	utils::ValidateU16Text(pid_buff, pid);
	utils::ValidateU8Text(rid_buff, rid);
	sprintf_s(vid_buff, 5, "%04X", vid);
	sprintf_s(pid_buff, 5, "%04X", pid);
	sprintf_s(rid_buff, 3, "%02X", rid);

	combo_tx_power_current_idx = reader.GetInteger("option", "tx_power", 0);
	combo_phy_current_idx = reader.GetInteger("option", "phy", 0);

	test_mode_setting = reader.GetInteger("option", "mode", 0);
	combo_single_mode_channel_current_idx = reader.GetInteger("option", "single_mode_channel", 0);
	combo_sweep_mode_channel_min_current_idx = reader.GetInteger("option", "sweep_mode_channel_min", 0);
	combo_sweep_mode_channel_max_current_idx = reader.GetInteger("option", "sweep_mode_channel_max", 39);
	combo_sweep_time_current_idx = reader.GetInteger("option", "sweep_time", 0);

	combo_payload_model_current_idx = reader.GetInteger("option", "payload_model", 0);
	combo_payload_len_current_idx = reader.GetInteger("option", "payload_len", 0);
	tx_rx_mode = reader.GetInteger("option", "tx_rx_mode", 0);
}

void RFToolApplication::SaveUserConfig()
{
	auto user_config_file = std::filesystem::current_path() / USER_CONFIG_FILE;

	std::ofstream out;

	out.open(user_config_file, std::ios::out);
	out << "[option]" << std::endl;
	out << "vid=" << vid_buff << std::endl;
	out << "pid=" << pid_buff << std::endl;
	out << "rid=" << rid_buff << std::endl;
	out << "tx_power=" << combo_tx_power_current_idx << std::endl;
	out << "phy=" << combo_phy_current_idx << std::endl;
	out << "mode=" << test_mode_setting << std::endl;
	out << "single_mode_channel=" << combo_single_mode_channel_current_idx << std::endl;
	out << "sweep_mode_channel_min=" << combo_sweep_mode_channel_min_current_idx << std::endl;
	out << "sweep_mode_channel_max=" << combo_sweep_mode_channel_max_current_idx << std::endl;
	out << "sweep_time=" << combo_sweep_time_current_idx << std::endl;
	out << "payload_model=" << combo_payload_model_current_idx << std::endl;
	out << "payload_len=" << combo_payload_len_current_idx << std::endl;
	out << "tx_rx_mode=" << tx_rx_mode << std::endl;
	out << std::endl;

	out.close();
}

