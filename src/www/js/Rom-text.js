export class Rom {
  constructor(src) {
    this.resv = []; // {tid,qual,rid,v:Uint8Array}, sorted
    this.decode(src);
  }
  
  decode(src) {
    let tid=1, qual=0, rid=1, i=0;
    const rdch = () => {
      let ch;
      for (;;) {
        ch = src.charCodeAt(i++);
        if (ch > 0x20) return ch;
        if (i >= src.length) return 0;
      }
    };
    const rdn = () => {
      let n=0;
      while (i < src.length) {
        const ch = rdch();
        if (!ch) break;
        if ((ch >= 0x30) && (ch <= 0x39)) { n <<= 4; n |= ch - 0x30; continue; }
        if ((ch >= 0x61) && (ch <= 0x66)) { n <<= 4; n |= ch - 0x61 + 10; continue; }
        if ((ch >= 0x41) && (ch <= 0x46)) { n <<= 4; n |= ch - 0x41 + 10; continue; }
        i--;
        break;
      }
      return n;
    };
    while (i < src.length) {
      const cmd = rdch();
      if (!cmd) break;
      switch (cmd) {
        case 0x74: tid += rdn() + 1; qual = 0; rid = 1; break;
        case 0x71: qual += rdn() + 1; rid = 1; break;
        case 0x73: rid += rdn() + 1; break;
        case 0x72: {
            if ((tid > 0x63) || (qual > 0x3ff) || (rid > 0xffff)) {
              throw new Error(`Invalid res id ${tid}:${qual}:${rid} around ${i}/${src.length} in ROM`);
            }
            const len = rdn();
            const body = new Uint8Array(len);
            let bodyp = 0;
            if (rdch() !== 0x28) throw new Error(`Expected '(' around ${i}/${src.length} in ROM`);
            const buf = [];
            for (;;) {
              const ch = rdch();
              if (ch === 0x29) break;
              if (!ch) throw new Error(`Unclosed resource body`);
                   if ((ch >= 0x41) && (ch <= 0x5a)) buf.push(ch - 0x41);
              else if ((ch >= 0x61) && (ch <= 0x7a)) buf.push(ch - 0x61 + 26);
              else if ((ch >= 0x30) && (ch <= 0x39)) buf.push(ch - 0x30 + 52);
              else if (ch === 0x2b) buf.push(62);
              else if (ch === 0x2f) buf.push(63);
              else throw new Error(`Expected ')' or base64 digit, found ${ch} (${i}/${src.length})`);
              if (buf.length >= 4) {
                body[bodyp++] = (buf[0] << 2) | (buf[1] >> 4);
                body[bodyp++] = (buf[1] << 4) | (buf[2] >> 2);
                body[bodyp++] = (buf[2] << 6) | buf[3];
                buf.splice(0, 4);
              }
            }
            body[bodyp++] = (buf[0] << 2) | (buf[1] >> 4);
            body[bodyp++] = (buf[1] << 4) | (buf[2] >> 2);
            body[bodyp++] = (buf[2] << 6) | buf[3];
            this.resv.push({ tid, qual, rid, v: body });
            rid++;
          } break;
        default: throw new Error(`Unexpected command '${cmd}' around ${i-1}/${src.length} in ROM`);
      }
    }
  }
}
