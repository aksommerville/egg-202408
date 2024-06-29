# Egg Save States

State saving takes a snapshot of the runtime's state and allows to return to that state in the future.

This will never be available to true-native builds, since the state of a native program is too complex for us to serialize,
and the specifics would vary from platform to platform.

It's currently not available to the web runtime, but that is doable and I might implement in the future.
If so, I'll make an effort to use exactly the same serial format, so saved states remain portable.

## For The User

Launch with `--state=none` to disable, `--state=PATH` to use an explicit path, or nothing to make up a path.

By default, for a ROM path `/home/me/roms/egg/cool-game.egg`, we may produce:
 - `/home/me/roms/egg/cool-game.save`: Game-accessible store (not related to save states).
 - `/home/me/roms/egg/cool-game.state`: Saved state. (what we're talking about here)
 
When the game is running, press F10 to save or F9 to load.
You can change those bindings in the input config file, by default `~/.egg/input.cfg`.

## File Format

Integers are big-endian.

File is organized in chunks, from the very start:
```
0000   4  Chunk ID, ASCII. Illegal if any byte is outside 0x21..0x7e.
0004   4  Length.
0008 ...  Payload.
```

The first chunk is a zero-length signature, it must be exactly: "EgSv\0\0\0\0".

After that, chunks may appear in any order.

|  ID  | Description |
|------|-------------|
| EgSv | Signature, must be the first chunk, and must have length zero. |
| mmry | RAM state, see below. |
| joid | Connected joystick devices, 4 bytes each (devid). Runtime will report them as disconnected immediately on loading. |
| rwid | '' raw devices. |
| keys | Keycode of any held keys, 4 bytes each. Runtime will report them as released immediately on loading. |
| song | Song state, see below. |
| titl | Exact content of metadata:0:1 "title", to prevent loading state for the wrong game. |
| magc | '' "magic" |
| vers | '' "version". Compared only if "magc" absent. |
| emsk | 4-byte event mask. |
| txim | 4-byte texture id, 2-byte qual, 2-byte image id |
| txrw | 4-byte texture id, PNG file |

Only `mmry`, `txim`, `txrw` may appear more than once.

`mmry`: Memory state.
Start with the heap zeroed, then write chunks in the order encoded.
```
0000   4  Address
0008 ...  Content (length inferred from chunk length)
```

`song`: Synthesizer state.
Sound effects are not persisted.
```
0000   2  Song qual
0002   2  Song rid
0004   1  Song repeat
0005   4  Playhead s16.16
0009
```
