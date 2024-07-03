# Egg ROM Format

Binary, and integers are big-endian.

Every resource is uniquely identified by 32 bits:
- 0xfc000000 6-bit type id "tid". Nonzero.
- 0x03ff0000 10-bit qualifier "qual". Usually zero, or a packed language code.
- 0x0000ffff 16-bit resource id "rid". Nonzero.

qual: Read as two 5-bit characters in this alphabet: "012345abcdefghijklmnopqrstuvwxyz".
So qual zero represents as "00", and the two-letter combinations are all nonzero.
When not zero, it's expected to be an ISO 639 language code.

Zero-length resources are indistinguishable from absent ones.
The archive's framing is designed such that resources must be sorted by (tid,qual,rid).

Maximum resource length is 538968190, a little over 500 MB.

Beware that it is possible to arbitrarily pad an Egg ROM.
Don't depend on a ROM file's hash if it needs to be secure.

Starts with Header:
```
0000   4 Signature: 0xea 0x00 0xff 0xff
0004   4 Header length, minimum 16.
0008   4 TOC length.
000c   4 Heap length.
0010
```

Any header content beyond 16 bytes is reserved for future use and must be ignored.

Header is followed immediately by TOC.
Read bytewise with a state machine:
```
tid = 1
qual = 0
rid = 1
heapp = 0
```
Then read commands, which are distinguishable from high bits of their first byte:
```
0lllllll                              SMALL. Add resource. heapp+=l, rid+=1. Zero is legal.
100lllll llllllll llllllll            MEDIUM. Add resource. heapp+=l+128, rid+=1
101lllll llllllll llllllll llllllll   LARGE. Add resource. heapp+=l+2097279, rid+=1
110000qq qqqqqqqq                     QUAL. qual+=q+1, rid=1
1101rrrr                              RID. rid+=r+1
1100rr..                              Reserved (r nonzero).
111ttttt                              TYPE. tid+=t+1, qual=0, rid=1
```

Overflowing rid DOES NOT advance qual, nor does overflowing qual advance tid.
It is an error if one of the "Add resource" commands occurs where tid>63, qual>1023, or rid>65535.

TOC is followed immediately by Heap, which is just loose data.

Content after Heap is reserved for future use and should be ignored.

## Alternate Format for Embedded Web Apps

We could base64 the whole ROM file and drop that in some HTML tag,
but then the app is burdened with decoding that massive chunk of base64, before it can start real work.
So when we build an embedded web app, we rewrite the ROM in a slightly different text format.

It operates with the same state machine as the binary format.
Read one character -- a lowercase letter -- from the input, then read a hexadecimal integer with no prefix,
then for the 'r' command, read a parenthesized chunk of base64.

Whitespace is permitted anywhere, even within those integer tokens.

Commands:
- 't N': tid+=N+1, qual=0, rid=1
- 'q N': qual+=N+1, rid=1
- 's N': rid+=N+1
- 'r N (...)': Add resource, length N, rid++. After decoding base64, zero-pad or truncate to the stated length.

## Conventional Source Layout

```
myproject/
  src/ -- Or sometimes I put "data/" in the root, it doesn't matter.
    data/
      metadata
      image/
        1-appicon.png
        2-whatever.png
      sound/
        midi_drums.sfg -- multi-resource
        mysounds.sfg   -- multi-resource
        1-pow.wav
        2-bleep.wav
      song/
        1-intro.mid
        2-play.mid
        3-gameover.mid
      string/
        en          -- multi-resource
        ru          -- multi-resource
        en-dialogue -- "-dialogue" gets lost during compile, it's only for the editor
        ru-dialogue -- ...and of course use whatever comments you like.
      mycustomtype/
        1
        2.myext
        3-name_of_thing.myext
        implicit_id.myext
```

Key points:
- All data must be stored in one directory, and nothing else in there.
- "metadata" lives in the data root.
- All other resources should be exactly one directory deep, in a directory whose name is exactly a resource type name.
- Single-resource files usually begin with a decimal ID, but we'll make one up if not.
- `string` and `sound` pack multiple resources into one source file.
- The name of a `string` file begins with its qualifier, a two-letter ISO 639 language code.
- Other types may also generically indicate a qualifier: `ID-QUAL-NAME.FORMAT`
