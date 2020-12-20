#include <Arduino.h>
#include <map>

#ifndef DEVICE_JSON_STRUCT
#define DEVICE_JSON_STRUCT
const size_t JSON_ENTRIES {4 + 10};

struct JsonData
{
  bool led_state {false};
  uint16_t current_value {0};
  std::map<std::string, std::string> devices {};  // can be up to 10 devices
  // time (millis) is also sent
};
#endif

void printBytes(std::string value);
void printBytes(uint8_t* strPrt, size_t length);
void bufferDeviceJson(const JsonData &deviceData, std::string &buf);