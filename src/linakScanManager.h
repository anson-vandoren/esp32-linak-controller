#include <BLEDevice.h>
#include <util.h>
#include <Arduino.h>
#include <ArduinoJson.h>

class AdvertisedDeviceCB : public BLEAdvertisedDeviceCallbacks
{
public:
    void onResult(BLEAdvertisedDevice device);
};

class LinakScanManager
{
public:
    void init();
    std::map<std::string, std::string> scan();

    bool isScanning = false;
private:
    BLEScan* pBLEScan = nullptr;
    std::map<std::string, std::string> dpgDevices;
};