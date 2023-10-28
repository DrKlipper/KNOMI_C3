#include "WiFiUser.h"
#include "lvgl_gui.h"

// WEB 
const char* HOST_NAME = "KNOMI-C3";     // Hostname
int connectTimeOut_s  = 30;             // Wifi Timeout

// WEB Config
String wifi_ssid  = "SSID";       // Wifi SSID
String wifi_pass  = "PWD";   // Wifi Password
String klipper_ip = "192.168.XXX.XXX";    // Klipper IP

void connectToWiFi(int timeOut_s)
{
  if (WiFi.status() == WL_CONNECTED) {
    // No new Connection if connection is established !
    return;
  }
    
  Serial.print("WIFI > Try Connection to network : ");
  Serial.printf("%s\r\n", wifi_ssid.c_str());
  
  WiFi.hostname(HOST_NAME);                   // Set Device Hostname
  WiFi.mode(WIFI_STA);                        // Set Wifi Station Mode
  WiFi.setAutoConnect(true);                  // Setting up automatic connections    
  
  if (wifi_ssid != "")                        
  {
    WiFi.begin(wifi_ssid.c_str(), wifi_pass.c_str()); 
  } 
 
  int Connect_time = 0;                       // Used to time the connection and reset the device if the connection is unsuccessful for a long time
  Serial.print("WIFI > ");
  while (WiFi.status() != WL_CONNECTED)       // Wait for the WIFI connection to succeed
  {  
    Serial.print(".");                        // Prints a total of 30 dots
    delay(500);
    Connect_time ++;
                                       
    if (Connect_time > 2 * timeOut_s)         // Can't connect for a long time
    { 
      wifi_connect_fail = 1;
      Serial.println("WIFI > Connect FAILED !");
      return;                                 // Jump Prevent infinite initialization
    }
  }
  
  if (WiFi.status() == WL_CONNECTED)          // If the connection is successful
  {
    Serial.println();
    Serial.println("WIFI > Connect Success");
    Serial.printf("WIFI > SSID      : %s\r\n", WiFi.SSID().c_str());
    Serial.printf("WIFI > PSW       : %s\r\n", WiFi.psk().c_str());
    Serial.print ("WIFI > LocalIP   : ");
    Serial.println(WiFi.localIP());
    Serial.print ("WIFI > GateIP    : ");
    Serial.println(WiFi.gatewayIP());

    Serial.print("WIFI > KlipperIP : ");
    Serial.println(klipper_ip);

    Serial.print("WIFI > Status    : ");
    Serial.println(WiFi.status());

    wifi_connect_ok = 1;                      // Connected to wifi, toggle display
  } else {
    Serial.println("WIFI > Connect ERROR !");
  }
}

void checkConnect(bool reConnect) 
{
  if (WiFi.status() != WL_CONNECTED)           // Check Connection state
  {
    if (reConnect == true && WiFi.getMode() != WIFI_AP && WiFi.getMode() != WIFI_AP_STA ) 
    {
      Serial.println("WIFI > Not connected.");
      Serial.print("WIFI > Mode : ");
      Serial.println(WiFi.getMode());
      Serial.println("WIFI > Try Connecting ...");
      connectToWiFi(connectTimeOut_s);          // Reconnect if needed
    }
  } 
}
