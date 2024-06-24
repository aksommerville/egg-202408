/* selectCustomEditor.js
 * This stub is only in play when the game declines to override it.
 * You must implement these two functions:
 *
 * selectCustomEditor(path, serial, type, qual, rid, name, format): Class | null
 *   path: File's path, under your data directory eg "image/1-my_image.png"
 *   serial: Uint8Array of file's content.
 *   type: string eg "image"
 *   qual: string, either empty or /^[a-z]{2}$/
 *   rid: number 1..65535, or zero if unknown from path.
 *   name: string eg "my_image" for "image/1-my_image.png"
 *   format: Portion of path after the last dot, forced lowercase, eg "png".
 * Return a class implementing (setup(serial, path)), or anything false to take the default.
 * See TextEditor.js for a bare-bones example of how editors should work.
 * It is perfectly fine to override standard editors, they're not special here.
 *
 * listCustomEditors(path, serial, type, qual, rid, name, format): Class[]
 *   Same inputs as selectCustomEditor.
 * Return the full set of editor classes that could conceivably open the given resource.
 * I recommend returning all custom editors in all cases. Let the user decide.
 */
 
export function selectCustomEditor(path, serial, type, qual, rid, name, format) {
  //console.log(`STUB selectCustomEditor`, { path, serial, type, qual, rid, name, format });
  return null;
}

export function listCustomEditors(path, serial, type, qual, rid, name, format) {
  return [];
}
