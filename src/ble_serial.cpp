// Copyright 2019 Ian Archbell / oddWires
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "ble_serial.h"
#ifdef USE_NIMBLE
static const  char* LOG_TAG = "[NimBLE]";

class BLESerialServerCallbacks: public BLEServerCallbacks {
    friend class BLESerial; 
    BLESerial* bleSerial;
    
    void onConnect(NimBLEServer* pServer) {
        // do anything needed on connection
        NIMBLE_LOGI(LOG_TAG , "BLE client connected");
        delay(1000); // wait for connection to complete or messages can be lost
    };

    void onDisconnect(NimBLEServer* pServer) {
        pServer->startAdvertising(); // restart advertising
        NIMBLE_LOGI(LOG_TAG, "BLE client disconnected, started advertising");
    }
};

class BLESerialCharacteristicCallbacks: public NimBLECharacteristicCallbacks {
    friend class BLESerial; 
    BLESerial* bleSerial;
    
    void onWrite(NimBLECharacteristic *pCharacteristic) {
      bleSerial->receiveBuffer += pCharacteristic->getValue();
    }

};
#else

class BLESerialServerCallbacks: public BLEServerCallbacks {
    friend class BLESerial; 
    BLESerial* bleSerial;
    
    void onConnect(BLEServer* pServer) {
        // do anything needed on connection
        LOG_INFO("BLE client connected");
        delay(1000); // wait for connection to complete or messages can be lost
    };

    void onDisconnect(BLEServer* pServer) {
        pServer->startAdvertising(); // restart advertising
        LOG_INFO("BLE client disconnected, started advertising");
    }
};

class BLESerialCharacteristicCallbacks: public BLECharacteristicCallbacks {
    friend class BLESerial; 
    BLESerial* bleSerial;
    
    void onWrite(BLECharacteristic *pCharacteristic) {
      bleSerial->receiveBuffer += pCharacteristic->getValue();
    }

};

#endif

// Constructor

BLESerial::BLESerial()
  : pService(nullptr)
  , pTxCharacteristic(nullptr)
  , receiveBuffer("")
{
}

// Destructor

BLESerial::~BLESerial(void)
{
    // clean up
}

// Begin bluetooth serial

bool BLESerial::begin(const char* localName)
{
#ifdef USE_NIMBLE
    // Create the BLE Device
    NimBLEDevice::init(localName);

    // Create the BLE Server
    pServer = NimBLEDevice::createServer();
    if (pServer == nullptr)
        return false;
#else
    // Create the BLE Device
    BLEDevice::init(localName);

    // Create the BLE Server
    pServer = BLEDevice::createServer();
    if (pServer == nullptr)
        return false;
#endif
    
    BLESerialServerCallbacks* bleSerialServerCallbacks =  new BLESerialServerCallbacks(); 
    bleSerialServerCallbacks->bleSerial = this;      
    pServer->setCallbacks(bleSerialServerCallbacks);

    // Create the BLE Service
    pService = pServer->createService(SERVICE_UUID);
    if (pService == nullptr)
        return false;

#ifdef USE_NIMBLE
    // Create a BLE Characteristic
    pTxCharacteristic = pService->createCharacteristic(
                                            CHARACTERISTIC_UUID_TX,
                                            NIMBLE_PROPERTY::NOTIFY
                                        );
    if (pTxCharacteristic == nullptr)
        return false;                    

    NimBLECharacteristic * pRxCharacteristic = pService->createCharacteristic(
                                                CHARACTERISTIC_UUID_RX,
                                                NIMBLE_PROPERTY::WRITE |
                                                NIMBLE_PROPERTY::WRITE_NR
                                                );
#else
    pTxCharacteristic = pService->createCharacteristic(
                                            CHARACTERISTIC_UUID_TX,
                                            BLECharacteristic::PROPERTY_NOTIFY
                                        );
    if (pTxCharacteristic == nullptr)
        return false;                    

    BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(
                                                CHARACTERISTIC_UUID_RX,
                                                BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
                                                );
#endif

    if (pRxCharacteristic == nullptr)
        return false; 

    BLESerialCharacteristicCallbacks* bleSerialCharacteristicCallbacks = new BLESerialCharacteristicCallbacks(); 
    bleSerialCharacteristicCallbacks->bleSerial = this;  
    pRxCharacteristic->setCallbacks(bleSerialCharacteristicCallbacks);
#ifdef USE_NIMBLE
    // Start the service
    pService->start();
    NIMBLE_LOGI(LOG_TAG , "BLE started service");

    // Start advertising
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(pService->getUUID());
    pAdvertising->setScanResponse(true);
    pAdvertising->start();
    NIMBLE_LOGI(LOG_TAG ,"BLE started advertising and waiting for client connection...");
    return true;
#else
    // Start the service
    pService->start();
    LOG_INFO("BLE started service");

    // Start advertising
    pServer->getAdvertising()->addServiceUUID(pService->getUUID()); 
    pServer->getAdvertising()->setScanResponse(true);
    pServer->getAdvertising()->setMinPreferred(0x06);
    pServer->getAdvertising()->setMaxPreferred(0x12);
    pServer->getAdvertising()->start();
    LOG_INFO("BLE started advertising and waiting for client connection...");
    return true;
#endif
}

int BLESerial::available(void)
{
    // reply with data available
    return receiveBuffer.length();
}

int BLESerial::peek(void)
{
    // return first character available
    // but don't remove it from the buffer
    if ((receiveBuffer.length() > 0)){
        uint8_t c = receiveBuffer[0];
        return c;
    }
    else
        return -1;
}

bool BLESerial::connected(void)
{
    // true if connected
    if (pServer->getConnectedCount() > 0)
        return true;
    else 
        return false;        
}

int BLESerial::read(void)
{
    // read a character
    if ((receiveBuffer.length() > 0)){
        uint8_t c = receiveBuffer[0];
        receiveBuffer.erase(0,1); // remove it from the buffer
        return c;
    }
    else
        return -1;
}

size_t BLESerial::write(uint8_t c)
{
    // write a character
    uint8_t _c = c;
    pTxCharacteristic->setValue(&_c, 1);
    pTxCharacteristic->notify();
    delay(3); // bluetooth stack will go into congestion, if too many packets are sent
    return 1;
}

size_t BLESerial::write(const uint8_t *buffer, size_t size)
{
    // write a buffer
    for(int i=0; i < size; i++){
        write(buffer[i]);
  }
  return size;
}

void BLESerial::flush()
{
    // remove buffered data
    receiveBuffer.clear();
}

void BLESerial::end()
{
    // close connection
}
