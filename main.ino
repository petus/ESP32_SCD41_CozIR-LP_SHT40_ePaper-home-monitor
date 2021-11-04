/*
* ESP32 MiniKit LP, CO2 CozIR-LP and SCD40 sensors and SHT40
* ePaper 4.2". Send data to The IoT Guru Cloud

* https://github.com/Sensirion/arduino-i2c-scd4x
* https://github.com/adafruit/Adafruit_SHT4X
* https://github.com/RobTillaart/Cozir/
* https://github.com/ZinggJM/GxEPD
* https://github.com/madhephaestus/ESP32AnalogRead // Analog read with calibration data of ADC

* chiptron.cz
*
* SPI = MOSI (23), CLK (18), CS (5), DC (26), RST (15), BUSY (13)
* I2C =  SCL (22), SDA (21)
* VBAT = IO36
* LED = IO2
* UART2 = RX (16), TX (17)
*/

#include <WiFi.h>

#include <GxEPD.h>
// select the display class to use, only one, copy from GxEPD_Example
#include <GxGDEW042Z15/GxGDEW042Z15.h>    // 4.2" b/w/r
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

#include "Adafruit_SHT4x.h" // SHT40
#include <IoTGuru.h>  // The IoT Guru Cloud
#include "cozir.h"  // COzIR-LP
#include <Wire.h>   // I2C
#include <SensirionI2CScd4x.h>  // SCD41

#include "OpenSansSB_12px.h"
#include "OpenSansSB_14px.h"
#include "OpenSansSB_16px.h"
#include "OpenSansSB_24px.h"
#include "OpenSansSB_32px.h"
#include "OpenSansSB_40px.h"
#include "OpenSansSB_80px.h"

#include <ESP32AnalogRead.h>

/* ePaper */
GxIO_Class io(SPI, /*CS=5*/ SS, /*DC=*/ 26, /*RST=*/ 15);
GxEPD_Class display(io, /*RST=*/ 15, /*BUSY=*/ 13);
/* SHT40 */
Adafruit_SHT4x sht4 = Adafruit_SHT4x();
/* CozIR-LP */
COZIR czr(&Serial2); // dedicate Serial2 for CozIR-LP
/* SCD41 */
SensirionI2CScd4x scd4x;

/*  CozIR-LP  */
uint32_t co2 = 0;
/*  SHT40  */
sensors_event_t hum, temp;
/* SCD40 */
uint16_t co2SCD;
float temperatureSCD;
float humiditySCD;

/* ADC */
ESP32AnalogRead adc;
const int adcPin = 36;
float vBat = 0.0;

/*  Wi-Fi settings */
const char* ssid     = "SSID";
const char* password = "PASSWORD";
int count = 0;

/*  IoTguru.cloud settings  */
String userShortId    = "userShortId";
String deviceShortId  = "deviceShortId";
String deviceKey      = "deviceKey";
IoTGuru iotGuru = IoTGuru(userShortId, deviceShortId, deviceKey);

String nodeKey        = "nodeKey";

