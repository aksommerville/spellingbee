/* textReview.js
 * Scans strings resources and string literals in C files, and reports anything that isn't in the dictionary.
 */

const fs = require("fs");

const SRCROOT = "src";
const DICTPATH = SRCROOT + "/data/dict/1-nwl2023";
const STRINGSROOT = SRCROOT + "/data/strings";
const CROOT = SRCROOT + "/game"; // We won't check eg "tool" or "editor".

const dict = fs.readFileSync(DICTPATH).toString("ascii").split("\n").filter(v => v);
console.log(`${DICTPATH}: Acquired dictionary, ${dict.length} words`);

let checkc = 0;
let filec = 0;

function isWord(word) {
  checkc++;
  
  // Anything false or of invalid length, illogically, we call it true, ie let it pass.
  if (!word || (word.length < 2) || (word.length > 7)) return true;
  
  word = word.toUpperCase();
  
  // Our lexer splits on apostrophes. So let a few known contractions through on faith.
  if (word === "LL") return true;
  if (word === "VE") return true;
  if (word === "RE") return true; // well.. this one actually is a word
  if (word === "HASN") return true;
  if (word === "DOESN") return true;
  
  // Likewise, roman numerals are fine. Only comes up once.
  if (word === "III") return true;
  
  // Everything else, search the dictionary. It's sorted length then alphabetical.
  let lo=0, hi=dict.length;
  while (lo < hi) {
    const ck = (lo + hi) >> 1;
    const q = dict[ck];
    if (word.length < q.length) hi = ck;
    else if (word.length > q.length) lo = ck + 1;
    else if (word < q) hi = ck;
    else if (word > q) lo = ck + 1;
    else return true;
  }
  return false;
}

function lineno(src, p) {
  let c = 0;
  for (let srcp=0; srcp<p; ) {
    const nlp = src.indexOf("\n", srcp);
    if (nlp < 0) break;
    if (nlp >= p) break;
    c++;
    srcp = nlp + 1;
  }
  return 1 + c;
}

function checkStringsFile(src, path) {
  filec++;
  // No need to overthink this. In theory, the JSON syntax lets files obscure words from us, but whatever.
  const re = new RegExp("[a-zA-Z]+", "g");
  let match;
  while (match = re.exec(src)) {
    if (!isWord(match[0])) {
      console.log(`!!! ${path}:${lineno(src, match.index)}: ${JSON.stringify(match[0])}`);
    }
  }
}

// Provide content of string literal, without fences. Returns offending word or null.
function checkCString(src) {
  const re = new RegExp("[a-zA-Z]+", "g");
  let match;
  while (match = re.exec(src)) {
    if (!isWord(match[0])) return match[0];
  }
  return null;
}

// Returns index of closing quote, or -1.
function measureString(src, openp) {
  for (let srcp=openp+1; srcp<src.length; ) {
    const closep = src.indexOf('"', srcp);
    if (closep < 0) return -1;
    if (src[closep-1] !== '\\') return closep;
    srcp = closep + 1;
  }
  return -1;
}

function checkCFile(src, path) {
  filec++;
  // We can't cheese it like with the strings files, because we specifically only want string literals.
  // But we can still cheese pretty hard. Assume that there are no stray quotes.
  for (let srcp=0; srcp<src.length; ) {
    const qp = src.indexOf('"', srcp);
    if (qp < 0) break;
    const closep = measureString(src, qp);
    if (closep < qp) {
      console.log(`!!! ${path}:${lineno(src, qp)}: Parser choke`);
      return;
    }
    const len = closep - qp - 1;
    if (len > 150) {
      console.log(`!!! ${path}:${lineno(src, qp)}: Improbable string length ${len}. Assuming tokenizer is broken.`);
      return;
    }
    const word = checkCString(src.substring(qp + 1, closep));
    if (word) {
      console.log(`!!! ${path}:${lineno(src, qp)}: ${JSON.stringify(word)}`);
    }
    srcp = closep + 1;
  }
}

function checkStringsInTree(root) {
  console.log(`checkStringsInTree ${JSON.stringify(root)}...`);
  for (const base of fs.readdirSync(root)) {
    const path = root + "/" + base;
    const st = fs.statSync(path);
    if (st.isDirectory()) checkStringsInTree(path);
    else if (st.isFile()) checkStringsFile(fs.readFileSync(path).toString("utf8"), path);
  }
}

function checkCInTree(root) {
  console.log(`checkCInTree ${JSON.stringify(root)}...`);
  for (const base of fs.readdirSync(root)) {
    const path = root + "/" + base;
    const st = fs.statSync(path);
    if (st.isDirectory()) checkCInTree(path);
    else if (base.endsWith(".c") && st.isFile()) checkCFile(fs.readFileSync(path).toString("utf8"), path);
  }
}

/* C files will have tons of false positives, and you need to sift thru those manually.
 */
checkStringsInTree(STRINGSROOT);
checkCInTree(CROOT);

console.log(`Checked ${checkc} words in ${filec} files.`);
