/* Input.js
 */
 
export class Input {
  constructor(egg) {
    this.egg = egg;
  }
  
  /*--------------------------- Public API entry points -----------------------------------*/
  
  egg_event_get(v, a) {
    console.log(`TODO egg_event_get`, { v, a });
    return 0;
  }
  
  egg_event_enable(type, enable) {
    console.log(`TODO egg_event_enable`, { type, enable });
    return 0;
  }
  
  egg_show_cursor(show) {
    console.log(`TODO egg_show_cursor`, { show });
  }
  
  egg_lock_cursor(lock) {
    console.log(`TODO egg_lock_cursor`, { lock });
    return 0;
  }
  
  egg_joystick_devid_by_index(p) {
    console.log(`TODO egg_joystick_devid_by_index`, { p });
    return 0;
  }
  
  egg_joystick_get_ids(vid, pid, ver, devid) {
    console.log(`TODO egg_joystick_get_ids`, { vid, pid, ver, devid });
  }
  
  egg_joystick_get_name(dst, dsta, devid) {
    console.log(`TODO egg_joystick_get_name`, { dst, dsta, devid });
    return 0;
  }
  
  egg_joystick_for_each_button(devid, cb, ctx) {
    console.log(`TODO egg_joystick_for_each_button`, { devid, cb, ctx });
    return 0;
  }
}
