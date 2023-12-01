#include "loraprs_service.h"

namespace LoraPrs {

TaskHandle_t Service::rigTaskHandle_;
volatile bool Service::rigIsRxActive_ = false;
volatile bool Service::rigIsRxIsrEnabled_ = true;

Service::Service()
  : Kiss::Processor()
  , config_()
  , csmaP_(CfgCsmaPersistence)
  , csmaSlotTime_(CfgCsmaSlotTimeMs)
  , csmaSlotTimePrev_(0)
  , beaconLastTimestampMs_(0)
  , rigCurrentTxPacketSize_(0)
  , isIsrInstalled_(false)
  , rigIsImplicitMode_(false)
  , serialBt_()
  , serialBLE_()
  , kissServer_(new WiFiServer(CfgKissPort))
  , isKissClientConnected_(false)
{
  rigIsRxIsrEnabled_ = true;
  rigIsRxActive_ = false;
}

void Service::setup(const Config &conf)
{
  config_ = conf;  
  beaconLastTimestampMs_ = 0;
  disableKiss_ = conf.EnableTextPackets;
  usePrefix3_ = conf.EnableTextPackets3;

  LOG_SET_OPTION(false, false, true);  // disable file, line, enable func

  // disable logging when USB is used for data transfer
  if (config_.UsbSerialEnable) {
    LOG_SET_LEVEL(DebugLogLevel::LVL_NONE);
  } else {
    LOG_SET_LEVEL(config_.LogLevel);
  }

  // KISS extensions are disabled in TNC2 mode
  if (disableKiss_) {
    LOG_INFO("KISS extensions are disabled in TNC2 mode");
    config_.KissEnableExtensions = false;
  }

  // APRS-IS loging callsign validity
  aprsMyCallsign_ = AX25::Callsign(config_.AprsLogin);
  if (!aprsMyCallsign_.IsValid()) {
    LOG_ERROR("Own callsign", config_.AprsLogin, "is not valid");
  }

  // APRS-IS login command
  aprsLoginCommand_ = String("user ") + config_.AprsLogin + String(" pass ") + 
    config_.AprsPass + String(" vers ") + CfgLoraprsVersion;
  if (config_.EnableIsToRf && config_.AprsFilter.length() > 0) {
    aprsLoginCommand_ += String(" filter ") + config_.AprsFilter;
  }
  aprsLoginCommand_ += String("\n");

  // radio module, FSK/LoRa
  if (config_.ModType == CFG_MOD_TYPE_FSK) {
    setupRigFsk(config_.LoraFreqRx, config_.FskBitRate, config_.FskFreqDev, config_.FskRxBw, config_.LoraPower);
  }
  else {
    setupRig(config_.LoraFreqRx, config_.LoraBw, config_.LoraSf, 
      config_.LoraCodingRate, config_.LoraPower, config_.LoraSync, config_.LoraCrc, config_.LoraExplicit);
  }

  // start radio task
  xTaskCreate(rigTask, "rigTask", 4096, this, 5, &rigTaskHandle_);

  // peripherls, WiFi
  if (needsWifi()) {
    setupWifi(config_.WifiSsid, config_.WifiKey);
  }
  
  // peripherals, Bluetooth/BLE
  if (needsBt()) {
    setupBt(config_.BtName);
  }

  // APRS-IS
  if (needsAprsis() && config_.EnablePersistentAprsConnection) {
    reconnectAprsis();
  }

  // peripherals, PTT
  if (config_.PttEnable) {
    LOG_INFO("External PTT is enabled");
    pinMode(config_.PttPin, OUTPUT);
  }

  // telemetry event
  if (config_.TlmEnable && config_.IsClientMode) {
    LOG_INFO("Telemetry event is enabled");
    telemetryTimer_.every(CfgTelemetryPeriodMs, sendModemTelemetryTimer, this);
  }
}

void Service::printConfig() const {
  LOG_INFO("Version:", CfgLoraprsVersion);
  LOG_INFO("Current mode:", config_.IsClientMode ? "NORMAL" : "APRS-IS iGate");
  LOG_INFO(disableKiss_ ? "Using TNC2 text mode" : "Using TNC KISS and AX.25 mode");
  LOG_INFO("UsbSerialEnable:", config_.UsbSerialEnable ? "yes" : "no");
  if (!config_.IsClientMode) {
    LOG_INFO("EnableSignalReport:", config_.EnableSignalReport ? "yes" : "no");
    LOG_INFO("EnablePersistentAprsConnection:", config_.EnablePersistentAprsConnection ? "yes" : "no");
    LOG_INFO("EnableRfToIs:", config_.EnableRfToIs ? "yes" : "no");
    LOG_INFO("EnableIsToRf:", config_.EnableIsToRf ? "yes" : "no");
    LOG_INFO("EnableRepeater:", config_.EnableRepeater ? "yes" : "no");
    LOG_INFO("EnableBeacon:", config_.EnableBeacon ? "yes" : "no");
  }
}

void Service::setupWifi(const String &wifiName, const String &wifiKey)
{
  WiFi.setHostname("loraprs");

  // AP mode
  if (config_.WifiEnableAp) {
    LOG_INFO("WIFI is running in AP mode", wifiName);
    WiFi.softAP(wifiName.c_str(), wifiKey.c_str());    
    LOG_INFO("IP address:", WiFi.softAPIP());

  // Client/STA mode
  } else {
    LOG_INFO("WIFI connecting to", wifiName);
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiName.c_str(), wifiKey.c_str());
  
    int retryCnt = 0;
    while (WiFi.status() != WL_CONNECTED) {
      delay(CfgConnRetryMs);
      LOG_INFO("WIFI retrying", retryCnt);
      if (retryCnt++ >= CfgConnRetryMaxTimes) {
        LOG_ERROR("WIFI connect failed");
        return;
      }
    }
    LOG_INFO("WIFI connected to", wifiName);
    LOG_INFO("IP address:", WiFi.localIP());
  }
  // Run KISS server if enabled
  if (config_.KissEnableTcpIp) {
    LOG_INFO("KISS TCP/IP server started on port", CfgKissPort);
    kissServer_->begin();
  }
}

