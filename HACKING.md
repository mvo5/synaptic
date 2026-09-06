# Hacking on synaptic

## Dependencies

On Debian or Ubuntu the quickest way to get everything is

    sudo apt build-dep synaptic

The authoritative list is `Build-Depends` in `debian/control`.

## Building

    meson setup build
    ninja -C build

The binary is `build/gtk/synaptic`. Build options live in
`meson_options.txt`; list them and their current values with

    meson configure build

and change one with e.g. `meson configure build -Dpkg_hold=true`.

## Running the tests

    ninja -C build test

is the `make test` equivalent. For more control use meson directly:

    meson test -C build --print-errorlogs
    meson test -C build test_rsources
    meson test -C build test_rsources --gtest-args='--gtest_shuffle --gtest_repeat=5'

`test_rsources` uses GoogleTest and is only built when `gtest` is found.
`-Dtests=enabled` makes a missing `libgtest-dev` a configure error, which is
what the Debian package build does; `-Dtests=disabled` skips it.

`tests/test_gtkpkglist` is an interactive viewer for the package list widget,
not a test, so it is built but not registered with `meson test`.

## Style checks

    ninja -C build lint                # whitespace errors introduced relative to master
    ninja -C build clang-format-check  # formatting per .clang-format

`./fmt` runs clang-format in place over every C++ file under `common/`,
`gtk/` and `tests/`; for a single file use `clang-format -i path/to/file.cc`.
Boolean arguments at call sites are annotated systemd style, e.g.
`GetListOfFilesInDir(Dir, ext, /* SortList */ true)`.

## Building the Debian package

    gbp buildpackage

or plain `dpkg-buildpackage -us -uc`; `debian/gbp.conf` holds the branch
layout for git-buildpackage.
