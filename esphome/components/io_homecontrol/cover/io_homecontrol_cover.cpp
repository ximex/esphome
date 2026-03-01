#include "io_homecontrol_cover.h"
#include "../io_homecontrol.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome::io_homecontrol {

static const char *const TAG = "io_homecontrol.cover";

void IOHomecontrolCover::setup() {
  // Register with the hub
  this->parent_->register_cover(this);

  // Restore last known position from flash
  auto restore = this->restore_state_();
  if (restore.has_value()) {
    restore->apply(this);
  } else {
    // Default: assume open
    this->position = cover::COVER_OPEN;
    this->publish_state();
  }
}

static constexpr uint32_t OPERATION_TIMEOUT_MS = 60000;

void IOHomecontrolCover::loop() {
  if (this->current_operation == cover::COVER_OPERATION_IDLE || this->last_command_time_ == 0)
    return;
  if (millis() - this->last_command_time_ < OPERATION_TIMEOUT_MS)
    return;

  // No new command received within timeout — assume operation completed
  if (this->current_operation == cover::COVER_OPERATION_OPENING) {
    ESP_LOGI(TAG, "0x%06X: operation timeout, assuming OPEN", this->address_);
    this->position = cover::COVER_OPEN;
  } else if (this->current_operation == cover::COVER_OPERATION_CLOSING) {
    ESP_LOGI(TAG, "0x%06X: operation timeout, assuming CLOSED", this->address_);
    this->position = cover::COVER_CLOSED;
  }
  this->current_operation = cover::COVER_OPERATION_IDLE;
  this->last_command_time_ = 0;
  this->publish_state();
}

void IOHomecontrolCover::dump_config() {
  LOG_COVER("", "io-homecontrol Cover", this);
  ESP_LOGCONFIG(TAG, "  Target Address: 0x%06X", this->address_);
}

cover::CoverTraits IOHomecontrolCover::get_traits() {
  auto traits = cover::CoverTraits();
  traits.set_supports_stop(true);
  traits.set_supports_position(true);
  traits.set_supports_tilt(false);
  traits.set_supports_toggle(false);
  traits.set_is_assumed_state(true);
  return traits;
}

void IOHomecontrolCover::control(const cover::CoverCall &call) {
  if (call.get_stop()) {
    ESP_LOGI(TAG, "Sending STOP to 0x%06X", this->address_);
    this->parent_->send_execute(this->address_, static_cast<uint16_t>(MainParam::STOP));
    this->current_operation = cover::COVER_OPERATION_IDLE;
    this->publish_state();
    return;
  }

  if (call.get_position().has_value()) {
    float pos = *call.get_position();

    if (pos >= cover::COVER_OPEN) {
      // Fully open
      ESP_LOGI(TAG, "Sending OPEN to 0x%06X", this->address_);
      this->parent_->send_execute(this->address_, static_cast<uint16_t>(MainParam::OPEN));
      this->current_operation = cover::COVER_OPERATION_OPENING;
      this->position = cover::COVER_OPEN;
    } else if (pos <= cover::COVER_CLOSED) {
      // Fully closed
      ESP_LOGI(TAG, "Sending CLOSE to 0x%06X", this->address_);
      this->parent_->send_execute(this->address_, static_cast<uint16_t>(MainParam::CLOSE));
      this->current_operation = cover::COVER_OPERATION_CLOSING;
      this->position = cover::COVER_CLOSED;
    } else {
      // Intermediate position
      // io-homecontrol: 0x0000 = fully open, 0xC800 = fully closed
      // ESPHome cover:  1.0 = fully open,    0.0 = fully closed
      // So we invert: param = (1.0 - pos) * 0xC800
      uint16_t main_param = static_cast<uint16_t>((1.0f - pos) * static_cast<uint16_t>(MainParam::CLOSE));
      ESP_LOGI(TAG, "Sending POSITION %.0f%% (param=0x%04X) to 0x%06X", pos * 100.0f, main_param, this->address_);
      this->parent_->send_execute(this->address_, main_param);
      this->current_operation =
          (pos > this->position) ? cover::COVER_OPERATION_OPENING : cover::COVER_OPERATION_CLOSING;
      this->position = pos;
    }

    this->publish_state();
  }
}

void IOHomecontrolCover::update_from_sniffed(uint16_t main_param) {
  // Convert io-homecontrol main_param to ESPHome cover position
  // io-homecontrol: 0x0000 = fully open, 0xC800 = fully closed
  // ESPHome cover:  1.0 = fully open,    0.0 = fully closed
  auto param = static_cast<MainParam>(main_param);

  if (param == MainParam::STOP || param == MainParam::MY_POS) {
    this->current_operation = cover::COVER_OPERATION_IDLE;
    this->last_command_time_ = 0;
  } else if (param == MainParam::OPEN) {
    this->position = cover::COVER_OPEN;
    this->current_operation = cover::COVER_OPERATION_OPENING;
    this->last_command_time_ = millis();
  } else if (param == MainParam::CLOSE) {
    this->position = cover::COVER_CLOSED;
    this->current_operation = cover::COVER_OPERATION_CLOSING;
    this->last_command_time_ = millis();
  } else if (main_param < static_cast<uint16_t>(MainParam::CLOSE)) {
    // Intermediate position: invert mapping
    float pos = 1.0f - (float) main_param / (float) static_cast<uint16_t>(MainParam::CLOSE);
    this->current_operation = (pos > this->position) ? cover::COVER_OPERATION_OPENING : cover::COVER_OPERATION_CLOSING;
    this->position = pos;
    this->last_command_time_ = millis();
  } else {
    return;  // Unknown param, don't update
  }

  ESP_LOGI(TAG, "Sniffed command for 0x%06X: param=0x%04X -> position=%.0f%%", this->address_, main_param,
           this->position * 100.0f);
  this->publish_state();
}

}  // namespace esphome::io_homecontrol
