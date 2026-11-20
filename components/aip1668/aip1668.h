#pragma once

#include <vector>

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"

#ifdef USE_OUTPUT
#include "esphome/components/output/binary_output.h"
#endif
#ifdef USE_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif

namespace esphome {
namespace aip1668 {

// Treiber für AiP1668 / TM1668 (LED-Treiber mit Tastenabfrage, 3-Draht-Bus).
// DIO wird beim Schreiben push-pull getrieben und nur zum Tastenlesen auf
// Eingang geschaltet. Mit Open-Drain und den internen Pull-ups steigt die
// Leitung zu langsam, der Chip liest dann nur Nullen.
class AIP1668Component : public PollingComponent {
 public:
  static constexpr uint8_t RAM_SIZE = 14;
  static constexpr uint8_t KEY_BYTES = 5;

  void set_clk_pin(InternalGPIOPin *pin) { this->clk_pin_ = pin; }
  void set_dio_pin(InternalGPIOPin *pin) { this->dio_pin_ = pin; }
  void set_stb_pin(InternalGPIOPin *pin) { this->stb_pin_ = pin; }
  void set_half_period(uint32_t us) { this->half_period_ = us; }
  void set_refresh_interval(uint32_t ms) { this->refresh_interval_ = ms; }

  // Display-Modus 0 bis 3 (4x13, 5x12, 6x11, 7x10)
  void set_display_mode(uint8_t mode) {
    this->display_mode_ = mode & 0x03;
    this->initialized_ = false;
  }
  // Helligkeit 0 bis 7 (1/16, 2/16, 4/16, 10/16, 11/16, 12/16, 13/16, 14/16)
  void set_brightness(uint8_t brightness) {
    this->brightness_ = brightness & 0x07;
    this->ctrl_dirty_ = true;
  }
  uint8_t get_brightness() const { return this->brightness_; }
  void set_display_on(bool on) {
    this->display_on_ = on;
    this->ctrl_dirty_ = true;
  }
  bool is_display_on() const { return this->display_on_; }

  // Display-RAM direkt bearbeiten
  void set_bit(uint8_t address, uint8_t bit, bool state);
  bool get_bit(uint8_t address, uint8_t bit) const;
  void set_byte(uint8_t address, uint8_t value);
  uint8_t get_byte(uint8_t address) const { return address < RAM_SIZE ? this->ram_[address] : 0; }
  void clear();

  // Zuletzt gelesene Tastenbytes
  uint8_t get_key_byte(uint8_t index) const { return index < KEY_BYTES ? this->keys_[index] : 0; }

#ifdef USE_BINARY_SENSOR
  void register_key(binary_sensor::BinarySensor *sensor, uint8_t key_byte, uint8_t bit) {
    this->key_sensors_.push_back({sensor, key_byte, bit});
  }
#endif

  void setup() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }

 protected:
  void dio_output_();
  void dio_input_();
  void stb_(bool level);
  void send_byte_(uint8_t data);
  uint8_t receive_byte_();
  void command_(uint8_t cmd);
  void write_ram_(const uint8_t *data);
  void read_keys_();

  InternalGPIOPin *clk_pin_{nullptr};
  InternalGPIOPin *dio_pin_{nullptr};
  InternalGPIOPin *stb_pin_{nullptr};
  uint32_t half_period_{20};
  uint32_t refresh_interval_{1000};
  uint32_t last_refresh_{0};

  uint8_t display_mode_{2};
  uint8_t brightness_{3};
  bool display_on_{true};

  uint8_t ram_[RAM_SIZE]{};
  uint8_t keys_[KEY_BYTES]{};
  bool initialized_{false};
  bool ram_dirty_{true};
  bool ctrl_dirty_{true};

#ifdef USE_BINARY_SENSOR
  struct KeySensor {
    binary_sensor::BinarySensor *sensor;
    uint8_t key_byte;
    uint8_t bit;
  };
  std::vector<KeySensor> key_sensors_;
#endif
};

#ifdef USE_OUTPUT
// Ein einzelnes Bit im Display-RAM als binärer Ausgang (eine LED)
class AIP1668Output : public output::BinaryOutput, public Parented<AIP1668Component> {
 public:
  void set_position(uint8_t address, uint8_t bit) {
    this->address_ = address;
    this->bit_ = bit;
  }

 protected:
  void write_state(bool state) override { this->parent_->set_bit(this->address_, this->bit_, state); }

  uint8_t address_{0};
  uint8_t bit_{0};
};
#endif

}  // namespace aip1668
}  // namespace esphome
