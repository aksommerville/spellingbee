/* MapCanvas.js
 * Main section of MapEditor, everything below the toolbar.
 */

import { Dom } from "./Dom.js";
import { TILESIZE } from "./spellingbeeConstants.js";
import { Data } from "./Data.js";
import { MapPaint } from "./MapPaint.js";

export class MapCanvas {
  static getDependencies() {
    return [HTMLElement, Dom, Window, Data, MapPaint];
  }
  constructor(element, dom, window, data, mapPaint) {
    this.element = element;
    this.dom = dom;
    this.window = window;
    this.data = data;
    this.mapPaint = mapPaint;
    
    // Caller may replace:
    this.tattle = (x, y) => {};
    
    this.map = null;
    this.res = null;
    this.scroller = null;
    this.canvas = null;
    this.ctx = null;
    this.renderTimeout = null;
    this.srcbits = mapPaint.image; // Image|null
    this.view = { ts:TILESIZE, x:0, y:0, w:0, h:0, cola:0, rowa:0, colz:0, rowz: 0 }; // Updates at render.
    this.mouseListener = null;
    this.mouseCol = 0;
    this.mouseRow = 0;
    this.mouseX = 0; // Straight off events
    this.mouseY = 0;
    
    this.uibits = new Image();
    this.uibits.src = "../uibits.png";
    
    this.buildUi();
    
    this.resizeObserver = new this.window.ResizeObserver(() => this.onResize());
    this.resizeObserver.observe(this.element);
    
    this.element.addEventListener("mousedown", e => this.onMouseDown(e));
    this.element.addEventListener("mousemove", e => this.onMouseMove(e));
    this.element.addEventListener("mouseleave", e => this.onMouseLeave(e));
    
    this.paintListener = this.mapPaint.listen(e => this.onPaintEvent(e));
  }
  
  onRemoveFromDom() {
    this.unloaded = true;
    if (this.renderTimeout) this.window.clearTimeout(this.renderTimeout);
    this.resizeObserver.disconnect();
    if (this.mouseListener) {
      this.window.removeEventListener("mousemove", this.mouseListener);
      this.window.removeEventListener("mouseup", this.mouseListener);
      this.mouseListener = null;
    }
    this.mapPaint.unlisten(this.paintListener);
  }
  
  setup(map, res) {
    this.map = map;
    this.res = res;
    this.updateSizer();
  }
  
  buildUi() {
    this.element.innerHTML = "";
    this.scroller = this.dom.spawn(this.element, "DIV", ["scroller"], { "on-scroll": () => { this.retattleAfterRender = true; this.renderSoon(); }});
    const sizer = this.dom.spawn(this.scroller, "DIV", ["sizer"]);
    this.canvas = this.dom.spawn(this.element, "CANVAS");
    this.ctx = this.canvas.getContext("2d");
  }
  
  /* Sets dimensions of the fake "sizer" element according to map size and zoom level.
   */
  updateSizer() {
    const sizer = this.element.querySelector(".sizer");
    if (!this.map || (this.map.w < 1) || (this.map.h < 1)) {
      sizer.style.width = "0";
      sizer.style.height = "0";
      return;
    }
    const w = this.map.w * TILESIZE;
    const h = this.map.h * TILESIZE;
    sizer.style.width = `${w * this.mapPaint.zoom}px`;
    sizer.style.height = `${h * this.mapPaint.zoom}px`;
    this.renderSoon();
  }
  
  onResize() {
    const bounds = this.canvas.getBoundingClientRect();
    this.canvas.width = bounds.width;
    this.canvas.height = bounds.height;
    this.renderSoon();
  }
  
  renderSoon() {
    if (this.renderTimeout) return;
    if (this.unloaded) return;
    this.renderTimeout = this.window.setTimeout(() => {
      this.renderTimeout = null;
      this.renderNow();
    }, 10);
  }
  
