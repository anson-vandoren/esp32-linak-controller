#include <Arduino.h>
#include <ArduinoJson.h>
#include <util.h>

void printBytes(std::string value)
{
    if (value.length() == 0)
    {
        Serial.println("NULL");
        return;
    }
    for (int i = 0; i < value.length(); i++)
    {
        if (i > 0)
            Serial.print("-");
        Serial.print(String(value[i], 16));
    }
    Serial.println();
}

void printBytes(uint8_t *strPrt, size_t length)
{
    Serial.print("0x");
    for (int i=0; i < length; i++) {
        if (i>0) Serial.print("-");
        Serial.printf("%02X", strPrt[i]);
    }
    Serial.println();
    return;
}

void bufferDeviceJson(const JsonData &deviceData, std::string &buf)
{
  buf.clear();
  StaticJsonDocument<JSON_OBJECT_SIZE(JSON_ENTRIES)> doc;
  doc["led_state"] = deviceData.led_state;
  doc["current_value"] = deviceData.current_value;
  doc["time"] = millis();

  if (deviceData.devices.size() != 0)
  {
    for (auto &kv : deviceData.devices)
    {
      doc["devices"][kv.first] = kv.second;
    }
  }

  serializeJson(doc, buf);
}