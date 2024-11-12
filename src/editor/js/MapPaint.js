/* MapPaint.js
 * Manages the model layer of map editing.
 * UI affairs are more in MapCanvas and MapToolbar.
 * We're a singleton. MapEditor resets us and takes control, there's one editor at a time (or none).
 * We serve as a communication bus between the various map-related UI components.
 */
 
import { Data } from "./Data.js";
import { Tilesheet } from "./Tilesheet.js";
import { MapRes } from "./MapRes.js";
import { Dom } from "./Dom.js";
import { PoiModal } from "./PoiModal.js";
import { SpriteEditor } from "./SpriteEditor.js";
import { TILESIZE } from "./spellingbeeConstants.js";
import { MapStore } from "./MapStore.js";
 
export class MapPaint {
  static getDependencies() {
    return [Data, Dom, MapStore, Window];
  }
  constructor(data, dom, mapStore, window) {
    this.data = data;
    this.dom = dom;
    this.mapStore = mapStore;
    this.window = window;
    
    this.jumpLocation = null; // null or [col,row]. If set on load, focus that cell (travelled thru door).
    this.editor = null;
    this.map = null;
    this.res = null;
    this.mouseCol = 0;
    this.mouseRow = 0;
    this.tool = "rainbow";
    this.effectiveTool = "rainbow";
    this.runningTool = "";
    this.selectedTile = 0x00;
    this.image = null;
    this.imageName = "";
    this.tilesheet = null;
    this.listeners = []; // {cb,id}
    this.nextListenerId = 1;
    this.controlKey = false;
    this.shiftKey = false;
    this.altKey = false;
    this.anchorx = 0; // monalisa,poimove
    this.anchory = 0;
    this.anchortile = 0; // monalisa
    this.zoom = 4;
    this.showGrid = true;
    this.showPoi = true;
    this.pois = []; // {x,y,sub:0..3,mapCommandId,icon} (icon) is 0..4, column in uibits row 3. Or an Image.
    // spriteIcons doesn't update when the sprite resources change. I think that's not worth wiring up. Just refresh if you change them.
    this.spriteIcons = {}; // Keyed by name (eg "sprite:dragon"), value is Image or the number 3.
  }
  
  reset(editor, map, res) {
    if (editor === this.editor) {
      if (!map || !res) {
        this.editor = null;
        this.map = null;
        this.res = null;
      } else {
        this.map = map;
        this.res = res;
      }
    } else if (!map || !res) {
      // Client bids farewell, but it seems we already have a new client. No worries.
      return;
    } else {
      this.editor = editor;
      this.map = map;
      this.res = res;
    }
    this.runningTool = "";
    
    this.regeneratePois();
    
    const imageName = map ? map.getImageName() : "";
    if (imageName !== this.imageName) {
      this.imageName = imageName;
      if (imageName) {
        this.data.getImageAsync(imageName).then(image => {
          this.image = image;
          this.broadcast({ id: "image", image });
        }).catch(e => console.error(e));
      } else {
        this.image = null;
      }
      this.tilesheet = this.tilesheetForImageName(this.imageName);
    }
    this.controlKey = false;
    this.shiftKey = false;
    this.altKey = false;
    this.effectiveTool = this.tool;
  }
  
  drawSpritePoiIcon(res, key) {
    if (!res) return 3;
    const imageParams = SpriteEditor.imageParamsFromSerial(res.serial);
    if (!imageParams) return 3;
    this.data.getImageAsync(imageParams.name).then(image => {
      const canvas = document.createElement("CANVAS");
      canvas.width = TILESIZE;
      canvas.height = TILESIZE;
      const ctx = canvas.getContext("2d");
      ctx.fillStyle = "#0f0"; // Don't let it be transparent, in case the icon is invalid.
      ctx.fillRect(0, 0, TILESIZE, TILESIZE);
      // Not bothering with xform.
      ctx.drawImage(image, (imageParams.tileid & 15) * TILESIZE, (imageParams.tileid >> 4) * TILESIZE, TILESIZE, TILESIZE, 0, 0, TILESIZE, TILESIZE);
      this.spriteIcons[key] = canvas;
      for (const p of this.pois) if (p.awaitingImage === key) {
        p.icon = canvas;
        delete p.awaitingImage;
      }
      this.broadcast({ id: "iconsChanged" });
    }).catch(e => console.error(e));
    return 3;
  }
  
