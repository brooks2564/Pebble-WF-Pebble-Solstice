# 🌅 Solstice - A Living Landscape Watchface

I'm new to making watchfaces and am having so much fun! Solstice transforms your Pebble into a window onto a living world. The sky shifts through dawn, day, dusk, and night using your actual local sunrise and sunset times. A sun or moon arcs across the sky on an accurate astronomical path. Live weather drives the atmosphere - clouds drift, rain falls, snow settles. The landscape changes with the real season. Shake your wrist to trigger a shooting star.

## ✨ Features

### 🌤️ Dynamic Sky System
- 4 phases: Night (deep blue), Dawn (peach/gold), Day (cerulean), Dusk (orange/sunset)
- Real sunrise/sunset times fetched from Open-Meteo - sky phases and celestial arc are astronomically accurate, not hardcoded
- ⭐ Stars visible at night and dusk

### ☀️ Celestial Tracking
- Sun and Moon arc across the sky based on your actual local sunrise/sunset times
- 🌙 Real lunar phases: Moon renders the correct phase (new, crescent, quarter, gibbous, full) calculated from the actual synodic cycle using a 2-circle rendering technique
- Sun has a warm glow halo
- Position updates every minute

### 🌦️ Live Weather
- Fetches from Open-Meteo API (free, no key needed) via PebbleKit JS
- Weather animates the scene: ☀️ Clear sky, ☁️ Drifting clouds, 🌧️ Rain streaks, ❄️ Snowflakes, ⛈️ Thunderstorm
- Refreshes every 30 minutes

### 🍂 Seasonal Landscape
- Mountains and terrain change color with the actual season:
  - 🌸 Spring: Green ranges
  - ☀️ Summer: Deep vibrant greens
  - 🍁 Fall: Orange and tan tones
  - ❄️ Winter: Gray mountains with snow caps on the peaks

### ⛰️ Parallax Mountains
- Two mountain ranges at different depths for a layered silhouette effect
- Colors shift with time of day and season

### 🌠 Shooting Star
- Shake your wrist to trigger a shooting star across the sky
- Brief animation preserves battery

### 🌡️ Weather Bar
- Single line at the bottom: current temp with daily high/low and wind direction/speed
- Example: 72° H:80 L:61 NW 12mph

### ❤️ Heart Rate Monitor *(Pebble Core Time 2 and Pebble Time 2 only)*
- Displays live heart rate in beats per minute in the complication bar
- Only appears on Pebble Core Time 2 (flint) and Pebble Time 2 (emery), which have built-in heart rate sensors
- Rate shown with a pulsing ring animation around the sun/moon
- Updates every minute alongside other health metrics

### 🌸 Flower Garden *(Steps Counter)*
- A small garden of flowers grows along the bottom of the landscape
- Each flower represents your daily step progress
- Flowers sprout and grow taller as you accumulate steps throughout the day
- Stems are green, petals shift from yellow to pink to red
- Resets at midnight with the step counter

### 🔋 Battery and Bluetooth Indicators
- 🔋 Battery icon top-right: turns red below 20%, green while charging
- 📵 Disconnect indicator top-left: small X when phone connection is lost

Please feel free to email me at broomaninks@gmail.com to let me know of any bugs or suggestions. Thank you for trying out Solstice and enjoy!

## 📱 Platform Support

| Platform | Model | Display | Screen Shape | HR Sensor |
|----------|-------|---------|--------------|-----------|
| emery | Pebble Time 2 | 200x228 color | Rectangular | ✅ |
| gabbro | Pebble Round 2 | 180x180 color | Round | ❌ |
| basalt | Pebble Time | 144x168 color | Rectangular | ❌ |
| chalk | Pebble Time Round | 180x180 color | Round | ❌ |

## ⚡ Battery Efficiency
- Redraws once per minute
- Particles computed deterministically, no continuous animation timer
- Weather refreshes every 30 minutes

## 🔧 Build and Install

### CloudPebble (no local toolchain needed)
Open in the community-hosted CloudPebble IDE:
```
https://cloudpebble.repebble.com/ide/import/github/brooks2564/Pebble-WF-Pebble-Solstice
```

### Local build
```bash
pebble build
cp build/Pebble-WF-Pebble-Solstice.pbw Pebble-WF-Pebble-Solstice.pbw
pebble install --phone <your-phone-ip>
git add -f Pebble-WF-Pebble-Solstice.pbw
git commit -m "Update PBW"
git push
```
> Note: `.gitignore` excludes `*.pbw` — use `git add -f` to force-add the compiled binary.

## 🙏 Credits
- 🌤️ Weather: Open-Meteo API (free, open-source) - https://open-meteo.com
- 💙 Built for the Rebble community
