#include "petwalk_esphome.h"

#include <algorithm>
#include <string>

#include "esphome/core/log.h"

namespace esphome {
namespace petwalk_esphome {

static const char *const TAG = "petwalk_esphome";

void PetwalkBitBinarySensor::publish_from_frame(const uint8_t *frame, uint8_t frame_bits) {
  if (this->bit_ == 0 || this->bit_ > frame_bits)
    return;

  const bool raw = frame[this->bit_ - 1] != 0;
  const bool state = this->active_low_ ? !raw : raw;
  const uint32_t now = millis();

  if (!this->candidate_valid_ || state != this->candidate_state_) {
    this->candidate_valid_ = true;
    this->candidate_state_ = state;
    this->candidate_since_ms_ = now;
  }

  if (this->published_valid_ && this->candidate_state_ == this->published_state_)
    return;

  const uint32_t required_ms = this->candidate_state_ ? this->minimum_on_time_ms_ : this->minimum_off_time_ms_;
  if (static_cast<uint32_t>(now - this->candidate_since_ms_) < required_ms)
    return;

  this->published_state_ = this->candidate_state_;
  this->published_valid_ = true;
  this->publish_state(this->published_state_);
}

void PetwalkBitSwitch::publish_from_frame(const uint8_t *frame, uint8_t frame_bits) {
  if (this->bit_ == 0 || this->bit_ > frame_bits)
    return;

  const bool raw = frame[this->bit_ - 1] != 0;
  const bool state = this->active_low_ ? !raw : raw;
  const uint32_t now = millis();

  if (!this->candidate_valid_ || state != this->candidate_state_) {
    this->candidate_valid_ = true;
    this->candidate_state_ = state;
    this->candidate_since_ms_ = now;
  }

  if (this->published_valid_ && this->candidate_state_ == this->published_state_)
    return;

  const uint32_t required_ms = this->candidate_state_ ? this->minimum_on_time_ms_ : this->minimum_off_time_ms_;
  if (static_cast<uint32_t>(now - this->candidate_since_ms_) < required_ms)
    return;

  this->published_state_ = this->candidate_state_;
  this->published_valid_ = true;
  this->publish_state(this->published_state_);
}

void PetwalkBitSwitch::write_state(bool state) {
  // The petWALK RC5 command is a toggle command. The requested state is therefore
  // not published optimistically; the display-bus bit remains the source of truth.
  if (this->published_valid_ && state == this->published_state_)
    return;

  if (this->transmitter_ == nullptr) {
    ESP_LOGE(TAG, "Cannot send RC5 command: no remote transmitter configured");
    return;
  }

  remote_base::RC5Data data{};
  data.address = this->address_;
  data.command = this->command_;
  this->transmitter_->transmit<remote_base::RC5Protocol>(data, this->repeat_times_, this->repeat_wait_us_);

  ESP_LOGD(TAG, "Sent RC5 for bit %u: address=0x%02X command=0x%02X repeats=%u wait=%u us",
           this->bit_, this->address_, this->command_, static_cast<unsigned>(this->repeat_times_),
           static_cast<unsigned>(this->repeat_wait_us_));
}


bool PetwalkDisplayTextSensor::segment_on_(const uint8_t *frame, uint8_t frame_bits, uint8_t bit) const {
  if (bit == 0 || bit > frame_bits)
    return false;
  const bool raw = frame[bit - 1] != 0;
  return this->active_low_ ? !raw : raw;
}

char PetwalkDisplayTextSensor::decode_digit_(const uint8_t *frame, uint8_t frame_bits, uint8_t position) const {
  // petWALK sends the four digits in descending bit ranges:
  // position 1: G=50 F=51 E=52 D=53 C=54 B=55 A=56
  // position 2: G=42 F=43 E=44 D=45 C=46 B=47 A=48
  // position 3: G=34 F=35 E=36 D=37 C=38 B=39 A=40
  // position 4: G=26 F=27 E=28 D=29 C=30 B=31 A=32
  if (position < 1 || position > 4)
    return '?';

  const uint8_t base = static_cast<uint8_t>(58 - position * 8);  // G bit: 50,42,34,26
  uint8_t mask = 0;
  if (this->segment_on_(frame, frame_bits, base + 6)) mask |= 0x40;  // A
  if (this->segment_on_(frame, frame_bits, base + 5)) mask |= 0x20;  // B
  if (this->segment_on_(frame, frame_bits, base + 4)) mask |= 0x10;  // C
  if (this->segment_on_(frame, frame_bits, base + 3)) mask |= 0x08;  // D
  if (this->segment_on_(frame, frame_bits, base + 2)) mask |= 0x04;  // E
  if (this->segment_on_(frame, frame_bits, base + 1)) mask |= 0x02;  // F
  if (this->segment_on_(frame, frame_bits, base + 0)) mask |= 0x01;  // G
  return this->decode_mask_(mask);
}

char PetwalkDisplayTextSensor::decode_mask_(uint8_t mask) const {
  // Mask order is A B C D E F G (A = bit 6, G = bit 0).
  // Duplicate patterns are inherently ambiguous: 0/O and 1/I. We return
  // the numeric character, which also keeps time decoding deterministic.
  switch (mask) {
    case 0b0000000: return ' ';
    case 0b1111110: return '0';
    case 0b0110000: return '1';
    case 0b1101101: return '2';
    case 0b1111001: return '3';
    case 0b0110011: return '4';
    case 0b1011011: return '5';
    case 0b1011111: return '6';
    case 0b1110000: return '7';
    case 0b1111111: return '8';
    case 0b1111011: return '9';
    case 0b1110111: return 'A';
    case 0b1001110: return 'C';
    case 0b1001111: return 'E';
    case 0b1000111: return 'F';
    case 0b0110111: return 'H';
    case 0b0001110: return 'L';
    case 0b1100111: return 'P';
    case 0b0111110: return 'U';
    case 0b0001101: return 'c';
    case 0b0111101: return 'd';
    case 0b0010111: return 'h';
    case 0b0010000: return 'i';
    case 0b0010101: return 'n';
    case 0b0011101: return 'o';
    case 0b0000101: return 'r';
    case 0b0011100: return 'u';
    default: return '?';
  }
}

std::string PetwalkDisplayTextSensor::decode_display_(const uint8_t *frame, uint8_t frame_bits) const {
  std::string digits;
  digits.reserve(5);
  bool all_numeric = true;

  for (uint8_t position = 1; position <= 4; position++) {
    const char value = this->decode_digit_(frame, frame_bits, position);
    digits.push_back(value);
    if (value < '0' || value > '9')
      all_numeric = false;
  }

  if (this->insert_time_separator_ && all_numeric)
    digits.insert(digits.begin() + 2, this->time_separator_);

  if (this->trim_spaces_) {
    const auto first = digits.find_first_not_of(' ');
    if (first == std::string::npos)
      return "";
    const auto last = digits.find_last_not_of(' ');
    digits = digits.substr(first, last - first + 1);
  }

  return digits;
}

bool PetwalkDisplayTextSensor::seen_recently(const std::string &value, uint32_t window_ms) const {
  const uint32_t now = millis();
  std::string current(this->raw_digits_.begin(), this->raw_digits_.end());
  if (current == value)
    return true;
  return this->last_non_blank_raw_ == value &&
         static_cast<uint32_t>(now - this->last_non_blank_time_ms_) <= window_ms;
}

void PetwalkDisplayTextSensor::publish_from_frame(const uint8_t *frame, uint8_t frame_bits) {
  if (frame_bits < 56)
    return;

  const uint32_t now = millis();
  bool any_non_blank = false;
  for (uint8_t position = 1; position <= 4; position++) {
    const char value = this->decode_digit_(frame, frame_bits, position);
    this->raw_digits_[position - 1] = value;
    if (value != ' ')
      any_non_blank = true;
  }
  this->last_frame_time_ms_ = now;
  if (any_non_blank) {
    this->last_non_blank_raw_.assign(this->raw_digits_.begin(), this->raw_digits_.end());
    this->last_non_blank_time_ms_ = now;
  }

  const std::string state = this->decode_display_(frame, frame_bits);

  if (!this->candidate_valid_ || state != this->candidate_state_) {
    this->candidate_valid_ = true;
    this->candidate_state_ = state;
    this->candidate_since_ms_ = now;
  }

  if (this->published_valid_ && this->candidate_state_ == this->published_state_)
    return;

  if (static_cast<uint32_t>(now - this->candidate_since_ms_) < this->minimum_state_time_ms_)
    return;

  this->published_state_ = this->candidate_state_;
  this->published_valid_ = true;
  this->publish_state(this->published_state_);
}


constexpr std::array<uint8_t, 7> PetwalkClockSyncButton::MENU_BITS;

void PetwalkClockSyncButton::dump_config() {
  ESP_LOGCONFIG(TAG, "petWALK clock sync button:");
  ESP_LOGCONFIG(TAG, "  RC5 address: 0x%02X", this->address_);
  ESP_LOGCONFIG(TAG, "  Commands: MENU=0x%02X TIME_PROGRAM=0x%02X OK=0x%02X UP=0x%02X DOWN=0x%02X",
                this->menu_command_, this->time_program_command_, this->ok_command_, this->up_command_,
                this->down_command_);
  ESP_LOGCONFIG(TAG, "  Logical key press: 2 RC5 frames, %u us apart", static_cast<unsigned>(this->repeat_wait_us_));
  ESP_LOGCONFIG(TAG, "  Delay between two logical MENU/OK presses: %u ms",
                static_cast<unsigned>(this->second_press_delay_ms_));
  ESP_LOGCONFIG(TAG, "  Target lead: %u minute(s)", this->target_lead_minutes_);
}

void PetwalkClockSyncButton::send_key_(uint8_t command) {
  if (this->transmitter_ == nullptr)
    return;
  remote_base::RC5Data data{};
  data.address = this->address_;
  data.command = command;
  // One logical key press is always exactly two equal RC5 commands with 82 ms
  // (configurable) between them. A single RC5 command is ignored by petWALK.
  this->transmitter_->transmit<remote_base::RC5Protocol>(data, 2, this->repeat_wait_us_);
  ESP_LOGD(TAG, "Clock sync: key command=0x%02X (2 frames, %u us gap)", command,
           static_cast<unsigned>(this->repeat_wait_us_));
}

void PetwalkClockSyncButton::set_state_(State state, uint32_t timeout_ms) {
  this->state_ = state;
  this->state_since_ms_ = millis();
  this->state_deadline_ms_ = timeout_ms == 0 ? 0 : this->state_since_ms_ + timeout_ms;
}

void PetwalkClockSyncButton::reset_menu_detection_() {
  this->menu_state_valid_.fill(false);
  this->menu_toggle_counts_.fill(0);
}

bool PetwalkClockSyncButton::menu_pattern_detected_() const {
  uint8_t toggling = 0;
  for (uint8_t count : this->menu_toggle_counts_) {
    if (count >= 2)
      toggling++;
  }
  return toggling >= 5;
}

void PetwalkClockSyncButton::publish_from_frame(const uint8_t *frame, uint8_t frame_bits) {
  if (this->state_ != State::WAIT_MENU || frame_bits < 23)
    return;

  for (size_t i = 0; i < MENU_BITS.size(); i++) {
    const uint8_t bit = MENU_BITS[i];
    const bool state = frame[bit - 1] == 0;  // petWALK LEDs are active-low.
    if (!this->menu_state_valid_[i]) {
      this->menu_state_valid_[i] = true;
      this->menu_last_states_[i] = state;
    } else if (state != this->menu_last_states_[i]) {
      this->menu_last_states_[i] = state;
      if (this->menu_toggle_counts_[i] < 255)
        this->menu_toggle_counts_[i]++;
    }
  }
}

bool PetwalkClockSyncButton::parse_hour_(uint8_t &hour) const {
  if (this->display_ == nullptr)
    return false;
  const auto &d = this->display_->get_raw_digits();
  if (d[0] < '0' || d[0] > '9' || d[1] < '0' || d[1] > '9' || d[2] != ' ' || d[3] != ' ')
    return false;
  const uint8_t value = static_cast<uint8_t>((d[0] - '0') * 10 + (d[1] - '0'));
  if (value > 23)
    return false;
  hour = value;
  return true;
}

bool PetwalkClockSyncButton::parse_time_(uint8_t &hour, uint8_t &minute) const {
  if (this->display_ == nullptr)
    return false;
  const auto &d = this->display_->get_raw_digits();
  for (char value : d) {
    if (value < '0' || value > '9')
      return false;
  }
  const uint8_t parsed_hour = static_cast<uint8_t>((d[0] - '0') * 10 + (d[1] - '0'));
  const uint8_t parsed_minute = static_cast<uint8_t>((d[2] - '0') * 10 + (d[3] - '0'));
  if (parsed_hour > 23 || parsed_minute > 59)
    return false;
  hour = parsed_hour;
  minute = parsed_minute;
  return true;
}

int8_t PetwalkClockSyncButton::shortest_delta_(uint8_t current, uint8_t target, uint8_t modulo) const {
  int16_t delta = (static_cast<int16_t>(target) - static_cast<int16_t>(current) + modulo) % modulo;
  if (delta > modulo / 2)
    delta -= modulo;
  return static_cast<int8_t>(delta);
}

void PetwalkClockSyncButton::press_action() {
  if (this->state_ != State::IDLE) {
    ESP_LOGW(TAG, "Clock sync is already running");
    return;
  }
  if (this->display_ == nullptr || this->clock_ == nullptr || this->transmitter_ == nullptr) {
    ESP_LOGE(TAG, "Clock sync cannot start: display, time source or transmitter is missing");
    return;
  }

  const auto now = this->clock_->now();
  if (!now.is_valid()) {
    ESP_LOGE(TAG, "Clock sync cannot start: ESPHome time is not valid");
    return;
  }
  if (static_cast<uint32_t>(millis() - this->display_->get_last_frame_time()) > 1000) {
    ESP_LOGE(TAG, "Clock sync cannot start: no current display frames");
    return;
  }

  uint16_t target_total = static_cast<uint16_t>(now.hour) * 60U + now.minute + this->target_lead_minutes_;
  target_total %= 24U * 60U;
  this->target_hour_ = static_cast<uint8_t>(target_total / 60U);
  this->target_minute_ = static_cast<uint8_t>(target_total % 60U);
  this->previous_value_valid_ = false;
  this->aborting_ = false;
  this->reset_menu_detection_();

  ESP_LOGI(TAG, "Clock sync started; target is %02u:%02u", this->target_hour_, this->target_minute_);
  this->send_key_(this->menu_command_);
  this->set_state_(State::WAIT_MENU, this->state_timeout_ms_);
}

void PetwalkClockSyncButton::fail_(const char *reason) {
  ESP_LOGE(TAG, "Clock sync failed: %s", reason);
  this->start_abort_();
}

void PetwalkClockSyncButton::start_abort_() {
  if (this->aborting_)
    return;
  this->aborting_ = true;
  // Emergency exit is two logical MENU key presses. Each logical press itself
  // consists of two RC5 commands separated by 82 ms.
  this->send_key_(this->menu_command_);
  this->not_before_ms_ = millis() + this->second_press_delay_ms_;
  this->set_state_(State::ABORT_WAIT_SECOND_MENU, this->second_press_delay_ms_ + 1000);
}

void PetwalkClockSyncButton::loop() {
  if (this->state_ == State::IDLE)
    return;

  const uint32_t now_ms = millis();
  if (this->state_deadline_ms_ != 0 && static_cast<int32_t>(now_ms - this->state_deadline_ms_) >= 0 &&
      this->state_ != State::WAIT_TARGET_MINUTE && this->state_ != State::ABORT_WAIT_SECOND_MENU &&
      this->state_ != State::ABORT_FINISH) {
    this->fail_("timeout while waiting for the expected display state");
    return;
  }

  uint8_t hour = 0;
  uint8_t minute = 0;

  switch (this->state_) {
    case State::IDLE:
      break;

    case State::WAIT_MENU:
      if (this->menu_pattern_detected_()) {
        ESP_LOGI(TAG, "Clock sync: menu mode detected");
        this->send_key_(this->time_program_command_);
        this->set_state_(State::WAIT_24H, this->state_timeout_ms_);
      }
      break;

    case State::WAIT_24H:
      if (this->display_->seen_recently(" 24h", 1500)) {
        ESP_LOGI(TAG, "Clock sync: 24h display detected");
        this->send_key_(this->ok_command_);
        this->set_state_(State::WAIT_HOUR, this->state_timeout_ms_);
      }
      break;

    case State::WAIT_HOUR:
      if (!this->parse_hour_(hour))
        break;
      {
        const int8_t delta = this->shortest_delta_(hour, this->target_hour_, 24);
        if (delta == 0) {
          ESP_LOGI(TAG, "Clock sync: hour set to %02u", hour);
          this->send_key_(this->ok_command_);
          this->set_state_(State::WAIT_MINUTE, this->state_timeout_ms_);
        } else {
          this->previous_value_ = hour;
          this->previous_value_valid_ = true;
          this->send_key_(delta > 0 ? this->up_command_ : this->down_command_);
          this->set_state_(State::WAIT_HOUR_STEP, this->step_timeout_ms_);
        }
      }
      break;

    case State::WAIT_HOUR_STEP:
      if (this->parse_hour_(hour) && this->previous_value_valid_ && hour != this->previous_value_) {
        this->previous_value_valid_ = false;
        this->set_state_(State::WAIT_HOUR, this->state_timeout_ms_);
      }
      break;

    case State::WAIT_MINUTE:
      if (!this->parse_time_(hour, minute))
        break;
      {
        const int8_t delta = this->shortest_delta_(minute, this->target_minute_, 60);
        if (delta == 0) {
          ESP_LOGI(TAG, "Clock sync: display prepared at %02u:%02u; waiting for system minute", hour, minute);
          this->set_state_(State::WAIT_TARGET_MINUTE, 180000);
        } else {
          this->previous_value_ = minute;
          this->previous_value_valid_ = true;
          this->send_key_(delta > 0 ? this->up_command_ : this->down_command_);
          this->set_state_(State::WAIT_MINUTE_STEP, this->step_timeout_ms_);
        }
      }
      break;

    case State::WAIT_MINUTE_STEP:
      if (this->parse_time_(hour, minute) && this->previous_value_valid_ && minute != this->previous_value_) {
        this->previous_value_valid_ = false;
        this->set_state_(State::WAIT_MINUTE, this->state_timeout_ms_);
      }
      break;

    case State::WAIT_TARGET_MINUTE: {
      const auto now = this->clock_->now();
      if (!now.is_valid()) {
        this->fail_("time source became invalid");
        break;
      }
      if (now.hour == this->target_hour_ && now.minute == this->target_minute_ && now.second == 0) {
        // First logical OK press: save the configured time. The petWALK clock
        // starts running immediately when this key press is accepted.
        ESP_LOGI(TAG, "Clock sync: saving at system time %02u:%02u:%02u", now.hour, now.minute, now.second);
        this->send_key_(this->ok_command_);
        this->not_before_ms_ = now_ms + this->second_press_delay_ms_;
        this->set_state_(State::WAIT_AFTER_SAVE, this->second_press_delay_ms_ + this->state_timeout_ms_);
      } else if (static_cast<uint32_t>(now_ms - this->state_since_ms_) > 180000) {
        this->fail_("target minute was not reached in time");
      }
      break;
    }

    case State::WAIT_AFTER_SAVE:
      if (static_cast<int32_t>(now_ms - this->not_before_ms_) >= 0) {
        // Second separate logical OK key press after two seconds: leave menu.
        this->send_key_(this->ok_command_);
        this->not_before_ms_ = now_ms + 1500;
        this->set_state_(State::WAIT_AFTER_EXIT, this->state_timeout_ms_);
      }
      break;

    case State::WAIT_AFTER_EXIT:
      if (static_cast<int32_t>(now_ms - this->not_before_ms_) >= 0) {
        ESP_LOGI(TAG, "Clock sync completed successfully");
        this->state_ = State::IDLE;
        this->aborting_ = false;
      }
      break;

    case State::ABORT_WAIT_SECOND_MENU:
      if (static_cast<int32_t>(now_ms - this->not_before_ms_) >= 0) {
        this->send_key_(this->menu_command_);
        this->not_before_ms_ = now_ms + 1500;
        this->set_state_(State::ABORT_FINISH, this->state_timeout_ms_);
      }
      break;

    case State::ABORT_FINISH:
      if (static_cast<int32_t>(now_ms - this->not_before_ms_) >= 0) {
        ESP_LOGW(TAG, "Clock sync aborted; two logical MENU presses were sent");
        this->state_ = State::IDLE;
        this->aborting_ = false;
      }
      break;
  }
}

void PetwalkEsphome::setup() {
  this->data_pin_->setup();
  this->clock_pin_->setup();
  this->latch_pin_->setup();
  this->data_isr_pin_ = this->data_pin_->to_isr();

  const auto clock_interrupt = this->clock_edge_ == CLOCK_EDGE_FALLING
                                   ? gpio::INTERRUPT_FALLING_EDGE
                                   : gpio::INTERRUPT_RISING_EDGE;
  const auto latch_interrupt = this->latch_edge_ == LATCH_EDGE_FALLING
                                   ? gpio::INTERRUPT_FALLING_EDGE
                                   : gpio::INTERRUPT_RISING_EDGE;

  this->clock_pin_->attach_interrupt(&PetwalkEsphome::clock_isr, this, clock_interrupt);
  this->latch_pin_->attach_interrupt(&PetwalkEsphome::latch_isr, this, latch_interrupt);
  this->disable_loop();
}

void PetwalkEsphome::dump_config() {
  ESP_LOGCONFIG(TAG, "petWALK display bus:");
  LOG_PIN("  DATA pin: ", this->data_pin_);
  LOG_PIN("  CLOCK pin: ", this->clock_pin_);
  LOG_PIN("  LATCH pin: ", this->latch_pin_);
  ESP_LOGCONFIG(TAG, "  Frame length: %u bits", this->frame_bits_);
  ESP_LOGCONFIG(TAG, "  CLOCK sampling edge: %s", this->clock_edge_ == CLOCK_EDGE_FALLING ? "FALLING" : "RISING");
  ESP_LOGCONFIG(TAG, "  LATCH frame-start edge: %s", this->latch_edge_ == LATCH_EDGE_FALLING ? "FALLING" : "RISING");
  ESP_LOGCONFIG(TAG, "  Registered binary sensors: %u", static_cast<unsigned>(this->binary_sensors_.size()));
  ESP_LOGCONFIG(TAG, "  Registered switches: %u", static_cast<unsigned>(this->switches_.size()));
  ESP_LOGCONFIG(TAG, "  Registered display text sensors: %u", static_cast<unsigned>(this->text_sensors_.size()));
  ESP_LOGCONFIG(TAG, "  Registered clock sync buttons: %u", static_cast<unsigned>(this->clock_sync_buttons_.size()));
}

void IRAM_ATTR PetwalkEsphome::clock_isr(PetwalkEsphome *arg) { arg->handle_clock_isr_(); }
void IRAM_ATTR PetwalkEsphome::latch_isr(PetwalkEsphome *arg) { arg->handle_latch_isr_(); }

void IRAM_ATTR PetwalkEsphome::handle_latch_isr_() {
  this->bit_index_ = 0;
  this->receiving_ = true;
}

void IRAM_ATTR PetwalkEsphome::handle_clock_isr_() {
  if (!this->receiving_)
    return;

  const uint8_t index = this->bit_index_;
  if (index >= this->frame_bits_) {
    this->receiving_ = false;
    return;
  }

  this->frames_[this->write_buffer_][index] = this->data_isr_pin_.digital_read() ? 1 : 0;
  this->bit_index_ = index + 1;

  if (this->bit_index_ == this->frame_bits_) {
    this->receiving_ = false;
    this->completed_buffer_ = this->write_buffer_;
    this->write_buffer_ ^= 1U;
    this->completed_sequence_++;
    this->enable_loop_soon_any_context();
  }
}

void PetwalkEsphome::loop() {
  const uint32_t sequence = this->completed_sequence_;
  if (sequence == this->processed_sequence_) {
    this->disable_loop();
    return;
  }

  const uint8_t completed = this->completed_buffer_;
  this->process_frame_(completed);
  this->processed_sequence_ = sequence;

  if (this->completed_sequence_ == this->processed_sequence_)
    this->disable_loop();
}

void PetwalkEsphome::process_frame_(uint8_t buffer_index) {
  std::array<uint8_t, PETWALK_MAX_FRAME_BITS> frame{};
  for (uint8_t i = 0; i < this->frame_bits_; i++)
    frame[i] = this->frames_[buffer_index][i];

  for (auto *sensor : this->binary_sensors_)
    sensor->publish_from_frame(frame.data(), this->frame_bits_);
  for (auto *petwalk_switch : this->switches_)
    petwalk_switch->publish_from_frame(frame.data(), this->frame_bits_);
  for (auto *text_sensor : this->text_sensors_)
    text_sensor->publish_from_frame(frame.data(), this->frame_bits_);
  for (auto *button : this->clock_sync_buttons_)
    button->publish_from_frame(frame.data(), this->frame_bits_);

  if (this->debug_frames_) {
    std::string text;
    text.reserve(this->frame_bits_);
    for (uint8_t i = 0; i < this->frame_bits_; i++)
      text.push_back(frame[i] ? '1' : '0');
    ESP_LOGD(TAG, "Frame: %s", text.c_str());
  }
}

}  // namespace petwalk_esphome
}  // namespace esphome