  renderNow() {
    this.ctx.fillStyle = "#000";
    this.ctx.fillRect(0, 0, this.canvas.width, this.canvas.height);
    if (!this.map) return;
    this.ctx.imageSmoothingEnabled = false;
    
    /* Determine some geometry.
     */
    const ts = TILESIZE * this.mapPaint.zoom;
    const cola = Math.max(0, Math.floor(this.scroller.scrollLeft / ts));
    const rowa = Math.max(0, Math.floor(this.scroller.scrollTop / ts));
    const colz = Math.min(this.map.w - 1, Math.floor((this.scroller.scrollLeft + this.canvas.width) / ts));
    const rowz = Math.min(this.map.h - 1, Math.floor((this.scroller.scrollTop + this.canvas.height) / ts));
    let dstx0, dsty0;
    if (this.scroller.scrollWidth <= this.canvas.width) {
      dstx0 = (this.canvas.width >> 1) - ((this.map.w * ts) >> 1);
    } else {
      dstx0 = cola*ts-this.scroller.scrollLeft;
    }
    if (this.scroller.scrollHeight <= this.canvas.height) {
      dsty0 = (this.canvas.height >> 1) - ((this.map.h * ts) >> 1);
    } else {
      dsty0 = rowa*ts-this.scroller.scrollTop;
    }
    this.view.ts = ts;
    this.view.cola = cola;
    this.view.colz = colz;
    this.view.rowa = rowa;
    this.view.rowz = rowz;
    this.view.x = dstx0;
    this.view.y = dsty0;
    this.view.w = Math.min(this.canvas.width, (colz - cola + 1) * ts);
    this.view.h = Math.min(this.canvas.height, (rowz - rowa + 1) * ts);
    
    /* Cell images, or flat randomish colors if image not loaded.
     */
    for (let row=rowa, y=dsty0; row<=rowz; row++, y+=ts) {
      for (let col=cola, x=dstx0, cellp=row*this.map.w+col; col<=colz; col++, cellp++, x+=ts) {
        if (this.srcbits) {
          const srcx = (this.map.v[cellp] & 0x0f) * TILESIZE;
          const srcy = (this.map.v[cellp] >> 4) * TILESIZE;
          this.ctx.drawImage(this.srcbits, srcx, srcy, TILESIZE, TILESIZE, x, y, ts, ts);
        } else {
          this.ctx.fillStyle = this.fallbackColorForCell(this.map.v[cellp]);
          this.ctx.fillRect(x, y, ts, ts);
        }
      }
    }
    
    /* Optional grid lines.
     */
    if (this.mapPaint.showGrid) {
      const dstx1 = dstx0 + (colz - cola + 1) * ts + 0.5;
      const dsty1 = dsty0 + (rowz - rowa + 1) * ts + 0.5;
      this.ctx.beginPath();
      for (let row=rowa+1, y=dsty0+ts+0.5; row<=rowz; row++, y+=ts) {
        this.ctx.moveTo(dstx0 + 0.5, y);
        this.ctx.lineTo(dstx1, y);
      }
      for (let col=cola+1, x=dstx0+ts+0.5; col<=colz; col++, x+=ts) {
        this.ctx.moveTo(x, dsty0 + 0.5);
        this.ctx.lineTo(x, dsty1);
      }
      this.ctx.strokeStyle = "#0f0";
      this.ctx.stroke();
    }
    
    /* Points of Interest.
     */
    if (this.mapPaint.showPoi) {
      for (const poi of this.mapPaint.pois) {
        let dstx = dstx0 + (poi.x - cola) * ts;
        let dsty = dsty0 + (poi.y - rowa) * ts;
        if (poi.sub & 1) dstx += ts >> 1;
        if (poi.sub & 2) dsty += ts >> 1;
        if (typeof(poi.icon) === "number") {
          this.ctx.drawImage(this.uibits, poi.icon * 16, 48, 16, 16, dstx, dsty, 16, 16);
        } else {
          this.ctx.drawImage(poi.icon, 0, 0, 16, 16, dstx, dsty, 16, 16);
        }
      }
    }
    
    if (this.retattleAfterRender) {
      this.updateTattle();
      this.retattleAfterRender = false;
    }
  }
  
