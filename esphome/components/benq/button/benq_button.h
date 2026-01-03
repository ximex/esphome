#pragma once

#include "esphome/core/component.h"
#include "esphome/components/button/button.h"
#include "../benq.h"

namespace esphome::benq {

class BenqButton : public button::Button, public Component {
 public:
  void set_parent(BenQ *parent) { this->parent_ = parent; }
  void set_command(BenqCommand command) { this->command_ = command; }
  void set_value(const char *value) { strncpy(this->value_, value, sizeof(this->value_) - 1); }

  void dump_config() override;

 protected:
  void press_action() override;

  BenQ *parent_{nullptr};
  BenqCommand command_{BenqCommand::MAX_COMMAND};
  char value_[16]{};
};

}  // namespace esphome::benq
