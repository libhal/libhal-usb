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
#include <coroutine>
#include <span>

#include <boost/ut.hpp>

import hal;
import hal.usb;
import test_util;

namespace {

struct test_in_endpoint : public hal::usb::in_endpoint
{
  std::array<hal::byte, 64> m_written{};
  hal::usize m_written_length = 0;
  hal::usize m_write_calls = 0;
  bool m_last_write_was_zlp = false;

  [[nodiscard]] std::span<hal::byte const> written() const
  {
    return std::span(m_written).first(m_written_length);
  }

private:
  [[nodiscard]] hal::usb::endpoint_info driver_info() const override
  {
    return { .size = 64, .number = 0x81, .stalled = false };
  }

  async::future<void> driver_stall(async::context&, bool) override
  {
    return {};
  }

  async::future<void> driver_reset(async::context&) override
  {
    return {};
  }

  async::future<void> driver_write(
    async::context&,
    mem::scatter_span<hal::byte const> p_data) override
  {
    ++m_write_calls;
    m_last_write_was_zlp = (p_data.length() == 0);
    for (auto chunk : p_data) {
      for (auto b : chunk) {
        if (m_written_length < m_written.size()) {
          m_written[m_written_length++] = b;
        }
      }
    }
    return {};
  }
};

struct test_out_endpoint : public hal::usb::out_endpoint
{
  std::array<hal::byte, 8> m_source{};
  hal::usize m_available = 0;

  void set_data(std::span<hal::byte const> p_data)
  {
    m_available = p_data.size();
    for (hal::usize i = 0; i < p_data.size() and i < m_source.size(); ++i) {
      m_source[i] = p_data[i];
    }
  }

private:
  [[nodiscard]] hal::usb::endpoint_info driver_info() const override
  {
    return { .size = 8, .number = 0x01, .stalled = false };
  }

  async::future<void> driver_stall(async::context&, bool) override
  {
    return {};
  }

  async::future<void> driver_reset(async::context&) override
  {
    return {};
  }

  async::future<void> driver_on_receive(async::context&) override
  {
    return {};
  }

  async::future<hal::usize> driver_read(
    async::context&,
    mem::scatter_span<hal::byte> p_buffer) override
  {
    hal::usize written = 0;
    for (auto chunk : p_buffer) {
      for (auto& b : chunk) {
        if (written >= m_available) {
          co_return written;
        }
        b = m_source[written++];
      }
    }
    m_available = 0;
    co_return written;
  }
};

async::inplace_context<1024> ctx;

void control_endpoint_write_test()
{
  using namespace boost::ut;

  "[success] write(control_endpoint&, scatter_span)"_test = []() {
    // Setup
    hal::usb::test::mock_control_endpoint endpoint;
    std::array<hal::byte, 3> const data{ 1, 2, 3 };

    // Exercise
    auto future =
      hal::usb::write(ctx, endpoint, mem::scatter_span<hal::byte const>{ data });
    hal::usb::test::pump(future);

    // Verify
    expect(that % 1 == endpoint.m_write_call_count);
    expect(not endpoint.m_last_write_was_zlp);
    expect(that % 3 == endpoint.written().size());
  };

  "[success] write_and_flush(control_endpoint&, span)"_test = []() {
    // Setup
    hal::usb::test::mock_control_endpoint endpoint;
    std::array<hal::byte, 2> const data{ 9, 8 };

    // Exercise
    auto future = hal::usb::write_and_flush(
      ctx, endpoint, std::span<hal::byte const>(data));
    hal::usb::test::pump(future);

    // Verify
    expect(that % 2 == endpoint.m_write_call_count);
    expect(endpoint.m_last_write_was_zlp);
    expect(that % 2 == endpoint.written().size());
  };
}

void in_endpoint_write_test()
{
  using namespace boost::ut;

  "[success] write(in_endpoint&, scatter_span)"_test = []() {
    // Setup
    test_in_endpoint endpoint;
    std::array<hal::byte, 4> const data{ 1, 2, 3, 4 };

    // Exercise
    auto future =
      hal::usb::write(ctx, endpoint, mem::scatter_span<hal::byte const>{ data });
    hal::usb::test::pump(future);

    // Verify
    expect(that % 1 == endpoint.m_write_calls);
    expect(not endpoint.m_last_write_was_zlp);
    expect(that % 4 == endpoint.written().size());
  };

  "[success] write_and_flush(in_endpoint&, span)"_test = []() {
    // Setup
    test_in_endpoint endpoint;
    std::array<hal::byte, 2> const data{ 5, 6 };

    // Exercise
    auto future = hal::usb::write_and_flush(
      ctx, endpoint, std::span<hal::byte const>(data));
    hal::usb::test::pump(future);

    // Verify
    expect(that % 2 == endpoint.m_write_calls);
    expect(endpoint.m_last_write_was_zlp);
  };

  "[success] write(in_endpoint&, variadic spans)"_test = []() {
    // Setup
    test_in_endpoint endpoint;
    std::array<hal::byte, 2> const header{ 0xAA, 0xBB };
    std::array<hal::byte, 3> const body{ 1, 2, 3 };

    // Exercise
    auto future = hal::usb::write(ctx, endpoint, header, body);
    hal::usb::test::pump(future);

    // Verify
    expect(that % 1 == endpoint.m_write_calls);
    expect(that % 5 == endpoint.written().size());
    expect(hal::byte{ 0xAA } == endpoint.written()[0]);
    expect(hal::byte{ 1 } == endpoint.written()[2]);
  };
}

void out_endpoint_read_test()
{
  using namespace boost::ut;

  "[success] read(out_endpoint&, scatter_span)"_test = []() {
    // Setup
    test_out_endpoint endpoint;
    std::array<hal::byte, 3> const source{ 7, 8, 9 };
    endpoint.set_data(source);
    std::array<hal::byte, 8> buffer{};

    // Exercise
    auto future = hal::usb::read(
      ctx, endpoint, mem::scatter_span<hal::byte>{ std::span(buffer) });
    hal::usb::test::pump(future);

    // Verify
    expect(that % 3 == future.value());
    expect(hal::byte{ 7 } == buffer[0]);
    expect(hal::byte{ 9 } == buffer[2]);
  };

  "[success] read(out_endpoint&, span)"_test = []() {
    // Setup
    test_out_endpoint endpoint;
    std::array<hal::byte, 2> const source{ 4, 5 };
    endpoint.set_data(source);
    std::array<hal::byte, 8> buffer{};

    // Exercise
    auto future = hal::usb::read(ctx, endpoint, std::span<hal::byte>(buffer));
    hal::usb::test::pump(future);

    // Verify
    expect(that % 2 == future.value());
    expect(hal::byte{ 4 } == buffer[0]);
  };
}

void control_endpoint_read_test()
{
  using namespace boost::ut;

  "[success] read(control_endpoint&, span)"_test = []() {
    // Setup
    hal::usb::test::mock_control_endpoint endpoint;
    std::array<hal::byte, 8> const setup{ 1, 2, 3, 4, 5, 6, 7, 8 };
    endpoint.set_read_data(setup);
    std::array<hal::byte, 8> buffer{};

    // Exercise
    auto future = hal::usb::read(ctx, endpoint, std::span<hal::byte>(buffer));
    hal::usb::test::pump(future);

    // Verify
    expect(that % 8 == future.value());
    expect(hal::byte{ 1 } == buffer[0]);
    expect(hal::byte{ 8 } == buffer[7]);
  };
}

}  // namespace

int main()
{
  control_endpoint_write_test();
  in_endpoint_write_test();
  out_endpoint_read_test();
  control_endpoint_read_test();
}
