/* Launcher.js
 * Manages loading the real Egg Runtime and starting it up in a modal.
 */
 
import { Dom } from "./Dom.js";

export class Launcher {
  static getDependencies() {
    return [Dom];
  }
  constructor(dom) {
    this.dom = dom;
    this.running = false;
  }
  
  launchSerial(serial) {
    if (this.running) return;
    const controller = this.dom.modalIframe("/rt/index.html?delivery=message");
    const iframe = controller.element.querySelector("iframe");
    this.running = true;
    iframe.addEventListener("load", () => {
      try {
        iframe.contentWindow.postMessage(serial);
      } catch (error) {
        console.log(`iframe.contentWindow.postMessage failed`, error);
      }
    });
    controller.result.catch(() => {}).then(() => {
      this.running = false;
    });
  }
}

Launcher.singleton = true;
