#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include <algorithm>
#include <cstdint>
#include <cstring>

namespace esphome::benq {

// Forward declarations
class BenqSensor;
class BenqSwitch;
class BenqNumber;
class BenqSelect;
class BenqMediaPlayer;

// Command enum for all supported BenQ projector commands
enum class BenqCommand : uint8_t {
  // Power & Basic Controls
  POWER,
  SOURCE,
  MUTE,
  VOLUME,

  // Picture Settings
  PICTURE_MODE,
  CONTRAST,
  BRIGHTNESS,
  COLOR,
  SHARPNESS,
  ASPECT_RATIO,

  // Display Controls
  BLANK,
  FREEZE,
  MENU,

  // Lamp Controls
  LAMP_MODE,
  LAMP_TIME,
  LAMP_HOUR_RESET,

  // Menu Navigation
  UP,
  DOWN,
  LEFT,
  RIGHT,
  ENTER,

  // Operation Settings
  BAUD_RATE,
  AUTO,

  MAX_COMMAND,
};

static constexpr uint8_t BENQ_COMMAND_COUNT = static_cast<uint8_t>(BenqCommand::MAX_COMMAND);

// Response structure from projector — fixed buffers, no heap allocation
struct BenqResponse {
  bool success{false};
  bool is_error{false};
  char command[16]{};
  char value[32]{};
  char error_message[48]{};
};

// Convert string to lowercase in a fixed buffer, returns length written
inline size_t str_to_lower_buf(char *dest, size_t dest_size, const char *src, size_t src_len) {
  size_t len = std::min(src_len, dest_size - 1);
  for (size_t i = 0; i < len; i++) {
    char c = src[i];
    if (c >= 'A' && c <= 'Z') {
      c = c + ('a' - 'A');
    }
    dest[i] = c;
  }
  dest[len] = '\0';
  return len;
}

// Main BenQ component (hub)
class BenQ : public uart::UARTDevice, public PollingComponent {
 public:
  void setup() override;
  void loop() override;
  void update() override;
  void dump_config() override;

  float get_setup_priority() const override { return setup_priority::DATA; }

  // Command execution
  void send_command(BenqCommand cmd, const char *value);
  void query_command(BenqCommand cmd);

  // Model configuration
  void set_model(const char *model) { strncpy(this->model_, model, sizeof(this->model_) - 1); }

  // Entity registration — called from platform to_code()
  void register_sensor(BenqSensor *sensor);
  void register_switch(BenqSwitch *sw);
  void register_number(BenqNumber *num);
  void register_select(BenqSelect *sel);
  void set_media_player(BenqMediaPlayer *mp) { this->media_player_ = mp; }

  // Get command string name (public for entity use)
  static const char *get_command_name(BenqCommand cmd);

 protected:
  // Power state tracking
  void check_power_state_change_(bool new_power_state);

  // Protocol handling
  void handle_line_();
  void dispatch_response_(const BenqResponse &response);
  BenqResponse parse_response_(const char *line, size_t len);
  void write_command_(const char *cmd, const char *value);
  void process_command_queue_();
  void clear_uart_buffer_();
  void send_raw_command_(const char *cmd, const char *value);

  // Query all queryable entities
  void query_all_entities_();

  // Get command enum by name
  static BenqCommand get_command_by_name_(const char *cmd_name);

  // Command queue — fixed ring buffer instead of std::queue<std::deque>
  static constexpr uint32_t COMMAND_TIMEOUT_MS = 3000;
  static constexpr uint32_t MIN_COMMAND_INTERVAL_MS = 100;
  static constexpr size_t MAX_QUEUE_SIZE = 32;

  struct PendingCommand {
    char command_name[16]{};
    char value[16]{};
  };

  PendingCommand command_queue_[MAX_QUEUE_SIZE]{};
  uint8_t queue_head_{0};
  uint8_t queue_tail_{0};
  uint8_t queue_count_{0};

  bool enqueue_command_(const char *cmd, const char *value);
  bool enqueue_command_front_(const char *cmd, const char *value);
  bool dequeue_command_(PendingCommand &out);

  // RX buffer — fixed size
  char rx_buffer_[128]{};
  uint8_t rx_len_{0};

  // State
  char model_[32]{};
  bool waiting_for_response_{false};
  char pending_command_[16]{};
  uint32_t last_command_time_{0};
  bool should_query_entities_{false};
  bool last_power_state_{false};

  // Registered entities — simple arrays with counts
  static constexpr size_t MAX_SENSORS = 4;
  static constexpr size_t MAX_SWITCHES = 8;
  static constexpr size_t MAX_NUMBERS = 8;
  static constexpr size_t MAX_SELECTS = 8;

  BenqSensor *sensors_[MAX_SENSORS]{};
  uint8_t sensor_count_{0};
  BenqSwitch *switches_[MAX_SWITCHES]{};
  uint8_t switch_count_{0};
  BenqNumber *numbers_[MAX_NUMBERS]{};
  uint8_t number_count_{0};
  BenqSelect *selects_[MAX_SELECTS]{};
  uint8_t select_count_{0};

  BenqMediaPlayer *media_player_{nullptr};
};

}  // namespace esphome::benq
