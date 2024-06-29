/* MetadataEditor.js
 */
 
import { Dom } from "./Dom.js";
import { Resmgr } from "./Resmgr.js";

//TODO Incorporate the "*String" variations more meaningfully.

export class MetadataEditor {
  static getDependencies() {
    return [HTMLElement, Dom, Resmgr];
  }
  constructor(element, dom, resmgr) {
    this.element = element;
    this.dom = dom;
    this.resmgr = resmgr;
    
    this.serial = [];
    this.path = "";
    
    this.buildUi();
    this.element.addEventListener("change", () => this.validateAll());
    this.element.addEventListener("input", () => this.resmgr.dirty(this.path, () => this.encode()));
  }
  
  setup(serial, path) {
    this.serial = serial;
    this.path = path;
    this.populateUi();
  }
  
  buildUi() {
    this.element.innerHTML = "";
    const table = this.dom.spawn(this.element, "TABLE");
    const trh = this.dom.spawn(table, "TR");
    this.dom.spawn(trh, "TH", "Key");
    this.dom.spawn(trh, "TH", "String");
    this.dom.spawn(trh, "TH", "Value");
    
    for (const [key, stringable, validate, help] of MetadataEditor.STANDARD_FIELDS) {
      const tr = this.dom.spawn(table, "TR", ["standard"], { "data-key": key });
      const tdk = this.dom.spawn(tr, "TD", ["key"], key);
      if (help) {
        tdk.classList.add("help");
        tdk.addEventListener("click", () => this.showHelp(help));
      }
      const tds = this.dom.spawn(tr, "TD", ["string"]);
      const sbtn = this.dom.spawn(tds, "INPUT", { type: "button", value: "--", name: key + "String", "on-click": () => this.editString(key) });
      if (!stringable) sbtn.disabled = true;
      const tdv = this.dom.spawn(tr, "TD", ["value"]);
      this.dom.spawn(tdv, "INPUT", { type: "text", name: key });
      this.dom.spawn(tdv, "DIV", ["validationError"]);
    }
    const tr = this.dom.spawn(table, "TR", ["lastRow"]);
    const td = this.dom.spawn(tr, "TD");
    this.dom.spawn(td, "INPUT", { type: "button", value: "+", "on-click": () => this.onAddRow() });
  }
  
  populateUi() {
    this.clearUi();
    this.forEachField((k, v) => this.populateField(k, v));
    this.validateAll();
  }
  
  forEachField(cb) {
    const src = new TextDecoder("utf8").decode(this.serial);
    for (let srcp=0, lineno=1; srcp<src.length; lineno++) {
      let nlp = src.indexOf("\n", srcp);
      if (nlp < 0) nlp = src.length;
      const line = src.substring(srcp, nlp).trim();
      srcp = nlp + 1;
      if (!line || line.startsWith("#")) continue;
      let eqp = line.indexOf("="); if (eqp < 0) eqp = line.length;
      let clp = line.indexOf(":"); if (clp < 0) clp = line.length;
      const sepp = Math.min(eqp, clp);
      const k = line.substring(0, sepp).trim();
      const v = line.substring(sepp + 1).trim();
      cb(k, v, lineno);
    }
  }
  
  clearUi() {
    for (const tr of this.element.querySelectorAll("tr.nonstandard")) {
      tr.remove();
    }
    for (const input of this.element.querySelectorAll("td.value input")) {
      input.value = "";
    }
    for (const button of this.element.querySelectorAll("td.string input")) {
      button.value = "--";
    }
    for (const err of this.element.querySelectorAll(".validationError")) {
      err.innerHTML = "";
    }
  }
  
  /* Create a new row if needed, and replace key and value in UI.
   * Does not run validation or touch validationError.
   */
  populateField(k, v) {
    if (k.endsWith("String")) {
      const pk = k.substring(0, k.length - 6);
      let tr = this.element.querySelector(`tr[data-key='${pk}']`);
      if (!tr) tr = this.spawnRow(pk);
      tr.querySelector(`input[name='${k}']`).value = v;
      return;
    }
    if (k.match(/^[a-z][a-zA-Z0-9_]*$/)) {
      const tr = this.element.querySelector(`tr[data-key='${k}']`);
      if (tr) {
        tr.querySelector("td.key").innerText = k;
        tr.querySelector("td.value input").value = v;
        return;
      }
    }
    const tr = this.spawnRow(k);
    tr.querySelector("td.value input").value = v;
  }
  
  spawnRow(k) {
    const tr = this.dom.spawn(null, "TR", ["nonstandard"], { "data-key": k });
    this.dom.spawn(tr, "TD", ["key"], k);
    const tds = this.dom.spawn(tr, "TD", ["string"]);
    this.dom.spawn(tds, "INPUT", { type: "button", value: "---", name: k + "String", "on-click": () => this.editString(k) });
    const tdv = this.dom.spawn(tr, "TD", ["value"]);
    this.dom.spawn(tdv, "INPUT", { type: "text", name: k, value: "" });
    this.dom.spawn(tdv, "DIV", ["validationError"]);
    const table = this.element.querySelector("table");
    const lastRow = this.element.querySelector("tr.lastRow");
    table.insertBefore(tr, lastRow);
  }
  
