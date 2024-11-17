/* PoiModal.js
 * Originally it was just for POI, which are a subset of map command.
 * I've extended its writ to include all commands, and adding static methods to make this class the map command authority.
 */

import { Dom } from "./Dom.js";
import { MapPaint } from "./MapPaint.js";
import { MapCommand } from "./MapRes.js";
import { Data } from "./Data.js";

export class PoiModal {
  static getDependencies() {
    return [HTMLDialogElement, Dom, MapPaint, Data];
  }
  constructor(element, dom, mapPaint, data) {
    this.element = element;
    this.dom = dom;
    this.mapPaint = mapPaint;
    this.data = data;
    
    this.poi = null;
    this.command = "";
    this.remotePath = "";
    this.selfContained = false; // true if we take a string and return a string; false to interact with MapPaint.map.
    this.literal = false; // true to skip semantic analysis.
  }
  
  /* A bunch of ways to set up. You must pick exactly one.
   ******************************************************************************/
  
  setup(poi) {
    this.selfContained = false;
    this.poi = poi;
    const command = this.mapPaint.map?.commands.find(c => c.id === poi.mapCommandId);
    if (command) this.command = command.kw + " " + command.args.join(" ");
    else this.command = "";
    this.buildUi();
  }
  
  setupRemote(poi, path, map) {
    this.selfContained = false;
    this.poi = poi;
    this.remotePath = path;
    this.remoteMap = map;
    const command = map?.commands.find(c => c.id === poi.mapCommandId);
    if (command) this.command = command.kw + " " + command.args.join(" ");
    else this.command = "";
    this.buildUi();
  }
  
  setupNew(x, y) {
    this.selfContained = false;
    this.poi = { x, y };
    this.command = `KEYWORD @${x},${y}`;
    this.buildUi();
  }
  
  setupText(src) {
    this.selfContained = true;
    this.poi = { x: 0, y: 0 };
    this.command = src;
    this.buildUi();
  }
  
  /* Generic UI.
   *********************************************************************************/
  
  buildUi() {
    this.element.innerHTML = "";
    if (this.remotePath) this.dom.spawn(this.element, "DIV", `Note: Editing command for map ${this.remotePath}`);
    const form = this.dom.spawn(this.element, "FORM", { "on-submit": e => { e.stopPropagation(); e.preventDefault(); } });
    const words = this.command.split(/\s+/g).filter(v => v);
    this.buildForm(form, words);
    if (!this.element.querySelector("input[name='literal']")) {
      this.dom.spawn(form, "INPUT", { type: "button", value: "Edit literally", "on-click": () => {
        this.literal = true;
        this.buildUi();
      }});
    }
    this.dom.spawn(form, "INPUT", { type: "submit", value: "Save", "on-click": e => this.onSave(e) });
  }
  
  onSave(event) {
    event.preventDefault();
    if (!this.poi) return;
    const text = this.getTextFromUi();
    
    // Self-contained, we just return the text without touching any globals.
    if (this.selfContained) {
      this.resolve(text);
    
    // Existing command? Including existing commands on remote maps.
    } else if (this.poi.mapCommandId) {
      const map = this.remoteMap || this.mapPaint.map;
      const command = map.commands.find(c => c.id === this.poi.mapCommandId);
      if (command) {
        command.decode(text);
        this.mapPaint.broadcast({ id: "cellsDirty" });
        this.resolve();
      }
    
    // New command.
    } else {
      const command = new MapCommand(text);
      command.id = this.mapPaint.map.nextCommandId++;
      this.mapPaint.map.commands.push(command);
      this.mapPaint.broadcast({ id: "cellsDirty" });
      this.resolve();
    }
  }
  
