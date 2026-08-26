#pragma once

#include "esphome/core/component.h"
#include "esphome/components/select/select.h"
#include "../benq.h"

namespace esphome::benq {

class BenqSelect : public select::Select, public Component, public BenqEntity {
 public:
  BenqSelect(BenQ *parent, BenqCommand command) : BenqEntity(parent, command), parent_(parent) {}

  void dump_config() override;
  void handle_response(const BenqResponse &response) override;

 protected:
  void control(const std::string &value) override;

  BenQ *parent_;
};

}  // namespace esphome::benq
