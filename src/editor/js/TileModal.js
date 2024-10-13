import { Dom } from "./Dom.js";
import { Data } from "./Data.js";
import { TILESIZE } from "./spellingbeeConstants.js";
import { MapPaint } from "./MapPaint.js";

export class TileModal {
  static getDependencies() {
    return [HTMLDialogElement, Dom, Window, Data];
  }
  constructor(element, dom, window, data) {
    this.element = element;
    this.dom = dom;
    this.window = window;
    this.data = data;
    
    this.res = null;
    this.tilesheet = null;
    this.srcbits = null;
    this.tileid = 0;
    this.mouseListener = null;
    this.neighborDragValue = false;
    
    this.buildUi();
  }
  
  onRemoveFromDom() {
    if (this.mouseListener) {
      this.window.removeEventListener("mouseup", this.mouseListener);
      this.window.removeEventListener("mousemove", this.mouseListener);
      this.mouseListener = null;
    }
  }
  
  setup(res, tilesheet, srcbits, tileid) {
    this.res = res;
    this.tilesheet = tilesheet;
    this.srcbits = srcbits;
    this.tileid = tileid;
    this.buildTablesForm();
    this.populateUi();
  }
  
  buildUi() {
    this.element.innerHTML = "";
    
    const topRow = this.dom.spawn(this.element, "DIV", ["topRow"]);
    const neighborGrid = this.dom.spawn(topRow, "TABLE", ["neighborGrid"], { "on-mousedown": e => this.onNeighborMouseDown(e) });
    this.buildNeighborGrid(neighborGrid);
    this.dom.spawn(topRow, "DIV", ["tileid"]);
    const navGrid = this.dom.spawn(topRow, "TABLE", ["navGrid"]);
    this.buildNavGrid(navGrid);
    
    const table = this.dom.spawn(this.element, "TABLE", ["tablesTable"], { "on-change": e => this.onTablesTableChanged(e) });
    
    const macros = this.dom.spawn(this.element, "DIV", ["macros"]);
    for (const macro of MapPaint.MACROS) {
      const button = this.dom.spawn(macros, "BUTTON", { "data-macro-name": macro.name, "on-click": () => this.applyMacro(macro) });
      this.dom.spawn(button, "CANVAS", { width: macro.w * TILESIZE, height: macro.h * TILESIZE });
      this.dom.spawn(button, "DIV", macro.name);
    }
  }
  
  buildNeighborGrid(neighborGrid) {
    let tr = this.dom.spawn(neighborGrid, "TR");
    this.dom.spawn(tr, "TD", ["neighbor"], { "data-mask": 0x80 });
    this.dom.spawn(tr, "TD", ["neighbor"], { "data-mask": 0x40 });
    this.dom.spawn(tr, "TD", ["neighbor"], { "data-mask": 0x20 });
    tr = this.dom.spawn(neighborGrid, "TR");
    this.dom.spawn(tr, "TD", ["neighbor"], { "data-mask": 0x10 });
    this.dom.spawn(tr, "TD", this.dom.spawn(null, "CANVAS", ["thumbnail"]));
    this.dom.spawn(tr, "TD", ["neighbor"], { "data-mask": 0x08 });
    tr = this.dom.spawn(neighborGrid, "TR");
    this.dom.spawn(tr, "TD", ["neighbor"], { "data-mask": 0x04 });
    this.dom.spawn(tr, "TD", ["neighbor"], { "data-mask": 0x02 });
    this.dom.spawn(tr, "TD", ["neighbor"], { "data-mask": 0x01 });
  }
  
  buildNavGrid(navGrid) {
    let tr = this.dom.spawn(navGrid, "TR");
    this.dom.spawn(tr, "TD");
    this.dom.spawn(tr, "TD", this.dom.spawn(null, "INPUT", { type: "button", value: "^", "on-click": () => this.navigate(0, -1) }));
    this.dom.spawn(tr, "TD");
    tr = this.dom.spawn(navGrid, "TR");
    this.dom.spawn(tr, "TD", this.dom.spawn(null, "INPUT", { type: "button", value: "<", "on-click": () => this.navigate(-1, 0) }));
    this.dom.spawn(tr, "TD");
    this.dom.spawn(tr, "TD", this.dom.spawn(null, "INPUT", { type: "button", value: ">", "on-click": () => this.navigate(1, 0) }));
    tr = this.dom.spawn(navGrid, "TR");
    this.dom.spawn(tr, "TD");
    this.dom.spawn(tr, "TD", this.dom.spawn(null, "INPUT", { type: "button", value: "v", "on-click": () => this.navigate(0, 1) }));
    this.dom.spawn(tr, "TD");
  }
  
