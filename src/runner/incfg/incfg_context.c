#include "incfg_internal.h"

/* Delete.
 */
 
void incfg_del(struct incfg *incfg) {
  if (!incfg) return;
  free(incfg);
}

/* New.
 */
 
struct incfg *incfg_new() {
  struct incfg *incfg=calloc(1,sizeof(struct incfg));
  if (!incfg) return 0;
  return incfg;
}

/* Trivial accessors.
 */
 
int incfg_is_finished(const struct incfg *incfg) {
  if (!incfg) return 1;
  return incfg->finished;
}

/* Declare startup properties as if we were a ROM file.
 */
 
int incfg_startup_props(struct egg_rom_startup_props *props,struct incfg *incfg) {
  props->title=strdup("egg --configure-input");
  props->fbw=160;
  props->fbh=90;
  return 0;
}

/* Start.
 */
 
int incfg_start(struct incfg *incfg) {

  // Prep video.
  render_texture_get_header(&incfg->screenw,&incfg->screenh,0,egg.render,1);
  if ((incfg->texid_font=render_texture_new(egg.render))<1) return -1;
  if (render_texture_load(egg.render,incfg->texid_font,0,0,0,0,incfg_font_tilesheet,incfg_font_tilesheet_size)<0) return -1;
  
  // Event mask.
  egg_event_enable(EGG_EVENT_JOY,0);
  egg_event_enable(EGG_EVENT_KEY,0);
  egg_event_enable(EGG_EVENT_TEXT,0);
  egg_event_enable(EGG_EVENT_MMOTION,0);
  egg_event_enable(EGG_EVENT_MBUTTON,0);
  egg_event_enable(EGG_EVENT_MWHEEL,0);
  egg_event_enable(EGG_EVENT_TOUCH,0);
  egg_event_enable(EGG_EVENT_ACCEL,0);
  egg_event_enable(EGG_EVENT_RAW,1);
  
  incfg->finished=0;
  incfg->state=INCFG_STATE_WAIT1;
  incfg->giveup_clock=INCFG_GIVEUP_TIME;
  memset(incfg->sourcev,0,sizeof(incfg->sourcev));
  memset(incfg->ignoreix,0,sizeof(incfg->ignoreix));
  
  return 0;
}

/* Deliver finished source list to inmap.
 */
 
static int incfg_deliver_sources(struct incfg *incfg) {
  int vid=0,pid=0,version=0;
  const char *name=egg_inmgr_get_device_ids(&vid,&pid,&version,egg.inmgr,incfg->devid);
  struct egg_inmap *inmap=egg_inmgr_get_inmap(egg.inmgr);
  struct egg_inmap_rules *rules=egg_inmap_rewrite_rules(inmap,vid,pid,version,name,-1);
  if (!rules) return -1;
  const struct incfg_source *source=incfg->sourcev;
  int i=0; for (;i<21;i++,source++) {
    if (!source->btnid) continue;
    int dstbtnid=0;
    
    switch (i) {
      case 0: dstbtnid=EGG_JOYBTN_SOUTH; break;
      case 1: dstbtnid=EGG_JOYBTN_EAST; break;
      case 2: dstbtnid=EGG_JOYBTN_WEST; break;
      case 3: dstbtnid=EGG_JOYBTN_NORTH; break;
      case 4: dstbtnid=EGG_JOYBTN_L1; break;
      case 5: dstbtnid=EGG_JOYBTN_R1; break;
      case 6: dstbtnid=EGG_JOYBTN_L2; break;
      case 7: dstbtnid=EGG_JOYBTN_R2; break;
      case 8: dstbtnid=EGG_JOYBTN_AUX2; break;
      case 9: dstbtnid=EGG_JOYBTN_AUX1; break;
      case 10: dstbtnid=EGG_JOYBTN_LP; break;
      case 11: dstbtnid=EGG_JOYBTN_RP; break;
      case 12: switch (source->part) {
          case 0: dstbtnid=EGG_JOYBTN_UP; break;
          case -1: dstbtnid=EGG_INMAP_BTN_VERT; break;
          case 1: dstbtnid=EGG_INMAP_BTN_NVERT; break;
          case 8: dstbtnid=EGG_INMAP_BTN_DPAD; break;
        } break;
      case 13: dstbtnid=EGG_JOYBTN_DOWN; break;
      case 14: switch (source->part) {
          case 0: dstbtnid=EGG_JOYBTN_LEFT;
          case -1: dstbtnid=EGG_INMAP_BTN_HORZ; break;
          case 1: dstbtnid=EGG_INMAP_BTN_NHORZ; break;
        } break;
      case 15: dstbtnid=EGG_JOYBTN_RIGHT; break;
      case 16: dstbtnid=EGG_JOYBTN_AUX3; break;
      case 17: switch (source->part) {
          case -1: dstbtnid=EGG_JOYBTN_LX; break;
          case 1: dstbtnid=EGG_INMAP_BTN_NLX; break;
        } break;
      case 18: switch (source->part) {
          case -1: dstbtnid=EGG_JOYBTN_LY; break;
          case 1: dstbtnid=EGG_INMAP_BTN_NLY; break;
        } break;
      case 19: switch (source->part) {
          case -1: dstbtnid=EGG_JOYBTN_RX; break;
          case 1: dstbtnid=EGG_INMAP_BTN_NRX; break;
        } break;
      case 20: switch (source->part) {
          case -1: dstbtnid=EGG_JOYBTN_RY; break;
          case 1: dstbtnid=EGG_INMAP_BTN_NRY; break;
        } break;
    }
    
    if (!dstbtnid) continue;
    if (egg_inmap_rules_add_button(rules,source->btnid,dstbtnid)<0) return -1;
  }
  return egg_inmap_ready(inmap);
}

