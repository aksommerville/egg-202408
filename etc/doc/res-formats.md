# Egg Resource Formats

A valid ROM file must contain `metadata:0:1` and `wasm:0:1`, and they must be the first two members of the archive.

| tid  | Name                  | Desc |
|------|-----------------------|------|
| 0x00 | (illegal)             | |
| 0x01 | metadata              | qual always 0, rid always 1. See below. |
| 0x02 | wasm                  | qual always 0, rid always 1. |
| 0x03 | string                | Loose text. qual is language. Recommend UTF-8. |
| 0x04 | image                 | Encoded image file. qual is language or zero. Only PNG is supported. |
| 0x05 | song                  | See below. |
| 0x06 | sound                 | WAV or SFG (see below). qual is language or zero. |
| 0x07..0x0f | reserved        | Reserved for future use by Egg. |
| 0x10..0x2f | client          | Reserved for client use. |
| 0x30..0x3f | reserved        | Reserved for future use by Egg. |

metadata, string, image, song, and sound all get processed somewhat during `eggdev pack`.
User-defined types don't enjoy that privilege.
You'll want to preprocess your resources as needed, before invoking eggdev.
You can supply a file containing "TID NAME" as `--types=PATH` to most eggdev commands, to at least get sensible names for custom types.

## metadata

`metadata:0:1` is required, and it must be the first resource in an archive.
Must contain a field `framebuffer`. And I strongly recommend `title`, `iconImage`, and `language`.

Binary form starts with a 2-byte signature "\xeeM".
Followed by a series of key=value pairs, with keys and values each limited to 256 bytes:

```
u8 kc
u8 vc
... k
... v
repeat
```

Input file is like INI or Java properties. "key = value" or "key: value", whitespace optional.
'#' starts a line comment, start of line only.

C identifiers beginning with a lowercase letter are reserved for definition by Egg.
You may use any keys you like, for your own reasons, but please start them with uppercase letters, or a dot or something.

Fields containing text for human consumption can have a partner "*String", which is the decimal resource ID of a string in this archive,
for translation purposes. eg "title=My Cool Game", and "titleString=44", and then resource string:en:44 also contains "My Cool Game".
But string:fr:44 is "Mon Chouette Jeu" or whatever.
I recommend using both in this case, so lazy decoders don't have to look up strings or pick a language.

| Key                 | Str? | Description |
|---------------------|------|-------------|
| title               | yes  | Game's name. |
| language            | no   | Comma-delimited list of ISO 639 codes ordered by your preference. |
| framebuffer         | no   | "WIDTHxHEIGHT", decimal. Stop reading at whitespace, we might extend in the future. |
| iconImage           | no   | Decimal ID of an image resource, recommend 16x16 with alpha. |
| author              | yes  | Your name. |
| copyright           | yes  | eg "(c) 2024 AK Sommerville" |
| description         | yes  | Long-form text for human consumption. |
| genre               | yes  | Recommend picking one that's been used on Itch.io, so we're all on the same page. |
| tags                | yes  | Comma-delimited. Ditto Itch. |
| advisory            | yes  | Freeform text, warn of anything offensive etc. |
| rating              | no   | Ratings from official agencies, machine readable. TODO exact format. |
| timestamp           | no   | ISO 8601 ie "YYYY-MM-DDTHH:MM:SSS.mmm" (but stop anywhere; just "YYYY" is sensible) |
| version             | no   | Recommend semver eg "1.2.3". Version of your game, not the desired Egg version. |
| posterImage         | no   | Like iconImage but bigger. TODO recommend dimensions. |
| url                 | no   | Game's promo page. |
| contact             | no   | Email, telephone, or whatever. |
| freedom             | no   | "restricted", "limited", "intact", "free". Default "limited". |
| require             | no   | Comma-delimited list of machine readable features. TODO define features. |
| players             | no   | Integers and elapsis eg "1..4" |

## string

Resources in the archive are just plain text. I recommend UTF-8.

These can be sourced from files whose name is the ISO 639 language code, eg "en" is English.
You can also use comments in the file names, eg: "en-menus", "en-cutscene1_dialogue". These are dropped during compile.
Each non-empty line in the file is one string resource.
Line comments begin with '#', full lines only.
Starts with decimal ID or a C identifier name.
Followed by loose text to end of line, or a JSON string.

## song

TODO

## sound

TODO
