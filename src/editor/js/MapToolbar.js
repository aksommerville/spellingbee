/* MapToolbar.js
 * Narrow ribbon along the top of MapEditor with all the controls.
 */
 
import { Dom } from "./Dom.js";
import { MapPaint } from "./MapPaint.js";
import { TILESIZE } from "./spellingbeeConstants.js";
import { PaletteModal } from "./PaletteModal.js";
import { CommandsModal } from "./CommandsModal.js";
import { MapSizeModal } from "./MapSizeModal.js";

export class MapToolbar {
  static getDependencies() {
    return [HTMLElement, Dom, MapPaint, "nonce"];
  }
  constructor(element, dom, mapPaint, nonce) {
    this.element = element;
    this.dom = dom;
    this.mapPaint = mapPaint;
    this.nonce = nonce;
    
    this.map = null;
    this.res = null;
    this.srcbits = mapPaint.image;
    
    this.buildUi();
    
    this.paintListener = this.mapPaint.listen(e => this.onPaintEvent(e));
  }
  
  onRemoveFromDom() {
    this.mapPaint.unlisten(this.paintListener);
  }
  
  setup(map, res) {
    this.map = map;
    this.res = res;
    this.populateUi();
  }
  
  buildUi() {
    this.element.innerHTML = "";
    this.dom.spawn(this.element, "CANVAS", ["palette"], { width: TILESIZE, height: TILESIZE, "on-click": () => this.onClickPalette() });
    
    const toolbox = this.dom.spawn(this.element, "DIV", ["toolbox"]);
    let i = 0;
    for (const tool of MapPaint.TOOLS) {
      const element = this.dom.spawn(toolbox, "DIV", ["tool"], { "on-click": () => this.onClickTool(tool.name), "data-name": tool.name });
      element.style.backgroundPositionX = (-16 * i) + "px";
      i++;
    }
    
    this.dom.spawn(this.element, "DIV", ["tattle"]);
    
    const zoom = this.dom.spawn(this.element, "SELECT", { "on-change": () => this.mapPaint.setZoom(zoom.value) });
    this.dom.spawn(zoom, "OPTION", { value: "4" }, "4x");
    this.dom.spawn(zoom, "OPTION", { value: "3" }, "3x");
    this.dom.spawn(zoom, "OPTION", { value: "2" }, "2x");
    this.dom.spawn(zoom, "OPTION", { value: "1" }, "1x");
    this.dom.spawn(zoom, "OPTION", { value: "0.500" }, "x/2");
    this.dom.spawn(zoom, "OPTION", { value: "0.250" }, "x/4");
    this.dom.spawn(zoom, "OPTION", { value: "0.125" }, "x/8");
    zoom.value = this.mapPaint.zoom;
    
    const gridlines = this.dom.spawn(this.element, "INPUT", {
      id: `MapToolbar-${this.nonce}-gridlines`,
      type: "checkbox",
      name: "gridlines",
      "on-change": () => this.mapPaint.setShowGrid(gridlines.checked),
    });
    if (this.mapPaint.showGrid) gridlines.checked = true;
    this.dom.spawn(this.element, "LABEL", { for: gridlines.id }, "Grid");
    
    this.dom.spawn(this.element, "INPUT", { type: "button", value: "Commands", "on-click": () => this.onEditCommands() });
    this.dom.spawn(this.element, "INPUT", { type: "button", name: "size", value: "Size", "on-click": () => this.onEditSize() });
    
    this.highlightTool(this.mapPaint.effectiveTool);
    this.renderPalette();
  }
  
  populateUi() {
    const sizeButton = this.element.querySelector("input[name='size']");
    if (sizeButton && this.map) {
      sizeButton.value = `${this.map.w} x ${this.map.h}`;
    } else {
      sizeButton.value = "Size";
    }
  }
  
  renderPalette() {
    const canvas = this.element.querySelector(".palette");
    if (!canvas) return;
    const ctx = canvas.getContext("2d");
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    if (!this.srcbits) return;
    const srcx = (this.mapPaint.selectedTile & 0x0f) * TILESIZE;
    const srcy = (this.mapPaint.selectedTile >> 4) * TILESIZE;
    ctx.drawImage(this.srcbits, srcx, srcy, TILESIZE, TILESIZE, 0, 0, TILESIZE, TILESIZE);
  }
  
  setTattle(x, y) {
    let desc = "";
    if (x === null) desc = "";
    else desc = `${x},${y}`;
    this.element.querySelector(".tattle").innerText = desc;
  }
  
  highlightTool(name) {
    for (const element of this.element.querySelectorAll(".toolbox .tool.selected")) element.classList.remove("selected");
    const element = this.element.querySelector(`.toolbox .tool[data-name='${name}']`);
    if (element) element.classList.add("selected");
  }
  
  onClickPalette() {
    const modal = this.dom.spawnModal(PaletteModal);
    modal.setImage(this.srcbits);
    modal.result.then(tileid => {
      if (tileid === null) return;
      this.mapPaint.setSelectedTile(tileid);
    }).catch(e => this.dom.modalError(e));
  }
  
  onClickTool(name) {
    this.mapPaint.setTool(name);
  }
  
  onEditCommands() {
    if (!this.map) return;
    const modal = this.dom.spawnModal(CommandsModal);
    modal.setup(this.map);
    modal.result.then(result => {
      if (!result) return;
      this.map.commands = result;
      this.mapPaint.broadcast({ id: "commandsReplaced" });
    }).catch(e => this.dom.modalError(e));
  }
  
  onEditSize() {
    if (!this.map) return;
    const modal = this.dom.spawnModal(MapSizeModal);
    modal.setup(this.map.w, this.map.h);
    modal.result.then(result => {
      if (!result) return;
      if (this.mapPaint.resizeMap(result)) {
        this.populateUi();
      }
    }).catch(e => this.dom.modalError(e));
  }
  
  onPaintEvent(event) {
    switch (event.id) {
      case "image": {
          this.srcbits = event.image;
          this.renderPalette();
        } break;
      case "tool": this.highlightTool(event.tool); break;
      case "selectedTile": this.renderPalette(); break;
    }
  }
}
