#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/button/button.h"
#include "esphome/components/remote_base/rc5_protocol.h"
#include "esphome/components/remote_base/remote_base.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/time/real_time_clock.h"
#include "esphome/core/component.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace petwalk_esphome {

static constexpr uint8_t PETWALK_MAX_FRAME_BITS = 64;

enum ClockEdge : uint8_t {
  CLOCK_EDGE_FALLING = 0,
  CLOCK_EDGE_RISING = 1,
};

enum LatchEdge : uint8_t {
  LATCH_EDGE_FALLING = 0,
  LATCH_EDGE_RISING = 1,
};

class PetwalkBitBinarySensor : public binary_sensor::BinarySensor {
 public:
  void set_bit(uint8_t bit) { this->bit_ = bit; }
  void set_active_low(bool active_low) { this->active_low_ = active_low; }
  void set_minimum_on_time(uint32_t milliseconds) { this->minimum_on_time_ms_ = milliseconds; }
  void set_minimum_off_time(uint32_t milliseconds) { this->minimum_off_time_ms_ = milliseconds; }
  void publish_from_frame(const uint8_t *frame, uint8_t frame_bits);

 protected:
  uint8_t bit_{1};
  bool active_low_{true};
  uint32_t minimum_on_time_ms_{0};
  uint32_t minimum_off_time_ms_{0};
  bool candidate_valid_{false};
  bool candidate_state_{false};
  uint32_t candidate_since_ms_{0};
  bool published_valid_{false};
  bool published_state_{false};
};

class PetwalkBitSwitch : public switch_::Switch {
 public:
  void set_bit(uint8_t bit) { this->bit_ = bit; }
  void set_active_low(bool active_low) { this->active_low_ = active_low; }
  void set_minimum_on_time(uint32_t milliseconds) { this->minimum_on_time_ms_ = milliseconds; }
  void set_minimum_off_time(uint32_t milliseconds) { this->minimum_off_time_ms_ = milliseconds; }
  void set_transmitter(remote_base::RemoteTransmitterBase *transmitter) { this->transmitter_ = transmitter; }
  void set_address(uint8_t address) { this->address_ = address; }
  void set_command(uint8_t command) { this->command_ = command; }
  void set_repeat_times(uint32_t times) { this->repeat_times_ = times; }
  void set_repeat_wait(uint32_t microseconds) { this->repeat_wait_us_ = microseconds; }
  void publish_from_frame(const uint8_t *frame, uint8_t frame_bits);

 protected:
  void write_state(bool state) override;

  uint8_t bit_{1};
  bool active_low_{true};
  uint32_t minimum_on_time_ms_{0};
  uint32_t minimum_off_time_ms_{0};
  bool candidate_valid_{false};
  bool candidate_state_{false};
  uint32_t candidate_since_ms_{0};
  bool published_valid_{false};
  bool published_state_{false};

  remote_base::RemoteTransmitterBase *transmitter_{nullptr};
  uint8_t address_{0};
  uint8_t command_{0};
  uint32_t repeat_times_{1};
  uint32_t repeat_wait_us_{0};
};


class PetwalkDisplayTextSensor : public text_sensor::TextSensor {
 public:
  void set_active_low(bool active_low) { this->active_low_ = active_low; }
  void set_minimum_state_time(uint32_t milliseconds) { this->minimum_state_time_ms_ = milliseconds; }
  void set_insert_time_separator(bool insert) { this->insert_time_separator_ = insert; }
  void set_time_separator(const std::string &separator) {
    if (!separator.empty())
      this->time_separator_ = separator[0];
  }
  void set_trim_spaces(bool trim_spaces) { this->trim_spaces_ = trim_spaces; }
  void publish_from_frame(const uint8_t *frame, uint8_t frame_bits);
  const std::array<char, 4> &get_raw_digits() const { return this->raw_digits_; }
  uint32_t get_last_frame_time() const { return this->last_frame_time_ms_; }
  bool seen_recently(const std::string &value, uint32_t window_ms) const;

