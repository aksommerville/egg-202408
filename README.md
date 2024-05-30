# Egg - Beaten

2024-05-07
Having got Egg up to a point where it basically works, I'm seeing some key flaws:
- Joysticks should report Standard Mapping and fake it as needed. (could fix this in Egg).
- Maintaining both JS and Wasm runtimes is painful.
- JS runtime for Web opens a gaping security hole: Devs can access browser APIs freely. Not sure I could prevent them.
- Our 3 external dependencies (QuickJS, wasm-micro-runtime, and libcurl) add about 5 MB to every build. Usually more than the actual runtime and game.
- QOI does not compress as well as PNG. I'd really prefer to use PNG.
- Networking vastly expands our vulnerability surface, and let's be honest, I'm not going to police it fully over time.
- The 1 MB/resource limit hasn't bit me yet, but it's likely to.
- Packing ROM files doesn't let the developer insert his own processing logic, and there's no obvious place for that to fit in.
- - Not going to address this as such. We'll recommend preprocessing before pack. Only standard resource types get the special privilege of compiling during pack.
- When packed into HTML, the whole archive is base64 encoded. Can we get smarter about that? Some more digestible text format for archives?
- Reconsider exposing GL generically. But failing that, do make a richer set of render calls.

So some important differences here:
- No Network API.
- No Javascript runtime.
- Resources basically unlimited size (~500MB).
- Smarter archive format for embedded HTML.
- No soft render. It causes problems already, and if we enable direct OpenGL, will be completely infeasible.

And some minor ones:
- Slightly cleaner build system.
- Single dev tool.
- Recommending clang over WASI, I didn't even realize that was an option, ha ha.
- Some new video API.
- Pointer warping.
- Move audio playhead.
- PNG available at runtime.
- Map joysticks platform-side.

And some major outstanding questions:
- GLES2/WebGL exposed directly?
- Can we get a GLES2 context in Mac and Windows? I've had trouble with this in the past, on Windows especially.

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
- [ ] Raspberry Pi.
- [ ] MS Windows
- [ ] MacOS
- [x] Some amount of libc for games. At least need memory and math functions.
- - It's OK (advisable!) to do that purely client-side, but we should supply a library or something.
- [ ] clang is inserting calls to memcpy. Can avoid by declaring arrays 'static const'. Or I guess by including libc. But can we tell it not to? I did say "-nostdlib"!
- [ ] All structs declared to the public API must be the same size in wasm and native. Can we assert that somehow?
- [ ] Storage access controls.
- [ ] Revisit exposing GLES2/WebGL directly to the game.
- - Exposing GL directly would be inconsistently low-level compared to the rest of the public api.
- - That's not a reason to reject it, but I think even if we implement, we should keep the high-level render api too.
- [ ] TOUCH events for native. What would that take? Maybe acquire some tablet thingy and try it out?
- [ ] Looks like egg_log %p is popping 8 bytes even if sourced from Wasm, where that's always 4 bytes.
- [ ] eggdev: tmp-egg-rom.s lingers sometimes, i think only after errors
- [ ] video api: Rect, line, and trig are subject to global alpha but not tint. That's how I defined it and it works, but it's inconsistent. Can we apply tint too?
- [x] Declare USB HID keysyms in a header
- [ ] xegl cursor lock on startup seems to be reporting absolute when we start in fullscreen? Try eggsamples/shmup.egg, it's haywire in fullscreen.
- [ ] eggdev: Better reporting for string ID conflict. We get a generic: !!! Resource ID conflict: (3:339:3), expecting rid 4.
- [ ] Notify of motion outside window. Platform might only be able to supply enter/exit, but that's enough. (see lingerful cursor in eggsamples/hardboiled).
- [ ] Tooling for synth playback and sound effects editing.
- [ ] Compiling metadata: Fail if key but no value present. eg I said "framebuffer 640x630" instead of "framebuffer=640x360" and got no useful diagnostics.
