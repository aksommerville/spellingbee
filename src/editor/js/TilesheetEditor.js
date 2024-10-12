import { Dom } from "./Dom.js";
import { Tilesheet } from "./Tilesheet.js";
import { Data } from "./Data.js";
import { TILESIZE } from "./spellingbeeConstants.js";
import { TileModal } from "./TileModal.js";

export class TilesheetEditor {
  static checkResource(res) {
    if (res.type === "tilesheet") return 2;
    return 0;
  }
  static getDependencies() {
    return [HTMLElement, Dom, "nonce", Window, Data];
  }
  constructor(element, dom, nonce, window, data) {
    this.element = element;
    this.dom = dom;
    this.nonce = nonce;
    this.window = window;
    this.data = data;
    
    this.res = null;
    this.tilesheet = null;
    this.tableName = "physics";
    this.toggles = {
      all: false,
      image: true,
      grid: true,
      numeric: false,
    };
    this.renderTimeout = null;
    this.srcbits = null;
    this.view = { // Updates at render.
      tilesize: 16,
      dstx: 0,
      dsty: 0,
    };
    this.uibits = null;
    this.selectp = -1; // 0..255 or OOB for none
    this.mouseListener = null;
    this.dragp = -1;
    
    const image = new Image();
    image.addEventListener("load", () => { this.uibits = image; }, { once: true });
    image.src = "../uibits.png";
    
    this.buildUi();
  }
  
  onRemoveFromDom() {
    if (this.renderTimeout) this.window.clearTimeout(this.renderTimeout);
    if (this.mouseListener) {
      this.window.removeEventListener("mousemove", this.mouseListener);
      this.window.removeEventListener("mouseup", this.mouseListener);
      this.mouseListener = null;
    }
  }
  
  setup(res) {
    this.res = res;
    this.tilesheet = new Tilesheet(this.res.serial);
    this.populateUi();
    this.data.getImageAsync(this.res.rid).then(image => {
      this.srcbits = image;
      this.renderSoon();
    }).catch(e => console.error(e));
  }
  
  buildUi() {
    this.innerHTML = "";
    const toolbar = this.dom.spawn(this.element, "DIV", ["toolbar"], { "on-change": () => this.onToolbarChange() });
    this.dom.spawn(toolbar, "DIV", ["tables"]);
    this.dom.spawn(toolbar, "DIV", ["spacer"]);
    const toggles = this.dom.spawn(toolbar, "DIV", ["toggles"]);
    for (const k of Object.keys(this.toggles)) {
      this.spawnToggle(toggles, k, this.toggles[k]);
    }
    this.dom.spawn(toolbar, "DIV", ["spacer"]);
    this.dom.spawn(toolbar, "DIV", ["tip"], "ctl-click to select, shift-click to copy, visible table only");
    const canvas = this.dom.spawn(this.element, "CANVAS", { "on-mousedown": e => this.onMouseDown(e) });
  }
  
  spawnToggle(parent, name, enable) {
    const id = `TilesheetEditor-${this.nonce}-toggle-${name}`;
    const input = this.dom.spawn(parent, "INPUT", { type: "checkbox", id, name });
    const label = this.dom.spawn(parent, "LABEL", ["toggle"], name, { for: id });
    if (enable) input.checked = true;
  }
  
  populateUi() {
    const tables = this.element.querySelector(".tables");
    tables.innerHTML = "";
    if (this.tilesheet) {
      for (const k of Object.keys(this.tilesheet.tables)) {
        this.spawnTable(tables, k);
      }
    }
    this.renderSoon();
  }
  
  spawnTable(parent, k) {
    const id = `TilesheetEditor-${this.nonce}-table-${k}`;
    const input = this.dom.spawn(parent, "INPUT", { type: "radio", name: "table", id, value: k });
    const label = this.dom.spawn(parent, "LABEL", ["table"], k, { for: id });
    if (k === this.tableName) input.checked = true;
  }
  
