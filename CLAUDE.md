# Solstice Watchface — Claude Code Instructions

## Project Overview
**Solstice** is a Pebble smartwatch **watchface** — a living landscape that shifts with real time, weather, and seasons.
UUID: `86c3c754-aa2f-4fdd-aaad-c3cb061d4bb8`
GitHub: `brooks2564/Pebble-WF-Pebble-Solstice`

## Build & Install
Always rebuild, update the committed PBW (force-add since .gitignore excludes *.pbw), and push together:
```bash
pebble build
cp build/Pebble-WF-Pebble-Solstice.pbw Pebble-WF-Pebble-Solstice.pbw
pebble install --phone 192.168.0.238
git add -f Pebble-WF-Pebble-Solstice.pbw
git commit -m "Update PBW"
git push
```
Note: `.gitignore` excludes `*.pbw` — always use `git add -f` for the PBW file.

## Project Structure
```
Pebble-WF-Pebble-Solstice/
├── package.json                     ← Manifest, all 7 platforms, 10 message keys
├── wscript                          ← Build script
├── CLAUDE.md                        ← This file
├── README.md                        ← Full feature description
├── Pebble-WF-Pebble-Solstice.pbw   ← Latest compiled binary
└── src/
    ├── c/main.c                     ← Watchface C code
    └── pkjs/index.js                ← PebbleKit JS (Open-Meteo weather API)
```

## Target Platforms (all 7)
aplite, basalt, chalk, diorite, emery, flint, gabbro

## Message Keys
| Key           | ID | Direction  |
|---------------|----|------------|
| TEMPERATURE   | 0  | JS → Watch |
| CONDITIONS    | 1  | JS → Watch |
| REQUEST_WEATHER | 2 | Watch → JS |
| WIND_SPEED    | 3  | JS → Watch |
| HUMIDITY      | 4  | JS → Watch |
| HIGH_TEMP     | 5  | JS → Watch |
| LOW_TEMP      | 6  | JS → Watch |
| WIND_DIR      | 7  | JS → Watch |
| SUNRISE_MINS  | 8  | JS → Watch |
| SUNSET_MINS   | 9  | JS → Watch |

## Key Architecture Details
- **MINUTE_UNIT tick** drives all updates (battery efficient)
- **Sky phases**: NIGHT / DAWN / DUSK / DAY — determined by real sunrise/sunset times from Open-Meteo
- **Celestial arc**: sun and moon position computed from elapsed fraction of their arc (sunrise→sunset or vice versa)
- **Lunar phase**: computed from known reference new moon + synodic period; rendered with 2-circle approximation
- **Weather**: Open-Meteo API (free, no key) fetched every 30 min via JS
- **Particles**: rain/snow rendered deterministically from `s_frame_counter` — no continuous timer
- **Flowers**: grow based on daily step count (10,000 steps = full garden)
- **Shooting star**: triggered by wrist tap via `accel_tap_service_subscribe`
- **HR pulse ring**: emery/flint only (platforms with heart rate sensor)
- **AppMessage**: inbox 512 bytes, outbox 64 bytes

## Known Fixed Bugs
- `update_time()` previously hardcoded `s_hour=12, s_minute=0, s_month=5` — fixed to read from `tick_time`

## CloudPebble (repebble)
```
https://cloudpebble.repebble.com/ide/import/github/brooks2564/Pebble-WF-Pebble-Solstice
```
