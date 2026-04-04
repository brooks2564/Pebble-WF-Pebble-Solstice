// Solstice Watchface — PebbleKit JS
// Fetches weather from Open-Meteo (free, no API key)

// WMO Weather Codes → human-readable condition strings
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

function getWeather() {
    navigator.geolocation.getCurrentPosition(
        function(pos) {
            var lat = pos.coords.latitude;
            var lon = pos.coords.longitude;

            var url = 'https://api.open-meteo.com/v1/forecast?' +
                'latitude=' + lat +
                '&longitude=' + lon +
                '&current=temperature_2m,weather_code,wind_speed_10m,relative_humidity_2m' +
                '&daily=temperature_2m_max,temperature_2m_min' +
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

                        var temp       = Math.round(data.current.temperature_2m);
                        var code       = data.current.weather_code;
                        var wind       = Math.round(data.current.wind_speed_10m);
                        var humidity   = Math.round(data.current.relative_humidity_2m);
                        var condition  = weatherCodeToCondition(code);

                        var highTemp = 0;
                        var lowTemp  = 0;
                        if (data.daily && data.daily.temperature_2m_max) {
                            highTemp = Math.round(data.daily.temperature_2m_max[0]);
                            lowTemp  = Math.round(data.daily.temperature_2m_min[0]);
                        }

                        var msg = {
                            'TEMPERATURE':    temp,
                            'CONDITIONS':     condition,
                            'WIND_SPEED':     wind,
                            'HUMIDITY':       humidity,
                            'HIGH_TEMP':      highTemp,
                            'LOW_TEMP':       lowTemp
                        };

                        Pebble.sendAppMessage(msg, function() {
                            console.log('Weather sent: ' + temp + '°F, ' + condition);
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
            req.onerror = function() {
                console.log('Weather request failed');
            };
            req.send();
        },
        function(err) {
            console.log('Geolocation error: ' + err.message);
        },
        { timeout: 15000, maximumAge: 600000 }
    );
}

// Fetch weather on launch
Pebble.addEventListener('ready', function() {
    console.log('Solstice JS ready');
    getWeather();
});

// Respond to weather refresh requests from C side
Pebble.addEventListener('appmessage', function(e) {
    if (e.payload['REQUEST_WEATHER']) {
        getWeather();
    }
});
