#pragma once

#include "esphome/core/component.h"
#include "esphome/components/media_player/media_player.h"
#include "../benq.h"
#include <cmath>

namespace esphome::benq {

class BenqMediaPlayer : public media_player::MediaPlayer, public Component, public BenqMediaPlayerBase {
 public:
  explicit BenqMediaPlayer(BenQ *parent) : parent_(parent) {}

  void dump_config() override;

  media_player::MediaPlayerTraits get_traits() override;
  bool is_muted() const override { return this->is_muted_; }

  // Called by hub dispatch
  void handle_response(BenqCommand cmd, const BenqResponse &response) override;
  // Called by the hub on every poll cycle
  void query_state() override;

 protected:
  void control(const media_player::MediaPlayerCall &call) override;
  void update_state_();

  BenQ *parent_;
  bool is_on_{false};
  bool is_muted_{false};
  // What the controllers were last told, so unchanged state is not sent again.
  // The volume starts out unknown: the projector only answers "pow" while it
  // is off, so there is nothing to report until it has been on once.
  float published_volume_{NAN};
  bool published_muted_{false};
};

}  // namespace esphome::benq
