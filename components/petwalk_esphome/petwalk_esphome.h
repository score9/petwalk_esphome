#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "esphome/components/binary_sensor/binary_sensor.h"
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
  void publish_from_frame(const uint8_t *frame, uint8_t frame_bits);

 protected:
  uint8_t bit_{1};  // 1-based: bit 1 is the first sampled clock pulse.
  bool active_low_{true};
  bool has_state_{false};
  bool last_state_{false};
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

  // Double buffering: the ISR writes one buffer while loop() reads the other.
  volatile uint8_t frames_[2][PETWALK_MAX_FRAME_BITS]{};
  volatile uint8_t write_buffer_{0};
  volatile uint8_t bit_index_{0};
  volatile bool receiving_{false};
  volatile uint8_t completed_buffer_{0};
  volatile uint32_t completed_sequence_{0};
  uint32_t processed_sequence_{0};

  std::vector<PetwalkBitBinarySensor *> binary_sensors_;
};

}  // namespace petwalk_esphome
}  // namespace esphome