/* Commit the currently tracking button and advance to the next one.
 * If not (save), advance but don't commit the current state.
 */
 
static void incfg_commit_button(struct incfg *incfg,int save) {
  if ((incfg->btnix>=21)||incfg->finished) {
    incfg->finished=1;
    return;
  }
  if (save) {
    incfg->sourcev[incfg->btnix].btnid=incfg->btnid;
    incfg->sourcev[incfg->btnix].part=incfg->part;
    
    // Check whether this assignment obviates some future one.
    switch (incfg->btnix) {
      case 12: if (incfg->part==8) {
          incfg->ignoreix[13]=1;
          incfg->ignoreix[14]=1;
          incfg->ignoreix[15]=1;
        } else if (incfg->part) {
          incfg->ignoreix[13]=1;
        } break;
      case 14: if (incfg->part) {
          incfg->ignoreix[15]=1;
        } break;
    }
    
  } else {
    incfg->sourcev[incfg->btnix].btnid=0;
    incfg->sourcev[incfg->btnix].part=0;
  }
  for (;;) {
    incfg->btnix++;
    if (incfg->btnix>=21) {
      incfg->finished=1;
      incfg_deliver_sources(incfg);
      return;
    }
    if (incfg->ignoreix[incfg->btnix]) continue;
    incfg->state=INCFG_STATE_WAIT1;
    incfg->giveup_clock=INCFG_GIVEUP_TIME;
    return;
  }
}

/* Raw input.
 */
 
