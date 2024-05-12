/* Input.js
 */
 
export class Input {
  constructor(egg) {
    this.egg = egg;
    
    this.evtq = []; // Each member is an array of 2..5 ints.
    this.evtmask = 
      (1 << Input.EGG_EVENT_JOY) |
      (1 << Input.EGG_EVENT_KEY) |
      (1 << Input.EGG_EVENT_TOUCH)|
      // MMOTION, MBUTTON, MWHEEL, TEXT, ACCEL: off by default
    0;
    // TODO Turn off JOY, KEY, and TOUCH if we can tell they aren't supported.
    
    this.cursorVisible = false;
    this.cursorDesired = true; // Should be visible when enabled. (egg_show_cursor())
    this.mouseEventListener = null;
    this.mouseButtonsDown = new Set();
    this.mouseX = 0;
    this.mouseY = 0;
    this.mouseLocked = false;
    
    this.keyListener = e => this.onKey(e);
    window.addEventListener("keydown", this.keyListener);
    window.addEventListener("keyup", this.keyListener);
    
    this.gamepads = []; // sparse
    this.gamepadListener = e => this.onGamepadConnection(e);
    window.addEventListener("gamepadconnected", this.gamepadListener);
    window.addEventListener("gamepaddisconnected", this.gamepadListener);
    
    this.touchListener = null;
    
    this.accel = null;
    this.accelListener = null;
    
    this.canvasChanged();
  }
  
  canvasChanged() {
    const canvas = this.egg.canvas;
    if (this.touchListener && (this.canvas !== canvas)) {
      this.canvas.removeEventListener("touchstart", this.touchListener);
      this.canvas.removeEventListener("touchend", this.touchListener);
      this.canvas.removeEventListener("touchcancel", this.touchListener);
      this.canvas.removeEventListener("touchmove", this.touchListener);
      this.touchListener = null;
    }
    if (canvas) {
      this.canvas = canvas;
      if (!this.touchListener) {
        this.touchListener = e => this.onTouch(e);
        canvas.addEventListener("touchstart", this.touchListener);
        canvas.addEventListener("touchend", this.touchListener);
        canvas.addEventListener("touchcancel", this.touchListener);
        canvas.addEventListener("touchmove", this.touchListener);
      }
    }
  }
  
  detach() {
    this._unlistenMouse();
    if (this.keyListener) {
      window.removeEventListener("keydown", this.keyListener);
      window.removeEventListener("keyup", this.keyListener);
      this.keyListener = null;
    }
    if (this.gamepadListener) {
      window.removeEventListener("gamepadconnected", this.gamepadListener);
      window.removeEventListener("gamepaddisconnected", this.gamepadListener);
      this.gamepadListener = null;
    }
    if (this.touchListener) {
      this.canvas.removeEventListener("touchstart", this.touchListener);
      this.canvas.removeEventListener("touchend", this.touchListener);
      this.canvas.removeEventListener("touchcancel", this.touchListener);
      this.canvas.removeEventListener("touchmove", this.touchListener);
      this.touchListener = null;
    }
    if (this.accel) {
      if (this.accelListener) {
        this.accel.removeEventListener("reading", this.accelListener);
        this.accelListener = null;
      }
      this.accel.stop();
    }
  }
  
  update() {
    this._updateGamepads();
  }
  
  // (v) is an array of 2..5 integers; see egg_input.h
  pushEvent(v) {
    if (!(this.evtmask & (1 << v[0]))) return;
    this.evtq.push(v);
  }
  
  reset() {
    this.evtq = [];
    const initialMask =
      (1 << Input.EGG_EVENT_JOY) |
      (1 << Input.EGG_EVENT_KEY) |
      (1 << Input.EGG_EVENT_TOUCH)|
    0;
    if (initialMask !== this.evtmask) {
      for (let i=0; i<30; i++) {
        if ((initialMask & (1 << i)) !== (this.evtmask & (1 << i))) {
          this.event_enable(i, initialMask & (1 << i));
        }
      }
    }
  }
  
