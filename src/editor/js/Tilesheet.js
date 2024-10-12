export class Tilesheet {
  constructor(src) {
    this.tables = {}; // [key]:Uint8Array(256)
    if (!src) {
    } else if (src instanceof Tilesheet) {
      for (const k of Object.keys(src.tables)) {
        this.tables[k] = new Uint8Array(src.tables[k]);
      }
    } else if (typeof(src) === "string") {
      this.decode(src);
    } else if ((src instanceof Uint8Array) || (src instanceof ArrayBuffer)) {
      this.decode(new TextDecoder("utf8").decode(src));
    } else {
      throw new Error(`Inappropriate input for Tilesheet`);
    }
    this.requireStandardTables();
  }
  
  encode() {
    let dst = "";
    for (const k of Object.keys(this.tables)) {
      const table = this.tables[k];
      if (this.tableIsEmpty(table)) continue;
      dst += k + "\n";
      for (let y=0, tp=0; y<16; y++) {
        for (let x=0; x<16; x++, tp++) {
          dst += "0123456789abcdef"[table[tp] >> 4];
          dst += "0123456789abcdef"[table[tp] & 15];
        }
        dst += "\n";
      }
      dst += "\n";
    }
    return new TextEncoder("utf8").encode(dst);
  }
  
  tableIsEmpty(table) {
    for (let i=0; i<256; i++) {
      if (table[i]) return 0;
    }
    return 1;
  }
  
  decode(src) {
    let table = null;
    let tp = 0;
    for (let srcp=0, lineno=1; srcp<src.length; lineno++) {
      let nlp = src.indexOf("\n", srcp);
      if (nlp < 0) nlp = src.length;
      const line = src.substring(srcp, nlp).trim();
      srcp = nlp + 1;
      if (!line) continue;
      
      if (table) {
        if (!line.match(/^[0-9a-fA-F]{32}$/)) {
          throw new Error(`Tilesheet:${lineno}: Expected 32 hex digits, found ${JSON.stringify(line)}`);
        }
        for (let linep=0; linep<line.length; linep+=2) {
          table[tp++] = parseInt(line.substring(linep, linep+2), 16);
        }
        if (tp >= 256) table = null;
      
      } else {
        if (!line.match(/^[a-z][a-zA-Z0-9_]{0,30}$/)) {
          throw new Error(`Tilesheet:${lineno}: Invalid table name ${JSON.stringify(line)}`);
        }
        table = new Uint8Array(256);
        this.tables[line] = table;
        tp = 0;
      }
    }
    if (table) throw new Error(`Tilesheet ends with partial table`);
  }
  
  requireStandardTables() {
    for (const k of ["physics", "overlay", "animate", "neighbors", "family", "weight"]) {
      if (this.tables[k]) continue;
      this.tables[k] = new Uint8Array(256);
    }
  }
}
