#include "egg_runner_internal.h"

#define EGG_EVENTQ_LIMIT 256
#define EGG_BUTTON_LIMIT 128 /* Refuse to list more than so many buttons per device, as a safety measure. */

/* Object definition.
 * We wastefully duplicate the entire device list, including every button on every device.
 * This speeds things up for public access.
 */
 
struct egg_button {
  int btnid;
  int hidusage;
  int lo,hi;
  int value;
};
 
struct egg_device {
  int devid;
  int rptcls; // What are we calling it, to the public? An EGG_EVENT_* or zero to ignore. (MMOTION for mouse)
  struct hostio_input *driver;
  char *name;
  int namec;
  int vid,pid,version;
  struct egg_button *buttonv;
  int buttonc,buttona;
  int x,y,w,h; // For fake mouse only.
};
 
struct egg_inmgr {
  union egg_event eventq[EGG_EVENTQ_LIMIT];
  int eventp,eventc;
  int eventmask;
  int cursor_desired;
  struct egg_device **devicev;
  int devicec,devicea;
  int mousex,mousey;
};

/* Cleanup.
 */
 
static void egg_device_del(struct egg_device *device) {
  if (!device) return;
  if (device->name) free(device->name);
  if (device->buttonv) free(device->buttonv);
  free(device);
}
 
void egg_inmgr_del(struct egg_inmgr *inmgr) {
  if (!inmgr) return;
  if (inmgr->devicev) {
    while (inmgr->devicec-->0) egg_device_del(inmgr->devicev[inmgr->devicec]);
    free(inmgr->devicev);
  }
  free(inmgr);
}

/* New.
 */
 
struct egg_inmgr *egg_inmgr_new() {
  struct egg_inmgr *inmgr=calloc(1,sizeof(struct egg_inmgr));
  if (!inmgr) return 0;
  
  inmgr->eventmask=(
    (1<<EGG_EVENT_JOY)|
    (1<<EGG_EVENT_KEY)|
    //(1<<EGG_EVENT_TOUCH)| // TOUCH should enable by default, but we're not supporting at all.
  0);
  inmgr->cursor_desired=1;
  
  return inmgr;
}

/* Test event capability.
 *   <1: Forbidden.
 *    1: Optional.
 *   >1: Required.
 */
 
static int egg_event_capable(int type) {
  switch (type) {
    case EGG_EVENT_JOY: return 1;
    case EGG_EVENT_KEY: return 1;
    case EGG_EVENT_TEXT: return 1;
    case EGG_EVENT_MMOTION: return 1;
    case EGG_EVENT_MBUTTON: return 1;
    case EGG_EVENT_MWHEEL: return 1;
    case EGG_EVENT_TOUCH: return 0;
    case EGG_EVENT_ACCEL: return 0;
  }
  return 0;
}

/* React to changes to the mouse event mask or visibility request.
 */
 
static void egg_inmgr_refresh_cursor(struct egg_inmgr *inmgr) {
  const int any_mouse_event=(
    (1<<EGG_EVENT_MMOTION)|
    (1<<EGG_EVENT_MBUTTON)|
    (1<<EGG_EVENT_MWHEEL)|
  0);
  if (inmgr->eventmask&any_mouse_event) {
    if (inmgr->cursor_desired) {
      if (egg.hostio->video->type->show_cursor) egg.hostio->video->type->show_cursor(egg.hostio->video,1);
    } else {
      if (egg.hostio->video->type->show_cursor) egg.hostio->video->type->show_cursor(egg.hostio->video,0);
    }
  } else {
    if (egg.hostio->video->type->show_cursor) egg.hostio->video->type->show_cursor(egg.hostio->video,0);
  }
}

/* Update.
 * Probably nothing for us to do?
 * If we ever do something like faking a pointer via key or touch events, it would require maintenance here.
 */
 
int egg_inmgr_update(struct egg_inmgr *inmgr) {
  return 0;
}

