import { Dom } from "./Dom.js";

export class SpriteEditor {
  static checkResource(res) {
    if (res.type === "sprite") return 2;
    return 0;
  }
  static getDependencies() {
    return [HTMLElement, Dom];
  }
  constructor(element, dom) {
    this.element = element;
    this.dom = dom;
    
    this.res = null;
  }
  
  setup(res) {
    this.res = res;
    console.log(`SpriteEditor.setup`, { res });
  }
}