  /* Return the number 3 (the cat POI icon), or an Image for this sprite command.
   */
  getSpritePoiIcon(command, poi) {
    if (command.args?.length >= 1) {
      const name = command.args[0];
      poi.awaitingImage = name;
      if (this.spriteIcons[name]) return this.spriteIcons[name];
      return this.spriteIcons[name] = this.drawSpritePoiIcon(this.data.resByString(name), name);
    }
    return 3;
  }
  
  regeneratePois() {
    this.pois = [];
    if (!this.map) return;
    for (const command of this.map.commands) {
      let x=-1, y=-1;
      for (const arg of command.args) {
        const match = arg.match(/^@(\d+),(\d+)(,\d+,\d+)?$/);
        if (!match) continue;
        x = +match[1];
        y = +match[2];
        break;
      }
      if ((x >= 0) && (y >= 0)) {
        const poi = {
          x, y,
          sub: 0,
          mapCommandId: command.id,
          icon: -1,
        };
        switch (command.kw) {
          case "hero": poi.icon = 4; break;
          case "door": poi.icon = 0; break;
          case "sprite": poi.icon = this.getSpritePoiIcon(command, poi); break;
          case "message": poi.icon = 2; break;
        }
        for (const existing of this.pois) {
          if (existing.x !== x) continue;
          if (existing.y !== y) continue;
          if (existing.sub < poi.sub) continue;
          poi.sub = existing.sub + 1;
        }
        this.pois.push(poi);
      }
    }
    /* Search all the other maps for doors leading into this one.
     * First time this runs, it probably instantiates the entire set, but that probably only happens once per run.
     */
    for (const [path, other] of this.mapStore.getAllMaps()) {
      if (path === this.res.path) continue;
      for (const command of other.commands) {
        if (command.kw !== "door") continue;
        const rid = this.data.resolveId(command.args[1]);
        if (rid !== this.res.rid) continue;
        const match = command.args[2].match(/^@(\d+),(\d+)(,\d+,\d+)?$/);
        if (!match) continue;
        const x = +match[1];
        const y = +match[2];
        const rmatch = command.args[0].match(/^@(\d+),(\d+)(,\d+,\d+)?$/);
        let remoteX=-1, remoteY=-1;
        if (rmatch) {
          remoteX = +rmatch[1];
          remoteY = +rmatch[2];
        }
        const poi = {
          x, y,
          sub: 0,
          mapCommandId: command.id,
          icon: 1,
          remoteMapPath: path,
          remoteX, remoteY,
        };
        for (const existing of this.pois) {
          if (existing.x !== x) continue;
          if (existing.y !== y) continue;
          if (existing.sub < poi.sub) continue;
          poi.sub = existing.sub + 1;
        }
        this.pois.push(poi);
      }
    }
  }
  
  tilesheetForImageName(imageName) {
    const image = this.data.resv.find(r => ((r.type === "image") && (r.name === imageName)));
    if (!image) return null;
    const res = this.data.resv.find(r => ((r.type === "tilesheet") && (r.rid === image.rid)));
    if (!res) return null;
    return new Tilesheet(res.serial);
  }
  
  /* Event bus.
   * Currently defined:
   *   { id:"tool", tool:string }
   *   { id:"image", image:Image }
   *   { id:"cellsDirty" }
   *   { id:"selectedTile" }
   *   { id:"zoom", zoom:number }
   *   { id:"showGrid", showGrid:boolean }
   *   { id:"commandsReplaced" }
   *   { id:"iconsChanged" }
   *************************************************************/
   
  listen(cb) {
    const id = this.nextListenerId++;
    this.listeners.push({ cb, id });
    return id;
  }
  
  unlisten(id) {
    const p = this.listeners.findIndex(l => l.id === id);
    if (p >= 0) this.listeners.splice(p, 1);
  }
  
  broadcast(event) {
    switch (event.id) {
      case "commandsReplaced": this.regeneratePois(); break;
    }
    for (const { cb } of this.listeners) cb(event);
  }
  
  /* Events from MapEditor.
   ****************************************************************/
   
