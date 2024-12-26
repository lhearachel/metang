<img src="img/metang.png" align="right" width="120" alt="In-game sprite of Metang from Pokémon Black and White"/>

# `metang`

A metaprogramming utility to generate multi-purpose C headers of enumerated
constants.

## Table of Contents

<!--toc:start-->
- [Background](#background)
  - [Etymology](#etymology)
- [Install](#install)
  - [Build from Source](#build-from-source)
  - [Integrate with a Meson Project](#integrate-with-a-meson-project)
- [Usage](#usage)
- [Contributing](#contributing)
- [License](#license)
<!--toc:end-->

## Background

C's support for enumerated constants is less than stellar. Member values of
`enum` don't contain any identifying information at runtime, which makes
employing them as waymarks awkward when parsing textual data. An easy way to
ameliorate this is by ingesting some simple input schema containing enumeration
members and generating a header file which exposes both the enumeration and a
lookup table mapping string values to members.

`metang` is a simple implementation of such a program. Its feature-set is basic,
yet allows a user to customize the content of their generated header as they see
fit. The headers that it generates are usable both in data parsing programs in
need of its lookup tables and in production code where only the enumeration
itself is desired.

### Etymology

`metang` takes its moniker from [the Pokémon of the same name][metang-pokedex].

## Install

### Build from Source

```shell
> git clone https://github.com/lhearachel/metang.git
> cd metang
> make install
```

Once installed, verify that you can run the executable:

```shell
> metang -v
0.1.0
```

By default, `metang` will install to `~/.local/bin`. If you wish to install it
to a different directory, run `make install` with your desired directory as an
argument:

```shell
> DESTDIR=path/to/target make install
```

`metang` will install itself into the `bin` subdirectory of the given value for
`DESTDIR`.

### Integrate with a Meson Project

`metang` also ships with a `meson.build` file to support integration as a Meson
subproject. To add `metang`'s tooling support to your project, create the
following `metang.wrap` file in your `subprojects` directory:

```ini
[wrap-git]
url = https://github.com/lhearachel/metang.git
; Replace <main> here with a release tag or commit hash, if desired.
revision = main
depth = 1

[provide]
program_names = metang
```

## Usage

`metang` will generate a C header from some input file with the following
content sections, each of which is surrounded by a corresponding preprocessor
guard:

1. An `enum` type definition.
2. Preprocessor definitions.
3. A lookup table mapping enumerators to a string identifier.

To illustrate, suppose that we have the following input file, named `my_input`:

```text
Bulbasaur
Ivysaur
Venusaur
Charmander
Charmeleon
Charizard
Squirtle
Wartortle
Blastoise
```

`metang` will then emit the following to standard output:

```c
/*
 * This file was generated by metang; DO NOT MODIFY IT!!
 * Source file: my_input
 * Program options:
 */
#ifndef METANG_STDOUT
#define METANG_STDOUT

#ifdef METANG_ENUM

#undef METANG_DEFS /* Do not allow also including the defs section */

enum my_input {
    STDOUT_BULBASAUR = 0,
    STDOUT_IVYSAUR = 1,
    STDOUT_VENUSAUR = 2,
    STDOUT_CHARMANDER = 3,
    STDOUT_CHARMELEON = 4,
    STDOUT_CHARIZARD = 5,
    STDOUT_SQUIRTLE = 6,
    STDOUT_WARTORTLE = 7,
    STDOUT_BLASTOISE = 8,
};

#endif // METANG_ENUM

#if defined(METANG_DEFS) || !defined(METANG_ENUM)

#define STDOUT_BULBASAUR 0
#define STDOUT_IVYSAUR 1
#define STDOUT_VENUSAUR 2
#define STDOUT_CHARMANDER 3
#define STDOUT_CHARMELEON 4
#define STDOUT_CHARIZARD 5
#define STDOUT_SQUIRTLE 6
#define STDOUT_WARTORTLE 7
#define STDOUT_BLASTOISE 8

#endif // defined(METANG_DEFS) || !defined(METANG_ENUM)

#ifdef METANG_LOOKUP

struct my_input_entry {
    const long value;
    const char *def;
};

const struct my_input_entry my_input_lookup[] = {
    { STDOUT_BLASTOISE, "BLASTOISE" },
    { STDOUT_BULBASAUR, "BULBASAUR" },
    { STDOUT_CHARIZARD, "CHARIZARD" },
    { STDOUT_CHARMANDER, "CHARMANDER" },
    { STDOUT_CHARMELEON, "CHARMELEON" },
    { STDOUT_IVYSAUR, "IVYSAUR" },
    { STDOUT_SQUIRTLE, "SQUIRTLE" },
    { STDOUT_VENUSAUR, "VENUSAUR" },
    { STDOUT_WARTORTLE, "WARTORTLE" },
};

#endif // METANG_LOOKUP

#endif // METANG_STDOUT
```

The default values in this emission are worth noting:

1. `METANG_` as a prefix on all preprocessor guards. This is a simple default to
   make output consistent.
2. `STDOUT` as the suffix on the inclusion sentinels and leading prefix on the
   generated symbols. This is adapted from the output file name.
3. `my_input` as the `enum` tag. This is adapted from the input file name.
4. `snake_case` on `enum`, `struct`, and variable tags.
5. A starting index of `0` for the enumeration.

All of these defaults are configurable via program options. As a summary of
available options, `metang`'s built-in help-text should be sufficient. For
further reading, each of these options has a corresponding example in the
`tests` directory to illustrate how its behavior differs from the defaults.

```shell
> metang --help
metang - Generate multi-purpose C headers for enumerators

Usage: metang [options] [<input_file>]

Options:
  -a, --append <entry>         Append <entry> to the input listing.
  -p, --prepend <entry>        Prepend <entry> to the input listing.
  -n, --start-from <number>    Start enumeration from <number>.
  -o, --output <file>          Write output to <file>.
  -l, --leader <leader>        Use <leader> as a prefix for generated symbols.
  -t, --tag-name <name>        Use <name> as the base tag for enums and lookup
                               tables.
  -c, --tag-case <case>        Customize the casing of generated tags for enums
                               and lookup tables. Options: snake, pascal
  -G, --preproc-guard <guard>  Use <guard> as a prefix for conditional
                               preprocessor directives.
  -B, --bitmask                If specified, generate symbols for a bitmask.
                               This option is incompatible with the “-D” and
                               “-n” options.
  -D, --allow-overrides        If specified, allow direct value-assignment.
  -h, --help                   Display this help text and exit.
  -v, --version                Display the program version number and exit.
```

## Contributing

`metang`'s small size and problem-scope mean that contribution guidelines are
loose. Feel free to file an issue or a pull request!

## License

`metang` is free software licensed under the Apache License, version 2.0. For
further details, refer to [the included license text](./LICENSE).

[metang-pokedex]: https://www.pokemon.com/us/pokedex/metang
