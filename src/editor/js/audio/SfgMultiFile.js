/* SfgMultiFile.js
 * Splits a text-format SFG file into its component resources, and glues them back together.
 * We are destructive: Comments and formatting will be lost during decode.
 */
 
export class SfgMultiFile {
  constructor(serial) {
    this.text = this.acquireText(serial);
    this.sounds = this.decodeText(this.text); // {id,text}[]: (id) is always a string, (text) is the portion between fences.
  }
  
  acquireText(serial) {
    if (!serial) {
      return "";
    } else if (typeof(serial) === "string") {
      return serial;
    } else if (serial instanceof Uint8Array) {
      return new TextDecoder("utf8").decode(serial);
    } else if (serial instanceof ArrayBuffer) {
      return new TextDecoder("utf8").decode(serial);
    } else if (ArrayBuffer.isView(serial)) {
      return new Textdecoder("utf8").decode(new Uint8Array(serial.buffer, serial.byteOffset, serial.byteLength));
    }
    throw new Error(`Unexpected input for SfgMultiFile`);
  }
  
  decodeText(src) {
    const sounds = [];
    let sound = null;
    for (let srcp=0, lineno=1; srcp<src.length; lineno++) {
      let nlp = src.indexOf("\n", srcp);
      if (nlp < 0) nlp = src.length;
      const line = src.substring(srcp, nlp).split('#')[0].trim();
      srcp = nlp + 1;
      if (!line) continue;
      const words = line.split(/\s+/);
      
      if (words[0] === "end") {
        if (!sound) throw new Error(`${lineno}: Unexpected 'end'`);
        sound = null;
        continue;
      }
      
      if (words[0] === "sound") {
        if (sound) throw new Error(`${lineno}: Expected 'end' before 'sound'`);
        if (words.length !== 2) throw new Error(`${lineno}: One token (ID) is required after 'sound', found ${words.length - 1}`);
        sound = { id: words[1], text: "" };
        sounds.push(sound);
        continue;
      }
      
      if (!sound) {
        throw new Error(`${lineno}: Expected 'sound ID' before '${words[0]}'`);
      }
      
      // We easily could split the sounds' text into commands, we're already there.
      // But lower levels are going to do that on their own, on individual sounds.
      // Simpler for us to keep raw text for each resource.
      sound.text += line + "\n";
    }
    return sounds;
  }
  
  encode() {
    let dst = "";
    for (const { id, text } of this.sounds) {
      dst += `sound ${id}\n`;
      dst += text;
      dst += "end\n\n";
    }
    return new TextEncoder("utf8").encode(dst);
  }
  
  getUnusedId() {//TODO A really fancy version of this would examine other files too. That is pretty hard to arrange.
    let lowestNumericId = 0x10000;
    const numericIds = new Set();
    for (const {id} of this.sounds) {
      const n = +id;
      if (!n) continue;
      numericIds.add(n);
      if (n < lowestNumericId) lowestNumericId = n;
    }
    if (lowestNumericId >= 0x10000) { // We have no numeric IDs. Propose "1".
      return "1";
    }
    for (let id=lowestNumericId+1; ; id++) {
      if (!numericIds.has(id)) return id.toString();
    }
  }
}
