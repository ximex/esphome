#include "benq_media_player.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include <cstdio>

namespace esphome::benq {

static const char *const TAG = "benq.media_player";

void BenqMediaPlayer::setup() {
  this->parent_->query_command(BenqCommand::POWER);
  this->parent_->query_command(BenqCommand::VOLUME);
  this->parent_->query_command(BenqCommand::MUTE);
  this->parent_->query_command(BenqCommand::SOURCE);
}

void BenqMediaPlayer::dump_config() { ESP_LOGCONFIG(TAG, "BenQ Media Player"); }

media_player::MediaPlayerTraits BenqMediaPlayer::get_traits() {
  auto traits = media_player::MediaPlayerTraits();
  traits.set_supports_pause(false);
  return traits;
}

void BenqMediaPlayer::handle_response(BenqCommand cmd, const BenqResponse &response) {
  if (response.is_error || !response.success) {
    return;
  }

  switch (cmd) {
    case BenqCommand::POWER:
      this->is_on_ = (strcasecmp(response.value, "on") == 0);
      break;
    case BenqCommand::VOLUME: {
      auto parsed = parse_number<int>(response.value);
      if (parsed.has_value()) {
        this->volume_ = parsed.value() / 10.0f;
      }
      break;
    }
    case BenqCommand::MUTE:
      this->is_muted_ = (strcasecmp(response.value, "on") == 0);
      break;
    case BenqCommand::SOURCE:
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
        this->parent_->send_command(BenqCommand::VOLUME, "+");
        break;
      case media_player::MEDIA_PLAYER_COMMAND_VOLUME_DOWN:
        this->parent_->send_command(BenqCommand::VOLUME, "-");
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
    int benq_volume = static_cast<int>(call.get_volume().value() * 10.0f);
    char buffer[8];
    snprintf(buffer, sizeof(buffer), "%d", benq_volume);
    this->parent_->send_command(BenqCommand::VOLUME, buffer);
  }

  if (call.get_media_url().has_value()) {
    const std::string &source = call.get_media_url().value();
    char buffer[16];
    str_to_lower_buf(buffer, sizeof(buffer), source.c_str(), source.size());
    this->parent_->send_command(BenqCommand::SOURCE, buffer);
  }
}

void BenqMediaPlayer::update_state_() {
  this->state = this->is_on_ ? media_player::MEDIA_PLAYER_STATE_ON : media_player::MEDIA_PLAYER_STATE_OFF;
  this->volume = this->volume_;
  this->publish_state();
}

}  // namespace esphome::benq