  /* Touch.
   ********************************************************************/
   
  onTouch(e) {
    if (!e.changedTouches) return;
    if (!this.canvas) return;
    const bounds = this.canvas.getBoundingClientRect();
    let state;
    switch (e.type) {
      case "touchstart": state = 1; break;
      case "touchend":
      case "touchcancel": state = 0; break;
      case "touchmove": state = 2; break;
    }
    for (const touch of e.changedTouches) {
      const x = ((touch.clientX - bounds.x) * this.canvas.width) / bounds.width;
      const y = ((touch.clientY - bounds.y) * this.canvas.height) / bounds.height;
      this.pushEvent([Input.EGG_EVENT_TOUCH, touch.identifier, state, x, y]);
    }
  }
  
  /* Gamepad.
   * We will use (gamepad.index+1) as (devid) for reporting to the client.
   ******************************************************************************/
   
  _updateGamepads() {
    if (!this.window.navigator.getGamepads) return;
    for (const gamepad of this.window.navigator.getGamepads()) {
      if (!gamepad) continue;
      const local = this.gamepads[gamepad.index];
      if (!local) continue;
      
      for (let i=local.axes.length; i-->0; ) {
        const pv = local.axes[i];
        const nx = gamepad.axes[i] ? Math.floor(gamepad.axes[i] * 127) : 0;
        if (pv === nx) continue;
        local.axes[i] = nx;
        this.pushEvent([Input.EGG_EVENT_INPUT, local.devid, local.axisBase + i, nx]);
      }
      
      for (let i=local.buttons.length; i-->0; ) {
        const pv = local.buttons[i];
        const nx = gamepad.buttons[i].value;
        if (pv === nx) continue;
        local.buttons[i] = nx;
        this.pushEvent([Input.EGG_EVENT_INPUT, local.devid, local.buttonBase + i, nx]);
      }
    }
  }
  
  onGamepadConnection(e) {
    switch (e.type) {
    
      case "gamepadconnected": {
          let axisBase, buttonBase;
          if (e.gamepad.mapping === "standard") {
            axisBase = 0x80;
            buttonBase = 0x40;
          } else {
            axisBase = 0x100;
            buttonBase = 0x200;
          }
          this.gamepads[e.gamepad.index] = {
            devid: e.gamepad.index + 1,
            index: e.gamepad.index,
            id: e.gamepad.id,
            axes: (e.gamepad.axes || []).map(v => v),
            buttons: (e.gamepad.buttons || []).map(v => 0),
            mapping: e.gamepad.mapping,
            axisBase,
            buttonBase,
          };
          let mapping = 0; // RAW
          switch (e.gamepad.mapping) {
            case "standard": mapping = 1; break;
          }
          this.pushEvent([Input.EGG_EVENT_JOY, e.gamepad.index + 1, 0, 1]);
        } break;
        
      case "gamepaddisconnected": {
          const local = this.gamepads[e.gamepad.index];
          if (local) {
            delete this.gamepads[e.gamepad.index];
            this.pushEvent([Input.EGG_EVENT_DISCONNECT, local.devid, 0, 0]);
          }
        } break;
    }
  }
  
  /* Mouse.
   ********************************************************************************/
  
  _checkCursorVisibility(show) {
    const enableEvents = show;
    if (this.cursorDesired) show = !!show;
    else show = false;
    if (show !== this.cursorVisible) {
      this.cursorVisible = show;
      if (this.canvas) {
        if (show) {
          this.canvas.style.cursor = "pointer";
        } else {
          this.canvas.style.cursor = "none";
        }
      }
    }
    if (enableEvents) {
      if (!this.mouseListener) {
        this._listenMouse();
      }
    } else {
      if (this.mouseListener) {
        this._unlistenMouse();
      }
    }
  }
  
