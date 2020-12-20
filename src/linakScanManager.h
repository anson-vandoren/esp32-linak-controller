#include <BLEDevice.h>
#include <util.h>
#include <Arduino.h>
#include <ArduinoJson.h>

class AdvDevCB : public BLEAdvertisedDeviceCallbacks
{
    AdvDevCB();
public:
    void onResult(BLEAdvertisedDevice device);

private:
    friend class LinakScanManager;
    std::map<std::string, std::string> *dpgDevices;
};

class LinakScanManager
{
public:
    void init();
    std::map<std::string, std::string> scan();

    bool isScanning = false;

private:
    BLEScan *pBLEScan = nullptr;
    std::map<std::string, std::string> dpgDevices;
};