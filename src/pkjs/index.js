// Solstice Watchface — PebbleKit JS
// Fetches weather from Open-Meteo (free, no API key)

function weatherCodeToCondition(code) {
    if (code === 0)                             return 'Clear';
    if (code >= 1  && code <= 3)                return 'Cloudy';
    if (code >= 45 && code <= 48)               return 'Fog';
    if (code >= 51 && code <= 57)               return 'Showers';
    if (code >= 61 && code <= 65)               return 'Rain';
    if (code >= 66 && code <= 67)               return 'Rain';
    if (code >= 71 && code <= 77)               return 'Snow';
    if (code >= 80 && code <= 82)               return 'Showers';
    if (code >= 85 && code <= 86)               return 'Snow';
    if (code >= 95 && code <= 99)               return 'T-Storm';
    return 'Cloudy';
}

function degreesToCompass(deg) {
    var dirs = ['N', 'NE', 'E', 'SE', 'S', 'SW', 'W', 'NW'];
    return dirs[Math.round(deg / 45) % 8];
}

function getWeather() {
    navigator.geolocation.getCurrentPosition(
        function(pos) {
            var lat = pos.coords.latitude;
            var lon = pos.coords.longitude;

            var url = 'https://api.open-meteo.com/v1/forecast?' +
                'latitude=' + lat +
                '&longitude=' + lon +
                '&current=temperature_2m,weather_code,wind_speed_10m,wind_direction_10m,relative_humidity_2m' +
                '&daily=temperature_2m_max,temperature_2m_min,sunrise,sunset,moonrise,moonset' +
                '&temperature_unit=fahrenheit' +
                '&wind_speed_unit=mph' +
                '&forecast_days=1' +
                '&timezone=auto';

            var req = new XMLHttpRequest();
            req.open('GET', url, true);
            req.onload = function() {
                if (req.status === 200) {
                    try {
                        var data = JSON.parse(req.responseText);
                        var temp     = Math.round(data.current.temperature_2m);
                        var code     = data.current.weather_code;
                        var wind     = Math.round(data.current.wind_speed_10m);
                        var windDir  = degreesToCompass(data.current.wind_direction_10m);
                        var humidity = Math.round(data.current.relative_humidity_2m);
                        var condition = weatherCodeToCondition(code);
                        var highTemp = 0, lowTemp = 0;
                        var sunriseMins = 360, sunsetMins = 1200;
                        var moonriseMins = 1200, moonsetMins = 360;
                        if (data.daily && data.daily.temperature_2m_max) {
                            highTemp = Math.round(data.daily.temperature_2m_max[0]);
                            lowTemp  = Math.round(data.daily.temperature_2m_min[0]);
                        }
                        if (data.daily && data.daily.sunrise) {
                            var sr = data.daily.sunrise[0]; // e.g. "2026-04-04T06:23"
                            var srParts = sr.split('T')[1].split(':');
                            sunriseMins = parseInt(srParts[0]) * 60 + parseInt(srParts[1]);
                        }
                        if (data.daily && data.daily.sunset) {
                            var ss = data.daily.sunset[0];
                            var ssParts = ss.split('T')[1].split(':');
                            sunsetMins = parseInt(ssParts[0]) * 60 + parseInt(ssParts[1]);
                        }
                        if (data.daily && data.daily.moonrise && data.daily.moonrise[0]) {
                            var mr = data.daily.moonrise[0];
                            var mrParts = mr.split('T')[1].split(':');
                            moonriseMins = parseInt(mrParts[0]) * 60 + parseInt(mrParts[1]);
                        }
                        if (data.daily && data.daily.moonset && data.daily.moonset[0]) {
                            var mset = data.daily.moonset[0];
                            var msParts = mset.split('T')[1].split(':');
                            moonsetMins = parseInt(msParts[0]) * 60 + parseInt(msParts[1]);
                        }
                        var msg = {
                            'TEMPERATURE':  temp,
                            'CONDITIONS':   condition,
                            'WIND_SPEED':   wind,
                            'WIND_DIR':     windDir,
                            'HUMIDITY':     humidity,
                            'HIGH_TEMP':    highTemp,
                            'LOW_TEMP':     lowTemp,
                            'SUNRISE_MINS': sunriseMins,
                            'SUNSET_MINS':  sunsetMins,
                            'MOONRISE_MINS': moonriseMins,
                            'MOONSET_MINS':  moonsetMins
                        };
                        Pebble.sendAppMessage(msg, function() {
                            console.log('Weather sent: ' + temp + 'F, ' + condition + ', ' + windDir + ' ' + wind + 'mph');
                        }, function(e) {
                            console.log('Weather send failed: ' + JSON.stringify(e));
                        });
                    } catch (e) {
                        console.log('Weather parse error: ' + e.message);
                    }
                } else {
                    console.log('Weather HTTP error: ' + req.status);
                }
            };
            req.onerror = function() { console.log('Weather request failed'); };
            req.send();
        },
        function(err) { console.log('Geolocation error: ' + err.message); },
        { timeout: 15000, maximumAge: 600000 }
    );
}

Pebble.addEventListener('ready', function() {
    console.log('Solstice JS ready');
    getWeather();
});

Pebble.addEventListener('appmessage', function(e) {
    if (e.payload['REQUEST_WEATHER']) {
        getWeather();
    }
});
