#include "rf_scanner.h"
#include "tx_decoder.h"
#include "text_sensor/rf_sensor.h"
#include "esphome/core/log.h"

namespace esphome {
namespace rf_scanner {

Config cfg;

static const char *const TAG = "rf_scanner";

float RfScanner::get_setup_priority() const { return setup_priority::HARDWARE; }

void RfScanner::setup() {
  if (sensor_type_ == DRY_TEST) return;

  // trasnfer settings to global Config class which is used a wrapper do old tx_decoder code
  dpin_->setup();
  cfg.Tx.RecvPin = dpin_->get_pin();
  if (spin_) {
    spin_->setup();
    cfg.Tx.ScanPin = spin_->get_pin();
  }
  if (lpin_) {
    lpin_->setup();
    cfg.Tx.LedPin = lpin_->get_pin();
  }
  cfg.Tx.Type = static_cast<enum eTx433Type>(sensor_type_);
  txd.Setup();
}

void RfScanner::loop() {
  SensorMessage data;

  if (sensor_type_ == DRY_TEST) {
    static uint32_t last_us = 0;
    uint32_t time_us = millis();

    if (time_us > last_us + 10000UL) {
      ESP_LOGI(TAG, "dummy receiver inject data");
      last_us = time_us;
      scan_data(&data);
    } else {
      return;
    }
  } else {
    TXDecoder::rec scan;
    txd.Loop();
    if (txd.GetRecord(scan)) {
      data.txid = scan.txid;
      data.channel = scan.channel;
      if (data.channel == 3) {  // channel 3 has special encoding for rain count
        data.precipitation = scan.humidity * RAINFACTOR;
      } else {
        data.humidity = scan.humidity;
        data.temperature = scan.temperature * 0.1f;
      }
      data.battery = scan.battery;
      data.timestamp = scan.timestamp;
    }
  }

  if (data.timestamp > 0L) {
    for (auto sensor : sensors_) {
      int16_t txid = sensor->get_txid();
      int16_t chan = sensor->get_chan();

      // ATTENTION: specyfying a sensor txid in config.yaml requires an uptate and uploading of a new configuration after battery change!  
      if ( (txid == data.txid && chan == data.channel)  // txid and chan is given (exact match)
        || (txid == data.txid && chan == -1)            // txid is given only (still very reliable because there are 255 ids available)
        || (txid == -1 && chan == data.channel)         // chan is given only (may interfer with sensors nearby)
        || (txid == -1 && chan == -1) ) {               // debug sensor (catches all sensors not found before)
        sensor->update_data(&data);
        break;
      }
    }
  }
}

void RfScanner::dump_config() {
  ESP_LOGCONFIG(TAG, "RfSensor:");
  ESP_LOGCONFIG(TAG,"  Sensor type: %s", sensor_type_to_str(sensor_type_));
  LOG_PIN("  Data Pin: ", dpin_);
  LOG_PIN("  Scan Pin: ", spin_);
  LOG_PIN("  LED  Pin: ", lpin_);
}

const char *RfScanner::sensor_type_to_str(EnumSensorType type) {
  switch (type) {
    case DRY_TEST:
      return "DRY-TEST";
    case GT_WT_02:
      return "GT-WT-02";
    case DG_TH8898:
      return "DG-TH8898";
    default:
      return "UNKNOWN";
  }
}

bool RfScanner::scan_data(struct SensorMessage *message)
{
  static float gauge_count = 0;
  size_t sensor_count = sensors_.size();
  if (sensor_type_ == DRY_TEST && sensor_count > 0) {
    uint8_t index = rand() % sensor_count;

    // generate some test data
    message->txid = sensors_[index]->get_txid();
    message->channel = sensors_[index]->get_chan();
    if (message->channel == 4) {
      message->precipitation = gauge_count++ * 0.25f;
    } else {
      message->humidity = 65.0f + (rand() % 100) * 0.05f;
      message->temperature = 22.1f + (rand() % 100) * 0.01f;
    }
    time_t now;
    time(&now);
    message->timestamp = now;
  }
  return true;
}

}  // namespace rf_scanner
}  // namespace esphome