void Service::reconnectWifi() const
{
  // AP mode does not require re-connection
  if (config_.WifiEnableAp) return;
  
  LOG_WARN("WIFI re-connecting...");

  int retryCnt = 0;
  while (WiFi.status() != WL_CONNECTED || WiFi.localIP() == IPAddress(0,0,0,0)) {
    WiFi.reconnect();
    delay(CfgConnRetryMs);
    LOG_WARN("WIFI re-connecting", retryCnt);
    if (retryCnt++ >= CfgConnRetryMaxTimes) {
      LOG_ERROR("WIFI re-connect failed");
      return;
    }
  }

  LOG_INFO("WIFI reconnected, IP address", WiFi.localIP());
  
  if (config_.KissEnableTcpIp) {
    LOG_INFO("KISS TCP/IP server started on port", CfgKissPort);
    kissServer_->begin();
  }
}

bool Service::reconnectAprsis()
{
  LOG_INFO("APRSIS connecting to", config_.AprsHost);
  
  if (!aprsisConnection_.connect(config_.AprsHost.c_str(), config_.AprsPort)) {
    LOG_ERROR("Failed to connect to", config_.AprsHost, ":", config_.AprsPort);
    return false;
  }
  LOG_INFO("APRSIS connected");
  aprsisConnection_.print(aprsLoginCommand_);
  LOG_INFO("APRSIS logged in");
  return true;
}

float Service::getSnrLimit(int sf, long bw) const
{
  float snrLimit = -7;
  switch (sf) {
    case 7:
        snrLimit = -7.5;
        break;
    case 8:
        snrLimit = -10.0;
        break;
    case 9:
        snrLimit = -12.6;
        break;
    case 10:
        snrLimit = -15.0;
        break;
    case 11:
        snrLimit = -17.5;
        break;
    case 12:
        snrLimit = -20.0;
        break;
  }
  return -174 + 10 * log10(bw) + 6 + snrLimit;
}

