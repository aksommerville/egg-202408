# Egg

Portable games platform with execution in WebAssembly, video in OpenGL ES 2, and a custom synthesizer.

This repo builds the generic runtime `egg` and also the developer tool `eggdev`.

Sample games can be found at https://github.com/aksommerville/eggsamples
(they're playable directly in your browser from that repo, if you're just curious).

```
sudo apt install clang wabt
```

## Is Egg a good choice for my project?

NO. This is a work in progress and the API is very likely to change before the first official release.

But that being said...

The target use case is the style I prefer personally: Pixelly sprite graphics, beepy music, discrete inputs, and local multiplayer.
As a quick rule of thumb, if the game could have been written for SNES then it probably could be written gracefully for Egg.
But the platform is pretty generic and can support a lot more than that.

### Not supported at all, find a different platform (no hard feelings!)

- Networking.
- Recorded music or sound effects.
- Arbitrary filesystem access.
- Multiple threads.
- Direct PCM out.
- Vulkan, or any other video API beyond OpenGL ES 2.
- Hardware access, beyond our well-defined APIs.

### Supported but not ideal

- 3D graphics or custom shaders. If you're comfortable coding directly aganst GLES2, no problem.
- Script languages. You could build a JS, Lua, or whatever runtime in your Wasm app, but that's probably more work than it's worth.
- Very large games. Limited to 65535 of each resource type, and we like to load everything at once.
- Languages other than C. Eventually I'll get around to writing headers and whatnot for C++, Rust, Go, maybe others.

### Well-supported features

- 2D sprite graphics, arbitrary resolution.
- Music sourced from MIDI, into a custom synthesizer.
- Wasm runtime. Bring your own libc, and whatever else.
- Custom data resources.
- Tiny output size. Egg ROMs are often under 1 MB, and the runtime is 200..1000 kB (varies by underlying platform).
- Bundle your ROM with the runtime for a fake-native app or web page, with no Egg branding.
- Input from gamepads, keyboard, mouse, touch, and accelerometer.
- Permanent key/value store.
- Input mapping, state saving, screencaps, hard pause managed transparently by the platform, you get them for free.
- Multiple languages.

## TODO

- [ ] Raspberry Pi 1 (Broadcom video)
- [ ] MS Windows
- [ ] MacOS
- [ ] All structs declared to the public API must be the same size in wasm and native. Can we assert that somehow?
- [ ] TOUCH events for native. What would that take? Maybe acquire some tablet thingy and try it out?
- [ ] Editor
- - [ ] SfgEditor: Option to play audio server-side, and toggle between server and client on the fly.
- - [ ] Synth receiver (I want to run midevil against this server).
- - - This is never going to be a good choice due to buffer latency. Can we bake Egg's synthesizer into midevil instead?
- - [ ] SongEventsEditor: Smart "auto-repair" to reassign or drop channels, eliminate tempo and program changes, anything else needed by beeeeeP.
- - - arrautza/7-heart_of_stone.mid needs this due to channel 9. (could easily fix, but leave it so we can test auto-repair)
- - [ ] Might need more visibility for the server-side audio state.
- [ ] Choose a way to indicate the loop point in MIDI files. I believe both synthesizers are already ready for it.
- [ ] egg_gles2_wrapper.xc:glGetVertexAttribPointerv: What does this function do?
- [ ] glReadPixels et al: Could our buffer measurement come up short, if the user sets eg GL_UNPACK_ALIGNMENT to something wacky?

