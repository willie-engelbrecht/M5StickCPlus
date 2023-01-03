// Written for Grafana Labs to demonstrate how to use the M5Stick CPlus with Grafana Cloud
// 2023/01/03
// Willie Engelbrecht - willie.engelbrecht@grafana.com
// Introduction to time series: https://grafana.com/docs/grafana/latest/fundamentals/timeseries/
// All the battery API documentation: https://docs.m5stack.com/en/api/stickc/axp192_m5stickc


// ===================================================
// All the things that needs to be changed 
// Your local WiFi details
// Your Grafana Cloud details
// ===================================================
#include "config.h.home"


// ===================================================
// Includes - no need to change anything here
// ===================================================
#include <M5StickCPlus.h>
#include "M5_ENV.h"
#include <HTTPClient.h>


// ===================================================
// Global Variables
// ===================================================
SHT3X sht30;
QMP6988 qmp6988;

float tmp      = 0.0;
float hum      = 0.0;
float pressure = 0.0;

HTTPClient http_grafana;
int httpResponseCode = -1;


// ===================================================
// Connect to your local WiFi using the credentials in config.h
// ===================================================
void initWifi() {
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");
  int wifi_loop_count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");

    // Restart the device if we can't connect to WiFi after 2 minutes
    wifi_loop_count += 1;
    if (wifi_loop_count > 240) {
      ESP.restart();
    }
  }
  Serial.println();

  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());
}

// ===================================================
// Set up procedure that runs once when the controller starts
// ===================================================
void setup() {
    M5.begin();               // Init M5StickCPlus.  
    M5.Lcd.setRotation(3);    // Rotate the screen.  
    M5.lcd.fillScreen(BLACK); // Fill the screen with black background

    M5.lcd.setTextSize(2);
    M5.lcd.setCursor(10, 10);
    M5.lcd.printf("==  Grafana Labs ==");
    
    Wire.begin(32, 33);       // Wire init, adding the I2C bus.  
    qmp6988.init();           // Initiallize the pressure sensor
    initWifi();               // Connect to the local WiFi AP
}

// ===================================================
// Loop continiously until forever,
// reading from both sensors, and submitting to your
// Grafana Cloud account for visualisation
// ===================================================
void loop() {
    // Get new updated values from our sensor
    pressure = qmp6988.calcPressure();
    if (sht30.get() == 0) {     // Obtain the data of sht30.  
        tmp = sht30.cTemp;      // Store the temperature obtained from sht30.                             
        hum = sht30.humidity;   // Store the humidity obtained from the sht30.
    } else {
        tmp = 0, hum = 0;
    }

    // Update the LCD screen
    M5.lcd.fillRect(00, 40, 100, 60, BLACK);  // Fill the screen with black (to clear the screen).
    M5.lcd.setCursor(0, 40);
    M5.Lcd.printf("  Temp: %2.1f  \r\n  Humi: %2.0f%%  \r\n  Pressure:%2.0f hPa\r\n", tmp, hum, pressure / 100);

    Serial.printf("\r\n====================================\r\n");
    Serial.printf("Temp: %2.1f °C \r\nHumi: %2.0f%%  \r\nPressure:%2.0f hPa\r\n", tmp, hum, pressure / 100);

    // In case there is an issue with the WiFi, let's reboot the microcontroller to try and reconnect as a fail safe
    if (WiFi.status() != WL_CONNECTED) {
        ESP.restart();
    }
        
    // Send temp,humidity,pressure data to Grafana Cloud
    http_grafana.begin("https://" + grafana_username + ":" + grafana_password + "@" + grafana_url + "/api/v1/push/influx/write");
    String POSTtext = "m5stick temp=" + String(tmp) +",hum=" + String(hum) + ",pressure=" + String(pressure);  
    Serial.println("Sending to Grafana Cloud: " + POSTtext);
    httpResponseCode = http_grafana.POST(POSTtext);
    Serial.println("httpResponseCode: " + String(httpResponseCode));

    Serial.println("\r\n");
    int Iusb = M5.Axp.GetIdischargeData() * 0.375;
    Serial.printf("Iusbin:%da\r\n", Iusb);

    int disCharge = M5.Axp.GetIdischargeData() / 2;
    Serial.printf("disCharge:%dma\r\n", disCharge);

    double Iin = M5.Axp.GetIinData() * 0.625;
    Serial.printf("Iin:%.3fmA\r\n", Iin);

    int BatTemp = M5.Axp.GetTempData()*0.1-144.7;
    Serial.printf("Battery temperature:%d\r\n", BatTemp);

    int Vaps = M5.Axp.GetVapsData();
    Serial.printf("battery capacity :%dmW\r\n", Vaps);

    int bat = M5.Axp.GetPowerbatData()*1.1*0.5/1000;
    Serial.printf("battery power:%dmW\r\n", bat);

    int charge = M5.Axp.GetIchargeData() / 2;
    Serial.printf("icharge:%dmA\r\n", charge);

    double vbat = M5.Axp.GetVbatData() * 1.1 / 1000;
    Serial.printf("vbat:%.3fV\r\n", vbat);

    POSTtext = "m5stick Iusb=" + String(Iusb) 
            + ",disCharge=" + String(disCharge) 
            + ",Iin=" + String(Iin) 
            + ",BatTemp=" + String(BatTemp)
            + ",Vaps=" + String(Vaps)
            + ",bat=" + String(bat)
            + ",charge=" + String(charge)
            + ",vbat=" + String(vbat);  
            
    Serial.println("Sending to Grafana Cloud: " + POSTtext);
    httpResponseCode = http_grafana.POST(POSTtext);
    Serial.println("httpResponseCode: " + String(httpResponseCode));

    // Sleep for 5 seconds
    delay(5000);    
}
