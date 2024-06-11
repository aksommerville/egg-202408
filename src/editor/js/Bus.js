/* Bus.js
 * State cache and event dispatcher.
 *
 * Events:
 *   {type:"status",status:"ok"|"error"|"pending"|"dead"}
 *   {type:"toc",toc}
 *   {type:"roms",roms:string[]}
 *   {type:"error",error:any}
 *   {type:"open",path,serial:Uint8Array}
 */
 
export class Bus {
  static getDependencies() {
    return [];
  }
  constructor() {
    this.listeners = [];
    this.nextId = 1;
    
    // General state.
    this.status = "pending";
    this.toc = {}; // {name,files:*[]} or {name,serial}
    this.roms = []; // string
  }
  
  listen(events, cb) {
    const id = this.nextId++;
    this.listeners.push({ events, cb, id });
    return id;
  }
  
  unlisten(id) {
    const p = this.listeners.findIndex(l => l.id === id);
    if (p >= 0) this.listeners.splice(p, 1);
  }
  
  broadcast(event) {
    if (!event?.type) throw new Error(`Events for broadcast must have a string 'type': ${JSON.stringify(event)}`);
    for (const { cb, events } of this.listeners) {
      if (events.includes(event.type)) cb(event);
    }
  }
  
  setStatus(status) {
    if (status === this.status) return;
    this.status = status;
    this.broadcast({ type: "status", status });
  }
  
  setToc(toc) {
    this.toc = toc;
    this.broadcast({ type: "toc", toc });
  }
  
  setRoms(roms) {
    this.roms = roms;
    this.broadcast({ type: "roms", roms });
  }
}

Bus.singleton = true;
