#include "milight_button.h"
#include "esphome/core/log.h"

namespace esphome::milight {

static const char *const TAG = "milight.button";

void MiLightButton::dump_config() {
  ESP_LOGCONFIG(TAG, "MiLight Button:");
  ESP_LOGCONFIG(TAG, "  Device ID: 0x%04X", this->device_id_);
  ESP_LOGCONFIG(TAG, "  Group ID: %u", this->group_id_);
  ESP_LOGCONFIG(TAG, "  Remote Type: %s", remote_type_to_string(this->remote_type_));
  ESP_LOGCONFIG(TAG, "  Command: %s", this->command_ == MILIGHT_CMD_PAIR ? "pair" : "unpair");
}

void MiLightButton::press_action() {
  if (this->command_ == MILIGHT_CMD_PAIR) {
    ESP_LOGI(TAG, "Sending pair command: dev=0x%04X group=%u", this->device_id_, this->group_id_);
    this->hub_->send_pair(this->device_id_, this->group_id_, this->remote_type_);
  } else {
    ESP_LOGI(TAG, "Sending unpair command: dev=0x%04X group=%u", this->device_id_, this->group_id_);
    this->hub_->send_unpair(this->device_id_, this->group_id_, this->remote_type_);
  }
}

}  // namespace esphome::milight
