#include "ble_serial.h"

namespace LoraPrs {

class BLESerialServerCallbacks: public NimBLEServerCallbacks {
    friend class BLESerial;
    BLESerial* bleSerial;

    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
        NimBLEDevice::stopAdvertising();
        LOG_INFO("BLE client connected, stopped advertising");
    };

    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
        NimBLEDevice::startAdvertising();
        LOG_INFO("BLE client disconnected, started advertising");
    }
};

class BLESerialCharacteristicCallbacks: public NimBLECharacteristicCallbacks {
    friend class BLESerial;
    BLESerial* bleSerial;

    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        if (!bleSerial || !pCharacteristic) return;
        NimBLEAttValue attValue = pCharacteristic->getValue();
        if (bleSerial->receiveQueue_.available() < attValue.size()) {
            LOG_ERROR("RX queue overflow");
            return;
        }
        for (int i = 0; i < attValue.size(); i++)
            bleSerial->receiveQueue_.unshift(attValue.data()[i]);
    }
};

BLESerial::BLESerial()
  : pService_(nullptr)
  , pTxCharacteristic_(nullptr)
{
}

BLESerial::~BLESerial(void)
{
}

bool BLESerial::begin(const char* localName)
{
    NimBLEDevice::init(localName);
    NimBLEDevice::setPower(CfgPower);

    pServer_ = NimBLEDevice::createServer();
    if (pServer_ == nullptr) {
        LOG_ERROR("Failed to create server");
        return false;
    }

    BLESerialServerCallbacks* bleSerialServerCallbacks = new BLESerialServerCallbacks();
    bleSerialServerCallbacks->bleSerial = this;
    pServer_->setCallbacks(bleSerialServerCallbacks);

    pService_ = pServer_->createService(CfgServiceUuid);
    if (pService_ == nullptr) {
        LOG_ERROR("Failed to create service");
        return false;
    }

    pTxCharacteristic_ = pService_->createCharacteristic(
        CfgCharacteristicUuidTx, NIMBLE_PROPERTY::NOTIFY);
    if (pTxCharacteristic_ == nullptr) {
        LOG_ERROR("Failed to create TX characteristic");
        return false;
    }

    NimBLECharacteristic * pRxCharacteristic = pService_->createCharacteristic(
        CfgCharacteristicUuidRx, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
    if (pRxCharacteristic == nullptr) {
        LOG_ERROR("Failed to create RX characteristic");
        return false;
    }

    BLESerialCharacteristicCallbacks* bleSerialCharacteristicCallbacks = new BLESerialCharacteristicCallbacks();
    bleSerialCharacteristicCallbacks->bleSerial = this;
    pRxCharacteristic->setCallbacks(bleSerialCharacteristicCallbacks);

    pService_->start();
    LOG_INFO("BLE started service");

    NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
    if (advertising) {
        advertising->addServiceUUID(pService_->getUUID());
        advertising->enableScanResponse(false);
        advertising->start();
    }
    LOG_INFO("BLE started advertising and waiting for client connection...");
    return true;
}

int BLESerial::available(void)
{
    return receiveQueue_.size() > 0;
}

int BLESerial::peek(void)
{
    if (receiveQueue_.size() > 0)
        return receiveQueue_.last();
    else
        return -1;
}

bool BLESerial::connected(void)
{
    if (pServer_ && pServer_->getConnectedCount() > 0)
        return true;
    else
        return false;
}

int BLESerial::read(void)
{
    if (receiveQueue_.size() > 0){
        return receiveQueue_.pop();
    }
    else
        return -1;
}

size_t BLESerial::write(uint8_t c)
{
    if (transmitQueue_.isFull()) {
        LOG_ERROR("TX queue overflow");
        return 0;
    }
    transmitQueue_.unshift(c);
    if (transmitQueue_.size() >= getMaxPayloadSize())
        transmit();
    return 1;
}

size_t BLESerial::write(const uint8_t *buffer, size_t size)
{
    for (size_t i = 0; i < size; i++)
        write(buffer[i]);
    return size;
}

size_t BLESerial::getMaxPayloadSize() 
{
    uint16_t mtu = NimBLEDevice::getMTU();
    size_t maxPayload = 0;
    return (mtu >= CfgMinMtuSize && mtu <= CfgMaxMtuSize)
        ? (size_t)(mtu - CfgHdrMtuSize)
        : CfgMinMtuSize - CfgHdrMtuSize;
}

void BLESerial::transmit()
{
    if (!pTxCharacteristic_) {
        LOG_ERROR("No TX characteristic to transmit");
        return;
    }
    size_t maxPayloadSize = getMaxPayloadSize();

    size_t queueLen;
    while ((queueLen = transmitQueue_.size()) > 0) {
        size_t txLen = queueLen > maxPayloadSize ? maxPayloadSize : queueLen;
        uint8_t buffer[txLen];
        for (int i = 0; i < txLen; i++)
            buffer[i] = transmitQueue_.pop();
        pTxCharacteristic_->setValue(buffer, txLen);
        pTxCharacteristic_->notify();
    }
}

void BLESerial::flush()
{
    transmit();
}

void BLESerial::end()
{
    LOG_INFO("Deinitializing BLE");
    NimBLEDevice::stopAdvertising();
    pServer_ = nullptr;
    pTxCharacteristic_ = nullptr;
    pService_ = nullptr;
    NimBLEDevice::deinit(true);
}

} // LoraPrs
