/* Audio.js
 */
 
export class Audio {
  constructor(egg) {
    this.egg = egg;
  }
  
  /*--------------------------------- Public API entry points -----------------------------*/
  
  egg_audio_play_song(qual, rid, force, repeat) {
    console.log(`TODO egg_audio_play_song`, { qual, rid, force, repeat });
  }
  
  egg_audio_play_sound(qual, rid, trim, pan) {
    console.log(`TODO egg_audio_play_sound`, { qual, rid, trim, pan });
  }
  
  egg_audio_event(chid, opcode, a, b) {
    console.log(`TODO egg_audio_event`, { chid, opcode, a, b });
  }
  
  egg_audio_get_playhead() {
    console.log(`TODO egg_audio_get_playhead`);
    return 0;
  }
  
  egg_audio_set_playhead(beat) {
    console.log(`TODO egg_audio_set_playhead`, { beat });
  }
}
