/* GenericModal.js
 */
 
import { Dom } from "./Dom.js";

export class GenericModal {
  static getDependencies() {
    return [HTMLDialogElement, Dom];
  }
  constructor(element, dom) {
    this.element = element;
    this.dom = dom;
  }
  
  setupPickOne(options, prompt) {
    this.element.innerText = "";
    if (prompt) {
      this.dom.spawn(this.element, "DIV", prompt);
    }
    for (const option of options) {
      this.dom.spawn(this.element, "INPUT", { type: "button", value: option, "on-click": () => this.resolve(option) });
    }
  }
  
  setupError(error) {
    this.element.innerHTML = "";
    if (!error) this.element.innerText = "Unspecified error";
    else if (error.stack) this.element.innerText = error.stack;
    else if (error.message) this.element.innerText = error.message;
    else this.element.innerText = error;
    this.element.classList.add("error");
  }
  
  setupIframe(url) {
    this.element.innerHTML = "";
    const iframe = this.dom.spawn(this.element, "IFRAME", { src: url });
  }
}
