/* PaletteModal.js
 * Shows a tilesheet image and resolves with the tileid clicked on.
 */
 
import { TILESIZE } from "./spellingbeeConstants.js";
import { Dom } from "./Dom.js";
 
export class PaletteModal {
  static getDependencies() {
    return [HTMLDialogElement, Dom];
  }
  constructor(element, dom) {
    this.element = element;
    this.dom = dom;
    
    this.canvas = this.dom.spawn(this.element, "CANVAS");
    this.canvas.width = TILESIZE * 16;
    this.canvas.height = TILESIZE * 16;
    
    this.canvas.addEventListener("click", e => this.onClick(e));
  }
  
  setImage(image) {
    const ctx = this.canvas.getContext("2d");
    ctx.drawImage(image, 0, 0);
    ctx.beginPath();
    for (let i=1; i<16; i++) {
      ctx.moveTo(0, TILESIZE * i);
      ctx.lineTo(TILESIZE * 16, TILESIZE * i);
      ctx.moveTo(TILESIZE * i, 0);
      ctx.lineTo(TILESIZE * i, TILESIZE * 16);
    }
    ctx.strokeStyle = "#0f0";
    ctx.globalAlpha = 0.5;
    ctx.stroke();
    ctx.globalAlpha = 1.0;
  }
  
  onClick(event) {
    const bounds = this.canvas.getBoundingClientRect();
    const x = event.x - bounds.x;
    const y = event.y - bounds.y;
    const col = Math.floor((x * 16) / bounds.width);
    if ((col < 0) || (col > 15)) return;
    const row = Math.floor((y * 16) / bounds.height);
    if ((row < 0) || (row > 15)) return;
    const tileid = (row << 4) | col;
    this.resolve(tileid);
  }
}