  buildTablesForm() {
    const parent = this.element.querySelector(".tablesTable");
    parent.innerHTML = "";
    for (const k of Object.keys(this.tilesheet.tables)) {
      const tr = this.dom.spawn(parent, "TR", { "data-k": k });
      let td = this.dom.spawn(tr, "TD", ["key"], k);
      td = this.dom.spawn(tr, "TD", ["value"]);
      this.dom.spawn(td, "INPUT", { type: "number", min: 0, max: 255, name: k, value: this.tilesheet.tables[k][this.tileid] });
      if (k === "family") {
        this.dom.spawn(tr, "TD", this.dom.spawn(null, "INPUT", { type: "button", value: "New", "on-click": () => this.newFamily() }));
      } else {
        this.dom.spawn(tr, "TD", ["comment"], { "data-k": k }, this.commentForTable(k, this.tilesheet.tables[k][this.tileid]));
      }
    }
  }
  
  commentForTable(tableName, value) {
    switch (tableName) {
      case "physics": switch (value) {
          case 0: return "vacant";
          case 1: return "solid";
          case 2: return "water";
          case 3: return "hole";
          case 4: return "safe";
          default: return "?";
        } break;
      // "neighbors" could sensibly represent, but we've got a better view by the thumbnail.
      // And we won't get asked for "family".
      case "weight": switch (value) {
          case 0: return "likeliest";
          case 255: return "appt only";
          default: return (value < 128) ? "likely" : "unlikely";
        } break;
    }
    return "";
  }
  
  populateUi() {
    this.drawThumbnail();
    for (const element of this.element.querySelectorAll(".neighborGrid .neighbor")) {
      const mask = +element.getAttribute("data-mask");
      if (mask & this.tilesheet.tables.neighbors[this.tileid]) element.classList.add("present");
      else element.classList.remove("present");
    }
    this.element.querySelector(".tileid").innerText = "0x" + this.tileid.toString(16).padStart(2, '0');
    this.populateTablesForm();
    for (const button of this.element.querySelectorAll(".macros > button")) {
      const name = button.getAttribute("data-macro-name");
      const macro = MapPaint.MACROS.find(m => m.name === name);
      this.drawMacroThumbnail(button.querySelector("canvas"), macro);
    }
  }
  
  drawMacroThumbnail(canvas, macro) {
    if (!canvas) return;
    const ctx = canvas.getContext("2d");
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    if (!macro) return;
    if (!this.srcbits) return;
    const tx = this.tileid & 0x0f;
    const ty = this.tileid >> 4;
    if ((tx + macro.w > 16) || (ty + macro.h > 16)) return;
    ctx.drawImage(this.srcbits, tx * TILESIZE, ty * TILESIZE, TILESIZE * macro.w, TILESIZE * macro.h, 0, 0, TILESIZE * macro.w, TILESIZE * macro.h);
  }
  
  drawThumbnail() {
    if (!this.srcbits) return;
    const canvas = this.element.querySelector(".thumbnail");
    canvas.width = TILESIZE;
    canvas.height = TILESIZE;
    const ctx = canvas.getContext("2d");
    const srcx = (this.tileid & 0x0f) * TILESIZE;
    const srcy = (this.tileid >> 4) * TILESIZE;
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    ctx.drawImage(this.srcbits, srcx, srcy, TILESIZE, TILESIZE, 0, 0, TILESIZE, TILESIZE);
  }
  
  populateTablesForm() {
    for (const tr of this.element.querySelectorAll(".tablesTable tr")) {
      const k = tr.getAttribute("data-k");
      const v = this.tilesheet.tables[k]?.[this.tileid] || 0;
      tr.querySelector("input").value = v;
      const comment = tr.querySelector(".comment"); // optional. "family" doesn't have one.
      if (comment) comment.innerText = this.commentForTable(k, v);
    }
  }
  
