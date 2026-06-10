#pragma once

#include "esphome/components/cover/cover.h"
#include "esphome/components/remote_base/remote_base.h"
#include "esphome/core/component.h"

namespace esphome::lifta {

class LiftaCover : public cover::Cover,
                   public Component,
                   public remote_base::RemoteTransmittable,
                   public remote_base::RemoteReceiverListener {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  cover::CoverTraits get_traits() override;

  void set_code(uint32_t code) { this->code_ = code; }
  void set_frame_interval(uint32_t frame_interval) { this->frame_interval_ = frame_interval; }
  void set_open_duration(uint32_t open_duration) { this->open_duration_ = open_duration; }
  void set_close_duration(uint32_t close_duration) { this->close_duration_ = close_duration; }
  void set_open_margin(uint32_t open_margin) { this->open_margin_ = open_margin; }
  void set_close_margin(uint32_t close_margin) { this->close_margin_ = close_margin; }

  bool on_receive(remote_base::RemoteReceiveData data) override;

 protected:
  void control(const cover::CoverCall &call) override;
  void start_direction_(cover::CoverOperation dir);
  void stop_();
  void transmit_frame_(cover::CoverOperation dir);
  void recompute_position_();
  bool is_at_target_() const;

  uint32_t code_{0};
  uint32_t frame_interval_{0};
  uint32_t open_duration_{0};
  uint32_t close_duration_{0};
  uint32_t open_margin_{0};
  uint32_t close_margin_{0};

  float target_position_{0.0f};
  uint32_t last_recompute_time_{0};
  uint32_t last_publish_time_{0};
  uint32_t last_tx_time_{0};
  uint32_t run_deadline_{0};
  uint32_t endstop_deadline_{0};  // 0 = no endstop overrun active
  uint32_t rx_last_frame_{0};
  bool rx_driven_{false};
  cover::CoverOperation last_operation_{cover::COVER_OPERATION_OPENING};
};

}  // namespace esphome::lifta
