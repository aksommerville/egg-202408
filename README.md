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
- [ ] xegl cursor lock on startup seems to be reporting absolute when we start in fullscreen? Try eggsamples/shmup.egg, it's haywire in fullscreen.
- [ ] Notify of motion outside window. Platform might only be able to supply enter/exit, but that's enough. (see lingerful cursor in eggsamples/hardboiled).
- [ ] Editor
- - [ ] SfgEditor: Option to play audio server-side, and toggle between server and client on the fly.
- - [ ] Synth receiver (I want to run midevil against this server).
- - [x] Is it crazy to add a MIDI editor? Not necessarily a full sequencer, but something where you can play songs and tweak the program selection and levels.
- - [ ] SongEditor: Playback.
- - - This is a big deal. We'll need to bring in the web runtime's synthesizer somehow, and either modify it to play MIDI, or also implement the song compiler.
- - [x] More fs ops: Rename file, rename directory, move file, make directory, remove directory. Low priority; user can always do this odd stuff in her filesystem directly.
- - - Not providing UI for directories. They get created implicitly as needed, and let the user figure it out from there.
- - [ ] StringEditor: Create new string
- - [x] Shift-click or something, to select an editor manually.
- - [ ] Are we allowing strings to split across multiple files? We must. eg "en-dialogue" and "en-menus", and those names would be discarded.
- - - StringEditor will need to be smart about that, and not compare like "fr-Apples" to "en-Oranges".
- - [ ] Global op to detect missing strings.
- [ ] eggdev bundle html: Minify platform Javascript.
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