static void incfg_on_raw(struct incfg *incfg,const struct egg_event_raw *event) {
  if (incfg->finished) return;
  
  // If we have a device bound, and it's not this one, ignore.
  if (incfg->devid&&(incfg->devid!=event->devid)) return;
  
  /* If the bound device is disconnected, abort hard.
   * Ignore all other connect and disconnect events.
   */
  if (!event->btnid) {
    if (!event->value&&(event->devid==incfg->devid)) {
      fprintf(stderr,"incfg: Bound device disconnected!\n");
      incfg->finished=1;
    }
    return;
  }
  
  /* Ask inmgr for this button's range, and digest all hats and axes into their relevant parts.
   * Egg does pass analogue axes through in Standard Mapping, but for configuration purposes we need two-state inputs only.
   * Axes and hats are special: Hats can only map to the dpad, and axes can only be LX, LY, RX, RY, or a dpad axis.
   */
  int lo=0,hi=0,rest=0,pressed=0;
  egg_inmgr_get_button_range(&lo,&hi,&rest,egg.inmgr,event->devid,event->btnid);
  int range=hi-lo+1;
  int part=0;
  if (range<2) {
    return;
  } else if (range==8) { // Hat
    int norm=event->value-lo;
    pressed=((norm>=0)&&(norm<=7));
    part=norm;
  } else if ((range>=3)&&(lo<rest)&&(rest<hi)) { // Axis
    int mid=(lo+hi)>>1;
    int midlo=(lo+mid)>>1;
    int midhi=(hi+mid)>>1;
    if (midlo>=mid) midlo=mid-1;
    if (midhi<=mid) midhi=mid+1;
    pressed=(event->value<=midlo)?-1:(event->value>=midhi)?1:0;
    part=pressed;
  } else if (lo!=0) { // Plain buttons (or one-way axes) must ground at zero.
    return;
  } else {
    pressed=event->value;
  }
  
  /* With no device bound, skip any zero-value events, and any 'pressed' events bind the device.
   * If we bind, do proceed with regular processing.
   */
  if (!incfg->devid) {
    if (!pressed) return;
    incfg->devid=event->devid;
    incfg->giveup_clock=INCFG_GIVEUP_TIME;
    incfg->state=INCFG_STATE_WAIT1;
  }
  
  switch (incfg->state) {
  
    case INCFG_STATE_FAULT:
    case INCFG_STATE_WAIT1: {
        if (!pressed) {
          // Leave FAULT state when anything releases, don't linger there.
          incfg->state=INCFG_STATE_WAIT1;
          incfg->giveup_clock=INCFG_GIVEUP_TIME;
          return;
        }
        if (range==8) { // Hats can only assign to the dpad, and only if they press up.
          if (incfg->btnix!=12) { // we're not asking for up, can't accept dpad
            incfg->state=INCFG_STATE_FAULT;
            incfg->giveup_clock=INCFG_GIVEUP_TIME;
            return;
          }
          switch (part) {
            case 0: case 2: case 4: case 6: break;
            default: return; // diagonal. wait for them to cardinalize it.
          }
          if (part!=0) { // they pressed something other than up. (or we're reading it wrong. either way, reject)
            incfg->state=INCFG_STATE_FAULT;
            incfg->giveup_clock=INCFG_GIVEUP_TIME;
          }
          part=8; // how we'll write it to statev.
        } else if (part) { // Analogue axes can only assign to axes or dpad.
          switch (incfg->btnix) {
            case 12: case 14: case 17: case 18: case 19: case 20: break;
            default: {
                incfg->state=INCFG_STATE_FAULT;
                incfg->giveup_clock=INCFG_GIVEUP_TIME;
              }
          }
        }
        incfg->part=part;
        incfg->btnid=event->btnid;
        incfg->state=INCFG_STATE_HOLD1;
        incfg->giveup_clock=INCFG_GIVEUP_TIME;
      } break;
      
    case INCFG_STATE_HOLD1: {
        if (event->btnid==incfg->btnid) {
          if (!pressed) {
            incfg->state=INCFG_STATE_WAIT2;
            incfg->giveup_clock=INCFG_GIVEUP_TIME;
          }
        } else if (pressed) {
          incfg->state=INCFG_STATE_FAULT;
          incfg->giveup_clock=INCFG_GIVEUP_TIME;
        }
      } break;
      
    case INCFG_STATE_WAIT2: {
        if (!pressed) return;
        if (range==8) { // Hats, we must ignore diagonals. Only accept if it's a cardinal direction.
          switch (part) {
            case 0: case 2: case 4: case 6: break;
            default: return;
          }
          if (part!=0) {
            incfg->state=INCFG_STATE_FAULT;
            incfg->giveup_clock=INCFG_GIVEUP_TIME;
          }
          part=8;
        }
        if ((event->btnid!=incfg->btnid)||(part!=incfg->part)) {
          incfg->state=INCFG_STATE_FAULT;
          incfg->giveup_clock=INCFG_GIVEUP_TIME;
        } else {
          incfg->state=INCFG_STATE_HOLD2;
          incfg->giveup_clock=INCFG_GIVEUP_TIME;
        }
      } break;
      
    case INCFG_STATE_HOLD2: {
        if (event->btnid==incfg->btnid) {
          if (!pressed) {
            incfg_commit_button(incfg,1);
          }
        } else if (pressed) {
          incfg->state=INCFG_STATE_FAULT;
          incfg->giveup_clock=INCFG_GIVEUP_TIME;
        }
      } break;
  }
}

