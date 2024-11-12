import { Dom } from "./Dom.js";
import { TILESIZE } from "./spellingbeeConstants.js";
import { Data } from "./Data.js";
import { PaletteModal } from "./PaletteModal.js";
import { PickImageModal } from "./PickImageModal.js";

export class SpriteEditor {
  static checkResource(res) {
    if (res.type === "sprite") return 2;
    return 0;
  }
  static getDependencies() {
    return [HTMLElement, Dom, Window, Data];
  }
  constructor(element, dom, window, data) {
    this.element = element;
    this.dom = dom;
    this.window = window;
    this.data = data;
    
    this.res = null;
    this.image = null; // null to reload at render, or Image, or Promise<Image> if loading, or "error" if load failed.
    
    this.buildUi();
    this.element.addEventListener("input", () => this.dirty());
    
    /* First time we get instantiated, load the type names.
     * All nonstatic methods may assume that it's present.
     */
    if (!SpriteEditor.SPRITE_TYPE_NAMES) {
      SpriteEditor.SPRITE_TYPE_NAMES = [];
      this.window.fetch("/game/sprite/sprite.h").then(rsp => {
        if (!rsp.ok) throw rsp;
        return rsp.text();
      }).then(src => {
        SpriteEditor.SPRITE_TYPE_NAMES = this.parseSpriteCHeader(src);
        this.populateSpriteTypeNames();
      }).catch(e => {
        this.window.console.error("Failed to load sprite type names.", e);
      });
    }
  }
  
  setup(res) {
    this.res = res;
    this.populateUi();
  }
  
  buildUi() {
    this.element.innerHTML = "";
    
    /* Starts with a table of known fields.
     */
    const table = this.dom.spawn(this.element, "TABLE", ["known"]);
    
    this.dom.spawn(table, "TR",
      this.dom.spawn(null, "TD", ["key"], "type"),
      this.dom.spawn(null, "TD", ["value"],
        this.dom.spawn(null, "INPUT", { type: "text", name: "type", data: "spriteTypes" }),
        this.dom.spawn(null, "DATALIST", { id: "spriteTypes" })
      )
    );
    
    this.dom.spawn(table, "TR", ["image"],
      this.dom.spawn(null, "TD", ["key"], "image"),
      this.dom.spawn(null, "TD", ["value"],
        this.dom.spawn(null, "INPUT", { type: "button", value: "...", "on-click": () => this.onSelectImage() }),
        this.dom.spawn(null, "INPUT", { type: "text", name: "image", "on-input": () => { this.image = null; this.drawTilePreview(); } }),
      )
    );
    
    this.dom.spawn(table, "TR", ["tile"],
      this.dom.spawn(null, "TD", ["key"], "tile"),
      this.dom.spawn(null, "TD", ["value"],
        this.dom.spawn(null, "CANVAS", ["tilePreview"], { width: TILESIZE, height: TILESIZE, "on-click": () => this.onSelectTile() }),
        this.dom.spawn(null, "INPUT", { type: "number", min: 0, max: 255, name: "tileid", "on-input": () => this.drawTilePreview() }),
        this.dom.spawn(null, "SELECT", { name: "xform", "on-input": () => this.drawTilePreview() },
          this.dom.spawn(null, "OPTION", { value: 0 }, "Natural"),
          this.dom.spawn(null, "OPTION", { value: 1 }, "XREV (flop)"),
          this.dom.spawn(null, "OPTION", { value: 2 }, "YREV (flip)"),
          this.dom.spawn(null, "OPTION", { value: 3 }, "XREV|YREV (180)"),
          this.dom.spawn(null, "OPTION", { value: 4 }, "SWAP (unusual)"),
          this.dom.spawn(null, "OPTION", { value: 5 }, "SWAP|XREV (counterclockwise)"),
          this.dom.spawn(null, "OPTION", { value: 6 }, "SWAP|YREV (clockwise)"),
          this.dom.spawn(null, "OPTION", { value: 7 }, "SWAP|XREV|YREF (unusual)"),
        ),
      )
    );
    
    this.dom.spawn(table, "TR",
      this.dom.spawn(null, "TD", ["key"], "groups"),
      this.dom.spawn(null, "TD", ["value"],
        this.dom.spawn(null, "INPUT", { type: "text", name: "groups" })//TODO Implement as 32 checkboxes instead?
      )
    );
    
    this.dom.spawn(table, "TR",
      this.dom.spawn(null, "TD", ["key"], "0x2f"),
      this.dom.spawn(null, "TD", ["value"],
        this.dom.spawn(null, "INPUT", { type: "text", name: "0x2f" })
      )
    );
    
    /* Unknown fields get collected into a text dump below the table.
     */
    this.dom.spawn(this.element, "TEXTAREA", { name: "other" });
  }
  
