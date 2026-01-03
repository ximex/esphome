#include "benq_select.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

namespace esphome::benq {

static const char *const TAG = "benq.select";

void BenqSelect::dump_config() { LOG_SELECT("", "BenQ Select", this); }

void BenqSelect::control(const std::string &value) {
  // Convert to lowercase for protocol and replace ':' with '_' for aspect ratios
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
  if (response.success) {
    // Normalize: uppercase and replace '_' with ':'
    char display[32];
    size_t len = strlen(response.value);
    if (len >= sizeof(display)) {
      len = sizeof(display) - 1;
    }
    for (size_t i = 0; i < len; i++) {
      char c = response.value[i];
      if (c >= 'a' && c <= 'z') {
        c = c - ('a' - 'A');
      }
      if (c == '_') {
        c = ':';
      }
      display[i] = c;
    }
    display[len] = '\0';

    // Match against known options
    for (const auto &option : this->traits.get_options()) {
      if (str_equals_case_insensitive(option, display)) {
        this->publish_state(option);
        return;
      }
    }
    ESP_LOGW(TAG, "Select '%s' received unknown value: %s", this->get_name().c_str(), display);
  }
}

}  // namespace esphome::benq
