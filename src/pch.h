#ifndef __PCH_H__
#define __PCH_H__

#define APP_X				100
#define APP_Y				50
#define APP_WIDTH			600
#define APP_HEIGHT			700
#define APP_TITLE           L"USB RF Tool"
//#define DEFAULT_FONT		u8"D:\\Users\\Download\\Open_Sans\\static\\OpenSans-Regular.ttf"
#define DEFAULT_FONT		u8"C:\\Windows\\Fonts\\simhei.ttf"
#define USER_CONFIG_FILE    "Main.ini"
#define LOG_CONFIG_FILE     "my-conf.conf"
#define LOGGER_NAME         "basic_logger"
#define LOGGER_FILE         "basic-log.txt"

#define CONFIG_SAVE_INTERVAL 60

#define CHILD_HEIGHT 130.0f

#define DOCKSPACE_ID "MyDockSpace"
#define WINNAME_RF "RF Test Tool"

#define USB_UID_EP_MPS              (32)
#define USB_HID_REPORT_SIZE         (33)

#define USB_HID_REPORT_ID           (0x00)
#define USB_HID_REPORT_CONTENT_SIZE (USB_HID_REPORT_SIZE - 1)   //ReportID:1

#define LOG_INFO
#define LOG_ERROR

#endif