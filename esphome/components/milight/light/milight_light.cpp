#include "milight_light.h"
#include "esphome/core/log.h"

#include <cmath>

namespace esphome::milight {

static const char *const TAG = "milight.light";

// ---- Effect names (must be static storage for LightEffect) ----

static const char *const EFFECT_NIGHT_MODE = "Night Mode";
static const char *const EFFECT_DISCO_CYCLE = "Disco Mode";
static const char *const EFFECT_DISCO_NAMES[] = {
    "Rainbow Cycle", "White Pulse", "RGB Pulse",  "Color Strobe", "Random",
    "Red Flash",     "Green Flash", "Blue Flash", "White Flash",
};

// ---- MiLightEffect ----

void MiLightEffect::start() {
  this->output_->set_effect_active(true);
  if (this->mode_ == 0xFF) {
    this->output_->send_night_mode();
  } else if (this->mode_ == 0xFE) {
    this->output_->send_effect(0);  // cycle
  } else {
    this->output_->send_effect(this->mode_);
  }
}

void MiLightEffect::stop() { this->output_->set_effect_active(false); }

// ---- MiLightLight ----

void MiLightLight::setup() { this->hub_->register_light(this, this->device_id_, this->group_id_, this->remote_type_); }

void MiLightLight::dump_config() {
  ESP_LOGCONFIG(TAG, "MiLight Light:");
  ESP_LOGCONFIG(TAG, "  Device ID: 0x%04X", this->device_id_);
  ESP_LOGCONFIG(TAG, "  Group ID: %u", this->group_id_);
  ESP_LOGCONFIG(TAG, "  Remote Type: %s", remote_type_to_string(this->remote_type_));
  ESP_LOGCONFIG(TAG, "  Cold White: %.0f mireds", this->cold_white_mireds_);
  ESP_LOGCONFIG(TAG, "  Warm White: %.0f mireds", this->warm_white_mireds_);
}

LightCommand MiLightLight::make_base_cmd_() const {
  LightCommand cmd{};
  cmd.device_id = this->device_id_;
  cmd.group_id = this->group_id_;
  cmd.remote_type = this->remote_type_;
  return cmd;
}

bool MiLightLight::is_step_based_brightness_() const {
  return this->remote_type_ == RemoteType::CCT || this->remote_type_ == RemoteType::RGB ||
         this->remote_type_ == RemoteType::FUT020;
}

bool MiLightLight::supports_disco_() const {
  return this->remote_type_ != RemoteType::CCT && this->remote_type_ != RemoteType::FUT091;
}

bool MiLightLight::supports_night_mode_() const {
  return this->remote_type_ != RemoteType::RGB && this->remote_type_ != RemoteType::FUT020;
}

bool MiLightLight::has_direct_mode_select_() const {
  return this->remote_type_ == RemoteType::RGB_CCT || this->remote_type_ == RemoteType::FUT089;
}

light::LightTraits MiLightLight::get_traits() {
  light::LightTraits traits;

  switch (this->remote_type_) {
    case RemoteType::RGB_CCT:
    case RemoteType::FUT089:
      traits.set_supported_color_modes({light::ColorMode::RGB, light::ColorMode::COLOR_TEMPERATURE});
      traits.set_min_mireds(this->cold_white_mireds_);
      traits.set_max_mireds(this->warm_white_mireds_);
      break;
    case RemoteType::CCT:
    case RemoteType::FUT091:
      traits.set_supported_color_modes({light::ColorMode::COLOR_TEMPERATURE});
      traits.set_min_mireds(this->cold_white_mireds_);
      traits.set_max_mireds(this->warm_white_mireds_);
      break;
    case RemoteType::RGB:
    case RemoteType::FUT020:
      traits.set_supported_color_modes({light::ColorMode::RGB});
      break;
    case RemoteType::RGBW:
      traits.set_supported_color_modes({light::ColorMode::RGB_WHITE});
      break;
    default:
      traits.set_supported_color_modes({light::ColorMode::ON_OFF});
      break;
  }

  return traits;
}

void MiLightLight::setup_state(light::LightState *state) {
  this->light_state_ = state;

  // Register built-in effects for disco modes and night mode
  if (this->supports_night_mode_()) {
    state->add_effects({new MiLightEffect(EFFECT_NIGHT_MODE, this, 0xFF)});
  }

  if (this->supports_disco_()) {
    if (this->has_direct_mode_select_()) {
      // V2: can select specific disco modes 0-8
      state->add_effects({
          new MiLightEffect(EFFECT_DISCO_NAMES[0], this, 0),
          new MiLightEffect(EFFECT_DISCO_NAMES[1], this, 1),
          new MiLightEffect(EFFECT_DISCO_NAMES[2], this, 2),
          new MiLightEffect(EFFECT_DISCO_NAMES[3], this, 3),
          new MiLightEffect(EFFECT_DISCO_NAMES[4], this, 4),
          new MiLightEffect(EFFECT_DISCO_NAMES[5], this, 5),
          new MiLightEffect(EFFECT_DISCO_NAMES[6], this, 6),
          new MiLightEffect(EFFECT_DISCO_NAMES[7], this, 7),
          new MiLightEffect(EFFECT_DISCO_NAMES[8], this, 8),
      });
    } else {
      // V1: can only cycle through disco modes
      state->add_effects({new MiLightEffect(EFFECT_DISCO_CYCLE, this, 0xFE)});
    }
  }
}

uint8_t MiLightLight::mireds_to_milight_(float mireds) const {
  float clamped = std::max(MI_MIREDS_MIN, std::min(MI_MIREDS_MAX, mireds));
  float ratio = (clamped - MI_MIREDS_MIN) / (MI_MIREDS_MAX - MI_MIREDS_MIN);
  return static_cast<uint8_t>(ratio * 100.0f);
}

// ---- Step-based brightness ----

void MiLightLight::send_step_brightness_(uint8_t target_pct) {
  LightCommand cmd = this->make_base_cmd_();
  uint8_t target_steps = (target_pct * V1_STEP_COUNT + 50) / 100;

  // Drive to minimum: send step_down (V1_STEP_COUNT + 1) times
  cmd.step_down_brightness = true;
  for (uint8_t i = 0; i <= V1_STEP_COUNT; i++) {
    this->hub_->send_command(cmd, STEP_COMMAND_REPEATS);
  }
  cmd.step_down_brightness = false;

  // Step up to target
  cmd.step_up_brightness = true;
  for (uint8_t i = 0; i < target_steps; i++) {
    this->hub_->send_command(cmd, STEP_COMMAND_REPEATS);
  }
}

void MiLightLight::send_step_temperature_(uint8_t target_pct) {
  LightCommand cmd = this->make_base_cmd_();
  uint8_t target_steps = (target_pct * V1_STEP_COUNT + 50) / 100;

  // Drive to minimum (cold): send step_down_temp (V1_STEP_COUNT + 1) times
  cmd.step_down_temp = true;
  for (uint8_t i = 0; i <= V1_STEP_COUNT; i++) {
    this->hub_->send_command(cmd, STEP_COMMAND_REPEATS);
  }
  cmd.step_down_temp = false;

  // Step up to target (warm)
  cmd.step_up_temp = true;
  for (uint8_t i = 0; i < target_steps; i++) {
    this->hub_->send_command(cmd, STEP_COMMAND_REPEATS);
  }
}

// ---- Special commands ----

void MiLightLight::send_effect(uint8_t mode) {
  LightCommand cmd = this->make_base_cmd_();

  if (this->has_direct_mode_select_()) {
    // V2: send specific mode number
    cmd.has_effect = true;
    cmd.effect = mode;
    this->hub_->send_command(cmd);
  } else {
    // V1: cycle to next mode
    cmd.effect_cycle = true;
    this->hub_->send_command(cmd);
  }
}

void MiLightLight::send_night_mode() {
  LightCommand cmd = this->make_base_cmd_();

  if (this->remote_type_ == RemoteType::RGBW) {
    // RGBW: first send OFF, then night mode command
    cmd.turn_off = true;
    this->hub_->send_command(cmd);
    cmd.turn_off = false;
    cmd.night_mode = true;
    this->hub_->send_command(cmd);
  } else {
    // CCT, V2 types: single night mode packet
    cmd.night_mode = true;
    this->hub_->send_command(cmd);
  }
}

void MiLightLight::send_pair() { this->hub_->send_pair(this->device_id_, this->group_id_, this->remote_type_); }

void MiLightLight::send_unpair() { this->hub_->send_unpair(this->device_id_, this->group_id_, this->remote_type_); }

// ---- RGB to hue/saturation helper ----

bool MiLightLight::rgb_to_hs_(float r, float g, float b, float &hue, float &saturation) {
  float max_val = std::max(r, std::max(g, b));
  float min_val = std::min(r, std::min(g, b));
  float delta = max_val - min_val;

  if (max_val <= 0.001f)
    return false;

  saturation = (delta / max_val) * 100.0f;

  if (delta <= 0.001f)
    return false;

  if (max_val == r) {
    hue = 60.0f * fmodf((g - b) / delta, 6.0f);
  } else if (max_val == g) {
    hue = 60.0f * ((b - r) / delta + 2.0f);
  } else {
    hue = 60.0f * ((r - g) / delta + 4.0f);
  }
  if (hue < 0)
    hue += 360.0f;
  return true;
}

// ---- write_state per-mode helpers ----

void MiLightLight::write_state_rgb_white_(light::LightState *state, float brightness) {
  LightCommand cmd = this->make_base_cmd_();
  float white = state->current_values.get_white();
  if (white > 0.01f) {
    // White mode
    cmd.white_mode = true;
    this->hub_->send_command(cmd);
    cmd.white_mode = false;
    cmd.has_brightness = true;
    cmd.brightness = static_cast<uint8_t>(brightness * 100.0f);
    this->hub_->send_command(cmd);
  } else {
    // Color mode
    float r, g, b;
    state->current_values_as_rgb(&r, &g, &b);
    float hue, sat;
    if (rgb_to_hs_(r, g, b, hue, sat)) {
      cmd.has_rgb = true;
      cmd.hue = static_cast<uint16_t>(hue);
      this->hub_->send_command(cmd);
      cmd.has_rgb = false;
    }
    cmd.has_brightness = true;
    cmd.brightness = static_cast<uint8_t>(brightness * 100.0f);
    this->hub_->send_command(cmd);
  }
}

void MiLightLight::write_state_rgb_(light::LightState *state, float brightness) {
  LightCommand cmd = this->make_base_cmd_();
  float r, g, b;
  state->current_values_as_rgb(&r, &g, &b);
  float hue, sat;
  if (rgb_to_hs_(r, g, b, hue, sat)) {
    cmd.has_rgb = true;
    cmd.hue = static_cast<uint16_t>(hue);
    this->hub_->send_command(cmd);
    cmd.has_rgb = false;

    // V2 RGB_CCT and FUT089 support saturation; send the actual value
    // derived from the RGB color to match what the user picked in HA
    if (this->remote_type_ == RemoteType::RGB_CCT || this->remote_type_ == RemoteType::FUT089) {
      cmd.has_saturation = true;
      cmd.saturation = static_cast<uint8_t>(sat);
      this->hub_->send_command(cmd);
      cmd.has_saturation = false;
    }
  }

  uint8_t brightness_pct = static_cast<uint8_t>(brightness * 100.0f);
  if (this->is_step_based_brightness_()) {
    this->send_step_brightness_(brightness_pct);
  } else {
    cmd.has_brightness = true;
    cmd.brightness = brightness_pct;
    this->hub_->send_command(cmd);
  }
}

void MiLightLight::write_state_color_temp_(light::LightState *state, float brightness) {
  LightCommand cmd = this->make_base_cmd_();
  float color_temp = state->current_values.get_color_temperature();
  uint8_t brightness_pct = static_cast<uint8_t>(brightness * 100.0f);

  if (this->is_step_based_brightness_()) {
    uint8_t temp_pct = this->mireds_to_milight_(color_temp);
    this->send_step_temperature_(temp_pct);
    this->send_step_brightness_(brightness_pct);
  } else {
    // FUT089: cmd 0x07 is shared between kelvin and saturation, disambiguated
    // by bulb mode. Send white_mode first to ensure bulb is in white mode.
    if (this->remote_type_ == RemoteType::FUT089) {
      cmd.white_mode = true;
      this->hub_->send_command(cmd);
      cmd.white_mode = false;
    }
    cmd.has_color_temp = true;
    cmd.color_temp = this->mireds_to_milight_(color_temp);
    this->hub_->send_command(cmd);
    cmd.has_color_temp = false;
    cmd.has_brightness = true;
    cmd.brightness = brightness_pct;
    this->hub_->send_command(cmd);
  }
}

void MiLightLight::write_state_brightness_(float brightness) {
  if (brightness <= 0.001f)
    return;
  uint8_t brightness_pct = static_cast<uint8_t>(brightness * 100.0f);
  if (this->is_step_based_brightness_()) {
    this->send_step_brightness_(brightness_pct);
  } else {
    LightCommand cmd = this->make_base_cmd_();
    cmd.has_brightness = true;
    cmd.brightness = brightness_pct;
    this->hub_->send_command(cmd);
  }
}

// ---- write_state ----

void MiLightLight::write_state(light::LightState *state) {
  if (this->suppress_next_write_) {
    this->suppress_next_write_ = false;
    return;
  }

  if (!state->current_values.is_on()) {
    LightCommand cmd = this->make_base_cmd_();
    cmd.turn_off = true;
    this->hub_->send_command(cmd);
    this->effect_active_ = false;
    return;
  }

  if (this->effect_active_)
    return;

  float brightness;
  state->current_values_as_brightness(&brightness);

  // Send ON command first
  LightCommand cmd = this->make_base_cmd_();
  cmd.turn_on = true;
  this->hub_->send_command(cmd);

  auto color_mode = state->current_values.get_color_mode();
  if (color_mode == light::ColorMode::RGB_WHITE) {
    this->write_state_rgb_white_(state, brightness);
  } else if (color_mode == light::ColorMode::RGB) {
    this->write_state_rgb_(state, brightness);
  } else if (color_mode == light::ColorMode::COLOR_TEMPERATURE) {
    this->write_state_color_temp_(state, brightness);
  } else {
    this->write_state_brightness_(brightness);
  }
}

// ---- Apply received command from physical remote ----

void MiLightLight::apply_received_command(const ReceivedCommand &rx) {
  if (this->light_state_ == nullptr)
    return;

  auto call = this->light_state_->make_call();

  if (rx.is_off) {
    call.set_state(false);
    this->suppress_next_write_ = true;
    call.perform();
    return;
  }

  if (rx.is_on) {
    call.set_state(true);
  }

  if (rx.night_mode) {
    // Activate night mode effect if available
    call.set_state(true);
    call.set_effect(std::string(EFFECT_NIGHT_MODE));
    this->suppress_next_write_ = true;
    call.perform();
    return;
  }

  if (rx.has_effect) {
    call.set_state(true);
    if (this->has_direct_mode_select_() && rx.effect < 9) {
      call.set_effect(std::string(EFFECT_DISCO_NAMES[rx.effect]));
    } else {
      call.set_effect(std::string(EFFECT_DISCO_CYCLE));
    }
    this->suppress_next_write_ = true;
    call.perform();
    return;
  }

  if (rx.white_mode) {
    call.set_state(true);
    if (this->remote_type_ == RemoteType::RGBW) {
      // RGBW: switch to white output mode
      call.set_color_mode(light::ColorMode::RGB_WHITE);
      call.set_white(1.0f);
    } else {
      // FUT089/RGB_CCT: switch to color temperature mode
      call.set_color_mode(light::ColorMode::COLOR_TEMPERATURE);
    }
    this->suppress_next_write_ = true;
    call.perform();
    return;
  }

  // Step-based brightness adjustments
  if (rx.step_up_brightness || rx.step_down_brightness) {
    float current_brightness;
    this->light_state_->current_values_as_brightness(&current_brightness);
    float step = 1.0f / V1_STEP_COUNT;
    if (rx.step_up_brightness) {
      current_brightness = std::min(1.0f, current_brightness + step);
    } else {
      current_brightness = std::max(0.01f, current_brightness - step);
    }
    call.set_state(true);
    call.set_brightness(current_brightness);
    this->suppress_next_write_ = true;
    call.perform();
    return;
  }

  // Step-based color temperature adjustments
  if (rx.step_up_temp || rx.step_down_temp) {
    float current_temp = this->light_state_->current_values.get_color_temperature();
    float step = (this->warm_white_mireds_ - this->cold_white_mireds_) / V1_STEP_COUNT;
    if (rx.step_up_temp) {
      current_temp = std::min(this->warm_white_mireds_, current_temp + step);
    } else {
      current_temp = std::max(this->cold_white_mireds_, current_temp - step);
    }
    call.set_state(true);
    call.set_color_temperature(current_temp);
    this->suppress_next_write_ = true;
    call.perform();
    return;
  }

  if (rx.has_hue) {
    call.set_state(true);
    float hue = rx.hue;
    float c = 1.0f;  // full saturation, full value
    float x = c * (1.0f - std::fabs(std::fmod(hue / 60.0f, 2.0f) - 1.0f));
    float r = 0, g = 0, b = 0;
    int segment = static_cast<int>(hue / 60.0f) % 6;
    switch (segment) {
      case 0:
        r = c;
        g = x;
        break;
      case 1:
        r = x;
        g = c;
        break;
      case 2:
        g = c;
        b = x;
        break;
      case 3:
        g = x;
        b = c;
        break;
      case 4:
        r = x;
        b = c;
        break;
      default:
        r = c;
        b = x;
        break;
    }
    call.set_rgb(r, g, b);
  }

  if (rx.has_brightness) {
    call.set_state(true);
    call.set_brightness(rx.brightness / 100.0f);
  }

  if (rx.has_color_temp) {
    call.set_state(true);
    float mireds =
        this->cold_white_mireds_ + (rx.color_temp / 100.0f) * (this->warm_white_mireds_ - this->cold_white_mireds_);
    call.set_color_temperature(mireds);
  }

  this->suppress_next_write_ = true;
  call.perform();
}

}  // namespace esphome::milight
