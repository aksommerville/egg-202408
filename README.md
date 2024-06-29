# Egg

```
sudo apt install clang wabt
```

## TODO

- [ ] Native runtime for Linux
- - [ ] Config file.
- - [x] incfg: Don't ask about buttons if a similar one was already skipped: L1=>R1, L1=>L2, L2=>R2, LP=>RP, DPADY=>DPADX, LX=>LY, RX=>RY, LX=>RX?
- - [x] Screencap. No need to implement for web -- wrappers can implement it if they want.
- - [ ] Save state. I think we only need song id, playhead, and wasm ram image. Won't work for true-native. Also connected joystick IDs, so they can report disconnection on restart.
- - - I think don't do this for web? At least not initially, because it's complicated. eg once serialized, what do you do with it?
- - [x] Before screencap and save state, we need configurable stateless inputs consumable by the platform.
- - [x] Hard pause.
- [ ] Raspberry Pi 1 (Broadcom video)
- [ ] MS Windows
- [ ] MacOS
- [ ] All structs declared to the public API must be the same size in wasm and native. Can we assert that somehow?
- [ ] TOUCH events for native. What would that take? Maybe acquire some tablet thingy and try it out?
- [ ] xegl cursor lock on startup seems to be reporting absolute when we start in fullscreen? Try eggsamples/shmup.egg, it's haywire in fullscreen.
- [ ] Editor
- - [ ] SfgEditor: Option to play audio server-side, and toggle between server and client on the fly.
- - [ ] Synth receiver (I want to run midevil against this server).
- - [ ] SongEditor: Playback.
- - - This is a big deal. We'll need to bring in the web runtime's synthesizer somehow, and either modify it to play MIDI, or also implement the song compiler.
- [ ] egg_gles2_wrapper.xc:glGetVertexAttribPointerv: What does this function do?
- [ ] glReadPixels et al: Could our buffer measurement come up short, if the user sets eg GL_UNPACK_ALIGNMENT to something wacky?