  _listenMouse() {
    if (this.mouseEventListener) return;
    this.mouseEventListener = e => this.onMouseEvent(e);
    window.addEventListener("mousewheel", this.mouseEventListener);
    window.addEventListener("mousemove", this.mouseEventListener);
    window.addEventListener("mouseup", this.mouseEventListener);
    if (this.canvas) {
      this.canvas.addEventListener("mousedown", this.mouseEventListener);
      this.canvas.addEventListener("contextmenu", this.mouseEventListener);
    }
  }
  
  _unlistenMouse() {
    if (this.mouseEventListener) {
      window.removeEventListener("mousewheel", this.mouseEventListener);
      window.removeEventListener("mousemove", this.mouseEventListener);
      window.removeEventListener("mouseup", this.mouseEventListener);
      if (this.canvas) {
        this.canvas.removeEventListener("mousedown", this.mouseEventListener);
        this.canvas.removeEventListener("contextmenu", this.mouseEventListener);
      }
      this.mouseEventListener = null;
    }
  }
  
  onMouseEvent(e) {
    if (!this.canvas) return;
    const bounds = this.canvas.getBoundingClientRect();
    const x = Math.floor(((e.x - bounds.x) * this.canvas.width) / bounds.width);
    const y = Math.floor(((e.y - bounds.y) * this.canvas.height) / bounds.height);
    switch (e.type) {
      case "mousemove": {
          if (this.mouseLocked) {
            if (e.movementX || e.movementY) {
              this.pushEvent([Input.EGG_EVENT_MMOTION, e.movementX, e.movementY]);
            }
          } else {
            if ((x === this.mouseX) && (y === this.mouseY)) return;
            this.mouseX = x;
            this.mouseY = y;
            this.pushEvent([Input.EGG_EVENT_MMOTION, x, y]);
          }
        } break;
      case "mousedown": {
          if (e.target !== this.canvas) return;
          let button;
          switch (e.button) {
            case 0: button = Input.EGG_MBUTTON_LEFT; break;
            case 1: button = Input.EGG_MBUTTON_MIDDLE; break;
            case 2: button = Input.EGG_MBUTTON_RIGHT; break;
            default: button = e.button;
          }
          if (this.mouseButtonsDown.has(button)) return;
          this.mouseButtonsDown.add(button);
          e.preventDefault();
          this.pushEvent([Input.EGG_EVENT_MBUTTON, x, y, button, 1]);
        } break;
      case "mouseup": {
          let button;
          switch (e.button) {
            case 0: button = Input.EGG_MBUTTON_LEFT; break;
            case 1: button = Input.EGG_MBUTTON_MIDDLE; break;
            case 2: button = Input.EGG_MBUTTON_RIGHT; break;
            default: button = e.button;
          }
          if (!this.mouseButtonsDown.has(button)) return;
          this.mouseButtonsDown.delete(button);
          this.pushEvent([Input.EGG_EVENT_MBUTTON, x, y, button, 0]);
        } break;
      case "contextmenu": e.preventDefault(); break;
      case "mousewheel": {
          if ((x < 0) || (y < 0) || (x >= bounds.width) || (y >= bounds.height)) return;
          let dx = e.deltaX, dy = e.deltaY;
          if (e.wheelDeltaX) dx /= Math.abs(e.wheelDeltaX);
          if (e.wheelDeltaY) dy /= Math.abs(e.wheelDeltaY);
          if (e.wheelDelta && !e.wheelDeltaX && !e.wheelDeltaY) {
            dx /= Math.abs(e.wheelDelta);
            dy /= Math.abs(e.wheelDelta);
          }
          if (e.shiftKey) { // I like Shift+Wheel to mean X instead of Y.
            let tmp = dx;
            dx = dy;
            dy = tmp;
          }
          if (dx || dy) {
            this.pushEvent([Input.EGG_EVENT_MWHEEL, x, y, dx, dy]);
          }
          //event.preventDefault(); // It's installed as passive... why is that? (Chrome Linux)
        } break;
    }
  }
  
