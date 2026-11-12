// Stacks black-and-white layer SVGs into one coloured picture: node render.js out.png a.svg=#hex b.svg=#hex ...
// Each layer is a CSS mask over a block of its colour, so whatever the layer draws shows in that colour.
const { chromium } = require('playwright');
const fs = require('fs');
const path = require('path');
(async () => {
  const [out, ...layers] = process.argv.slice(2);
  const WIDTH = 1800, PAD = 40;
  const first = fs.readFileSync(layers[0].split('=')[0], 'utf8');
  const vb = first.match(/viewBox="([^"]+)"/)[1].split(/\s+/).map(Number);
  const height = Math.round((WIDTH * vb[3]) / vb[2]);
  const divs = layers.map((l) => {
    const [file, colour] = l.split('=');
    const url = `data:image/svg+xml;base64,${fs.readFileSync(file).toString('base64')}`;
    return `<div style="position:absolute;inset:0;background:${colour};-webkit-mask:url(${url}) 0 0/100% 100% no-repeat;mask:url(${url}) 0 0/100% 100% no-repeat"></div>`;
  });
  const b = await chromium.launch();
  const p = await b.newPage({ viewport: { width: WIDTH + 2 * PAD, height: height + 2 * PAD }, deviceScaleFactor: 1 });
  await p.setContent(`<!doctype html><body style="margin:0;background:#ffffff">
    <div style="position:absolute;left:${PAD}px;top:${PAD}px;width:${WIDTH}px;height:${height}px;filter:drop-shadow(0 6px 14px rgba(0,0,0,.25))">${divs.join('')}</div></body>`);
  await p.waitForTimeout(300);
  await p.screenshot({ path: out });
  await b.close();
  console.log(path.basename(out));
})();