  editString(k) {
    // It would be nice of us to present a filterable list of strings, or at least validate the string id at some point.
    // But that is much more complicated.
    const btn = this.element.querySelector(`tr[data-key='${k}'] td.string input`);
    const pv = (btn.value === "--") ? "" : btn.value;
    this.dom.modalInput("ID of string resource to override natural value:", pv).then(v => {
      if (!btn) return;
      btn.value = v.trim() || "--";
      this.resmgr.dirty(this.path, () => this.encode());
    }).catch(e => { if (e) this.bus.broadcast({ type: "error", error: e }); });
  }
  
  encode() {
  
    /* Gather new values.
     */
    const content = {};
    for (const tr of this.element.querySelectorAll("tr.standard,tr.nonstandard")) {
      const k = tr.querySelector("td.key").innerText.trim();
      const v = tr.querySelector("td.value input").value.trim();
      if (v) {
        content[k] = v;
      }
      const stringid = tr.querySelector("td.string input").value.trim();
      if (stringid && (stringid !== "--")) {
        content[k + "String"] = stringid;
      }
    }
    
    /* Read the previous encoded content, preserve blanks and comments, replace existing fields, and drop deleted ones.
     */
    let dst = "";
    const src = new TextDecoder("utf8").decode(this.serial);
    for (let srcp=0, lineno=1; srcp<src.length; lineno++) {
      let nlp = src.indexOf("\n", srcp);
      if (nlp < 0) nlp = src.length;
      const line = src.substring(srcp, nlp).trim();
      srcp = nlp + 1;
      if (!line || line.startsWith("#")) {
        dst += line + "\n";
        continue;
      }
      let eqp = line.indexOf("="); if (eqp < 0) eqp = line.length;
      let clp = line.indexOf(":"); if (clp < 0) clp = line.length;
      const sepp = Math.min(eqp, clp);
      const k = line.substring(0, sepp).trim();
      const v = line.substring(sepp + 1).trim();
      if (k in content) {
        dst += k + "=" + content[k] + "\n";
        delete content[k];
      }
    }
    
    /* Any fields remaining in content are new -- append them.
     */
    for (const k of Object.keys(content)) {
      dst += k + "=" + content[k] + "\n";
    }
    
    const serial = new TextEncoder("utf8").encode(dst);
    this.serial = serial;
    return serial;
  }
  
  validateAll() {
    for (const tr of this.element.querySelectorAll("tr.standard,tr.nonstandard")) {
      const k = tr.getAttribute("data-key");
      const v = (tr.querySelector("td.value input").value || "").trim();
      if (v.length > 0xff) {
        tr.querySelector(".validationError").innerText = `Illegal length ${v.length}, limit 255.`;
      } else {
        const def = MetadataEditor.STANDARD_FIELDS.find(f => f[0] === k);
        if (def && def[2]) {
          tr.querySelector(".validationError").innerText = (def[2])(v) || "";
        } else {
          tr.querySelector(".validationError").innerText = "";
        }
      }
    }
  }
  
  onAddRow() {
    this.dom.modalInput("Key:", "").catch(() => "").then(key => {
      key = key.trim();
      if (!key) return;
      this.populateField(key, "");
      this.validateAll();
    });
  }
  
  showHelp(text) {
    this.dom.modalMessage(text);
  }
}

/* Validators.
 * These return false on success, or a string describing the violation.
 * Input should be sanitized in advance (eg trim space and assert length<256).
 * Validation errors are not fatal, user can save anyway. So it's OK to issue advisories in addition to real errors.
 ********************************************************************************/
 
function validateTitle(src) {
  if (!src) return "Title is strongly recommended.";
  return 0;
}
 
function validateLanguage(src) {
  if (!src) return "Declaring a preferred language is recommended.";
  for (const token of src.split(',').map(s => s.trim())) {
    if (!token.match(/^[a-z]{2}$/)) {
      return `Invalid language ${JSON.stringify(token)}, must be a 2-letter ISO 639 code.`;
    }
    // We could check against string resources, advise them only to prefer languages they actually have.
    // Not sure that would come up often enough to care.
  }
  return 0;
}

function validateFramebuffer(src) {
  if (!src) return "Required. ('WIDTHxHEIGHT [gl]')";
  const match = src.match(/^(\d+)x(\d+)(\s.*)?$/);
  if (!match) return "Malformed. 'WIDTHxHEIGHT [gl]'";
  const w = +match[1];
  const h = +match[2];
  if ((w < 1) || (w > 4096) || (h < 1) || (h > 4096)) {
    return "Width and height must be in 1..4096";
  }
  for (const token of (match[3] || "").split(/\s+/)) {
    if (!token) ;
    else if (token === "gl") ;
    else return `WARNING: Unknown framebuffer token ${JSON.stringify(token)}`;
  }
  return 0;
}