void setup() {
  /* LED*/
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);

  /* Button */
  pinMode(0, INPUT);

  /*Serial */
  Serial.begin(115200); // Debug serial
  Serial2.begin(9600, SERIAL_8N1, 16, 17); // Serial2 for CozIR-LP

  /* vBat */
  adc.attach(adcPin); // read ADC
  vBat = float((adc.readVoltage()*(100+620))/100); // convert ADC to voltage
  Serial.print("Voltage: "); Serial.println(vBat);

  /* I2C */
  Wire.begin();

  /* SCD 40 */
  scd4x.begin(Wire);

  // start measurement
  scd4x.startPeriodicMeasurement();

  /* CozIR-LP init*/
  czr.init(); 

  /* SHT40 */
  sht4.begin();
  sht4.setPrecision(SHT4X_MED_PRECISION);

  //delay after initialization
  delay(5000);
  
  /* SCD 40 */
  scd4x.readMeasurement(co2SCD, temperatureSCD, humiditySCD);
  Serial.print("CO2 (SCD40) : "); Serial.println(co2SCD);
  Serial.print("Temp (SCD40) : "); Serial.println(temperatureSCD);
  Serial.print("Hum (SCD40) : "); Serial.println(humiditySCD);

  // stop measurement
  scd4x.stopPeriodicMeasurement();

  /*  CozIR-LP  */
  Serial2.flush();
  delay(100);
  co2 = czr.CO2();
  
  delay(200);

  /* CozIR-LP */
  Serial.print("CO2 (CozIR-LP) : "); Serial.println(co2);
  
  /* SHT40 */
  sht4.getEvent(&hum, &temp);
  Serial.print("Temperature: "); Serial.print(temp.temperature); Serial.println(" degrees C");
  Serial.print("Humidity: "); Serial.print(hum.relative_humidity); Serial.println("% rH");


  /* ePaper 4.2" */
  display.init();
  display.setRotation(0);
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&OpenSansSB_16px);

  display.setCursor(50, 20);
  display.println("CO2 (SCD): ");

  display.setCursor(250, 20);
  display.println("CO2 (CozIR): ");

  display.setCursor(5, 100);
  display.setFont(&OpenSansSB_80px);
  display.print(co2SCD); 
  display.setFont(&OpenSansSB_24px);
  //display.println(" ppm");

  display.setCursor(210, 100);
  display.setFont(&OpenSansSB_80px);
  display.print(co2); 
  display.setFont(&OpenSansSB_24px);
  //display.println(" ppm");

  display.setCursor(5, 180);
  display.setFont(&OpenSansSB_80px);
  display.print(temperatureSCD, 1); 
  display.setFont(&OpenSansSB_24px);
  //display.println(" 'C");

  display.setCursor(5, 260);
  display.setFont(&OpenSansSB_80px);
  display.print(humiditySCD, 1); 
  display.setFont(&OpenSansSB_24px);
  //display.println(" 'C");

  display.setCursor(210, 180);
  display.setFont(&OpenSansSB_80px);
  display.print(temp.temperature, 1); 
  display.setFont(&OpenSansSB_24px);
  //display.println(" 'C");

  display.setCursor(210, 260);
  display.setFont(&OpenSansSB_80px);
  display.print(hum.relative_humidity); 
  display.setFont(&OpenSansSB_24px);
  //display.println(" %");

  display.setFont(&OpenSansSB_24px);
  if(vBat < 3.7)
  {
    display.setCursor(0, 290);
    display.print("NIZKY STAV AKUMULATORU "); // warning, low voltage
    display.setCursor(330, 300);
    display.print(vBat); 
    //display.println(" V");
  }
  else
  {
    display.setCursor(330, 300);
    display.print(vBat); 
    //display.println(" V");
  }

  delay(100);
  display.update();

  /* Wi-Fi */
  Serial.println("The IoT guru cloud ");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    count++;
    if(count >= 10) // not able to connect, go to deep-sleep
    {
      Serial.flush();
      Serial2.flush();
      esp_sleep_enable_timer_wakeup(900*1000000);
      esp_deep_sleep_start();
    }
  }

  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());
  
  //Set check in duration, the default is 60000 milliseconds.
  iotGuru.setCheckDuration(60000);
  
  //Set the debug printer.
  iotGuru.setDebugPrinter(&Serial);
  
  iotGuru.check();

  /*  Send value to IoTGuru.cloud */
  // fill the fields in IoT Guru
  iotGuru.sendHttpValue(nodeKey, "co2", co2);
  iotGuru.sendHttpValue(nodeKey, "temperature", temp.temperature);
  iotGuru.sendHttpValue(nodeKey, "humidity", hum.relative_humidity);
  iotGuru.sendHttpValue(nodeKey, "vbat", vBat);
  iotGuru.sendHttpValue(nodeKey, "co2scd", co2SCD);
  iotGuru.sendHttpValue(nodeKey, "temperaturescd", temperatureSCD);
  iotGuru.sendHttpValue(nodeKey, "humidityscd", humiditySCD);

  /* Go to sleep */
  Serial.println("Go to sleep");
  digitalWrite(2, HIGH);
  
  Serial.flush();
  Serial2.flush();
  delay(100);
  esp_sleep_enable_timer_wakeup(900 * 1000000); // wakeup timer
  esp_deep_sleep_start();

}

void loop() {
  // put your main code here, to run repeatedly:

}
