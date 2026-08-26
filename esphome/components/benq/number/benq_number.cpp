#include "benq_number.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include <cmath>
#include <cstdio>

namespace esphome::benq {

static const char *const TAG = "benq.number";

void BenqNumber::dump_config() { LOG_NUMBER("", "BenQ Number", this); }

void BenqNumber::control(float value) {
  if (this->command_ == BenqCommand::VOLUME) {
    this->parent_->set_volume(static_cast<int8_t>(lroundf(value)));
    return;
  }
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
    if (!parsed.has_value()) {
      ESP_LOGW(TAG, "Number '%s' received non-numeric value: %s", this->get_name().c_str(), response.value);
      return;
    }
    // Every poll reads this setting back. publish_state() reaches every
    // controller, so only report a value that actually moved.
    if (this->has_state() && this->state == parsed.value()) {
      return;
    }
    this->publish_state(parsed.value());
  }
}

}  // namespace esphome::benq