  getTextFromUi() {
  
    // If we have a "literal" input, that's the whole answer.
    const literal = this.element.querySelector("input[name='literal']");
    if (literal) return literal.value;
    
    // Everything else is going to need the command split up.
    const words = this.command.split(/\s+/g).filter(v => v);
    
    // Look for the super-specific modes. They each have an unambiguous parent node.
    let parent;
    if (parent = this.element.querySelector(".kitchen")) {
      let params = 0;
      for (const checkbox of parent.querySelectorAll("input[type='checkbox']:checked")) params |= +checkbox.value;
      return `${words[0]} ${words[1]} ${words[2]} 0x${params.toString(16).padStart(8, '0')}`;
    }
    if (parent = this.element.querySelector(".merchant")) {
      let params = 0;
      for (const checkbox of parent.querySelectorAll("input[type='checkbox']:checked")) params |= +checkbox.value;
      return `${words[0]} ${words[1]} ${words[2]} 0x${params.toString(16).padStart(8, '0')}`;
    }
    if (parent = this.element.querySelector("table[name='spriteParams:customer']")) {
      const stringsid = parent.querySelector("input[name='stringsid']").value;
      const index = parent.querySelector("input[name='index']").value;
      return `${words[0]} ${words[1]} ${words[2]} ${stringsid} ${index}`;
    }
    if (parent = this.element.querySelector("table[name='spriteParams:foe']")) {
      const stringsid = parent.querySelector("input[name='flag']").value;
      const index = parent.querySelector("input[name='reserved']").value;
      return `${words[0]} ${words[1]} ${words[2]} ${flag} ${reserved}`;
    }
    
    // Everything else must have been built from the schema.
    const schema = PoiModal.COMMANDS[words[0]];
    if (schema) {
      const dst = schema.map(([len, name]) => this.element.querySelector(`input[name='${name}']`).value);
      dst.splice(0, 0, words[0]);
      return dst.join(" ");
    }
    
    throw new Error(`PoiModal unable to find its own ass.`);
  }
  
  /* Compose UI appropriate to the given command, split on tokens.
   * If we don't recognize the keyword, or parameters are malformed, we produce the generic "literal" input.
   */
  buildForm(form, words) {
    if (!this.literal) {
    
      // Certain known commands get highly specific UI.
      switch (words[0]) {
        case "sprite": if (this.buildFormSprite(form, words)) return; break;
        case "message": if (this.buildFormMessage(form, words)) return; break;
      }
    
      // Can we apply a schema generically?
      const schema = PoiModal.COMMANDS[words[0]];
      if (schema) {
        if (words.length - 1 === schema.length) {
          this.dom.spawn(form, "DIV", `Map command ${JSON.stringify(words[0])}`);
          const table = this.dom.spawn(form, "TABLE");
          for (let i=0; i<schema.length; i++) {
            const k = schema[i][1];
            const v = words[1 + i];
            const tr = this.dom.spawn(table, "TR", this.dom.spawn(null, "TD", ["key"], k));
            const tdv = this.dom.spawn(tr, "TD", this.dom.spawn(null, "TD", ["value"]));
            this.dom.spawn(tdv, "INPUT", { type: "text", name: k, value: v });
          }
          return;
        }
      }
    }
    // Finally, we can always present it as literal text.
    this.dom.spawn(form, "INPUT", { name: "literal", value: this.command });
  }
  
