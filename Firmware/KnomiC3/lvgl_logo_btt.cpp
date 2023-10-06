#include "lvgl_logo_btt.h"
#include <Arduino.h>
#include <lvgl.h>
#include <WiFi.h>
#include <LovyanGFX.h>
#include "WiFiUser.h"

LV_IMG_DECLARE(BTT_LOGO);

// Boot Screen Object Definition
lv_obj_t * img_open_logo;


void init_img_open_logo_display()
{
	//lv_disp_set_bg_color(NULL, lv_color_black());
  
   img_open_logo = lv_img_create(lv_scr_act());
   lv_img_set_src(img_open_logo, &BTT_LOGO);
   lv_obj_align(img_open_logo,LV_ALIGN_CENTER,0,0);
}