  /* Keyboard.
   **************************************************************************/
  
  onKey(e) {
    
    // Ignore all keyboard events when Alt or Ctrl is held.
    if (e.ctrlKey || e.altKey) {
      return;
    }
    
    // TODO We might be too heavy-handed with event suppression. Bear in mind that we are listening on Window.
    
    // If we recognize the key and user wants key events, pass it on and suppress it in browser.
    if (this.evtmask & (1 << Input.EGG_EVENT_KEY)) {
      const usage = this.hidUsageByKeyCode(e.code);
      if (usage) {
        const v = (e.type === "keyup") ? 0 : e.repeat ? 2 : 1;
        this.pushEvent([Input.EGG_EVENT_KEY, usage, v]);
        e.preventDefault();
        e.stopPropagation();
      }
    }
    
    // Likewise, if user wants text and it looks like text. (but not for "keyup" of course).
    if (e.type !== "keyup") {
      if (this.evtmask & (1 << Input.EGG_EVENT_TEXT)) {
        switch (e.key) {
          case "Backspace": this.pushEvent([Input.EGG_EVENT_TEXT, 0x08]); e.preventDefault(); e.stopPropagation(); break;
          case "Tab":       this.pushEvent([Input.EGG_EVENT_TEXT, 0x09]); e.preventDefault(); e.stopPropagation(); break;
          case "Enter":     this.pushEvent([Input.EGG_EVENT_TEXT, 0x0a]); e.preventDefault(); e.stopPropagation(); break;
          case "Escape":    this.pushEvent([Input.EGG_EVENT_TEXT, 0x1b]); e.preventDefault(); e.stopPropagation(); break;
          default: if (e.key?.length === 1) {
              this.pushEvent([Input.EGG_EVENT_TEXT, e.key.charCodeAt(0)]);
              e.preventDefault();
              e.stopPropagation();
            } break;
        }
      }
    }
  }
  
  hidUsageByKeyCode(code) {
    if (!code) return 0;
  
    // "KeyA".."KeyZ" => 0x04..0x1d
    if ((code.length === 4) && code.startsWith("Key")) {
      const ch = code.charCodeAt(3);
      if ((ch >= 0x41) && (ch <= 0x5a)) return 0x00070004 + ch - 0x41;
    }
    
    // "Digit1".."Digit9" => 0x1e..0x25, some jackass put "0" on the right side... why...
    if ((code.length === 6) && code.startsWith("Digit")) {
      const ch = code.charCodeAt(5);
      if ((ch >= 0x31) && (ch <= 0x39)) return 0x0007001e + ch - 0x31;
      if (ch === 0x30) return 0x00070027; // zero
    }
    
    // "F1".."F12" => 0x3a..0x45
    // "F13".."F24" => 0x68..0x73
    if (((code.length === 2) || (code.length === 3)) && (code[0] === 'F')) {
      const v = +code.substring(1);
      if ((v >= 1) && (v <= 12)) return 0x0007003a + v - 1;
      if ((v >= 13) && (v <= 24)) return 0x00070068 + v - 13;
    }
    
    // "Numpad1".."Numpad9" => 0x59..0x61, again with zero on top because Jesus hates me.
    if ((code.length === 7) && code.startsWith("Numpad")) {
      const v = +code[7];
      if ((v >= 1) && (v <= 9)) return 0x00070059 + v - 1;
      if (v === 0) return 0x00070062;
    }
    
    // And finally a not-too-crazy set of one-off names.
    switch (code) {
      case "Enter":          return 0x00070028;
      case "Escape":         return 0x00070029;
      case "Backspace":      return 0x0007002a;
      case "Tab":            return 0x0007002b;
      case "Space":          return 0x0007002c;
      case "Minus":          return 0x0007002d;
      case "Equal":          return 0x0007002e;
      case "BracketLeft":    return 0x0007002f;
      case "BracketRight":   return 0x00070039;
      case "Backslash":      return 0x00070031;
      case "Semicolon":      return 0x00070033;
      case "Quote":          return 0x00070034;
      case "Backquote":      return 0x00070035;
      case "Comma":          return 0x00070036;
      case "Period":         return 0x00070037;
      case "Slash":          return 0x00070038;
      case "CapsLock":       return 0x00070039;
      case "Pause":          return 0x00070048;
      case "Insert":         return 0x00070049;
      case "Home":           return 0x0007004a;
      case "PageUp":         return 0x0007004b;
      case "Delete":         return 0x0007004c;
      case "PageDown":       return 0x0007004e;
      case "ArrowRight":     return 0x0007004f;
      case "ArrowLeft":      return 0x00070050;
      case "ArrowDown":      return 0x00070051;
      case "ArrowUp":        return 0x00070052;
      case "NumLock":        return 0x00070053;
      case "NumpadDivide":   return 0x00070054;
      case "NumpadMultiply": return 0x00070055;
      case "NumpadSubtract": return 0x00070056;
      case "NumpadAdd":      return 0x00070057;
      case "NumpadEnter":    return 0x00070058;
      case "NumpadDecimal":  return 0x00070063;
      case "ContextMenu":    return 0x00070076;
      case "ShiftLeft":      return 0x000700e1;
      case "ShiftRight":     return 0x000700e5;
      case "ControlLeft":    return 0x000700e0;
      case "ControlRight":   return 0x000700e4;
      case "AltLeft":        return 0x000700e2;
      case "AltRight":       return 0x000700e6;
    }
    return 0;
  }
  