function validateIconImage(src) {
  if (!src) return "Recommended. Should be 16x16 with alpha.";
  const rid = +src;
  if (!rid || (rid < 1) || (rid > 0xffff)) return "Expected decimal integer in 1..65535.";
  //TODO Confirm image resource exists. Resmgr doesn't do that yet, and it's more complicated than it sounds.
  return 0;
}

function validateRating(src) {
  return 0; // TODO Format still needs to be defined.
}

function validateTimestamp(src) {
  if (!src) return 0; // Undeclared is fine.
  if (src.match(/^\d{4}(-\d{2}(-\d{2}(T\d{2}(:\d{2}(:\d{2}(\.\d+)?)?)?)?)?)?$/)) return 0;
  return "Expected ISO 8601, eg 'YYYY' or 'YYYY-MM-DD'";
}

function validatePosterImage(src) {
  if (!src) return 0; // Entirely optional.
  const rid = +src;
  if (!rid || (rid < 1) || (rid > 0xffff)) return "Expected decimal integer in 1..65535.";
  //TODO Confirm image resource exists. Resmgr doesn't do that yet, and it's more complicated than it sounds.
  return 0;
}

function validateUrl(src) {
  if (!src) return 0;
  //TODO Is it worth validating URLs?
  return 0;
}

function validateFreedom(src) {
  switch (src) {
    case "restricted":
    case "limited":
    case "intact":
    case "free":
    case "": // Empty equivalent to "limited".
      return 0;
  }
  return "Must be one of: 'restricted' 'limited' 'intact' 'free', default 'limited'";
}

function validateRequire(src) {
  if (!src) return 0;
  //TODO Define tokens.
  return 0;
}

function validatePlayers(src) {
  if (!src) return "Recommended to declare. Integer or 'lo..hi'";
  const n = +src;
  if (n > 16) return "Improbably high. Are you sure?";
  if (n > 1) return `Use 'lo..hi' if other counts are supported. As stated, this means "exactly ${n} players only".`;
  if (n === 1) return;
  const match = src.match(/^(\d+)..(\d+)$/);
  if (!match) return "Must be Integer or 'lo..hi'";
  const lo = +match[1];
  const hi = +match[2];
  if (!lo || !hi) return "Must be Integer or 'lo..hi'";
  if ((lo < 1) || (lo > hi)) return "Invalid range.";
  if (hi > 16) return "Improbably high. Are you sure?";
  return 0;
}

/* [key,stringable,validator,help] for the fields defined in our docs.
 * Loose user-defined keys are also allowed of course.
 */
MetadataEditor.STANDARD_FIELDS = [
  ["title", true, validateTitle, "For display to user. Recommend full name, in title case."],
  ["language", false, validateLanguage, "Two-letter ISO 639 language codes, comma delimited, in order of your preference."],
  ["framebuffer", false, validateFramebuffer, "'WIDTHxHEIGHT [gl]'. Required."],
  ["iconImage", false, validateIconImage, "ID of image resource. Recommend 16x16 with alpha."],
  ["author", true, null, "Your name, or your company's. For display to user."],
  ["copyright", true, null, "Recommend eg: '(c) 2024 AK Sommerville'"],
  ["description", true, null, "Freeform text describing the game."],
  ["genre", true, null, "Recommend picking from those used by itch.io, just so there's a common reference."],
  ["tags", true, null, "Comma-delimited strings. Recommend using ones seen on itch.io, just so there's a common reference."],
  ["advisory", true, null, "Call out any potentially objectionable material. Nudity, profanity, etc."],
  ["rating", false, validateRating, "Machine-readable declaration of ratings from official agencies. TODO Format unspecified."],
  ["timestamp", false, validateTimestamp, "ISO 8601 eg 'YYYY-MM-DD' or 'YYYY'"],
  ["version", false, null, "Version of your game (not the platform version). Recommend semver eg 'v1.2.3'"],
  ["magic", false, null, "Used in preference to 'version' for matching save states. Content can be any text."],
  ["posterImage", false, validatePosterImage, "ID of image resource. TODO Recommend dimensions."],
  ["url", false, validateUrl, "Project or author's home page."],
  ["contact", false, null, "Your email, phone, or whatever. Displays verbatim to the user."],
  ["freedom", false, validateFreedom, "Simple summary of license terms. 'restricted', 'limited', 'intact', 'free'. Default 'limited'. Games marked 'intact' or 'free' may be redistributed."],
  ["require", false, validateRequire, "Comma-delimited tokens for required platform features. TODO Define these tokens."],
  ["players", false, validatePlayers, "'N' or 'N..N', how many can play at a time. '1..2' is more common than '2', which would mean 'ONLY two players, not one'."],
];