void Service::setupRig(long loraFreq, long bw, int sf, int cr, int pwr, int sync, int crcBytes, bool isExplicit)
{
  rigIsImplicitMode_ = !isExplicit;
  rigIsImplicitMode_ = sf == 6;      // must be implicit for SF6
  int loraSpeed = getSpeed(sf, cr, bw);
        
  LOG_INFO("Initializing LoRa");
  LOG_INFO("Frequency:", loraFreq, "Hz");
  LOG_INFO("Bandwidth:", bw, "Hz");
  LOG_INFO("Spreading:", sf);
  LOG_INFO("Coding rate:", cr);
  LOG_INFO("Power:", pwr, "dBm");
  LOG_INFO("Sync:", "0x" + String(sync, HEX));
  LOG_INFO("CRC:", crcBytes);
  LOG_INFO("Header:", rigIsImplicitMode_ ? "implicit" : "explicit");
  LOG_INFO("Speed:", loraSpeed, "bps");
  LOG_INFO("TOA (compressed): ~", 37.0 / ((double)loraSpeed / 8.0), "sec");
  LOG_INFO("TOA (uncompressed): ~", 64.0 / ((double)loraSpeed / 8.0), "sec");
  LOG_INFO("Min level:", getSnrLimit(sf, bw), "dBm");
  printConfig();

  rig_ = std::make_shared<MODULE_NAME>(new Module(config_.LoraPinSs, config_.LoraPinA, config_.LoraPinRst, config_.LoraPinB));
  int state = rig_->begin((float)loraFreq / 1e6, (float)bw / 1e3, sf, cr, sync, pwr);
  if (state != RADIOLIB_ERR_NONE) {
    LOG_ERROR("Radio start error:", state);
  }
  rig_->setCRC(crcBytes);
  rig_->setPreambleLength(config_.LoraPreamble);
#ifdef USE_SX126X
    #pragma message("Using SX126X")
    LOG_INFO("Using SX126X module");
    rig_->setRfSwitchPins(config_.LoraPinSwitchRx, config_.LoraPinSwitchTx);
    if (isIsrInstalled_) rig_->clearDio1Action();
    rig_->setDio1Action(onRigIsrRxPacket);
    isIsrInstalled_ = true;
#else
    #pragma message("Using SX127X")
    LOG_INFO("Using SX127X module");
    if (isIsrInstalled_) rig_->clearDio0Action();
    rig_->setDio0Action(onRigIsrRxPacket, RISING);
    isIsrInstalled_ = true;
#endif

  if (rigIsImplicitMode_) {
    rig_->implicitHeader(0xff);
  } else {
    rig_->explicitHeader();
  }
  
  state = rig_->startReceive();
  if (state != RADIOLIB_ERR_NONE) {
    LOG_ERROR("Receive start error:", state);
  }

  LOG_INFO("LoRa initialized");
}

void Service::setupRigFsk(long freq, float bitRate, float freqDev, float rxBw, int pwr)
{
  LOG_INFO("Initializing FSK");
  LOG_INFO("Frequency:", freq, "Hz");
  LOG_INFO("Bit rate:", bitRate, "kbps");
  LOG_INFO("Deviation:", freqDev, "kHz");
  LOG_INFO("Bandwidth:", rxBw, "kHz");
  LOG_INFO("Power:", pwr, "dBm");
  rig_ = std::make_shared<MODULE_NAME>(new Module(config_.LoraPinSs, config_.LoraPinA, config_.LoraPinRst, config_.LoraPinB));
  int state = rig_->beginFSK((float)freq / 1e6, bitRate, freqDev, rxBw, pwr);
  if (state != RADIOLIB_ERR_NONE) {
    LOG_ERROR("Radio start error:", state);
  }
  rig_->disableAddressFiltering();
#ifdef USE_SX126X
    #pragma message("Using SX126X")
    LOG_INFO("Using SX126X module");
    rig_->setRfSwitchPins(config_.LoraPinSwitchRx, config_.LoraPinSwitchTx);
    if (isIsrInstalled_) rig_->clearDio1Action();
    rig_->setDio1Action(onRigIsrRxPacket);
    isIsrInstalled_ = true;
#else
    #pragma message("Using SX127X")
    LOG_INFO("Using SX127X module");
    if (isIsrInstalled_) rig_->clearDio0Action();
    rig_->setDio0Action(onRigIsrRxPacket, RISING);
    isIsrInstalled_ = true;
#endif

  state = rig_->startReceive();
  if (state != RADIOLIB_ERR_NONE) {
    LOG_ERROR("Receive start error:", state);
  }

  LOG_INFO("FSK initialized");
}

