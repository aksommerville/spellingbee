import { Dom } from "./Dom.js";
import { MapRes } from "./MapRes.js";
import { MapToolbar } from "./MapToolbar.js";
import { MapCanvas } from "./MapCanvas.js";
import { MapPaint } from "./MapPaint.js";
import { Data } from "./Data.js";
import { MapStore } from "./MapStore.js";

export class MapEditor {
  static checkResource(res) {
    if (res.type === "map") return 2;
    return 0;
  }
  static getDependencies() {
    return [HTMLElement, Dom, MapPaint, Window, Data, MapStore];
  }
  constructor(element, dom, mapPaint, window, data, mapStore) {
    this.element = element;
    this.dom = dom;
    this.mapPaint = mapPaint;
    this.window = window;
    this.data = data;
    this.mapStore = mapStore;
    
    this.res = null;
    this.map = null;
    this.mapToolbar = this.dom.spawnController(this.element, MapToolbar);
    this.mapCanvas = this.dom.spawnController(this.element, MapCanvas);
    
    this.mapCanvas.tattle = (x, y) => this.mapToolbar.setTattle(x, y);
    
    this.keyListener = e => this.onKey(e);
    this.window.addEventListener("keydown", this.keyListener);
    this.window.addEventListener("keyup", this.keyListener);
    
    this.paintListener = this.mapPaint.listen(e => this.onPaintEvent(e));
  }
  
  onRemoveFromDom() {
    this.mapPaint.reset(this, null, null);
    this.mapPaint.unlisten(this.paintListener);
    this.window.removeEventListener("keydown", this.keyListener);
    this.window.removeEventListener("keyup", this.keyListener);
  }
  
  setup(res) {
    this.res = res;
    this.map = this.mapStore.getMap(res.path, res);
    this.mapPaint.reset(this, this.map, this.res);
    this.mapToolbar.setup(this.map, this.res);
    this.mapCanvas.setup(this.map, this.res);
    if (this.mapPaint.jumpLocation) {
      const x = this.mapPaint.jumpLocation[0] || 0;
      const y = this.mapPaint.jumpLocation[1] || 0;
      this.mapPaint.jumpLocation = null;
      this.mapCanvas.scrollToCell(x, y);
    }
  }
  
  onKey(event) {
    switch (event.code) {
      case "ShiftLeft": case "ShiftRight": this.mapPaint.setShiftKey(event.type === "keydown"); break;
      case "ControlLeft": case "ControlRight": this.mapPaint.setControlKey(event.type === "keydown"); break;
      case "AltLeft": case "AltRight": this.mapPaint.setAltKey(event.type === "keydown"); break;
    }
    
    if (document.querySelector("dialog[open]")) {
      return;
    }
    
    if ((event.type === "keydown") && event.code.startsWith("Digit")) {
      const digit = +event.code.substring(5);
      const tool = MapPaint.TOOLS[digit - 1];
      if (tool) {
        this.mapPaint.setTool(tool.name);
        event.stopPropagation();
      }
    }
  }
  
  onDirty() {
    if (!this.res || !this.map) return;
    this.mapStore.dirty(this.res.path, this.map);
  }
  
  onPaintEvent(event) {
    switch (event.id) {
      case "cellsDirty": this.onDirty(); break;
      case "commandsReplaced": this.onDirty(); break;
    }
  }
}
