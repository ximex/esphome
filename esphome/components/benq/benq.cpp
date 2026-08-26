#include "benq.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include <cinttypes>

namespace esphome::benq {

static const char *const TAG = "benq";

// BenQ command strings indexed by BenqCommand enum
static const char *const BENQ_COMMAND_STRINGS[] = {
    "pow",     // POWER
    "sour",    // SOURCE
    "mute",    // MUTE
    "vol",     // VOLUME
    "appmod",  // PICTURE_MODE
    "con",     // CONTRAST
    "bri",     // BRIGHTNESS
    "color",   // COLOR
    "sharp",   // SHARPNESS
    "asp",     // ASPECT_RATIO
    "blank",   // BLANK
    "freeze",  // FREEZE
    "menu",    // MENU
    "lampm",   // LAMP_MODE
    "ltim",    // LAMP_TIME
    "lrst",    // LAMP_HOUR_RESET
    "up",      // UP
    "down",    // DOWN
    "left",    // LEFT
    "right",   // RIGHT
    "enter",   // ENTER
    "baud",    // BAUD_RATE
    "auto",    // AUTO
};

static_assert(sizeof(BENQ_COMMAND_STRINGS) / sizeof(BENQ_COMMAND_STRINGS[0]) == BENQ_COMMAND_COUNT,
              "BENQ_COMMAND_STRINGS needs one entry per BenqCommand value");

const char *BenQ::get_command_name(BenqCommand cmd) {
  uint8_t idx = static_cast<uint8_t>(cmd);
  if (idx >= BENQ_COMMAND_COUNT) {
    return "";
  }
  return BENQ_COMMAND_STRINGS[idx];
}

BenqCommand BenQ::get_command_by_name(const char *cmd_name) {
  for (uint8_t i = 0; i < BENQ_COMMAND_COUNT; ++i) {
    if (strcasecmp(BENQ_COMMAND_STRINGS[i], cmd_name) == 0) {
      return static_cast<BenqCommand>(i);
    }
  }
  ESP_LOGW(TAG, "Unknown command string: %s", cmd_name);
  return BenqCommand::MAX_COMMAND;
}

bool BenQ::may_repeat(BenqCommand cmd, const char *value) {
  if (strcmp(value, "?") == 0) {
    // The projector answers "pow" whether it is off, warming up or running, so
    // a missing answer there can only be a lost frame — and until it arrives
    // the hub does not know whether to ask for anything else at all. Every
    // other query may go unanswered simply because the projector is not ready
    // yet, and the next poll asks again anyway.
    return cmd == BenqCommand::POWER;
  }
  // Relative and momentary commands would act twice if the first one did get
  // through and only its reply was lost.
  if (value[0] == '\0' || strcmp(value, "+") == 0 || strcmp(value, "-") == 0) {
    return false;
  }
  switch (cmd) {
    case BenqCommand::UP:
    case BenqCommand::DOWN:
    case BenqCommand::LEFT:
    case BenqCommand::RIGHT:
    case BenqCommand::ENTER:
    case BenqCommand::AUTO:
    case BenqCommand::LAMP_HOUR_RESET:
      return false;
    default:
      // Everything else names an absolute state, so arriving twice changes
      // nothing beyond arriving once
      return true;
  }
}

// Ring buffer operations
bool BenQ::CommandQueue::push(BenqCommand cmd, const char *value) {
  if (this->count >= this->capacity) {
    return false;
  }
  auto &entry = this->entries[this->tail];
  entry.command = cmd;
  strncpy(entry.value, value, sizeof(entry.value) - 1);
  entry.value[sizeof(entry.value) - 1] = '\0';
  this->tail = (this->tail + 1) % this->capacity;
  this->count++;
  return true;
}

bool BenQ::CommandQueue::pop(PendingCommand &out) {
  if (this->count == 0) {
    return false;
  }
  out = this->entries[this->head];
  this->head = (this->head + 1) % this->capacity;
  this->count--;
  return true;
}

bool BenQ::CommandQueue::contains(BenqCommand cmd, const char *value) const {
  for (uint8_t i = 0; i < this->count; i++) {
    const auto &entry = this->entries[(this->head + i) % this->capacity];
    if (entry.command == cmd && strcmp(entry.value, value) == 0) {
      return true;
    }
  }
  return false;
}

BenqEntity::BenqEntity(BenQ *parent, BenqCommand command) : command_(command) { parent->register_entity(this); }

void BenQ::register_entity(BenqEntity *entity) {
  entity->next_ = this->entities_;
  this->entities_ = entity;
}

void BenQ::setup() {
  ESP_LOGCONFIG(TAG, "Setting up BenQ projector...");
  this->clear_uart_buffer_();
  this->query_command(BenqCommand::POWER);
}

void BenQ::loop() {
  if (this->should_query_entities_) {
    this->should_query_entities_ = false;
    this->query_all_entities_();
  }

  // Probe once at a time; each attempt is paced by the command timeout
  if (this->waiting_for_ready_ && !this->waiting_for_response_ && this->queued_command_count_() == 0) {
    if (millis() - this->power_on_time_ >= READY_TIMEOUT_MS) {
      ESP_LOGW(TAG, "Projector did not answer within %" PRIu32 " s of switching on; polling normally from now on",
               READY_TIMEOUT_MS / 1000);
      this->waiting_for_ready_ = false;
      this->should_query_entities_ = true;
    } else {
      this->query_command(READY_PROBE);
    }
  }

  this->process_command_queue_();

  // Every reply is framed as *...#. Bytes outside a frame are noise: the prompt
  // the projector prints, padding, or the tail of a reply that was cut short.
  // Starting over on each '*' keeps the parser in step even after lost bytes.
  while (this->available()) {
    uint8_t c;
    this->read_byte(&c);

    if (c == '*') {
      if (this->rx_len_ > 0) {
        this->rx_buffer_[this->rx_len_] = '\0';
        ESP_LOGV(TAG, "Dropping unterminated reply: %s", this->rx_buffer_);
      }
      this->rx_buffer_[0] = '*';
      this->rx_len_ = 1;
      continue;
    }

    if (this->rx_len_ == 0 || c == '\r' || c == '\n' || c == 0x00) {
      continue;
    }

    if (c == '#') {
      this->rx_buffer_[this->rx_len_++] = '#';
      this->rx_buffer_[this->rx_len_] = '\0';
      ESP_LOGD(TAG, "Complete message received: %s", this->rx_buffer_);
      this->handle_line_();
      this->rx_len_ = 0;
      continue;
    }

    if (this->rx_len_ >= sizeof(this->rx_buffer_) - 2) {
      // No reply is this long. Drop it instead of silently losing the bytes
      // that follow, which would swallow the delimiters of the next replies.
      this->rx_buffer_[this->rx_len_] = '\0';
      ESP_LOGW(TAG, "Reply too long, dropping: %s", this->rx_buffer_);
      this->rx_len_ = 0;
      continue;
    }

    this->rx_buffer_[this->rx_len_++] = static_cast<char>(c);
  }
}

void BenQ::update() {
  // Skip scheduling new queries if the previous cycle is still processing
  if (this->waiting_for_response_ || this->queued_command_count_() > 0) {
    ESP_LOGD(TAG, "Previous query cycle still in progress, skipping this update");
    return;
  }
  if (this->last_power_state_) {
    // Power is ON — query all entities on each update interval
    this->should_query_entities_ = true;
  }
  // Always query power state
  this->query_command(BenqCommand::POWER);
}

void BenQ::dump_config() {
  ESP_LOGCONFIG(TAG, "BenQ Projector:");
  ESP_LOGCONFIG(TAG, "  Model: %s", this->model_);
  LOG_UPDATE_INTERVAL(this);
  ESP_LOGCONFIG(TAG, "  Command Timeout: %" PRIu32 " ms", this->command_timeout_ms_);
  unsigned entity_count = 0;
  for (BenqEntity *entity = this->entities_; entity != nullptr; entity = entity->next_) {
    entity_count++;
  }
  ESP_LOGCONFIG(TAG, "  Command entities: %u", entity_count);
  ESP_LOGCONFIG(TAG, "  Media Player: %s", YESNO(this->media_player_ != nullptr));
}

void BenQ::send_command(BenqCommand cmd, const char *value) {
  if (static_cast<uint8_t>(cmd) >= BENQ_COMMAND_COUNT) {
    ESP_LOGW(TAG, "Unknown command enum: %u", static_cast<unsigned>(cmd));
    return;
  }
  const char *val = value != nullptr ? value : "?";
  bool is_query = strcmp(val, "?") == 0;

  // Everything goes through a queue so that ordering and the minimum gap
  // between commands hold for hub-initiated polls and user actions alike.
  CommandQueue &queue = is_query ? this->query_queue_ : this->set_queue_;
  if (is_query && queue.contains(cmd, val)) {
    return;  // Repeating a pending query would not add anything
  }
  if (!queue.push(cmd, val)) {
    ESP_LOGW(TAG, "Command queue full, dropping: *%s=%s#", get_command_name(cmd), val);
    return;
  }
  ESP_LOGV(TAG, "Queued: *%s=%s#", get_command_name(cmd), val);
}

void BenQ::check_power_state_change_(bool new_power_state) {
  if (new_power_state && !this->last_power_state_) {
    ESP_LOGI(TAG, "Power turned on, waiting for the projector to answer");
    this->power_on_time_ = millis();
    this->waiting_for_ready_ = true;
  }
  if (!new_power_state && this->last_power_state_) {
    ESP_LOGI(TAG, "Power turned off, clearing command queue");
    this->set_queue_.clear();
    this->query_queue_.clear();
    this->should_query_entities_ = false;
    this->waiting_for_ready_ = false;
  }
  this->last_power_state_ = new_power_state;
}

void BenQ::set_volume(int8_t target) {
  this->volume_target_ = std::clamp(target, VOLUME_MIN, VOLUME_MAX);
  this->volume_rounds_left_ = VOLUME_MAX_ROUNDS;
  if (this->volume_level_ < 0) {
    // Stepping needs a starting point; the reply picks the work up from here
    ESP_LOGD(TAG, "Volume level not known yet, asking before stepping to %d", this->volume_target_);
    this->query_command(BenqCommand::VOLUME);
    return;
  }
  this->step_volume_();
}

void BenQ::nudge_volume(int8_t delta) {
  if (this->volume_level_ < 0) {
    this->send_command(BenqCommand::VOLUME, delta > 0 ? "+" : "-");
    this->query_command(BenqCommand::VOLUME);
    return;
  }
  this->set_volume(this->volume_level_ + delta);
}

void BenQ::step_volume_() {
  int8_t delta = this->volume_target_ - this->volume_level_;
  if (delta == 0) {
    this->volume_target_ = -1;
    return;
  }
  const char *step = delta > 0 ? "+" : "-";
  uint8_t count = delta > 0 ? delta : -delta;
  ESP_LOGD(TAG, "Volume %d -> %d: sending %u x *vol=%s#", this->volume_level_, this->volume_target_, count, step);
  for (uint8_t i = 0; i < count; i++) {
    this->send_command(BenqCommand::VOLUME, step);
  }
  // Read the level back: a step lost on the way would go unnoticed otherwise
  this->query_command(BenqCommand::VOLUME);
}

void BenQ::query_all_entities_() {
  ESP_LOGD(TAG, "Querying state of all entities...");
  for (BenqEntity *entity = this->entities_; entity != nullptr; entity = entity->next_) {
    this->query_command(entity->get_command());
  }
  if (this->media_player_ != nullptr) {
    this->media_player_->query_state();
  }
}

void BenQ::dispatch_response_(BenqCommand cmd, const BenqResponse &response) {
  // Track power state in hub. An error reply carries no state, so it must not
  // be read as "projector off".
  if (cmd == BenqCommand::POWER && !response.is_error) {
    this->check_power_state_change_(strcasecmp(response.value, "on") == 0);
  }

  if (cmd == BenqCommand::VOLUME && response.success) {
    if (auto level = parse_number<int>(response.value); level.has_value()) {
      this->volume_level_ = static_cast<int8_t>(level.value());
      if (this->volume_target_ >= 0) {
        if (this->volume_level_ == this->volume_target_) {
          this->volume_target_ = -1;
        } else if (this->volume_rounds_left_ > 0) {
          this->volume_rounds_left_--;
          this->step_volume_();
        } else {
          ESP_LOGW(TAG, "Volume stopped at %d instead of %d", this->volume_level_, this->volume_target_);
          this->volume_target_ = -1;
        }
      }
    }
  }

  if (this->waiting_for_ready_ && cmd == READY_PROBE && response.success) {
    ESP_LOGI(TAG, "Projector became reachable %" PRIu32 " ms after switching on", millis() - this->power_on_time_);
    this->waiting_for_ready_ = false;
    this->should_query_entities_ = true;
  }

  for (BenqEntity *entity = this->entities_; entity != nullptr; entity = entity->next_) {
    if (entity->get_command() == cmd) {
      entity->handle_response(response);
    }
  }
  if (this->media_player_ != nullptr) {
    this->media_player_->handle_response(cmd, response);
  }
}

void BenQ::handle_line_() {
  BenqResponse response = this->parse_response_(this->rx_buffer_, this->rx_len_);

  if (!response.is_error && !response.success) {
    // A complete frame that does not parse means bytes were lost on the way
    ESP_LOGW(TAG, "Could not parse reply: %s", this->rx_buffer_);
    return;
  }
  // A standalone error line such as "*Illegal format#" names no command, so
  // there is nothing to route it to.
  if (response.command[0] == '\0') {
    ESP_LOGW(TAG, "RX Error: %s", response.error_message);
    this->waiting_for_response_ = false;
    return;
  }

  BenqCommand cmd = get_command_by_name(response.command);
  if (cmd == BenqCommand::MAX_COMMAND) {
    return;
  }

  // The command names this component sends are lower case, and the projector
  // answers in upper case. A frame that spells the command exactly the way it
  // is sent is therefore the projector repeating what it received, not a
  // statement about its state. This must not depend on which command is
  // outstanding: replies routinely lag a command behind.
  bool is_echo = strcmp(response.command, get_command_name(cmd)) == 0;

  // Only the reply this component is actually waiting for ends the wait. A late
  // reply to an earlier command must not release the next one too early, or
  // every following frame is attributed to the wrong command.
  bool ends_wait = this->waiting_for_response_ && cmd == this->pending_command_;

  if (response.is_error) {
    ESP_LOGW(TAG, "RX Error on %s: %s", response.command, response.error_message);
    if (ends_wait) {
      this->waiting_for_response_ = false;
    }
    this->dispatch_response_(cmd, response);
    return;
  }

  if (is_echo) {
    ESP_LOGV(TAG, "RX echo: *%s=%s#", response.command, response.value);
    // A set command has been delivered; a query still owes us its answer
    if (ends_wait && strcmp(response.value, "?") != 0) {
      this->waiting_for_response_ = false;
    }
    return;
  }

  ESP_LOGD(TAG, "RX: %s=%s", response.command, response.value);
  if (ends_wait) {
    this->waiting_for_response_ = false;
  }
  this->dispatch_response_(cmd, response);
}

BenqResponse BenQ::parse_response_(const char *line, size_t len) {
  BenqResponse response;

  // Try to parse standard response format: *COMMAND=VALUE#
  if (len > 0 && line[0] == '*') {
    const char *eq = static_cast<const char *>(memchr(line, '=', len));
    const char *hash = static_cast<const char *>(memchr(line, '#', len));

    if (eq != nullptr && hash != nullptr && eq < hash) {
      size_t cmd_len = eq - line - 1;
      size_t val_len = hash - eq - 1;

      // Anything longer than the buffers is not a response this component knows
      if (cmd_len >= sizeof(response.command) || val_len >= sizeof(response.value)) {
        return response;
      }
      memcpy(response.command, line + 1, cmd_len);
      response.command[cmd_len] = '\0';
      memcpy(response.value, eq + 1, val_len);
      response.value[val_len] = '\0';

      // Check if value indicates an error
      if (strstr(response.value, "Illegal") != nullptr || strstr(response.value, "Unsupported") != nullptr ||
          strstr(response.value, "Block") != nullptr) {
        response.is_error = true;
        strncpy(response.error_message, response.value, sizeof(response.error_message) - 1);
      } else {
        response.success = true;
      }
      return response;
    }
  }

  // Check for standalone error messages
  if (strstr(line, "Illegal") != nullptr || strstr(line, "Unsupported") != nullptr ||
      strstr(line, "Block") != nullptr) {
    response.is_error = true;
    strncpy(response.error_message, line, sizeof(response.error_message) - 1);
    return response;
  }

  return response;
}

void BenQ::clear_uart_buffer_() {
  while (this->available()) {
    this->read();
  }
}

void BenQ::process_command_queue_() {
  if (this->waiting_for_response_) {
    if (millis() - this->last_command_time_ >= this->command_timeout_ms_) {
      if (!this->already_repeated_ && may_repeat(this->pending_command_, this->pending_value_)) {
        ESP_LOGW(TAG, "No reply to *%s=%s#, sending it once more", get_command_name(this->pending_command_),
                 this->pending_value_);
        this->already_repeated_ = true;
        this->send_raw_command_(this->pending_command_, this->pending_value_);
        return;
      }
      ESP_LOGD(TAG, "No response received for '%s' in %" PRIu32 " ms, moving on",
               get_command_name(this->pending_command_), this->command_timeout_ms_);
      this->waiting_for_response_ = false;
    } else {
      return;
    }
  }

  // Enforce minimum delay between commands to let the projector finish responding
  if (millis() - this->last_command_time_ < MIN_COMMAND_INTERVAL_MS) {
    return;
  }

  PendingCommand pending;
  if (!this->set_queue_.pop(pending) && !this->query_queue_.pop(pending)) {
    return;
  }

  ESP_LOGD(TAG, "Processing queued command: *%s=%s# (queued: %u)", get_command_name(pending.command), pending.value,
           static_cast<unsigned>(this->queued_command_count_()));
  this->already_repeated_ = false;
  this->send_raw_command_(pending.command, pending.value);
}

void BenQ::send_raw_command_(BenqCommand cmd, const char *value) {
  const char *cmd_name = get_command_name(cmd);
  this->write_str("\r*");
  this->write_str(cmd_name);
  this->write_str("=");
  this->write_str(value);
  this->write_str("#\r");
  this->flush();

  ESP_LOGD(TAG, "TX: *%s=%s#", cmd_name, value);
  this->waiting_for_response_ = true;
  this->pending_command_ = cmd;
  strncpy(this->pending_value_, value, sizeof(this->pending_value_) - 1);
  this->pending_value_[sizeof(this->pending_value_) - 1] = '\0';
  this->last_command_time_ = millis();
}

}  // namespace esphome::benq