/* Push to event queue.
 * Never fails. If the queue is full, we'll log a warning and overwrite one entry.
 */
 
static union egg_event *egg_event_push(struct egg_inmgr *inmgr) {
  int p=inmgr->eventp+inmgr->eventc;
  if (p>=EGG_EVENTQ_LIMIT) p-=EGG_EVENTQ_LIMIT;
  union egg_event *event=inmgr->eventq+p;
  if (inmgr->eventc>=EGG_EVENTQ_LIMIT) {
    fprintf(stderr,"%s:ERROR: Event queue exhausted. Overwriting one event.\n",egg.exename);
    if (++(inmgr->eventp)>=EGG_EVENTQ_LIMIT) inmgr->eventp=0;
  } else {
    inmgr->eventc++;
  }
  return event;
}

/* Button list in device.
 */
 
static int egg_device_buttonv_search(const struct egg_device *device,int btnid) {
  int lo=0,hi=device->buttonc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    int q=device->buttonv[ck].btnid;
         if (btnid<q) hi=ck;
    else if (btnid>q) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

static struct egg_button *egg_device_buttonv_insert(struct egg_device *device,int p,int btnid) {
  if ((p<0)||(p>device->buttonc)) return 0;
  if (device->buttonc>=device->buttona) {
    if (device->buttona>EGG_BUTTON_LIMIT) return 0;
    int na=device->buttona+32;
    void *nv=realloc(device->buttonv,sizeof(struct egg_button)*na);
    if (!nv) return 0;
    device->buttonv=nv;
    device->buttona=na;
  }
  struct egg_button *button=device->buttonv+p;
  memmove(button+1,button,sizeof(struct egg_button)*(device->buttonc-p));
  device->buttonc++;
  memset(button,0,sizeof(struct egg_button));
  button->btnid=btnid;
  return button;
}

static struct egg_button *egg_device_get_button(const struct egg_device *device,int btnid) {
  int p=egg_device_buttonv_search(device,btnid);
  if (p<0) return 0;
  return device->buttonv+p;
}

/* Search device list.
 */
 
static int egg_inmgr_devicev_search(const struct egg_inmgr *inmgr,int devid) {
  int lo=0,hi=inmgr->devicec;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    int q=inmgr->devicev[ck]->devid;
         if (devid<q) hi=ck;
    else if (devid>q) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

/* Get device.
 */
 
static inline struct egg_device *egg_inmgr_get_device(const struct egg_inmgr *inmgr,int devid) {
  int p=egg_inmgr_devicev_search(inmgr,devid);
  if (p<0) return 0;
  return inmgr->devicev[p];
}

/* Remove device.
 */
 
static void egg_inmgr_remove_device(struct egg_inmgr *inmgr,int devid) {
  int p=egg_inmgr_devicev_search(inmgr,devid);
  if (p<0) return;
  struct egg_device *device=inmgr->devicev[p];
  inmgr->devicec--;
  memmove(inmgr->devicev+p,inmgr->devicev+p+1,sizeof(void*)*(inmgr->devicec-p));
  egg_device_del(device);
}

/* Receive device's button declarations.
 * No need to make big decisions at this point, just insert it in the list.
 */
 
static int egg_inmgr_cb_declare_button(int btnid,int hidusage,int lo,int hi,int value,void *userdata) {
  struct egg_device *device=userdata;
  fprintf(stderr,"%s %08x %08x %d..%d =%d\n",__func__,btnid,hidusage,lo,hi,value);
  int range=hi-lo+1;
  if (range<2) return 0; // Not interesting.
  int p=egg_device_buttonv_search(device,btnid);
  if (p>=0) return 0; // Already got it? Something is fishy.
  p=-p-1;
  struct egg_button *button=egg_device_buttonv_insert(device,p,btnid);
  if (!button) return -1; // Stop iterating if we refused to add, maybe there's too many.
  button->hidusage=hidusage;
  button->lo=lo;
  button->hi=hi;
  button->value=value;
  return 0;
}

/* Configure new device.
 * Decide what kind of thing it is, and set (rptcls) accordingly.
 * If buttons need mapped, anything like that, set it all up.
 * Do not push any events or remove the device.
 * Fail, or leave (rptcls) zero, and the caller should disconnect it.
 */
 
static int egg_device_configure(struct egg_inmgr *inmgr,struct egg_device *device) {
  fprintf(stderr,
    "%s:%d:TODO: Establish mapping etc for new input device %04x:%04x:%04x '%.*s'\n",
    __FILE__,__LINE__,device->vid,device->pid,device->version,device->namec,device->name
  );
  
  /* If it has at least 40 keys and one of them is 0x00070004 (Keyboard A), call it a keyboard.
   * These map straight off the buttons' hidusage.
   */
  if (device->buttonc>=40) {
    const struct egg_button *button=device->buttonv;
    int i=device->buttonc;
    for (;i-->0;button++) {
      if (button->hidusage==0x00070004) {
        device->rptcls=EGG_EVENT_KEY;
        return 0;
      }
    }
  }
  
  /* REL_X, REL_Y, and BTN_LEFT, assume it's a mouse.
   * We're using evdev button ids here, this is only going to work for Linux.
   * And of course if you're running a window manager, the mouse device shouldn't appear at all.
   */
  struct egg_button *relx=egg_device_get_button(device,0x00020000);
  if (relx) {
    struct egg_button *rely=egg_device_get_button(device,0x00020001);
    struct egg_button *bleft=egg_device_get_button(device,0x00010110);
    if (rely&&bleft) {
      device->rptcls=EGG_EVENT_MMOTION;
      relx->hidusage=0x00010030;
      rely->hidusage=0x00010031;
      bleft->hidusage=0x00090000;
      struct egg_button *button;
      if (button=egg_device_get_button(device,0x00010111)) button->hidusage=0x00090001; // right
      if (button=egg_device_get_button(device,0x00010112)) button->hidusage=0x00090002; // middle
      if (button=egg_device_get_button(device,0x00020006)) button->hidusage=0x00010033; // hwheel, calling it "Rx" in HID terms
      if (button=egg_device_get_button(device,0x00020008)) button->hidusage=0x00010038; // wheel
      int btnusagenext=0x00090003;
      int i=device->buttonc;
      for (button=device->buttonv;i-->0;button++) {
        if (button->hidusage) continue; // already got it, cool
        if ((button->lo==0)&&((button->hi==1)||(button->hi==2))) {
          button->hidusage=btnusagenext++;
        }
      }
      device->w=egg.hostio->video->w;
      device->h=egg.hostio->video->h;
      device->x=device->w>>1;
      device->y=device->h>>1;
      return 0;
    }
  }

  return 0;
}

/* Add device.
 */
 
static struct egg_device *egg_inmgr_add_device(struct egg_inmgr *inmgr,struct hostio_input *driver,int devid) {

  // Allocate device and insert in list.
  if (devid<1) return 0;
  int p=egg_inmgr_devicev_search(inmgr,devid);
  if (p>=0) return 0;
  p=-p-1;
  if (inmgr->devicec>=inmgr->devicea) {
    int na=inmgr->devicea+16;
    if (na>INT_MAX/sizeof(void*)) return 0;
    void *nv=realloc(inmgr->devicev,sizeof(void*)*na);
    if (!nv) return 0;
    inmgr->devicev=nv;
    inmgr->devicea=na;
  }
  struct egg_device *device=calloc(1,sizeof(struct egg_device));
  if (!device) return 0;
  memmove(inmgr->devicev+p+1,inmgr->devicev+p,sizeof(void*)*(inmgr->devicec-p));
  inmgr->devicec++;
  inmgr->devicev[p]=device;
  device->devid=devid;
  device->driver=driver;
  
  // Collect IDs and name.
  if (driver->type->get_ids) {
    const char *name=driver->type->get_ids(&device->vid,&device->pid,&device->version,driver,devid);
    if (name&&name[0]) {
      int namec=0; while (name[namec++]) namec++;
      if (device->name=malloc(namec+1)) {
        memcpy(device->name,name,namec);
        device->name[namec]=0;
        device->namec=namec;
      }
    }
  }
  
  // Collect buttons.
  if (driver->type->for_each_button) {
    driver->type->for_each_button(driver,devid,egg_inmgr_cb_declare_button,device);
  }
  
  // Decide how we're going to use the device (keyboard, mouse, joystick, touch, accelerometer?), and configure that.
  if ((egg_device_configure(egg.inmgr,device)<0)||!device->rptcls) {
    fprintf(stderr,"%s: Dropping input device '%.*s' after failure to configure.\n",egg.exename,device->namec,device->name);
    egg_inmgr_remove_device(egg.inmgr,devid);
    if (driver->type->disconnect) driver->type->disconnect(driver,devid);
    return 0;
  }
  
  return device;
}

/* hostio callbacks.
 ******************************************************************************************************/

int egg_cb_key(struct hostio_video *driver,int keycode,int value) {
  if (egg.inmgr->eventmask&(1<<EGG_EVENT_KEY)) {
    union egg_event *event=egg_event_push(egg.inmgr);
    event->key.type=EGG_EVENT_KEY;
    event->key.keycode=keycode;
    event->key.value=value;
  }
  if (egg.inmgr->eventmask&(1<<EGG_EVENT_TEXT)) return 0;
  return 1;
}

void egg_cb_text(struct hostio_video *driver,int codepoint) {
  if (!(egg.inmgr->eventmask&(1<<EGG_EVENT_TEXT))) return;
  union egg_event *event=egg_event_push(egg.inmgr);
  event->text.type=EGG_EVENT_TEXT;
  event->text.codepoint=codepoint;
}
 
void egg_cb_mmotion(struct hostio_video *driver,int x,int y) {
  render_coords_fb_from_screen(egg.render,&x,&y);
  if ((x==egg.inmgr->mousex)&&(y==egg.inmgr->mousey)) return;
  egg.inmgr->mousex=x;
  egg.inmgr->mousey=y;
  if (!(egg.inmgr->eventmask&(1<<EGG_EVENT_MMOTION))) return;
  union egg_event *event=egg_event_push(egg.inmgr);
  event->mmotion.type=EGG_EVENT_MMOTION;
  event->mmotion.x=x;
  event->mmotion.y=y;
}

void egg_cb_mbutton(struct hostio_video *driver,int btnid,int value) {
  if (!(egg.inmgr->eventmask&(1<<EGG_EVENT_MBUTTON))) return;
  union egg_event *event=egg_event_push(egg.inmgr);
  event->mbutton.type=EGG_EVENT_MBUTTON;
  event->mbutton.x=egg.inmgr->mousex;
  event->mbutton.y=egg.inmgr->mousey;
  event->mbutton.btnid=btnid;
  event->mbutton.value=value;
}

void egg_cb_mwheel(struct hostio_video *driver,int dx,int dy) {
  if (!(egg.inmgr->eventmask&(1<<EGG_EVENT_MWHEEL))) return;
  union egg_event *event=egg_event_push(egg.inmgr);
  event->mwheel.type=EGG_EVENT_MWHEEL;
  event->mwheel.x=egg.inmgr->mousex;
  event->mwheel.y=egg.inmgr->mousey;
  event->mwheel.dx=dx;
  event->mwheel.dy=dy;
}

void egg_cb_connect(struct hostio_input *driver,int devid) {
  fprintf(stderr,"%s %d\n",__func__,devid);
  struct egg_device *device=egg_inmgr_add_device(egg.inmgr,driver,devid);
  if (!device) return;
  switch (device->rptcls) {
  
    case EGG_EVENT_JOY: {
        if (egg.inmgr->eventmask&(1<<EGG_EVENT_JOY)) {
          union egg_event *event=egg_event_push(egg.inmgr);
          event->joy.type=EGG_EVENT_JOY;
          event->joy.devid=devid;
          event->joy.btnid=0; // "connection state"
          event->joy.value=1;
        }
      } break;
    
    // Other reporting classes, I think we don't need any "connect" event.
    case EGG_EVENT_KEY: break;
    case EGG_EVENT_MMOTION: break;
    case EGG_EVENT_TOUCH: break;
    case EGG_EVENT_ACCEL: break;
  }
}

void egg_cb_disconnect(struct hostio_input *driver,int devid) {
  fprintf(stderr,"%s %d\n",__func__,devid);
  struct egg_device *device=egg_inmgr_get_device(egg.inmgr,devid);
  if (!device) return;
  switch (device->rptcls) {
  
    case EGG_EVENT_JOY: {
        //TODO Drop nonzero buttons? Or is that the client's problem?
        if (egg.inmgr->eventmask&(1<<EGG_EVENT_JOY)) {
          union egg_event *event=egg_event_push(egg.inmgr);
          event->joy.type=EGG_EVENT_JOY;
          event->joy.devid=devid;
          event->joy.btnid=0; // "connection state"
          event->joy.value=0;
        }
      } break;
      
    case EGG_EVENT_KEY: {
        if (egg.inmgr->eventmask&(1<<EGG_EVENT_KEY)) {
          struct egg_button *button=device->buttonv;
          int i=device->buttonc;
          for (;i-->0;button++) {
            if (!button->value) continue;
            if ((button->hidusage<0x00070000)||(button->hidusage>=0x00080000)) continue;
            union egg_event *event=egg_event_push(egg.inmgr);
            event->key.type=EGG_EVENT_KEY;
            event->key.keycode=button->hidusage;
            event->key.value=0;
          }
        }
      } break;
      
    case EGG_EVENT_MMOTION: break;//TODO drop held buttons
    case EGG_EVENT_TOUCH: break;//TODO?
    case EGG_EVENT_ACCEL: break;//TODO?
  }
  egg_inmgr_remove_device(egg.inmgr,devid);
}

void egg_cb_button(struct hostio_input *driver,int devid,int btnid,int value) {
  //fprintf(stderr,"%s %d.0x%08x=%d\n",__func__,devid,btnid,value);
  if (!btnid) return;
  struct egg_device *device=egg_inmgr_get_device(egg.inmgr,devid);
  if (!device) return;
  switch (device->rptcls) {
  
    case EGG_EVENT_JOY: {
        //TODO Mapping etc.
        if (egg.inmgr->eventmask&(1<<EGG_EVENT_JOY)) {
          union egg_event *event=egg_event_push(egg.inmgr);
          event->joy.type=EGG_EVENT_JOY;
          event->joy.devid=devid;
          event->joy.btnid=btnid;
          event->joy.value=value;
        }
      } break;
      
    case EGG_EVENT_KEY: {
        if (!(egg.inmgr->eventmask&(1<<EGG_EVENT_KEY))) return;
        struct egg_button *button=egg_device_get_button(device,btnid);
        if (!button) return;
        if ((button->hidusage<0x00070000)||(button->hidusage>=0x00080000)) return;
        if (value==button->value) return;
        button->value=value;
        union egg_event *event=egg_event_push(egg.inmgr);
        event->key.type=EGG_EVENT_KEY;
        event->key.keycode=button->hidusage;
        event->key.value=value;
        // We'll fake KEY events straight off the device, but not TEXT. That would be too complicated. TODO But should we?
      } break;
      
    case EGG_EVENT_MMOTION: {
        struct egg_button *button=egg_device_get_button(device,btnid);
        if (!button||!button->hidusage) return;
        switch (button->hidusage) {
          case 0x00010030: { // X
              int nx=device->x+value;
              if (nx<0) nx=0; else if (nx>=device->w) nx=device->w-1;
              if (nx!=device->x) {
                device->x=nx;
                egg_cb_mmotion(0,device->x,device->y);
              }
            } break;
          case 0x00010031: { // Y
              int ny=device->y+value;
              if (ny<0) ny=0; else if (ny>=device->h) ny=device->h-1;
              if (ny!=device->y) {
                device->y=ny;
                egg_cb_mmotion(0,device->x,device->y);
              }
            } break;
          case 0x00010033: { // horz wheel.
              egg_cb_mwheel(0,value,0);
            } break;
          case 0x00010038: { // vert (default) wheel
              egg_cb_mwheel(0,0,-value);
            } break;
          default: if ((button->hidusage>=0x00090000)&&(button->hidusage<0x000a0000)) {
              egg_cb_mbutton(0,button->hidusage-0x00090000+1,value);
            }
        }
      } break;

    case EGG_EVENT_TOUCH: break;//TODO?
    case EGG_EVENT_ACCEL: break;//TODO?
  }
}

/* Public API.
 ***********************************************************************************************************/
 
int egg_event_get(union egg_event *v,int a) {
  if (a>egg.inmgr->eventc) a=egg.inmgr->eventc;
  int headc=EGG_EVENTQ_LIMIT-egg.inmgr->eventp;
  if (headc>a) headc=a;
  memcpy(v,egg.inmgr->eventq+egg.inmgr->eventp,sizeof(union egg_event)*headc);
  if ((egg.inmgr->eventp+=headc)>=EGG_EVENTQ_LIMIT) egg.inmgr->eventp=0;
  egg.inmgr->eventc-=headc;
  int tailc=a-headc;
  if (tailc>0) {
    memcpy(v+headc,egg.inmgr->eventq,sizeof(union egg_event)*tailc);
    egg.inmgr->eventp+=tailc;
    egg.inmgr->eventc-=tailc;
  }
  return a;
}

int egg_event_enable(int type,int enable) {
  if ((type<1)||(type>8)) return 0;
  int bit=1<<type;
  if (enable) {
    if (egg.inmgr->eventmask&bit) return 1;
    if (egg_event_capable(type)<1) return 0;
    egg.inmgr->eventmask|=bit;
  } else {
    if (!(egg.inmgr->eventmask&bit)) return 0;
    if (egg_event_capable(type)>1) return 1;
    egg.inmgr->eventmask&=~bit;
  }
  switch (type) {
    case EGG_EVENT_JOY: break;
    case EGG_EVENT_KEY: break;
    case EGG_EVENT_TEXT: break;
    case EGG_EVENT_MMOTION: egg_inmgr_refresh_cursor(egg.inmgr); break;
    case EGG_EVENT_MBUTTON: egg_inmgr_refresh_cursor(egg.inmgr); break;
    case EGG_EVENT_MWHEEL: egg_inmgr_refresh_cursor(egg.inmgr); break;
    case EGG_EVENT_TOUCH: break;
    case EGG_EVENT_ACCEL: break;
  }
  return (egg.inmgr->eventmask&bit)?1:0;
}

void egg_show_cursor(int show) {
  if (show) {
    if (egg.inmgr->cursor_desired) return;
    egg.inmgr->cursor_desired=1;
  } else {
    if (!egg.inmgr->cursor_desired) return;
    egg.inmgr->cursor_desired=0;
  }
  egg_inmgr_refresh_cursor(egg.inmgr);
}

int egg_lock_cursor(int lock) {
  //TODO Pointer Capture. Will require additional support from hostio.
  return 0;
}

int egg_joystick_devid_by_index(int p) {
//TODO
  return 0;
}

void egg_joystick_get_ids(int *vid,int *pid,int *version,int devid) {
//TODO
}

int egg_joystick_get_name(char *dst,int dsta,int devid) {
//TODO
  return 0;
}

int egg_joystick_for_each_button(int devid,int (*cb)(int btnid,int usage,int lo,int hi,int value,void *userdata),void *userdata) {
//TODO
  return 0;
}
