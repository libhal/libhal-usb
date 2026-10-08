// Copyright 2026 Khalil Estell and the libhal contributors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

module;

#include <array>
#include <span>

export module test_util;

import hal;
import hal.util;
import hal.usb;
import scatter_span;

namespace hal::usb::inline v1::test {

/**
 * @brief Mock USB control endpoint for testing enumerator/endpoint logic.
 *
 * Records every driver call it receives and lets a test script canned
 * responses (bytes to hand back from `read()`). `queue_event()` simulates the
 * driver's interrupt by invoking the installed bus event callback.
 */
export class mock_control_endpoint : public hal::usb::control_endpoint
{
public:
  ~mock_control_endpoint() = default;

  void set_read_data(std::span<hal::byte const> p_data)
  {
    m_read_available = p_data.size();
    for (hal::usize i = 0; i < p_data.size() and i < m_read_buffer.size();
         ++i) {
      m_read_buffer[i] = p_data[i];
    }
  }

  void queue_event(hal::usb::bus_event p_event)
  {
    if (m_bus_event_callback.has_value()) {
      m_bus_event_callback.value()->callback(p_event);
    }
  }

  [[nodiscard]] bool has_bus_event_callback() const
  {
    return m_bus_event_callback.has_value();
  }

  void reset_write_log()
  {
    m_write_length = 0;
    m_write_call_count = 0;
    m_last_write_was_zlp = false;
  }

  [[nodiscard]] std::span<hal::byte const> written() const
  {
    return std::span(m_write_log).first(m_write_length);
  }

  hal::usize m_connect_calls = 0;
  bool m_connected = false;
  bool m_address_set = false;
  hal::u8 m_address = 0;
  hal::usize m_stall_calls = 0;
  hal::usize m_remote_wakeup_enable_calls = 0;
  bool m_remote_wakeup_enabled = false;
  hal::usb::lpm_support m_lpm{};
  hal::usize m_bus_event_calls = 0;
  hal::usize m_write_call_count = 0;
  bool m_last_write_was_zlp = false;

private:
  [[nodiscard]] hal::usb::endpoint_info driver_info() const override
  {
    return { .size = 8, .number = 0, .stalled = false };
  }

  void driver_stall(bool) override
  {
    ++m_stall_calls;
  }

  void driver_reset() override
  {
  }

  void driver_connect(bool p_should_connect) override
  {
    ++m_connect_calls;
    m_connected = p_should_connect;
  }

  void driver_set_address(hal::u8 p_address) override
  {
    m_address_set = true;
    m_address = p_address;
  }

  void driver_write(mem::scatter_span<hal::byte const> p_data) override
  {
    ++m_write_call_count;
    m_last_write_was_zlp = (p_data.length() == 0);
    for (auto chunk : p_data) {
      for (auto b : chunk) {
        if (m_write_length < m_write_log.size()) {
          m_write_log[m_write_length++] = b;
        }
      }
    }
  }

  hal::usize driver_read(mem::scatter_span<hal::byte> p_buffer) override
  {
    hal::usize written = 0;
    for (auto chunk : p_buffer) {
      for (auto& b : chunk) {
        if (written >= m_read_available) {
          return written;
        }
        b = m_read_buffer[written++];
      }
    }
    m_read_available = 0;
    return written;
  }

  void driver_on_bus_event(
    mem::optional_ptr<hal::usb::bus_event_callback> const& p_callback) override
  {
    ++m_bus_event_calls;
    m_bus_event_callback = p_callback;
  }

  void driver_remote_wakeup_enable(bool p_enabled) override
  {
    ++m_remote_wakeup_enable_calls;
    m_remote_wakeup_enabled = p_enabled;
  }

  bool driver_remote_wakeup_granted() override
  {
    return m_remote_wakeup_enabled;
  }

  void driver_acknowledge_sleep(bool) override
  {
  }

  [[nodiscard]] hal::usb::lpm_support driver_supports_lpm() override
  {
    return m_lpm;
  }

  std::array<hal::byte, 8> m_read_buffer{};
  hal::usize m_read_available = 0;
  std::array<hal::byte, 256> m_write_log{};
  hal::usize m_write_length = 0;
  mem::optional_ptr<hal::usb::bus_event_callback> m_bus_event_callback;
};

/**
 * @brief Mock USB interface for testing the enumerator's dispatch logic.
 *
 * Records host events forwarded to it and lets a test script the responses
 * `handle_request()`/`write_string_descriptor()`/`write_descriptors()`
 * should return.
 */
export class mock_interface : public hal::usb::interface
{
public:
  ~mock_interface() = default;

  bool m_handle_request_result = false;
  bool m_write_string_result = false;
  descriptor_count m_descriptor_count{ .interface = 1, .string = 0 };
  hal::usize m_write_descriptors_calls = 0;
  hal::usize m_handle_request_calls = 0;
  std::array<hal::usb::host_event, 16> m_host_events{};
  hal::usize m_host_event_count = 0;

private:
  descriptor_count driver_write_descriptors(descriptor_start,
                                            hal::usb::endpoint_io&) override
  {
    ++m_write_descriptors_calls;
    return m_descriptor_count;
  }

  bool driver_write_string_descriptor(hal::u8, hal::usb::endpoint_io&) override
  {
    return m_write_string_result;
  }

  bool driver_handle_request(hal::usb::setup_packet const&,
                             hal::usb::endpoint_io&) override
  {
    ++m_handle_request_calls;
    return m_handle_request_result;
  }

  void driver_handle_host_event(hal::usb::host_event p_event) override
  {
    if (m_host_event_count < m_host_events.size()) {
      m_host_events[m_host_event_count++] = p_event;
    }
  }
};

}  // namespace hal::usb::inline v1::test