void Service::setupBt(const String &btName)
{
  String btType = config_.BtEnableBle ? "BLE" : "BT";
  LOG_INFO(btType, "init", btName);
  
  bool btOk = config_.BtEnableBle 
    ? serialBLE_.begin(btName.c_str()) 
    : serialBt_.begin(btName);
  
  if (btOk) {
    LOG_INFO(btType, "initialized");
  }
  else {
    LOG_ERROR(btType, "failed");
  }
}

void Service::loop()
{ 
  // wifi connectivity check
  if (needsWifi() && WiFi.status() != WL_CONNECTED) {
    reconnectWifi();
  }
  // aprs connectivity check
  if (needsAprsis() && !aprsisConnection_.connected() && config_.EnablePersistentAprsConnection) {
    reconnectAprsis();
  }
  // tcp/ip kiss client connectivity check
  if (config_.KissEnableTcpIp) {
    attachKissNetworkClient();
  }

  // RX path, Rig -> Serial
  if (!processRigToSerial()) {

    // TX path, Serial -> Rig
    long currentTime = millis();
    if (!isRigRxBusy() && currentTime > csmaSlotTimePrev_ + csmaSlotTime_ && random(0, 255) < csmaP_) {
      // new data from aprsis
      if (aprsisConnection_.available() > 0) {
        onAprsisDataAvailable();
      }
      // periodic beacon
      if (needsBeacon()) {
        sendPeriodicBeacon();
      }
      // tx if data available
      processSerialToRig();
      csmaSlotTimePrev_ = currentTime;
    }
  }

  // timers
  if (config_.TlmEnable) {
    telemetryTimer_.tick();
  }
  startRxTimer_.tick();
}

ICACHE_RAM_ATTR void Service::onRigIsrRxPacket() {
  BaseType_t xHigherPriorityTaskWoken;
  if (rigIsRxIsrEnabled_) {
    rigIsRxActive_ = true;
    xTaskNotifyFromISR(rigTaskHandle_, RadioTaskBits::Receive, eSetBits, &xHigherPriorityTaskWoken);
  }
}

void Service::rigTask(void *self) {
  LOG_INFO("Radio task started");
  while (true) {
    uint32_t commandBits = 0;
    xTaskNotifyWaitIndexed(0, 0x00, ULONG_MAX, &commandBits, portMAX_DELAY);
    if (commandBits & RadioTaskBits::Receive) {
      static_cast<Service*>(self)->onRigTaskRxPacket();
    }
    else if (commandBits & RadioTaskBits::Transmit) {
      static_cast<Service*>(self)->onRigTaskTxPacket();
    }
    if (commandBits & RadioTaskBits::StartReceive) {
      static_cast<Service*>(self)->onRigTaskStartRx();
    }
    else if (commandBits & RadioTaskBits::StartTransmit) {
      static_cast<Service*>(self)->onRigTaskStartTx();
    }
  }
}

void Service::onRigTaskStartRx() {
  LOG_TRACE("onRigTaskStartRx");
  if (config_.PttEnable) {
    digitalWrite(config_.PttPin, LOW);
  }
  if (isHalfDuplex()) {
    setFreq(config_.LoraFreqRx);
  }
  int state = rig_->startReceive();
  if (state != RADIOLIB_ERR_NONE) {
    LOG_ERROR("Start receive error: ", state);
  }
  rigIsRxIsrEnabled_ = true;
}

void Service::onRigTaskRxPacket() {
  int packetSize = rig_->getPacketLength();
  LOG_TRACE("onRigTaskRxPacket", packetSize);
  if (packetSize > 0) {
    byte rxBuf[packetSize];
    int state = rig_->readData(rxBuf, packetSize);
    if (state == RADIOLIB_ERR_NONE) {
      queueRigToSerial(Cmd::Data, rxBuf, packetSize);
    } else {
      LOG_ERROR("Read data error: ", state);
    }

    state = rig_->startReceive();
    if (state != RADIOLIB_ERR_NONE) {
      LOG_ERROR("Start receive error: ", state);
    }
  }
  rigIsRxActive_ = false;
}

void Service::onRigTaskStartTx() {
  LOG_TRACE("onRigTaskStartTx");
  if (rigIsRxIsrEnabled_) {
    rigIsRxIsrEnabled_ = false;
    if (isHalfDuplex()) {
      setFreq(config_.LoraFreqTx);
    }
    if (config_.PttEnable) {
      digitalWrite(config_.PttPin, HIGH);
    }
    delay(config_.PttTxDelayMs);
  }
}

