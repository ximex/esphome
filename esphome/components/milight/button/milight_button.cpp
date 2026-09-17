#include "milight_button.h"
#include "esphome/core/log.h"

namespace esphome::milight {

static const char *const TAG = "milight.button";

void MiLightButton::dump_config() {
  ESP_LOGCONFIG(TAG,
                "MiLight Button:\n"
                "  Device ID: 0x%04X\n"
                "  Group ID: %u\n"
                "  Remote Type: %s\n"
                "  Command: %s",
                this->device_id_, this->group_id_, LOG_STR_ARG(remote_type_to_string(this->remote_type_)),
                this->command_ == MILIGHT_CMD_PAIR ? LOG_STR_LITERAL("pair") : LOG_STR_LITERAL("unpair"));
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