  onToolbarChange() {
    console.log(`TilesheetEditor.onToolbarChange`);
    const tableElement = this.element.querySelector(".tables input:checked");
    if (tableElement) this.tableName = tableElement.value || "";
    else this.tableName = "";
    for (const element of this.element.querySelectorAll(".toggles input")) {
      this.toggles[element.name] = element.checked;
    }
    this.renderSoon();
  }
  
  renderSoon() {
    if (this.renderTimeout) return;
    this.renderTimeout = this.window.setTimeout(() => {
      this.renderTimeout = null;
      this.renderNow();
    }, 20);
  }
  
  renderNow() {
    const canvas = this.element.querySelector("canvas");
    const bounds = canvas.getBoundingClientRect();
    canvas.width = bounds.width;
    canvas.height = bounds.height;
    const ctx = canvas.getContext("2d");
    ctx.imageSmoothingEnabled = false;
    const tilesize = Math.max(4, Math.floor(Math.min(canvas.width, canvas.height) / 16));
    const dstw = tilesize << 4;
    const dstx = (canvas.width >> 1) - (dstw >> 1);
    const dsty = (canvas.height >> 1) - (dstw >> 1);
    this.view.dstx = dstx;
    this.view.dsty = dsty;
    this.view.tilesize = tilesize;
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    ctx.fillStyle = "#666";
    ctx.fillRect(dstx, dsty, dstw, dstw);
    if (!this.tilesheet) return;
    if (this.srcbits && this.toggles.image) {
      ctx.drawImage(this.srcbits, 0, 0, TILESIZE * 16, TILESIZE * 16, dstx, dsty, tilesize * 16, tilesize * 16);
    }
    if (this.toggles.grid) {
      ctx.beginPath();
      for (let i=1; i<16; i++) {
        ctx.moveTo(dstx + i * tilesize, dsty);
        ctx.lineTo(dstx + i * tilesize, dsty + dstw);
        ctx.moveTo(dstx, dsty + i * tilesize);
        ctx.lineTo(dstx + dstw, dsty + i * tilesize);
      }
      ctx.strokeStyle = "#0f0";
      ctx.stroke();
    }
    if (this.toggles.all) {
      let position = 0;
      for (const k of Object.keys(this.tilesheet.tables)) {
        if (this.toggles.numeric) this.drawTableNumeric(ctx, k, this.tilesheet.tables[k], position);
        else this.drawTableMnemonic(ctx, k, this.tilesheet.tables[k], position);
        position++;
      }
    } else {
      const table = this.tilesheet.tables[this.tableName];
      if (table) {
        if (this.toggles.numeric) this.drawTableNumeric(ctx, this.tableName, table, 0);
        else this.drawTableMnemonic(ctx, this.tableName, table, 0);
      }
    }
    if ((this.selectp >= 0) && (this.selectp <= 0xff)) {
      const col = this.selectp & 15;
      const row = this.selectp >> 4;
      ctx.fillStyle = "#0ff";
      ctx.globalAlpha = 0.75;
      ctx.fillRect(dstx + col * tilesize, dsty + row * tilesize, tilesize, tilesize);
      ctx.globalAlpha = 0;
    }
  }
  
  drawTableNumeric(ctx, name, table, position) {
    const addy = (position % 4) * (this.view.tilesize >> 2);
    const addx = (position & 4) ? (this.view.tilesize >> 1) : 0;
    ctx.fillStyle = "#fff";
    for (let row=0, dsty=this.view.dsty+addy, tp=0; row<16; row++, dsty+=this.view.tilesize) {
      for (let col=0, dstx=this.view.dstx+addx; col<16; col++, dstx+=this.view.tilesize, tp++) {
        if (!table[tp]) continue;
        ctx.fillText(table[tp], dstx+2, dsty+9);
      }
    }
  }
  
