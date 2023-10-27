#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include "WiFiUser.h"
#include <EEPROM.h>
#include "key.h"
#include <Ticker.h>
#include <lvgl.h>
#include <LovyanGFX.h>
#include <SPI.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "lvgl_gui.h"
#include "lvgl_gif.h"
//#include "test.h"


//全局变量
const char * statedata;

uint16_t bedtemp_actual = 0;
uint16_t bedtemp_target = 0;
uint16_t last_bedtemp_target = 0;
uint16_t tooltemp_actual = 0;
uint16_t tooltemp_target = 0;
uint16_t last_tooltemp_target = 0;

String text_print_status = "standby"; //打印状态
String text_print_file_name = "No Printfile";  //打印文件名

String text_ext_actual_temp = " °C";
String text_ext_target_temp = " °C";
String text_bed_actual_temp = " °C";
String text_bed_target_temp = " °C";


int httpswitch = 1;

String nameStrpriting="0";
String fanspeed="0";

uint32_t keyscan_nowtime=0;
uint32_t keyscan_nexttime=0;

uint32_t debug_nexttime=0;

uint32_t netcheck_nowtime=0;
uint32_t netcheck_nexttime=0;
uint32_t sysInfo=0;

uint32_t httprequest_nowtime=0;
uint32_t httprequest_nexttime=0;

String to_String(int n);
Ticker timer1; 
Ticker timer2; 

String actState = "Standby ";

extern uint8_t test_mode_flag;
extern uint8_t test_key_cnt;
extern uint32_t test_key_timer_cnt;

#define TFT_WIDTH 240
#define TFT_HEIGHT 240 

static lv_disp_draw_buf_t draw_buf;    //定义显示器变量
static lv_color_t buf[TFT_WIDTH*10]; //定义refresh缓存

#define TP_INT 0
#define TP_RST 1

#define off_pin 35
#define buf_size 100

class LGFX : public lgfx::LGFX_Device
{

  lgfx::Panel_GC9A01 _panel_instance;
 // lgfx::Light_PWM     _light_instance;
  lgfx::Bus_SPI _bus_instance;

public:
  LGFX(void)
  {
    {
      auto cfg = _bus_instance.config();

      // SPIバスの設定
      cfg.spi_host = SPI2_HOST; // 使用するSPIを選択  ESP32-S2,C3 : SPI2_HOST or SPI3_HOST / ESP32 : VSPI_HOST or HSPI_HOST
      // ※ ESP-IDFバージョンアップに伴い、VSPI_HOST , HSPI_HOSTの記述は非推奨になるため、エラーが出る場合は代わりにSPI2_HOST , SPI3_HOSTを使用してください。
      cfg.spi_mode = 0;                  // SPI通信モードを設定 (0 ~ 3)
      cfg.freq_write = 80000000;         // 传输时的SPI时钟（最高80MHz，四舍五入为80MHz除以整数得到的值）
      cfg.freq_read = 20000000;          // 接收时的SPI时钟
      cfg.spi_3wire = true;              // 受信をMOSIピンで行う場合はtrueを設定
      cfg.use_lock = true;               // 使用事务锁时设置为 true
      cfg.dma_channel = SPI_DMA_CH_AUTO; // 使用するDMAチャンネルを設定 (0=DMA不使用 / 1=1ch / 2=ch / SPI_DMA_CH_AUTO=自動設定)
      // ※ ESP-IDFバージョンアップに伴い、DMAチャンネルはSPI_DMA_CH_AUTO(自動設定)が推奨になりました。1ch,2chの指定は非推奨になります。
      cfg.pin_sclk = 6;  // SPIのSCLKピン番号を設定
      cfg.pin_mosi = 7;  // SPIのCLKピン番号を設定
      cfg.pin_miso = -1; // SPIのMISOピン番号を設定 (-1 = disable)
      cfg.pin_dc = 2;    // SPIのD/Cピン番号を設定  (-1 = disable)

      _bus_instance.config(cfg);              // 設定値をバスに反映します。
      _panel_instance.setBus(&_bus_instance); // バスをパネルにセットします。
    }

    {                                      // 表示パネル制御の設定を行います。
      auto cfg = _panel_instance.config(); // 表示パネル設定用の構造体を取得します。

      cfg.pin_cs = 10;   // CSが接続されているピン番号   (-1 = disable)
      cfg.pin_rst = -1;  // RSTが接続されているピン番号  (-1 = disable)
      cfg.pin_busy = -1; // BUSYが接続されているピン番号 (-1 = disable)

      // The following settings are the general default values for each panel and the pin number to which BUSY is connected (-1 = disable).

      cfg.memory_width = 240;   // Maximum width supported by driver IC
      cfg.memory_height = 240;  // Maximum height supported by driver IC
      cfg.panel_width = 240;    // Actual displayable width
      cfg.panel_height = 240;   // Actual displayable height
      cfg.offset_x = 0;         // Amount of panel offset in X direction
      cfg.offset_y = 0;         // Amount of offset in Y direction of panel
      cfg.offset_rotation = 0;  // Value offset 0 to 7 in the direction of rotation (4 to 7 are inverted) 
      cfg.dummy_read_pixel = 8; // The number of virtual bits read before reading pixels
      cfg.dummy_read_bits = 1;  // Number of virtual read bits before reading data other than pixels
      cfg.readable = false;     // Set to true if you can read the data
      cfg.invert = true;        // Set to true if the panel's light/darkness is inverted
      cfg.rgb_order = false;     // Set to true if the red and blue colors of the panel are exchanged
      cfg.dlen_16bit = false;   // Set to true for panels that send data length in 16-bit units
      cfg.bus_shared = false;   // Set to true if the bus is shared with the SD card (perform bus control using drawJpgFile, etc.)

      _panel_instance.config(cfg);
    }

    setPanel(&_panel_instance); // 使用するパネルをセットします。
//    { // バックライト制御の設定を行います。(必要なければ削除）
//    auto cfg = _light_instance.config();// バックライト設定用の構造体を取得します。
//    cfg.pin_bl = 8;             // バックライトが接続されているピン番号 BL
//    cfg.invert = false;          // バックライトの輝度を反転させる場合 true
//    cfg.freq   = 44100;          // バックライトのPWM周波数
//    cfg.pwm_channel = 7;         // 使用するPWMのチャンネル番号
//    _light_instance.config(cfg);
//    _panel_instance.setLight(&_light_instance);//バックライトをパネルにセットします。
//    }
  }
};

 
// TFT_eSPI tft = TFT_eSPI(240,240);
LGFX tft;
 
// Printing Progress Percentage Variable Definitions
int16_t progress_data=0;
// Nozzle Temperature Percentage Variable Definition
int16_t ext_per_data=0;
// Hot Bed Temperature Percentage Variable Definition
int16_t bed_per_data=0;
// Fan Speed Percentage Variable Definition
int16_t fanspeed_data=0;

//----------------------------------------//
using namespace std;
#define max 100
String to_String(int n)
{
  int m = n;
  char s[max];
  char ss[max];
  int i=0,j=0;
  if (n < 0)
  {
      m = 0 - m;
      j = 1;
      ss[0] = '-';
  }

  while (m>0){
      s[i++] = m % 10 + '0';
      m /= 10;
  }

  s[i] = '\0';
  i = i - 1;
  while (i >= 0){
      ss[j++] = s[i--];
  }    
    ss[j] = '\0';    
  return ss;
 }

//--------------------------------------screen1---initialization----------------------------------------------//
void init_label_print_status()
{
    label_print_status = lv_label_create(lv_scr_act());

    lv_label_set_text(label_print_status, text_print_status.c_str());
    lv_obj_align(label_print_status, LV_ALIGN_CENTER, 0, 50); //居中显示
}