  /* If (words) is well formed, add fields to (form) and return true.
   * Returns false if you should try other options.
   */
  buildFormSprite(form, words) {
    if (words[0] !== "sprite") return false;
    if (words.length < 3) return false; // Must be at least ["sprite", spriteid, pos]
    if (words.length > 7) return false; // [spriteParams] is 0..4 tokens.
    
    // Find the sprite resource and decode it.
    const res = this.data.resByString(words[1], "sprite");
    if (!res) return false;
    const src = new TextDecoder("utf8").decode(res.serial);
    const sprite = {};
    for (const line of src.split("\n")) {
      const lineWords = line.trim().split(/\s+/g);
      if (!lineWords[0]) continue;
      sprite[lineWords[0]] = lineWords.slice(1).join(" ");
    }
    
    /* We'll do special UI for a few known types.
     * When we do this, we don't create UI for keyword, spriteid, or position.
     * That's OK. The user can "Edit literally" to get those back.
     */
    switch (sprite.type) {
    
      case "kitchen": {
          if (words.length !== 4) return false; // Must be a single 32-bit integer at the end.
          const param = +words[3];
          if (isNaN(param)) return false;
          this.dom.spawn(form, "DIV", `Kitchen sprite at ${words[2]}`);
          const pantry = this.dom.spawn(form, "DIV", ["kitchen"]);
          let mask = 1;
          for (const item of PoiModal.KITCHEN_ITEMS) {
            let checkbox;
            this.dom.spawn(pantry, "LABEL", item.name,
              checkbox = this.dom.spawn(null, "INPUT", { type: "checkbox", value: mask })
            );
            if (param & mask) checkbox.checked = true;
            mask <<= 1;
          }
        } return true;
        
      case "merchant": { // Merchants are exactly the same as kitchens, but a different schedule of items, and will be sparse.
          if (words.length !== 4) return false;
          const param = +words[3];
          if (isNaN(param)) return false;
          this.dom.spawn(form, "DIV", `Merchant sprite at ${words[2]}`);
          const pantry = this.dom.spawn(form, "DIV", ["merchant"]);
          let mask = 1;
          for (const item of PoiModal.MERCHANT_ITEMS) {
            let checkbox;
            this.dom.spawn(pantry, "LABEL", item.name || ("0x" + mask.toString(16).padStart(8, '0')),
              checkbox = this.dom.spawn(null, "INPUT", { type: "checkbox", value: mask })
            );
            if (param & mask) checkbox.checked = true;
            mask <<= 1;
          }
        } return true;
        
      case "customer": {
          if (words.length !== 5) return false; // Expect 2 tokens for spriteParams: stringsid, index
          this.dom.spawn(form, "DIV", `Customer sprite at ${words[2]}`);
          this.dom.spawn(form, "TABLE", { name: "spriteParams:customer" },
            this.dom.spawn(null, "TR",
              this.dom.spawn(null, "TD", ["key"], "stringsid",
                this.dom.spawn(null, "INPUT", { name: "stringsid", value: words[3] })
              )
            ),
            this.dom.spawn(null, "TR",
              this.dom.spawn(null, "TD", ["key"], "index",
                this.dom.spawn(null, "INPUT", { name: "index", value: words[4] })
              )
            ),
          );
        } return true;
        
      case "foe": {
          if (words.length !== 5) return false; // Expect 2 tokens for spriteParams: flag, reserved
          this.dom.spawn(form, "DIV", `Foe sprite at ${words[2]}`);
          this.dom.spawn(form, "TABLE", { name: "spriteParams:foe" },
            this.dom.spawn(null, "TR",
              this.dom.spawn(null, "TD", ["key"], "flag",
                this.dom.spawn(null, "INPUT", { name: "flag", value: words[3] })
              )
            ),
            this.dom.spawn(null, "TR",
              this.dom.spawn(null, "TD", ["key"], "reserved, 3 bytes",
                this.dom.spawn(null, "INPUT", { name: "reserved", value: words[4] })
              )
            ),
          );
        } return true;
    }
    return false;
  }
  
  /* If (words) is well formed, add fields to (form) and return true.
   * Returns false if you should try other options.
   */
  buildFormMessage(form, words) {
    //TODO
    return false;
  }
}

/* Keyed by the text keyword. Opcodes listed as commentary only, editor doesn't use or know about them.
 * Values are an array of [len,name] for the expected arguments.
 * Names ending "id", the rest of the name should be a resource type.
 * Position and range arguments are condensed, as that's the convention in source files ("@X,Y").
 * Our specs list their components separately, since that's more convenient when decoding the compiled command.
 */