  setShiftKey(v) {
    v = !!v;
    if (v === this.shiftKey) return;
    this.shiftKey = v;
    this.refreshEffectiveTool();
  }
  
  setControlKey(v) {
    v = !!v;
    if (v === this.controlKey) return;
    this.controlKey = v;
    this.refreshEffectiveTool();
  }
  
  setAltKey(v) {
    v = !!v;
    if (v === this.altKey) return;
    this.altKey = v;
    this.refreshEffectiveTool();
  }
  
  refreshEffectiveTool() {
    let et = this.tool;
    const tool = MapPaint.TOOLS.find(t => t.name === this.tool);
    if (tool) {
      if (this.altKey && this.controlKey && this.shiftKey && tool.altControlShift) et = tool.altControlShift;
      else if (this.altKey && this.controlKey && tool.altControl) et = tool.altControl;
      else if (this.altKey && tool.alt) et = tool.alt;
      else if (this.controlKey && this.shiftKey && tool.controlShift) et = tool.controlShift;
      else if (this.controlKey && tool.control) et = tool.control;
      else if (this.shiftKey && tool.shift) et = tool.shift;
    }
    if (et === this.effectiveTool) return;
    this.effectiveTool = et;
    this.broadcast({ id: "tool", tool: this.effectiveTool });
  }
  
  /* Events from MapToolbar.
   ******************************************************************/
   
  setTool(name) {
    if (this.tool === name) return;
    this.tool = name;
    this.effectiveTool = name;
    this.broadcast({ id: "tool", tool: name });
  }
  
  setSelectedTile(tileid) {
    if (tileid === this.selectedTile) return;
    if (isNaN(tileid) || (tileid < 0) || (tileid > 0xff)) return;
    this.selectedTile = tileid;
    this.broadcast({ id: "selectedTile" });
  }
  
  setZoom(n) {
    if (n === this.zoom) return;
    this.zoom = n;
    this.broadcast({ id: "zoom", zoom: this.zoom });
  }
  
  setShowGrid(show) {
    if (show) {
      if (this.showGrid) return;
      this.showGrid = true;
    } else {
      if (!this.showGrid) return;
      this.showGrid = false;
    }
    this.broadcast({ id: "showGrid", showGrid: this.showGrid });
  }
  
  /* Events from MapCanvas.
   **************************************************************/
  
  /* Returns true if something started. Caller should track motion and tell us move() and eventually end().
   * If we return false, no further action is necessary.
   */
  begin(x, y, fx, fy) {
    if (this.runningTool) return true;
    if (!this.map) return false;
    if ((x < 0) || (y < 0) || (x >= this.map.w) || (y >= this.map.h)) return false;
    switch (this.effectiveTool) {
      case "pencil": this.pencilMove(x, y); this.runningTool = "pencil"; return true;
      case "rainbow": this.pencilMove(x, y); this.healMove(x, y); this.runningTool = "rainbow"; return true;
      case "monalisa": this.monalisaBegin(x, y); this.runningTool = "monalisa"; return true;
      case "pickup": this.pickupBegin(x, y); return false;
      case "poimove": return this.poimoveBegin(x, y, fx, fy);
      case "poiedit": this.poieditBegin(x, y, fx, fy); return false;
      case "poidelete": this.poideleteBegin(x, y, fx, fy); return false;
      case "heal": this.healMove(x, y); this.runningTool = "heal"; return true;
      case "door": this.doorBegin(x, y, fx, fy); return false;
    }
    return false;
  }
  
  move(x, y) {
    if (x < 0) x = 0; else if (x >= this.map.w) x = this.map.w - 1;
    if (y < 0) y = 0; else if (y >= this.map.h) y = this.map.h - 1;
    switch (this.runningTool) {
      case "pencil": this.pencilMove(x, y); break;
      case "rainbow": this.pencilMove(x, y); this.healMove(x, y); break;
      case "monalisa": this.monalisaMove(x, y); break;
      case "poimove": this.poimoveMove(x, y); break;
      case "heal": this.healMove(x, y); break;
    }
  }
  
  end() {
    this.runningTool = "";
  }
  
  /* pencil: Copy selected tile to the map.
   */
   
