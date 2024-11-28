/* GenerateSaveModal.js
 * For making saved games.
 */
 
import { Dom } from "./Dom.js";

export class GenerateSaveModal {
  static getDependencies() {
    return [HTMLDialogElement, Dom, Window];
  }
  constructor(element, dom, window) {
    this.element = element;
    this.dom = dom;
    this.window = window;
    
    this.model = GenerateSaveModal.defaultModel();
    this.encoded = GenerateSaveModal.encode(this.model);
    
    /* Load flag names on the first instantiation.
     * Defer UI construction until that completes.
     */
    if (GenerateSaveModal.FLAG_NAMES) {
      this.buildUi();
    } else {
      GenerateSaveModal.FLAG_NAMES = [];
      this.window.fetch("/game/flag_names.h").then(rsp => {
        if (!rsp.ok) throw rsp;
        return rsp.text();
      }).then(src => {
        GenerateSaveModal.FLAG_NAMES = this.parseFlagNames(src);
        this.buildUi();
      }).catch(e => {
        this.window.console.error("Failed to load sprite type names.", e);
        this.buildUi();
      });
    }
  }
  
  parseFlagNames(src) {
    const names = [];
    for (let srcp=0, lineno=1; srcp<src.length; lineno++) {
      let nlp = src.indexOf("\n", srcp);
      if (nlp < 0) nlp = src.length;
      const line = src.substring(srcp, nlp).trim();
      srcp = nlp + 1;
      
      const match = line.match(/^#define\s+FLAG_([a-zA-Z0-9_]+)\s+(\d+)/);
      if (!match) continue;
      const name = match[1];
      const flagid = +match[2];
      if (isNaN(flagid) || (flagid < 0) || (flagid > 0xff)) throw new Error(`flag_names.h:${lineno}: Invalid flag id ${JSON.stringify(match[2])}`);
      while (names.length <= flagid) names.push("");
      names[flagid] = name;
    }
    return names;
  }
  
  buildUi() {
    this.element.innerHTML = "";
    const table = this.dom.spawn(this.element, "TABLE");
    
    this.dom.spawn(table, "TR",
      this.dom.spawn(null, "TD", ["key"], "Encoded"),
      this.dom.spawn(null, "TD", ["value"], { colspan: 5 },
        this.dom.spawn(null, "INPUT", {
          type: "text",
          name: "encoded",
          value: this.encoded,
          "on-input": e => this.onEncodedInput(e.target.value),
        })
      )
    );
    
    for (let i=0; ; i++) {
      const scalark = GenerateSaveModal.SCALAR_NAMES[i];
      const invk = GenerateSaveModal.INVENTORY[i];
      const flagk = GenerateSaveModal.FLAG_NAMES[i];
      if (!scalark && !invk && !flagk) break;
      const tr = this.dom.spawn(table, "TR");
      
      if (scalark) {
        this.dom.spawn(tr, "TD", ["key"], scalark);
        this.dom.spawn(tr, "TD", ["value"],
          this.dom.spawn(null, "INPUT", {
            name: scalark,
            type: "text", // All except 'bestword' are integers, but meh. Easier if they're all the same thing.
            value: this.model[scalark] || '',
            "on-input": () => this.onLooseInput(),
          })
        );
      } else {
        this.dom.spawn(tr, "TD");
        this.dom.spawn(tr, "TD");
      }
      
      if (invk) {
        this.dom.spawn(tr, "TD", ["key"], invk);
        this.dom.spawn(tr, "TD", ["value"],
          this.dom.spawn(null, "INPUT", {
            name: invk,
            type: "number",
            min: 0,
            max: 99,
            value: this.model.inventory[i] || 0,
            "on-input": () => this.onLooseInput(),
            "data-itemid": i,
          })
        );
      } else {
        this.dom.spawn(tr, "TD");
        this.dom.spawn(tr, "TD");
      }
      
      if (flagk) {
        let box;
        this.dom.spawn(tr, "TD", ["key"], flagk);
        this.dom.spawn(tr, "TD", ["value"],
          box = this.dom.spawn(null, "INPUT", {
            name: flagk,
            type: "checkbox",
            "on-input": () => this.onLooseInput(),
            "data-flagid": i,
          })
        );
        box.checked = !!this.model.flags[i];
      } else {
        this.dom.spawn(tr, "TD");
        this.dom.spawn(tr, "TD");
      }
    }
  }
  
  onEncodedInput(src) {
    this.encoded = src;
    this.decode(src);
  }
  
  /* Input on any field other than 'encoded'.
   * Read the whole loose form, encode it, write to 'encoded'.
   */
  onLooseInput() {
    let encodedElement = null;
    for (const element of this.element.querySelectorAll("input")) {
      if (!element.name) continue;
      if (element.name === "encoded") {
        encodedElement = element;
        continue;
      }
      // Luckily (itemid) and (flagid) can't be zero, so we don't need to distinguish zero from unset.
      const itemid = +element.getAttribute("data-itemid");
      if (itemid) {
        const v = +element.value;
        if (isNaN(v) || (v < 0) || (v > 99)) throw new Error(`Inventory must be in 0..99; found ${JSON.stringify(element.value)} for item ${itemid}`);
        while (this.model.inventory.length <= itemid) this.model.inventory.push(0);
        this.model.inventory[itemid] = +element.value;
        continue;
      }
      const flagid = +element.getAttribute("data-flagid");
      if (flagid) {
        while (this.model.flags.length <= flagid) this.model.flags.push(false);
        this.model.flags[flagid] = element.checked;
        continue;
      }
      const v = +element.value;
      if (isNaN(v)) this.model[element.name] = element.value;
      else this.model[element.name] = v;
    }
    this.encoded = GenerateSaveModal.encode(this.model);
    if (encodedElement) encodedElement.value = this.encoded;
  }
  
  /* Populate the loose UI fields and loose model from this encoded save.
   */
  decode(src) {
    this.model = GenerateSaveModal.decode(src);
    for (const k of GenerateSaveModal.SCALAR_NAMES) {
      const input = this.element.querySelector(`input[name='${k}']`);
      if (input) input.value = this.model[k];
    }
    for (const element of this.element.querySelectorAll("input[data-itemid]")) {
      element.value = this.model.inventory[+element.getAttribute("data-itemid")] || 0;
    }
    for (const element of this.element.querySelectorAll("input[data-flagid]")) {
      element.checked = !!this.model.flags[+element.getAttribute("data-flagid") || 0];
    }
  }
  
  /* Encode, static.
   * Models contain every field in SCALAR_NAMES, plus 'inventory' and 'flags'.
   **************************************************************************/
  
  static defaultModel() {
    const model = GenerateSaveModal.SCALAR_NAMES.reduce((a, v) => { a[v] = 0; return a; }, {});
    model.hp = 100;
    model.bestword = "";
    model.inventory = GenerateSaveModal.INVENTORY.map(v => 0);
    model.flags = [false, true]; // Don't assume that the flag names are populated yet.
    return model;
  }
   
  static encode(model) {
    
    /* Trim trailing items and flags, and calculate the full output length.
     */
    let itemc = model.inventory.length;
    while (itemc && !model.inventory[itemc - 1]) itemc--;
    let flagc = model.flags.length;
    while (flagc && !model.flags[flagc - 1]) flagc--;
    const flagbc = (flagc + 7) >> 3;
    if ((itemc > 0xff) || (flagbc > 0xff)) throw new Error(`Too many items or flags (${itemc}, ${flagc})`);
    let total = 37; // Fixed fields, including checksum and item and flag counts.
    total += itemc;
    total += flagbc;
    const mod3 = total % 3; // Binary length must be a multiple of 3.
    if (mod3) total += 3 - mod3;
    
    /* Create the binary buffer.
     */
    const bin = new Uint8Array(total);
    
    /* Fixed scalars.
     */
    bin[4] = model.hp;
    bin[5] = model.xp >> 8;
    bin[6] = model.xp;
    bin[7] = model.gold >> 8;
    bin[8] = model.gold;
    bin[9] = model.gravep;
    bin[10] = model.playtime >> 16;
    bin[11] = model.playtime >> 8;
    bin[12] = model.playtime;
    bin[13] = model.battlec >> 16;
    bin[14] = model.battlec >> 8;
    bin[15] = model.battlec;
    bin[16] = model.wordc >> 16;
    bin[17] = model.wordc >> 8;
    bin[18] = model.wordc;
    bin[19] = model.scoretotal >> 16;
    bin[20] = model.scoretotal >> 8;
    bin[21] = model.scoretotal;
    bin[22] = model.bestscore;
    bin[23] = model.bestword.charCodeAt(0) || 0;
    bin[24] = model.bestword.charCodeAt(1) || 0;
    bin[25] = model.bestword.charCodeAt(2) || 0;
    bin[26] = model.bestword.charCodeAt(3) || 0;
    bin[27] = model.bestword.charCodeAt(4) || 0;
    bin[28] = model.bestword.charCodeAt(5) || 0;
    bin[29] = model.bestword.charCodeAt(6) || 0;
    bin[30] = model.stepc >> 16;
    bin[31] = model.stepc >> 8;
    bin[32] = model.stepc;
    bin[33] = model.flower_stepc;
    bin[34] = model.bugspray;
    
    /* Inventory and flags are variable length but also not complicated.
     */
    let binp = 35;
    bin[binp++] = itemc;
    for (let i=0; i<itemc; i++) bin[binp++] = model.inventory[i];
    bin[binp++] = flagbc;
    for (let i=0, flagp=0; i<flagbc; i++) { // model.flags are bits little-endianly.
      let byte=0, mask=0x01;
      for (; mask<0x100; mask<<=1, flagp++) {
        if (model.flags[flagp]) byte |= mask;
      }
      bin[binp++] = byte;
    }
    
    /* Compute the checksum and write it to the first 4 bytes of output.
     */
    let checksum = 0xc396a5e7;
    for (let i=4; i<total; i++) {
      checksum = (checksum << 7) | (checksum >>> 25);
      checksum ^= bin[i];
    }
    bin[0] = checksum >> 24;
    bin[1] = checksum >> 16;
    bin[2] = checksum >> 8;
    bin[3] = checksum;
    
    /* XOR each byte against the previous filtered value (ie front to back).
     */
    for (let i=1; i<total; i++) bin[i] ^= bin[i - 1];
    
    /* Encode base64ish to a new buffer.
     * Binary length must be a multiple of 3, so there's never a partial unit.
     * Alphabet is sequential enough that we could do it arithmetically, but it's still easier to use a 64-character lookup string.
     * 0x23..0x5b => 0..56
     * 0x5d..0x63 => 57..63
     */
    const alphabet = "#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[]^_`abc";
    const text = new Uint8Array((total * 4) / 3);
    for (let binp=0, textp=0; binp<total; ) {
      const a = bin[binp++];
      const b = bin[binp++];
      const c = bin[binp++];
      text[textp++] = alphabet.charCodeAt(a >> 2);
      text[textp++] = alphabet.charCodeAt(((a << 4) | (b >> 4)) & 0x3f);
      text[textp++] = alphabet.charCodeAt(((b << 2) | (c >> 6)) & 0x3f);
      text[textp++] = alphabet.charCodeAt(c & 0x3f);
    }
    
    /* And that's it. Read that as ASCII and we're done.
     */
    return new TextDecoder("ascii").decode(text);
  }
  
  /* Decode, static.
   * Here's a real save for testing:
   *   JR[&0#;)4WBM^QNM^QNM^QNM^QNM^QNM^QNM^QNM^QNM_OV/6(06
   *****************************************************************************/
   
  static decode(src) {
    const model = GenerateSaveModal.defaultModel();
    
    /* Validate length and decode base64ish.
     */
    if (src.length & 3) throw new Error(`Invalid length`);
    const total = (src.length * 3) / 4;
    if (total < 37) throw new Error(`Invalid length`);
    const bin = new Uint8Array(total);
    const un64 = v => {
      if ((v >= 0x23) && (v <= 0x5b)) return v - 0x23;
      if ((v >= 0x5d) && (v <= 0x63)) return v - 0x5d + 57;
      throw new Error(`Illegal byte ${v}`);
    };
    for (let srcp=0, binp=0; binp<total; ) {
      const a = un64(src.charCodeAt(srcp++));
      const b = un64(src.charCodeAt(srcp++));
      const c = un64(src.charCodeAt(srcp++));
      const d = un64(src.charCodeAt(srcp++));
      bin[binp++] = (a << 2) | (b >> 4);
      bin[binp++] = (b << 4) | (c >> 2);
      bin[binp++] = (c << 6) | d;
    }
    
    /* XOR each byte against the previous filtered value (ie back to front).
     */
    for (let i=total; i-->1; ) bin[i] ^= bin[i - 1];
    
    /* Compute and assert checksum.
     */
    let checksum = 0xc396a5e7;
    for (let i=4; i<total; i++) {
      checksum = (checksum << 7) | (checksum >>> 25);
      checksum ^= bin[i];
    }
    const expecksum = (bin[0] << 24) | (bin[1] << 16) | (bin[2] << 8) | bin[3];
    if (checksum !== expecksum) throw new Error(`Checksum mismatch`);
    
    /* Read fixed header.
     */
    let binp = 4;
    model.hp = bin[binp++];
    model.xp = (bin[binp] << 8) | bin[binp+1]; binp += 2;
    model.gold = (bin[binp] << 8) | bin[binp+1]; binp += 2;
    model.gravep = bin[binp++];
    model.playtime = (bin[binp] << 16) | (bin[binp+1] << 8) | bin[binp+2]; binp += 3;
    model.battlec = (bin[binp] << 16) | (bin[binp+1] << 8) | bin[binp+2]; binp += 3;
    model.wordc = (bin[binp] << 16) | (bin[binp+1] << 8) | bin[binp+2]; binp += 3;
    model.scoretotal = (bin[binp] << 16) | (bin[binp+1] << 8) | bin[binp+2]; binp += 3;
    model.bestscore = bin[binp++];
    model.bestword = new TextDecoder("ascii").decode(bin.slice(binp, binp+7).filter(v => v)); binp += 7;
    model.stepc = (bin[binp] << 16) | (bin[binp+1] << 8) | bin[binp+2]; binp += 3;
    model.flower_stepc = bin[binp++];
    model.bugspray = bin[binp++];
    
    /* Items are a 1-byte count followed by 1 byte each.
     */
    const itemc = bin[binp++] || 0;
    for (let i=itemc; i-->0; ) {
      model.inventory.push(bin[binp++])
    }
    
    /* Flags are a 1-byte byte count followed by little-endian bits.
     */
    const flagc = bin[binp++] || 0;
    for (let i=flagc; i-->0; ) {
      const flagbits = bin[binp++];
      for (let mask=1; mask<0x100; mask<<=1) {
        model.flags.push(!!(flagbits & mask));
      }
    }
    
    /* Validate aggressively.
     */
    if (binp > total) throw new Error(`Inventory or flags overruns content (${binp}>${total})`);
    if ((model.hp < 1) || (model.hp > 100)) throw new Error(`Invalid HP ${model.hp}`);
    if (model.xp > 32767) throw new Error(`Invalid XP ${model.xp}`);
    if (model.gold > 32767) throw new Error(`Invalid Gold ${model.gold}`);
    for (let i=model.inventory.length; i-->0; ) {
      const c = model.inventory[i];
      if (c > 99) throw new Error(`Invalid inventory[${i}] ${c}`);
    }
    if (model.inventory[0]) throw new Error(`Invalid inventory[0] ${model.inventory[0]}`);
    if (model.flags[0] || !model.flags[1]) throw new Error(`Invalid constant flags (${model.flags[0]}, ${model.flags[1]})`);
    
    return model;
  }
}

GenerateSaveModal.FLAG_NAMES = null; // See ctor. Array indexed by flagid, value is the simple name.

GenerateSaveModal.INVENTORY = [
  "NOOP",
  "BUGSPRAY",
  "UNFAIRIE",
  "2XWORD",
  "3XWORD",
  "ERASER",
];

GenerateSaveModal.SCALAR_NAMES = [
  "hp",
  "xp",
  "gold",
  "gravep",
  "playtime",
  "battlec",
  "wordc",
  "scoretotal",
  "bestscore",
  "bestword",
  "stepc",
  "flower_stepc",
  "bugspray",
];
