#pragma once

#include "esphome/core/component.h"
#include "esphome/components/button/button.h"
#include "../milight_hub.h"
#include "../packet_formatter.h"

namespace esphome::milight {

enum MiLightButtonCommand : uint8_t {
  MILIGHT_CMD_PAIR = 0,
  MILIGHT_CMD_UNPAIR = 1,
};

class MiLightButton : public Component, public button::Button {
 public:
  void set_hub(MiLightHub *hub) { this->hub_ = hub; }
  void set_device_id(uint16_t id) { this->device_id_ = id; }
  void set_group_id(uint8_t id) { this->group_id_ = id; }
  void set_remote_type(RemoteType type) { this->remote_type_ = type; }
  void set_command(MiLightButtonCommand cmd) { this->command_ = cmd; }

  void dump_config() override;

 protected:
  void press_action() override;

  MiLightHub *hub_{nullptr};
  uint16_t device_id_{0};
  uint8_t group_id_{0};
  RemoteType remote_type_{RemoteType::RGBW};
  MiLightButtonCommand command_{MILIGHT_CMD_PAIR};
};

}  // namespace esphome::milight
