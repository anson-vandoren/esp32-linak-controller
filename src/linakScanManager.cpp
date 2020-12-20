#include <linakScanManager.h>

#define CONTROL_SERVICE "99FA0001-338A-1024-8A49-009C0215F78A"

void AdvDevCB::onResult(BLEAdvertisedDevice device)
{
    bool isDPG = device.isAdvertisingService(BLEUUID(CONTROL_SERVICE));
    std::string addr = device.getAddress().toString();

    bool known = dpgDevices->find(addr) != dpgDevices->end();

    /* not advertising DPG and not already known as a DPG - don't add or update
       since underlying BLE library seems to have problems returning both the 
       advertising data and the advertising scan results at the same time,
       we may get either the name, or the CONTROL service, but possibly not both
       in the same set of data.
    */
    if (!isDPG && !known) return;

    std::string name = device.getName();

    /* we already know about this device (and that it's a DPG device), but the
       current data doesn't have a name associated with it, so there's nothing
       useful we can get from this - just return early
    */
    if (name.empty() && known) return;

    if (known)
    {
        // we already know about this device. Check if name should be updated
        std::string oldName = dpgDevices->at(addr);
        if (!name.empty() && oldName != name)
        {
            dpgDevices->at(addr) = name;
        }
    }
    else
    {
        // we don't know about this DPG device yet, so add it
        dpgDevices->insert(std::pair<std::string, std::string>(addr, name));
    }
}

AdvDevCB::AdvDevCB()
{
    dpgDevices = nullptr;
}

void LinakScanManager::init()
{
    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(1349);
    pBLEScan->setWindow(449);
    Serial.println("LinakScanManager initialized");

    // create callback functions and give them access to our list of DPG devices
    AdvDevCB *cb = new AdvDevCB();
    cb->dpgDevices = &dpgDevices;

    pBLEScan->setAdvertisedDeviceCallbacks(cb, true); // accept duplicates because name may be updated
    isScanning = false;
}

std::map<std::string, std::string> LinakScanManager::scan()
{
    if (isScanning){
        Serial.println("Already scanning: returning empty results.");
        return std::map<std::string, std::string> {};
    }

    // New list of devices is generated each time
    dpgDevices.clear();

    int scanTime = 5;
    isScanning = true;
    pBLEScan->start(scanTime, false);
    isScanning = false;

    // don't keep the BLE cache of found devices - it won't update with additional data
    pBLEScan->clearResults();  
    return dpgDevices;
}
