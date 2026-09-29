// Verify the generated page's render logic without a browser:
// stub a minimal DOM, execute the page script, then assert the computed
// positions/classes and exercise the click + scroll handlers.
//
// Self-contained: the page's <script> is pulled straight out of the generated
// HTML, so this never depends on a cleaned-up intermediate file. Tile plates
// live inside an innerHTML string the stub does not parse, hence the regexes.
const fs = require("fs");

const html = fs.readFileSync("docs/stage-select/index.html", "utf8");
const m = html.match(/<script>([\s\S]*)<\/script>\s*<\/body>/);
if (!m) throw new Error("could not locate the page <script> block");
const src = m[1];

const made = {};
function el(tag) {
  return {
    tagName: tag, style: {}, className: "", textContent: "", _html: "",
    children: [], onclick: null, src: "", id: "", _h: {},
    appendChild(c) { this.children.push(c); },
    addEventListener(t, fn) { this._h[t] = fn; },
    set innerHTML(v) { this._html = v; this.children = []; },
    get innerHTML() { return this._html; },
  };
}
global.addEventListener = () => {};
global.innerWidth = 1600;
global.innerHeight = 900;
global.document = {
  getElementById(id) { if (!made[id]) { made[id] = el("div"); made[id].id = id; } return made[id]; },
  createElement(tag) { return el(tag); },
};

eval(src);

const num = (v) => parseFloat(v);
const failures = [];
const check = (ok, label) => { if (!ok) failures.push(label); };

const PITCH = 222, TILE_W = 166, GAP = 56, JOG = 80, RAIL_W = 585, MAX_SC = 469;
const tileX = (i) => i * PITCH;
const tileY = (i) => (i % 2 === 0 ? 10 : 90);
const midY = (i) => tileY(i) + 41;
const nodeX = (i) => (i === 0 ? TILE_W + GAP / 2 : i * PITCH - GAP / 2);

const tiles = () => made["rail-inner"].children.filter((n) => n.className.includes("tile"));
const scNow = () => -parseFloat(made["rail-inner"].style.transform.match(/-?[\d.]+/)[0]);

function selVisible(label) {
  const i = tiles().findIndex((n) => n.className.includes("sel"));
  const s = scNow();
  const inView = tileX(i) >= s - 1 && tileX(i) + TILE_W <= s + RAIL_W + 1;
  check(i >= 0 && inView, `${label}: selected stage ${i} not in view (scroll=${s})`);
}

function dump(label) {
  const inner = made["rail-inner"];
  const nodes = tiles();
  const segs = inner.children.filter((n) => n.className.includes("seg"));
  const marks = inner.children.filter((n) => n.tagName === "img" && !n.className.includes("plate"));

  console.log(`\n--- ${label}`);
  console.log(`    rail-inner width=${inner.style.width} transform=${inner.style.transform}`
    + `  thumb left=${num(made["sbar-thumb"].style.left).toFixed(1)} width=${made["sbar-thumb"].style.width}`);
  check(inner.style.width === "1054px", `${label}: content width`);

  nodes.forEach((n, i) => {
    const code = (n._html.match(/class="abs code">([^<]*)</) || [])[1];
    check(num(n.style.left) === tileX(i), `${label}: tile ${i} left ${n.style.left}`);
    check(num(n.style.top) === tileY(i), `${label}: tile ${i} top ${n.style.top}`);

    const hs = segs.filter((s) => num(s.style.height) === 4 && num(s.style.width) === GAP / 2);
    const vs = segs.filter((s) => num(s.style.width) === 4 && num(s.style.height) === JOG);
    if (i < 4) {
      check(hs.some((s) => num(s.style.left) === tileX(i) + TILE_W && num(s.style.top) === midY(i) - 2),
        `${label}: tile ${i} out-stub`);
      check(hs.some((s) => num(s.style.left) === tileX(i) + TILE_W + GAP / 2 && num(s.style.top) === midY(i + 1) - 2),
        `${label}: tile ${i} in-stub`);
      check(vs.some((s) => num(s.style.left) === tileX(i) + TILE_W + GAP / 2 - 2), `${label}: tile ${i} riser`);
    }
    const mark = marks.find((k) => num(k.style.left) === nodeX(i) - num(k.style.width) / 2
                               && num(k.style.top) === midY(i) - num(k.style.width) / 2);
    check(!!mark, `${label}: tile ${i} node marker`);

    console.log(`    tile ${i}  left=${String(n.style.left).padStart(5)} top=${String(n.style.top).padStart(4)}`
      + `  ${n.className.replace("abs ", "").padEnd(24)} code=${code}`
      + `  node@${nodeX(i)},${midY(i)} ${mark ? mark.style.width : "MISSING"}`);
  });
  console.log(`    panel   code=${made["p-code"].textContent} state=${made["p-state"].textContent} btn="${made.start.className}"`);
  console.log(`    tabs    ${made.tabs.children.map((t) => (t.className.includes("on") ? "[" : "") + (t._html.match(/t-num">([^<]*)</) || [])[1] + (t.className.includes("on") ? "]" : "")).join(" ")}`);
}

dump("initial (第一章, stage index 3)");
selVisible("initial");

made["tabs"].children[2].onclick();
dump("after clicking 第二章 tab");
selVisible("after 第二章");

tiles()[4].onclick();
const lockedOk = made.start.className.includes("off") && made["p-state"].textContent === "未解锁";
dump("after clicking locked stage 5");
check(lockedOk, "locked stage should show 未解锁 with the button disabled");
check(scNow() === MAX_SC, `selecting the last stage should scroll to the end (scroll=${scNow()})`);

const rail = made["rail"];
check(typeof rail._h.wheel === "function", "rail wheel handler registered");

rail._h.wheel({ deltaY: -99999 });
check(scNow() === 0, `wheel clamps at 0 (scroll=${scNow()})`);
rail._h.wheel({ deltaY: 300 });
check(scNow() === 300, `wheel +300 -> ${scNow()}`);
rail._h.wheel({ deltaY: 99999 });
check(scNow() === MAX_SC, `wheel clamps at max (scroll=${scNow()})`);
check(made["sbar-thumb"].style.left === "260px", `thumb at max -> ${made["sbar-thumb"].style.left}`);
check(made["sbar-thumb"].style.width === "325px", `thumb width -> ${made["sbar-thumb"].style.width}`);

made["tabs"].children[0].onclick();
dump("after clicking 序章 tab");
selVisible("after 序章");

const uris = [];
for (const n of made["rail-inner"].children) {
  if (n._html) uris.push(...[...n._html.matchAll(/src="data:image\/png;base64,([^"]*)"/g)].map((x) => x[1]));
  if (n.style.backgroundImage) uris.push((n.style.backgroundImage.match(/base64,([^)]*)\)/) || [])[1] || "");
}
uris.push((made.sbar.style.backgroundImage.match(/base64,([^)]*)\)/) || [])[1] || "");
console.log(`\ndata-uris seen=${uris.length}  malformed=${uris.filter((s) => !s || s.length < 64).length}`);
console.log("distinct byte-lengths:", [...new Set(uris.map((s) => s.length))].sort((a, b) => a - b).join(", "));
console.log(failures.length ? `\n${failures.length} CHECK(S) FAILED:\n  ` + failures.join("\n  ")
                            : "\nall geometry checks passed");