void init_label_print_progress()
{
    String TEXT = to_String(progress_data);

    label_print_progress = lv_label_create(lv_scr_act()); //创建文字对象

    lv_style_set_text_font(&style_label_print_progress, &lv_font_montserrat_48);  //设置字体样机及大小
    lv_style_set_text_color(&style_label_print_progress,lv_color_hex(0xFF0000));     //设置样式文本字颜色

    lv_obj_add_style(label_print_progress,&style_label_print_progress,LV_PART_MAIN);           //将样式添加到文字对象中
    lv_label_set_text(label_print_progress,TEXT.c_str());
    lv_obj_align(label_print_progress, LV_ALIGN_CENTER, 0, 0); //居中显示
}

void init_arc_print_progress()
{
    arc_print_progress = lv_arc_create(lv_scr_act()); //创建圆弧对象

    lv_style_set_arc_width(&style_arc_print_progress, 24);  // 设置样式的圆弧粗细
    lv_style_set_arc_color(&style_arc_print_progress, lv_color_hex(0x000000)); //设置背景圆环颜色

    lv_obj_add_style(arc_print_progress, &style_arc_print_progress, LV_PART_MAIN);  // 将样式应用到圆弧背景
    lv_obj_add_style(arc_print_progress, &style_arc_print_progress, LV_PART_INDICATOR);  // 将样式应用到圆弧前景


    lv_obj_remove_style(arc_print_progress,NULL,LV_PART_KNOB);  //移除样式
    lv_obj_clear_flag(arc_print_progress, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_color(arc_print_progress, lv_color_hex(0xFF0000), LV_PART_INDICATOR);//进度条颜色
    lv_obj_set_size(arc_print_progress,240,240);                   //设置尺寸
    lv_arc_set_rotation(arc_print_progress,270);                   //设置零度位置
    lv_arc_set_bg_angles(arc_print_progress,0,360);                //设置角度
    lv_arc_set_value(arc_print_progress,0);                       //设置初始值
    lv_obj_center(arc_print_progress);                             //居中显示
}

//----------------------------------------screen2----initialization------------------------------------------------------//
void init_label_extruder_actual_temp()
{
    label_ext_actual_temp = lv_label_create(lv_scr_act()); //创建文字对象

    lv_style_set_text_font(&style_label_ext_actual_temp, &lv_font_montserrat_32);  //设置字体样机及大小
    lv_style_set_text_color(&style_label_ext_actual_temp,lv_color_hex(0xFF0000));     //设置样式文本字颜色

    lv_obj_add_style(label_ext_actual_temp,&style_label_ext_actual_temp,LV_PART_MAIN);           //将样式添加到文字对象中
    lv_label_set_text(label_ext_actual_temp,text_ext_actual_temp.c_str());
    lv_obj_align(label_ext_actual_temp, LV_ALIGN_CENTER, 0, 75); //居中显示

}

void init_label_extruder_target_temp()
{
    label_ext_target_temp = lv_label_create(lv_scr_act()); //创建文字对象

    lv_style_set_text_font(&style_label_ext_target_temp, &lv_font_montserrat_32);  //设置字体样机及大小
    lv_style_set_text_color(&style_label_ext_target_temp,lv_color_hex(0xFF0000));     //设置样式文本字颜色

    lv_obj_add_style(label_ext_target_temp,&style_label_ext_target_temp,LV_PART_MAIN);           //将样式添加到文字对象中
    lv_label_set_text(label_ext_target_temp,text_ext_target_temp.c_str());
    lv_obj_align(label_ext_target_temp, LV_ALIGN_CENTER, 0, -75); //居中显示
}

void init_label_heaterbed_actual_temp()
{
    label_bed_actual_temp = lv_label_create(lv_scr_act()); //创建文字对象

    lv_style_set_text_font(&style_label_bed_actual_temp, &lv_font_montserrat_32);  //设置字体样机及大小
    lv_style_set_text_color(&style_label_bed_actual_temp,lv_color_hex(0xFF0000));     //设置样式文本字颜色

    lv_obj_add_style(label_bed_actual_temp,&style_label_bed_actual_temp,LV_PART_MAIN);           //将样式添加到文字对象中
    lv_label_set_text(label_bed_actual_temp,text_bed_actual_temp.c_str());
    lv_obj_align(label_bed_actual_temp, LV_ALIGN_CENTER, 0, 75); //居中显示
}

void init_label_heaterbed_target_temp()
{
    label_bed_target_temp = lv_label_create(lv_scr_act()); //创建文字对象

    lv_style_set_text_font(&style_label_bed_target_temp, &lv_font_montserrat_32);  //设置字体样机及大小
    lv_style_set_text_color(&style_label_bed_target_temp,lv_color_hex(0xFF0000));     //设置样式文本字颜色

    lv_obj_add_style(label_bed_target_temp,&style_label_bed_target_temp,LV_PART_MAIN);           //将样式添加到文字对象中
    lv_label_set_text(label_bed_target_temp,text_bed_target_temp.c_str());
    lv_obj_align(label_bed_target_temp, LV_ALIGN_CENTER, 0, -75); //居中显示
}

void init_arc_extruder_temp()
{
    arc_extruder_temp = lv_arc_create(lv_scr_act()); //创建圆弧对象

    lv_style_set_arc_width(&style_arc_extruder_temp, 8);  // 设置样式的圆弧粗细
    lv_obj_add_style(arc_extruder_temp, &style_arc_extruder_temp, LV_PART_MAIN);  // 将样式应用到圆弧背景
    lv_obj_add_style(arc_extruder_temp, &style_arc_extruder_temp, LV_PART_INDICATOR);  // 将样式应用到圆弧前景

    lv_obj_remove_style(arc_extruder_temp,NULL,LV_PART_KNOB);  //移除样式
    lv_obj_clear_flag(arc_extruder_temp, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_color(arc_extruder_temp, lv_palette_main(LV_PALETTE_ORANGE), LV_PART_INDICATOR);//进度条颜色
    lv_obj_set_size(arc_extruder_temp,240,240);                   //设置尺寸
    lv_arc_set_rotation(arc_extruder_temp,270);                   //设置零度位置
    lv_arc_set_bg_angles(arc_extruder_temp,0,360);                //设置角度
    lv_arc_set_value(arc_extruder_temp,100);                       //设置初始值
    lv_obj_center(arc_extruder_temp);                             //居中显示
}

void init_arc_heaterbed_temp()
{
    arc_heaterbed_temp = lv_arc_create(lv_scr_act()); //创建圆弧对象

    lv_style_set_arc_width(&style_arc_heaterbed_temp, 8);  // 设置样式的圆弧粗细
    lv_obj_add_style(arc_heaterbed_temp, &style_arc_heaterbed_temp, LV_PART_MAIN);  // 将样式应用到圆弧背景
    lv_obj_add_style(arc_heaterbed_temp, &style_arc_heaterbed_temp, LV_PART_INDICATOR);  // 将样式应用到圆弧前景

    lv_obj_remove_style(arc_heaterbed_temp,NULL,LV_PART_KNOB);  //移除样式
    lv_obj_clear_flag(arc_heaterbed_temp, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_color(arc_heaterbed_temp, lv_palette_main(LV_PALETTE_TEAL), LV_PART_INDICATOR);//进度条颜色
    lv_obj_set_size(arc_heaterbed_temp,224,224);                   //设置尺寸
    lv_arc_set_rotation(arc_heaterbed_temp,270);                   //设置零度位置
    lv_arc_set_bg_angles(arc_heaterbed_temp,0,360);                //设置角度
    lv_arc_set_value(arc_heaterbed_temp,100);                       //设置初始值
    lv_obj_center(arc_heaterbed_temp);                             //居中显示
}

//----------------------------------------screen3----initialization------------------------------------------------------//
void init_label_print_file()
{
    label_print_file = lv_label_create(lv_scr_act()); //创建文字对象

    //设置背景圆角半径为: 5
    lv_style_set_radius(&style_label_print_file, 3);
    //设置背景透明度
    lv_style_set_bg_opa(&style_label_print_file, LV_OPA_COVER);
    //设置背景颜色
    lv_style_set_bg_color(&style_label_print_file, lv_palette_lighten(LV_PALETTE_BLUE, 1));

    //设置外边框颜色为蓝色
    lv_style_set_border_color(&style_label_print_file, lv_palette_main(LV_PALETTE_BLUE));
    //设置填充
    lv_style_set_pad_all(&style_label_print_file, 2);

    lv_style_set_text_font(&style_label_print_file, &lv_font_montserrat_28);  //设置字体样机及大小
    lv_style_set_text_color(&style_label_print_file,lv_color_hex(0xffffff));     //设置样式文本字颜色

    lv_obj_add_style(label_print_file,&style_label_print_file,LV_PART_MAIN);           //将样式添加到文字对象中

    lv_label_set_long_mode(label_print_file, LV_LABEL_LONG_SCROLL_CIRCULAR);     
    lv_obj_set_width(label_print_file, 200);
    lv_obj_set_height(label_print_file, 200);

    lv_label_set_text(label_print_file, text_print_file_name.c_str());
    lv_obj_align(label_print_file, LV_ALIGN_CENTER, 0, 0);  
}

//----------------------------------------screen4----initialization------------------------------------------------------//
void init_label_ap_config()
{
  String TEXT = "AP Config...."; 

  label_ap_config = lv_label_create(lv_scr_act()); //创建文字对象

  lv_style_set_text_font(&style_label_ap_config, &lv_font_montserrat_20);  //设置字体样机及大小
  lv_style_set_text_color(&style_label_ap_config,lv_color_hex(0x2400FF));     //设置样式文本字颜色

  lv_obj_add_style(label_ap_config,&style_label_ap_config,LV_PART_MAIN);           //将样式添加到文字对象中
  lv_label_set_text(label_ap_config,TEXT.c_str());
  lv_obj_align(label_ap_config, LV_ALIGN_CENTER, 0, 0); //居中显示
}

//----------------------------------------screen5----initialization------------------------------------------------------//
void init_label_no_klipper()
{
  String TEXT = "No klipper connect";

  label_no_klipper = lv_label_create(lv_scr_act()); //创建文字对象

  lv_style_set_text_font(&style_label_no_klipper, &lv_font_montserrat_20);  //设置字体样机及大小
  lv_style_set_text_color(&style_label_no_klipper,lv_color_hex(0x2400FF));     //设置样式文本字颜色

  lv_obj_add_style(label_no_klipper,&style_label_no_klipper,LV_PART_MAIN);           //将样式添加到文字对象中
  lv_label_set_text(label_no_klipper,TEXT.c_str());
  lv_obj_align(label_no_klipper, LV_ALIGN_CENTER, 0, 0); //居中显示
}

//----------------------------------------screen6----initialization------------------------------------------------------//
void init_label_fan_speed()
{
    String TEXT = to_String(fanspeed_data);

    label_fan_speed = lv_label_create(lv_scr_act()); //创建文字对象

    lv_style_set_text_font(&style_label_fan_speed, &lv_font_montserrat_24);  //设置字体样机及大小
    lv_style_set_text_color(&style_label_fan_speed,lv_palette_main(LV_PALETTE_RED));     //设置样式文本字颜色

    lv_obj_add_style(label_fan_speed,&style_label_fan_speed,LV_PART_MAIN);           //将样式添加到文字对象中
    lv_label_set_text(label_fan_speed,TEXT.c_str());
    lv_obj_align(label_fan_speed, LV_ALIGN_CENTER, 0, -40); //居中显示
}

void init_bar_fan_speed()
{
  bar_fan_speed = lv_bar_create(lv_scr_act()); //创建圆弧对象

  lv_obj_set_style_arc_color(bar_fan_speed, lv_palette_main(LV_PALETTE_BLUE), LV_PART_INDICATOR);//进度条颜色

  lv_obj_set_size(bar_fan_speed,200,20);                   //设置尺寸
  lv_bar_set_range(bar_fan_speed,0,100);                   //设置开始结束
  lv_bar_set_value(bar_fan_speed,0, LV_ANIM_OFF);          //设置初始值
	lv_obj_align(bar_fan_speed, LV_ALIGN_CENTER, 0, 20); //居中显示
}

//----------------------------------------screen1---refresh-------------------------------------------------------//
void update_label_print_status(){

	label_print_status = lv_label_create(lv_scr_act()); //创建文字对象

	lv_obj_add_style(label_print_status,&style_label_print_status,LV_PART_MAIN);           //将样式添加到文字对象中
  lv_label_set_text(label_print_status, text_print_status.c_str());
  lv_obj_align(label_print_status, LV_ALIGN_CENTER, 0, 50); //居中显示
}

void update_label_print_progress(){

  String TEXT = to_String(progress_data);

  if(progress_data==0){
    TEXT = "0%";
  }else{
    TEXT = TEXT+"%";
  }

	label_print_progress = lv_label_create(lv_scr_act()); //创建文字对象

	lv_obj_add_style(label_print_progress,&style_label_print_progress,LV_PART_MAIN);           //将样式添加到文字对象中
	lv_label_set_text(label_print_progress,TEXT.c_str());
	lv_obj_align(label_print_progress, LV_ALIGN_CENTER, 0, 0); //居中显示

}

void update_arc_print_progress(){

  arc_print_progress = lv_arc_create(lv_scr_act()); //创建圆弧对象

  lv_style_set_arc_width(&style_arc_print_progress, 24);  // 设置样式的圆弧粗细
  lv_obj_add_style(arc_print_progress, &style_arc_print_progress, LV_PART_MAIN);  // 将样式应用到圆弧背景
  lv_obj_add_style(arc_print_progress, &style_arc_print_progress, LV_PART_INDICATOR);  // 将样式应用到圆弧前景

  lv_obj_remove_style(arc_print_progress,NULL,LV_PART_KNOB);  //移除样式
  lv_obj_clear_flag(arc_print_progress, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_arc_color(arc_print_progress, lv_color_hex(0xFF0000), LV_PART_INDICATOR);//进度条颜色
  lv_obj_set_size(arc_print_progress,240,240);                   //设置尺寸
  lv_arc_set_rotation(arc_print_progress,270);                   //设置零度位置
  lv_arc_set_bg_angles(arc_print_progress,0,360);                //设置角度
  lv_arc_set_value(arc_print_progress,progress_data);            //设置值
  lv_obj_center(arc_print_progress);                             //居中显示

}

//-----------------------------------------------screen2--refresh-----------------------------------------------------//
void update_label_extruder_actual_temp()
{
	label_ext_actual_temp = lv_label_create(lv_scr_act()); //创建文字对象

	lv_obj_add_style(label_ext_actual_temp,&style_label_ext_actual_temp,LV_PART_MAIN);           //将样式添加到文字对象中
  lv_label_set_text(label_ext_actual_temp, text_ext_actual_temp.c_str());
  lv_obj_align(label_ext_actual_temp, LV_ALIGN_CENTER, 0, 75); //居中显示
}

void update_label_extruder_target_temp()
{
	label_ext_target_temp = lv_label_create(lv_scr_act()); //创建文字对象

	lv_obj_add_style(label_ext_target_temp,&style_label_ext_target_temp,LV_PART_MAIN);           //将样式添加到文字对象中
  lv_label_set_text(label_ext_target_temp, text_ext_target_temp.c_str());
  lv_obj_align(label_ext_target_temp, LV_ALIGN_CENTER, 0, -75); //居中显示
}

void update_label_heaterbed_actual_temp()
{
	label_bed_actual_temp = lv_label_create(lv_scr_act()); //创建文字对象

	lv_obj_add_style(label_bed_actual_temp,&style_label_bed_actual_temp,LV_PART_MAIN);           //将样式添加到文字对象中
  lv_label_set_text(label_bed_actual_temp, text_bed_actual_temp.c_str());
  lv_obj_align(label_bed_actual_temp, LV_ALIGN_CENTER, 0, 75); //居中显示
}

void update_label_heaterbed_target_temp()
{
	label_bed_target_temp = lv_label_create(lv_scr_act()); //创建文字对象

	lv_obj_add_style(label_bed_target_temp,&style_label_bed_target_temp,LV_PART_MAIN);           //将样式添加到文字对象中
  lv_label_set_text(label_bed_target_temp, text_bed_target_temp.c_str());
  lv_obj_align(label_bed_target_temp, LV_ALIGN_CENTER, 0, -75); //居中显示
}

void update_arc_extruder_temp()
{

  arc_extruder_temp = lv_arc_create(lv_scr_act()); //创建圆弧对象

  lv_style_set_arc_width(&style_arc_extruder_temp, 8);  // 设置样式的圆弧粗细
  lv_obj_add_style(arc_extruder_temp, &style_arc_extruder_temp, LV_PART_MAIN);  // 将样式应用到圆弧背景
  lv_obj_add_style(arc_extruder_temp, &style_arc_extruder_temp, LV_PART_INDICATOR);  // 将样式应用到圆弧前景

  lv_obj_remove_style(arc_extruder_temp,NULL,LV_PART_KNOB);  //移除样式
  lv_obj_clear_flag(arc_extruder_temp, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_arc_color(arc_extruder_temp, lv_palette_main(LV_PALETTE_ORANGE), LV_PART_INDICATOR);//进度条颜色
  lv_obj_set_size(arc_extruder_temp,240,240);                   //设置尺寸
  lv_arc_set_rotation(arc_extruder_temp,270);                   //设置零度位置
  lv_arc_set_bg_angles(arc_extruder_temp,0,360);                //设置角度
  lv_arc_set_value(arc_extruder_temp,100);            //设置值
  lv_obj_center(arc_extruder_temp);                             //居中显示
}

void update_arc_heaterbed_temp()
{
  arc_heaterbed_temp = lv_arc_create(lv_scr_act()); //创建圆弧对象

  lv_style_set_arc_width(&style_arc_heaterbed_temp, 8);  // 设置样式的圆弧粗细
  lv_obj_add_style(arc_heaterbed_temp, &style_arc_heaterbed_temp, LV_PART_MAIN);  // 将样式应用到圆弧背景
  lv_obj_add_style(arc_heaterbed_temp, &style_arc_heaterbed_temp, LV_PART_INDICATOR);  // 将样式应用到圆弧前景

  lv_obj_remove_style(arc_heaterbed_temp,NULL,LV_PART_KNOB);  //移除样式
  lv_obj_clear_flag(arc_heaterbed_temp, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_arc_color(arc_heaterbed_temp, lv_palette_main(LV_PALETTE_TEAL), LV_PART_INDICATOR);//进度条颜色
  lv_obj_set_size(arc_heaterbed_temp,224,224);                   //设置尺寸
  lv_arc_set_rotation(arc_heaterbed_temp,270);                   //设置零度位置
  lv_arc_set_bg_angles(arc_heaterbed_temp,0,360);                //设置角度
  lv_arc_set_value(arc_heaterbed_temp,100);            //设置值
  lv_obj_center(arc_heaterbed_temp);                             //居中显示
}

//----------------------------------------screen3---refresh-------------------------------------------------------//
void update_label_print_file()
{
	label_print_file = lv_label_create(lv_scr_act()); //创建文字对象

	lv_obj_add_style(label_print_file,&style_label_print_file,LV_PART_MAIN);           //将样式添加到文字对象中

  lv_label_set_long_mode(label_print_file, LV_LABEL_LONG_SCROLL_CIRCULAR);
  lv_obj_set_width(label_print_file, 200);
  lv_label_set_text(label_print_file, text_print_file_name.c_str());
  lv_obj_align(label_print_file, LV_ALIGN_CENTER, 0, 0);  
}

//----------------------------------------screen4---refresh-------------------------------------------------------//
void update_label_ap_config()
{
  String TEXT = "AP Config...."; 

  label_ap_config = lv_label_create(lv_scr_act()); //创建文字对象

  lv_obj_add_style(label_ap_config,&style_label_ap_config,LV_PART_MAIN);           //将样式添加到文字对象中
  lv_label_set_text(label_ap_config,TEXT.c_str());
  lv_obj_align(label_ap_config, LV_ALIGN_CENTER, 0, 0); //居中显示
}

//----------------------------------------screen5---refresh-------------------------------------------------------//
void update_label_no_klipper()
{
  String TEXT = "No klipper connect";

  label_no_klipper = lv_label_create(lv_scr_act()); //创建文字对象

  lv_obj_add_style(label_no_klipper,&style_label_no_klipper,LV_PART_MAIN);           //将样式添加到文字对象中
  lv_label_set_text(label_no_klipper,TEXT.c_str());
  lv_obj_align(label_no_klipper, LV_ALIGN_CENTER, 0, 0); //居中显示
}

//----------------------------------------screen6---refresh-------------------------------------------------------//
void update_label_fan_speed()
{
  String TEXT = to_String(fanspeed_data);

  if(fanspeed_data==0){
    TEXT = "fan speed: 0%";
  }else{
    TEXT = "fan speed: "+TEXT+"%";
  }

	label_fan_speed = lv_label_create(lv_scr_act()); //创建文字对象

	lv_obj_add_style(label_fan_speed,&style_label_fan_speed,LV_PART_MAIN);           //将样式添加到文字对象中
	lv_label_set_text(label_fan_speed,TEXT.c_str());
	lv_obj_align(label_fan_speed, LV_ALIGN_CENTER, 0, -40); //居中显示
}

void update_bar_fan_speed()
{
  bar_fan_speed = lv_bar_create(lv_scr_act()); //创建圆弧对象

  lv_obj_set_style_arc_color(bar_fan_speed, lv_palette_main(LV_PALETTE_BLUE), LV_PART_INDICATOR);//进度条颜色

  lv_obj_set_size(bar_fan_speed,200,20);                   //设置尺寸
  lv_bar_set_range(bar_fan_speed,0,100);                   //设置开始结束
  lv_bar_set_value(bar_fan_speed,fanspeed_data, LV_ANIM_OFF);          //设置初始值
	lv_obj_align(bar_fan_speed, LV_ALIGN_CENTER, 0, 20); //居中显示
}

//-----------------------------------------------------------------------------------------------------//
void update_screen1(lv_timer_t * timer)
{
  update_gif_black_back_display();
  update_label_print_progress();
  update_arc_print_progress();

  exist_object_screen_flg = 1;
}

void update_screen2(lv_timer_t * timer)
{

}

void update_screen3(lv_timer_t * timer)
{
  update_label_print_file();

  exist_object_screen_flg = 3;
}

void update_screen4(lv_timer_t * timer)
{
  update_label_ap_config();

  exist_object_screen_flg = 4;
}

void update_screen5(lv_timer_t * timer)
{
  update_label_no_klipper();

  exist_object_screen_flg = 5;
}

void update_screen6(lv_timer_t * timer)
{
  update_label_fan_speed();
  update_bar_fan_speed();

  exist_object_screen_flg = 6;
}

void update_screen7(lv_timer_t * timer)
{
  update_gif_black_back_display();
  update_gif_Standby_display();

  exist_object_screen_flg = 7;
}

void update_screen8(lv_timer_t * timer)
{
  update_gif_black_back_display();
  update_gif_StartPrinting_display();

  exist_object_screen_flg = 8;
}

void update_screen9(lv_timer_t * timer)
{
  update_gif_black_back_display();
  update_gif_Printing_display();

  exist_object_screen_flg = 9;
}

void update_screen10(lv_timer_t * timer)
{
  update_gif_black_back_display();
  update_gif_PrintComplete_display();

  exist_object_screen_flg = 10;
}

void update_screen11(lv_timer_t * timer)
{
  update_gif_black_back_display();
  update_gif_bed_temp_display();
  update_label_heaterbed_actual_temp();
  update_label_heaterbed_target_temp();

  exist_object_screen_flg = 11;
}

void update_screen12(lv_timer_t * timer)
{
  update_gif_black_back_display();
  update_gif_ext_temp_display();
  update_label_extruder_actual_temp();
  update_label_extruder_target_temp();

  exist_object_screen_flg = 12;
}

void update_screen13(lv_timer_t * timer)
{

}

void update_screen14(lv_timer_t * timer)
{
  update_gif_black_back_display();
  update_gif_OK_display();

  exist_object_screen_flg = 14;
}

void update_screen15(lv_timer_t * timer)
{
  update_gif_black_back_display();
  update_gif_voron_display();

  exist_object_screen_flg = 15;
}

void update_screen18(lv_timer_t * timer)
{
  update_gif_black_back_display();
  update_gif_BeforePrinting_display();

  exist_object_screen_flg = 18;
}

void update_screen19(lv_timer_t * timer)
{
  update_gif_black_back_display();
  update_gif_AfterPrinting_display();

  exist_object_screen_flg = 19;
}

void update_screen20(lv_timer_t * timer)
{
  update_gif_AP_Config_back_display();
  update_gif_AP_Config_display();

  exist_object_screen_flg = 20;
}

void update_screen21(lv_timer_t * timer)
{
  update_gif_black_back_display();
  update_gif_Home_display();

  exist_object_screen_flg = 21;
}

void update_screen22(lv_timer_t * timer)
{
  update_gif_black_back_display();
  update_gif_levelling_display();

  exist_object_screen_flg = 22;
}

void update_screen23(lv_timer_t * timer)
{
  update_gif_wait_back_display();

  exist_object_screen_flg = 23;
}

//-----------------------------------------------------------//
void update_screen16(lv_timer_t * timer)
{
  if(exist_object_screen_flg==11)
  {
    lv_obj_del(label_bed_actual_temp); 
    lv_obj_del(label_bed_target_temp); 
    update_label_heaterbed_actual_temp();
    update_label_heaterbed_target_temp();
  }
}

void update_screen17(lv_timer_t * timer)
{
  if(exist_object_screen_flg==12)
  {
    lv_obj_del(label_ext_actual_temp);
    lv_obj_del(label_ext_target_temp);
    update_label_extruder_actual_temp();
    update_label_extruder_target_temp();
  }
}

//------------------------------------------------------------------------------------------------------------//
/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
 
    tft.startWrite();                                        //使能写功能
    tft.setAddrWindow(area->x1, area->y1, w, h);             //设置填充区域
    tft.pushColors((uint16_t *)&color_p->full, w * h, true); //写入颜色缓存和缓存大小
    tft.endWrite();                                          //关闭写功能
 
    lv_disp_flush_ready(disp); //调用区域填充颜色函数
}

void lv_display_led_Init()
{
    pinMode(3, OUTPUT);                 
    digitalWrite(3, HIGH);         //Backlight default start
}

void lv_display_Init()
{
    delay(100);
    tft.init();               // Initialization
    tft.setRotation(0);     //Screen rotation direction (landscape)
    lv_init();
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, TFT_WIDTH*10);

    //Initialize the display
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);

    //Change the following line to your display resolution
    disp_drv.hor_res = TFT_WIDTH;
    disp_drv.ver_res = TFT_HEIGHT;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
}

void timer1_cb() 
{
    lv_tick_inc(1);/* le the GUI do its work */ 
    KeyScan();
}

void timer2_cb() // Short press to clear the timer
{
    test_key_timer_cnt++;
    if(test_key_timer_cnt>10){
      test_key_timer_cnt = 0;
      test_key_cnt = 0;
    }
}

void delete_exist_object()
{
    if(exist_object_screen_flg==1){    //del screen1

      lv_obj_del(img_black_back);
      lv_obj_del(label_print_progress);
      lv_obj_del(arc_print_progress); 

    }else if(exist_object_screen_flg==2){

    }else if(exist_object_screen_flg==3){

      lv_obj_del(label_print_file);

    }else if(exist_object_screen_flg==4){

      lv_obj_del(label_ap_config);

    }else if(exist_object_screen_flg==5){

      lv_obj_del(label_no_klipper);

    }else if(exist_object_screen_flg==6){

      lv_obj_del(label_fan_speed);
      lv_obj_del(bar_fan_speed);

    }else if(exist_object_screen_flg==7){

      lv_obj_del(img_black_back);
      lv_obj_del(gif_Standby);
    }else if(exist_object_screen_flg==8){

      lv_obj_del(img_black_back);
      lv_obj_del(gif_StartPrinting);
    }else if(exist_object_screen_flg==9){

      lv_obj_del(img_black_back);
      lv_obj_del(gif_Printing);
    }else if(exist_object_screen_flg==10){

      lv_obj_del(img_black_back);
      lv_obj_del(gif_PrintComplete);
    }else if(exist_object_screen_flg==11){

      lv_obj_del(img_black_back);
      lv_obj_del(gif_bed_temp);
      lv_obj_del(label_bed_actual_temp); 
      lv_obj_del(label_bed_target_temp); 
    }else if(exist_object_screen_flg==12){

      lv_obj_del(img_black_back);
      lv_obj_del(gif_ext_temp);
      lv_obj_del(label_ext_actual_temp);
      lv_obj_del(label_ext_target_temp);
    }else if(exist_object_screen_flg==13){


    }else if(exist_object_screen_flg==14){

      lv_obj_del(img_black_back);
      lv_obj_del(gif_OK);
    }else if(exist_object_screen_flg==15){

      //lv_obj_del(img_black_back);
      //lv_obj_del(gif_voron);
    }else if(exist_object_screen_flg==18){

      lv_obj_del(img_black_back);
      lv_obj_del(gif_BeforePrinting);
    }else if(exist_object_screen_flg==19){

      lv_obj_del(img_black_back);
      lv_obj_del(gif_AfterPrinting);
    }else if(exist_object_screen_flg==20){

      lv_obj_del(gif_AP_Config_back);
      lv_obj_del(gif_AP_Config);
    }else if(exist_object_screen_flg==21){

      lv_obj_del(img_black_back);
      lv_obj_del(gif_Home);
    }else if(exist_object_screen_flg==22){

      lv_obj_del(img_black_back);
      lv_obj_del(gif_levelling);
    }else if(exist_object_screen_flg==23){

      lv_obj_del(gif_wait_back);
    }else{

    }

}

void Display_Object_Init()
{
    init_label_print_status();
    init_label_print_progress();
    init_arc_print_progress();

    init_label_extruder_actual_temp();
    init_label_extruder_target_temp();

    init_label_heaterbed_actual_temp();
    init_label_heaterbed_target_temp();

    init_label_print_file();

    init_label_ap_config();

    init_label_no_klipper();

    init_label_fan_speed();
    init_bar_fan_speed();

    init_gif_black_back_display();
    init_gif_Standby_display();

    lv_obj_del(label_print_status);
    lv_obj_del(label_print_progress);
    lv_obj_del(arc_print_progress); 

    lv_obj_del(label_ext_actual_temp);
    lv_obj_del(label_ext_target_temp);
    lv_obj_del(label_bed_actual_temp); 
    lv_obj_del(label_bed_target_temp); 

    lv_obj_del(label_print_file);

    lv_obj_del(label_ap_config);

    lv_obj_del(label_no_klipper);

    lv_obj_del(label_fan_speed);
    lv_obj_del(bar_fan_speed);

    lv_obj_del(img_black_back);
    lv_obj_del(gif_Standby);

    exist_object_screen_flg = 0;
    screen_begin_dis_flg = 0;
}

void setup()
{
    Serial.begin(115200);           //波特率
    // Erstmal etwas warten um ggf. den seriellen Port überhaupt erwischen zu können ... 
    delay(5000);

    Serial.println(" ____  __.                     .__  _________ ________     ");    
    Serial.println("|    |/ _| ____   ____   _____ |__| \\_   ___ \\\\_____  \\    ");     
    Serial.println("|      <  /    \\ /  _ \\ /     \\|  | /    \\  \\/  _(__  <    ");     
    Serial.println("|    |  \\|   |  (  <_> )  Y Y  \\  | \\     \\____/       \\   ");     
    Serial.println("|____|__ \\___|  /\\____/|__|_|  /__|  \\______  /______  /   ");     
    Serial.println("        \\/    \\/             \\/             \\/       \\/    ");     
    Serial.println("________            ____  __.__  .__                       ");     
    Serial.println("\\______ \\_______   |    |/ _|  | |__|_____ ______   ___________  ");
    Serial.println(" |    |  \\_  __ \\  |      < |  | |  \\____ \\\\____ \\_/ __ \\_  __ \\ ");
    Serial.println(" |____|   \\  | \\/  |    |  \\|  |_|  |  |_> >  |_> >  ___/|  | \\/ ");
    Serial.println("/_______  /__| /\\  |____|__ \\____/__|   __/|   __/ \\___  >__|    "); 
    Serial.println("        \\/     \\/          \\/       |__|   |__|        \\/        ");

    // 1024 hat Probleme gemacht ? 
    if (!EEPROM.begin(1000)) {
      Serial.println("Failed to initialise EEPROM");
    }
    //deletewificonfig();

    delay(100);
  //  readwificonfig(); //将wifi账号读出，判断是否进入配网界面

  //  if(wificonf.apmodeflag[0] != '8') 
  //  {   //直接进入配网
  //      wifi_ap_config_flg = 1;
  //  }

    connectToWiFi(15);

    // Serial.printf("SSID:%s\r\n",wificonf.stassid);
    
    Serial.println("Init Key interface");
    InitKeyInterface();             // Keypad Interface Initialization
    
    Serial.println("Init lv Display");
    lv_display_Init();              // Display initialization

    Serial.println("Init Display Object");
    Display_Object_Init();          // Initialize all display objects once

    Serial.println("Init Open Display");
    Open_display_init();
    
    // Don´t show SplashScreen
    screen_begin_dis_flg = 1;

    Serial.println("Init LV Display LED");
    lv_display_led_Init();              // Backlighting later.  

    timer1.attach(0.001, timer1_cb);    // Timer 0.001s, i.e. 1ms, the callback function is timer1_cb and starts the timer 
    timer2.attach(0.1,   timer2_cb); 

  //  if(wifi_ap_config_flg == 1){
  //    wifiConfig();                     // Commencement of network distribution functions
   // }

 /*   while(true) {
      Serial.print("Free Mem : ");
      Serial.println(ESP.getFreeHeap());
      delay(2000);
    } */
}     

void loop() 
{
  // lv_tick_inc(1);/* le the GUI do its work */ 
  lv_task_handler();  
  
  //----------------Test mode, search for online networks------------------//
 /* if(test_mode_flag==1){

    screen_begin_dis_flg = 0;

    delay(100);
    update_blue_back_display();
    lv_task_handler(); 
    delay(4000);
    lv_obj_del(img_blue_test);

    delay(100);
    init_gif_White_back_display();
    update_label_scan_networks_test();
    lv_task_handler(); 
    delay(100);

    wifiConfig_test();
    delay(100);

    update_label_networksID_test();

    while(1){
      lv_task_handler();  
      delay(10);
    }
  }

  */

  if((screen_begin_dis_flg==1) && (test_mode_flag==0))
  {
    //-------------HTTP request-----------------------//
    httprequest_nowtime = millis();
    if (httprequest_nowtime > httprequest_nexttime) {
      if ((WiFi.status() == WL_CONNECTED) && (KeyDownFlag != KEY_DWON) && (start_http_request_flg == 1)) {    // wifi has been connected successfully, send http request to get data
          //Serial.println("Get Data");
          HTTPClient http; 
          
          wifi_ap_config_flg = 0;         // Verbunden mit Wifi

          if(First_connection_flg == 0){  // Verbinden Sie sich mit dem WLAN und schalten Sie zurück zur normalen Anzeige.
            actState = "Standby ";
            timer_contne = 0;
            display_step = 2; 
            First_connection_flg = 1;  
          }

          if(httpswitch==1){
            http.begin("http://"+klipper_ip+"/api/printer");                                            // Ermitteln der Temperatur
          }else if(httpswitch==2){
            http.begin("http://"+klipper_ip+"/printer/objects/query?display_status");                   // Drucken lassen
          }else if(httpswitch==3){
            //http.begin("http://"+klipper_ip+"/printer/objects/query?gcode_macro%20G28");                // Status der Home abrufen
            http.begin("http://"+klipper_ip+"/printer/objects/query?gcode_macro%20HomeSetVar");                // Status der Home abrufen
          }else if(httpswitch==4){
            //http.begin("http://"+klipper_ip+"/printer/objects/query?gcode_macro%20BED_MESH_CALIBRATE"); // Nivellierungsstatus abrufen
            http.begin("http://"+klipper_ip+"/printer/objects/query?gcode_macro%20BedLevelVar"); // Nivellierungsstatus abrufen
          }else{

          }
  
          int httpCode = http.GET();                                        //Make the request
      
          if (httpCode == 200) { // Check for the returning code  // Fehler > 0
              //Serial.println("HTTP Decode ...");
              //Serial.print  ("HTTP > Code    : ");
              //Serial.println(httpCode);
              screen_no_klipper_dis_flg = 0;

              String payload = http.getString();
              //Serial.println("HTTP > Payload :");
              //Serial.println(payload);
              DynamicJsonDocument doc(payload.length()*2);
              deserializeJson(doc, payload);

              if(httpswitch==1){  // Nozzle hot bed temperature display

                  String nameStr1 = doc["temperature"]["bed"]["actual"].as<String>();
                  String nameStr2 = doc["temperature"]["bed"]["target"].as<String>();
                  String nameStr3 = doc["temperature"]["tool0"]["actual"].as<String>();
                  String nameStr4 = doc["temperature"]["tool0"]["target"].as<String>();
                  String nameStr5 = doc["state"]["flags"]["printing"].as<String>();
                  String nameStr6 = doc["state"]["flags"]["paused"].as<String>();

                  bedtemp_actual = (uint16_t)((doc["temperature"]["bed"]["actual"].as<double>())*100);
                  bedtemp_target = (uint16_t)((doc["temperature"]["bed"]["target"].as<double>())*100);
                  tooltemp_actual = (uint16_t)((doc["temperature"]["tool0"]["actual"].as<double>())*100);
                  tooltemp_target = (uint16_t)((doc["temperature"]["tool0"]["target"].as<double>())*100);

                  text_ext_actual_temp = nameStr3+"°C";
                  text_ext_target_temp = nameStr4+"°C";
                  text_bed_actual_temp = nameStr1+"°C";
                  text_bed_target_temp = nameStr2+"°C";

                  if(nameStr5 == "true"){
                      text_print_status = "Printing";
                      print_status = 1;               
                  }else{
                    if(nameStr6 == "true"){
                      actState = "Paused  ";
                      text_print_status = "paused"; 
                      print_status = 2;      
                    }else{
                      actState = "Standby ";
                      text_print_status = "standby";
                      print_status = 0;  
                    }
                  }    

                  // Print Status ist Standby
                  // Logic Change: 
                  // Wenn der Drucker startet ist er schon im Priting Modus und nicht im Standby !
                  //if(print_status == 0){
                  if(print_status == 1){ // || print_status == 0){
                      // Wechsel von 0 auf x°C beim Bett Target und Bett Target nicht 0 
                      if((bedtemp_target > last_bedtemp_target) && (bedtemp_target != 0))    // Druckbett vorheizen
                      {
                          actState = "Heat Bed";
                          timer_contne = 0;
                          display_step = 3;
                      }

                      // Wechsel von 0 auf x°C beim Extruder Target und Extruder Target nicht 0 
                      if((tooltemp_target > last_tooltemp_target)&&(tooltemp_target != 0))  // Extruder vorheizen
                      {
                          actState = "Heat Ext";
                          timer_contne = 0;
                          display_step = 4;
                      }

                      //if (print_status == 0 && bedtemp_target == 0 && tooltemp_target == 0 && progress_data < 100) {
                      //  actState = "Standby ";
                      //  timer_contne = 0;
                      //  display_step = 2; 
                      //}
                  }

                  last_bedtemp_target = bedtemp_target;
                  last_tooltemp_target = tooltemp_target;

                  httpswitch = 2;
              }else if(httpswitch == 2){   // Printing Progress

                  double nameStr7= (doc["result"]["status"]["display_status"]["progress"].as<double>())*1000;
                  uint16_t datas = (uint16_t)(nameStr7);
                  uint16_t datas1 = datas%10;

                  if(datas1 > 4){
                      datas = (datas + 10) / 10;
                  }else{
                      datas = datas / 10;
                  }

                  progress_data = datas;
                  if(datas == 0){
                    nameStrpriting = "0";
                  }else{
                    nameStrpriting = to_String(datas);
                    actState = "Printing";
                  }
                  
                  httpswitch = 3;
              }else if(httpswitch == 3){   // home state

                  //String nameStr8 = doc["result"]["status"]["gcode_macro G28"]["homing"].as<String>();
                  String nameStr8 = doc["result"]["status"]["gcode_macro HomeSetVar"]["homing"].as<String>();

                  if(nameStr8 == "true"){
                      actState = "Homing  ";
                      homing_status = 1; 
                      display_step = 12;  // Faster access to the display 
                      //timer_contne = 0;         
                  }else{
                      homing_status = 0;  
                  }

                  httpswitch = 4;
              }else if(httpswitch == 4){   // levelling status

                  String nameStr9 = doc["result"]["status"]["gcode_macro BedLevelVar"]["leveling"].as<String>();

                  if(nameStr9 == "true"){
                      actState = "Leveling";
                      levelling_status = 1;    
                      display_step = 13;  // Faster access to the display  
                      //timer_contne = 0;                   
                  }else{
                      levelling_status = 0; 
                  }

                  httpswitch = 1;
              }else{

              }

            }
          else {

            if(screen_no_klipper_dis_flg < 10)screen_no_klipper_dis_flg ++;

            Serial.println("Error on HTTP request");

            if(screen_no_klipper_dis_flg >3){
              actState = "No Cnect";
              display_step = 8;                             //no klipper connect       
              timer_contne = 0;
            }
          }
          http.end(); //Free the resources
        }

        httprequest_nexttime = httprequest_nowtime + 200UL; //97UL;
      }

      keyscan_nowtime = millis();

      if (1==1){
        if (keyscan_nowtime > debug_nexttime) {
          // Serial output for DEBUGGING
          Serial.print("M: "); Serial.print(actState);
          Serial.print(" D: "); if (display_step < 10) {Serial.print(" ");} Serial.print(display_step);
          Serial.print(" T: "); Serial.print(timer_contne);
          Serial.print(" %: "); Serial.print(progress_data);
          Serial.print(" S: "); 
          switch (print_status) { case 0: Serial.print("Stby"); break;
                                  case 1: Serial.print("Prnt"); break;
                                  case 2: Serial.print("Paus"); break; }
          Serial.print(" H: "); Serial.print(homing_status);
          Serial.print(" L: "); Serial.print(levelling_status);
    
          Serial.print(" | B a : "); Serial.print(bedtemp_actual);
          Serial.print(" l : "); Serial.print(last_bedtemp_target);
          Serial.print(" t : "); Serial.print(bedtemp_target);
          Serial.print(" | E a : "); Serial.print(tooltemp_actual);
          Serial.print(" l : "); Serial.print(last_tooltemp_target);
          Serial.print(" t : "); Serial.println(tooltemp_target);
          debug_nexttime = keyscan_nowtime + 500;
        }
      }
      
      bool testing = false;
      if (keyscan_nowtime > keyscan_nexttime && testing){
          Serial.print("TESTING : "); Serial.println(keyscan_nexttime);
          delete_exist_object();
          update_timer = lv_timer_create(update_screen14, 0, NULL);
          lv_timer_set_repeat_count(update_timer,1);

          // print_status = 0     -> Standby
          // print_status = 1     -> Printing
          // print_status = 2     -> Paused
          
          // update_screen1  -> Prozentanzeige / Druckanzeige             Print Progress
          // update_screen2  -> Weißer Bildschirm ???
          // update_screen3  -> Text in Weiß/Blau "No Printfile"
          // update_screen4  -> Text in Blau "AP Config..."
          // update_screen5  -> Text in Blau "No klipper connect"
          // update_screen6  -> fan Speed: xx% mit Balken
          // update_screen7  -> Gesicht mit Schlitzaugen                  Standby
          // update_screen8  -> Blinkendes Rechtes Auge mit Gesicht       Ready to Print
          // update_screen9  -> Defektes Gesicht mit weißem Balken 
          // update_screen10 -> Spiralaugen die sich drehen 
          // update_screen11 -> Tempanzeige (Welche ??)                   Heizbett Temp
          // update_screen12 -> Nozzel Temp Anzeige                       Extruder Temp
          // update_screen13 -> Weißer Bildschirm ???
          // update_screen14 -> Grüner Bogen Links Oben                   Print Complete
          // update_screen15 -> Voron Logo
          // update_screen16 -> Weißer Bildschirm ???
          // update_screen17 -> Weißer Bildschirm ???
          // update_screen18 -> Gesicht mit großen Augen                  ????
          // update_screen19 -> Spiralaugen die sich drehen 
          // update_screen20 -> Hello -> WLAN Verbindung und Konfig       Connect Konfig 
          // update_screen21 -> Blinkendes Haus -> Homing                 Homing
          // update_screen22 -> Bed Leveling                              Bed Leveling
          // update_screen23 -> Knomi has Lost Touch with the Printer     Connect Verloren
          
          keyscan_nexttime = keyscan_nowtime + 4000;
      }
      
      if (keyscan_nowtime > keyscan_nexttime && !testing) {

        if(timer_contne > 0) timer_contne--;  // Display Timing
        
        if((wifi_ap_config_flg == 0) && (test_mode_flag == 0))
        {
          if((display_step == 2)&&(timer_contne==0)){    // Standby
            timer_contne = 15;  // 5

            if(homing_status == 1){
              timer_contne = 5;
              display_step = 12;
            }else if(levelling_status == 1){
              timer_contne = 5;
              display_step = 13;              
            }
            else if(print_status == 1){
              timer_contne = 5;
              display_step = 3;
          //    standby_voron_dis_flg = 0;
            }else{
            //    if(standby_voron_dis_flg==0){

            //      standby_voron_dis_flg = 1;

                  delete_exist_object();
                  //update_timer = lv_timer_create(update_screen14, 0, NULL);         // TEST !
                  update_timer = lv_timer_create(update_screen7, 0, NULL);  // Standby Augen
                  lv_timer_set_repeat_count(update_timer,1);
           //     }
                //else{
                //    display_step = 11;     //to voron
                //}
            }
          }

   /*       if((display_step == 11)&&(timer_contne==0)){   // Voron Logo
            timer_contne = 5;

            if(homing_status == 1){
              timer_contne = 5;
              display_step = 12;
            }else if(levelling_status == 1){
              timer_contne = 5;
              display_step = 13;
            }else if(print_status == 1){
              timer_contne = 5;
              display_step = 3;
              standby_voron_dis_flg = 0;
            }
            else{
                if(standby_voron_dis_flg==1){

                    standby_voron_dis_flg = 0;

                    delete_exist_object();
                    update_timer = lv_timer_create(update_screen15, 0, NULL);
                    lv_timer_set_repeat_count(update_timer,1);                      
                }else{
                    display_step = 2;    //to Standby
                }
            }
          }
  */
          if((display_step == 12)&&(timer_contne==0)){   // homing
            timer_contne = 15; //5

            if(homing_status==0){
              display_step = 2;
              timer_contne = 1;
            }else{
              delete_exist_object();
              update_timer = lv_timer_create(update_screen21, 0, NULL);   
              lv_timer_set_repeat_count(update_timer,1);
            }
          }

          if((display_step == 13)&&(timer_contne==0)){   // levelling
            timer_contne = 15; //5;

            if(levelling_status==0){
              display_step = 2;
            }else{
              delete_exist_object();
              update_timer = lv_timer_create(update_screen22, 0, NULL);   
              lv_timer_set_repeat_count(update_timer,1);
            }
          }

          if((display_step == 3)&&(timer_contne==0)){    // Bed Heat
            timer_contne = 15;  // 5

            if((bedtemp_actual >= bedtemp_target)&&(bedtemp_target != 0)){
              display_step = 4;
            }else{

              if(bedtemp_target == 0){
                display_step = 4;
              }else{
                delete_exist_object();
                update_timer = lv_timer_create(update_screen11, 0, NULL);   // BED aufheizen
                lv_timer_set_repeat_count(update_timer,1);
              }
            }
          }

          if((display_step == 4)&&(timer_contne==0)){    // Before Printing // Extruder Heat
            timer_contne = 15;  // 5

            if((tooltemp_actual >= tooltemp_target)&&(tooltemp_target != 0)){

              if(print_status == 0){
                display_step = 2;
              }else{
                display_step = 9;
                delete_exist_object();
                update_timer = lv_timer_create(update_screen18, 0, NULL);   // BeforePrinting 
                lv_timer_set_repeat_count(update_timer,1);  
              }
            
            }else{

              if(tooltemp_target == 0)
              {
                display_step = 9;
              }else{
                delete_exist_object();
                update_timer = lv_timer_create(update_screen12, 0, NULL);   // EXT 
                lv_timer_set_repeat_count(update_timer,1);
              }

            }
          }

          if((display_step == 9)&&(timer_contne==0)){    // ???
            timer_contne = 3; //1;
            display_step = 5;
          }

          if((display_step == 5)&&(timer_contne==0)){    // Printing 
            timer_contne = 15;  // 5

            if ((print_status == 1 || print_status == 0) && progress_data == 100){
              display_step = 10; //6;
              timer_contne = 300;
              delete_exist_object();
              update_timer = lv_timer_create(update_screen14, 0, NULL);         // print_ok
              lv_timer_set_repeat_count(update_timer,1);
            }

            if(print_status == 1){
               // if(progress_data == 100){
               //   display_step = 6;
               //   timer_contne = 7;
               //   delete_exist_object();
               //   update_timer = lv_timer_create(update_screen14, 0, NULL);         // print_ok
               //   lv_timer_set_repeat_count(update_timer,1);
               // }else{
                  if (progress_data < 100) {    
                    if(progress_data >= 1){
                        delete_exist_object();
                        update_timer = lv_timer_create(update_screen1, 0, NULL);    // Fortschritte jenseits der 1-Prozent-Marke
                        lv_timer_set_repeat_count(update_timer,1);
                    }else{
                        delete_exist_object();
                        update_timer = lv_timer_create(update_screen9, 0, NULL);    // printing
                        lv_timer_set_repeat_count(update_timer,1);                      
                    }
                }
            }
            // TBD ... Rücksprung nach Standby nach x Sekunden
            //else{
            //    display_step = 2;
            //}

          }

         // TBD raus ... Drehende Augen
         //  
         //  if((display_step == 6)&&(timer_contne==0)){    // AfterPrinting
         //  timer_contne = 5;
         //   display_step = 10;
//
         //   delete_exist_object();
         //   update_timer = lv_timer_create(update_screen19, 0, NULL);   
         //   lv_timer_set_repeat_count(update_timer,1);
         // }

          if((display_step == 10)&&(timer_contne==0)){   // Standby ???
            timer_contne = 15;  // 5
            display_step = 2;
          }

          if((display_step == 8)&&(timer_contne==0)){    // no klipper connect
            timer_contne = 2;

            if(screen_no_klipper_dis_flg==0){
              display_step = 2;
            }else{
              delete_exist_object();
              update_timer = lv_timer_create(update_screen23, 0, NULL);   
              lv_timer_set_repeat_count(update_timer,1);
            }
          }
      
        }

        start_http_request_flg = 1;             // Nach der Verbindung mit dem WLAN starten Sie die http-Anfrage.
        keyscan_nexttime = keyscan_nowtime + 400;                                               
      }
  }

  //----------------Network connectivity check, AP hotspot mapping------------------//
  netcheck_nowtime = millis();
  if (netcheck_nowtime > netcheck_nexttime && !wifi_ap_config_flg) {
      // Debugging RAM Usage
      Serial.print("Free Mem : ");
      Serial.println(ESP.getFreeHeap());
      // Serial.println("Check Connection");
      checkConnect(true);                       // Der Parameter true bedeutet, dass die Verbindung wiederhergestellt wird, wenn die Verbindung getrennt wurde.

      if (WiFi.status() != WL_CONNECTED) {      // Die Wifi-Verbindung wurde nicht erfolgreich hergestellt
          checkDNS_HTTP();                      // Erkennung von DNS- und HTTP-Anfragen von Clients, d. h. Überprüfung der Seite, die den Anforderungen entspricht
          First_connection_flg = 0;
        } 
      netcheck_nexttime = netcheck_nowtime + 1000UL; //TBD100UL;     
    } 

}
 
