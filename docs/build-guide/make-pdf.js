// Prints build-guide.html to ../build-guide.pdf: node docs/build-guide/make-pdf.js (needs Playwright)
const { chromium } = require('playwright');
const path = require('path');
(async () => {
  const html = path.join(__dirname, 'build-guide.html');
  const out = path.join(__dirname, '..', 'build-guide.pdf');
  const browser = await chromium.launch();
  const page = await browser.newPage();
  await page.goto('file://' + html, { waitUntil: 'load' });
  await page.pdf({
    path: out, format: 'A4', printBackground: true, preferCSSPageSize: true,
    displayHeaderFooter: true,
    headerTemplate: '<div></div>',
    footerTemplate: '<div style="font-family: Liberation Sans, Arial, sans-serif; font-size:7.5pt; color:#5d6875; width:100%; padding:0 16mm; display:flex; justify-content:space-between"><span>pinewood-time build guide</span><span><span class="pageNumber"></span> / <span class="totalPages"></span></span></div>',
  });
  await browser.close();
  console.log(out);
})();
