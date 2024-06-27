# Egg

```
sudo apt install clang wabt
```

## TODO

- [ ] Web runtime
- - [ ] incfg: First, is it worth the effort? How much coverage does Standard Mapping give us? And then.... how?
- [ ] Native runtime for Linux
- - [ ] Invoke incfg via client too.
- - - Think this thru. We'll need to add some kind of abort option, otherwise the user is stuck for about 2 minutes while it cycles thru.
- - [ ] Config file.
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

- [x] What would it take to provide a raw PCM out interface? Easy for native, obviously, but under WebAudio....?
- - ...FINAL ANSWER: NO. Oddly, the web side is easy. But we'd need multi-thread support out of WAMR, and I can't get that running.
- - If we had that, it would potentially save a ton of effort in porting existing games to Egg.
- - And would enable some really interesting use cases, like running console emulators.
- - Looks possible, see this excellent SO answer:
- - - https://stackoverflow.com/questions/60921018/web-audio-api-efficiently-play-a-pcm-stream
- - [x] Do an external POC
- - - Works, it's easy. See etc/notes/20240626-webpcm.html
- - [x] *** Does wasm-micro-runtime support calling in from a separate thread? *** It's a deal breaker if not. ...DEAL BROKEN
- - - See wasm_runtime_spawn_exec_env, it is supported but I'm not sure whether this is using some WASI extension?
- - - Do a POC before committing to it. Be sure it's realistic, the details might matter:
- - - - Call a wasm function that was delivered to us as an anonymous callback.
- - - - See both threads overlap.
- - - - See pthread_mutex prevent that overlap.
- - - - Can we prevent the callback thread from calling Egg APIs? If not, a malicious developer could... I don't know, but something horrible.
- - - Tried a POC but couldn't even get it to load. Looks like wamr expected something out of globals that clang didn't produce (TLS stats or something?)
- - If we're going to do this, we need to keep it single-threaded. I don't think that's acceptable.

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
