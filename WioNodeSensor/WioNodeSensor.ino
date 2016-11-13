
#include <ESP8266WiFi.h>
#include <Wire.h>
#include "HttpClient.h"
#include "netconfig.h"

// Wio Node Ports
const uint8_t FUNC_BTN = 0;
const uint8_t BLUE_LED = 2;

const uint8_t PORT0A = 1;
const uint8_t PORT0B = 3;
const uint8_t PORT1A = 4;
const uint8_t PORT1B = 5;
const uint8_t PORT_POWER = 15; // (common with RED_LED)

const uint8_t UART_TX = PORT0A;
const uint8_t UART_RX = PORT0B;
const uint8_t I2C_SDA = PORT1A;
const uint8_t I2C_SCL = PORT1B;

// HDC1000
const int HDC1000_ADDR = 0x40;
const uint8_t HDC1000_TEMPERATURE_POINTER     = 0x00;
const uint8_t HDC1000_HUMIDITY_POINTER        = 0x01;
const uint8_t HDC1000_CONFIGURATION_POINTER   = 0x02;
const uint8_t HDC1000_SERIAL_ID1_POINTER      = 0xfb;
const uint8_t HDC1000_SERIAL_ID2_POINTER      = 0xfc;
const uint8_t HDC1000_SERIAL_ID3_POINTER      = 0xfd;
const uint8_t HDC1000_MANUFACTURER_ID_POINTER = 0xfe;

struct Stat {
  float temp;
  float humid;
};

extern "C" {
#include "user_interface.h"
}

void setup() {
  pinMode(FUNC_BTN, INPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(PORT_POWER, OUTPUT);
  digitalWrite(PORT_POWER, HIGH);
  Serial.begin(115200);
  Wire.begin(I2C_SDA,I2C_SCL) ;

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  delay(15); // sensor wait.

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(WiFi.localIP());

  //wifi_set_sleep_type(LIGHT_SLEEP_T);

  if ((ESP.getResetInfoPtr())->reason != REASON_DEEP_SLEEP_AWAKE) {
    digitalWrite(BLUE_LED, HIGH);
    Serial.println(getManufacturerId(), HEX);
  }

  sendStat();

  ESP.deepSleep(300 * 1000 * 1000 - micros() , WAKE_RF_DEFAULT);
  delay(1);

}

void sendStat() {
  //digitalWrite(BLUE_LED, HIGH);

  auto stat = getStat();
  Serial.print("Temperature = ");
  Serial.print(stat.temp);
  Serial.print(" degree, Humidity = ");
  Serial.print(stat.humid);
  Serial.print("%");
  HttpClient client;
  client.setHeader("Content-Type","application/x-www-form-urlencoded");
  client.post(API_URL, String("_secret=") + API_SECRET + "&temp=" + String(stat.temp) + "&humid="+String(stat.humid));
  Serial.println(".");

  //digitalWrite(BLUE_LED, LOW);
}

void loop()
{
  delay(10);
  if (digitalRead(FUNC_BTN) == LOW) {
    Serial.write("b");
  }
}

uint16_t getManufacturerId() { 
  Wire.beginTransmission(HDC1000_ADDR);
  Wire.write(HDC1000_MANUFACTURER_ID_POINTER);
  Wire.endTransmission();
 
  Wire.requestFrom(HDC1000_ADDR, 2);
  while (Wire.available() < 2) {
  }

  uint16_t h = Wire.read() << 8;
  return h | Wire.read();
}

Stat getStat() {
  Wire.beginTransmission(HDC1000_ADDR);
  Wire.write(HDC1000_TEMPERATURE_POINTER);
  Wire.endTransmission();

  delay(15); // Wait ADC.;

  Wire.requestFrom(HDC1000_ADDR, 4);
  while (Wire.available() < 4) {}

  Stat s;
  uint16_t temp, humid;
  temp = Wire.read() << 8;
  temp |= Wire.read();
  s.temp = (float)temp / 65536 * 165 - 40; // -40 - 125(C)
 
  humid = Wire.read() << 8;
  humid |= Wire.read();
  s.humid = (float)humid / 65536 * 100; // 0-100(%)
  return s;
}

