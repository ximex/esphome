#include "lifta_protocol.h"
#include "esphome/core/log.h"

namespace esphome::remote_base {

static const char *const TAG = "remote.lifta";

// All durations are multiples of the 256 µs chip time
static constexpr uint32_t SHORT_US = 256;          // 1 chip: '0' bit mark/space
static constexpr uint32_t LONG_US = 768;           // 3 chips: '1' bit mark/space
static constexpr uint32_t SYNC_US = 2304;          // 9 chips: sync mark and space
static constexpr uint32_t FOOTER_EXTRA_US = 1536;  // the final space extends 6 chips into the inter-frame gap
static constexpr uint32_t PREAMBLE_PAIRS = 50;     // 256 µs high/low pairs before the sync
static constexpr uint32_t NBITS = 31;
static constexpr uint32_t COMMAND_MASK = 0x06000000;  // frame bits 26-25
static constexpr uint32_t COMMAND_SHIFT = 25;
// sync item + all bits except the last as full items + the final mark (its space may be truncated)
static constexpr uint32_t MIN_FRAME_ITEMS = 2 + 2 * (NBITS - 1) + 1;

static const char *lifta_command_to_string(LiftaCommand command) {
  switch (command) {
    case LiftaCommand::UP:
      return "UP";
    case LiftaCommand::DOWN:
      return "DOWN";
    case LiftaCommand::BEACON:
      return "BEACON";
    default:
      return "UNKNOWN";
  }
}

void LiftaProtocol::encode(RemoteTransmitData *dst, const LiftaData &data) {
  dst->set_carrier_frequency(0);
  dst->reserve(2 * PREAMBLE_PAIRS + 2 + 2 * NBITS);

  for (uint32_t i = 0; i < PREAMBLE_PAIRS; i++)
    dst->item(SHORT_US, SHORT_US);
  dst->item(SYNC_US, SYNC_US);

  const uint32_t frame = (data.code & ~COMMAND_MASK) | ((static_cast<uint32_t>(data.command) & 0b11) << COMMAND_SHIFT);
  // 31 data bits, MSB first, occupying frame bits 31..1 (bit 0 is padding)
  for (uint32_t mask = 1UL << 31; mask >= (1UL << 1); mask >>= 1) {
    const uint32_t pulse = (frame & mask) ? LONG_US : SHORT_US;
    const uint32_t space = (mask == (1UL << 1)) ? pulse + FOOTER_EXTRA_US : pulse;
    dst->item(pulse, space);
  }
}

optional<LiftaData> LiftaProtocol::decode(RemoteReceiveData src) {
  // The preamble length varies at the receiver, so scan for the sync item and ignore everything before it
  while (src.is_valid(MIN_FRAME_ITEMS - 1)) {
    if (!src.peek_item(SYNC_US, SYNC_US)) {
      src.advance(2);  // advance a full item to keep mark/space alignment
      continue;
    }
    src.advance(2);

    uint32_t frame = 0;
    uint32_t bit = 0;
    for (; bit < NBITS - 1; bit++) {
      if (src.expect_item(LONG_US, LONG_US)) {
        frame = (frame << 1) | 1;
      } else if (src.expect_item(SHORT_US, SHORT_US)) {
        frame = frame << 1;
      } else {
        break;
      }
    }
    if (bit != NBITS - 1)
      continue;  // corrupted payload; keep scanning for a later sync in the same buffer

    // The final space extends into the inter-frame gap and may be truncated by the receiver,
    // so only the mark is checked
    if (src.expect_mark(LONG_US)) {
      frame = (frame << 1) | 1;
    } else if (src.expect_mark(SHORT_US)) {
      frame = frame << 1;
    } else {
      continue;
    }

    frame <<= 1;  // pad to the canonical 32-bit representation
    return LiftaData{
        .code = frame & ~COMMAND_MASK,
        .command = static_cast<LiftaCommand>((frame & COMMAND_MASK) >> COMMAND_SHIFT),
    };
  }
  return {};
}

void LiftaProtocol::dump(const LiftaData &data) {
  ESP_LOGI(TAG, "Received Lifta: code=0x%08" PRIX32 ", command=%s", data.code, lifta_command_to_string(data.command));
}

}  // namespace esphome::remote_base