void Service::onRigTaskTxPacket() {
  while (rigTxQueueIndex_.size() > 0) {
    int txPacketSize = rigTxQueueIndex_.shift();
    LOG_TRACE("onRigTaskTxPacket", txPacketSize);
    byte txBuf[txPacketSize];

    for (int i = 0; i < txPacketSize; i++) {
      txBuf[i] = rigTxQueue_.shift();
    }

    int state = rig_->transmit(txBuf, txPacketSize);
    if (state != RADIOLIB_ERR_NONE) {
      LOG_ERROR("TX error: ", state);
    }
    vTaskDelay(1);
  }
  startRxTimer_.in(config_.PttTxTailMs, &startRxTimer);
}

void Service::sendPeriodicBeacon()
{
  long currentMs = millis();

  if (beaconLastTimestampMs_ == 0 || currentMs - beaconLastTimestampMs_ >= config_.AprsRawBeaconPeriodMinutes * 60 * 1000) {
      AX25::Payload payload(config_.AprsRawBeacon);
      if (payload.IsValid()) {
        sendAx25PayloadToRig(payload);
        if (config_.EnableRfToIs) {
          sendToAprsis(payload.ToString());
        }
        LOG_INFO("Periodic beacon is sent");
      }
      else {
        LOG_ERROR("Beacon payload is invalid");
      }
      beaconLastTimestampMs_ = currentMs;
  }
}

void Service::sendToAprsis(const String &aprsMessage)
{
  if (needsWifi() && WiFi.status() != WL_CONNECTED) {
    reconnectWifi();
  }
  if (needsAprsis() && !aprsisConnection_.connected()) {
    reconnectAprsis();
  }
  aprsisConnection_.println(aprsMessage);

  if (!config_.EnablePersistentAprsConnection) {
    aprsisConnection_.stop();
  }
}

void Service::onAprsisDataAvailable()
{
  String aprsisData;

  while (aprsisConnection_.available() > 0) {
    char c = aprsisConnection_.read();
    if (c == '\r') continue;
    if (c == '\n') break;
    aprsisData += c;
    if (aprsisData.length() >= CfgMaxPacketSize) {
      LOG_WARN("APRS-IS incoming message is too long, skipping tail");
      break;
    }
  }

  LOG_INFO(aprsisData);
  
  if (config_.EnableIsToRf && aprsisData.length() > 0) {
    AX25::Payload payload(aprsisData);
    if (payload.IsValid()) {
      sendAx25PayloadToRig(payload);
    }
    else {
      LOG_WARN("Unknown payload from APRSIS, ignoring");
    }
  }
}

void Service::sendSignalReportEvent(int rssi, float snr)
{
  struct SignalReport signalReport;

  signalReport.rssi = htobe16(rssi);
  signalReport.snr = htobe16(snr * 100);

  sendRigToSerial(Cmd::SignalReport, (const byte *)&signalReport, sizeof(SignalReport));
}

bool Service::sendModemTelemetryTimer(void *param)
{
  static_cast<Service*>(param)->sendModemTelemetry();
  return true;
}

bool Service::startRxTimer(void *param)
{
  static_cast<Service*>(param)->startRx();
  return true;
}

void Service::startRx() {
  xTaskNotify(rigTaskHandle_, RadioTaskBits::StartReceive, eSetBits);
}

void Service::sendModemTelemetry()
{
  float batVoltage = 2 * analogRead(config_.TlmBatMonPin) * (3.3 / 4096.0) + config_.TlmBatMonCal;
  LOG_INFO("Battery voltage", batVoltage);

  struct Telemetry telemetry;
  telemetry.batteryVoltage = htobe16(100 * batVoltage);
  sendRigToSerial(Cmd::Telemetry, (const byte *)&telemetry, sizeof(Telemetry));
}

bool Service::sendAx25PayloadToRig(const AX25::Payload &payload)
{
  int bytesWritten;
  byte buf[CfgMaxPacketSize];

  // TNC2 text mode
  if (config_.EnableTextPackets) {
    String textPayload = payload.ToString();
    int textPayloadLength = textPayload.length() + 1; // include null terminator
    bytesWritten = textPayloadLength > CfgMaxPacketSize ? CfgMaxPacketSize : textPayloadLength;
    textPayload.getBytes(buf, bytesWritten);
    buf[bytesWritten-1] = '\0';

  // KISS TNC
  } else {
    bytesWritten = payload.ToBinary(buf, sizeof(buf));
    if (bytesWritten <= 0) {
      LOG_WARN("Failed to serialize payload");
      return false;
    }
  }
  queueSerialToRig(Cmd::Data, buf, bytesWritten);
  return true;
}

