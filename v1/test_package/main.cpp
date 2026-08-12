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
#include <coroutine>
#include <print>
#include <string_view>

import hal;
import hal.usb;

namespace {
template<typename T>
void pump(async::future<T>& p_future, hal::usize p_max_steps = 50)
{
  for (hal::usize i = 0; i < p_max_steps and not p_future.done(); ++i) {
    p_future.resume();
  }
}

class stub_control_endpoint : public hal::usb::control_endpoint
{
public:
  ~stub_control_endpoint() override = default;

  hal::usize m_connect_calls = 0;

private:
  [[nodiscard]] hal::usb::endpoint_info driver_info() const override
  {
    return { .size = 8, .number = 0, .stalled = false };
  }

  async::future<void> driver_stall(async::context&, bool) override
  {
    return {};
  }

  async::future<void> driver_reset(async::context&) override
  {
    return {};
  }

  async::future<void> driver_connect(async::context&, bool) override
  {
    ++m_connect_calls;
    return {};
  }

  async::future<void> driver_set_address(async::context&, hal::u8) override
  {
    return {};
  }

  async::future<void> driver_write(
    async::context&,
    mem::scatter_span<hal::byte const>) override
  {
    return {};
  }

  async::future<hal::usize> driver_read(
    async::context&,
    mem::scatter_span<hal::byte>) override
  {
    co_return 0;
  }

  async::future<hal::usb::bus_event> driver_on_bus_event(
    async::context& p_context) override
  {
    // A real driver suspends here until hardware reports a bus event. This
    // stub does the same (via block_by_signal) since a bare `co_return`
    // here would let the enumerator's `while (true)` event loop spin
    // forever synchronously inside a single resume() call, instead of
    // yielding back to the `pump()` loop below after each step.
    co_await p_context.block_by_signal();
    co_return hal::usb::bus_event::data_packet;
  }

  async::future<void> driver_remote_wakeup_enable(async::context&,
                                                    bool) override
  {
    return {};
  }

  async::future<bool> driver_remote_wakeup_granted(async::context&) override
  {
    co_return false;
  }

  async::future<void> driver_acknowledge_sleep(async::context&, bool) override
  {
    return {};
  }

  [[nodiscard]] hal::usb::lpm_support driver_supports_lpm() override
  {
    return {};
  }
};

class stub_interface : public hal::usb::interface
{
public:
  ~stub_interface() override = default;

private:
  async::future<descriptor_count> driver_write_descriptors(
    async::context&,
    descriptor_start,
    hal::usb::endpoint_io&) override
  {
    co_return descriptor_count{ .interface = 0, .string = 0 };
  }

  async::future<bool> driver_write_string_descriptor(
    async::context&,
    hal::u8,
    hal::usb::endpoint_io&) override
  {
    co_return false;
  }

  async::future<bool> driver_handle_request(
    async::context&,
    hal::usb::setup_packet const&,
    hal::usb::endpoint_io&) override
  {
    co_return false;
  }

  async::future<void> driver_handle_host_event(
    async::context&,
    hal::usb::host_event) override
  {
    return {};
  }
};
}  // namespace

int main()
{
  using namespace std::string_view_literals;

  async::inplace_context<1024> ctx;

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

  auto future = enumerator.run(ctx);
  pump(future);

  std::println("libhal-usb test_package: enumerator constructed and run.");
  std::println("connect() calls observed: {}", control_endpoint.m_connect_calls);
  std::println("is_enumerated(): {}", enumerator.is_enumerated());

  return 0;
}
