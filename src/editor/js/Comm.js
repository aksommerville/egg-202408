/* Comm.js
 * HTTP and WebSocket interface.
 */
 
export class Comm {
  static getDependencies() {
    return [Window];
  }
  constructor(window) {
    this.window = window;
  }
  
  http(method, path, query, headers, body, returnType) {
    const options = { method };
    if (headers) options.headers = headers;
    if (body) options.body = body;
    return this.window.fetch(this.composeHttpUrl(path, query), options).then(rsp => {
      if (!rsp.ok) return rsp.text().then(body => { throw body || `${method} ${path} => ${rsp.status}`; });
      return this.getHttpResponse(rsp, returnType);
    });
  }
  
  // ...and since returnType is all the way at the end, after the optional args, some conveniences:
  httpText(method, path, query, headers, body) { return this.http(method, path, query, headers, body, "text"); }
  httpBinary(method, path, query, headers, body) { return this.http(method, path, query, headers, body, "arrayBuffer"); }
  httpJson(method, path, query, headers, body) { return this.http(method, path, query, headers, body, "json"); }
  
  composeHttpUrl(path, query) {
    if (query) path += '?' + Object.keys(query).map(k => encodeUriComponent(k) + '=' + encodeUriComponent(query[k])).join('&');
    return path;
  }
  
  getHttpResponse(rsp, returnType) {
    switch (returnType) {
      case "arrayBuffer": return rsp.arrayBuffer();
      case "text": return rsp.text();
      case "json": return rsp.json();
    }
    return rsp.arrayBuffer();
  }
  
}

Comm.singleton = true;
