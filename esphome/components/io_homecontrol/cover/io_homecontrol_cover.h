#pragma once

#include "esphome/components/cover/cover.h"
#include "esphome/core/component.h"

#include <vector>

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
  void add_address(uint32_t address) { this->addresses_.push_back(address); }

  /// Primary source address used for TX (first configured address)
  uint32_t get_primary_address() const { return this->addresses_.empty() ? 0 : this->addresses_[0]; }

  /// Check if this cover is associated with a given source address
  bool has_address(uint32_t address) const;

  /// Called by hub when an EXECUTE command from a matching source is sniffed
  void update_from_sniffed(uint16_t main_param);

 protected:
  void control(const cover::CoverCall &call) override;

  IOHomecontrol *parent_{nullptr};
  std::vector<uint32_t> addresses_;
  uint32_t last_command_time_{0};
};

}  // namespace esphome::io_homecontrol
