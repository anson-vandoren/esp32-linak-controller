#include <linakScanManager.h>
#include <util.h>
#include <ArduinoJson.h>

#define CONTROL_SERVICE "99FA0001-338A-1024-8A49-009C0215F78A"
class AdvDevCB : public BLEAdvertisedDeviceCallbacks
{
    AdvDevCB();
    void onResult(BLEAdvertisedDevice device)
    {
        bool isDPG = device.isAdvertisingService(BLEUUID(CONTROL_SERVICE));
        std::string name = device.getName();
        std::string addr = device.getAddress().toString();

        bool known = dpgDevices->find(addr) != dpgDevices->end();

        if (!isDPG && !known){
            // not advertising DPG and not already known as a DPG - don't add or update
            return;
        }

        if (name.empty() && known){
            // no name and already known - don't update
            return;
        } 

        if (known)
        {
            // we already know about this device. Check if name should be updated
            std::string oldName = dpgDevices->at(addr);
            if (!name.empty() && oldName != name)
            {
                dpgDevices->at(addr) = name;
                Serial.printf("[%s] updated name from '%s' to '%s'\n", addr.c_str(), oldName.c_str(), name.c_str());
            }
        }
        else
        {
            // we don't know about this DPG device yet, so add it
            dpgDevices->insert(std::pair<std::string, std::string>(addr, name));
            Serial.printf("%s [%s] advertised DPG CONTROL service\n", name.c_str(), addr.c_str());
        }
    }

private:
    friend class LinakScanManager;
    std::map<std::string, std::string> *dpgDevices;
};

AdvDevCB::AdvDevCB()
{
    dpgDevices = nullptr;
}

void LinakScanManager::init()
{
    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(40);
    pBLEScan->setWindow(30);
    Serial.println("LinakScanManager initialized");
    AdvDevCB *cb = new AdvDevCB();
    cb->dpgDevices = &dpgDevices;
    pBLEScan->setAdvertisedDeviceCallbacks(cb, true); // accept duplicates because name may be updated
    isScanning = false;
}

std::map<std::string, std::string> LinakScanManager::scan()
{
    if (isScanning) return dpgDevices;
    dpgDevices.clear();
    int scanTime = 5;
    isScanning = true;
    pBLEScan->start(scanTime, false);
    isScanning = false;
    pBLEScan->clearResults();

    return dpgDevices;
}
