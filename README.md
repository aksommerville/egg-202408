# Egg

```
sudo apt install clang wabt
```

## TODO

- [ ] Raspberry Pi 1 (Broadcom video)
- [ ] MS Windows
- [ ] MacOS
- [ ] All structs declared to the public API must be the same size in wasm and native. Can we assert that somehow?
- [ ] TOUCH events for native. What would that take? Maybe acquire some tablet thingy and try it out?
- [ ] Web: Pointer lock is broken, see eggsamples/shmup
- [ ] Web: Pitch bend is broken, painfully obvious in Courage Quest (eggsamples/arrautza)
- [ ] Editor
- - [ ] SfgEditor: Option to play audio server-side, and toggle between server and client on the fly.
- - [ ] Synth receiver (I want to run midevil against this server).
- - [ ] SongEditor: Playback.
- - - Partially working.
- - - [ ] Take configuration from command line (driver setup, and path to drums)
- - - [ ] HTTP call to start and stop the audio context. Don't start it by default. (seeing contention between server and browser, fighting for ALSA's attention)
- - - [ ] Can we provide something for adjusting playhead on the fly?
- - - [ ] Edit levels and program assignments on the fly? Would be great to not interrupt the song on a change.
- - [ ] SongEditor: By default, show a much simpler view. Just channel headers and tempo, I think.
- [ ] Choose a way to indicate the loop point in MIDI files. I believe both synthesizers are already ready for it.
- [ ] egg_gles2_wrapper.xc:glGetVertexAttribPointerv: What does this function do?
- [ ] glReadPixels et al: Could our buffer measurement come up short, if the user sets eg GL_UNPACK_ALIGNMENT to something wacky?
