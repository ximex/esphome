#pragma once

#include "esphome/core/component.h"
#include "esphome/components/switch/switch.h"
#include "../benq.h"

namespace esphome::benq {

class BenqSwitch : public switch_::Switch, public Component, public BenqEntity {
 public:
  BenqSwitch(BenQ *parent, BenqCommand command) : BenqEntity(parent, command), parent_(parent) {}

  void dump_config() override;
  void handle_response(const BenqResponse &response) override;

 protected:
  void write_state(bool state) override;

  BenQ *parent_;
};

}  // namespace esphome::benq
