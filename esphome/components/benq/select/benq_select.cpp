#include "benq_select.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

namespace esphome::benq {

static const char *const TAG = "benq.select";

void BenqSelect::dump_config() { LOG_SELECT("", "BenQ Select", this); }

void BenqSelect::control(const std::string &value) {
  // The protocol wants lowercase, and it spells aspect ratios "16_9"
  char buffer[32];
  size_t len = str_to_lower_buf(buffer, sizeof(buffer), value.c_str(), value.size());
  for (size_t i = 0; i < len; i++) {
    if (buffer[i] == ':') {
      buffer[i] = '_';
    }
  }
  this->parent_->send_command(this->command_, buffer);
}

void BenqSelect::handle_response(const BenqResponse &response) {
  if (response.is_error) {
    ESP_LOGW(TAG, "Select '%s' error: %s", this->get_name().c_str(), response.error_message);
    return;
  }
  if (!response.success) {
    return;
  }

  // Undo the aspect ratio spelling; the comparison below ignores case
  char display[32];
  size_t len = str_to_lower_buf(display, sizeof(display), response.value, strlen(response.value));
  for (size_t i = 0; i < len; i++) {
    if (display[i] == '_') {
      display[i] = ':';
    }
  }

  for (size_t i = 0; i < this->size(); i++) {
    if (!str_equals_case_insensitive(this->option_at(i), display)) {
      continue;
    }
    // Every poll reads this setting back. publish_state() reaches every
    // controller, so only report an option that actually changed.
    auto active = this->active_index();
    if (active.has_value() && active.value() == i) {
      return;
    }
    this->publish_state(i);
    return;
  }
  ESP_LOGW(TAG, "Select '%s' received unknown value: %s", this->get_name().c_str(), response.value);
}

}  // namespace esphome::benq
