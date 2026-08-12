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

#include <boost/ut.hpp>

import hal;
import hal.usb;

namespace {
void descriptor_size_test()
{
  using namespace boost::ut;

  "[success] descriptor sizes match the USB spec"_test = []() {
    // Setup / Exercise / Verify
    expect(that % 18 == hal::usb::constants::device_descriptor_size);
    expect(that % 9 == hal::usb::constants::configuration_descriptor_size);
    expect(that % 9 == hal::usb::constants::interface_descriptor_size);
    expect(that % 7 == hal::usb::constants::endpoint_descriptor_size);
    expect(that % 8 ==
           hal::usb::constants::interface_association_descriptor_size);
    expect(that % 8 == hal::usb::constants::standard_request_size);
  };
}

void enum_value_test()
{
  using namespace boost::ut;

  "[success] descriptor_type values match the USB spec"_test = []() {
    // Setup / Exercise / Verify
    expect(hal::byte{ 0x1 } ==
           static_cast<hal::byte>(hal::usb::descriptor_type::device));
    expect(hal::byte{ 0x2 } ==
           static_cast<hal::byte>(hal::usb::descriptor_type::configuration));
    expect(hal::byte{ 0x3 } ==
           static_cast<hal::byte>(hal::usb::descriptor_type::string));
    expect(hal::byte{ 0x4 } ==
           static_cast<hal::byte>(hal::usb::descriptor_type::interface));
    expect(hal::byte{ 0x5 } ==
           static_cast<hal::byte>(hal::usb::descriptor_type::endpoint));
  };

  "[success] class_code values match the USB spec"_test = []() {
    // Setup / Exercise / Verify
    expect(hal::byte{ 0x03 } ==
           static_cast<hal::byte>(hal::usb::class_code::hid));
    expect(hal::byte{ 0x08 } ==
           static_cast<hal::byte>(hal::usb::class_code::mass_storage));
    expect(hal::byte{ 0x02 } ==
           static_cast<hal::byte>(hal::usb::class_code::cdc_control));
  };
}
}  // namespace

int main()
{
  descriptor_size_test();
  enum_value_test();
}
