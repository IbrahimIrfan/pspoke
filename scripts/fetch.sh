#!/usr/bin/env bash
# fetch.sh NAME... : download pinned upstream sources (third_party.lock) into .cache/upstream/NAME.
source "$(dirname "$0")/common.sh"
mkdir -p "$CACHE/upstream"
for name in "$@"; do
  line=$(awk -v n="$name" '$1==n{print $2, $3}' "$ROOT/third_party.lock"); [ -n "$line" ] || die "$name is not in third_party.lock"
  set -- $line; url=$1 commit=$2; dest="$CACHE/upstream/$name"
  if [ -d "$dest/.git" ] && [ "$(git -C "$dest" rev-parse HEAD 2>/dev/null)" = "$commit" ]; then echo "    $name already at ${commit:0:12}"; continue; fi
  rm -rf "$dest"; mkdir -p "$dest"; git -C "$dest" init -q
  git -C "$dest" fetch -q --depth 1 "$url" "$commit" || die "could not fetch $name ($url @ $commit)"
  git -C "$dest" checkout -q FETCH_HEAD; echo "    $name ${commit:0:12}"
done