  /* Accelerometer.
   **************************************************************************/
   
  accelerometerEnable() {
    this._accelerometerEnableInternal().then(() => {
      if (!this.accelListener) {
        this.accelListener = () => this.onAccelerometer();
        this.accel.addEventListener("reading", this.accelListener);
      }
      this.accel.start();
    }).catch(() => {});
  }
  
  _accelerometerEnableInternal() {
    if (this.accel) return Promise.resolve();
    if (!window.navigator.permissions) return Promise.reject();
    return window.navigator.permissions.query({ name: "accelerometer" }).then(result => {
      if (result.state === "denied") throw null;
      if (!this.accel) {
        this.accel = new Accelerometer({ referenceFrame: "device", frequency: 60 });
      }
    });
  }
  
  accelerometerDisable() {
    if (!this.accel) return;
    if (this.accelListener) {
      this.accel.removeEventListener("reading", this.accelListener);
      this.accelListener = null;
    }
    this.accel.stop();
  }
  
  onAccelerometer() {
    if (!this.accel) return;
    const x = ~~(this.accel.x * 65536.0);
    const y = ~~(this.accel.y * 65536.0);
    const z = ~~(this.accel.z * 65536.0);
    this.pushEvent([Input.EGG_EVENT_ACCEL, x, y, z]);
  }
  
  /*--------------------------- Public API entry points -----------------------------------*/
  
  egg_event_get(v, a) {
    const eventSizeWords = 5;//TODO Can we assert that this is sizeof(union egg_event)/sizeof(int)?
    const cpc = Math.min(a, this.evtq.length);
    if (cpc < 1) return 0;
    let dst = this.egg.exec.mem32;
    let dstp = v >> 2;
    for (let i=0; i<cpc; i++) {
      const e = this.evtq[i];
      for (let j=0; j<eventSizeWords; j++) {
        dst[dstp++] = e[j];
      }
    }
    this.evtq.splice(0, cpc);
    return cpc;
  }
  