  populateUi() {
  
    // Clear everything first.
    const type = this.element.querySelector("input[name='type']");
    const image = this.element.querySelector("input[name='image']");
    const tileid = this.element.querySelector("input[name='tileid']");
    const xform = this.element.querySelector("select[name='xform']");
    const groups = this.element.querySelector("input[name='groups']");
    const reserved = this.element.querySelector("input[name='0x2f']");
    const other = this.element.querySelector("textarea[name='other']");
    type.value = "";
    image.value = "";
    tileid.value = "0";
    xform.value = "0";
    groups.value = "";
    reserved.value = "";
    other.value = "";
    
    // Read as text.
    const src = new TextDecoder("utf8").decode(this.res?.serial || new Uint8Array(0));
    for (let srcp=0, lineno=0; srcp<src.length; lineno++) {
      let nlp = src.indexOf("\n", srcp);
      if (nlp < 0) nlp = src.length;
      const line = src.substring(srcp, nlp);
      srcp = nlp + 1;
      const words = line.split(/\s+/).filter(v => v);
      if (words.length < 1) continue;
      const keyword = words[0];
      const args = words.slice(1);
      const argstr = args.join(" ");
      
      // Generally for known fields there's `input[name='KEYWORD']` but I think it's not worth generalizing.
      switch (keyword) {
        case "type": type.value = argstr; break;
        case "image": image.value = argstr; break;
        case "tile": tileid.value = +args[0]; xform.value = +args[1]; break;
        case "0x2f": reserved.value = argstr; break;
        case "groups": groups.value = argstr; break;
        default: other.value += line + "\n";
      }
    }
    
    this.drawTilePreview();
  }
  
  drawTilePreview() {
  
    const canvas = this.element.querySelector(".tilePreview");
    if (!canvas) return;
    const ctx = canvas.getContext("2d");
    ctx.resetTransform();
    
    /* If we don't have an image, start loading it.
     * Or if load in progress or failed, abort.
     * Either of those cases, also clear the preview.
     */
    if (!this.image) {
      const imageName = this.element.querySelector("input[name='image']").value;
      this.image = this.data.getImageAsync(imageName);
      this.image.catch(e => "error").then(image => {
        this.image = image;
        this.drawTilePreview();
      });
      ctx.fillStyle = "#888";
      ctx.fillRect(0, 0, TILESIZE, TILESIZE);
      return;
    } else if (this.image instanceof Promise) {
      ctx.fillStyle = "#888";
      ctx.fillRect(0, 0, TILESIZE, TILESIZE);
      return;
    } else if (this.image === "error") {
      ctx.fillStyle = "#f00";
      ctx.fillRect(0, 0, TILESIZE, TILESIZE);
      return;
    }
  
    const tileid = +this.element.querySelector("input[name='tileid']").value;
    const xform = +this.element.querySelector("select[name='xform']").value;
    
    ctx.clearRect(0, 0, TILESIZE, TILESIZE);
    const srcx = (tileid & 15) * TILESIZE;
    const srcy = (tileid >> 4) * TILESIZE;
    switch (xform) {
      case 1: ctx.scale(-1, 1); ctx.translate(-TILESIZE, 0); break;
      case 2: ctx.scale(1, -1); ctx.translate(0, -TILESIZE); break;
      case 3: ctx.rotate(Math.PI); ctx.translate(-TILESIZE, -TILESIZE); break;
      case 4: ctx.rotate(Math.PI/-2); ctx.scale(-1, 1); break;
      case 5: ctx.rotate(Math.PI/-2); ctx.translate(-TILESIZE, 0); break;
      case 6: ctx.rotate(Math.PI/2); ctx.translate(0, -TILESIZE); break;
      case 7: ctx.rotate(Math.PI/2); ctx.scale(-1, 1); ctx.translate(-TILESIZE, -TILESIZE); break;
    }
    ctx.drawImage(this.image, srcx, srcy, TILESIZE, TILESIZE, 0, 0, TILESIZE, TILESIZE);
  }
  
