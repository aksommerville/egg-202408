# Egg

```
sudo apt install clang wabt
```

## TODO

- [ ] Web runtime
- - [ ] Reject event enablement if we know it's not supported. eg Gamepad and Accelerometer are easy to know. Does anything tell us about touch or keyboard?
- - [ ] Audio playhead
- - [ ] Audio: Shut down faster, at least drop events that haven't started yet.
- - [ ] Audio: egg_audio_event
- - [ ] Audio: I'm not hearing drums in music (see hardboiled)
- - [ ] incfg: First, is it worth the effort? How much coverage does Standard Mapping give us? And then.... how?
- - [ ] Send RAW events from gamepad. Whether standard mapping or not.
- [ ] Native runtime for Linux
- - [ ] Invoke incfg via client too.
- - - Think this thru. We'll need to add some kind of abort option, otherwise the user is stuck for about 2 minutes while it cycles thru.
- - [ ] egg_audio_get_playhead: Adjust per driver.
- - [ ] synth_set_playhead
- - [ ] synth wrap playhead
- [ ] Raspberry Pi 1 (Broadcom video)
- [ ] MS Windows
- [ ] MacOS
- [ ] All structs declared to the public API must be the same size in wasm and native. Can we assert that somehow?
- [ ] Storage access controls.
- [ ] TOUCH events for native. What would that take? Maybe acquire some tablet thingy and try it out?
- [ ] Looks like egg_log %p is popping 8 bytes even if sourced from Wasm, where that's always 4 bytes.
- [ ] eggdev: tmp-egg-rom.s lingers sometimes, i think only after errors
- [ ] video api: Rect, line, and trig are subject to global alpha but not tint. That's how I defined it and it works, but it's inconsistent. Can we apply tint too?
- [ ] xegl cursor lock on startup seems to be reporting absolute when we start in fullscreen? Try eggsamples/shmup.egg, it's haywire in fullscreen.
- [ ] eggdev: Better reporting for string ID conflict. We get a generic: !!! Resource ID conflict: (3:339:3), expecting rid 4.
- [ ] Notify of motion outside window. Platform might only be able to supply enter/exit, but that's enough. (see lingerful cursor in eggsamples/hardboiled).
- [ ] Tooling for synth playback and sound effects editing.
- [ ] Compiling metadata: Fail if key but no value present. eg I said "framebuffer 640x630" instead of "framebuffer=640x360" and got no useful diagnostics.
- [ ] eggdev bundle html: Minify platform Javascript.
- [ ] Port Doom: https://github.com/id-Software/DOOM
- [ ] egg_gles2_wrapper.xc:glGetVertexAttribPointerv: What does this function do?
- [ ] glReadPixels et al: Could our buffer measurement come up short, if the user sets eg GL_UNPACK_ALIGNMENT to something wacky?

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
