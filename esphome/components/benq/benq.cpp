#include "benq.h"
#include "sensor/benq_sensor.h"
#include "switch/benq_switch.h"
#include "number/benq_number.h"
#include "select/benq_select.h"
#include "media_player/benq_media_player.h"
#include "esphome/core/log.h"

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

const char *BenQ::get_command_name(BenqCommand cmd) {
  uint8_t idx = static_cast<uint8_t>(cmd);
  if (idx >= BENQ_COMMAND_COUNT) {
    return "";
  }
  return BENQ_COMMAND_STRINGS[idx];
}

BenqCommand BenQ::get_command_by_name_(const char *cmd_name) {
  for (uint8_t i = 0; i < BENQ_COMMAND_COUNT; ++i) {
    if (strcasecmp(BENQ_COMMAND_STRINGS[i], cmd_name) == 0) {
      return static_cast<BenqCommand>(i);
    }
  }
  ESP_LOGW(TAG, "Unknown command string: %s", cmd_name);
  return BenqCommand::MAX_COMMAND;
}

// Ring buffer operations
bool BenQ::enqueue_command_(const char *cmd, const char *value) {
  const char *val = value != nullptr ? value : "?";
  // Check for duplicate — skip if same command+value already in queue
  for (uint8_t i = 0; i < this->queue_count_; i++) {
    auto &entry = this->command_queue_[(this->queue_head_ + i) % MAX_QUEUE_SIZE];
    if (strcmp(entry.command_name, cmd) == 0 && strcmp(entry.value, val) == 0) {
      return true;  // Already queued, treat as success
    }
  }
  if (this->queue_count_ >= MAX_QUEUE_SIZE) {
    return false;
  }
  auto &entry = this->command_queue_[this->queue_tail_];
  strncpy(entry.command_name, cmd, sizeof(entry.command_name) - 1);
  entry.command_name[sizeof(entry.command_name) - 1] = '\0';
  strncpy(entry.value, value != nullptr ? value : "?", sizeof(entry.value) - 1);
  entry.value[sizeof(entry.value) - 1] = '\0';
  this->queue_tail_ = (this->queue_tail_ + 1) % MAX_QUEUE_SIZE;
  this->queue_count_++;
  return true;
}

bool BenQ::enqueue_command_front_(const char *cmd, const char *value) {
  if (this->queue_count_ >= MAX_QUEUE_SIZE) {
    return false;
  }
  // Move head back one slot and insert there
  this->queue_head_ = (this->queue_head_ + MAX_QUEUE_SIZE - 1) % MAX_QUEUE_SIZE;
  auto &entry = this->command_queue_[this->queue_head_];
  strncpy(entry.command_name, cmd, sizeof(entry.command_name) - 1);
  entry.command_name[sizeof(entry.command_name) - 1] = '\0';
  strncpy(entry.value, value != nullptr ? value : "?", sizeof(entry.value) - 1);
  entry.value[sizeof(entry.value) - 1] = '\0';
  this->queue_count_++;
  return true;
}

bool BenQ::dequeue_command_(PendingCommand &out) {
  if (this->queue_count_ == 0) {
    return false;
  }
  out = this->command_queue_[this->queue_head_];
  this->queue_head_ = (this->queue_head_ + 1) % MAX_QUEUE_SIZE;
  this->queue_count_--;
  return true;
}

// Entity registration
void BenQ::register_sensor(BenqSensor *sensor) {
  if (this->sensor_count_ < MAX_SENSORS) {
    this->sensors_[this->sensor_count_++] = sensor;
  }
}

void BenQ::register_switch(BenqSwitch *sw) {
  if (this->switch_count_ < MAX_SWITCHES) {
    this->switches_[this->switch_count_++] = sw;
  }
}

void BenQ::register_number(BenqNumber *num) {
  if (this->number_count_ < MAX_NUMBERS) {
    this->numbers_[this->number_count_++] = num;
  }
}

