<img src="docs/metang.png" align="right" width="120" alt="In-game sprite of Metang from Pokémon Black and White"/>

# Metang

A metaprogramming utility to generate enumerable constants for multiple
languages.

## Table of Contents

- [Background](#background)
  - [Etymology](#etymology)
- [Install](#install)
  - [Build from Source](#build-from-source)
- [Usage](#usage)
- [Contributing](#contributing)
- [License](#license)

## Background

C's `enum` support is simple and powerful, yet lacks modern features common in
other languages. Member values decay to numeric types and thus contain no
personally-identifiable information at runtime; this can make employing them as
waymarks in text-based formats awkward, e.g., when representing program data via
JSON structures. An easy amelioration is to write a small program which ingests
an input stream of symbols and generates a header file to expose both the `enum`
definition and a corresponding table to match member-names to their values.

`metang` is an implementation of such a program. Its feature-set is basic, yet
allows a user to customize the generated output via a small suite of input
options. The headers that it generates are usable both by data-parsing tools
which benefit from fast member-lookup and by production code where only the
`enum` itself is needed. Its multi-language model also allows for sharing sets
of definitions across toolchains within a polyglot project; define your `enum`
once, process it into output files, and safely make use of it everywhere with
guaranteed consensus.

### Etymology

`metang` takes its name from [the Pokémon of the same name][metang-pokedex]. It
serves as a nod to [the project which inspired its construction][gh-pokeplat]
and lightly hints to its purpose with the `meta-` prefix.

## Install

### Build from Source

If you only want to build the project:

```shell
> git clone https://github.com/lhearachel/metang.git
> cd metang
> meson setup build
> meson compile -C build
```

Specify any additional options to Meson as desired. The default build
configuration will produce an executable with debug symbols, no optimizations,
and assertions enabled, and it will attempt to link against Address Sanitizer.

If you have installed [`casey/just`][gh-casey-just], you may also make use of
the `Justfile` included with the repository, which will configure a
release-build of `metang`, install it to `~/.local/bin/metang`, and generate
manual pages to `~/.local/share/man/man1/metang.1`:

```shell
> just install
```

## Usage

An summary of program options is available via `metang`'s built-in help-text:

```text
metang - generate enumerable constants for multiple langauges

Usage: metang [options] <file>
       metang -h | --help
       metang --version

Options:
  -b / --bitmask         Generate an enumerated bitmask.
  -o / --output <file>   Write generated content to a file.
  -l / --lang <lang>     Generate enumerables for a specified language.
  -t / --tag <tag>       Prefix generated enums and structs with <tag>.
  -g / --guard <guard>   Prefix pre-processor conditionals with <guard>.
  -s / --sized <size>    Bind the size and sign of the enum, if supported.
                         e.g. in C, 8 binds to uint8_t, -8 to int8_t, etc.

Languages Supported:
  c     C enum, #defines, and a binary-searchable member-lookup table.
  cpp   C++ enum with a member-lookup table using std::map.
  py    Python class derived from enum.IntEnum or enum.IntFlag.
```

For greater detail on generated content, refer to [the more verbose
documentation](./docs/metang.adoc). This document is converted into a Unix
manual page by the installation process and can be accessed via `man metang`.

## Contributing

`metang`'s small size and problem-scope mean that contribution guidelines are
loose. Feel free to file an issue or a pull request to support a new language or
fix a bug!

## License

`metang` is free software distributed under the MIT License. For further
details, refer to [the included license text](./LICENSE).

[gh-casey-just]: https://github.com/casey/just
[gh-pokeplat]: https://github.com/pret/pokeplatinum
[metang-pokedex]: https://www.pokemon.com/us/pokedex/metang
