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
- What would it take to support MP3 as a song format?
- - It's no longer under patent, but the spec is paywalled. Should I pay the $200 to be able to know conclusively that we can't support it? Or just assume.
- - We can come back to this later.

```
sudo apt install clang wabt
```

## TODO

- [ ] Web runtime
- - [ ] Pointer Capture
- - [ ] Reject event enablement if we know it's not supported. eg Gamepad and Accelerometer are easy to know. Does anything tell us about touch or keyboard?
- - [ ] Audio playhead
- - [ ] Audio: Shut down faster, at least drop events that haven't started yet.
- - [ ] Audio: egg_audio_event
- - [ ] incfg: First, is it worth the effort? How much coverage does Standard Mapping give us? And then.... how?
- [ ] Native runtime for Linux
- - [x] Event dispatch
- - [x] Clock
- - [x] Input digestion. eg receiving mouse or keyboard via evdev.
- - [x] Fake standard mapping.
- - [x] Live input config. Trigger with --configure-input
- - [ ] Invoke incfg via client too.
- - [ ] Pointer Capture
- - [ ] egg_audio_get_playhead: Adjust per driver.
- - [ ] synth_set_playhead
- [ ] Raspberry Pi
- [ ] MS Windows
- [ ] MacOS
- [ ] Some amount of libc for games. At least need memory and math functions.
- - It's OK (advisable!) to do that purely client-side, but we should supply a library or something.
- [ ] clang is inserting calls to memcpy. Can avoid by declaring arrays 'static const'. Or I guess by including libc. But can we tell it not to? I did say "-nostdlib"!
- [ ] All structs declared to the public API must be the same size in wasm and native. Can we assert that somehow?
- [x] Since we're allowing line gradients, can we also do rect gradients? No need, if full OpenGL works out. ...see below, raw triangle strip would cover it.
- [ ] Storage access controls.
- [ ] Remove the "fb" hooks from hostio_video. No harm if they exist, but they're misleading and dead weight, we'll never use.
- [ ] Revisit exposing GLES2/WebGL directly to the game.
- - [ ] If it's a no, add a few more render ops:
- - - [ ] Raw triangle strip.
- - - [ ] Decal with arbitrary scale and rotation.
- - - [ ] Textured triangle strip?
- [ ] TOUCH events for native. What would that take? Maybe acquire some tablet thingy and try it out?
- - https://shop.puri.sm/shop/librem-11/ oooh that looks cool. $1k, don't rush it.
- - https://pine64.com/product/pinetab2-10-1-8gb-128gb-linux-tablet-with-detached-backlit-keyboard/ about $200, looks low quality
- - https://raspad.com/products/raspadv3?variant=41161312764080 $300 project kit, pi+touchscreen. looks adorable
- [ ] When we fake KEY events via hostio_input, should we also attempt TEXT mapping? Determining the user's preferred keyboard layout sounds like a can of rotten worms.
