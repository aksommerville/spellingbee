/* PickImageModal.js
 * Shows all image resources visually, and resolves with one of their names.
 */
 
import { Dom } from "./Dom.js";
import { Data } from "./Data.js";

export class PickImageModal {
  static getDependencies() {
    return [HTMLDialogElement, Dom, Data];
  }
  constructor(element, dom, data) {
    this.element = element;
    this.dom = dom;
    this.data = data;
    
    this.buildUi();
  }
  
  buildUi() {
    this.element.innerHTML = "";
    const colv = [];
    colv.push(this.dom.spawn(this.element, "DIV", ["col"]));
    colv.push(this.dom.spawn(this.element, "DIV", ["col"]));
    colv.push(this.dom.spawn(this.element, "DIV", ["col"]));
    let colp = 0;
    for (const res of this.data.resv) {
      if (res.type !== "image") continue;
      if (colp >= colv.length) colp = 0;
      const button = this.dom.spawn(colv[colp++], "BUTTON", { "on-click": () => this.onChooseImage(res) });
      const name = this.dom.spawn(button, "DIV", ["name"], res.name || res.rid || res.path);
      // 1. We could save some trouble in cases where the image is already loaded, but meh.
      // 2. I dislike the idea of inserting images owned by Data into the DOM. Can we clone them instead?
      // ...This does not appear possible, since the Image was built from a Blob which has already been revoked.
      // ...I guess we could shovel it out to a Canvas and compose a new image from that, but who cares.
      // 3. Is it worth allowing our owner to filter images? eg "only tilesheets"
      this.data.getImageAsync(res.name || res.rid).then(image => {
        button.insertBefore(image, name);
      }).catch(e => {});
    }
  }
  
  onChooseImage(res) {
    if (res.name) this.resolve(`image:${res.name}`);
    else if (res.rid) this.resolve(`image:${res.rid}`);
    else this.resolve(res.path);
  }
}
