#pragma once

#include "esphome/core/component.h"
#include "remote_base.h"

#include <cinttypes>

namespace esphome::remote_base {

enum class LiftaCommand : uint8_t {
  BEACON = 0b00,
  DOWN = 0b01,
  UP = 0b10,
};

struct LiftaData {
  // 31-bit frame left-aligned in 32 bits, with the command bits (26-25) and the padding bit (0) zeroed
  uint32_t code;
  // 2-bit command
  LiftaCommand command;

  bool operator==(const LiftaData &rhs) const { return code == rhs.code && command == rhs.command; }
};

class LiftaProtocol : public RemoteProtocol<LiftaData> {
 public:
  void encode(RemoteTransmitData *dst, const LiftaData &data) override;
  optional<LiftaData> decode(RemoteReceiveData src) override;
  void dump(const LiftaData &data) override;
};

DECLARE_REMOTE_PROTOCOL(Lifta)

template<typename... Ts> class LiftaAction : public RemoteTransmitterActionBase<Ts...> {
 public:
  TEMPLATABLE_VALUE(uint32_t, code)
  TEMPLATABLE_VALUE(LiftaCommand, command)

  void encode(RemoteTransmitData *dst, Ts... x) override {
    LiftaData data{};
    data.code = this->code_.value(x...);
    data.command = this->command_.value(x...);
    LiftaProtocol().encode(dst, data);
  }
};

}  // namespace esphome::remote_base