 protected:
  bool segment_on_(const uint8_t *frame, uint8_t frame_bits, uint8_t bit) const;
  char decode_digit_(const uint8_t *frame, uint8_t frame_bits, uint8_t position) const;
  char decode_mask_(uint8_t mask) const;
  std::string decode_display_(const uint8_t *frame, uint8_t frame_bits) const;

  bool active_low_{true};
  uint32_t minimum_state_time_ms_{0};
  bool insert_time_separator_{true};
  char time_separator_{'.'};
  bool trim_spaces_{true};

  bool candidate_valid_{false};
  std::string candidate_state_{};
  uint32_t candidate_since_ms_{0};
  bool published_valid_{false};
  std::string published_state_{};
  std::array<char, 4> raw_digits_{{' ', ' ', ' ', ' '}};
  std::string last_non_blank_raw_{};
  uint32_t last_non_blank_time_ms_{0};
  uint32_t last_frame_time_ms_{0};
};

class PetwalkEsphome;

class PetwalkClockSyncButton : public button::Button, public Component {
 public:
  void set_parent(PetwalkEsphome *parent) { this->parent_ = parent; }
  void set_display(PetwalkDisplayTextSensor *display) { this->display_ = display; }
  void set_clock(time::RealTimeClock *clock) { this->clock_ = clock; }
  void set_transmitter(remote_base::RemoteTransmitterBase *transmitter) { this->transmitter_ = transmitter; }
  void set_address(uint8_t address) { this->address_ = address; }
  void set_menu_command(uint8_t command) { this->menu_command_ = command; }
  void set_up_command(uint8_t command) { this->up_command_ = command; }
  void set_down_command(uint8_t command) { this->down_command_ = command; }
  void set_ok_command(uint8_t command) { this->ok_command_ = command; }
  void set_time_program_command(uint8_t command) { this->time_program_command_ = command; }
  void set_repeat_wait(uint32_t microseconds) { this->repeat_wait_us_ = microseconds; }
  void set_second_press_delay(uint32_t milliseconds) { this->second_press_delay_ms_ = milliseconds; }
  void set_step_timeout(uint32_t milliseconds) { this->step_timeout_ms_ = milliseconds; }
  void set_state_timeout(uint32_t milliseconds) { this->state_timeout_ms_ = milliseconds; }
  void set_target_lead_minutes(uint8_t minutes) { this->target_lead_minutes_ = minutes; }
  void publish_from_frame(const uint8_t *frame, uint8_t frame_bits);
  void loop() override;
  void dump_config() override;

 protected:
  enum class State : uint8_t {
    IDLE,
    WAIT_MENU,
    WAIT_24H,
    WAIT_HOUR,
    WAIT_HOUR_STEP,
    WAIT_MINUTE,
    WAIT_MINUTE_STEP,
    WAIT_TARGET_MINUTE,
    WAIT_AFTER_SAVE,
    WAIT_AFTER_EXIT,
    ABORT_WAIT_SECOND_MENU,
    ABORT_FINISH,
  };

  void press_action() override;
  void send_key_(uint8_t command);
  void set_state_(State state, uint32_t timeout_ms);
  void fail_(const char *reason);
  void start_abort_();
  bool parse_hour_(uint8_t &hour) const;
  bool parse_time_(uint8_t &hour, uint8_t &minute) const;
  int8_t shortest_delta_(uint8_t current, uint8_t target, uint8_t modulo) const;
  bool menu_pattern_detected_() const;
  void reset_menu_detection_();