  fallbackColorForCell(tileid) {
    // Never black, that's the OOB color.
    const r = (tileid | 0x1f) & 0xff;
    const g = ((tileid << 3) | 0x1f) & 0xff;
    const b = ((tileid << 6) | 0x3f) & 0xff;
    return `rgb(${r},${g},${b})`;
  }
  
  coordsMapFromEvent(x, y) {
    const bounds = this.element.getBoundingClientRect();
    x = (x - bounds.x - this.view.x) / this.view.ts + this.view.cola;
    y = (y - bounds.y - this.view.y) / this.view.ts + this.view.rowa;
    return [Math.floor(x), Math.floor(y), x % 1.0, y % 1.0];
  }
  
  onMouseLeave(event) {
    if (!this.mouseListener) this.tattle(null, null);
  }
  
  /* Registered always, on this.element.
   * Only for passively tattling the logical mouse position.
   */
  onMouseMove(event) {
    if (!this.map) return;
    if (this.mouseListener) return; // Window listener is handling it now.
    this.mouseX = event.x;
    this.mouseY = event.y;
    const [x, y] = this.coordsMapFromEvent(event.x, event.y);
    if ((x === this.mouseCol) && (y === this.mouseRow)) return;
    this.mouseCol = x;
    this.mouseRow = y;
    this.tattle(x, y);
  }
  
  // Called after render when scroll changed (tattle coords will have changed without mouse motion).
  updateTattle() {
    const [x, y] = this.coordsMapFromEvent(this.mouseX, this.mouseY);
    if ((x === this.mouseCol) && (y === this.mouseRow)) return;
    this.mouseCol = x;
    this.mouseRow = y;
    this.tattle(x, y);
  }
  
  /* Registered on window, only while tracking a drag.
   */
  onMouseUpOrMove(event) {
    if (event.type === "mouseup") {
      this.window.removeEventListener("mousemove", this.mouseListener);
      this.window.removeEventListener("mouseup", this.mouseListener);
      this.mouseListener = null;
      if (!this.map || (this.mouseCol < 0) || (this.mouseRow < 0) || (this.mouseCol >= this.map.w) || (this.mouseRow >= this.map.h)) {
        this.tattle(null, null);
      }
      this.mapPaint.end();
      return;
    }
    this.mouseX = event.x;
    this.mouseY = event.y;
    const [x, y] = this.coordsMapFromEvent(event.x, event.y);
    if ((x === this.mouseCol) && (y === this.mouseRow)) return;
    this.mouseCol = x;
    this.mouseRow = y;
    this.tattle(x, y);
    this.mapPaint.move(x, y);
  }
  
  onMouseDown(event) {
    if (this.mouseListener) return;
    if (!this.map) return;
    
    const [x, y, fx, fy] = this.coordsMapFromEvent(event.x, event.y);
    this.mouseCol = x;
    this.mouseRow = y;
    if (!this.mapPaint.begin(x, y, fx, fy)) return;
    
    this.mouseListener = e => this.onMouseUpOrMove(e);
    this.window.addEventListener("mousemove", this.mouseListener);
    this.window.addEventListener("mouseup", this.mouseListener);
  }
  
  onPaintEvent(event) {
    switch (event.id) {
      case "image": {
          this.srcbits = event.image;
          this.renderSoon();
        } break;
      case "cellsDirty": this.renderSoon(); break;
      case "zoom": this.updateSizer(); break;
      case "showGrid": this.renderSoon(); break;
      case "commandsReplaced": this.renderSoon(); break;
      case "iconsChanged": this.renderSoon(); break;
    }
  }
}
