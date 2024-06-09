# Egg

```
sudo apt install clang wabt
```

## TODO

- [ ] Web runtime
- - [ ] Audio set playhead
- - [ ] incfg: First, is it worth the effort? How much coverage does Standard Mapping give us? And then.... how?
- [ ] Native runtime for Linux
- - [ ] Invoke incfg via client too.
- - - Think this thru. We'll need to add some kind of abort option, otherwise the user is stuck for about 2 minutes while it cycles thru.
- - [ ] egg_audio_get_playhead: Adjust per driver.
- - [ ] synth_set_playhead
- [ ] Raspberry Pi 1 (Broadcom video)
- [ ] MS Windows
- [ ] MacOS
- [ ] All structs declared to the public API must be the same size in wasm and native. Can we assert that somehow?
- [ ] Storage access controls.
- [ ] TOUCH events for native. What would that take? Maybe acquire some tablet thingy and try it out?
- [x] Looks like egg_log %p is popping 8 bytes even if sourced from Wasm, where that's always 4 bytes. ...unable to reproduce. added lights-on regression
- [x] eggdev: tmp-egg-rom.s lingers sometimes, i think only after errors
- - Repro: ../../egg/out/tools/eggdev bundle -ono/such/directory --rom=out/lightson.egg
- [x] video api: Rect, line, and trig are subject to global alpha but not tint. That's how I defined it and it works, but it's inconsistent. Can we apply tint too?
- - ...Changed native and web: Tint and alpha now both apply to every operation.
- [ ] xegl cursor lock on startup seems to be reporting absolute when we start in fullscreen? Try eggsamples/shmup.egg, it's haywire in fullscreen.
- [x] eggdev: Better reporting for string ID conflict. We get a generic: !!! Resource ID conflict: (3:339:3), expecting rid 4.
- [ ] Notify of motion outside window. Platform might only be able to supply enter/exit, but that's enough. (see lingerful cursor in eggsamples/hardboiled).
- [ ] Tooling for synth playback and sound effects editing.
- [x] Compiling metadata: Fail if key but no value present. eg I said "framebuffer 640x630" instead of "framebuffer=640x360" and got no useful diagnostics.
- [ ] eggdev bundle html: Minify platform Javascript.
- [x] Port Doom: https://github.com/id-Software/DOOM ...gave up, too much effort with libc (and didn't even get to audio...)
- [ ] egg_gles2_wrapper.xc:glGetVertexAttribPointerv: What does this function do?
- [ ] glReadPixels et al: Could our buffer measurement come up short, if the user sets eg GL_UNPACK_ALIGNMENT to something wacky?
- [ ] What would it take to provide a raw PCM out interface? Easy for native, obviously, but under WebAudio....?
- - If we had that, it would potentially save a ton of effort in porting existing games to Egg.
- - And would enable some really interesting use cases, like running console emulators.

## Major changes since 202405 (which is a separate repo)

- No more Javascript support.
- No more networking.
- Drop QOI in favor of PNG.
- Larger resource size limit (~500MB from ~1MB).
- Special format for archives embedded in HTML.
- Direct OpenGL, in addition to the bespoke video API.
- No soft render.
- Dev tools all packed in one executable.
- Pointer warping.
- Move audio playhead.
- Map joysticks to standard, platform-side.
