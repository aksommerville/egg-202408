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
- When packed into HTML, the whole archive is base64 encoded. Can we get smarter about that? Some more digestible text format for archives?
- Reconsider exposing GL generically. But failing that, do make a richer set of render calls.

## TODO

- [x] eggdev
- - [x] pack: Extension points. ...only needed `--types=PATH`, i think.
- - [x] bundle
- - [x] unbundle ...ha ha i can't believe it works :D
- - [x] serve
- - - [x] htdocs etc
- - - [x] JSON listing of available ROMs.
- [x] Define API.
- [ ] Split up and generalize makefiles.
- [ ] Web runtime
- [ ] Native runtime for Linux
- [ ] Raspberry Pi
- [ ] MS Windows
- [ ] MacOS