  egg_event_enable(type, enable) {
    if (!type) return 0;
    const bit = 1 << type;
    if (enable) {
      if (this.evtmask & bit) return 1;
      this.evtmask |= bit;
    } else {
      if (!(this.evtmask & bit)) return 0;
      this.evtmask &= ~bit;
    }
    //TODO Reject changes if we can tell they aren't supported.
    switch (type) {
      case Input.EGG_EVENT_RAW: break;
      case Input.EGG_EVENT_JOY: break;
      case Input.EGG_EVENT_KEY: break;
      case Input.EGG_EVENT_TEXT: break;
      case Input.EGG_EVENT_MMOTION:
      case Input.EGG_EVENT_MBUTTON:
      case Input.EGG_EVENT_MWHEEL: {
          const mouseEvents = (1 << Input.EGG_EVENT_MMOTION) | (1 << Input.EGG_EVENT_MBUTTON) | (1 << Input.EGG_EVENT_MWHEEL);
          this._checkCursorVisibility(this.evtmask & mouseEvents);
        } break;
      case Input.EGG_EVENT_TOUCH: break;
      case Input.EGG_EVENT_ACCEL: {
          if (enable) {
            this.accelerometerEnable();
          } else {
            this.accelerometerDisable();
          }
        } break;
    }
    return enable ? 1 : 0;
  }
  
  egg_show_cursor(show) {
    this.cursorDesired = !!show;
    const mouseEvents = (1 << Input.EGG_EVENT_MMOTION) | (1 << Input.EGG_EVENT_MBUTTON) | (1 << Input.EGG_EVENT_MWHEEL);
    this._checkCursorVisibility(this.evtmask & mouseEvents);
  }
  
  egg_lock_cursor(lock) {
    console.log(`TODO egg_lock_cursor`, { lock });
    if (!this.canvas || !this.canvas.requestPointerLock) return 0;
    if (lock) {
      if (this.mouseLocked) return 1;
      this.mouseLocked = true;
      console.log(`requesting...`);
      this.canvas.requestPointerLock(/*{
        unadjustedMovement: true, // Not supported in Chrome/Linux, and the whole request gets rejected for it.
      }*/).then(rsp => {
        console.log(`...locked`, rsp);
      }).catch(e => {
        console.error(`...denied`, e);
        this.mouseLocked = false;
      });
    } else if (this.mouseLocked) {
      this.mouseLocked = false;
      document.exitPointerLock();
    }
    return 1;
  }
  
  egg_joystick_devid_by_index(p) {
    if (p < 0) return 0;
    for (let i=0; i<this.gamepads.length; i++) {
      if (!this.gamepads[i]) continue;
      if (!p--) return i + 1;
    }
    return 0;
  }
  
  egg_joystick_get_ids(vidp, pidp, verp, devid) {
    const local = this.gamepads[devid - 1];
    if (!local || (local.devid !== devid)) return;
    // Linux Chrome: Microsoft X-Box 360 pad (STANDARD GAMEPAD Vendor: 045e Product: 028e)
    // Not at all sure how standard that formatting is, but we don't have much else to go on...
    let vid=0, pid=0, version=0;
    let match;
    if (match = local.id.match(/Vendor: ([0-9a-fA-F]{4})/)) {
      vid = parseInt(match[1], 16);
    }
    if (match = local.id.match(/Product: ([0-9a-fA-F]{4})/)) {
      pid = parseInt(match[1], 16);
    }
    if (match = local.id.match(/Version: ([0-9a-fA-F]{4})/)) {
      // This one doesn't exist for me. And not sure whether they'd break it out as MAJOR.MINOR.REVISION.
      // 0xf000=MAJOR, 0x0f00=MINOR, 0x00ff=REVISION
      version = parseInt(match[1], 16);
    }
    if (vidp) this.egg.exec.mem32[vidp >> 2] = vid;
    if (pidp) this.egg.exec.mem32[pidp >> 2] = pid;
    if (verp) this.egg.exec.mem32[verp >> 2] = version;
  }
  
  egg_joystick_get_name(dst, dsta, devid) {
    const local = this.gamepads[devid - 1];
    if (!local || (local.devid !== devid)) return 0;
    const name = local.id.split('(')[0].trim();
    return name || local.id;
    return this.egg.exec.safeWrite(dst, dsta, name);
  }
  
