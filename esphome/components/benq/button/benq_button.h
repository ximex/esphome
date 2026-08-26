#pragma once

#include "esphome/core/component.h"
#include "esphome/components/button/button.h"
#include "../benq.h"

namespace esphome::benq {

class BenqButton : public button::Button, public Component {
 public:
  BenqButton(BenQ *parent, BenqCommand command) : parent_(parent), command_(command) {}

  // The pointer comes from codegen and refers to a string literal
  void set_value(const char *value) { this->value_ = value; }

  void dump_config() override;

 protected:
  void press_action() override;

  BenQ *parent_;
  BenqCommand command_;
  const char *value_{""};
};

}  // namespace esphome::benq
