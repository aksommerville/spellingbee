/* MapRes.js
 * Live representation of a map resource.
 */
 
export class MapRes {
  constructor(src) {
    this.w = 0;
    this.h = 0;
    this.v = []; // Uint8Array or empty
    this.commands = []; // MapCommand
    if (!src) {
    } else if (src instanceof MapRes) {
      this.w = src.w;
      this.h = src.h;
      this.v = new Uint8Array(src.v);
      this.commands = src.commands.map(c => new MapCommand(c));
    } else if (typeof(src) === "string") {
      this.decode(src);
    } else if ((src instanceof Uint8Array) || (src instanceof ArrayBuffer)) {
      this.decode(new TextDecoder("utf8").decode(src));
    } else {
      throw new Error(`inappropriate input to MapRes ctor`);
    }
  }
  
  getImageName() {
    for (const command of this.commands) {
      if (command.kw === "image") return (command.args[0] || "").replace("image:", "");
    }
    return "";
  }
  
  encode() {
    let dst = "";
    for (let y=0, vp=0; y<this.h; y++) {
      for (let x=0; x<this.w; x++, vp++) {
        dst += "0123456789abcdef"[this.v[vp] >> 4];
        dst += "0123456789abcdef"[this.v[vp] & 15];
      }
      dst += "\n";
    }
    dst += "\n";
    for (const command of this.commands) {
      dst += command.kw;
      for (const arg of command.args) dst += " " + arg;
      dst += "\n";
    }
    return new TextEncoder("utf8").encode(dst);
  }
  
  decode(src) {
    let stage = 0; // (0,1,2) = (init,image,commands)
    let colc=0, rowc=0;
    const precells = [];
    this.commands = [];
    for (let srcp=0, lineno=1; srcp<src.length; lineno++) {
      try {
        let nlp = src.indexOf("\n", srcp);
        if (nlp < 0) nlp = src.length;
        const line = src.substring(srcp, nlp).trim();
        srcp = nlp + 1;
        switch (stage) {
      
          case 0: { // Awaiting image.
              if (!line) continue;
              stage = 1;
              colc = line.length >> 1;
            } // pass
          
          case 1: { // Reading image.
              if (!line) {
                stage = 2;
                continue;
              }
              if (line.length !== colc << 1) throw new Error(`expected ${colc<<1} bytes for map image line, found ${line.length}`);
              for (let i=0; i<line.length; i+=2) {
                const hi = parseInt(line[i], 16);
                const lo = parseInt(line[i+1], 16);
                if (isNaN(hi) || isNaN(lo)) throw new Error(`expected hex byte, found '${line.substring(i, i+2)}'`);
                precells.push((hi << 4) | lo);
              }
              rowc++;
            } break;
          
          case 2: { // Reading commands.
              if (!line) continue;
              const command = new MapCommand(line);
              this.commands.push(command);
            } break;
          
        }
      } catch (e) {
        e.message = lineno + ": " + e.message;
        throw e;
      }
    }
    if ((colc < 1) || (rowc < 1) || (colc > 0xff) | (rowc > 0xff)) {
      throw new Error(`Map dimensions must be in 1x1..255x255, found ${colc}x${rowc}`);
    }
    this.v = new Uint8Array(precells);
    this.w = colc;
    this.h = rowc;
  }
}

export class MapCommand {
  constructor(src) {
    if (!MapCommand.nextCommandId) MapCommand.nextCommandId = 1;
    this.kw = "";
    this.args = []; // string
    this.id = MapCommand.nextCommandId++;
    if (!src) {
    } else if (src instanceof MapCommand) {
      this.kw = src.kw;
      this.args = src.args.map(s => s);
      this.id = src.id;
    } else if (typeof(src) === "string") {
      this.decode(src);
    } else {
      throw new Error(`inappropriate input to MapCommand ctor`);
    }
  }
  
  decode(src) {
    const words = src.split(/\s+/g);
    if (words.length < 1) throw new Error(`Invalid map command: ${JSON.stringify(src)}`);
    this.kw = words[0];
    this.args = words.slice(1);
  }
  
  encode() {
    return this.kw + this.args.map(v => " "+v).join("");
  }
}
