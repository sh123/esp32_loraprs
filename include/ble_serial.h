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

#ifndef _BLE_SERIAL_H_
#define _BLE_SERIAL_H_

#include "Arduino.h"
#include "Stream.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <DebugLog.h>

#define SERVICE_UUID           "00000001-ba2a-46c9-ae49-01b0961f68bb" // KISS service UUID
#define CHARACTERISTIC_UUID_TX "00000003-ba2a-46c9-ae49-01b0961f68bb"
#define CHARACTERISTIC_UUID_RX "00000002-ba2a-46c9-ae49-01b0961f68bb"

class BLESerial: public Stream
{
    public:

        BLESerial(void);
        ~BLESerial(void);

        bool begin(const char* localName);
        int available(void);
        int peek(void);
        bool connected(void);
        int read(void);
        size_t write(uint8_t c);
        size_t write(const uint8_t *buffer, size_t size);
        void flush();
        void end(void);

    private:
        String local_name;
        BLEServer *pServer = NULL;
        BLEService *pService;
        BLECharacteristic * pTxCharacteristic;
        bool deviceConnected = false;
        uint8_t txValue = 0;
        
        std::string receiveBuffer;

        friend class BLESerialServerCallbacks;
        friend class BLESerialCharacteristicCallbacks;

};

#endif
