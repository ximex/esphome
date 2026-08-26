#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "../benq.h"

namespace esphome::benq {

class BenqSensor : public sensor::Sensor, public Component, public BenqEntity {
 public:
  BenqSensor(BenQ *parent, BenqCommand command) : BenqEntity(parent, command) {}

  void dump_config() override;
  void handle_response(const BenqResponse &response) override;
};

}  // namespace esphome::benq
