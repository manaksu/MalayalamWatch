/* Keralam — PebbleKit JS
 * Keys (alphabetical order matching appinfo.json):
 *   BATTERY_STYLE=0, BATTERY_POS=1, HAND_STYLE=2,
 *   BOLD_STYLE=3,    BG_STYLE=4,    CORNER_ROT=5
 */

function load() {
  return {
    bs:  +(localStorage.getItem('bs')  || '0'),
    bp:  +(localStorage.getItem('bp')  || '0'),
    hs:  +(localStorage.getItem('hs')  || '0'),
    bld: +(localStorage.getItem('bld') || '0'),
    bg:  +(localStorage.getItem('bg')  || '0'),
    cr:  +(localStorage.getItem('cr')  || '1')
  };
}

function save(c) {
  localStorage.setItem('bs',  c.bs);
  localStorage.setItem('bp',  c.bp);
  localStorage.setItem('hs',  c.hs);
  localStorage.setItem('bld', c.bld);
  localStorage.setItem('bg',  c.bg);
  localStorage.setItem('cr',  c.cr);
}

function send(c) {
  var m = {};
  m[0] = c.bp;   /* BATTERY_POS   */
  m[1] = c.bs;   /* BATTERY_STYLE */
  m[2] = c.bg;   /* BG_STYLE      */
  m[3] = c.bld;  /* BOLD_STYLE    */
  m[4] = c.cr;   /* CORNER_ROT    */
  m[5] = c.hs;   /* HAND_STYLE    */
  Pebble.sendAppMessage(m,
    function() { console.log('Keralam settings sent ok'); },
    function(e) { console.log('Keralam settings fail: ' + JSON.stringify(e)); }
  );
}

function sel(arr, cur) {
  return arr.map(function(l, i) {
    return '<option value=' + i + (i === cur ? ' selected' : '') + '>' + l + '</option>';
  }).join('');
}

function page(c) {
  var s = '<style>'
    + 'body{font-family:sans-serif;background:#f0ece0;color:#1a1610;margin:0;padding:16px}'
    + 'h2{font-size:16px;margin:0 0 14px}'
    + 'label{display:block;font-size:12px;color:#666;margin-bottom:3px}'
    + 'select{width:100%;padding:8px;font-size:14px;border:1px solid #ccc;border-radius:5px;background:#fff;margin-bottom:14px}'
    + 'button{width:100%;padding:12px;font-size:15px;background:#1a1610;color:#f0ece0;border:none;border-radius:5px}'
    + '</style>';

  var b = '<h2>Keralam Settings</h2>'
    + '<label>Background</label><select id="bg">'
    + sel(['Cream','White','Light Grey'], c.bg) + '</select>'
    + '<label>Battery Style</label><select id="bs">'
    + sel(['Circular Spiral','Flower Radial','Square Spiral','Battery Bar'], c.bs) + '</select>'
    + '<label>Battery Position</label><select id="bp">'
    + sel(['On Watch Face','Bottom Left'], c.bp) + '</select>'
    + '<label>Hand Style</label><select id="hs">'
    + sel(['Smooth','Pixel Blocky','Pixel Tapered','Pixel Literary'], c.hs) + '</select>'
    + '<label>Numerals</label><select id="bld">'
    + sel(['Regular','Bold'], c.bld) + '</select>'
    + '<label>Corner Numerals</label><select id="cr">'
    + sel(['Flat','Rotated'], c.cr) + '</select>'
    + '<button id="sv">Save</button>';

  var js = '<script>'
    + 'document.getElementById("sv").onclick=function(){'
    + 'function g(id){return +document.getElementById(id).value;}'
    + 'var r={bs:g("bs"),bp:g("bp"),hs:g("hs"),bld:g("bld"),bg:g("bg"),cr:g("cr")};'
    + 'location.href="pebblejs://close#"+encodeURIComponent(JSON.stringify(r));'
    + '};'
    + '<\/script>';

  return 'data:text/html,' + encodeURIComponent(
    '<html><head><meta name=viewport content="width=device-width,initial-scale=1">'
    + s + '</head><body>' + b + js + '</body></html>'
  );
}

Pebble.addEventListener('ready', function() {
  console.log('Keralam ready');
});

Pebble.addEventListener('showConfiguration', function() {
  Pebble.openURL(page(load()));
});

Pebble.addEventListener('webviewclosed', function(e) {
  if (!e || !e.response || e.response === '' || e.response === 'CANCELLED') return;
  var raw = e.response;
  if (raw.indexOf('#') !== -1) raw = raw.substring(raw.lastIndexOf('#') + 1);
  var c;
  try { c = JSON.parse(decodeURIComponent(raw)); } catch(err) { return; }
  save(c);
  send(c);
});
