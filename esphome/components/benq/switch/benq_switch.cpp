#include "benq_switch.h"
#include "esphome/core/log.h"

namespace esphome::benq {

static const char *const TAG = "benq.switch";

void BenqSwitch::dump_config() { LOG_SWITCH("", "BenQ Switch", this); }

void BenqSwitch::write_state(bool state) { this->parent_->send_command(this->command_, state ? "on" : "off"); }

void BenqSwitch::handle_response(const BenqResponse &response) {
  if (response.is_error) {
    ESP_LOGW(TAG, "Switch '%s' error: %s", this->get_name().c_str(), response.error_message);
    return;
  }
  if (response.success) {
    this->publish_state(strcasecmp(response.value, "on") == 0);
  }
}

}  // namespace esphome::benq
