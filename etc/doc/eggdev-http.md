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
 - `POST /api/song/adjust`. Change tempo or channel headers of the running song.
 - - Request body is 42 bytes binary, a beeeeeP song without any events:
 - - - "\xbe\xee\xeeP", u16:tempo, u16:dummy, u16:loopp, 8 * (u8:pid, u8:volume, u8:pan, u8:dummy)
 - - - (loopp) is normalized 0..0xffff, it doesn't count the header like we do in the real beeeeeP files.
 - - Empty request body is legal, and no changes will be made.
 - - Responds with the updated state, same shape as request and normally identical to request.
 - `POST /api/audio` req body {driver,rate,chanc,buffer,device}, driver="none" to turn off, or "" to use a default.
 
WebSocket:
 - `/ws/midi`: Send realtime MIDI events to the server.
 - - Beware that due to audio driver buffering, timing is not reliable enough for playback.
 - - Should be OK for noodling tho.
 - `/ws/playhead`: Receive regular updates on the song's playhead.
 - - Binary packets contain a big-endian integer in 0..65535, where 65535 is end of song.
 - - You may also write to this socket, to move the playhead.
