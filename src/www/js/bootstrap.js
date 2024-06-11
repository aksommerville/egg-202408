import { Rom } from "./Rom.js";
import { Egg } from "./Egg.js";

function startEgg(rom) {
  const egg = new Egg(rom);
  egg.attachToDom();
  egg.start().then(() => {
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
  if (emb) {
    // We have an embedded ROM. Easiest case, and unambiguous what to do.
    startEgg(new Rom(emb.innerText));
  } else {
    // If we were launched with '?delivery=message', wait for our parent to postMessage with the serial ROM.
    const query = window.location.search || "";
    if (query.startsWith("?")) {
      const qo = {};
      for (const encoded of query.substring(1).split('&')) {
        const sepp = encoded.indexOf('=');
        if (sepp >= 0) {
          const k = decodeURIComponent(encoded.substring(0, sepp));
          const v = decodeURIComponent(encoded.substring(sepp + 1));
          qo[k] = v;
        } else {
          qo[decodeURIComponent(encoded)] = "";
        }
      }
      if (qo.delivery === "message") {
        window.addEventListener("message", event => {
          startEgg(new Rom(event.data));
        });
        return;
      }
    }
    // Assume the dev server is running and ask it for a ROM.
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

