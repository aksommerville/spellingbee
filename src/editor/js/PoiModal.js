import { Dom } from "./Dom.js";
import { MapPaint } from "./MapPaint.js";
import { MapCommand } from "./MapRes.js";

export class PoiModal {
  static getDependencies() {
    return [HTMLDialogElement, Dom, MapPaint];
  }
  constructor(element, dom, mapPaint) {
    this.element = element;
    this.dom = dom;
    this.mapPaint = mapPaint;
    
    this.poi = null;
    this.command = "";
  }
  
  setup(poi) {
    this.poi = poi;
    const command = this.mapPaint.map?.commands.find(c => c.id === poi.mapCommandId);
    if (command) this.command = command.kw + " " + command.args.join(" ");
    else this.command = "";
    this.buildUi();
  }
  
  setupNew(x, y) {
    this.poi = { x, y };
    this.command = `door @${x},${y} map:DST_ID @DSTX,DSTY 0 0`;
    this.buildUi();
  }
  
  buildUi() {
    this.element.innerHTML = "";
    this.dom.spawn(this.element, "INPUT", { name: "literal", value: this.command, "on-input": () => this.onLiteralChanged() });
    this.dom.spawn(this.element, "INPUT", { type: "button", value: "Save", "on-click": () => this.onSave() });
  }
  
  onLiteralChanged() {
    //TODO
  }
  
  onSave() {
    if (!this.poi) return;
    const text = this.element.querySelector("input[name='literal']").value;
    if (this.poi.mapCommandId) {
      const command = this.mapPaint.map.commands.find(c => c.id === this.poi.mapCommandId);
      if (command) {
        command.decode(text);
        this.mapPaint.broadcast({ id: "cellsDirty" });
        this.resolve();
        return;
      }
    }
    const command = new MapCommand(text);
    command.id = this.mapPaint.map.nextCommandId++;
    this.mapPaint.map.commands.push(command);
    this.mapPaint.broadcast({ id: "cellsDirty" });
    this.resolve();
    return;
  }
}
