# Standard stuff

.SUFFIXES:

MAKEFLAGS+= --no-builtin-rules  # Disable the built-in implicit rules.
# MAKEFLAGS+= --warn-undefined-variables        # Warn when an undefined variable is referenced.
# MAKEFLAGS+= --include-dir=$(CURDIR)/conan     # Search DIRECTORY for included makefiles (*.mk).

#-dk: note to self: PATH=/opt/llvm-19.1.6/bin:$PATH LDFLAGS=-fuse-ld=lld

.PHONY: config test default install examples clean distclean doc docs html pdf format clang-format tidy

export CXX := g++

BUILDDIR = build
PRESET = gcc-release
UNAME = $(shell uname -s)
ifeq ($(UNAME),Darwin)
    # PRESET = appleclang-release
    ifeq ($(shell uname -m),arm64)
        PRESET = appleclang-release
    else
        PRESET = gcc-release
    endif
endif
BUILD = $(BUILDDIR)/$(PRESET)

default: $(BUILD)/compile_commands.json
	cmake --workflow --preset=$(PRESET)
	cmake --install $(BUILD) --prefix=$(BUILDDIR)/statedir

examples: default
	cmake -S $@ -G Ninja -B build/examples -D CMAKE_BUILD_TYPE=RelWithDebInfo \
		-D CMAKE_PREFIX_PATH=$(CURDIR)/$(BUILDDIR)/statedir \
		--fresh --log-level=VERBOSE
	ninja -C build/examples all
	ninja -C build/examples test

docs doc:
	cd docs; $(MAKE)

pdf html:
	cd docs; $(MAKE) doc-$@

$(BUILD)/compile_commands.json: Makefile CMakePresets.json
	cmake --preset=$(PRESET) --log-level=VERBOSE
	ln -sf $(@) .

list:
	cmake --workflow --list-presets

format:
	pre-commit run --all

clang-format:
	git clang-format main

$(BUILDDIR)/tidy/compile_commands.json: Makefile CMakeLists.txt
	CC=$(CXX) cmake -G Ninja -S . -B $(BUILDDIR)/tidy \
	  -D CMAKE_EXPORT_COMPILE_COMMANDS=1 \
      --fresh --log-level=VERBOSE
	ln -sf $(@) .

tidy: $(BUILDDIR)/tidy/compile_commands.json
	run-clang-tidy -p $(BUILDDIR)/tidy tests examples

test: $(BUILDDIR)
	cd $(BUILDDIR); ctest

clean:
	cmake --build $(BUILD) --target clean

distclean:
	$(RM) -r $(BUILD) $(BUILDDIR)
