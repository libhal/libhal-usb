// Copyright 2026 Khalil Estell and the libhal contributors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <array>
#include <print>
#include <string_view>

import hal;
import hal.usb;

namespace {
class stub_control_endpoint : public hal::usb::control_endpoint
{
public:
  ~stub_control_endpoint() = default;

  hal::usize m_connect_calls = 0;
  mem::optional_ptr<hal::usb::bus_event_callback> m_bus_event_callback;

private:
  [[nodiscard]] hal::usb::endpoint_info driver_info() const override
  {
    return { .size = 8, .number = 0, .stalled = false };
  }

  void driver_stall(bool) override
  {
  }

  void driver_reset() override
  {
  }

  void driver_connect(bool) override
  {
    ++m_connect_calls;
  }

  void driver_set_address(hal::u8) override
  {
  }

  void driver_write(mem::scatter_span<hal::byte const>) override
  {
  }

  hal::usize driver_read(mem::scatter_span<hal::byte>) override
  {
    return 0;
  }

  void driver_on_bus_event(
    mem::optional_ptr<hal::usb::bus_event_callback> const& p_callback) override
  {
    // A real driver stores this and invokes it from its USB interrupt.
    m_bus_event_callback = p_callback;
  }

  void driver_remote_wakeup_enable(bool) override
  {
  }

  bool driver_remote_wakeup_granted() override
  {
    return false;
  }

  void driver_acknowledge_sleep(bool) override
  {
  }

  [[nodiscard]] hal::usb::lpm_support driver_supports_lpm() override
  {
  }
};

class stub_interface : public hal::usb::interface
{
public:
  ~stub_interface() = default;

private:
  descriptor_count driver_write_descriptors(descriptor_start,
                                            hal::usb::endpoint_io&) override
  {
    return descriptor_count{ .interface = 0, .string = 0 };
  }

  bool driver_write_string_descriptor(hal::u8, hal::usb::endpoint_io&) override
  {
    return false;
  }

  bool driver_handle_request(hal::usb::setup_packet const&,
                             hal::usb::endpoint_io&) override
  {
    return false;
  }

  void driver_handle_host_event(hal::usb::host_event) override
  {
  }
};
}  // namespace

int main()
{
  using namespace std::string_view_literals;

  stub_control_endpoint control_endpoint;
  stub_interface interface;

  hal::ptr<hal::usb::control_endpoint> const control_endpoint_ptr(
    mem::unsafe_assume_static_tag{}, control_endpoint);
  hal::ptr<hal::usb::interface> const interface_ptr(
    mem::unsafe_assume_static_tag{}, interface);

  hal::usb::info const device_info{
    .manufacturer = u"libhal",
    .product = u"libhal-usb test_package",
    .serial_number = u"0001",
    .vendor_id = 0x1209,
    .product_id = 0x0001,
  };

  hal::usb::inplace_enumerator enumerator(
    control_endpoint_ptr, device_info, interface_ptr);

  enumerator.poll();

  std::println("libhal-usb test_package: enumerator constructed and polled.");
  std::println("connect() calls observed: {}",
               control_endpoint.m_connect_calls);
  std::println("is_enumerated(): {}", enumerator.is_enumerated());

  return 0;
}
