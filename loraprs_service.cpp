#include "loraprs_service.h"

namespace LoraPrs {

TaskHandle_t Service::radioTaskHandle_;
volatile bool Service::isRadioRxActive_ = false;
bool Service::interruptEnabled_ = true;

Service::Service()
  : Kiss::Processor()
  , csmaP_(CfgCsmaPersistence)
  , csmaSlotTime_(CfgCsmaSlotTimeMs)
  , csmaSlotTimePrev_(0)
  , currentTxPacketSize_(0)
  , serialBt_()
  , serialBLE_()
  , kissServer_(new WiFiServer(CfgKissPort))
  , isKissConn_(false)
{
  interruptEnabled_ = true;
  isRadioRxActive_ = false;
}

void Service::setup(const Config &conf)
{
  config_ = conf;  
  previousBeaconMs_ = 0;
  disableKiss_ = conf.EnableTextPackets;

  LOG_SET_OPTION(false, false, true);  // disable file, line, enable func

  // disable logging when USB is used for data transfer
  if (config_.UsbSerialEnable) {
    LOG_SET_LEVEL(DebugLogLevel::LVL_NONE);
  } else {
    LOG_SET_LEVEL(config_.LogLevel);
  }

  printConfig();
  
  // KISS extensions are disabled in TNC2 mode
  if (disableKiss_) {
    LOG_INFO("KISS extensions are disabled in TNC2 mode");
    config_.KissEnableExtensions = false;
  }

  // APRS-IS loging callsign validity
  ownCallsign_ = AX25::Callsign(config_.AprsLogin);
  if (!ownCallsign_.IsValid()) {
    LOG_ERROR("Own callsign", config_.AprsLogin, "is not valid");
  }

  // APRS-IS login command
  aprsLoginCommand_ = String("user ") + config_.AprsLogin + String(" pass ") + 
    config_.AprsPass + String(" vers ") + CfgLoraprsVersion;
  if (config_.EnableIsToRf && config_.AprsFilter.length() > 0) {
    aprsLoginCommand_ += String(" filter ") + config_.AprsFilter;
  }
  aprsLoginCommand_ += String("\n");

  // peripherals, LoRa
  setupRadio(config_.LoraFreqRx, config_.LoraBw, config_.LoraSf, 
    config_.LoraCodingRate, config_.LoraPower, config_.LoraSync, config_.LoraCrc, config_.LoraExplicit);

  // start radio task
  xTaskCreate(radioTask, "radioTask", 4096, this, 5, &radioTaskHandle_);

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
}

void Service::printConfig() {
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
  
  if (!aprsisConn_.connect(config_.AprsHost.c_str(), config_.AprsPort)) {
    LOG_ERROR("Failed to connect to", config_.AprsHost, ":", config_.AprsPort);
    return false;
  }
  LOG_INFO("APRSIS connected");
  aprsisConn_.print(aprsLoginCommand_);
  LOG_INFO("APRSIS logged in");
  return true;
}

void Service::setupRadio(long loraFreq, long bw, int sf, int cr, int pwr, int sync, int crcBytes, bool isExplicit)
{
  isImplicitHeaderMode_ = !isExplicit;
  isImplicitHeaderMode_ = sf == 6;      // must be implicit for SF6
  int loraSpeed = (int)(sf * (4.0 / cr) / (pow(2.0, sf) / bw));
        
  LOG_INFO("Initializing LoRa");
  LOG_INFO("Frequency:", loraFreq, "Hz");
  LOG_INFO("Bandwidth:", bw, "Hz");
  LOG_INFO("Spreading:", sf);
  LOG_INFO("Coding rate:", cr);
  LOG_INFO("Power:", pwr, "dBm");
  LOG_INFO("Sync:", "0x" + String(sync, HEX));
  LOG_INFO("CRC:", crcBytes);
  LOG_INFO("Header:", isImplicitHeaderMode_ ? "implicit" : "explicit");
  LOG_INFO("Speed:", loraSpeed, "bps");
  LOG_INFO("TOA (compressed):", 37.0 / ((double)loraSpeed / 8.0), "sec");
  LOG_INFO("TOA (uncompressed):", 64.0 / ((double)loraSpeed / 8.0), "sec");
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
  LOG_INFO("Min level:", -174 + 10 * log10(bw) + 6 + snrLimit, "dBm");
  radio_ = std::make_shared<MODULE_NAME>(new Module(config_.LoraPinSs, config_.LoraPinA, config_.LoraPinRst, config_.LoraPinB));
  int state = radio_->begin((float)loraFreq / 1e6, (float)bw / 1e3, sf, cr, sync, pwr);
  if (state != RADIOLIB_ERR_NONE) {
    LOG_ERROR("Radio start error:", state);
  }
  radio_->setCRC(crcBytes);
#ifdef USE_SX126X
    #pragma message("Using SX126X")
    LOG_INFO("Using SX126X module");
    radio_->setRfSwitchPins(config_.LoraPinSwitchRx, config_.LoraPinSwitchTx);
    radio_->clearDio1Action();
    radio_->setDio1Action(onRadioDataAvailableIsr);
#else
    #pragma message("Using SX127X")
    LOG_INFO("Using SX127X module");
    radio_->clearDio0Action();
    radio_->setDio0Action(onLoraDataAvailableIsr);
#endif

  if (isImplicitHeaderMode_) {
    radio_->implicitHeader(0xff);
  } else {
    radio_->explicitHeader();
  }
  
  state = radio_->startReceive();
  if (state != RADIOLIB_ERR_NONE) {
    LOG_ERROR("Receive start error:", state);
  }

  LOG_INFO("LoRa initialized");
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
  if (needsWifi() && WiFi.status() != WL_CONNECTED) {
    reconnectWifi();
  }
  if (needsAprsis() && !aprsisConn_.connected() && config_.EnablePersistentAprsConnection) {
    reconnectAprsis();
  }
  if (config_.KissEnableTcpIp) {
    attachKissNetworkClient();
  }

  // RX path, Rig -> Serial
  bool isRigToSerialProcessed = false;
  isRigToSerialProcessed = processRigToSerial();

  // TX path, Serial -> Rig
  if (!isRigToSerialProcessed) {

    long currentTime = millis();
    if (!isRadioRxBusy() && currentTime > csmaSlotTimePrev_ + csmaSlotTime_ && random(0, 255) < csmaP_) {
      if (aprsisConn_.available() > 0) {
        onAprsisDataAvailable();
      }
      if (needsBeacon()) {
        sendPeriodicBeacon();
      }
      processSerialToRig();
      csmaSlotTimePrev_ = currentTime;
    }
  }
  delay(CfgPollDelayMs);
}

bool Service::isRadioRxBusy() {
  return config_.LoraUseCad && isRadioRxActive_;
}

ICACHE_RAM_ATTR void Service::onRadioDataAvailableIsr() {
  BaseType_t xHigherPriorityTaskWoken;
  if (interruptEnabled_) {
    isRadioRxActive_ = true;
    uint32_t radioReceiveBit = RadioTaskBits::Receive;
    xTaskNotifyFromISR(radioTaskHandle_, radioReceiveBit, eSetBits, &xHigherPriorityTaskWoken);
  }
}

void Service::radioTask(void *param) {
  Service *self = (Service*)param;
  LOG_INFO("Radio task started");
  while (true) {
    uint32_t commandBits = 0;
    xTaskNotifyWaitIndexed(0, 0x00, ULONG_MAX, &commandBits, portMAX_DELAY);
    if (commandBits & RadioTaskBits::Receive) {
      self->onRadioReceive();
    }
    else if (commandBits & RadioTaskBits::Transmit) {
      self->onRadioTransmit();
    }
  }
}

void Service::onRadioReceive() {
  int packetSize = radio_->getPacketLength();
  if (packetSize > 0) {

    int state = radio_->readData(radioRxPacketBuffer_, packetSize);
    if (state == RADIOLIB_ERR_NONE) {
      queueRigToSerial(Cmd::Data, radioRxPacketBuffer_, packetSize);
    } else {
      LOG_ERROR("Read data error: ", state);
    }

    state = radio_->startReceive();
    if (state != RADIOLIB_ERR_NONE) {
      LOG_ERROR("Start receive error: ", state);
    }
  }
  isRadioRxActive_ = false;
}

void Service::onRadioTransmit() {
  while (radioTxQueueIndex_.size() > 0) {
    int txPacketSize = radioTxQueueIndex_.shift();
    byte txBuf[txPacketSize];

    for (int i = 0; i < txPacketSize; i++) {
      txBuf[i] = radioTxQueue_.shift();
    }

    interruptEnabled_ = false;
    int state = radio_->transmit(txBuf, txPacketSize);
    if (state != RADIOLIB_ERR_NONE) {
      LOG_ERROR("TX error: ", state);
    }
  }
  int state = radio_->startReceive();
  if (state != RADIOLIB_ERR_NONE) {
    LOG_ERROR("Start receive error: ", state);
  }
  interruptEnabled_ = true;
  if (config_.PttEnable) {
    delay(config_.PttTxTailMs);
    digitalWrite(config_.PttPin, LOW);
  }
  if (splitEnabled()) {
    setFreq(config_.LoraFreqRx);
  }
}

void Service::sendPeriodicBeacon()
{
  long currentMs = millis();

  if (previousBeaconMs_ == 0 || currentMs - previousBeaconMs_ >= config_.AprsRawBeaconPeriodMinutes * 60 * 1000) {
      AX25::Payload payload(config_.AprsRawBeacon);
      if (payload.IsValid()) {
        sendAX25ToRadio(payload);
        if (config_.EnableRfToIs) {
          sendToAprsis(payload.ToString());
        }
        LOG_INFO("Periodic beacon is sent");
      }
      else {
        LOG_ERROR("Beacon payload is invalid");
      }
      previousBeaconMs_ = currentMs;
  }
}

void Service::sendToAprsis(const String &aprsMessage)
{
  if (needsWifi() && WiFi.status() != WL_CONNECTED) {
    reconnectWifi();
  }
  if (needsAprsis() && !aprsisConn_.connected()) {
    reconnectAprsis();
  }
  aprsisConn_.println(aprsMessage);

  if (!config_.EnablePersistentAprsConnection) {
    aprsisConn_.stop();
  }
}

void Service::onAprsisDataAvailable()
{
  String aprsisData;

  while (aprsisConn_.available() > 0) {
    char c = aprsisConn_.read();
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
      sendAX25ToRadio(payload);
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

bool Service::sendAX25ToRadio(const AX25::Payload &payload)
{
  int bytesWritten;
  byte buf[CfgMaxPacketSize];

  // TNC2 text mode
  if (config_.EnableTextPackets) {
    String textPayload = payload.ToString();
    bytesWritten = textPayload.length() > CfgMaxPacketSize ? CfgMaxPacketSize : textPayload.length();
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
    sendSignalReportEvent(radio_->getRSSI(), radio_->getSNR());
  }
  if (!config_.IsClientMode) {
    processIncomingRawPacketAsServer((const byte*)packet, packetLength);
  }
}

void Service::performFrequencyCorrection() {
#ifdef USE_SX126X
  long frequencyErrorHz = 0;
#else
  long frequencyErrorHz = radio_->getFrequencyError();
#endif
  if (abs(frequencyErrorHz) > config_.AutoFreqCorrectionDeltaHz) {
    config_.LoraFreqRx -= frequencyErrorHz;
    LOG_INFO("Correcting frequency:", frequencyErrorHz);
    setFreq(config_.LoraFreqRx);
  }
}

void Service::setFreq(long loraFreq) const {
  radio_->setFrequency((float)config_.LoraFreqRx / 1e6);
  int state = radio_->startReceive();
  if (state != RADIOLIB_ERR_NONE) {
    LOG_ERROR("Start receive error:", state);
  }
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
    float snr = radio_->getSNR();
    int rssi = radio_->getRSSI();
#ifdef USE_SX126X
    long frequencyError = 0;
#else
    long frequencyError = radio_->getFrequencyError();
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
    if (config_.EnableRepeater && payload.Digirepeat(ownCallsign_)) {
      sendAX25ToRadio(payload);
      LOG_INFO("Packet digirepeated");
    }
  } else {
    LOG_WARN("Skipping non-AX25 payload");
  }
}

bool Service::onRigTxBegin()
{
  currentTxPacketSize_ = 0;
  if (splitEnabled()) {
    setFreq(config_.LoraFreqTx);
  }
  if (config_.PttEnable) {
    digitalWrite(config_.PttPin, HIGH);
    delay(config_.PttTxDelayMs);
  }
  return true;
}

void Service::onRigTx(byte b)
{
  LOG_TRACE((char)b, String(b, HEX));
  radioTxQueue_.push(b);
  currentTxPacketSize_++;
}

void Service::onRigTxEnd()
{
  radioTxQueueIndex_.push(currentTxPacketSize_);
  uint32_t radioTransmitBit = RadioTaskBits::Transmit;
  xTaskNotify(radioTaskHandle_, radioTransmitBit, eSetBits);
}

void Service::attachKissNetworkClient() 
{
  // connected, client dropped off
  if (isKissConn_) {
    if (!kissConn_.connected()) {
      LOG_INFO("KISS TCP/IP client disconnected");
      isKissConn_ = false;
      kissConn_.stop();
    }
  }
  WiFiClient wifiClient = kissServer_->available();
  // new client connected
  if (wifiClient && wifiClient.connected()) {
    // drop off current one
    if (isKissConn_) {
      kissConn_.stop();
    }
    LOG_INFO("New KISS TCP/IP client connected");
    kissConn_ = wifiClient;
    isKissConn_ = true;
  }
}

void Service::onSerialTx(byte b)
{
  LOG_TRACE((char)b, String(b, HEX));
  if (config_.UsbSerialEnable) {
    Serial.write(b);
  } 
  else if (isKissConn_) {
    kissConn_.write(b);
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
  else if (isKissConn_) {
    return kissConn_.available();
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
  else if (isKissConn_) {
    rxResult = kissConn_.read();
    // client dropped off
    if (rxResult == -1) {
      kissConn_.stop();
      isKissConn_ = false;
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
    
    config_.LoraFreqRx = be32toh(setHardware->freq);
    config_.LoraBw = be32toh(setHardware->bw);
    config_.LoraSf = be16toh(setHardware->sf);
    config_.LoraCodingRate = be16toh(setHardware->cr);
    config_.LoraPower = be16toh(setHardware->pwr);
    config_.LoraSync = be16toh(setHardware->sync);
    int crcType = setHardware->crc ? config_.LoraCrc : 0;

    setupRadio(config_.LoraFreqRx, config_.LoraBw, config_.LoraSf, 
      config_.LoraCodingRate, config_.LoraPower, config_.LoraSync, crcType, config_.LoraExplicit);
  } else {
    LOG_ERROR("Radio control command of wrong size");
  }
}

void Service::onRebootCommand()
{
  LOG_INFO("Reboot requested");
  ESP.restart();
}

} // LoraPrs