void Service::onRigPacket(void *packet, int packetLength)
{
  if (config_.EnableAutoFreqCorrection) {
    performFrequencyCorrection();
  }
  if (config_.KissEnableExtensions) {
    sendSignalReportEvent(rig_->getRSSI(), rig_->getSNR());
  }
  if (!config_.IsClientMode || config_.EnableRepeater) {
    processIncomingRawPacketAsServer((const byte*)packet, packetLength);
  }
}

void Service::performFrequencyCorrection() {
#ifdef USE_SX126X
  long frequencyErrorHz = 0;
#else
  long frequencyErrorHz = rig_->getFrequencyError();
#endif
  if (abs(frequencyErrorHz) > config_.AutoFreqCorrectionDeltaHz) {
    config_.LoraFreqRx -= frequencyErrorHz;
    LOG_INFO("Correcting frequency:", frequencyErrorHz);
    setFreq(config_.LoraFreqRx);
  }
}

void Service::setFreq(long loraFreq) const {
  rig_->setFrequency((float)loraFreq / 1e6);
}

void Service::processIncomingRawPacketAsServer(const byte *packet, int packetLength) {

  // create from binary AX25
  AX25::Payload payload(packet, packetLength);

  // try to parse as text for clients, who submit plain text
  if (!payload.IsValid() && config_.EnableTextPackets) {
    char buf[CfgMaxPacketSize + 1];
    int cpySize = packetLength > CfgMaxPacketSize ? CfgMaxPacketSize : packetLength;
    memcpy(buf, packet, cpySize);
    buf[cpySize] = '\0';
    payload = AX25::Payload(String((char*)buf));
  }

  if (payload.IsValid()) {
    float snr = rig_->getSNR();
    int rssi = rig_->getRSSI();
#ifdef USE_SX126X
    long frequencyError = 0;
#else
    long frequencyError = rig_->getFrequencyError();
#endif
    String signalReport = String("rssi: ") +
      String(snr < 0 ? rssi + snr : rssi) +
      String("dBm, ") +
      String("snr: ") +
      String(snr) +
      String("dB");
    if (frequencyError != 0) {
      signalReport += String(", err: ") +
      String(frequencyError) +
      String("Hz");
    }

    String textPayload = payload.ToString(config_.EnableSignalReport ? signalReport : String());
    LOG_INFO(textPayload);

    if (config_.EnableRfToIs) {
      sendToAprsis(textPayload);
      LOG_INFO("Packet sent to APRS-IS");
    }
    if (config_.EnableRepeater && payload.Digirepeat(aprsMyCallsign_)) {
      sendAx25PayloadToRig(payload);
      LOG_INFO("Packet digirepeated");
    }
  } else {
    LOG_WARN("Skipping non-AX25 payload");
  }
}

bool Service::onRigTxBegin()
{
  LOG_TRACE("onRigTxBegin");
  startRxTimer_.cancel();
  xTaskNotify(rigTaskHandle_, RadioTaskBits::StartTransmit, eSetBits);
  rigCurrentTxPacketSize_ = 0;
  return true;
}

void Service::onRigTx(byte b)
{
  LOG_TRACE((char)b, String(b, HEX));
  rigTxQueue_.push(b);
  rigCurrentTxPacketSize_++;
}

void Service::onRigTxEnd()
{
  LOG_TRACE("onRigTxEnd", rigCurrentTxPacketSize_);
  rigTxQueueIndex_.push(rigCurrentTxPacketSize_);
  xTaskNotify(rigTaskHandle_, RadioTaskBits::Transmit, eSetBits);
}

void Service::attachKissNetworkClient() 
{
  // connected, client dropped off
  if (isKissClientConnected_) {
    if (!kissConnnection_.connected()) {
      LOG_INFO("KISS TCP/IP client disconnected");
      isKissClientConnected_ = false;
      kissConnnection_.stop();
    }
  }
  WiFiClient wifiClient = kissServer_->available();
  // new client connected
  if (wifiClient && wifiClient.connected()) {
    // drop off current one
    if (isKissClientConnected_) {
      kissConnnection_.stop();
    }
    LOG_INFO("New KISS TCP/IP client connected");
    kissConnnection_ = wifiClient;
    isKissClientConnected_ = true;
  }
}

