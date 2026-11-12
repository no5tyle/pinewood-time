// Renders an SVG (e.g. from kicad-cli pcb export svg) to a PNG on a background: node svg2png.js in.svg out.png [width] [background]
const { chromium } = require('playwright');
const fs = require('fs');
const path = require('path');
(async () => {
  const [src, out, width = '1600', bg = '#ffffff'] = process.argv.slice(2);
  const svg = fs.readFileSync(src, 'utf8');
  const b = await chromium.launch();
  const p = await b.newPage({ viewport: { width: +width, height: 100 }, deviceScaleFactor: 1 });
  await p.setContent(`<!doctype html><html><body style="margin:0;background:${bg}">
    <img id="i" style="display:block;width:${width}px" src="data:image/svg+xml;base64,${Buffer.from(svg).toString('base64')}"></body></html>`);
  await p.waitForFunction(() => document.getElementById('i').complete);
  const h = await p.evaluate(() => document.getElementById('i').getBoundingClientRect().height);
  await p.setViewportSize({ width: +width, height: Math.ceil(h) });
  await p.screenshot({ path: out });
  await b.close();
  console.log(path.basename(out), width + 'x' + Math.ceil(h));
})();
