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

#include <coroutine>
#include <span>

export module hal.usb:endpoints;

import hal;

// TODO(#79): Add doxygen docs to USB APIs
namespace hal::usb::inline v1 {

export async::future<void> write(async::context& p_context,
                                  control_endpoint& p_endpoint,
                                  mem::scatter_span<hal::byte const> p_data_out)
{
  co_await p_endpoint.write(p_context, p_data_out);
}

export async::future<void> write_and_flush(
  async::context& p_context,
  control_endpoint& p_endpoint,
  mem::scatter_span<hal::byte const> p_data_out)
{
  co_await p_endpoint.write(p_context, p_data_out);
  co_await p_endpoint.write(p_context, {});
}

export async::future<void> write(async::context& p_context,
                                  control_endpoint& p_endpoint,
                                  std::span<hal::byte const> p_data_out)
{
  co_await p_endpoint.write(p_context, mem::scatter_span<hal::byte const>{
                                          p_data_out });
}

export async::future<void> write_and_flush(
  async::context& p_context,
  control_endpoint& p_endpoint,
  std::span<hal::byte const> p_data_out)
{
  co_await p_endpoint.write(p_context, mem::scatter_span<hal::byte const>{
                                          p_data_out });
  co_await p_endpoint.write(p_context, {});
}

export async::future<void> write(async::context& p_context,
                                  in_endpoint& p_endpoint,
                                  mem::scatter_span<hal::byte const> p_data_out)
{
  co_await p_endpoint.write(p_context, p_data_out);
}

export async::future<void> write_and_flush(
  async::context& p_context,
  in_endpoint& p_endpoint,
  mem::scatter_span<hal::byte const> p_data_out)
{
  co_await p_endpoint.write(p_context, p_data_out);
  co_await p_endpoint.write(p_context, {});
}

export async::future<void> write(async::context& p_context,
                                  in_endpoint& p_endpoint,
                                  std::span<hal::byte const> p_data_out)
{
  co_await p_endpoint.write(p_context, mem::scatter_span<hal::byte const>{
                                          p_data_out });
}

export async::future<void> write_and_flush(
  async::context& p_context,
  in_endpoint& p_endpoint,
  std::span<hal::byte const> p_data_out)
{
  co_await p_endpoint.write(p_context, mem::scatter_span<hal::byte const>{
                                          p_data_out });
  co_await p_endpoint.write(p_context, {});
}

export async::future<void> write(async::context& p_context,
                                  in_endpoint& p_endpoint,
                                  mem::spanable auto... p_data_out)
{
  mem::scatter_array<hal::byte const, sizeof...(p_data_out)> const data{
    p_data_out...
  };
  co_await p_endpoint.write(p_context, data);
}

export async::future<void> write_and_flush(async::context& p_context,
                                            in_endpoint& p_endpoint,
                                            mem::spanable auto... p_data_out)
{
  mem::scatter_array<hal::byte const, sizeof...(p_data_out)> const data{
    p_data_out...
  };
  co_await p_endpoint.write(p_context, data);
  co_await p_endpoint.write(p_context, {});
}

export async::future<hal::usize> read(async::context& p_context,
                                       out_endpoint& p_endpoint,
                                       mem::scatter_span<hal::byte> p_data_in)
{
  co_return co_await p_endpoint.read(p_context, p_data_in);
}

export async::future<hal::usize> read(async::context& p_context,
                                       out_endpoint& p_endpoint,
                                       std::span<hal::byte> p_data_in)
{
  co_return co_await p_endpoint.read(
    p_context, mem::scatter_span<hal::byte>{ p_data_in });
}

export async::future<hal::usize> read(async::context& p_context,
                                       control_endpoint& p_endpoint,
                                       mem::scatter_span<hal::byte> p_data_in)
{
  co_return co_await p_endpoint.read(p_context, p_data_in);
}

export async::future<hal::usize> read(async::context& p_context,
                                       control_endpoint& p_endpoint,
                                       std::span<hal::byte> p_data_in)
{
  co_return co_await p_endpoint.read(
    p_context, mem::scatter_span<hal::byte>{ p_data_in });
}

}  // namespace hal::usb::inline v1
