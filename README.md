# libhal-usb

A USB device enumerator and a set of USB protocol utilities (descriptor
construction, endpoint I/O helpers, standard-request handling) for embedded
systems using modern C++.

[![✅CI](https://github.com/libhal/libhal-usb/actions/workflows/ci.yml/badge.svg)](https://github.com/libhal/libhal-usb/actions/workflows/ci.yml)
[![GitHub stars](https://img.shields.io/github/stars/libhal/libhal-usb.svg)](https://github.com/libhal/libhal-usb/stargazers)
[![GitHub forks](https://img.shields.io/github/forks/libhal/libhal-usb.svg)](https://github.com/libhal/libhal-usb/network)
[![GitHub issues](https://img.shields.io/github/issues/libhal/libhal-usb.svg)](https://github.com/libhal/libhal-usb/issues)

## 📜 Origin

The `hal::usb::enumerator`, descriptor-construction, and endpoint I/O helper
code in this repository originated as `libhal-util`'s own
`include/libhal-util/usb/` directory (`constants.hpp`, `descriptors.hpp`,
`endpoints.hpp`, `enumerator.hpp`) in `libhal-util` v5. It was split out into
this standalone package and rewritten against the async, coroutine-based
`hal::usb` interfaces (`hal::usb::control_endpoint`, `in_endpoint`,
`out_endpoint`, `interface`) shipped by `libhal` v5's core `usb` module.

## 📚 Software APIs & Usage

To learn about the available APIs see the
[API Reference](https://libhal.github.io/latest/api/)
documentation page or look at the
[`modules`](https://github.com/libhal/libhal-usb/tree/main/modules)
directory.

## 🧰 Getting Started with libhal

Following the
[🚀 Getting Started](https://libhal.github.io/getting_started/)
instructions.

## 📥 Adding `libhal-usb` to your project

This section assumes you are using the
[`libhal-starter`](https://github.com/libhal/libhal-starter)
project.

Add the following to your `requirements()` method to the `ConanFile` class:

```python
    def requirements(self):
          self.requires("libhal-usb/[^1.0.0]")
```

The version number can be changed to whatever is appropriate for your
application. If you don't know what version to use, consider using the
[🚀 latest release](https://github.com/libhal/libhal-usb/releases/latest).

## 📥 Adding `libhal-usb` to your library

To add libhal-usb to your library package, do the following in the
`requirements` method of your `ConanFile` object:

```python
    def requirements(self):
          self.requires("libhal-usb/[^1.0.0]", transitive_headers=True)
```

Its important to add the `transitive_headers=True` to ensure that the
libhal-usb modules are accessible to the library user.

## 📦 Building & Installing the Library Package

If you'd like to build and install the libhal-usb package to your local conan
cache execute the following command:

```bash
conan create . --version=latest
```

Replace `latest` with the SEMVER version that fits the changes you've made. Or
just choose a number greater than whats been released.

> [!NOTE]
> `libhal-usb` distributes a prebuilt static library compiled from C++20
> modules, not headers. Setting the build type using the flag `-s build_type`
> affects the actual distributed library binary as well as the unit tests.
>
> It is advised to NOT use a platform profile such as `-pr lpc4078` or a cross
> compiler profile such as `-pr arm-gcc-12.3` when building the unit tests, as
> this will cause them to be built for an architecture that cannot be executed
> on your machine. Best to just stick with the defaults or specify your own
> compiler profile yourself.

## 🌟 Package Semantic Versioning Explained

In libhal, different libraries have different requirements and expectations for
how their libraries will be used and how to interpret changes in the semantic
version of a library.

If you are not familiar with [SEMVER](https://semver.org/) you can click the
link to learn more.

### 💥 Major changes

The major number will increment in the event of:

1. An API break
2. An ABI break
3. A behavior change

We define an API break as an intentional change to the public interface, found
within the `modules/` directory, that removes or changes an API in such a way
that code that previously built would no longer be capable of building.

We define an ABI break as an intentional change to the ABI of an object or
interface.

We define a "behavior change" as an intentional change to the documentation of
a public API that would change the API's behavior such that previous and later
versions of the same API would do observably different things. For example,
consider a call to `hal::usb::enumerator::run(context)`. If the description
for this API was changed from, "loops forever processing bus events" to
"processes a single pending bus event and returns", that would be a
behavioral change, since code relying on `run()` to drive enumeration forever
would no longer work correctly.

The usage of the term "intentional" means that the break or behavior change was
expected and accepted for a release. If an API break occurs on accident when it
wasn't previously desired, then such a change should be rolled back and an
alternative non-API breaking solution should be found.

You can depend on the major number to provide API, ABI, and behavioral
stability for your application. If you upgrade to a new major numbered version
of libhal, your code and applications may or may not continue to work as
expected or compile. Because of this, we try our best to not update the
major number.

### 🚀 Minor changes

The minor number will increment if a new interface, API, or type is introduced
into the public interface of `libhal-usb`.

### 🐞 Patch Changes

The patch number will increment if bug fixes that align code to the behavior
of an API, improve performance, or improve code size efficiency are made.

## :busts_in_silhouette: Contributing

See [`CONTRIBUTING.md`](CONTRIBUTING.md) for details.

## License

Apache 2.0; see [`LICENSE`](LICENSE) for details.