/* Update.
 */
 
int incfg_update(struct incfg *incfg,double elapsed) {
  if (incfg->finished) return 0;
  if ((incfg->focus_blink_clock-=elapsed)<=0.0) {
    incfg->focus_blink_clock+=INCFG_FOCUS_BLINK_PERIOD;
  }
  if ((incfg->giveup_clock-=elapsed)<=0.0) {
    incfg_commit_button(incfg,0);
  }
  union egg_event eventv[16];
  int eventc;
  while ((eventc=egg_event_get(eventv,16))>0) {
    const union egg_event *event=eventv;
    int i=eventc; for (;i-->0;event++) switch (event->type) {
      case EGG_EVENT_RAW: incfg_on_raw(incfg,&event->raw); break;
    }
    if (eventc<16) break;
  }
  return 0;
}

/* Render.
 */
 
void incfg_render(struct incfg *incfg) {
  if (incfg->finished) return;
  render_draw_rect(egg.render,1,0,0,incfg->screenw,incfg->screenh,0x203040ff);
  
  /* Static outline is 28 tiles. 7*4, 0x80..0xc3, then 0x80..0xc2 flopped on the right.
   * On top of that, the button outlines: 19 (17 but L1 and R2 each take 2 tiles)
   * And finally, the highlight: 1 or 2 tiles
   */
  struct egg_draw_tile vtxv[7*4];
  const int colw=8;
  int diagramw=colw*7;
  int diagramh=colw*4;
  int diagramx=(incfg->screenw>>1)-(diagramw>>1);
  int diagramy=(incfg->screenh>>1)-(diagramh>>1);
  #define SETVTX(p,rx,ry,tid,xf) { \
    vtxv[p].x=diagramx+(rx); \
    vtxv[p].y=diagramy+(ry); \
    vtxv[p].tileid=tid; \
    vtxv[p].xform=xf; \
  }
  
  // Static outline.
  struct egg_draw_tile *vtx=vtxv;
  int row=0,y=diagramy+(colw>>1); for (;row<4;row++,y+=colw) {
    int col=0,x=diagramx+(colw>>1); for (;col<7;col++,x+=colw,vtx++) {
      vtx->x=x;
      vtx->y=y;
      if (col<4) {
        vtx->tileid=0x80+col;
        vtx->xform=0;
      } else {
        vtx->tileid=0x80-col+6;
        vtx->xform=EGG_XFORM_XREV;
      }
      vtx->tileid+=row<<4;
    }
  }
  render_tint(egg.render,0x808080ff);
  render_draw_tile(egg.render,1,incfg->texid_font,vtxv,28);
  
  // Button outlines.
  SETVTX(0,2+colw*5,colw*3-3,0x86,0) // South
  SETVTX(1,2+colw*6-3,colw*2,0x86,0) // East
  SETVTX(2,2+colw*4+3,colw*2,0x86,0) // West
  SETVTX(3,2+colw*5,colw+3,0x86,0) // North
  SETVTX(4,colw+(colw>>1),colw>>1,0x94,0) // L1 (left)
  SETVTX(5,colw*2+(colw>>1),colw>>1,0x95,0) // L1 (right)
  SETVTX(6,colw*4+(colw>>1),colw>>1,0x95,EGG_XFORM_XREV) // R1 (left)
  SETVTX(7,colw*5+(colw>>1),colw>>1,0x94,EGG_XFORM_XREV) // R1 (right)
  SETVTX(8,colw,colw>>1,0x98,0) // L2
  SETVTX(9,colw*6,colw>>1,0x98,EGG_XFORM_XREV) // R2
  SETVTX(10,colw*3+(colw>>1),colw*2+(colw>>1),0x8a,0) // Aux2 (upper)
  SETVTX(11,colw*3+(colw>>1),colw*2+(colw>>1),0x8a,EGG_XFORM_XREV) // Aux1 (lower)
  SETVTX(12,colw*2+(colw>>1),colw*3+(colw>>1),0x86,0) // LP
  SETVTX(13,colw*4+(colw>>1),colw*3+(colw>>1),0x86,0) // RP
  SETVTX(14,colw*2-2,colw+(colw>>1),0x84,EGG_XFORM_SWAP) // Up
  SETVTX(15,colw*2-2,colw*2+(colw>>1),0x84,EGG_XFORM_XREV|EGG_XFORM_SWAP) // Down
  SETVTX(16,colw+(colw>>1)-2,colw*2,0x84,0) // Left
  SETVTX(17,colw*2+(colw>>1)-2,colw*2,0x84,EGG_XFORM_XREV) // Right
  SETVTX(18,colw*3+(colw>>1),colw+(colw>>1),0x86,0) // Aux3
  render_tint(egg.render,0xffffffff);
  render_draw_tile(egg.render,1,incfg->texid_font,vtxv,19);
  
  // Highlight.
  int focusp=-1,focuspart=0;
  switch (incfg->btnix) {
    case 0: focusp=0; break;
    case 1: focusp=1; break;
    case 2: focusp=2; break;
    case 3: focusp=3; break;
    case 4: focusp=4; break;
    case 5: focusp=6; break;
    case 6: focusp=8; break;
    case 7: focusp=9; break;
    case 8: focusp=10; break;
    case 9: focusp=11; break;
    case 10: focusp=12; break;
    case 11: focusp=13; break;
    case 12: focusp=14; break;
    case 13: focusp=15; break;
    case 14: focusp=16; break;
    case 15: focusp=17; break;
    case 16: focusp=18; break;
    case 17: focusp=12; focuspart=1; break; // lx
    case 18: focusp=12; focuspart=2; break; // ly
    case 19: focusp=13; focuspart=1; break; // rx
    case 20: focusp=13; focuspart=2; break; // ry
  }
  if (focusp>=0) {
    int vtxc=1;
    if ((focusp==4)||(focusp==6)) vtxc=2; // L1 and R1 are each two tiles.
    memmove(vtxv,vtxv+focusp,sizeof(struct egg_draw_tile)*vtxc);
    switch (focusp) {
      case 12: case 13: switch (focuspart) {
          case 0: vtxv[0].tileid=0x89; break; // LP,RP
          case 1: vtxv[0].tileid=0x88; break; // LX,RX
          case 2: vtxv[0].tileid=0x88; vtxv[0].xform=EGG_XFORM_SWAP; break; // LY,RY
        } break;
      default: {
          vtxv[0].tileid+=vtxc;
          vtxv[1].tileid+=vtxc;
        }
    }
    int color=0;
    int selection=(incfg->focus_blink_clock>=INCFG_FOCUS_BLINK_PERIOD*0.5)?1:0;
    switch (incfg->state) {
      case INCFG_STATE_FAULT: color=selection?0xff0000ff:0xc00000ff; break;
      case INCFG_STATE_WAIT1: color=selection?0xc0c000ff:0xa0a000ff; break;
      case INCFG_STATE_HOLD1: color=0xffff00ff; break;
      case INCFG_STATE_WAIT2: color=selection?0x00c000ff:0x00a000ff; break;
      case INCFG_STATE_HOLD2: color=0x00ff00ff; break;
    }
    render_tint(egg.render,color);
    render_draw_tile(egg.render,1,incfg->texid_font,vtxv,vtxc);
  }
  
  /* Give-up clock.
   * Counts down 5,4,3,2,1 when we're in the WARNING period.
   */
  if (incfg->giveup_clock<INCFG_GIVEUP_WARNING_TIME) {
    int digit=1+(int)((incfg->giveup_clock*5.0)/INCFG_GIVEUP_WARNING_TIME);
    if (digit<1) digit=1; else if (digit>5) digit=5;
    vtxv[0].x=incfg->screenw>>1;
    vtxv[0].y=diagramy+diagramh+colw*2;
    vtxv[0].tileid=0x30+digit;
    vtxv[0].xform=0;
    render_tint(egg.render,0xff2040ff);
    render_draw_tile(egg.render,1,incfg->texid_font,vtxv,1);
  }
  
  #undef SETVTX
}