void Service::onSerialTx(byte b)
{
  LOG_TRACE((char)b, String(b, HEX));
  if (config_.UsbSerialEnable) {
    Serial.write(b);
  } 
  else if (isKissClientConnected_) {
    kissConnnection_.write(b);
  }
  else if (config_.BtEnableBle) {
    serialBLE_.write(b);
  }
  else {
    serialBt_.write(b);
  }
}

bool Service::onSerialRxHasData()
{
  if (config_.UsbSerialEnable) {
    return Serial.available();
  } 
  else if (isKissClientConnected_) {
    return kissConnnection_.available();
  }
  else if (config_.BtEnableBle) {
    return serialBLE_.available();
  }
  else {
    return serialBt_.available();
  }
}

bool Service::onSerialRx(byte *b)
{
  int rxResult;

  if (config_.UsbSerialEnable) {
    rxResult = Serial.read();
  } 
  else if (isKissClientConnected_) {
    rxResult = kissConnnection_.read();
    // client dropped off
    if (rxResult == -1) {
      kissConnnection_.stop();
      isKissClientConnected_ = false;
    }
  }
  else {
    rxResult = config_.BtEnableBle 
      ? serialBLE_.read() 
      : serialBt_.read();
  }
  if (rxResult == -1) {
    return false;
  }
  *b = (byte)rxResult;
  LOG_TRACE((char)rxResult, String(rxResult, HEX));
  return true;
}

void Service::onControlCommand(Cmd cmd, byte value)
{
  switch (cmd) {
    case Cmd::P:
      LOG_INFO("CSMA P:", value);
      csmaP_ = value;
      break;
    case Cmd::SlotTime:
      LOG_INFO("CSMA SlotTime:", value);
      csmaSlotTime_ = (long)value * 10;
      break;
    case Cmd::TxDelay:
      LOG_INFO("TX delay:", value);
      config_.PttTxDelayMs = (long)value * 10;
      break;
    case Cmd::TxTail:
      LOG_INFO("TX tail:", value);
      config_.PttTxTailMs = (long)value * 10;
      break;
    default:
      break;
  }
}

void Service::onRadioControlCommand(const std::vector<byte> &rawCommand) {

  if (config_.KissEnableExtensions && rawCommand.size() == sizeof(SetHardware)) {
    LOG_INFO("Setting new radio parameters");
    const struct SetHardware * setHardware = reinterpret_cast<const struct SetHardware*>(rawCommand.data());

    config_.LoraFreqRx = be32toh(setHardware->freqRx);
    config_.LoraFreqTx = be32toh(setHardware->freqTx);
    config_.ModType = setHardware->modType;
    config_.LoraBw = be32toh(setHardware->bw);
    config_.LoraSf = be16toh(setHardware->sf);
    config_.LoraCodingRate = be16toh(setHardware->cr);
    config_.LoraPower = (int16_t)be16toh(setHardware->pwr);
    config_.LoraSync = be16toh(setHardware->sync);
    config_.FskBitRate = (float)be32toh(setHardware->fskBitRate) / 1e3;
    config_.FskFreqDev = (float)be32toh(setHardware->fskFreqDev) / 1e3; 
    config_.FskRxBw = (float)be32toh(setHardware->fskRxBw) / 1e3;
    int crcType = setHardware->crc ? config_.LoraCrc : 0;

    if (config_.ModType == CFG_MOD_TYPE_FSK) {
      setupRigFsk(config_.LoraFreqRx, config_.FskBitRate, config_.FskFreqDev, config_.FskRxBw, config_.LoraPower);
    } 
    else {
      setupRig(config_.LoraFreqRx, config_.LoraBw, config_.LoraSf, 
        config_.LoraCodingRate, config_.LoraPower, config_.LoraSync, crcType, config_.LoraExplicit);
    }
  } else {
    LOG_ERROR("Radio control command of wrong size", rawCommand.size());
  }
}

void Service::onRebootCommand()
{
  LOG_INFO("Reboot requested");
  ESP.restart();
}

} // LoraPrs
