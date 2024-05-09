import { Rom } from "./Rom.js";

function startEgg(rom) {
  console.log(`TODO bootstrap.js:startEgg`, { rom });
}

window.addEventListener("load", () => {
  let rom;
  const emb = document.querySelector("egg-rom");
  if (emb) startEgg(new Rom(emb.innerText));
  else {
    window.fetch("/api/roms").then(rsp => {
      if (!rsp.ok) throw rsp;
      return rsp.json();
    }).then(roms => {
      console.log(`available roms from dev server`, roms);
      //TODO Should stop here and present these options to the user.
      // But maybe just load it, if there's exactly one.
      return window.fetch(roms[0]);
    }).then(rsp => {
      if (!rsp.ok) throw rsp;
      return rsp.arrayBuffer();
    }).then(serial => {
      console.log(`download complete`, { serial });
      startEgg(new Rom(serial));
    }).catch(error => {
      console.error(`Download failed!`, error);
    });
  }
});