  pencilMove(x, y) {
    const p = y * this.map.w + x;
    if (this.map.v[p] === this.selectedTile) return;
    this.map.v[p] = this.selectedTile;
    this.broadcast({ id: "cellsDirty" });
  }
  
  /* monalisa: Copy a range of tiles from the palette to the map, anchoring selected tile against mouse down cell.
   */
   
  monalisaBegin(x, y) {
    this.anchorx = x;
    this.anchory = y;
    this.anchortile = this.selectedTile;
    this.monalisaMove(x, y);
  }
  
  monalisaMove(x, y) {
    const dx = x - this.anchorx;
    const dy = y - this.anchory;
    const tx = (this.anchortile & 0x0f) + dx;
    const ty = (this.anchortile >> 4) + dy;
    if ((tx < 0) || (ty < 0) || (tx >= 16) || (ty >= 16)) return;
    const tileid = (ty << 4) | tx;
    if (this.map.v[y * this.map.w + x] === tileid) return;
    this.map.v[y * this.map.w + x] = tileid;
    this.broadcast({ id: "cellsDirty" });
  }
  
  /* pickup: Change selectedTile to whatever's in the map here.
   */
   
  pickupBegin(x, y) {
    const p = y * this.map.w + x;
    if (this.map.v[p] === this.selectedTile) return;
    this.selectedTile = this.map.v[p];
    this.broadcast({ id: "selectedTile" });
  }
  
  /* heal: Reassess neighbor and random relationships for the named cell and its neighbors.
   * The rainbow pencil is just pencil+heal.
   */
   
  healMove(x, y) {
    if (!this.tilesheet) return;
    let dirty = false;
    for (let dx=-1; dx<=1; dx++) {
      for (let dy=-1; dy<=1; dy++) {
        if (this.heal1(x + dx, y + dy)) dirty = true;
      }
    }
    if (dirty) this.broadcast({ id: "cellsDirty" });
  }
  
  heal1(x, y) {
    if ((x < 0) || (y < 0) || (x >= this.map.w) || (y >= this.map.h)) return;
    const cellp = y * this.map.w + x;
    const tileid = this.map.v[cellp];
    if (this.tilesheet.tables.weight[tileid] === 0xff) {
      // Appointment only. Don't change this tile.
      return;
    }
    const family = this.tilesheet.tables.family[tileid];
    if (!family) return;
    const neighbors = this.gatherNeighborMask(x, y, family);
    let candidates = []; // tileid
    let candidates_bc = 0; // Set bit count of all neighbor masks for tiles in (candidates).
    const popcnt8 = v => {
      let c = 0;
      for (; v; v>>=1) {
        if (v & 1) c++;
      }
      return c;
    };
    for (let i=0; i<256; i++) {
      // Candidate tiles must be in the same family, and must not require neighbors that don't exist, and must not be appointment-only.
      if (this.tilesheet.tables.family[i] !== family) continue;
      if (this.tilesheet.tables.neighbors[i] & ~neighbors) continue;
      if (this.tilesheet.tables.weight[i] === 0xff) continue;
      // Clear candidates if this neighbor mask has more bits set.
      const bc = popcnt8(this.tilesheet.tables.neighbors[i]);
      if (bc > candidates_bc) {
        candidates_bc = bc;
        candidates = [];
      } else if (bc < candidates_bc) {
        continue;
      }
      candidates.push(i);
    }
    if (candidates.length < 1) return; // oops
    if (candidates.length > 1) {
      // Reduce to 1 randomly, according to their weights.
      const weights = candidates.map(t => 256-this.tilesheet.tables.weight[t]);
      let range = 0;
      for (const w of weights) range += w;
      if (range > 0) {
        let choice = Math.random() * range;
        for (let i=0; i<candidates.length; i++) {
          choice -= weights[i];
          if (choice <= 0) {
            candidates = [candidates[i]];
            break;
          }
        }
      } else {
        candidates = [candidates[0]];
      }
    }
    if (this.map.v[cellp] === candidates[0]) return;
    this.map.v[cellp] = candidates[0];
    return true;
  }
  
