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
#include <memory_resource>
#include <span>

#include <boost/ut.hpp>

import hal;
import hal.usb;
import test_util;

namespace {

struct test_bulk_in_endpoint : public hal::usb::bulk_in_endpoint
{
  hal::usb::endpoint_info m_info{ .size = 64, .number = 0x81, .stalled = false };

private:
  [[nodiscard]] hal::usb::endpoint_info driver_info() const override
  {
    return m_info;
  }

  async::future<void> driver_stall(async::context&, bool) override
  {
    return {};
  }

  async::future<void> driver_reset(async::context&) override
  {
    return {};
  }

  async::future<void> driver_write(async::context&,
                                    mem::scatter_span<hal::byte const>) override
  {
    return {};
  }
};

void interface_descriptor_test()
{
  using namespace boost::ut;

  "[success] generate_interface_descriptor"_test = []() {
    // Setup
    hal::usb::interface_descriptor_info const info{
      .interface_number = 2,
      .alternate_setting = 0,
      .num_endpoints = 3,
      .interface_class = hal::usb::class_code::cdc_data,
      .interface_subclass = 0x01,
      .interface_protocol = 0x02,
      .interface_string_index = 5,
    };

    // Exercise
    auto const descriptor = hal::usb::generate_interface_descriptor(info);

    // Verify
    expect(that % hal::usb::constants::interface_descriptor_size ==
           descriptor.size());
    expect(that % descriptor.size() == descriptor[0]);
    expect(that % 0x04 == descriptor[1]);  // b_descriptor_type == interface
    expect(that % 2 == descriptor[2]);     // b_interface_number
    expect(that % 0 == descriptor[3]);     // b_alternate_setting
    expect(that % 3 == descriptor[4]);     // b_num_endpoints
    expect(that % 0x0A == descriptor[5]);  // b_interface_class == cdc_data
    expect(that % 0x01 == descriptor[6]);  // b_interface_sub_class
    expect(that % 0x02 == descriptor[7]);  // b_interface_protocol
    expect(that % 5 == descriptor[8]);     // i_interface
  };
}

void endpoint_descriptor_test()
{
  using namespace boost::ut;

  "[success] generate_endpoint_descriptor"_test = []() {
    // Setup
    test_bulk_in_endpoint endpoint;

    // Exercise
    auto const descriptor =
      hal::usb::generate_endpoint_descriptor(endpoint, 10);

    // Verify
    expect(that % hal::usb::constants::endpoint_descriptor_size ==
           descriptor.size());
    expect(that % descriptor.size() == descriptor[0]);
    expect(that % 0x05 == descriptor[1]);   // b_descriptor_type == endpoint
    expect(that % 0x81 == descriptor[2]);   // b_endpoint_address
    expect(that % 0x02 == descriptor[3]);   // bm_attributes == bulk
    expect(that % 64 == descriptor[4]);     // w_max_packet_size low byte
    expect(that % 0 == descriptor[5]);      // w_max_packet_size high byte
    expect(that % 10 == descriptor[6]);     // b_interval
  };
}

void device_test()
{
  using namespace boost::ut;

  "[success] device"_test = []() {
    // Setup
    using namespace std::string_view_literals;

    // Exercise
    hal::usb::device const dev({
      .bcd_usb = 0x0200,
      .device_class = hal::usb::class_code::use_interface_descriptor,
      .device_subclass = 0,
      .device_protocol = 0,
      .id_vendor = 0x1234,
      .id_product = 0x5678,
      .bcd_device = 0x0100,
      .p_manufacturer = u"Acme"sv,
      .p_product = u"Widget"sv,
      .p_serial_number_str = u"0001"sv,
    });

    // Verify
    expect(that % 0x0200 == dev.bcd_usb());
    expect(that % 0x1234 == dev.id_vendor());
    expect(that % 0x5678 == dev.id_product());
    expect(that % 0x0100 == dev.bcd_device());
    std::span<hal::u8 const> const dev_bytes = dev;
    expect(that % 16 == dev_bytes.size());
  };
}

void configuration_test()
{
  using namespace boost::ut;

  "[success] configuration"_test = []() {
    // Setup
    using namespace std::string_view_literals;
    std::array<std::byte, 512> buffer{};
    std::pmr::monotonic_buffer_resource resource(buffer.data(), buffer.size());
    hal::allocator const alloc(&resource);
    auto iface = hal::allocate<hal::usb::test::mock_interface>(alloc);

    // Exercise
    hal::usb::configuration config(
      {
        .name = u"Config 1"sv,
        .attributes = { /* self_powered */ true, /* remote_wakeup */ false },
        .max_power = 100,
        .allocator = alloc,
      },
      iface);

    // Verify
    expect(that % 1 == config.interfaces().size());
    expect(config.attributes().self_powered());
    expect(not config.attributes().remote_wakeup());
    expect(that % 100 == config.max_power());
  };
}

}  // namespace

int main()
{
  interface_descriptor_test();
  endpoint_descriptor_test();
  device_test();
  configuration_test();
}
