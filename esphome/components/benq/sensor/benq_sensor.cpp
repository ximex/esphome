#include "benq_sensor.h"
#include "esphome/core/log.h"

namespace esphome::benq {

static const char *const TAG = "benq.sensor";

void BenqSensor::dump_config() { LOG_SENSOR("", "BenQ Sensor", this); }

void BenqSensor::handle_response(const BenqResponse &response) {
  if (response.is_error) {
    ESP_LOGW(TAG, "Sensor error: %s", response.error_message);
    return;
  }
  if (response.success) {
    auto value = parse_number<float>(response.value);
    this->publish_state(value.value_or(NAN));
  }
}

}  // namespace esphome::benq
