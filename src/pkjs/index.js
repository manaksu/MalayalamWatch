/* Keralam — PebbleKit JS
 * Keys: BATTERY_STYLE=0, BG_STYLE=1, BOLD_STYLE=2, CORNER_ROT=3,
 *       DATE_BOLD=4, GRID_BOLD=5, GRID_SHADE=6, HAND_STYLE=7
 */

function load() {
  return {
    bs:  +(localStorage.getItem('bs')  || '0'),
    bg:  +(localStorage.getItem('bg')  || '0'),
    bld: +(localStorage.getItem('bld') || '0'),
    cr:  +(localStorage.getItem('cr')  || '1'),
    db:  +(localStorage.getItem('db')  || '0'),
    gb:  +(localStorage.getItem('gb')  || '0'),
    gs:  +(localStorage.getItem('gs')  || '1'),
    hs:  +(localStorage.getItem('hs')  || '0')
  };
}

function save(c) {
  ['bs','bg','bld','cr','db','gb','gs','hs'].forEach(function(k){ localStorage.setItem(k,c[k]); });
}

function send(c) {
  var m={};
  m[0]=c.bs; m[1]=c.bg; m[2]=c.bld; m[3]=c.cr;
  m[4]=c.db; m[5]=c.gb; m[6]=c.gs;  m[7]=c.hs;
  Pebble.sendAppMessage(m, function(){console.log('ok');}, function(e){console.log('fail',e);});
}

function opt(arr, cur) {
  return arr.map(function(l,i){
    return '<option value='+i+(i===cur?' selected':'')+'>'+l+'</option>';
  }).join('');
}

function buildURL(c) {
  var h='<html><head><meta name=viewport content="width=device-width,initial-scale=1">'
    +'<style>'
    +'*{box-sizing:border-box;margin:0;padding:0}'
    +'body{font:13px sans-serif;background:#f0ece0;color:#1a1610;padding:10px}'
    +'h2{font-size:15px;margin-bottom:10px}'
    +'p{font-size:11px;color:#888;margin:8px 0 2px}'
    +'select{width:100%;padding:6px;font-size:13px;border:1px solid #ccc;border-radius:4px;background:#fff;margin-bottom:2px}'
    +'button{width:100%;padding:11px;font-size:14px;background:#1a1610;color:#f0ece0;border:none;border-radius:4px;margin-top:10px}'
    +'</style></head><body>'
    +'<h2>Keralam</h2>'
    +'<p>Background</p><select id=bg>'+opt(['Cream','White','Light Grey'],c.bg)+'</select>'
    +'<p>Battery</p><select id=bs>'+opt(['Circular Spiral','Flower','Square Spiral','Bar'],c.bs)+'</select>'
    +'<p>Hands</p><select id=hs>'+opt(['Smooth','Pixel Blocky','Pixel Tapered','Pixel Literary'],c.hs)+'</select>'
    +'<p>Clock Numerals</p><select id=bld>'+opt(['Regular','Bold'],c.bld)+'</select>'
    +'<p>Corner Numerals</p><select id=cr>'+opt(['Flat','Rotated'],c.cr)+'</select>'
    +'<p>Date Style</p><select id=db>'+opt(['Regular','Bold'],c.db)+'</select>'
    +'<p>Legend Weight</p><select id=gb>'+opt(['Regular','Bold'],c.gb)+'</select>'
    +'<p>Legend Shade</p><select id=gs>'+opt(['Light','Medium','Strong'],c.gs)+'</select>'
    +'<button id=sv>Save</button>'
    +'<script>document.getElementById("sv").onclick=function(){'
    +'function g(id){return +document.getElementById(id).value;}'
    +'var r={bs:g("bs"),bg:g("bg"),bld:g("bld"),cr:g("cr"),db:g("db"),gb:g("gb"),gs:g("gs"),hs:g("hs")};'
    +'location.href="pebblejs://close#"+encodeURIComponent(JSON.stringify(r));'
    +'}<\/script></body></html>';
  return 'data:text/html,'+encodeURIComponent(h);
}

Pebble.addEventListener('ready', function(){ console.log('Keralam ready'); });
Pebble.addEventListener('showConfiguration', function(){ Pebble.openURL(buildURL(load())); });
Pebble.addEventListener('webviewclosed', function(e) {
  if (!e||!e.response||e.response===''||e.response==='CANCELLED') return;
  var raw=e.response;
  if (raw.indexOf('#')!==-1) raw=raw.substring(raw.lastIndexOf('#')+1);
  var c; try{c=JSON.parse(decodeURIComponent(raw));}catch(err){return;}
  save(c); send(c);
});