PoiModal.COMMANDS = {
  /*00*/EOF: [],
  /*20*/song: [[2, "songid"]],
  /*21*/image: [[2, "imageid"]],
  /*22*/hero: [[2, "pos"]],
  /*40*/battle: [[2, "battleid"], [2, "weight"]],
  /*41*/flagtile: [[2, "pos"], [1, "flagid"], [1, "reserved"]],
  /*60*/door: [[2, "pos"], [2, "mapid"], [2, "dstpos"], [2, "reserved"]],
  /*61*/sprite: [[2, "spriteid"], [2, "pos"], [4, "spriteParams"]],
  /*62*/message: [[2, "pos"], [2, "stringid"], [2, "index"], [1, "messageAction"], [1, "qualifier"]],
};

PoiModal.MESSAGE_ACTIONS = [ // Indexed by (action) value.
  "Nothing",
  "Restore HP",
  "Increment string index by 1 if flag (qualifier) set.",
];

PoiModal.KITCHEN_ITEMS = [
  { name:"TEA    ", strength: 3, price: 6 },
  { name:"NOG    ", strength: 4, price: 8 },
  { name:"EGG    ", strength: 5, price:10 },
  { name:"RYE    ", strength: 6, price:12 },
  { name:"FLAN   ", strength: 7, price:14 },
  { name:"HAM    ", strength: 8, price:16 },
  { name:"OX     ", strength: 9, price:18 },
  { name:"CAKE   ", strength:10, price:19 },
  { name:"ZA     ", strength:11, price:20 },
  { name:"MELON  ", strength:12, price:21 },
  { name:"ZITI   ", strength:13, price:22 },
  { name:"BACON  ", strength:14, price:23 },
  { name:"CURRY  ", strength:15, price:24 },
  { name:"SNAILS ", strength:16, price:25 },
  { name:"GRAVY  ", strength:17, price:26 },
  { name:"TOMATO ", strength:18, price:26 },
  { name:"TAHINI ", strength:19, price:27 },
  { name:"CREPES ", strength:20, price:27 },
  { name:"HAGGIS ", strength:21, price:28 },
  { name:"PEPPER ", strength:22, price:28 },
  { name:"OXTAIL ", strength:23, price:29 },
  { name:"COFFEE ", strength:24, price:29 },
  { name:"WAFFLE ", strength:25, price:30 },
  { name:"SQUIDS ", strength:26, price:30 },
  { name:"PIZZA  ", strength:30, price:35 },
  { name:"QUOKKA ", strength:33, price:36 },
  { name:"LASAGNA", strength:58, price:40 },
  { name:"TREACLE", strength:59, price:41 },
  { name:"GUMDROP", strength:63, price:45 },
  { name:"YOGHURT", strength:64, price:46 },
  { name:"PRETZEL", strength:68, price:48 },
  { name:"QUETZAL", strength:75, price:50 },
];

PoiModal.MERCHANT_ITEMS = [
  /* 0*/{ name:"NOOP" },
  /* 1*/{ name:"2XLETTER" },
  /* 2*/{ name:"3XLETTER" },
  /* 3*/{ name:"2XWORD" },
  /* 4*/{ name:"3XWORD" },
  /* 5*/{ name:"ERASER" },
  /* 6*/{},
  /* 7*/{},
  /* 8*/{},
  /* 9*/{},
  /*10*/{},
  /*11*/{},
  /*12*/{},
  /*13*/{},
  /*14*/{},
  /*15*/{},
  /*16*/{},
  /*17*/{},
  /*18*/{},
  /*19*/{},
  /*20*/{},
  /*21*/{},
  /*22*/{},
  /*23*/{},
  /*24*/{},
  /*25*/{},
  /*26*/{},
  /*27*/{},
  /*28*/{},
  /*29*/{},
  /*30*/{},
  /*31*/{},
];
