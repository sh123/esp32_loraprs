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

class BLESerialServerCallbacks: public BLEServerCallbacks {
    friend class BLESerial; 
    BLESerial* bleSerial;
    
    void onConnect(BLEServer* pServer) {
        // do anything needed on connection
        delay(1000); // wait for connection to complete or messages can be lost
    };

    void onDisconnect(BLEServer* pServer) {
        pServer->startAdvertising(); // restart advertising
        LOG_INFO("Started advertising");
    }
};

class BLESerialCharacteristicCallbacks: public BLECharacteristicCallbacks {
    friend class BLESerial; 
    BLESerial* bleSerial;
    
    void onWrite(BLECharacteristic *pCharacteristic) {
 
      bleSerial->receiveBuffer = bleSerial->receiveBuffer + pCharacteristic->getValue();
    }

};

// Constructor

BLESerial::BLESerial()
{
  // create instance  
  receiveBuffer = "";
}

// Destructor

BLESerial::~BLESerial(void)
{
    // clean up
}

// Begin bluetooth serial

bool BLESerial::begin(const char* localName)
{
    // Create the BLE Device
    BLEDevice::init(localName);

    // Create the BLE Server
    pServer = BLEDevice::createServer();
    if (pServer == nullptr)
        return false;
    
    BLESerialServerCallbacks* bleSerialServerCallbacks =  new BLESerialServerCallbacks(); 
    bleSerialServerCallbacks->bleSerial = this;      
    pServer->setCallbacks(bleSerialServerCallbacks);

    // Create the BLE Service
    pService = pServer->createService(SERVICE_UUID);
    if (pService == nullptr)
        return false;

    // Create a BLE Characteristic
    pTxCharacteristic = pService->createCharacteristic(
                                            CHARACTERISTIC_UUID_TX,
                                            BLECharacteristic::PROPERTY_NOTIFY
                                        );
    if (pTxCharacteristic == nullptr)
        return false;                    
    pTxCharacteristic->addDescriptor(new BLE2902());

    BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(
                                                CHARACTERISTIC_UUID_RX,
                                                BLECharacteristic::PROPERTY_WRITE
                                            );
    if (pRxCharacteristic == nullptr)
        return false; 

    BLESerialCharacteristicCallbacks* bleSerialCharacteristicCallbacks =  new BLESerialCharacteristicCallbacks(); 
    bleSerialCharacteristicCallbacks->bleSerial = this;  
    pRxCharacteristic->setCallbacks(bleSerialCharacteristicCallbacks);

    // Start the service
    pService->start();
    LOG_INFO("starting service");

    // Start advertising
    pServer->getAdvertising()->addServiceUUID(pService->getUUID()); 
    pServer->getAdvertising()->start();
    LOG_INFO("Waiting a client connection to notify...");
    return true;
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
    delay(10); // bluetooth stack will go into congestion, if too many packets are sent
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
