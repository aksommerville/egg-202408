import { Rom } from "./Rom.js";
import { Egg } from "./Egg.js";

function startEgg(rom) {
  console.log(`TODO bootstrap.js:startEgg`, { rom });
  const egg = new Egg(rom);
  egg.attachToDom();
  egg.start().then(() => {
    console.log(`bootstrap: Egg started`);
  }).catch(displayError);
}

function displayError(error) {
  document.body.innerHTML = "";
  const element = document.createElement("DIV");
  document.body.appendChild(element);
  element.classList.add("error");
  console.error(error);
  if (typeof(error) === "string") {
    element.innerText = error;
  } else if (!error) {
    element.innerText = "Unspecified error.";
  } else if (error.stack) {
    element.innerText = error.stack;
  } else if (error.message) {
    element.innerText = error.message;
  } else {
    element.innerText = JSON.stringify(error, null, 2);
  }
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
      //TODO Should stop here and present these options to the user.
      // But maybe just load it, if there's exactly one.
      return window.fetch(roms[0]);
    }).then(rsp => {
      if (rsp.status === 599) {
        return rsp.text().then(msg => { throw msg; });
      }
      if (!rsp.ok) throw rsp;
      return rsp.arrayBuffer();
    }).then(serial => {
      startEgg(new Rom(serial));
    }).catch(error => {
      displayError(error);
    });
  }
});