  egg_joystick_for_each_button(devid, cb, ctx) {
    // int (*cb)(int btnid,int usage,int lo,int hi,int value,void *userdata)
    if (!(cb = this.egg.exec.fntab.get(cb))) return 0;

    const local = this.gamepads[devid - 1];
    if (!local || (local.devid !== devid)) return null;
    
    for (let i=0; i<local.axes.length; i++) {
      let hidusage = 0;
      if (local.mapping === "standard") {
        hidusage = Input.STANDARD_AXIS_USAGE[i] || 0;
      }
      const err = cb(local.axisBase + i, hidusage, -128, 127, 0, ctx);
      if (err) return err;
    }
    
    for (let i=0; i<local.buttons.length; i++) {
      let hidusage = 0x00090000 + i;
      if (local.mapping === "standard") {
        const alt = Input.STANDARD_BUTTON_USAGE[i];
        if (alt) hidusage = alt;
      }
      const err = cb(local.buttonBase + i, hidusage, 0, 1, 0, ctx);
    }
    
    return 0;
  }
}

Input.EGG_EVENT_JOY = 1;
Input.EGG_EVENT_KEY = 2;
Input.EGG_EVENT_TEXT = 3;
Input.EGG_EVENT_MMOTION = 4;
Input.EGG_EVENT_MBUTTON = 5;
Input.EGG_EVENT_MWHEEL = 6;
Input.EGG_EVENT_TOUCH = 7;
Input.EGG_EVENT_ACCEL = 8;
Input.EGG_EVENT_RAW = 9;

Input.EGG_JOYBTN_LX    = 0x40;
Input.EGG_JOYBTN_LY    = 0x41;
Input.EGG_JOYBTN_RX    = 0x42;
Input.EGG_JOYBTN_RY    = 0x43;
Input.EGG_JOYBTN_SOUTH = 0x80;
Input.EGG_JOYBTN_EAST  = 0x81;
Input.EGG_JOYBTN_WEST  = 0x82;
Input.EGG_JOYBTN_NORTH = 0x83;
Input.EGG_JOYBTN_L1    = 0x84;
Input.EGG_JOYBTN_R1    = 0x85;
Input.EGG_JOYBTN_L2    = 0x86;
Input.EGG_JOYBTN_R2    = 0x87;
Input.EGG_JOYBTN_AUX2  = 0x88;
Input.EGG_JOYBTN_AUX1  = 0x89;
Input.EGG_JOYBTN_LP    = 0x8a;
Input.EGG_JOYBTN_RP    = 0x8b;
Input.EGG_JOYBTN_UP    = 0x8c;
Input.EGG_JOYBTN_DOWN  = 0x8d;
Input.EGG_JOYBTN_LEFT  = 0x8e;
Input.EGG_JOYBTN_RIGHT = 0x8f;
Input.EGG_JOYBTN_AUX3  = 0x90;

Input.EGG_MBUTTON_LEFT = 1;
Input.EGG_MBUTTON_RIGHT = 2;
Input.EGG_MBUTTON_MIDDLE = 3;

Input.EGG_TOUCH_END = 0;
Input.EGG_TOUCH_BEGIN = 1;
Input.EGG_TOUCH_MOVE = 2;

Input.STANDARD_AXIS_USAGE = [
  0x00010030, // lx
  0x00010031, // ly
  0x00010033, // rx
  0x00010034, // ry
];

Input.STANDARD_BUTTON_USAGE = [
  0x00050037, // south
  0x00050037, // east
  0x00050037, // west
  0x00050037, // north
  0x00050039, // l1
  0x00050039, // r1
  0x00050039, // l2
  0x00050039, // r2
  0x0001003e, // select
  0x0001003d, // start
  0x00090000, // lp
  0x00090001, // rp
  0x00010090, // dup
  0x00010091, // ddown
  0x00010093, // dleft
  0x00010092, // dright
  0x00010085, // heart -- "System Main Menu", debatable.
];
