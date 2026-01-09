#include "ble_serial.h"

namespace LoraPrs {

  class BLESerialServerCallbacks: public NimBLEServerCallbacks {
    friend class BLESerial;
    BLESerial* bleSerial_ = nullptr;

    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
      NimBLEDevice::stopAdvertising();
      bleSerial_->isConnected_ = true;
      LOG_INFO("BLE client connected, stopped advertising");
    };

    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
      NimBLEDevice::startAdvertising();
      bleSerial_->isConnected_ = false;
      LOG_INFO("BLE client disconnected, started advertising");
    }
  };

  class BLESerialCharacteristicCallbacks: public NimBLECharacteristicCallbacks {
    friend class BLESerial;
    BLESerial* bleSerial_ = nullptr;

    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
      if (!bleSerial_ || !pCharacteristic) return;
      NimBLEAttValue attValue = pCharacteristic->getValue();
      if (bleSerial_->receiveQueue_.available() < attValue.size()) {
        LOG_ERROR("RX queue overflow");
        return;
      }
      for (int i = 0; i < attValue.size(); i++)
        bleSerial_->receiveQueue_.unshift(attValue.data()[i]);
    }
  };

  BLESerial::BLESerial()
    : isConnected_(false)
    , pService_(nullptr)
    , pTxCharacteristic_(nullptr)
  {
  }

  BLESerial::~BLESerial(void)
  {
  }

  bool BLESerial::begin(const Config &conf)
  {
    config_ = conf;

    LOG_INFO("Setting up BLE", config_.BtName.c_str());

    NimBLEDevice::init(config_.BtName.c_str());
    NimBLEDevice::setPower(config_.BtBlePwr);

    bool hasPinCode = config_.BtBlePinCode != 0;

    if (hasPinCode) {
      LOG_INFO("Enabling pincode auth");
      NimBLEDevice::setSecurityAuth(true, true, false);
      NimBLEDevice::setSecurityPasskey(config_.BtBlePinCode);
      NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
    }
    pServer_ = NimBLEDevice::createServer();
    if (pServer_ == nullptr) {
      LOG_ERROR("Failed to create server");
      return false;
    }

    BLESerialServerCallbacks* bleSerialServerCallbacks = new BLESerialServerCallbacks();
    bleSerialServerCallbacks->bleSerial_ = this;
    pServer_->setCallbacks(bleSerialServerCallbacks);

    pService_ = pServer_->createService(CfgServiceUuid);
    if (pService_ == nullptr) {
      LOG_ERROR("Failed to create service");
      return false;
    }

    uint32_t txProperties = NIMBLE_PROPERTY::NOTIFY;
    pTxCharacteristic_ = pService_->createCharacteristic(CfgCharacteristicUuidTx, txProperties);
    if (pTxCharacteristic_ == nullptr) {
      LOG_ERROR("Failed to create TX characteristic");
      return false;
    }

    uint32_t rxProperties = NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR;
    if (hasPinCode)
      rxProperties |= NIMBLE_PROPERTY::WRITE_ENC | NIMBLE_PROPERTY::WRITE_AUTHEN;
    NimBLECharacteristic * pRxCharacteristic = pService_->createCharacteristic(CfgCharacteristicUuidRx, rxProperties);
    if (pRxCharacteristic == nullptr) {
      LOG_ERROR("Failed to create RX characteristic");
      return false;
    }

    BLESerialCharacteristicCallbacks* bleSerialCharacteristicCallbacks = new BLESerialCharacteristicCallbacks();
    bleSerialCharacteristicCallbacks->bleSerial_ = this;
    pRxCharacteristic->setCallbacks(bleSerialCharacteristicCallbacks);

    pService_->start();
    LOG_INFO("BLE started service");

    NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
    if (advertising) {
      advertising->setName(config_.BtName.c_str());
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
    if (!isConnected_) return 0;
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
    if (!isConnected_) return 0;
    if (transmitQueue_.isFull()) {
      LOG_ERROR("Tx queue overflow");
      return 0;
    }
    int cntWritten = 0;
    for (size_t i = 0; i < size; i++)
      cntWritten += write(buffer[i]);
    return cntWritten;
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
