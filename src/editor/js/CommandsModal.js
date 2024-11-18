/* CommandsModal.js
 * Mutable list of map commands.
 * Does not interact with MapPaint; our caller is responsible for loading and saving.
 * We take a full MapRes at setup, but resolve with only an array of MapCommand.
 */
 
import { Dom } from "./Dom.js";
import { MapCommand } from "./MapRes.js";
import { PoiModal } from "./PoiModal.js";

export class CommandsModal {
  static getDependencies() {
    return [HTMLDialogElement, Dom];
  }
  constructor(element, dom) {
    this.element = element;
    this.dom = dom;
    
    this.map = null; // Read only.
  }
  
  setup(map) {
    this.map = map;
    this.buildUi();
  }
  
  buildUi() {
    this.element.innerHTML = "";
    const list = this.dom.spawn(this.element, "DIV", ["list"]);
    if (this.map) {
      for (const command of this.map.commands) {
        this.spawnRow(list, command);
      }
    }
    const controls = this.dom.spawn(this.element, "DIV", ["controls"]);
    this.dom.spawn(controls, "INPUT", { type: "button", value: "OK", "on-click": () => this.onSubmit() });
    this.dom.spawn(controls, "INPUT", { type: "button", value: "+", "on-click": () => this.onAddCommand() });
    this.validateAll();
  }
  
  // Both parameters optional.
  spawnRow(list, command) {
    if (!list) list = this.element.querySelector(".list");
    const row = this.dom.spawn(list, "DIV", ["row"]);
    this.dom.spawn(row, "DIV", ["buttons"],
      this.dom.spawn(null, "INPUT", { type: "button", value: "X", "on-click": () => row.remove() }),
      this.dom.spawn(null, "INPUT", { type: "button", value: "^", "on-click": () => this.moveRow(row, -1) }),
      this.dom.spawn(null, "INPUT", { type: "button", value: "v", "on-click": () => this.moveRow(row, 1) }),
      this.dom.spawn(null, "DIV", ["validation"], { title: "" }, "\u00a0"),
      this.dom.spawn(null, "INPUT", { type: "button", value: "...", "on-click": () => this.editRow(row) })
    );
    this.dom.spawn(row, "DIV", ["command"],
      this.dom.spawn(null, "INPUT", { type: "text", "on-input": () => this.validateRow(row), value: command?.encode() || "" })
    );
    return row;
  }
  
  validateRow(row) {
    /*TODO Re-implement validation, in PoiModal statically.
    if (!row) return;
    const validation = row.querySelector(".validation");
    const input = row.querySelector(".command > input");
    if (!validation || !input) return;
    const result = this.validateText(input.value || "");
    validation.classList.remove("valid");
    validation.classList.remove("invalid");
    validation.classList.remove("questionable");
    validation.classList.add(result.status);
    validation.setAttribute("title", result.message || "");
    /**/
  }
  
  validateAll() {
    for (const row of this.element.querySelectorAll(".row")) this.validateRow(row);
  }
  
  readModelFromUi() {
    const dst = [];
    for (const input of this.element.querySelectorAll(".command > input")) {
      dst.push(new MapCommand(input.value || ""));
    }
    return dst;
  }
  
  moveRow(row, d) {
    const list = this.element.querySelector(".list");
    const kids = Array.from(list.children || []);
    const index = kids.indexOf(row);
    if (index < 0) return;
    if (d < 0) {
      if (index <= 0) return;
      list.insertBefore(row, kids[index-1]);
    } else if (d > 0) {
      if (index >= kids.length - 1) return;
      list.insertBefore(row, kids[index+2]);
    }
  }
  
  onAddCommand() {
    const row = this.spawnRow();
    this.validateRow(row);
  }
  
  onSubmit() {
    this.resolve(this.readModelFromUi());
  }
  
  editRow(row) {
    const input = row.querySelector(".command > input");
    const modal = this.dom.spawnModal(PoiModal);
    modal.setupText(input.value);
    modal.result.then(result => {
      if (!result) return;
      input.value = result;
      this.validateRow(row);
    }).catch(e => this.dom.modalError(e));
  }
}
