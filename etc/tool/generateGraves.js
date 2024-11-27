/* The cemetery has 133 graves which need names and dates assigned.
 * Obviously we're not going to do that individually.
 * It's an interesting problem because the assignment needs to be randomish but also strictly needs to follow two ordering rules:
 *  - Surnames are alphabetical horizontally.
 *  - Dates of Death are reverse chronological vertically.
 * And it's important that DOD ties break randomly, there must not be any pattern to that tie-breaking.
 * Output will be ordered LRTB, so higher index in general is easier to reach. I might use that in selecting the treasure graves.
 *
 * Run this script against src/data/map/4-cemetery to generate a conformant list of grave commands to paste back in.
 */

const fs = require("fs");

if (process.argv.length !== 3) {
  throw new Error(`Usage: node ${process.argv[1]} MAP`);
}
const srcpath = process.argv[2];
const src = fs.readFileSync(srcpath).toString("ascii");

const DOD_MIN = 1200;
const DOD_MAX = 1924; // Usually I say Dot lives in the 13th century, but those numbers would be weirdly low. Don't go too recent, for risk of naming someone a player knows.
const AGE_MIN = 20;
const AGE_MAX = 90;
const FIRST_NAMES = [ // Order not important, but I'm going alphabetical and alternating genders to force broad coverage.
  "Albert", "Alice",
  "Bob", "Betty",
  "Charles", "Clarissa",
  "Donald", "Debbie",
  "Ed", "Ellen",
  "Frank", "Felicia",
  "George", "Gabrielle",
  "Henry", "Helen",
  "Isaac", "Isabelle",
  "Jack", "Jill",
  "Kevin", "Karen",
  "Larry", "Lucy",
  "Melvin", "Mary",
  "Norbert", "Nancy",
  "Oliver", "Olivia",
  "Peter", "Patsy",
  "Richard", "Rebecca",
  "Steve", "Susie",
  "Tom", "Tabitha",
  "Victor", "Veronica",
  "Walter", "Winnie",
];
const LAST_NAMES = [ // Must be sorted alphabetically. Not necessary to use the full alphabet. Ideal would be a unique name for each grave, but that's too many.
  "Albert",
  "Anderson",
  "Bannister",
  "Billings",
  "Calais",
  "Chapman",
  "Curt",
  "Davis",
  "Dunkle",
  "East",
  "Farrin",
  "Funkenstein",
  "Gable",
  "Garris",
  "Hammer",
  "Henderson",
  "Itzbah",
  "Jackson",
  "Katz",
  "Kemple",
  "Laramie",
  "Lender",
  "Lundgren",
  "Massey",
  "Morley",
  "Norris",
  "Nutter",
  "O'Malley",
  "O'Reilly",
  "Pendergast",
  "Pocking",
  "Quigley",
  "Quentin",
  "Robinson",
  "Rutherford",
  "Spelling",
  "Starr",
  "Tulley",
  "Ullman",
  "Underhill",
  "Vace",
  "Valley",
  "Wall",
  "Waters",
  "Yarrow",
  "Yeunger",
  "Zachary",
  "Zivic",
];

// Tombstone tiles are in a 2x4 rectangle 0x06..0x37.
const tombstones = []; // {x,y,text}
let mapw=0, maph=0;
let xlo=999, xhi=0; // Collect these as we go. The y limits, we can trivially read after, since tombstones will naturally sort LRTB.
for (let srcp=0; srcp<src.length; ) {
  let nlp = src.indexOf("\n", srcp);
  if (nlp < 0) nlp = src.length;
  const line = src.substring(srcp, nlp).trim();
  srcp = nlp + 1;
  if (!line) break; // First empty line ends the map image, and we don't care about commands.
  maph++;
  if ((line.length & 1) || !line.match(/^[0-9a-fA-F]*$/)) {
    throw new Error(`${srcpath}:${maph}: Malformed image line.`);
  }
  if (!mapw) {
    mapw = line.length >> 1;
  } else if (mapw !== line.length >> 1) {
    throw new Error(`${srcpath}:${maph}: Expected ${mapw << 1} hex digits but found ${line.length}`);
  }
  for (let linep=0,x=0; linep<line.length; linep+=2,x++) {
    const tileid = parseInt(line.substring(linep, linep+2), 16);
    const col = tileid & 15;
    if ((col < 6) || (col > 7)) continue;
    const row = tileid >> 4;
    if (row > 3) continue;
    tombstones.push({ x, y: maph-1, text: "" });
    if (x < xlo) xlo = x;
    if (x > xhi) xhi = x;
  }
}
if (tombstones.length < 1) throw new Error(`${srcpath}: No tombstones`);
const ylo = tombstones[0].y;
const yhi = tombstones[tombstones.length - 1].y;

/* My first thought was use normalized (x,y) to directly index surname and dod, but that leads to clumping: Very few graves are alone on their column or row.
 * So to distribute more uniformly, sort the graves by x and y, and use the index from that sort.
 * Also mind that we need to reverse the Y sort: The building is south of the cemetery, and older graves should be nearer the building.
 */
const xgraves=[...tombstones], ygraves=[...tombstones];
xgraves.sort((a, b) => a.x - b.x);
ygraves.sort((a, b) => b.y - a.y);
for (let i=0; i<xgraves.length; i++) {
  const grave = xgraves[i];
  grave.lastName = LAST_NAMES[Math.floor((i * LAST_NAMES.length) / xgraves.length)];
}
for (let i=0; i<ygraves.length; i++) {
  const grave = ygraves[i];
  grave.dod = Math.floor(DOD_MIN + (i * (DOD_MAX - DOD_MIN)) / ygraves.length);
}

/* Our strictly mathematical approach to DOD has caused them to be ordered horizontally as well as vertically.
 * We can't allow that! The whole idea is you need both pieces of information: name and dod.
 * So bucket ygraves by identical y and randomize dod within each bucket, preserving the limits.
 * The same reasoning applies to lastName, but in that case it actually makes narrative sense to have collisions near other, families tend to get buried together.
 */
for (let i=0; i<ygraves.length; ) {
  const y = ygraves[i].y;
  let c = 1;
  while ((i + c < ygraves.length) && (ygraves[i + c].y === y)) c++;
  if (c > 1) {
    const bucket = ygraves.slice(i, i + c);
    const hi = bucket[0].dod; // They're sorted descending. Um. But actually the math doesn't care.
    const lo = bucket[bucket.length - 1].dod;
    const range = hi - lo;
    for (const grave of bucket) {
      grave.dod = Math.floor(lo + Math.random() * range);
    }
  }
  i += c;
}

/* First name and age are purely random.
 */
for (const grave of tombstones) {
  grave.firstName = FIRST_NAMES[Math.floor(Math.random() * FIRST_NAMES.length)];
  const age = AGE_MIN + Math.random() * (AGE_MAX - AGE_MIN);
  grave.dob = Math.floor(grave.dod - age);
}

/* OK, dump them.
 */
for (const grave of tombstones) {
  console.log(`grave ${grave.x} ${grave.y} ${JSON.stringify(`${grave.firstName} ${grave.lastName}\n${grave.dob} - ${grave.dod}`)}`);
}

