#include "benq_media_player.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include <algorithm>
#include <cmath>

namespace esphome::benq {

static const char *const TAG = "benq.media_player";

void BenqMediaPlayer::dump_config() { ESP_LOGCONFIG(TAG, "BenQ Media Player"); }

void BenqMediaPlayer::query_state() {
  this->parent_->query_command(BenqCommand::POWER);
  this->parent_->query_command(BenqCommand::VOLUME);
  this->parent_->query_command(BenqCommand::MUTE);
}

media_player::MediaPlayerTraits BenqMediaPlayer::get_traits() {
  auto traits = media_player::MediaPlayerTraits();
  // A projector has no media library or transport, so drop the playback
  // features the base set assumes and add the ones it really has.
  traits.clear_feature_flags(
      media_player::MediaPlayerEntityFeature::PLAY_MEDIA | media_player::MediaPlayerEntityFeature::BROWSE_MEDIA |
      media_player::MediaPlayerEntityFeature::STOP | media_player::MediaPlayerEntityFeature::MEDIA_ANNOUNCE);
  traits.add_feature_flags(media_player::MediaPlayerEntityFeature::TURN_ON |
                           media_player::MediaPlayerEntityFeature::TURN_OFF |
                           media_player::MediaPlayerEntityFeature::VOLUME_STEP);
  return traits;
}

void BenqMediaPlayer::handle_response(BenqCommand cmd, const BenqResponse &response) {
  if (cmd != BenqCommand::POWER && cmd != BenqCommand::VOLUME && cmd != BenqCommand::MUTE) {
    return;
  }
  if (response.is_error) {
    ESP_LOGW(TAG, "'%s': projector rejected %s: %s", this->get_name().c_str(), BenQ::get_command_name(cmd),
             response.error_message);
    return;
  }
  if (!response.success) {
    return;
  }

  switch (cmd) {
    case BenqCommand::POWER:
      this->is_on_ = (strcasecmp(response.value, "on") == 0);
      break;
    case BenqCommand::VOLUME: {
      auto parsed = parse_number<int>(response.value);
      if (!parsed.has_value()) {
        return;
      }
      this->volume = std::clamp(parsed.value() / static_cast<float>(BenQ::VOLUME_MAX), 0.0f, 1.0f);
      break;
    }
    case BenqCommand::MUTE:
      this->is_muted_ = (strcasecmp(response.value, "on") == 0);
      break;
    default:
      return;
  }
  this->update_state_();
}

void BenqMediaPlayer::control(const media_player::MediaPlayerCall &call) {
  if (call.get_command().has_value()) {
    switch (call.get_command().value()) {
      case media_player::MEDIA_PLAYER_COMMAND_TURN_ON:
        this->parent_->send_command(BenqCommand::POWER, "on");
        break;
      case media_player::MEDIA_PLAYER_COMMAND_TURN_OFF:
        this->parent_->send_command(BenqCommand::POWER, "off");
        break;
      case media_player::MEDIA_PLAYER_COMMAND_TOGGLE:
        this->parent_->send_command(BenqCommand::POWER, this->is_on_ ? "off" : "on");
        break;
      case media_player::MEDIA_PLAYER_COMMAND_VOLUME_UP:
        this->parent_->nudge_volume(1);
        break;
      case media_player::MEDIA_PLAYER_COMMAND_VOLUME_DOWN:
        this->parent_->nudge_volume(-1);
        break;
      case media_player::MEDIA_PLAYER_COMMAND_MUTE:
        this->parent_->send_command(BenqCommand::MUTE, "on");
        break;
      case media_player::MEDIA_PLAYER_COMMAND_UNMUTE:
        this->parent_->send_command(BenqCommand::MUTE, "off");
        break;
      default:
        break;
    }
  }

  if (call.get_volume().has_value()) {
    this->parent_->set_volume(static_cast<int8_t>(lroundf(call.get_volume().value() * BenQ::VOLUME_MAX)));
  }
}

void BenqMediaPlayer::update_state_() {
  auto new_state = this->is_on_ ? media_player::MEDIA_PLAYER_STATE_ON : media_player::MEDIA_PLAYER_STATE_OFF;
  // publish_state() notifies every controller, so only speak up on a change
  if (new_state == this->state && this->volume == this->published_volume_ &&
      this->is_muted_ == this->published_muted_) {
    return;
  }
  this->state = new_state;
  this->published_volume_ = this->volume;
  this->published_muted_ = this->is_muted_;
  this->publish_state();
}

}  // namespace esphome::benq
