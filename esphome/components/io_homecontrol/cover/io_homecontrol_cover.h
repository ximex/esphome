#pragma once

#include "esphome/components/cover/cover.h"
#include "esphome/core/component.h"

namespace esphome::io_homecontrol {

class IOHomecontrol;

class IOHomecontrolCover : public cover::Cover, public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  cover::CoverTraits get_traits() override;

  void set_parent(IOHomecontrol *parent) { this->parent_ = parent; }
  void set_address(uint32_t address) { this->address_ = address; }
  uint32_t get_address() const { return this->address_; }

  /// Called by hub when an EXECUTE command targeting this cover is sniffed
  void update_from_sniffed(uint16_t main_param);

 protected:
  void control(const cover::CoverCall &call) override;

  IOHomecontrol *parent_{nullptr};
  uint32_t address_{0};
  uint32_t last_command_time_{0};
};

}  // namespace esphome::io_homecontrol