  onSelectImage() {
    const modal = this.dom.spawnModal(PickImageModal);
    modal.result.then(name => {
      if (name === null) return;
      this.element.querySelector("input[name='image']").value = name;
      this.image = null;
      this.drawTilePreview();
      this.dirty();
    }).catch(e => this.dom.modalError(e));
  }
  
  onSelectTile() {
    // Borrow the map editor's PaletteModal, setting a sprite tile is exactly the same thing.
    if (!(this.image instanceof Image)) return;
    const input = this.element.querySelector("input[name='tileid']");
    const modal = this.dom.spawnModal(PaletteModal);
    modal.setImage(this.image);
    modal.result.then(tileid => {
      if (tileid === null) return;
      input.value = tileid;
      this.drawTilePreview();
      this.dirty();
    }).catch(e => this.dom.modalError(e));
  }
  
  dirty() {
    if (!this.res) return;
    this.data.dirty(this.res.path, () => this.encode());
  }
  
  encode() {
  
    /* Gather all the inputs.
     */
    const type = this.element.querySelector("input[name='type']").value.trim();
    const image = this.element.querySelector("input[name='image']").value.trim();
    const tileid = +this.element.querySelector("input[name='tileid']").value || 0;
    const xform = +this.element.querySelector("select[name='xform']").value || 0;
    const groups = this.element.querySelector("input[name='groups']").value.trim();
    const reserved = this.element.querySelector("input[name='0x2f']").value.trim();
    const other = this.element.querySelector("textarea[name='other']").value.trim();
    
    /* Compose output as a string.
     * Start with the known fields; they're pretty straightforward.
     */
    let dst = "";
    if (type) dst += `type ${type}\n`;
    if (image) dst += `image ${image}\n`;
    if (tileid || xform) dst += `tile 0x${tileid.toString(16).padStart(2, '0')} ${xform}\n`;
    if (groups) dst += `groups ${groups}\n`;
    if (reserved) dst += `0x2f ${reserved}\n`;
    
    /* Hm, and actually "other" is even straightforwarder.
     * We don't need to split lines and review each, nothing like that.
     */
    if (other) dst += other + "\n";
    
    return new TextEncoder("utf8").encode(dst);
  }
  
  populateSpriteTypeNames() {
    const list = this.element.querySelector("#spriteTypes");
    if (!list) return;
    list.innerHTML = "";
    for (const name of SpriteEditor.SPRITE_TYPE_NAMES) {
      this.dom.spawn(list, "OPTION", { value: name }, name);
    }
  }
  
  parseSpriteCHeader(src) {
    const typeNameById = [];
    for (let srcp=0, lineno=1, ready=false; srcp<src.length; lineno++) {
      let nlp = src.indexOf("\n", srcp);
      if (nlp < 0) nlp = src.length;
      const line = src.substring(srcp, nlp).trim();
      srcp = nlp + 1;
      if (!line) {
        if (ready) break; // First blank line after the list, we're done.
        continue;
      }
      if (ready) {
        const name = line.match(/_\(([[0-9a-zA-Z_]+)\)/)?.[1];
        if (!name) {
          this.window.console.error(`SpriteEditor.parseSpriteCHeader:${lineno}: Expected '_(NAME) \\', found ${JSON.stringify(line)}`);
          break;
        }
        typeNameById.push(name);
      } else {
        if (line.match(/#define\s+SPRITE_TYPE_FOR_EACH/)) {
          ready = true;
        }
      }
    }
    return typeNameById;
  }
}

/* null as a signal to load on the first construction.
 * After that, it's an array of string indexed contiguously by type id (that id is not relevant for us).
 */
SpriteEditor.SPRITE_TYPE_NAMES = null;