  PetwalkEsphome *parent_{nullptr};
  PetwalkDisplayTextSensor *display_{nullptr};
  time::RealTimeClock *clock_{nullptr};
  remote_base::RemoteTransmitterBase *transmitter_{nullptr};
  uint8_t address_{0x04};
  uint8_t menu_command_{0x0C};
  uint8_t up_command_{0x0E};
  uint8_t down_command_{0x14};
  uint8_t ok_command_{0x11};
  uint8_t time_program_command_{0x15};
  uint32_t repeat_wait_us_{82000};
  uint32_t second_press_delay_ms_{3000};
  uint32_t step_timeout_ms_{2500};
  uint32_t state_timeout_ms_{5000};
  uint8_t target_lead_minutes_{1};

  State state_{State::IDLE};
  uint32_t state_since_ms_{0};
  uint32_t state_deadline_ms_{0};
  uint32_t not_before_ms_{0};
  uint8_t target_hour_{0};
  uint8_t target_minute_{0};
  uint8_t previous_value_{0};
  bool previous_value_valid_{false};
  bool aborting_{false};
  uint8_t exit_retry_count_{0};

  static constexpr std::array<uint8_t, 7> MENU_BITS{{2, 4, 9, 11, 13, 15, 23}};
  std::array<bool, 7> menu_last_states_{};
  std::array<bool, 7> menu_state_valid_{};
  std::array<uint8_t, 7> menu_toggle_counts_{};
};

class PetwalkEsphome : public Component {
 public:
  void set_data_pin(InternalGPIOPin *pin) { this->data_pin_ = pin; }
  void set_clock_pin(InternalGPIOPin *pin) { this->clock_pin_ = pin; }
  void set_latch_pin(InternalGPIOPin *pin) { this->latch_pin_ = pin; }
  void set_frame_bits(uint8_t frame_bits) { this->frame_bits_ = frame_bits; }
  void set_clock_edge(ClockEdge edge) { this->clock_edge_ = edge; }
  void set_latch_edge(LatchEdge edge) { this->latch_edge_ = edge; }
  void set_debug_frames(bool debug_frames) { this->debug_frames_ = debug_frames; }
  void register_binary_sensor(PetwalkBitBinarySensor *sensor) { this->binary_sensors_.push_back(sensor); }
  void register_switch(PetwalkBitSwitch *petwalk_switch) { this->switches_.push_back(petwalk_switch); }
  void register_text_sensor(PetwalkDisplayTextSensor *sensor) { this->text_sensors_.push_back(sensor); }
  void register_clock_sync_button(PetwalkClockSyncButton *button) { this->clock_sync_buttons_.push_back(button); }

  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }

 protected:
  static void IRAM_ATTR clock_isr(PetwalkEsphome *arg);
  static void IRAM_ATTR latch_isr(PetwalkEsphome *arg);
  void IRAM_ATTR handle_clock_isr_();
  void IRAM_ATTR handle_latch_isr_();
  void process_frame_(uint8_t buffer_index);

  InternalGPIOPin *data_pin_{nullptr};
  InternalGPIOPin *clock_pin_{nullptr};
  InternalGPIOPin *latch_pin_{nullptr};
  ISRInternalGPIOPin data_isr_pin_{};

  uint8_t frame_bits_{56};
  ClockEdge clock_edge_{CLOCK_EDGE_FALLING};
  LatchEdge latch_edge_{LATCH_EDGE_RISING};
  bool debug_frames_{false};

  volatile uint8_t frames_[2][PETWALK_MAX_FRAME_BITS]{};
  volatile uint8_t write_buffer_{0};
  volatile uint8_t bit_index_{0};
  volatile bool receiving_{false};
  volatile uint8_t completed_buffer_{0};
  volatile uint32_t completed_sequence_{0};
  uint32_t processed_sequence_{0};

  std::vector<PetwalkBitBinarySensor *> binary_sensors_;
  std::vector<PetwalkBitSwitch *> switches_;
  std::vector<PetwalkDisplayTextSensor *> text_sensors_;
  std::vector<PetwalkClockSyncButton *> clock_sync_buttons_;
};

}  // namespace petwalk_esphome
}  // namespace esphome