void BenQ::register_select(BenqSelect *sel) {
  if (this->select_count_ < MAX_SELECTS) {
    this->selects_[this->select_count_++] = sel;
  }
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

  this->process_command_queue_();

  while (this->available()) {
    uint8_t c;
    this->read_byte(&c);

    if (c == 0x00 || c == '>') {
      continue;
    }

    if (c == '#') {
      if (this->rx_len_ > 0) {
        this->rx_buffer_[this->rx_len_++] = '#';
        this->rx_buffer_[this->rx_len_] = '\0';
        ESP_LOGD(TAG, "Complete message received: %s", this->rx_buffer_);
        this->handle_line_();
        this->rx_len_ = 0;
      }
    } else if (c != '\r' && c != '\n') {
      if (this->rx_len_ < sizeof(this->rx_buffer_) - 2) {
        this->rx_buffer_[this->rx_len_++] = static_cast<char>(c);
      }
    }
  }
}

void BenQ::update() {
  // Skip scheduling new queries if the previous cycle is still processing
  if (this->waiting_for_response_ || this->queue_count_ > 0) {
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
  ESP_LOGCONFIG(TAG, "  Sensors: %u", this->sensor_count_);
  ESP_LOGCONFIG(TAG, "  Switches: %u", this->switch_count_);
  ESP_LOGCONFIG(TAG, "  Numbers: %u", this->number_count_);
  ESP_LOGCONFIG(TAG, "  Selects: %u", this->select_count_);
  ESP_LOGCONFIG(TAG, "  Media Player: %s", this->media_player_ != nullptr ? "yes" : "no");
}

void BenQ::send_command(BenqCommand cmd, const char *value) {
  const char *cmd_name = get_command_name(cmd);
  if (cmd_name[0] == '\0') {
    ESP_LOGW(TAG, "Unknown command enum: %d", static_cast<int>(cmd));
    return;
  }
  this->write_command_(cmd_name, value);
}

void BenQ::query_command(BenqCommand cmd) { this->send_command(cmd, "?"); }

void BenQ::check_power_state_change_(bool new_power_state) {
  if (new_power_state && !this->last_power_state_) {
    ESP_LOGI(TAG, "Power turned on");
    this->should_query_entities_ = true;
  }
  if (!new_power_state && this->last_power_state_) {
    ESP_LOGI(TAG, "Power turned off, clearing command queue");
    this->queue_head_ = 0;
    this->queue_tail_ = 0;
    this->queue_count_ = 0;
    this->should_query_entities_ = false;
  }
  this->last_power_state_ = new_power_state;
}

void BenQ::query_all_entities_() {
  ESP_LOGD(TAG, "Querying state of all entities...");
  for (uint8_t i = 0; i < this->sensor_count_; i++) {
    this->query_command(this->sensors_[i]->get_command());
  }
  for (uint8_t i = 0; i < this->switch_count_; i++) {
    this->query_command(this->switches_[i]->get_command());
  }
  for (uint8_t i = 0; i < this->number_count_; i++) {
    this->query_command(this->numbers_[i]->get_command());
  }
  for (uint8_t i = 0; i < this->select_count_; i++) {
    this->query_command(this->selects_[i]->get_command());
  }
}

void BenQ::dispatch_response_(const BenqResponse &response) {
  BenqCommand cmd = get_command_by_name_(response.command);
  if (cmd == BenqCommand::MAX_COMMAND) {
    return;
  }

  // Track power state in hub
  if (cmd == BenqCommand::POWER) {
    bool is_on = (strcasecmp(response.value, "on") == 0);
    this->check_power_state_change_(is_on);
  }

  // Dispatch to registered sensors
  for (uint8_t i = 0; i < this->sensor_count_; i++) {
    if (this->sensors_[i]->get_command() == cmd) {
      this->sensors_[i]->handle_response(response);
    }
  }
  // Dispatch to registered switches
  for (uint8_t i = 0; i < this->switch_count_; i++) {
    if (this->switches_[i]->get_command() == cmd) {
      this->switches_[i]->handle_response(response);
    }
  }
  // Dispatch to registered numbers
  for (uint8_t i = 0; i < this->number_count_; i++) {
    if (this->numbers_[i]->get_command() == cmd) {
      this->numbers_[i]->handle_response(response);
    }
  }
  // Dispatch to registered selects
  for (uint8_t i = 0; i < this->select_count_; i++) {
    if (this->selects_[i]->get_command() == cmd) {
      this->selects_[i]->handle_response(response);
    }
  }
  // Dispatch to media player
  if (this->media_player_ != nullptr) {
    this->media_player_->handle_response(cmd, response);
  }
}

void BenQ::handle_line_() {
  BenqResponse response = this->parse_response_(this->rx_buffer_, this->rx_len_);

  if (response.is_error) {
    if (response.command[0] != '\0') {
      ESP_LOGW(TAG, "RX Error on %s: %s", response.command, response.error_message);
    } else {
      ESP_LOGW(TAG, "RX Error: %s", response.error_message);
    }
    this->waiting_for_response_ = false;
  } else if (response.success) {
    if (strcmp(response.value, "?") == 0) {
      ESP_LOGV(TAG, "RX Query echo: %s=?", response.command);
      return;
    }
    ESP_LOGD(TAG, "RX: %s=%s", response.command, response.value);
    this->waiting_for_response_ = false;
  } else {
    ESP_LOGV(TAG, "Could not parse: %s", this->rx_buffer_);
    return;
  }

  this->dispatch_response_(response);
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

      if (cmd_len < sizeof(response.command)) {
        memcpy(response.command, line + 1, cmd_len);
        response.command[cmd_len] = '\0';
      }
      if (val_len < sizeof(response.value)) {
        memcpy(response.value, eq + 1, val_len);
        response.value[val_len] = '\0';
      }

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

void BenQ::write_command_(const char *cmd, const char *value) {
  if (this->waiting_for_response_) {
    bool is_query = (value == nullptr || strcmp(value, "?") == 0);
    if (!is_query) {
      // Set commands go to the front of the queue so they're sent next
      if (!this->enqueue_command_front_(cmd, value)) {
        ESP_LOGW(TAG, "Command queue full, dropping: %s=%s", cmd, value);
        return;
      }
      ESP_LOGD(TAG, "Priority queuing set command: %s=%s", cmd, value);
    } else {
      if (!this->enqueue_command_(cmd, value)) {
        ESP_LOGW(TAG, "Command queue full, dropping: %s=%s", cmd, value != nullptr ? value : "?");
        return;
      }
      ESP_LOGD(TAG, "Busy, queuing command: %s=%s", cmd, value != nullptr ? value : "?");
    }
    return;
  }
  this->send_raw_command_(cmd, value);
}

void BenQ::clear_uart_buffer_() {
  while (this->available()) {
    this->read();
  }
}

void BenQ::process_command_queue_() {
  if (this->waiting_for_response_) {
    if (millis() - this->last_command_time_ >= COMMAND_TIMEOUT_MS) {
      ESP_LOGD(TAG, "No response received for '%s' in %u ms, moving on", this->pending_command_, COMMAND_TIMEOUT_MS);
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
  if (!this->dequeue_command_(pending)) {
    return;
  }

  ESP_LOGD(TAG, "Processing queued command: %s=%s (queue size: %u)", pending.command_name, pending.value,
           this->queue_count_);
  this->send_raw_command_(pending.command_name, pending.value);
}

void BenQ::send_raw_command_(const char *cmd, const char *value) {
  this->write_str("\r*");
  this->write_str(cmd);
  this->write_str("=");
  this->write_str(value != nullptr ? value : "?");
  this->write_str("#\r");
  this->flush();

  ESP_LOGD(TAG, "TX: *%s=%s#", cmd, value != nullptr ? value : "?");
  this->waiting_for_response_ = true;
  strncpy(this->pending_command_, cmd, sizeof(this->pending_command_) - 1);
  this->pending_command_[sizeof(this->pending_command_) - 1] = '\0';
  this->last_command_time_ = millis();
}

}  // namespace esphome::benq
