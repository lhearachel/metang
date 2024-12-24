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
- [Usage](#usage)
- [Contributing](#contributing)
- [License](#license)
<!--toc:end-->

## Background

C's support for enumerated constants is less than stellar. Member values of
`enum` don't contain any identifying information at runtime, which makes them
employing them as waymarks awkward when parsing string-based data. An easy way
to ameliorate this is by ingesting some simple input schema containing
enumeration members and generating a header file which exposes both the
enumeration and a lookup table mapping string values to members.

`metang` is a simple implementation of such a program. Its feature-set is basic,
yet allows a user to customize the content of their generated header as they see
fit. The headers that it generates are usable both in data parsing programs in
need of its lookup tables and in production code where only the enumeration
itself is desired.

### Etymology

`metang` takes its moniker from [the Pokémon of the same name][metang-pokedex].

## Install

### Build from Source

By default, `metang` will install to `/usr/local/bin`. If you wish to install it
to a different directory, simply prefix the `sudo make install` command below
with `DESTDIR=path/to/install/location`.

```shell
> git clone https://github.com/lhearachel/metang.git
> cd metang
> sudo make install
```

Once installed, verify that you can run the executable:

```shell
> metang -v
0.1.0
```

## Usage

For a summary of available options, `metang`'s built-in help-text should be
sufficient.

```shell
> metang --help
metang - Generate multi-purpose C headers for enumerators

Usage: metang [options] [<input_file>]

Options:
  -a, --append <entry>         Append <entry> to the input listing.
  -p, --prepend <entry>        Prepend <entry> to the input listing.
  -n, --start-from <number>    Start enumeration from <number>.
  -o, --output <file>          Write output to <file>.
  -G, --preproc-guard <guard>  Use <guard> as a prefix for conditional
                               preprocessor directives.
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
