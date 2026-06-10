#include "lifta_cover.h"
#include "esphome/components/remote_base/lifta_protocol.h"
#include "esphome/core/application.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

#include <cinttypes>

namespace esphome::lifta {

static const char *const TAG = "lifta.cover";

using namespace esphome::cover;
using remote_base::LiftaCommand;
using remote_base::LiftaData;
using remote_base::LiftaProtocol;

// A remote sends a frame about every 150 ms while a button is held; if no frame has been
// seen for this long, the lift has stopped (dead-man control).
static constexpr uint32_t RX_IDLE_TIMEOUT_MS = 1000;

void LiftaCover::setup() {
  auto restore = this->restore_state_();
  if (restore.has_value()) {
    restore->apply(this);
  } else {
    // The lift usually parks at the bottom; with assumed state a wrong guess is harmless
    this->position = COVER_CLOSED;
  }
}

void LiftaCover::loop() {
  const uint32_t now = App.get_loop_component_start_time();

  if (this->rx_driven_) {
    // The remote stopped sending frames, so the lift has stopped.
    // Position was already integrated per received frame in on_receive().
    if (now - this->rx_last_frame_ > RX_IDLE_TIMEOUT_MS) {
      this->rx_driven_ = false;
      this->current_operation = COVER_OPERATION_IDLE;
      this->publish_state();
    }
    return;
  }

  if (this->current_operation == COVER_OPERATION_IDLE)
    return;

  // Dead-man control: the lift only moves while frames keep arriving
  if (now - this->last_tx_time_ >= this->frame_interval_) {
    this->transmit_frame_(this->current_operation);
    this->last_tx_time_ = now;
  }

  this->recompute_position_();

  if (this->endstop_deadline_ != 0) {
    // Overrun phase: keep transmitting so the lift reliably reaches its physical endstop,
    // then snap the position estimate to the exact end position
    if ((int32_t) (now - this->endstop_deadline_) >= 0) {
      this->position = this->target_position_;
      this->stop_();
      return;
    }
  } else if (this->is_at_target_()) {
    if (this->target_position_ == COVER_OPEN || this->target_position_ == COVER_CLOSED) {
      const uint32_t margin =
          this->current_operation == COVER_OPERATION_OPENING ? this->open_margin_ : this->close_margin_;
      this->endstop_deadline_ = now + margin;
    } else {
      this->stop_();
      return;
    }
  } else if ((int32_t) (now - this->run_deadline_) >= 0) {
    // Safety cap: never transmit longer than the full travel time plus margin
    this->stop_();
    return;
  }

  if (now - this->last_publish_time_ > 1000) {
    this->publish_state(false);
    this->last_publish_time_ = now;
  }
}

CoverTraits LiftaCover::get_traits() {
  auto traits = CoverTraits();
  traits.set_supports_stop(true);
  traits.set_supports_position(true);
  traits.set_supports_toggle(true);
  traits.set_is_assumed_state(true);
  return traits;
}

void LiftaCover::control(const CoverCall &call) {
  if (call.get_stop()) {
    this->stop_();
  }
  if (call.get_toggle().has_value()) {
    if (this->current_operation != COVER_OPERATION_IDLE) {
      this->stop_();
    } else {
      if (this->position == COVER_CLOSED || this->last_operation_ == COVER_OPERATION_CLOSING) {
        this->target_position_ = COVER_OPEN;
        this->start_direction_(COVER_OPERATION_OPENING);
      } else {
        this->target_position_ = COVER_CLOSED;
        this->start_direction_(COVER_OPERATION_CLOSING);
      }
    }
  }
  auto pos_val = call.get_position();
  if (pos_val.has_value()) {
    auto pos = *pos_val;
    if (pos == this->position) {
      // The lift has physical endstops, so re-issuing a full open/close re-sends the
      // command even if the estimate says it is already there
      if (pos == COVER_OPEN || pos == COVER_CLOSED) {
        auto op = pos == COVER_CLOSED ? COVER_OPERATION_CLOSING : COVER_OPERATION_OPENING;
        this->target_position_ = pos;
        this->start_direction_(op);
      }
    } else {
      auto op = pos < this->position ? COVER_OPERATION_CLOSING : COVER_OPERATION_OPENING;
      this->target_position_ = pos;
      this->start_direction_(op);
    }
  }
}

void LiftaCover::start_direction_(CoverOperation dir) {
  if (dir != COVER_OPERATION_OPENING && dir != COVER_OPERATION_CLOSING)
    return;
  if (dir == this->current_operation && !this->rx_driven_) {
    // Already transmitting in that direction; the (possibly new) target is picked up by loop()
    return;
  }

  this->recompute_position_();
  this->rx_driven_ = false;
  this->last_operation_ = dir;
  this->current_operation = dir;

  const uint32_t now = millis();
  this->last_recompute_time_ = now;
  const uint32_t duration = dir == COVER_OPERATION_OPENING ? this->open_duration_ : this->close_duration_;
  const uint32_t margin = dir == COVER_OPERATION_OPENING ? this->open_margin_ : this->close_margin_;
  this->run_deadline_ = now + duration + margin;
  this->endstop_deadline_ = 0;

  this->transmit_frame_(dir);
  this->last_tx_time_ = now;
  this->publish_state();
}

void LiftaCover::stop_() {
  // There is no stop frame; ceasing transmission stops the lift (dead-man control)
  this->current_operation = COVER_OPERATION_IDLE;
  this->endstop_deadline_ = 0;
  this->rx_driven_ = false;
  this->publish_state();
}

void LiftaCover::transmit_frame_(CoverOperation dir) {
  const LiftaCommand command = dir == COVER_OPERATION_OPENING ? LiftaCommand::UP : LiftaCommand::DOWN;
  this->transmit_<LiftaProtocol>(LiftaData{.code = this->code_, .command = command});
}

void LiftaCover::recompute_position_() {
  float dir;
  float action_dur;
  switch (this->current_operation) {
    case COVER_OPERATION_OPENING:
      dir = 1.0f;
      action_dur = this->open_duration_;
      break;
    case COVER_OPERATION_CLOSING:
      dir = -1.0f;
      action_dur = this->close_duration_;
      break;
    default:
      return;
  }

  const uint32_t now = millis();
  this->position += dir * (now - this->last_recompute_time_) / action_dur;
  this->position = clamp(this->position, 0.0f, 1.0f);
  this->last_recompute_time_ = now;
}

bool LiftaCover::is_at_target_() const {
  switch (this->current_operation) {
    case COVER_OPERATION_OPENING:
      return this->position >= this->target_position_;
    case COVER_OPERATION_CLOSING:
      return this->position <= this->target_position_;
    default:
      return true;
  }
}

bool LiftaCover::on_receive(remote_base::RemoteReceiveData data) {
  auto decoded = LiftaProtocol().decode(data);
  if (!decoded.has_value() || decoded->code != this->code_)
    return false;
  // Ignore our own transmissions echoed back by the transceiver
  if (this->current_operation != COVER_OPERATION_IDLE && !this->rx_driven_)
    return true;

  const uint32_t now = millis();
  switch (decoded->command) {
    case LiftaCommand::UP:
    case LiftaCommand::DOWN: {
      const auto op = decoded->command == LiftaCommand::UP ? COVER_OPERATION_OPENING : COVER_OPERATION_CLOSING;
      if (!this->rx_driven_ || this->current_operation != op) {
        // First frame of a remote-driven run: start the position clock
        this->rx_driven_ = true;
        this->current_operation = op;
        this->last_operation_ = op;
        this->last_recompute_time_ = now;
        this->publish_state();
        this->last_publish_time_ = now;
      } else {
        // Integrate the position per received frame, so the RX idle timeout
        // adds no phantom travel after the remote button is released
        this->recompute_position_();
        if (now - this->last_publish_time_ > 1000) {
          this->publish_state(false);
          this->last_publish_time_ = now;
        }
      }
      this->rx_last_frame_ = now;
      break;
    }
    case LiftaCommand::BEACON:
      // Directionless heartbeat: if we were tracking remote movement, it has stopped
      if (this->rx_driven_) {
        this->rx_driven_ = false;
        this->current_operation = COVER_OPERATION_IDLE;
        this->publish_state();
      }
      break;
    default:
      break;
  }
  return true;
}

void LiftaCover::dump_config() {
  LOG_COVER("", "Lifta Cover", this);
  ESP_LOGCONFIG(TAG,
                "  Code: 0x%08" PRIX32 "\n"
                "  Frame Interval: %" PRIu32 " ms\n"
                "  Open Duration: %.1fs (margin %.1fs)\n"
                "  Close Duration: %.1fs (margin %.1fs)",
                this->code_, this->frame_interval_, this->open_duration_ / 1e3f, this->open_margin_ / 1e3f,
                this->close_duration_ / 1e3f, this->close_margin_ / 1e3f);
}

}  // namespace esphome::lifta