  gatherNeighborMask(x, y, family) {
    let mask = 0;
    for (let dy=-1, bit=0x80; dy<=1; dy++) {
      for (let dx=-1; dx<=1; dx++) {
        if (!dy && !dx) continue; // continue without stepping (bit).
        const ox = x + dx;
        const oy = y + dy;
        if ((ox >= 0) && (oy >= 0) && (ox < this.map.w) && (oy < this.map.h)) {
          const otileid = this.map.v[oy * this.map.w + ox];
          if (this.tilesheet.tables.family[otileid] === family) {
            mask |= bit;
          }
        } else {
          // Debatable: I'm saying anything OOB does count as a neighbor.
          mask |= bit;
        }
        bit >>= 1;
      }
    }
    return mask;
  }
  
  /* poimove: Drag commands around.
   */
   
  poimoveBegin(x, y, fx, fy) {
    for (const poi of this.pois) {
      if (poi.x !== x) continue;
      if (poi.y !== y) continue;
      switch (poi.sub) {
        case 0: if ((fx >= 0.5) || (fy >= 0.5)) continue; break;
        case 1: if ((fx < 0.5) || (fy >= 0.5)) continue; break;
        case 2: if ((fx >= 0.5) || (fy < 0.5)) continue; break;
        case 3: if ((fx < 0.5) || (fy < 0.5)) continue; break;
      }
      // (poi) may belong to a remote map. At this point, that's fine.
      this.poimoveIndex = this.pois.indexOf(poi);
      this.runningTool = "poimove";
      return true;
    }
    return false;
  }
  
  poimoveMove(x, y) {
    const poi = this.pois[this.poimoveIndex];
    poi.x = x;
    poi.y = y;
    
    const subInUse = [false, false, false, false];
    for (const other of this.pois) {
      if (other === poi) continue;
      if (other.x !== poi.x) continue;
      if (other.y !== poi.y) continue;
      subInUse[other.sub] = true;
    }
    for (let i=0; i<4; i++) if (!subInUse[i]) { poi.sub = i; break; }
    
    const map = poi.remoteMapPath ? this.mapStore.getMap(poi.remoteMapPath) : this.map;
    const command = map.commands.find(c => c.id === poi.mapCommandId);
    if (!command) return;
    let index = poi.remoteMapPath ? 1 : 0; // When editing remotely, we want the second '@' arg.
    for (let i=0; i<command.args.length; i++) {
      const arg = command.args[i];
      if (arg.startsWith('@')) {
        if (index--) continue;
        command.args[i] = `@${x},${y}`;
        this.broadcast({ id: "cellsDirty" });
        break;
      }
    }
    if (poi.remoteMapPath) this.mapStore.dirty(poi.remoteMapPath, map);
  }
  
  /* poiedit: Open a modal for one POI command.
   */
   
  poieditBegin(x, y, fx, fy) {
    for (const poi of this.pois) {
      if (poi.x !== x) continue;
      if (poi.y !== y) continue;
      switch (poi.sub) {
        case 0: if ((fx >= 0.5) || (fy >= 0.5)) continue; break;
        case 1: if ((fx < 0.5) || (fy >= 0.5)) continue; break;
        case 2: if ((fx >= 0.5) || (fy < 0.5)) continue; break;
        case 3: if ((fx < 0.5) || (fy < 0.5)) continue; break;
      }
      const modal = this.dom.spawnModal(PoiModal);
      if (poi.remoteMapPath) modal.setupRemote(poi, poi.remoteMapPath, this.mapStore.getMap(poi.remoteMapPath));
      else modal.setup(poi);
      modal.result.then((result) => {
        if (result === null) return;
        this.regeneratePois();
        if (poi.remoteMapPath) this.mapStore.dirty(poi.remoteMapPath);
        else this.mapStore.dirty(this.res.path, this.map);
      }).catch(e => this.dom.modalError(e));
      return;
    }
    const modal = this.dom.spawnModal(PoiModal);
    modal.setupNew(x, y);
    modal.result.then((result) => {
      if (result === null) return;
      this.regeneratePois();
      this.mapStore.dirty(this.res.path, this.map);
    }).catch(e => this.dom.modalError(e));
  }
  
  /* poidelete: Delete one POI command.
   */
   
