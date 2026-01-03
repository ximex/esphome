#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "../benq.h"

namespace esphome::benq {

class BenqSensor : public sensor::Sensor, public Component {
 public:
  void dump_config() override;

  void set_parent(BenQ *parent) { this->parent_ = parent; }
  void set_command(BenqCommand command) { this->command_ = command; }
  BenqCommand get_command() const { return this->command_; }
  void handle_response(const BenqResponse &response);

 protected:
  BenQ *parent_{nullptr};
  BenqCommand command_{BenqCommand::MAX_COMMAND};
};

}  // namespace esphome::benq
