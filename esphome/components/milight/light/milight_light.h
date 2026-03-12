#pragma once

#include "esphome/core/component.h"
#include "esphome/components/light/light_effect.h"
#include "esphome/components/light/light_output.h"
#include "esphome/components/light/light_state.h"
#include "esphome/components/light/light_traits.h"
#include "../milight_hub.h"
#include "../packet_formatter.h"

namespace esphome::milight {

/// MiLight color temperature range (fixed by protocol)
static constexpr float MI_MIREDS_MIN = 153.0f;
static constexpr float MI_MIREDS_MAX = 370.0f;

/// Number of brightness/temperature steps for V1 step-based protocols
static constexpr uint8_t V1_STEP_COUNT = 10;

class MiLightLight;

/// Light effect for MiLight disco modes and night mode
class MiLightEffect : public light::LightEffect {
 public:
  /// mode 0-8 = disco modes, 0xFE = cycle disco, 0xFF = night mode
  MiLightEffect(const char *name, MiLightLight *output, uint8_t mode)
      : LightEffect(name), output_(output), mode_(mode) {}

  void start() override;
  void stop() override;
  void apply() override {}

 protected:
  MiLightLight *output_;
  uint8_t mode_;
};

class MiLightLight : public Component, public light::LightOutput {
 public:
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void setup() override;
  void set_hub(MiLightHub *hub) { this->hub_ = hub; }
  void set_device_id(uint16_t id) { this->device_id_ = id; }
  void set_group_id(uint8_t id) { this->group_id_ = id; }
  void set_remote_type(RemoteType type) { this->remote_type_ = type; }
  void set_cold_white_temperature(float mireds) { this->cold_white_mireds_ = mireds; }
  void set_warm_white_temperature(float mireds) { this->warm_white_mireds_ = mireds; }

  light::LightTraits get_traits() override;
  void setup_state(light::LightState *state) override;
  void write_state(light::LightState *state) override;

  /// Send a disco effect command (mode 0-8 for V2, cycle for V1)
  void send_effect(uint8_t mode);

  /// Send night mode command
  void send_night_mode();

  /// Send pair command
  void send_pair();

  /// Send unpair command
  void send_unpair();

  void set_effect_active(bool active) { this->effect_active_ = active; }

  /// Apply a received command from a physical remote
  void apply_received_command(const ReceivedCommand &cmd);

 protected:
  /// Create a base LightCommand with device/group/type pre-filled
  LightCommand make_base_cmd_() const;

  /// Map ESPHome color temperature (mireds) to MiLight's 0-100 scale
  uint8_t mireds_to_milight_(float mireds) const;

  /// Send step-based brightness for CCT/RGB/FUT020
  void send_step_brightness_(uint8_t target_pct);

  /// Send step-based temperature for CCT
  void send_step_temperature_(uint8_t target_pct);

  /// Whether this remote type uses step-based brightness
  bool is_step_based_brightness_() const;

  /// Whether this remote type supports disco modes
  bool supports_disco_() const;

  /// Whether this remote type supports night mode
  bool supports_night_mode_() const;

  /// Whether this remote type supports direct disco mode select (V2)
  bool has_direct_mode_select_() const;

  /// Convert RGB values to hue (0-359) and saturation (0-100).
  /// Returns false if color is achromatic (no meaningful hue/saturation).
  static bool rgb_to_hs_(float r, float g, float b, float &hue, float &saturation);

  /// Write state for RGB_WHITE color mode (RGBW remotes)
  void write_state_rgb_white_(light::LightState *state, float brightness);

  /// Write state for RGB color mode
  void write_state_rgb_(light::LightState *state, float brightness);

  /// Write state for COLOR_TEMPERATURE color mode
  void write_state_color_temp_(light::LightState *state, float brightness);

  /// Write state for brightness-only / ON_OFF
  void write_state_brightness_(float brightness);

  MiLightHub *hub_{nullptr};
  light::LightState *light_state_{nullptr};
  uint16_t device_id_{0};
  uint8_t group_id_{0};
  RemoteType remote_type_{RemoteType::RGBW};
  float cold_white_mireds_{MI_MIREDS_MIN};
  float warm_white_mireds_{MI_MIREDS_MAX};
  bool effect_active_{false};
  bool suppress_next_write_{false};
};

}  // namespace esphome::milight