  poideleteBegin(x, y, fx, fy) {
    for (let i=this.pois.length; i-->0; ) {
      const poi = this.pois[i];
      if (poi.x !== x) continue;
      if (poi.y !== y) continue;
      if (poi.remoteMapPath) continue; // Could do this, but it's fishy. Delete it at its home map.
      switch (poi.sub) {
        case 0: if ((fx >= 0.5) || (fy >= 0.5)) continue; break;
        case 1: if ((fx < 0.5) || (fy >= 0.5)) continue; break;
        case 2: if ((fx >= 0.5) || (fy < 0.5)) continue; break;
        case 3: if ((fx < 0.5) || (fy < 0.5)) continue; break;
      }
      this.pois.splice(i, 1);
      const cmdp = this.map.commands.findIndex(c => c.id === poi.mapCommandId);
      if (cmdp >= 0) {
        this.map.commands.splice(cmdp, 1);
        this.mapStore.dirty(this.res.path, this.map);
      }
      this.broadcast({ id: "cellsDirty" });
      return;
    }
  }
  
  /* door: Navigate to another map by clicking on door commands.
   * Or prompt to create a door, if there isn't one here.
   */
  doorBegin(x, y, fx, fy) {
    for (let i=this.pois.length; i-->0; ) {
      const poi = this.pois[i];
      if (poi.x !== x) continue;
      if (poi.y !== y) continue;
      switch (poi.sub) {
        case 0: if ((fx >= 0.5) || (fy >= 0.5)) continue; break;
        case 1: if ((fx < 0.5) || (fy >= 0.5)) continue; break;
        case 2: if ((fx >= 0.5) || (fy < 0.5)) continue; break;
        case 3: if ((fx < 0.5) || (fy < 0.5)) continue; break;
      }
      const map = poi.remoteMapPath ? this.mapStore.getMap(poi.remoteMapPath) : this.map;
      const command = map?.commands.find(c => c.id === poi.mapCommandId);
      if (command?.kw !== "door") continue;
      if (poi.remoteMapPath) {
        this.jumpLocation = command.args[0].substring(1).split(',').map(v => +v);
        this.window.location = "#" + poi.remoteMapPath;
      } else {
        this.jumpLocation = command.args[2].substring(1).split(',').map(v => +v);
        const name = command.args[1] || "";
        const res = this.data.resByString(name, "map");
        if (!res) continue;
        this.window.location = "#" + res.path;
      }
      return;
    }
  }
}

MapPaint.singleton = true;

MapPaint.TOOLS = [
  { name: "pencil", control: "pickup", shift: "poimove", },
  { name: "rainbow", control: "pickup", shift: "poimove", },
  { name: "monalisa", control: "pickup", shift: "poimove", },
  { name: "pickup", shift: "poimove", },
  { name: "poimove", control: "poiedit", },
  { name: "poiedit", shift: "poimove", },
  { name: "poidelete", shift: "poimove", },
  { name: "heal", control: "pickup", shift: "poimove", },
  { name: "door", shift: "poimove", },
];

/* MACROS are conventional clumpings of neighbor tiles that I use out of habit.
 * If you draw the tilesheets in these patterns, there's a handy macro in TileModal to select one of these.
 */
MapPaint.MACROS = [{
  name: "fat5x3",
  w: 5,
  h: 3,
  neighbors: [
    0x0b,0x1f,0x16,0xfe,0xfb,
    0x6b,0xff,0xd6,0xdf,0x7f,
    0x68,0xf8,0xd0,0x00,0x00,
  ],
}, {
  name: "fat6x3",
  w: 6,
  h: 3,
  neighbors: [
    0x0b,0x1f,0x16,0xfe,0xfb,0x00,
    0x6b,0xff,0xd6,0xdf,0x7f,0x00,
    0x68,0xf8,0xd0,0xdb,0x7e,0x00,
  ],
}, {
  name: "rect3x3",
  w: 3,
  h: 3,
  neighbors: [
    0x0b,0x1f,0x16,
    0x6b,0xff,0xd6,
    0x68,0xf8,0xd0,
  ],
}, {
  name: "skinny4x4",
  w: 4,
  h: 4,
  neighbors: [
    0x0a,0x1a,0x12,0x02,
    0x4a,0x5a,0x52,0x42,
    0x48,0x58,0x50,0x40,
    0x08,0x18,0x10,0x00,
  ],
}];
