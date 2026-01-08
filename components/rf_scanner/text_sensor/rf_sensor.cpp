#include "rf_sensor.h"
#include "esphome/core/log.h"

namespace esphome {
namespace rf_scanner {

static const char *const TAG = "RfSensor";

RfSensor::RfSensor(int16_t txid, int16_t chan) : txid_(txid), chan_(chan) {}

float RfSensor::get_setup_priority() const { return setup_priority::DATA; }

void RfSensor::setup() {
	parent_->register_sensor(this);
    last_ms_ = millis();	// initialize timeout counter
	clear_data();			// report status sensor offline
}

void RfSensor::loop() {
  uint32_t time_ms = millis();

  // check for sensor being offline
  if (time_ms > last_ms_ + timeout_ms_) {
    clear_data();
  }

  rem_history_();
}

void RfSensor::dump_config() {
  LOG_TEXT_SENSOR("", "RF Sensor", this);
  ESP_LOGCONFIG(TAG, "  txid: %d", this->txid_);
  ESP_LOGCONFIG(TAG, "  chan: %d", this->chan_);
  
  LOG_SENSOR("  ", "Temperature", this->temperature_sensor_);
  LOG_SENSOR("  ", "Humidity", this->humidity_sensor_);
  LOG_SENSOR("  ", "Precipitation", this->precipitation_sensor_);
}

void RfSensor::update() {
	String res;
    float precipitation_history = get_history_();
    float precipitation_intensity = get_history_rate_();
	last_ms_ = millis();	// update timeout counter
	
	if (hasdata_) {
		status_clear_warning();
		res = "{ txid: " + String(data_.txid) + ", chan: " + String(data_.channel);
		if (data_.channel == 3) {
			add_history_(data_.precipitation, data_.timestamp);
	  		res += ", perc: " + String(data_.precipitation, 2);
	  		res += ", hist: " + String(precipitation_history, 2);
	  		res += ", rate: " + String(precipitation_intensity, 2);
		} else {
	  		res += ", temp: " + String(data_.temperature, 1);
			res += ", humi: " + String(data_.humidity, 0);
		}
		res += ", batt: " + String(data_.battery ? "LOW" : "OK");
		res += ", utc: " + String(data_.timestamp) + " }";
	} else {
		status_set_warning();
		res = "sensor offline";
	}

	if (temperature_sensor_ != nullptr)
		temperature_sensor_->publish_state(data_.temperature);
	if (humidity_sensor_ != nullptr)
		humidity_sensor_->publish_state(data_.humidity);
	if (precipitation_sensor_ != nullptr)
		precipitation_sensor_->publish_state(data_.precipitation);
	if (history_sensor_ != nullptr)
		history_sensor_->publish_state(precipitation_history);
	if (intensity_sensor_ != nullptr)
		intensity_sensor_->publish_state(precipitation_intensity);

	publish_state(res.c_str());
}

void RfSensor::update_data(struct SensorMessage *data) {
	data_ = *data;
	hasdata_ = true;
	update();
}

void RfSensor::clear_data() {
	SensorMessage data;
	data_ = data;
	hasdata_ = false;
	update();
}

void RfSensor::add_history_(float value, time_t timestamp) {
    if (history_sensor_ != nullptr) {
		if (last_value_ <= 0) {
			// initialize last value
            last_value_ = value;
		}
	    if (value > last_value_) {
			// value changed - add new history record 
		    struct history_rec rec;
		    rec.timestamp = timestamp;
		    rec.value = value - last_value_;
		    history_.push_back(rec);
			ESP_LOGD(TAG, "add_history_ %d %0.2f", rec.timestamp, rec.value);
		    last_value_ = value;
	    }
	}
}

void RfSensor::rem_history_() {
    if (history_sensor_ != nullptr) {
		time_t now;
		time(&now);
        time_t timestamp_min = now - 48 * 60 * 60; // last two days
	    if (history_.size() > 0 && history_[0].timestamp < timestamp_min) {
			// remove expired history record
			ESP_LOGD(TAG, "rem_history_ %d %0.2f", history_[0].timestamp, history_[0].value);
	        history_.erase(history_.begin());
	    }
    }
}

float RfSensor::get_history_() {
    float hist = 0;
    if (history_sensor_ != nullptr) {
        for (auto rec : history_) {
			// accumulate history records
	        hist += rec.value;
        }
	}
    return hist;
}

float RfSensor::get_history_rate_() {
    float rate = 0;
	time_t now;
	time(&now);
    time_t timestamp_min = now - 60 * 60; // last hour
	time_t timestamp_beg = 0;
    if (history_sensor_ != nullptr) {
        for (auto rec : history_) {
			if (rec.timestamp > timestamp_min) {
                rate += rec.value;
				if (timestamp_beg == 0) {
                    timestamp_beg = rec.timestamp;
				}
			}
        }
	}

    return (now > (timestamp_beg + 60)) ? rate * 3600 / (now - timestamp_beg) : 0;
}


}  // namespace rf_scanner
}  // namespace esphome
