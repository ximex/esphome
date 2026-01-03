#include "benq_number.h"
#include "esphome/core/log.h"
#include <cstdio>

namespace esphome::benq {

static const char *const TAG = "benq.number";

void BenqNumber::dump_config() { LOG_NUMBER("", "BenQ Number", this); }

void BenqNumber::control(float value) {
  char buffer[16];
  snprintf(buffer, sizeof(buffer), "%.0f", value);
  this->parent_->send_command(this->command_, buffer);
}

void BenqNumber::handle_response(const BenqResponse &response) {
  if (response.is_error) {
    ESP_LOGW(TAG, "Number '%s' error: %s", this->get_name().c_str(), response.error_message);
    return;
  }
  if (response.success) {
    auto parsed = parse_number<float>(response.value);
    if (parsed.has_value()) {
      this->publish_state(parsed.value());
    } else {
      ESP_LOGW(TAG, "Number '%s' received non-numeric value: %s", this->get_name().c_str(), response.value);
    }
  }
}

}  // namespace esphome::benq
