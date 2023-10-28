#ifndef __WIFIUSER_H__
#define __WIFIUSER_H__
 
#include <WiFi.h>
#include <ESPmDNS.h>      //用于设备域名 MDNS.begin("esp32")
#include <esp_wifi.h>     //用于esp_wifi_restore() 删除保存的wifi信息

extern const char* HOST_NAME;                 //设置设备名
extern int connectTimeOut_s;                 //WiFi连接超时时间，单位秒
extern String klipper_ip;                     //暂时存储KlipperIP
extern String wifi_ssid;                     //暂时存储wifi账号密码
extern int connectTimeOut_s;                 //WiFi连接超时时间，单位秒

void checkConnect(bool reConnect);    //检测wifi是否已经连接
void connectToWiFi(int timeOut_s);    //连接WiFi
 
#endif
