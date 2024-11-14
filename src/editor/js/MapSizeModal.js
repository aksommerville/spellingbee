/* MapSizeModal.js
 */
 
import { Dom } from "./Dom.js";

export class MapSizeModal {
  static getDependencies() {
    return [HTMLDialogElement, Dom];
  }
  constructor(element, dom) {
    this.element = element;
    this.dom = dom;
    
    this.w = 0;
    this.h = 0;
    
    this.buildUi();
  }
  
  setup(w, h) {
    this.w = w;
    this.h = h;
    this.populateUi();
  }
  
  buildUi() {
    this.element.innerHTML = "";
    this.dom.spawn(this.element, "DIV", ["prompt"]);
    this.dom.spawn(this.element, "DIV", ["row"],
      this.dom.spawn(null, "INPUT", { type: "number", min: 0, max: 255, name: "w" }),
      this.dom.spawn(null, "DIV", "x"),
      this.dom.spawn(null, "INPUT", { type: "number", min: 0, max: 255, name: "h" })
    );
    this.dom.spawn(this.element, "DIV", ["row"],
      this.dom.spawn(null, "DIV", "Anchor"),
      this.dom.spawn(null, "SELECT", { name: "anchor" },
        this.dom.spawn(null, "OPTION", { value: "nw", selected: "selected" }, "NW"),
        this.dom.spawn(null, "OPTION", { value: "n" }, "N"),
        this.dom.spawn(null, "OPTION", { value: "ne" }, "NE"),
        this.dom.spawn(null, "OPTION", { value: "w" }, "W"),
        this.dom.spawn(null, "OPTION", { value: "mid" }, "Center"),
        this.dom.spawn(null, "OPTION", { value: "e" }, "E"),
        this.dom.spawn(null, "OPTION", { value: "sw" }, "SW"),
        this.dom.spawn(null, "OPTION", { value: "n" }, "S"),
        this.dom.spawn(null, "OPTION", { value: "se" }, "SE")
      )
    );
    this.dom.spawn(this.element, "DIV", ["row"],
      this.dom.spawn(null, "INPUT", { type: "button", value: "OK", "on-click": () => this.onSubmit() })
    );
  }
  
  populateUi() {
    this.element.querySelector(".prompt").innerText = `Resize map from ${this.w}x${this.h}...`;
    this.element.querySelector("input[name='w']").value = this.w;
    this.element.querySelector("input[name='h']").value = this.h;
  }
  
  onSubmit() {
    const w = +this.element.querySelector("input[name='w']").value;
    const h = +this.element.querySelector("input[name='h']").value;
    if (isNaN(w) || (w < 0) || (w > 255) || isNaN(h) || (h < 0) || (h > 255)) return;
    const anchor = this.element.querySelector("select[name='anchor']").value;
    this.resolve({ w, h, anchor });
  }
}
