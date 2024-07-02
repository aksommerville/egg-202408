# Eggdev HTTP Server

When you run `eggdev serve`, we run an HTTP server with a few bells and whistles.
This document is of interest if you're writing a replacement for our editor web app.
Otherwise, `eggdev --help=serve` should tell you everything you need to know.
    
Static files:
 - `--htdocs=PATH`: GET only, and no prefix.
 - `--override=PATH`: GET only, and no prefix. Any file we find here, we serve it instead of the one in `htdocs`.
 - `--runtime=PATH`: GET only, prefix `/rt/`. Location of the Egg runtime for launching games.
 - `--data=PATH`: GET/PUT/DELETE, prefix `/res/`.
 - Loose ROMs on the command line match by basename only.
 
REST hooks:
 - `GET /api/roms`, JSON array of string, basenames of all the ROM files.
 - `GET /api/res-all`, nested JSON objects containing the entire `--data` set.
 - - Each node is either `{name:string,files:NODE[]}` or `{name:string,serial:string}`, serial is base64 encoded.
 - `POST /api/song`, request body is a MIDI file or empty. Stop any current playback and begin playing this song.
 
WebSocket:
 - Any binary packet is presumed to be MIDI events; we deliver directly to the synthesizer if present.
 - - I wrote this behavior, but now I've decided not to use it. Leaving it in, in case it becomes useful, maybe for testing instruments?
 - - To play a song, prefer `POST /api/song`.