  drawTableMnemonic(ctx, name, table, position) {
    
    // Neighbors are icons in the lower-right.
    if ((name === "neighbors") && this.uibits) {
      for (let row=0, dsty=this.view.dsty+this.view.tilesize-16, tp=0; row<16; row++, dsty+=this.view.tilesize) {
        for (let col=0, dstx=this.view.dstx+this.view.tilesize-16; col<16; col++, dstx+=this.view.tilesize, tp++) {
          if (!table[tp]) continue;
          for (let mask=0x80, srcx=0; mask; mask>>=1, srcx+=16) {
            if (!(table[tp] & mask)) continue;
            ctx.drawImage(this.uibits, srcx, 32, 16, 16, dstx, dsty, 16, 16);
          }
        }
      }
      return;
    }
    
    // All the others use colored enum boxes.
    const addy = (position % 4) * (this.view.tilesize >> 2);
    const addx = (position & 4) ? (this.view.tilesize >> 1) : 0;
    for (let row=0, dsty=this.view.dsty+addy, tp=0; row<16; row++, dsty+=this.view.tilesize) {
      for (let col=0, dstx=this.view.dstx+addx; col<16; col++, dstx+=this.view.tilesize, tp++) {
        if (!table[tp]) continue;
        ctx.fillStyle = this.styleForMnemonic(name, table[tp]);
        ctx.fillRect(dstx, dsty, 12, 6);
      }
    }
  }
  
  styleForMnemonic(name, value) {
    switch (value) {
      case 1: return "#f00";
      case 2: return "#f80";
      case 3: return "#ff0";
      case 4: return "#0f0";
      case 5: return "#0ff";
      case 6: return "#00f";
      case 7: return "#f0f";
    }
    const r = value | 0x1f;
    const g = ((value << 3) | 0x1f) & 0xff;
    const b = ((value << 6) | 0x3f) & 0xff;
    return `rgb(${r},${g},${b})`;
  }
  
  onMouseDown(event) {
    if (!this.tilesheet) return;
    if (this.mouseListener) return;
    const canvas = this.element.querySelector("canvas");
    const bounds = canvas.getBoundingClientRect();
    const col = Math.floor((event.x - bounds.x - this.view.dstx) / this.view.tilesize);
    const row = Math.floor((event.y - bounds.y - this.view.dsty) / this.view.tilesize);
    if ((col < 0) || (row < 0) || (col > 15) || (row > 15)) return;
    const tileid = (row << 4) | col;
    
    if (event.ctrlKey) {
      if (this.selectp === tileid) this.selectp = -1;
      else this.selectp = tileid;
      this.renderSoon();
      
    } else if (event.shiftKey) {
      this.dragp = -1;
      if ((this.selectp < 0) || (this.selectp > 0xff)) return;
      this.mouseListener = e => this.onMouseUpOrMove(e);
      this.window.addEventListener("mousemove", this.mouseListener);
      this.window.addEventListener("mouseup", this.mouseListener);
      this.onMouseUpOrMove(event);
      
    } else {
      const modal = this.dom.spawnModal(TileModal);
      modal.setup(this.res, this.tilesheet, this.srcbits, tileid);
      modal.result.then(result => {
        // TileModal talks to Data on its own. All we do is assume dirty and rerender when it closes.
        this.renderSoon();
      }).catch(e => this.dom.modalError(e));
    }
  }
  
  onMouseUpOrMove(event) {
    if (event.type === "mouseup") {
      this.window.removeEventListener("mouseup", this.mouseListener);
      this.window.removeEventListener("mousemove", this.mouseListener);
      this.mouseListener = null;
      return;
    }
    const canvas = this.element.querySelector("canvas");
    const bounds = canvas.getBoundingClientRect();
    const col = Math.floor((event.x - bounds.x - this.view.dstx) / this.view.tilesize);
    const row = Math.floor((event.y - bounds.y - this.view.dsty) / this.view.tilesize);
    if ((col < 0) || (row < 0) || (col > 15) || (row > 15)) return;
    const tileid = (row << 4) | col;
    if (tileid === this.dragp) return;
    this.dragp = tileid;
    if (this.toggles.all) {
      for (const k of Object.keys(this.tilesheet.tables)) {
        const table = this.tilesheet.tables[k];
        table[this.dragp] = table[this.selectp];
      }
    } else {
      const table = this.tilesheet.tables[this.tableName];
      if (!table) return;
      table[this.dragp] = table[this.selectp];
    }
    this.renderSoon();
    this.data.dirty(this.res.path, () => this.tilesheet.encode());
  }
}
