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

#include <array>

#include <boost/ut.hpp>

import hal;
import hal.usb;
import test_util;
import async_context;

namespace {

// Bundles a mock control endpoint + mock interface + enumerator wired
// together, matching the construction pattern real applications use. Wraps
// the stack-local mocks with `unsafe_assume_static_tag` (they outlive the
// enumerator within each test case, so this is safe here).
//
// Each rig owns its own `async::context`: `run()` never completes on its
// own (it loops forever), so its future is always destroyed mid-flight at
// the end of each test, which cancels rather than naturally finishes the
// coroutine. Reusing one shared context across many such not-quite-finished
// runs isn't a scenario the scheduler is meant to support, so each test
// gets a fresh context instead.
struct test_rig
{
  explicit test_rig(hal::u8 p_retry_max = 3)
    : info{ .manufacturer = u"Acme",
            .product = u"Widget",
            .serial_number = u"0001",
            .vendor_id = 0x1234,
            .product_id = 0x5678,
            .retry_max = p_retry_max,
            .remote_wakeup = true }
  {
  }

  async::inplace_context<2048> ctx{};
  hal::usb::test::mock_control_endpoint ctrl_ep{};
  hal::usb::test::mock_interface iface{};
  hal::ptr<hal::usb::control_endpoint> ctrl_ep_ptr{ mem::unsafe_assume_static_tag{},
                                                     ctrl_ep };
  std::array<hal::ptr<hal::usb::interface>, 1> ifaces{
    hal::ptr<hal::usb::interface>(mem::unsafe_assume_static_tag{}, iface)
  };
  hal::usb::info info;
  hal::usb::enumerator en{ ctrl_ep_ptr, info, ifaces };
};

hal::usb::setup_packet make_standard_device_request(
  bool p_device_to_host,
  hal::usb::standard_request_types p_request,
  hal::u16 p_value,
  hal::u16 p_length = 0)
{
  return hal::usb::setup_packet({
    .device_to_host = p_device_to_host,
    .type = hal::usb::setup_packet::request_type::standard,
    .recipient = hal::usb::setup_packet::request_recipient::device,
    .request = static_cast<hal::u8>(p_request),
    .value = p_value,
    .index = 0,
    .length = p_length,
  });
}

void startup_test()
{
  using namespace boost::ut;

  "[success] run() connects and resets interfaces on start"_test = []() {
    // Setup
    test_rig rig;

    // Exercise
    auto future = rig.en.run(rig.ctx);
    hal::usb::test::pump(future, 20);

    // Verify
    expect(that % 0 < rig.ctrl_ep.m_connect_calls);
    expect(rig.ctrl_ep.m_connected);
    expect(that % 0 < rig.iface.m_host_event_count);
    expect(hal::usb::host_event::reset == rig.iface.m_host_events[0]);
    expect(not rig.en.is_enumerated());
  };

  "[success] a real bus_event::reset re-connects and re-notifies"_test =
    []() {
      // Setup
      test_rig rig;
      auto future = rig.en.run(rig.ctx);
      hal::usb::test::pump(future, 20);
      auto const connects_before = rig.ctrl_ep.m_connect_calls;

      // Exercise
      rig.ctrl_ep.queue_event(hal::usb::bus_event::reset);
      hal::usb::test::pump(future, 20);

      // Verify
      expect(that % connects_before < rig.ctrl_ep.m_connect_calls);
    };
}

void get_descriptor_test()
{
  using namespace boost::ut;

  "[success] GET_DESCRIPTOR(device) responds with a device descriptor"_test =
    []() {
      // Setup
      test_rig rig;
      auto future = rig.en.run(rig.ctx);
      hal::usb::test::pump(future, 20);
      rig.ctrl_ep.reset_write_log();

      auto const request = make_standard_device_request(
        true,
        hal::usb::standard_request_types::get_descriptor,
        0x0100,  // descriptor_type::device (1) << 8 | index 0
        64);
      rig.ctrl_ep.set_read_data(request.raw_request_bytes);
      rig.ctrl_ep.queue_event(hal::usb::bus_event::setup_packet);

      // Exercise
      hal::usb::test::pump(future, 20);

      // Verify
      auto const response = rig.ctrl_ep.written();
      expect(that % 18 == response.size());
      expect(hal::byte{ 18 } == response[0]);  // bLength
      expect(hal::byte{ 1 } == response[1]);   // bDescriptorType == device
      expect(hal::byte{ 0x34 } == response[8]);   // idVendor low byte
      expect(hal::byte{ 0x12 } == response[9]);   // idVendor high byte
      expect(rig.ctrl_ep.m_last_write_was_zlp);
    };
}

void set_address_test()
{
  using namespace boost::ut;

  "[success] SET_ADDRESS records the assigned address"_test = []() {
    // Setup
    test_rig rig;
    auto future = rig.en.run(rig.ctx);
    hal::usb::test::pump(future, 20);

    auto const request = make_standard_device_request(
      false, hal::usb::standard_request_types::set_address, 42);
    rig.ctrl_ep.set_read_data(request.raw_request_bytes);
    rig.ctrl_ep.queue_event(hal::usb::bus_event::setup_packet);

    // Exercise
    hal::usb::test::pump(future, 20);

    // Verify
    expect(rig.ctrl_ep.m_address_set);
    expect(that % 42 == rig.ctrl_ep.m_address);
  };
}

void set_configuration_test()
{
  using namespace boost::ut;

  "[success] SET_CONFIGURATION marks the device enumerated"_test = []() {
    // Setup
    test_rig rig;
    auto future = rig.en.run(rig.ctx);
    hal::usb::test::pump(future, 20);

    auto const request = make_standard_device_request(
      false, hal::usb::standard_request_types::set_configuration, 1);
    rig.ctrl_ep.set_read_data(request.raw_request_bytes);
    rig.ctrl_ep.queue_event(hal::usb::bus_event::setup_packet);

    // Exercise
    hal::usb::test::pump(future, 20);

    // Verify
    expect(rig.en.is_enumerated());
    bool saw_enumerated = false;
    for (hal::usize i = 0; i < rig.iface.m_host_event_count; ++i) {
      if (rig.iface.m_host_events[i] == hal::usb::host_event::enumerated) {
        saw_enumerated = true;
      }
    }
    expect(saw_enumerated);
  };
}

void get_status_test()
{
  using namespace boost::ut;

  "[success] GET_STATUS reports the remote-wakeup-granted bit"_test = []() {
    // Setup
    test_rig rig;
    rig.ctrl_ep.m_remote_wakeup_enabled = true;
    auto future = rig.en.run(rig.ctx);
    hal::usb::test::pump(future, 20);
    rig.ctrl_ep.reset_write_log();

    auto const request = make_standard_device_request(
      true, hal::usb::standard_request_types::get_status, 0, 2);
    rig.ctrl_ep.set_read_data(request.raw_request_bytes);
    rig.ctrl_ep.queue_event(hal::usb::bus_event::setup_packet);

    // Exercise
    hal::usb::test::pump(future, 20);

    // Verify
    auto const response = rig.ctrl_ep.written();
    expect(that % 2 == response.size());
    // bit0 == self_powered (false), bit1 == remote wakeup granted (true)
    expect(hal::byte{ 0b10 } == response[0]);
  };
}

void malformed_request_test()
{
  using namespace boost::ut;

  "[failure] a malformed setup packet stalls and increments the retry "
  "counter"_test = []() {
    // Setup
    test_rig rig;
    auto future = rig.en.run(rig.ctx);
    hal::usb::test::pump(future, 20);
    auto const stalls_before = rig.ctrl_ep.m_stall_calls;

    // Exercise: no read data armed, so the "read" of the setup packet comes
    // back with 0 bytes instead of the required 8.
    rig.ctrl_ep.queue_event(hal::usb::bus_event::setup_packet);
    hal::usb::test::pump(future, 20);

    // Verify
    expect(that % stalls_before < rig.ctrl_ep.m_stall_calls);
  };

  "[failure] retry_max consecutive errors throws hal::io_error"_test = []() {
    // Setup
    test_rig rig(2);
    auto future = rig.en.run(rig.ctx);
    hal::usb::test::pump(future, 20);

    // Exercise + Verify
    expect(throws<hal::io_error>([&] {
      for (int i = 0; i < 10 and not future.done(); ++i) {
        rig.ctrl_ep.queue_event(hal::usb::bus_event::setup_packet);
        hal::usb::test::pump(future, 20);
      }
    }));
  };
}

}  // namespace

int main()
{
  startup_test();
  get_descriptor_test();
  set_address_test();
  set_configuration_test();
  get_status_test();
  malformed_request_test();
}
