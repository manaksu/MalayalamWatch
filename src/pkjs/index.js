Pebble.addEventListener('ready', function() {
  console.log('Keralam JS ready');
});

Pebble.addEventListener('showConfiguration', function() {
  var batt  = localStorage.getItem('battery_style') || '0';
  var hand  = localStorage.getItem('hand_style')    || '0';

  var html = '<!DOCTYPE html><html><head>'
    + '<meta name="viewport" content="width=device-width,initial-scale=1">'
    + '<style>'
    + 'body{font-family:sans-serif;background:#f0ece0;color:#1a1610;margin:0;padding:20px;}'
    + 'h2{font-size:18px;margin:0 0 24px;}'
    + 'label{display:block;font-size:14px;margin-bottom:8px;color:#555;}'
    + 'select{width:100%;padding:10px;font-size:15px;border:1px solid #ccc;'
    + 'border-radius:6px;background:#fff;margin-bottom:24px;}'
    + 'button{width:100%;padding:14px;font-size:16px;background:#1a1610;'
    + 'color:#f0ece0;border:none;border-radius:6px;cursor:pointer;}'
    + '</style></head><body>'
    + '<h2>Keralam Settings</h2>'

    + '<label>Battery Style</label>'
    + '<select id="bs">'
    + '<option value="0"' + (batt=='0'?' selected':'') + '>Spiral (Athapookkalam)</option>'
    + '<option value="1"' + (batt=='1'?' selected':'') + '>Flower Radial Fill</option>'
    + '</select>'

    + '<label>Hand Style</label>'
    + '<select id="hs">'
    + '<option value="0"' + (hand=='0'?' selected':'') + '>Smooth Line</option>'
    + '<option value="1"' + (hand=='1'?' selected':'') + '>Pixel Blocky</option>'
    + '<option value="2"' + (hand=='2'?' selected':'') + '>Pixel Tapered</option>'
    + '</select>'

    + '<button onclick="save()">Save</button>'
    + '<script>'
    + 'function save(){'
    + 'var b=document.getElementById("bs").value;'
    + 'var h=document.getElementById("hs").value;'
    + 'localStorage.setItem("battery_style",b);'
    + 'localStorage.setItem("hand_style",h);'
    + 'location.href="pebblejs://close#"'
    + '+encodeURIComponent(JSON.stringify({BATTERY_STYLE:parseInt(b),HAND_STYLE:parseInt(h)}));'
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
    } catch(err) {
      console.log('Parse error: ' + err);
    }
  }
});
