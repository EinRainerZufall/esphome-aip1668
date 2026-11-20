#include "aip1668.h"
#include "esphome/core/log.h"

namespace esphome {
namespace aip1668 {

static const char *const TAG = "aip1668";

static const uint8_t CMD_DATA_WRITE = 0x40;  // Daten schreiben, Auto-Increment
static const uint8_t CMD_DATA_READ = 0x42;   // Tasten lesen
static const uint8_t CMD_ADDRESS = 0xC0;     // Startadresse 00H
static const uint8_t CMD_DISPLAY_OFF = 0x80;
static const uint8_t CMD_DISPLAY_ON = 0x88;  // | Helligkeit 0..7

void AIP1668Component::setup() {
  this->clk_pin_->setup();
  this->clk_pin_->digital_write(true);
  this->clk_pin_->pin_mode(gpio::FLAG_OUTPUT);

  this->stb_pin_->setup();
  this->stb_pin_->digital_write(true);
  this->stb_pin_->pin_mode(gpio::FLAG_OUTPUT);

  this->dio_pin_->setup();
  this->dio_output_();

  this->initialized_ = false;
}

void AIP1668Component::dump_config() {
  static const char *const MODES[] = {"4x13", "5x12", "6x11", "7x10"};
  ESP_LOGCONFIG(TAG, "AiP1668:");
  LOG_PIN("  CLK Pin: ", this->clk_pin_);
  LOG_PIN("  DIO Pin: ", this->dio_pin_);
  LOG_PIN("  STB Pin: ", this->stb_pin_);
  ESP_LOGCONFIG(TAG, "  Display-Modus: %s (Grids x Segmente)", MODES[this->display_mode_]);
  ESP_LOGCONFIG(TAG, "  Helligkeit: %u", this->brightness_);
  ESP_LOGCONFIG(TAG, "  Takt-Halbperiode: %u us", (unsigned) this->half_period_);
  LOG_UPDATE_INTERVAL(this);
}

void AIP1668Component::set_bit(uint8_t address, uint8_t bit, bool state) {
  if (address >= RAM_SIZE || bit > 7)
    return;
  uint8_t old = this->ram_[address];
  if (state) {
    this->ram_[address] |= (1 << bit);
  } else {
    this->ram_[address] &= ~(1 << bit);
  }
  if (this->ram_[address] != old)
    this->ram_dirty_ = true;
}

bool AIP1668Component::get_bit(uint8_t address, uint8_t bit) const {
  if (address >= RAM_SIZE || bit > 7)
    return false;
  return (this->ram_[address] >> bit) & 1;
}

void AIP1668Component::set_byte(uint8_t address, uint8_t value) {
  if (address >= RAM_SIZE || this->ram_[address] == value)
    return;
  this->ram_[address] = value;
  this->ram_dirty_ = true;
}

void AIP1668Component::clear() {
  for (uint8_t i = 0; i < RAM_SIZE; i++)
    this->set_byte(i, 0);
}

void AIP1668Component::update() {
  const uint32_t now = millis();
  if (this->refresh_interval_ > 0 && now - this->last_refresh_ >= this->refresh_interval_) {
    // Regelmäßig alles neu schreiben, falls der Chip z. B. durch einen
    // Spannungseinbruch seinen Zustand verloren hat.
    this->last_refresh_ = now;
    this->ram_dirty_ = true;
    this->ctrl_dirty_ = true;
  }

  if (!this->initialized_) {
    const uint8_t zeros[RAM_SIZE] = {};
    this->command_(this->display_mode_);
    this->write_ram_(zeros);
    this->initialized_ = true;
    this->ram_dirty_ = true;
    this->ctrl_dirty_ = true;
    ESP_LOGD(TAG, "Initialisiert");
  }

  if (this->ram_dirty_) {
    this->ram_dirty_ = false;
    this->write_ram_(this->ram_);
  }

  if (this->ctrl_dirty_) {
    this->ctrl_dirty_ = false;
    this->command_(this->display_on_ ? (CMD_DISPLAY_ON | this->brightness_) : CMD_DISPLAY_OFF);
  }

  this->read_keys_();
}

void AIP1668Component::dio_output_() {
  this->dio_pin_->digital_write(true);
  this->dio_pin_->pin_mode(gpio::FLAG_OUTPUT);
}

void AIP1668Component::dio_input_() { this->dio_pin_->pin_mode(gpio::FLAG_INPUT | gpio::FLAG_PULLUP); }

void AIP1668Component::stb_(bool level) {
  this->stb_pin_->digital_write(level);
  delayMicroseconds(this->half_period_);
}

void AIP1668Component::send_byte_(uint8_t data) {
  for (uint8_t i = 0; i < 8; i++) {  // LSB zuerst
    this->clk_pin_->digital_write(false);
    this->dio_pin_->digital_write((data >> i) & 1);
    delayMicroseconds(this->half_period_);
    this->clk_pin_->digital_write(true);  // Chip übernimmt mit steigender Flanke
    delayMicroseconds(this->half_period_);
  }
}

uint8_t AIP1668Component::receive_byte_() {
  uint8_t data = 0;
  for (uint8_t i = 0; i < 8; i++) {
    this->clk_pin_->digital_write(false);  // Chip legt Bit nach fallender Flanke an
    delayMicroseconds(this->half_period_);
    if (this->dio_pin_->digital_read())
      data |= (1 << i);
    this->clk_pin_->digital_write(true);
    delayMicroseconds(this->half_period_);
  }
  return data;
}

void AIP1668Component::command_(uint8_t cmd) {
  this->stb_(false);
  this->send_byte_(cmd);
  this->stb_(true);
}

void AIP1668Component::write_ram_(const uint8_t *data) {
  this->command_(CMD_DATA_WRITE);
  this->stb_(false);
  this->send_byte_(CMD_ADDRESS);
  for (uint8_t i = 0; i < RAM_SIZE; i++)
    this->send_byte_(data[i]);
  this->stb_(true);
}

void AIP1668Component::read_keys_() {
  uint8_t data[KEY_BYTES];

  this->stb_(false);
  this->send_byte_(CMD_DATA_READ);
  this->dio_input_();  // nach dem 8. Takt freigeben, Chip treibt DIO
  delayMicroseconds(this->half_period_);
  for (uint8_t i = 0; i < KEY_BYTES; i++)
    data[i] = this->receive_byte_();
  this->stb_(true);
  this->dio_output_();

  bool changed = false;
  for (uint8_t i = 0; i < KEY_BYTES; i++) {
    if (data[i] != this->keys_[i]) {
      this->keys_[i] = data[i];
      changed = true;
    }
  }
  if (changed) {
    ESP_LOGV(TAG, "Tastenbytes: %02X %02X %02X %02X %02X", data[0], data[1], data[2], data[3], data[4]);
  }

#ifdef USE_BINARY_SENSOR
  for (auto &k : this->key_sensors_)
    k.sensor->publish_state((this->keys_[k.key_byte] >> k.bit) & 1);
#endif
}

}  // namespace aip1668
}  // namespace esphome
