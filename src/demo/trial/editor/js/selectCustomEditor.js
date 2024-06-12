/* selectCustomEditor.js
 */
 
import { PrivatetypeEditor } from "./PrivatetypeEditor.js";
 
export function selectCustomEditor(path, serial, type, qual, rid, name, format) {
  //console.log(`TRIAL selectCustomEditor`, { path, serial, type, qual, rid, name, format });
  if (type === "privatetype") return PrivatetypeEditor;
  return null;
}
