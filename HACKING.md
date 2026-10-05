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

## The varlink backend (synapticd)

`daemon/` holds `synapticd`, the privileged half of synaptic. It needs
libsystemd >= 257 for sd-varlink. To build against a systemd tree that
is built but not installed, point pkg-config at a `libsystemd.pc` that
describes it:

    meson configure build -Dpkg_config_path=/path/to/dir-with-libsystemd.pc

Poke at it with varlinkctl, which starts it as a child process:

    varlinkctl introspect build/daemon/synapticd io.github.mvo5.synaptic
    varlinkctl call build/daemon/synapticd io.github.mvo5.synaptic.Status '{}'
    sudo varlinkctl call --more build/daemon/synapticd io.github.mvo5.synaptic.UpdateCache '{}'

A commit that only downloads is a safe way to see the whole path
without changing the system (pick an installable version from
`apt-cache policy hello`):

    sudo varlinkctl call --more build/daemon/synapticd io.github.mvo5.synaptic.Commit \
      '{"selections":[{"name":"hello","arch":"amd64","action":"install","version":"2.10-3build1","auto":false}],
        "options":{"conffile":"keep","download_only":true}}'

Without `"terminal":true` the dpkg output is streamed as `output`
events, which is what you want from a shell; the GUI asks for the
terminal and gets the pty master passed as a file descriptor.

To run the GUI against the daemon instead of doing the privileged work
in-process, name the daemon in the environment; as a user it is started
through pkexec, as root directly:

    SYNAPTIC_DAEMON=$PWD/build/daemon/synapticd ./build/gtk/synaptic

`data/io.github.mvo5.synaptic.varlink` is the textual interface
description; `tests/test_synapticd.sh` checks it against what the
daemon serves, so regenerate it with the introspect command above after
changing `daemon/interface.c`.
