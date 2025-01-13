METANG 1
========

NAME
----

metang - generate multi-purpose C headers for enumerators

SYNOPSIS
--------

`metang` `COMMAND` [`OPTIONS`] [`<FILE>`]

DESCRIPTION
-----------

`metang` generates a customized C header of enumerated constants from a plain
text file. Each line of the input file is treated as another member of the
enumeration. It can generate enumerations of standard integer sequences, or
enumerations representative of a bit-mask.

COMMANDS
--------

`enum`
  Generate an integer-sequence enumeration.

`mask`
  Generate a bit-mask enumeration.

`help`
  Print help text and exit.

`version`
  Print the program version number and exit.

OPTIONS
-------

The following options are available to all generators:

`-o`, `--output` `<OFILE>`
  Write output to `<OFILE>`. If unspecified, then `metang` will write all
  generated content to standard output.

`-l`, `--leader` `<LEADER>`
  Treat `<LEADER>` as a prefix for generated enumeration symbols. Any value
  for `<LEADER>` -- given or derived -- will be converted to `UPPER_SNAKE_CASE`.

`-t`, `--tag-name` `<NAME>`
  Use `<NAME>` as the base name for the tag applied to the generated `enum`,
  lookup table member `struct`, and lookup table definition. If unspecified,
  `<NAME>` will be derived from the input file's basename, minus any extension.
  If the input stream feeds from standard input and this option is not
  specified, then `<NAME>` will be derived as `stdin`. Any value for this option
  is treated literally; no special formatting or casing will be applied.

`-G`, `--guard` `<GUARD>`
  Use `<GUARD>` as a prefix for all conditional directives. In C, this prefix is
  applied to all inclusion guards.

The following options are available to integer-sequence enumerations:

`-a`, `--append` `<ENTRY>`
  Append `<ENTRY>` to the input listing.

`-p`, `--prepend` `<ENTRY>`
  Prepend `<ENTRY>` to the input listing.

`-n`, `--start-from` `<NUMBER>`
  Start the integer-sequence enumeration from `<NUMBER>`.

Additionally, integer-sequence enumerations may directly assign the value of
member values in the input listing by appending `= <NUMBER>` to the member name.

AUTHOR
------

Rachel Forshee <lhearachel@proton.me>
