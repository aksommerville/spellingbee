/* MapStore.js
 * Repository of live MapRes objects.
 */
 
import { Data } from "./Data.js";
import { MapRes } from "./MapRes.js";
 
export class MapStore {
  static getDependencies() {
    return [Data];
  }
  constructor(data) {
    this.data = data;
    this.maps = {}; // [path]:MapRes
    this.dirtyListener = this.data.listenDirty(e => this.onDirty(e));
  }
  
  // (res) is optional; provide if you have it.
  getMap(path, res) {
    let map = this.maps[path];
    if (map) return map;
    if (!res) res = this.data.resv.find(r => r.path === path);
    if (res) map = new MapRes(res.serial);
    else map = new MapRes();
    this.maps[path] = map;
    return map;
  }
  
  dirty(path, map) {
    // Notify Data first; we're going to drop it from the cache during that call.
    if (!map) {
      if (!(map = this.getMap(path))) return;
    }
    this.data.dirty(path, () => map.encode());
    this.maps[path] = map;
  }
  
  // Returns [path, MapRes] for every map resource.
  getAllMaps() {
    const maps = [];
    for (const res of this.data.resv) {
      if (res.type !== "map") continue;
      maps.push([res.path, this.getMap(res.path, res)]);
    }
    return maps;
  }
  
  onDirty(event) {
    switch (event) {
      case "dirty": {
          for (const path of Object.keys(this.data.dirties)) {
            delete this.maps[path];
          }
        } break;
    }
  }
}

MapStore.singleton = true;
