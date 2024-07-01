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
- - - This is a big deal. We'll need to bring in the web runtime's synthesizer somehow, and either modify it to play MIDI, or also implement the song compiler.
- [ ] egg_gles2_wrapper.xc:glGetVertexAttribPointerv: What does this function do?
- [ ] glReadPixels et al: Could our buffer measurement come up short, if the user sets eg GL_UNPACK_ALIGNMENT to something wacky?
