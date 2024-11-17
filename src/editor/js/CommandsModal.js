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
  
  //XXX --------------------------------- Most of the below should move to PoiModal, it will be the authority for specific command formats.
  
  /* Given a list of command arguments (no keyword), return their compiled length in bytes.
   * If there's a lexical error, return a validation error {status,message} instead.
   */
  measureArguments(args) {
    let len = 0;
    for (const arg of args) {
      
      // Hexadecimal literals, length is determined by input length, and must be even.
      if (arg.startsWith("0x")) {
        if (arg.length & 1) return { status: "invalid", message: "Hex literal must have even length." };
        len += ((arg.length - 2) >> 1);
        continue;
      }
      
      // Naked integers are a single byte. (even if >255). Assume that anything starting with a digit is a valid integer.
      if (arg.match(/^\d/)) {
        len++;
        continue;
      }
      
      // Integers with a cast prefix emit that explicit length. Assume the suffix is valid.
      if (arg.startsWith("(u8)")) { len += 1; continue; }
      if (arg.startsWith("(u16)")) { len += 2; continue; }
      if (arg.startsWith("(u24)")) { len += 3; continue; }
      if (arg.startsWith("(u32)")) { len += 4; continue; }
      
      // Quoted strings are evaluated as JSON, then emitted verbatim.
      if (arg.startsWith('"')) {
        try {
          const v = JSON.parse(arg);
          len += v.length;
        } catch (e) {
          return { status: "invalid", message: "Malformed string token." };
        }
        continue;
      }
      
      // '@' followed by comma-delimited integers, one byte each. (should be 2 or 4 but whatever).
      if (arg.startsWith('@')) {
        len += arg.split(',').length;
        continue;
      }
      
      // "flag:NAME" emits a flag ID in one byte. Important to check this before resources.
      if (arg.startsWith("flag:")) {
        len += 1;
        continue;
      }
      
      // "TYPE:NAME" emits a resource ID in two bytes.
      if (arg.indexOf(":") >= 0) {
        len += 2;
        continue;
      }
      
      // Anything else is a hard error.
      return { status: "invalid", message: `Unexpected argument ${JSON.stringify(arg)}` };
    }
    return len;
  }
  
  /* Returns {status,message} where (status) is a class applicable to `.validation`.
   * Validation is entirely passive. Commands are just text, so we can allow invalid ones, no problem.
   * And later when it fails compile, we can say I told you so.
   */
  validateText(src) {
    return { status: "valid", message: "" };//XXX Move this to PoiModal and repair
    const words = src.split(/\s+/g).filter(v => v);
    if (words.length < 1) return { status: "invalid", message: "Empty" };
    const schema = CommandsModal.COMMANDS[words[0]];
    if (!schema) return { status: "invalid", message: `Unknown command ${JSON.stringify(words[0])}` };
    const args = words.slice(1); // Just to make indices line up.
    
    /* We don't record opcodes. If we did, we could validate argument lengths generically.
     * Instead, we'll depend on the schema adding up correctly.
     */
    const expectlen = schema.reduce((a, v) => a + v[0], 0);
    const actuallen = this.measureArguments(args);
    if (typeof(actuallen) === "object") return actuallen;
    
    if (expectlen !== actuallen) return {
      status: "invalid",
      message: `Expected ${expectlen} bytes argument, found ${actuallen}. Per schema: ${schema.map(([c, n]) => `${c}:${n}`).join(", ")}`,
    };
    if (schema.length !== args.length) return {
      status: "questionable",
      message: `Total length ${actuallen} is correct, but you got there with ${args.length} tokens and schema lists ${schema.length}: ${schema.map(([c, n]) => `${c}:${n}`).join(", ")}`,
    };
    
    //TODO We could certainly validate harder.
    
    return { status: "valid", message: "" };
  }
}
