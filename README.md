# dyld-shared-cache-extractor

[As of macOS Big Sur][mjtsai], instead of shipping the system libraries
with macOS, Apple ships a generated cache of all built in dynamic
libraries and excludes the originals. This tool allows you to extract
these libraries from the cache for reverse engineering.

## Usage

Extract the default shared cache to `/tmp/libraries`:

```sh
dyld-shared-cache-extractor /System/Library/dyld/dyld_shared_cache_arm64e /tmp/libraries
```

## Installation

[Homebrew](https://brew.sh):

```sh
brew install keith/formulae/dyld-shared-cache-extractor
```

Manually, after [installing rust](https://rustup.rs):

```sh
cargo install --locked --path .
```

## More details

There are a few different ways you can interact with these shared
caches.

1. Depending on what you're doing inspecting them in [Hopper][hopper] is
   the easiest option
2. For a bit more functionality you can build the
   `dyld_shared_cache_util` target from the latest `dyld` [source
   dump][dump], but this requires some [modifications][modifications]

The problem with the 2 options above is that they can lag behind format
changes in the shared cache. This tool loads the private
`dsc_extractor.bundle` from Xcode, meaning it should always be able to
extract the shared cache files even from beta OS versions (potentially
using a beta Xcode version).

This logic is based on the function at the bottom of
`dyld3/shared-cache/dsc_extractor.cpp` from the `dyld` [source
dump][dump].

[dump]: https://opensource.apple.com
[hopper]: https://www.hopperapp.com
[mjtsai]: https://mjtsai.com/blog/2020/06/26/reverse-engineering-macos-11-0
[modifications]: https://lapcatsoftware.com/articles/bigsur.html
