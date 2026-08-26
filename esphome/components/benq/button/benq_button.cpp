#include "benq_button.h"
#include "esphome/core/log.h"

namespace esphome::benq {

static const char *const TAG = "benq.button";

void BenqButton::dump_config() {
  LOG_BUTTON("", "BenQ Button", this);
  if (this->value_[0] != '\0') {
    ESP_LOGCONFIG(TAG, "  Value: %s", this->value_);
  }
}

void BenqButton::press_action() { this->parent_->send_command(this->command_, this->value_); }

}  // namespace esphome::benq
