#!/bin/sh
# The tree has pre-existing trailing whitespace, so only whitespace
# errors introduced relative to the base branch (default: main) are
# flagged.
set -eu

# meson run_target executes from the build dir
if [ -n "${MESON_SOURCE_ROOT:-}" ]; then
   cd "$MESON_SOURCE_ROOT"
fi

# prefer the remote-tracking ref: it is updated by every fetch, while
# the local main branch is often stale
base="${1:-$(git rev-parse --abbrev-ref -q --verify 'main@{upstream}' || echo main)}"

# no HEAD endpoint so that uncommitted changes are checked too
git diff --check "$(git merge-base "$base" HEAD)"
echo "OK: no whitespace errors introduced relative to $base"
