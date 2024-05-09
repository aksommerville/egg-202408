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

## TODO

- [x] eggdev
- - [x] pack: Extension points. ...only needed `--types=PATH`, i think.
- - [x] bundle
- - [x] unbundle ...ha ha i can't believe it works :D
- - [x] serve
- - - [x] htdocs etc
- - - [x] JSON listing of available ROMs.
- [x] Define API.
- [x] Split up and generalize makefiles.
- [x] eggdev unbundle for HTML (text format)
- [x] Can we use PNG as favicon? That will make the difference, whether we allow multiple image formats. ...YES PNG is fine.
- [ ] Web runtime
- [ ] Native runtime for Linux
- [ ] Raspberry Pi
- [ ] MS Windows
- [ ] MacOS
- [ ] eggdev pack: Validate wasm imports and exports, is that feasible?
- [ ] eggdev bundle with native code: Automatically strip wasm resource, so we can bundle from the regular egg file
- [ ] Some amount of libc for games. At least need memory and math functions.
- - It's OK (advisable!) to do that purely client-side, but we should supply a library or something.
