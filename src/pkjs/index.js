Pebble.addEventListener('ready', function() {
  console.log('Keralam JS ready');
});

Pebble.addEventListener('showConfiguration', function() {
  var batt = localStorage.getItem('battery_style') || '0';
  var bpos = localStorage.getItem('battery_pos')   || '0';
  var hand = localStorage.getItem('hand_style')    || '0';
  var bold = localStorage.getItem('bold_style')    || '0';
  var bg   = localStorage.getItem('bg_style')      || '0';
  var crot = localStorage.getItem('corner_rot')    || '1';

  var html = '<!DOCTYPE html><html><head>'
    + '<meta name="viewport" content="width=device-width,initial-scale=1">'
    + '<style>'
    + 'body{font-family:sans-serif;background:#f0ece0;color:#1a1610;margin:0;padding:20px;}'
    + 'h2{font-size:18px;margin:0 0 20px;}'
    + 'label{display:block;font-size:14px;margin-bottom:6px;color:#555;}'
    + 'select{width:100%;padding:10px;font-size:15px;border:1px solid #ccc;'
    + 'border-radius:6px;background:#fff;margin-bottom:20px;}'
    + 'button{width:100%;padding:14px;font-size:16px;background:#1a1610;'
    + 'color:#f0ece0;border:none;border-radius:6px;cursor:pointer;}'
    + '</style></head><body>'
    + '<h2>Keralam Settings</h2>'

    + '<label>Background</label>'
    + '<select id="bg">'
    + '<option value="0"' + (bg=='0'?' selected':'') + '>Cream (#f0ece0)</option>'
    + '<option value="1"' + (bg=='1'?' selected':'') + '>White (#FFFFFF)</option>'
    + '<option value="2"' + (bg=='2'?' selected':'') + '>Light Grey (#AAAAAA)</option>'
    + '</select>'

    + '<label>Battery Style</label>'
    + '<select id="bs">'
    + '<option value="0"' + (batt=='0'?' selected':'') + '>Circular Spiral</option>'
    + '<option value="1"' + (batt=='1'?' selected':'') + '>Flower Radial Fill</option>'
    + '<option value="2"' + (batt=='2'?' selected':'') + '>Square Spiral (Aldo)</option>'
    + '<option value="3"' + (batt=='3'?' selected':'') + '>Battery % (Aldo)</option>'
    + '</select>'

    + '<label>Battery Position</label>'
    + '<select id="bp">'
    + '<option value="0"' + (bpos=='0'?' selected':'') + '>On Watch Face</option>'
    + '<option value="1"' + (bpos=='1'?' selected':'') + '>Bottom Left</option>'
    + '</select>'

    + '<label>Hand Style</label>'
    + '<select id="hs">'
    + '<option value="0"' + (hand=='0'?' selected':'') + '>Smooth Line</option>'
    + '<option value="1"' + (hand=='1'?' selected':'') + '>Pixel Blocky</option>'
    + '<option value="2"' + (hand=='2'?' selected':'') + '>Pixel Tapered</option>'
    + '</select>'

    + '<label>Numeral Weight</label>'
    + '<select id="bld">'
    + '<option value="0"' + (bold=='0'?' selected':'') + '>Regular</option>'
    + '<option value="1"' + (bold=='1'?' selected':'') + '>Bold</option>'
    + '</select>'

    + '<label>Corner Numerals</label>'
    + '<select id="cr">'
    + '<option value="1"' + (crot=='1'?' selected':'') + '>Rotated 45°</option>'
    + '<option value="0"' + (crot=='0'?' selected':'') + '>Flat</option>'
    + '</select>'

    + '<button onclick="save()">Save</button>'
    + '<script>'
    + 'function save(){'
    + 'var g=document.getElementById("bg").value;'
    + 'var b=document.getElementById("bs").value;'
    + 'var p=document.getElementById("bp").value;'
    + 'var h=document.getElementById("hs").value;'
    + 'var d=document.getElementById("bld").value;'
    + 'var r=document.getElementById("cr").value;'
    + 'localStorage.setItem("bg_style",g);'
    + 'localStorage.setItem("battery_style",b);'
    + 'localStorage.setItem("battery_pos",p);'
    + 'localStorage.setItem("hand_style",h);'
    + 'localStorage.setItem("bold_style",d);'
    + 'localStorage.setItem("corner_rot",r);'
    + 'location.href="pebblejs://close#"+encodeURIComponent(JSON.stringify({'
    + 'BG_STYLE:parseInt(g),BATTERY_STYLE:parseInt(b),BATTERY_POS:parseInt(p),'
    + 'HAND_STYLE:parseInt(h),BOLD_STYLE:parseInt(d),CORNER_ROT:parseInt(r)}));'
    + '}'
    + '</script></body></html>';

  Pebble.openURL('data:text/html,' + encodeURIComponent(html));
});

Pebble.addEventListener('webviewclosed', function(e) {
  if (e.response) {
    try {
      var config = JSON.parse(decodeURIComponent(e.response));
      Pebble.sendAppMessage(config,
        function() { console.log('Settings sent ok'); },
        function(e) { console.log('Settings failed: ' + JSON.stringify(e)); }
      );
    } catch(err) { console.log('Parse error: ' + err); }
  }
});
