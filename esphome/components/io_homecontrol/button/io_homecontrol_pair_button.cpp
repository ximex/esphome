#include "io_homecontrol_pair_button.h"
#include "../io_homecontrol.h"
#include "esphome/core/log.h"

namespace esphome::io_homecontrol {

static const char *const TAG = "io_homecontrol.button";

void IOHomecontrolPairButton::press_action() {
  ESP_LOGI(TAG, "Pair button pressed, target=0x%06X", this->target_address_);
  if (!this->parent_->send_pair(this->target_address_)) {
    ESP_LOGE(TAG, "Pairing failed");
  }
}

void IOHomecontrolPairButton::dump_config() {
  LOG_BUTTON("", "io-homecontrol Pair Button", this);
  ESP_LOGCONFIG(TAG, "  Target Address: 0x%06X", this->target_address_);
}

}  // namespace esphome::io_homecontrol
