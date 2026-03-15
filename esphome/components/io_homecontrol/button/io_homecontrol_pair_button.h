#pragma once

#include "esphome/components/button/button.h"
#include "esphome/core/component.h"

namespace esphome::io_homecontrol {

class IOHomecontrol;

class IOHomecontrolPairButton : public button::Button, public Component {
 public:
  void dump_config() override;

  void set_parent(IOHomecontrol *parent) { this->parent_ = parent; }
  void set_target_address(uint32_t address) { this->target_address_ = address; }

 protected:
  void press_action() override;

  IOHomecontrol *parent_{nullptr};
  uint32_t target_address_{0x00003F};  // Default: broadcast
};

}  // namespace esphome::io_homecontrol
