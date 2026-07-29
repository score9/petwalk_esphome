#include "petwalk_esphome.h"

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

  // A changed raw value starts a new candidate period. If it changes back before
  // the configured time expires, the short pulse is discarded automatically.
  if (!this->candidate_valid_ || state != this->candidate_state_) {
    this->candidate_valid_ = true;
    this->candidate_state_ = state;
    this->candidate_since_ms_ = now;
  }

  // No need to republish the state that Home Assistant already knows.
  if (this->published_valid_ && this->candidate_state_ == this->published_state_)
    return;

  const uint32_t required_ms = this->candidate_state_ ? this->minimum_on_time_ms_ : this->minimum_off_time_ms_;

  // Unsigned subtraction intentionally handles millis() rollover.
  if (static_cast<uint32_t>(now - this->candidate_since_ms_) < required_ms)
    return;

  this->published_state_ = this->candidate_state_;
  this->published_valid_ = true;
  this->publish_state(this->published_state_);
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

  // Nothing to process until the first complete frame arrives.
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
}

void IRAM_ATTR PetwalkEsphome::clock_isr(PetwalkEsphome *arg) { arg->handle_clock_isr_(); }

void IRAM_ATTR PetwalkEsphome::latch_isr(PetwalkEsphome *arg) { arg->handle_latch_isr_(); }

void IRAM_ATTR PetwalkEsphome::handle_latch_isr_() {
  // The selected LATCH edge marks the start of a new 56-bit frame.
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

  // If no newer frame arrived while processing, sleep until the next ISR wakeup.
  if (this->completed_sequence_ == this->processed_sequence_)
    this->disable_loop();
}

void PetwalkEsphome::process_frame_(uint8_t buffer_index) {
  std::array<uint8_t, PETWALK_MAX_FRAME_BITS> frame{};
  for (uint8_t i = 0; i < this->frame_bits_; i++)
    frame[i] = this->frames_[buffer_index][i];

  for (auto *sensor : this->binary_sensors_)
    sensor->publish_from_frame(frame.data(), this->frame_bits_);

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
