#pragma once

#include <array>
#include <numeric>

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"

namespace esphome {
namespace helios_kwl_component {

class HeliosKwlComponent : public uart::UARTDevice, public PollingComponent {
 private:
  static constexpr uint8_t SYSTEM = 0x01;
  static constexpr uint8_t ADDRESS = 0x2F;
  static constexpr uint8_t MAINBOARD = 0x11;
  static constexpr uint8_t MAINBOARD_BROADCAST = 0x10;
  static constexpr uint8_t REMOTE_BROADCAST = 0x20;
  static constexpr uint8_t REMOTE_MIN = 0x21;
  static constexpr uint8_t REMOTE_MAX = 0x2F;
  static constexpr uint32_t REGISTER_CACHE_TTL_MS = 120000;
  static const int TEMPERATURE[];

 public:
  using Datagram = std::array<uint8_t, 6>;

  void setup() override;
  void update() override;
  void dump_config() override;

  void set_passive(bool passive) { m_passive = passive; }
  void set_repeat_final_checksum(bool repeat_final_checksum) { m_repeat_final_checksum = repeat_final_checksum; }
  void set_write_address(uint8_t write_address) { m_write_address = write_address; }
  void set_use_mainboard_write_checksum(bool use_mainboard_write_checksum) {
    m_use_mainboard_write_checksum = use_mainboard_write_checksum;
  }
  void set_write_bus_idle_ms(uint32_t write_bus_idle_ms) { m_write_bus_idle_ms = write_bus_idle_ms; }
  void set_write_frame_delay_ms(uint32_t write_frame_delay_ms) { m_write_frame_delay_ms = write_frame_delay_ms; }

  void set_fan_speed(float speed);
  bool set_fan_speed_level(uint8_t level);
  bool read_register(uint8_t address);
  bool write_register(uint8_t address, uint8_t value) { return set_value(address, value); }
  void set_state_flag(uint8_t bit, bool state);

  void set_fan_speed_sensor(sensor::Sensor* sensor) { m_fan_speed = sensor; }
  void set_temperature_outside_sensor(sensor::Sensor* sensor) { m_temperature_outside = sensor; }
  void set_temperature_exhaust_sensor(sensor::Sensor* sensor) { m_temperature_exhaust = sensor; }
  void set_temperature_inside_sensor(sensor::Sensor* sensor) { m_temperature_inside = sensor; }
  void set_temperature_incoming_sensor(sensor::Sensor* sensor) { m_temperature_incoming = sensor; }

  void set_power_state_sensor(binary_sensor::BinarySensor* sensor) { m_power_state = sensor; }
  void set_bypass_state_sensor(binary_sensor::BinarySensor* sensor) { m_bypass_state = sensor; }
  void set_heating_indicator_sensor(binary_sensor::BinarySensor* sensor) { m_heating_indicator = sensor; }
  void set_fault_indicator_sensor(binary_sensor::BinarySensor* sensor) { m_fault_indicator = sensor; }
  void set_service_reminder_sensor(binary_sensor::BinarySensor* sensor) { m_service_reminder = sensor; }

  void set_winter_mode_switch(switch_::Switch* switch_) { m_winter_mode_switch = switch_; }

 private:
  void poll_temperature_outside();
  void poll_temperature_exhaust();
  void poll_temperature_inside();
  void poll_temperature_incoming();
  void poll_fan_speed();
  void poll_states();

  optional<uint8_t> poll_register(uint8_t address, bool allow_shared_cache = true);

  bool set_value(uint8_t address, uint8_t value);

  bool read_datagram(Datagram& datagram, uint32_t timeout_ms);
  bool wait_for_write_confirmation(uint8_t address, uint8_t value, uint8_t acknowledge, uint32_t timeout_ms);
  bool flush_read_buffer(uint32_t idle_ms = 10, uint32_t timeout_ms = 250);
  bool cache_register_value(const Datagram& datagram);
  optional<uint8_t> cached_register_value(uint8_t address) const;

  template <typename Iterator>
  static bool check_crc(const Iterator begin, const Iterator end) {
    const auto crc = checksum(begin, std::prev(end));
    return *std::prev(end) == crc;
  }

  template <typename Iterator>
  static uint8_t checksum(const Iterator begin, const Iterator end) {
    return std::accumulate(begin, end, 0);
  }

  static uint8_t count_ones(uint8_t byte);
  static uint8_t fan_speed_byte(uint8_t level);
  static bool is_remote_recipient(uint8_t recipient) {
    return recipient == REMOTE_BROADCAST || (recipient >= REMOTE_MIN && recipient <= REMOTE_MAX);
  }

 private:
  sensor::Sensor* m_fan_speed{nullptr};
  sensor::Sensor* m_temperature_outside{nullptr};
  sensor::Sensor* m_temperature_exhaust{nullptr};
  sensor::Sensor* m_temperature_inside{nullptr};
  sensor::Sensor* m_temperature_incoming{nullptr};

  binary_sensor::BinarySensor* m_power_state{nullptr};
  binary_sensor::BinarySensor* m_bypass_state{nullptr};
  binary_sensor::BinarySensor* m_heating_indicator{nullptr};
  binary_sensor::BinarySensor* m_fault_indicator{nullptr};
  binary_sensor::BinarySensor* m_service_reminder{nullptr};

  switch_::Switch* m_winter_mode_switch{nullptr};

  using PollerFunction = std::function<void()>;
  std::vector<PollerFunction> m_pollers{};
  std::vector<PollerFunction>::const_iterator m_current_poller{};
  std::array<uint8_t, 256> m_register_cache{};
  std::array<uint32_t, 256> m_register_cache_time{};
  uint32_t m_last_register_frame_time{0};
  bool m_passive{false};
  bool m_repeat_final_checksum{true};
  uint8_t m_write_address{ADDRESS};
  bool m_use_mainboard_write_checksum{true};
  uint32_t m_write_bus_idle_ms{30};
  uint32_t m_write_frame_delay_ms{2};
};

template<typename... Ts> class SetFanSpeedAction : public Action<Ts...>, public Parented<HeliosKwlComponent> {
 public:
  TEMPLATABLE_VALUE(uint8_t, level)

  void play(const Ts&... x) override { this->parent_->set_fan_speed_level(this->level_.value(x...)); }
};

template<typename... Ts> class ReadRegisterAction : public Action<Ts...>, public Parented<HeliosKwlComponent> {
 public:
  TEMPLATABLE_VALUE(uint8_t, address)

  void play(const Ts&... x) override { this->parent_->read_register(this->address_.value(x...)); }
};

template<typename... Ts> class WriteRegisterAction : public Action<Ts...>, public Parented<HeliosKwlComponent> {
 public:
  TEMPLATABLE_VALUE(uint8_t, address)
  TEMPLATABLE_VALUE(uint8_t, value)

  void play(const Ts&... x) override { this->parent_->write_register(this->address_.value(x...), this->value_.value(x...)); }
};

}  // namespace helios_kwl_component
}  // namespace esphome