  dirty() {
    this.data.dirty(this.res.path, () => this.tilesheet.encode());
  }
  
  navigate(dx, dy) {
    let x = this.tileid & 0x0f;
    let y = this.tileid >> 4;
    x += dx;
    y += dy;
    if ((x < 0) || (y < 0) || (x > 15) || (y > 15)) return;
    this.tileid = (y << 4) | x;
    this.populateUi();
  }
  
  onTablesTableChanged(event) {
    let dirty = false;
    for (const input of this.element.querySelectorAll(".tablesTable input")) {
      const name = input.name;
      if (!this.tilesheet.tables[name]) continue;
      const v = +input.value;
      if (isNaN(v) || (v < 0) || (v > 0xff)) continue;
      if (this.tilesheet.tables[name][this.tileid] === v) continue;
      this.tilesheet.tables[name][this.tileid] = v;
      const comment = this.element.querySelector(`.tablesTable .comment[data-k='${name}']`);
      if (comment) comment.innerText = this.commentForTable(name, v);
      dirty = true;
    }
    if (dirty) this.dirty();
  }
  
  onNeighborMouseDown(event) {
    if (!this.tilesheet) return;
    const target = event.target;
    if (!target) return;
    if (!target.classList.contains("neighbor")) return;
    const mask = +target.getAttribute("data-mask");
    this.neighborDragValue = !(this.tilesheet.tables.neighbors[this.tileid] & mask);
    this.tilesheet.tables.neighbors[this.tileid] ^= mask;
    if (this.neighborDragValue) target.classList.add("present");
    else target.classList.remove("present");
    this.populateTablesForm();
    this.dirty();
    
    this.mouseListener = e => this.onNeighborMouseUpOrMove(e);
    this.window.addEventListener("mouseup", this.mouseListener);
    this.window.addEventListener("mousemove", this.mouseListener);
  }
  
  onNeighborMouseUpOrMove(event) {
    if (event.type === "mouseup") {
      this.window.removeEventListener("mouseup", this.mouseListener);
      this.window.removeEventListener("mousemove", this.mouseListener);
      this.mouseListener = null;
      return;
    }
    const mask = +event.target?.getAttribute?.("data-mask");
    if (!mask) return;
    const table = this.tilesheet.tables.neighbors;
    if (this.neighborDragValue) {
      if (table[this.tileid] & mask) return;
      table[this.tileid] |= mask;
      event.target.classList.add("present");
    } else {
      if (!(table[this.tileid] & mask)) return;
      table[this.tileid] &= ~mask;
      event.target.classList.remove("present");
    }
    this.populateTablesForm();
    this.dirty();
  }
  
  applyMacro(macro) {
    if (!macro) return;
    const tx = this.tileid & 0x0f;
    const ty = this.tileid >> 4;
    if (tx + macro.w > 16) return;
    if (ty + macro.h > 16) return;
    /* (physics,family) copy from the focussed tile.
     * (neighbors) overwrites all from the macro.
     * Other tables are not touched.
     */
    for (let dy=0, np=0; dy<macro.h; dy++) {
      for (let dx=0, dstp=this.tileid+dy*16; dx<macro.w; dx++, np++, dstp++) {
        this.tilesheet.tables.physics[dstp] = this.tilesheet.tables.physics[this.tileid];
        this.tilesheet.tables.family[dstp] = this.tilesheet.tables.family[this.tileid];
        this.tilesheet.tables.neighbors[dstp] = macro.neighbors[np];
      }
    }
    this.populateUi();
    this.dirty();
  }
  
  newFamily() {
    if (!this.tilesheet) return;
    const inuse = new Set();
    const src = this.tilesheet.tables.family;
    for (let i=0; i<256; i++) {
      if (!src[i]) continue; // Zero doesn't count.
      inuse.add(src[i]);
    }
    for (let i=1; i<256; i++) {
      if (!inuse.has(i)) {
        this.tilesheet.tables.family[this.tileid] = i;
        this.populateUi();
        this.dirty();
        return;
      }
    }
  }
}
