# ☀️ Solstice — A Living Landscape Watchface

**Theme: The 2026 Solar Maximum**

Solstice transforms your Pebble into a window onto a living world. The sky shifts through dawn, day, dusk, and night in real time. A sun or moon arcs across the sky following its actual path. Weather data drives the atmosphere — clouds drift, rain falls, snow settles. Your daily steps literally grow a flower garden along the terrain. Your heart rate pulses as a ring around the celestial body. Shake your wrist to trigger a shooting star.

## Features

### 🌅 Dynamic Sky System
- **4 phases**: Night (deep blue), Dawn (peach/gold), Day (cerulean/light), Dusk (orange/sunset)
- **8-band gradient** sky on color models; clean B&W contrast on aplite/diorite
- **Stars with twinkle** visible at night and dusk — brightness shifts each minute

### ☀️🌙 Celestial Tracking
- **Sun** (6AM–8PM) and **Moon** (8PM–6AM) arc across the sky on a sinusoidal path
- **Real lunar phases**: Moon renders the correct phase (new, crescent, quarter, gibbous, full) calculated from the actual synodic cycle
- Sun has a warm glow halo; moon uses 2-circle rendering to show the lit/dark terminator accurately
- Position updates every minute based on real hour + minute

### 🌧️ Live Weather
- Fetches from **Open-Meteo API** (free, no key needed) via PebbleKit JS
- Weather affects the scene:
  - **Clear**: Blue sky, no particles
  - **Cloudy**: Drifting cloud formations
  - **Rain/Showers**: Falling rain streaks
  - **Snow**: Floating snowflakes
  - **Fog**: Dense cloud layer
  - **Thunderstorm**: Dark clouds + rain
- Clouds drift across the sky each minute (deterministic pseudo-animation)
- Temperature displayed in complication bar

### 🏔️ Parallax Mountains
- Two mountain ranges at different depths create parallax depth
- Colors shift with time of day (purple at day → black at night)
- Snow-capped when weather reports snow

### 🌸 Step-Powered Flower Garden
- Your **daily steps grow flowers** along the terrain!
- 10,000 steps = full garden of 10 blooming flowers
- Each flower grows a stem first, then blooms with colorful petals
- 7 different petal colors cycle through the garden
- On B&W models: clean white blooms on dark stems

### ❤️ Heart Rate Pulse Ring *(Pebble 2 & Pebble Time 2 only)*
- A ring pulses around the sun/moon based on your heart rate
- **Color-coded**: Green (<80bpm), Orange (80-120), Red (>120)
- Ring radius oscillates with each minute tick
- On B&W: white ring outline
- Only shown on models with a heart rate sensor (diorite/emery platforms)

### 💫 Shooting Star (Tap Animation)
- **Shake your wrist** to trigger a shooting star across the sky!
- Brief animation (~1 second) preserves battery
- Bright yellow head with white tail on color; clean white on B&W

### 📊 Complication Bar
- Clean bottom bar with:
  - **Temperature** (left)
  - **Step count** (center)
  - **Heart rate** (right, in melon/coral)
- **Step progress bar** along the very bottom edge
- Accent line color matches time-of-day phase

## Platform Support

| Platform | Model | Display | Notes |
|----------|-------|---------|-------|
| ✅ emery | Pebble Time 2 | 200×228 color | Primary target |
| ✅ basalt | Pebble Time | 144×168 color | Full color support |
| ✅ chalk | Pebble Time Round | 180×180 color | Round display |
| ✅ aplite | Pebble Classic | 144×168 B&W | Clean B&W adaptation |
| ✅ diorite | Pebble 2 | 144×168 B&W | B&W with health data |

### B&W Adaptations
- Sky: solid white (day) or black (night) — no gradient
- Sun: filled white circle with black outline
- Moon: crescent via overlapping circles
- Stars: only bright stars visible (high contrast)
- Flowers: white blooms, black stems
- All text: high-contrast white-on-black in complication bar

## Battery Efficiency
- Uses `MINUTE_UNIT` — redraws only once per minute
- Particle/cloud positions computed deterministically (no animation timer running)
- Shooting star animation runs briefly on tap, then stops
- Weather refreshes every 30 minutes

## Build & Install
```bash
pebble build
pebble install --emulator emery
pebble install --cloudpebble  # to real watch
```

## Credits
- Weather: [Open-Meteo API](https://open-meteo.com/) (free, open-source)
- Built for the rePebble / Rebble community
