# Solstice - A Living Landscape Watchface

Solstice transforms your Pebble into a window onto a living world. The sky shifts through dawn, day, dusk, and night using your actual local sunrise and sunset times. A sun or moon arcs across the sky on an accurate astronomical path. Live weather drives the atmosphere - clouds drift, rain falls, snow settles. The landscape changes with the real season. Shake your wrist to trigger a shooting star.

## Features

### Dynamic Sky System
- 4 phases: Night (deep blue), Dawn (peach/gold), Day (cerulean), Dusk (orange/sunset)
- Real sunrise/sunset times fetched from Open-Meteo - sky phases and celestial arc are astronomically accurate, not hardcoded
- 8-band gradient sky on color models; solid black/white on B&W
- Stars visible at night and dusk

### Celestial Tracking
- Sun and Moon arc across the sky based on your actual local sunrise/sunset times
- Real lunar phases: Moon renders the correct phase (new, crescent, quarter, gibbous, full) calculated from the actual synodic cycle using a 2-circle rendering technique
- Sun has a warm glow halo on color displays
- Position updates every minute

### Live Weather
- Fetches from Open-Meteo API (free, no key needed) via PebbleKit JS
- Weather animates the scene: Clear sky, Drifting clouds, Rain streaks, Snowflakes, Thunderstorm
- Refreshes every 30 minutes

### Seasonal Landscape
- Mountains and terrain change color with the actual season:
  - Spring: Green ranges
  - Summer: Deep vibrant greens
  - Fall: Orange and tan tones
  - Winter: Gray mountains with snow caps on the peaks
- B&W models show white snow caps on peaks in winter

### Parallax Mountains
- Two mountain ranges at different depths for a layered silhouette effect
- Colors shift with time of day and season
- On B&W at night: back range in light gray, front range with white outline for depth
- Ground is dark gray at night to distinguish from the black sky

### Shooting Star
- Shake your wrist to trigger a shooting star across the sky
- Brief animation preserves battery

### Weather Bar
- Single line at the bottom: current temp with daily high/low and wind direction/speed
- Example: 72 H:80 L:61 NW 12mph

### Battery and Bluetooth Indicators
- Battery icon top-right: turns red below 20%, green while charging
- Disconnect indicator top-left: small X when phone connection is lost

## Platform Support

| Platform | Model | Display |
|----------|-------|---------|
| emery | Pebble Time 2 | 200x228 color |
| basalt | Pebble Time | 144x168 color |
| chalk | Pebble Time Round | 180x180 color |
| aplite | Pebble Classic | 144x168 B&W |
| diorite | Pebble 2 | 144x168 B&W |

## B&W Adaptations
- Sky: solid white (day) or black (night)
- Sun: filled circle with outline
- Moon: correct lunar phase in white/black
- Mountains: gray/outlined for night visibility
- Ground: dark gray at night

## Battery Efficiency
- Redraws once per minute
- Particles computed deterministically, no continuous animation timer
- Weather refreshes every 30 minutes

## Build and Install
pebble build
pebble install --emulator basalt
pebble install --phone YOUR_PHONE_IP

## Credits
- Weather: Open-Meteo API (free, open-source) - https://open-meteo.com
- Built for the Rebble community
