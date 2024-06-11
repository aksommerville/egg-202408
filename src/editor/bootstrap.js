import { Injector } from "./js/Injector.js";
import { Dom } from "./js/Dom.js";
import { Comm } from "./js/Comm.js";
import { Bus } from "./js/Bus.js";
import { RootUi } from "./js/RootUi.js";
import { Resmgr } from "./js/Resmgr.js";

window.addEventListener("load", () => {
  const injector = new Injector(window, document);
  const dom = injector.getInstance(Dom);
  const bus = injector.getInstance(Bus);
  const comm = injector.getInstance(Comm);
  const resmgr = injector.getInstance(Resmgr);
  const rootUi = dom.replaceContentWithController(document.body, RootUi);
  bus.listen(["error"], event => {
    console.error(event.error);
    dom.modalError(event.error);
  });
  comm.httpJson("GET", "/api/roms").then(rsp => {
    bus.setRoms(rsp);
  }).catch(error => bus.broadcast({ type: "error", error }));
  resmgr.fetchAll().then(toc => {
    bus.setToc(toc);
    bus.setStatus("ok");
  }).catch(error => {
    bus.broadcast({ type: "error", error });
    bus.setStatus("dead");
  });
});
