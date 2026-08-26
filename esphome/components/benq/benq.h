#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include <algorithm>
#include <cstdint>
#include <cstring>

namespace esphome::benq {

// Forward declarations
class BenQ;

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

// Implemented by BenqMediaPlayer. Declared here so the hub does not have to
// include the media_player platform, which is not part of every build.
class BenqMediaPlayerBase {
 public:
  virtual void query_state() = 0;
  virtual void handle_response(BenqCommand cmd, const BenqResponse &response) = 0;
};

// Base for entities that map to a single projector command. Instances add
// themselves to the hub's list on construction, so there is no upper bound on
// how many entities a configuration may declare.
class BenqEntity {
 public:
  BenqEntity(BenQ *parent, BenqCommand command);

  BenqCommand get_command() const { return this->command_; }
  virtual void handle_response(const BenqResponse &response) = 0;

 protected:
  friend class BenQ;

  BenqCommand command_;
  BenqEntity *next_{nullptr};
};

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
  void query_command(BenqCommand cmd) { this->send_command(cmd, "?"); }

  // Volume is set by stepping. Many projectors acknowledge an absolute level
  // such as "*vol=6#" and then keep the level they had, while "+" and "-" are
  // in every BenQ command table. The level reached is read back afterwards, so
  // a step that gets lost on the way is noticed and made up for.
  static constexpr int8_t VOLUME_MIN = 0;
  static constexpr int8_t VOLUME_MAX = 10;
  void set_volume(int8_t target);
  void nudge_volume(int8_t delta);

  void set_command_timeout(uint32_t timeout_ms) { this->command_timeout_ms_ = timeout_ms; }

  // Model configuration. The pointer comes from codegen and refers to a string
  // literal, so it stays valid for the lifetime of the application.
  void set_model(const char *model) { this->model_ = model; }

  // Entity registration
  void register_entity(BenqEntity *entity);
  void set_media_player(BenqMediaPlayerBase *mp) { this->media_player_ = mp; }

  // Get command string name (public for entity use)
  static const char *get_command_name(BenqCommand cmd);

 protected:
  // Power state tracking
  void check_power_state_change_(bool new_power_state);

  // Protocol handling
  void handle_line_();
  void dispatch_response_(BenqCommand cmd, const BenqResponse &response);
  BenqResponse parse_response_(const char *line, size_t len);
  void process_command_queue_();
  void clear_uart_buffer_();
  void send_raw_command_(BenqCommand cmd, const char *value);

  // Query all queryable entities
  void query_all_entities_();

  void step_volume_();
  static constexpr uint8_t VOLUME_MAX_ROUNDS = 3;

  // Get command enum by name
  static BenqCommand get_command_by_name(const char *cmd_name);

  // Whether a command may be sent a second time when no reply arrives
  static bool may_repeat(BenqCommand cmd, const char *value);

  static constexpr uint32_t MIN_COMMAND_INTERVAL_MS = 100;

  // After switching on, the projector answers "pow" long before it answers
  // anything else. Asking for everything right away only runs into timeouts, so
  // a single harmless query is repeated until the projector really is awake.
  static constexpr BenqCommand READY_PROBE = BenqCommand::LAMP_TIME;
  static constexpr uint32_t READY_TIMEOUT_MS = 180000;

  // Two fixed ring buffers instead of std::queue<std::deque>. Set commands take
  // precedence over pending queries but keep their order among themselves, so a
  // quick series of changes ends on the value the user picked last.
  // Room for a full sweep of the volume range plus a few other actions
  static constexpr size_t MAX_SET_QUEUE_SIZE = 14;
  static constexpr size_t MAX_QUERY_QUEUE_SIZE = 16;

  struct PendingCommand {
    BenqCommand command{BenqCommand::MAX_COMMAND};
    char value[16]{};
  };

  struct CommandQueue {
    PendingCommand *entries;
    uint8_t capacity;
    uint8_t head{0};
    uint8_t tail{0};
    uint8_t count{0};

    bool push(BenqCommand cmd, const char *value);
    bool pop(PendingCommand &out);
    bool contains(BenqCommand cmd, const char *value) const;
    void clear() { this->head = this->tail = this->count = 0; }
  };

  PendingCommand set_queue_entries_[MAX_SET_QUEUE_SIZE]{};
  PendingCommand query_queue_entries_[MAX_QUERY_QUEUE_SIZE]{};
  CommandQueue set_queue_{this->set_queue_entries_, MAX_SET_QUEUE_SIZE};
  CommandQueue query_queue_{this->query_queue_entries_, MAX_QUERY_QUEUE_SIZE};

  size_t queued_command_count_() const { return this->set_queue_.count + this->query_queue_.count; }

  // RX buffer. The longest reply the parser accepts is a command of 15 plus a
  // value of 31 characters, so 64 leaves room to spare.
  char rx_buffer_[64]{};
  uint8_t rx_len_{0};

  // State
  const char *model_{""};
  uint32_t command_timeout_ms_{5000};
  int8_t volume_level_{-1};   // last level the projector reported, -1 = unknown
  int8_t volume_target_{-1};  // level being aimed for, -1 = nothing pending
  uint8_t volume_rounds_left_{0};
  bool waiting_for_response_{false};
  BenqCommand pending_command_{BenqCommand::MAX_COMMAND};
  char pending_value_[16]{};
  bool already_repeated_{false};
  uint32_t last_command_time_{0};
  bool should_query_entities_{false};
  bool last_power_state_{false};
  bool waiting_for_ready_{false};
  uint32_t power_on_time_{0};

  // Registered entities — intrusive singly linked list, no fixed capacity
  BenqEntity *entities_{nullptr};
  BenqMediaPlayerBase *media_player_{nullptr};
};

}  // namespace esphome::benq
