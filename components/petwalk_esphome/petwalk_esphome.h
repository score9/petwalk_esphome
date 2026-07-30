#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/remote_base/rc5_protocol.h"
#include "esphome/components/remote_base/remote_base.h"
#include "esphome/components/switch/switch.h"
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
};

}  // namespace petwalk_esphome
}  // namespace esphome
