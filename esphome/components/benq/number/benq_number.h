#pragma once

#include "esphome/core/component.h"
#include "esphome/components/number/number.h"
#include "../benq.h"

namespace esphome::benq {

class BenqNumber : public number::Number, public Component, public BenqEntity {
 public:
  BenqNumber(BenQ *parent, BenqCommand command) : BenqEntity(parent, command), parent_(parent) {}

  void dump_config() override;
  void handle_response(const BenqResponse &response) override;

 protected:
  void control(float value) override;

  BenQ *parent_;
};

}  // namespace esphome::benq
