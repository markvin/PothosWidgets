# Graphical widgets to supplement the Pothos GUI

This toolkit gives graphical control widgets for the Pothos GUI: sliders,
entries, drop-down lists, radio groups, buttons, displays and more.

This branch (`pothos2`) builds against the modernized core
(https://github.com/markvin/PothosCore, branch `pothos2`). The module uses
only the public API of the core.

## Dependencies

- an installed Pothos core (`find_package(Pothos)`);
- Qt 6.5 or higher with the Widgets component (for example the Arch Linux
  package `qt6-base`, or a Qt install from the Qt installer);
- CMake 3.25 or higher and a C++23 compiler.

When Qt 6 is missing or older than 6.5, the configure step stops with an
error. Qt 5 is not supported.

## Build

```sh
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=<core prefix>
cmake --build build
ctest --test-dir build --output-on-failure
```

`<core prefix>` is the installation prefix of the Pothos core. Do not set
it when the core is installed in a default location of the system.

When Qt is not in a default location of the system, add its prefix to
`CMAKE_PREFIX_PATH` (for example `-DCMAKE_PREFIX_PATH="<core prefix>;$HOME/Qt/6.8.0/gcc_64"`),
or give `Qt6_DIR`. On Linux and macOS, the installed module then has an
RPATH to the Qt library directory. On Windows, put the Qt `bin` directory
on `PATH`.

Install the module:

```sh
cmake --install build
```

The core loads modules only from its own prefix (and from
`POTHOS_PLUGIN_PATH`). Thus the default install prefix of this project is
the prefix of the core. When you give `CMAKE_INSTALL_PREFIX`, set
`POTHOS_PLUGIN_PATH` to `<prefix>/lib/Pothos/modules<ABI version>`.

## Tests

The test `widgets.stage` installs the module into `build/stage`. Then
`widgets.selftests` runs each self-test (`/widgets/tests/...`) with the
PothosUtil of the core and `POTHOS_PLUGIN_PATH` set to the staged module.

The self-tests make each widget, call its setters and read the values
back. They need no display: CTest sets `QT_QPA_PLATFORM=offscreen`. When
you run a self-test without CTest and no Qt application exists, the test
also uses the `offscreen` platform:

```sh
POTHOS_PLUGIN_PATH=build/stage/lib/Pothos/modules<ABI version> \
    <core prefix>/bin/PothosUtil --self-test1 /widgets/tests/test_slider
```

`widgets.selftests` stops with an error when the widgets module is also
installed in the prefix of the core (the core would load the two copies).
Remove the installed copy, or test against another installation of the
core.

## Documentation

* https://github.com/pothosware/PothosWidgets/wiki

## Licensing information

Use, modification and distribution is subject to the Boost Software
License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
