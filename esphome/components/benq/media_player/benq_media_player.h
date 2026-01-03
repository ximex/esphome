#pragma once

#include "esphome/core/component.h"
#include "esphome/components/media_player/media_player.h"
#include "../benq.h"

namespace esphome::benq {

class BenqMediaPlayer : public media_player::MediaPlayer, public Component {
 public:
  void setup() override;
  void dump_config() override;

  void set_parent(BenQ *parent) { this->parent_ = parent; }

  media_player::MediaPlayerTraits get_traits() override;
  bool is_muted() const override { return this->is_muted_; }

  // Called by hub dispatch
  void handle_response(BenqCommand cmd, const BenqResponse &response);

 protected:
  void control(const media_player::MediaPlayerCall &call) override;
  void update_state_();

  BenQ *parent_{nullptr};
  bool is_on_{false};
  bool is_muted_{false};
  float volume_{0.5f};
};

}  // namespace esphome::benq
