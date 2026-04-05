/*
 * SOLSTICE — A Living Landscape Watchface
 * =========================================
 * Theme: "The Golden Age of Exploration" (2026 Solar Maximum)
 *
 * The sky shifts from dawn→day→dusk→night based on real time.
 * A sun or moon arcs across the sky on its actual path.
 * Weather data drives: sky color, cloud layers, rain/snow particles.
 * Steps grow a flower garden along the bottom terrain.
 * Heart rate pulses a ring around the celestial body.
 * Tap triggers a brief shooting-star animation.
 *
 * Supports: emery (200x228), basalt (144x168), chalk (180x180 round),
 *           aplite/diorite (144x168 B&W)
 */

#include <pebble.h>

// ───────── Constants ─────────
#define ANIM_DURATION_MS    50
#define ANIM_TOTAL_FRAMES   20
#define MAX_PARTICLES        16
#define MAX_STARS            12
#define MAX_FLOWERS          10
#define WEATHER_REFRESH_MIN  30

// Weather condition codes (mapped from Open-Meteo weather_code)
#define WEATHER_CLEAR    0
#define WEATHER_CLOUDY   2
#define WEATHER_FOG      45
#define WEATHER_RAIN     63
#define WEATHER_SNOW     73
#define WEATHER_STORM    95
#define WEATHER_UNKNOWN  -1

// Time-of-day phases
#define PHASE_NIGHT      0

// Seasons
#define SEASON_WINTER    0
#define SEASON_SPRING    1
#define SEASON_SUMMER    2
#define SEASON_FALL      3
#define PHASE_DAWN       1
#define PHASE_DAY        2
#define PHASE_DUSK       3

// ───────── Data Structures ─────────
typedef struct {
    int16_t x;
    int16_t y;
    int8_t  speed;
    int8_t  size;
} Particle;

typedef struct {
    int16_t x;
    int16_t y;
    int8_t  brightness; // 0-3 for twinkle
} Star;

typedef struct {
    int16_t x;
    int16_t height;
    bool    bloomed;
} Flower;

// ───────── State ─────────
static Window    *s_window;
static Layer     *s_canvas;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static TextLayer *s_temp_layer;
static bool       s_has_hr = false;
#if defined(PBL_PLATFORM_EMERY)
static TextLayer *s_hr_layer;
static char       s_hr_buf[12];
#endif


static char s_time_buf[8];
static char s_date_buf[16];
static char s_temp_buf[20];

// Weather
static int  s_temperature    = 0;
static int  s_high_temp      = 0;
static int  s_low_temp       = 0;
static int  s_wind_speed     = 0;
static char s_wind_dir[4]    = "--";
static int  s_weather_code   = WEATHER_CLEAR;
static bool s_weather_valid  = false;

static int    s_steps     = 0;
static Flower s_flowers[MAX_FLOWERS];
#if defined(PBL_PLATFORM_EMERY)
static int    s_heart_rate = 0;
#endif

// Health


// Animation
static AppTimer *s_anim_timer  = NULL;
static int       s_anim_frame  = 0;
static bool      s_animating   = false;

// Shooting star on tap
static bool  s_shooting_star_active = false;
static int   s_star_frame = 0;

// Scene elements
static Particle s_particles[MAX_PARTICLES];
static Star     s_stars[MAX_STARS];
static int      s_frame_counter = 0; // incremented each minute for pseudo-animation

// Current time phase
static int s_hour        = 12;
static int s_minute      = 0;
static int s_month       = 0;
static int s_sunrise_mins = 360;   // default 6:00am
static int s_sunset_mins  = 1200;  // default 8:00pm

// ───────── Color Helpers ─────────

// Get time-of-day phase using real sunrise/sunset times
static int get_phase(int hour) {
    int mins = hour * 60 + s_minute;
    int dawn_start = s_sunrise_mins - 60;
    int dusk_end   = s_sunset_mins  + 60;
    if (mins >= dawn_start && mins < s_sunrise_mins) return PHASE_DAWN;
    if (mins >= s_sunrise_mins && mins < s_sunset_mins) return PHASE_DAY;
    if (mins >= s_sunset_mins  && mins < dusk_end)   return PHASE_DUSK;
    return PHASE_NIGHT;
}

// Get current season from month
static int get_season(void) {
    if (s_month == 11 || s_month <= 1) return SEASON_WINTER;
    if (s_month <= 4)                  return SEASON_SPRING;
    if (s_month <= 7)                  return SEASON_SUMMER;
    return SEASON_FALL;
}

#ifdef PBL_COLOR
static GColor sky_top_color(int phase) {
    switch (phase) {
        case PHASE_DAWN:  return GColorMelon;
        case PHASE_DAY:   return (s_weather_code == WEATHER_RAIN || s_weather_code == WEATHER_STORM)
                                  ? GColorDarkGray : GColorVividCerulean;
        case PHASE_DUSK:  return GColorOrange;
        case PHASE_NIGHT: return GColorOxfordBlue;
        default:          return GColorBlack;
    }
}

static GColor sky_bottom_color(int phase) {
    switch (phase) {
        case PHASE_DAWN:  return GColorRajah;
        case PHASE_DAY:   return (s_weather_code == WEATHER_RAIN || s_weather_code == WEATHER_STORM)
                                  ? GColorLightGray : GColorCeleste;
        case PHASE_DUSK:  return GColorSunsetOrange;
        case PHASE_NIGHT: return GColorOxfordBlue;
        default:          return GColorBlack;
    }
}

static GColor terrain_color(int phase) {
    switch (phase) {
        case PHASE_DAWN:  return GColorArmyGreen;
        case PHASE_DAY:   return GColorIslamicGreen;
        case PHASE_DUSK:  return GColorDarkGreen;
        case PHASE_NIGHT: return GColorBlack;
        default:          return GColorDarkGreen;
    }
}

static GColor mountain_color(int phase) {
    switch (phase) {
        case PHASE_DAWN:  return GColorDarkGray;
        case PHASE_DAY:   return (s_weather_code == WEATHER_SNOW) ? GColorWhite : GColorImperialPurple;
        case PHASE_DUSK:  return GColorBulgarianRose;
        case PHASE_NIGHT: return GColorBlack;
        default:          return GColorDarkGray;
    }
}

static GColor sun_color(void) { return GColorChromeYellow; }
static GColor moon_color(void) { return GColorPastelYellow; }

#endif

#ifdef PBL_COLOR
static GColor flower_stem_color(void) { return GColorMayGreen; }
static GColor flower_petal_color(int idx) {
    GColor options[] = { GColorRed, GColorYellow, GColorMagenta, GColorOrange,
                         GColorCyan, GColorRoseVale, GColorVividViolet };
    return options[idx % 7];
}
#endif

// Compute lunar phase as 0-255 (0=new moon, 128=full moon, 255=just before new)
static int compute_lunar_phase(void) {
    // Reference new moon: Jan 6, 2000 18:14 UTC (Unix timestamp 947182440)
    // Synodic period: 29.530589 days = 2551443 seconds
    time_t known_new_moon = 947182440;
    int32_t cycle_sec = 2551443;
    time_t now = time(NULL);
    int32_t elapsed = (int32_t)((now - known_new_moon) % cycle_sec);
    if (elapsed < 0) elapsed += cycle_sec;
    return (int)(elapsed * 256 / cycle_sec);
}

// Draw moon with correct lunar phase using 2-circle approximation
// phase_256: 0=new moon, 64=first quarter, 128=full moon, 192=last quarter
static void draw_moon_phase(GContext *ctx, int cx, int cy, int r, int phase_256,
                             GColor lit, GColor dark) {
    phase_256 = phase_256 % 256;
    bool waxing = (phase_256 <= 128);
    // half_phase: 0=new(or full), 128=full(or new) — where we are in each half-cycle
    int half_phase = waxing ? phase_256 : (256 - phase_256);
    // Compute horizontal offset of the shadow circle using cosine
    // half_phase=0 → angle=0 → cos=1 → dx=-r  (shadow fully covers = new moon)
    // half_phase=64 → angle=π/2 → cos=0 → dx=0 (shadow at center = quarter moon)
    // half_phase=128 → angle=π → cos=-1 → dx=+r (shadow off edge = full moon)
    int32_t trig_angle = (int32_t)half_phase * TRIG_MAX_ANGLE / 256;
    int dx = -(int)(r * cos_lookup(trig_angle) / TRIG_MAX_RATIO);

    if (waxing) {
        // Right side lit: draw lit base, overlay dark shadow on left
        graphics_context_set_fill_color(ctx, lit);
        graphics_fill_circle(ctx, GPoint(cx, cy), r);
        graphics_context_set_fill_color(ctx, dark);
        graphics_fill_circle(ctx, GPoint(cx + dx, cy), r);
    } else {
        // Left side lit: draw lit base, overlay dark shadow on right (mirrored)
        graphics_context_set_fill_color(ctx, lit);
        graphics_fill_circle(ctx, GPoint(cx, cy), r);
        graphics_context_set_fill_color(ctx, dark);
        graphics_fill_circle(ctx, GPoint(cx - dx, cy), r);
    }
}


// ───────── Deterministic Pseudo-Random ─────────
static int pseudo_rand(int seed) {
    seed = (seed * 1103515245 + 12345) & 0x7FFFFFFF;
    return seed;
}

// ───────── Initialize Scene Elements ─────────
static void init_stars(GRect bounds) {
    for (int i = 0; i < MAX_STARS; i++) {
        int seed = pseudo_rand(i * 7919);
        s_stars[i].x = (seed >> 4) % bounds.size.w;
        seed = pseudo_rand(seed);
        s_stars[i].y = (seed >> 4) % (bounds.size.h / 2);
        s_stars[i].brightness = i % 4;
    }
}

static void init_particles(GRect bounds) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        int seed = pseudo_rand(i * 6151 + s_frame_counter * 37);
        s_particles[i].x = (seed >> 4) % bounds.size.w;
        seed = pseudo_rand(seed);
        s_particles[i].y = (seed >> 4) % bounds.size.h;
        s_particles[i].speed = 2 + (i % 4);
        s_particles[i].size  = 1 + (i % 2);
    }
}

// ───────── Update Health Data ─────────
static void update_health(void) {
#if PBL_API_EXISTS(health_service_peek_current_value)
    s_steps = 1200;
#if defined(PBL_PLATFORM_EMERY)
    if (s_has_hr) {
        HealthValue hr = health_service_peek_current_value(HealthMetricHeartRateBPM);
        if (hr > 0) s_heart_rate = (int)hr;
        snprintf(s_hr_buf, sizeof(s_hr_buf), "%d bpm", s_heart_rate > 0 ? s_heart_rate : 0);
        if (s_hr_layer) text_layer_set_text(s_hr_layer, s_hr_buf);
    }
#endif
#endif

    // Step-driven flower growth (10,000 steps = full garden)
    int step_pct = s_steps * 100 / 10000;
    if (step_pct > 100) step_pct = 100;
    int flowers_active = step_pct * MAX_FLOWERS / 100;
    if (flowers_active < 1 && s_steps > 0) flowers_active = 1;
    for (int i = 0; i < MAX_FLOWERS; i++) {
        if (i < flowers_active) {
            int target_h = 8 + (i * 3) % 12;
            if (s_flowers[i].height < target_h) {
                s_flowers[i].height += 2;
                if (s_flowers[i].height > target_h) s_flowers[i].height = target_h;
            }
            s_flowers[i].bloomed = (s_flowers[i].height >= target_h);
        } else {
            s_flowers[i].height  = 0;
            s_flowers[i].bloomed = false;
        }
    }
}

// ───────── Drawing Functions ─────────

// Gradient sky (simulated with horizontal stripes)
static void draw_sky(GContext *ctx, GRect bounds) {
    int phase = get_phase(s_hour);
    int sky_h = bounds.size.h * 60 / 100; // sky takes 60% of screen

#ifdef PBL_COLOR
    GColor top = sky_top_color(phase);
    GColor bot = sky_bottom_color(phase);

    // 8-band gradient
    int bands = 8;
    int band_h = sky_h / bands;
    for (int b = 0; b < bands; b++) {
        // Interpolate R/G/B components between top and bottom colors
        // Using simple step interpolation for 64-color palette
        GColor c = (b < bands / 2) ? top : bot;
        graphics_context_set_fill_color(ctx, c);
        GRect band_rect = GRect(0, b * band_h, bounds.size.w, band_h + 1);
        graphics_fill_rect(ctx, band_rect, 0, GCornerNone);
    }
#else
    // B&W: dithered pattern based on phase
    if (phase == PHASE_NIGHT) {
        graphics_context_set_fill_color(ctx, GColorBlack);
        graphics_fill_rect(ctx, GRect(0, 0, bounds.size.w, sky_h), 0, GCornerNone);
    } else {
        // White upper sky, LightGray lower band near horizon for depth
        graphics_context_set_fill_color(ctx, GColorWhite);
        graphics_fill_rect(ctx, GRect(0, 0, bounds.size.w, sky_h * 2 / 3), 0, GCornerNone);
        graphics_context_set_fill_color(ctx, GColorLightGray);
        graphics_fill_rect(ctx, GRect(0, sky_h * 2 / 3, bounds.size.w, sky_h / 3), 0, GCornerNone);
    }
#endif
}

// Stars (visible at night)
static void draw_stars(GContext *ctx, GRect bounds) {
    int phase = get_phase(s_hour);
    if (phase != PHASE_NIGHT && phase != PHASE_DUSK) return;

    for (int i = 0; i < MAX_STARS; i++) {
        // Twinkle: brightness varies with frame counter
        int bright = (s_stars[i].brightness + s_frame_counter) % 4;

#ifdef PBL_COLOR
        GColor star_colors[] = { GColorDarkGray, GColorLightGray, GColorWhite, GColorPastelYellow };
        graphics_context_set_fill_color(ctx, star_colors[bright]);
#else
        if (bright < 2) continue; // dim stars invisible in B&W
        graphics_context_set_fill_color(ctx, GColorWhite);
#endif

        int sz = (bright >= 2) ? 2 : 1;
        graphics_fill_rect(ctx, GRect(s_stars[i].x, s_stars[i].y, sz, sz), 0, GCornerNone);
    }
}

// Celestial body (sun or moon) arcing across the sky
static void draw_celestial(GContext *ctx, GRect bounds) {
    int phase = get_phase(s_hour);
    bool is_sun = (phase == PHASE_DAWN || phase == PHASE_DAY || phase == PHASE_DUSK);

    // Calculate arc position using real sunrise/sunset times
    int total_min, elapsed_min;
    int current_mins = s_hour * 60 + s_minute;
    if (is_sun) {
        total_min   = s_sunset_mins - s_sunrise_mins;
        elapsed_min = current_mins - s_sunrise_mins;
    } else {
        total_min = (24 * 60 - s_sunset_mins) + s_sunrise_mins;
        if (current_mins >= s_sunset_mins) elapsed_min = current_mins - s_sunset_mins;
        else elapsed_min = (24 * 60 - s_sunset_mins) + current_mins;
    }
    if (total_min <= 0) total_min = 1;
    if (elapsed_min < 0) elapsed_min = 0;
    if (elapsed_min > total_min) elapsed_min = total_min;

    // Arc from left to right across sky
    // angle: 0 (left horizon) → 180 degrees (right horizon)
    int32_t angle_deg = 180 * elapsed_min / total_min;
    int32_t angle = DEG_TO_TRIGANGLE(180 - angle_deg); // invert so left→right

    int sky_h    = bounds.size.h * 60 / 100;
    int arc_top  = bounds.size.h / 8;
    int arc_base = sky_h - 5;
    int cx = bounds.size.w / 2;
    int arc_radius_x = bounds.size.w * 40 / 100;
    int arc_radius_y = (arc_base - arc_top);

    int body_x = cx + (int)(arc_radius_x * cos_lookup(angle) / TRIG_MAX_RATIO);
    int body_y = arc_base - (int)(arc_radius_y * sin_lookup(angle) / TRIG_MAX_RATIO);

    int body_r = is_sun ? 14 : 10;

#ifdef PBL_COLOR
    GColor body_c = is_sun ? sun_color() : moon_color();

    // Glow effect: larger semi-transparent circle behind
    if (is_sun) {
        graphics_context_set_fill_color(ctx, GColorRajah);
        graphics_fill_circle(ctx, GPoint(body_x, body_y), body_r + 6);
        graphics_context_set_fill_color(ctx, GColorYellow);
        graphics_fill_circle(ctx, GPoint(body_x, body_y), body_r + 3);
    }

    if (is_sun) {
        graphics_context_set_fill_color(ctx, body_c);
        graphics_fill_circle(ctx, GPoint(body_x, body_y), body_r);
    } else {
        int lp = compute_lunar_phase();
        draw_moon_phase(ctx, body_x, body_y, body_r, lp,
                        GColorPastelYellow, GColorOxfordBlue);
    }
#else
    // B&W: filled circle for sun, outline for moon
    if (is_sun) {
        graphics_context_set_fill_color(ctx, GColorWhite);
        graphics_fill_circle(ctx, GPoint(body_x, body_y), body_r);
        graphics_context_set_stroke_color(ctx, GColorBlack);
        graphics_draw_circle(ctx, GPoint(body_x, body_y), body_r);
    } else {
        int lp = compute_lunar_phase();
        draw_moon_phase(ctx, body_x, body_y, body_r, lp,
                        GColorWhite, GColorBlack);
    }
#endif

#if defined(PBL_PLATFORM_EMERY)
    if (s_has_hr && s_heart_rate > 40) {
        int pulse_phase = (s_frame_counter * 6 + s_heart_rate) % 20;
        int pulse_r = body_r + 8 + pulse_phase / 4;
#ifdef PBL_COLOR
        GColor pulse_c = (s_heart_rate > 120) ? GColorRed :
                         (s_heart_rate > 80)  ? GColorOrange : GColorMintGreen;
        graphics_context_set_stroke_color(ctx, pulse_c);
#else
        graphics_context_set_stroke_color(ctx, GColorWhite);
#endif
        graphics_context_set_stroke_width(ctx, 1);
        graphics_draw_circle(ctx, GPoint(body_x, body_y), pulse_r);
    }
#endif

}

// Draw clouds
static void draw_clouds(GContext *ctx, GRect bounds) {
    if (s_weather_code != WEATHER_CLOUDY && s_weather_code != WEATHER_RAIN &&
        s_weather_code != WEATHER_STORM && s_weather_code != WEATHER_FOG)
        return;

    int sky_h = bounds.size.h * 60 / 100;
    int num_clouds = (s_weather_code == WEATHER_FOG) ? 6 : 3;

    for (int i = 0; i < num_clouds; i++) {
        int cx = (pseudo_rand(i * 4231 + 99) >> 4) % bounds.size.w;
        int cy = 15 + (pseudo_rand(i * 7717) >> 4) % (sky_h / 3);
        // Drift with frame counter
        cx = (cx + s_frame_counter * (3 + i)) % (bounds.size.w + 40) - 20;

#ifdef PBL_COLOR
        GColor cc = (s_weather_code == WEATHER_STORM) ? GColorDarkGray : GColorLightGray;
        graphics_context_set_fill_color(ctx, cc);
#else
        // Day/dawn: dark clouds visible on light sky; Night: light on dark
        graphics_context_set_fill_color(ctx, (get_phase(s_hour) == PHASE_NIGHT) ? GColorLightGray : GColorDarkGray);
#endif
        // Cloud = overlapping circles
        graphics_fill_circle(ctx, GPoint(cx, cy), 8);
        graphics_fill_circle(ctx, GPoint(cx + 10, cy - 3), 10);
        graphics_fill_circle(ctx, GPoint(cx + 20, cy), 7);
        graphics_fill_circle(ctx, GPoint(cx + 8, cy + 4), 6);
    }
}

// Mountains (parallax — two layers)
static void draw_mountains(GContext *ctx, GRect bounds) {
    int sky_h   = bounds.size.h * 60 / 100;
    int phase   = get_phase(s_hour);
    int season  = get_season();
    int base_y  = sky_h;

#ifdef PBL_COLOR
    // Seasonal back-range color
    GColor back_c;
    if (season == SEASON_WINTER)      back_c = GColorDarkGray;
    else if (season == SEASON_SPRING) back_c = GColorDarkGreen;
    else if (season == SEASON_SUMMER) back_c = GColorArmyGreen;
    else                              back_c = GColorWindsorTan;
    if (phase == PHASE_NIGHT) back_c = GColorBlack;

    // Seasonal front-range color
    GColor front_c;
    if (season == SEASON_WINTER)      front_c = GColorLightGray;
    else if (season == SEASON_SPRING) front_c = GColorMayGreen;
    else if (season == SEASON_SUMMER) front_c = GColorIslamicGreen;
    else                              front_c = GColorOrange;
    if (phase == PHASE_NIGHT) front_c = GColorDarkGray;
#endif

    // Back mountain range
    int num_back = 5;
    for (int i = 0; i < num_back; i++) {
        int px = bounds.size.w * i / (num_back - 1);
        int peak_h = 20 + pseudo_rand(i * 1913) % 25;
        GPoint tri[3] = {
            GPoint(px - 25, base_y),
            GPoint(px, base_y - peak_h),
            GPoint(px + 25, base_y)
        };
        GPathInfo info = { .num_points = 3, .points = tri };
        GPath *p = gpath_create(&info);
#ifdef PBL_COLOR
        graphics_context_set_fill_color(ctx, back_c);
        gpath_draw_filled(ctx, p);
        // Winter snow caps
        if (season == SEASON_WINTER) {
            int cap_h = peak_h / 3;
            GPoint cap[3] = {
                GPoint(px - cap_h, base_y - peak_h + cap_h),
                GPoint(px, base_y - peak_h),
                GPoint(px + cap_h, base_y - peak_h + cap_h)
            };
            GPathInfo ci = { .num_points = 3, .points = cap };
            GPath *cp = gpath_create(&ci);
            graphics_context_set_fill_color(ctx, GColorWhite);
            gpath_draw_filled(ctx, cp);
            gpath_destroy(cp);
        }
#else
        if (phase == PHASE_NIGHT) {
            // Night: light gray silhouette against black sky
            graphics_context_set_fill_color(ctx, GColorLightGray);
            gpath_draw_filled(ctx, p);
        } else {
            // Day/dawn/dusk: dark gray so front range has more contrast
            graphics_context_set_fill_color(ctx, GColorDarkGray);
            gpath_draw_filled(ctx, p);
            // Winter snow caps on B&W (day only)
            if (season == SEASON_WINTER) {
                int cap_h = peak_h / 3;
                GPoint cap[3] = {
                    GPoint(px - cap_h, base_y - peak_h + cap_h),
                    GPoint(px, base_y - peak_h),
                    GPoint(px + cap_h, base_y - peak_h + cap_h)
                };
                GPathInfo ci = { .num_points = 3, .points = cap };
                GPath *cp = gpath_create(&ci);
                graphics_context_set_fill_color(ctx, GColorWhite);
                gpath_draw_filled(ctx, cp);
                gpath_destroy(cp);
            }
        }
#endif
        gpath_destroy(p);
    }

    // Front mountain range — outline on B&W for depth
    int num_front = 4;
    for (int i = 0; i < num_front; i++) {
        int px = bounds.size.w * (2 * i + 1) / (2 * num_front);
        int peak_h = 12 + pseudo_rand(i * 2741 + 100) % 15;
        GPoint tri[3] = {
            GPoint(px - 20, base_y),
            GPoint(px, base_y - peak_h),
            GPoint(px + 20, base_y)
        };
        GPathInfo info = { .num_points = 3, .points = tri };
        GPath *p = gpath_create(&info);
#ifdef PBL_COLOR
        graphics_context_set_fill_color(ctx, front_c);
        gpath_draw_filled(ctx, p);
#else
        if (phase == PHASE_NIGHT) {
            // Night: DarkGray against black sky, lighter than terrain
            graphics_context_set_fill_color(ctx, GColorDarkGray);
            gpath_draw_filled(ctx, p);
            graphics_context_set_stroke_color(ctx, GColorLightGray);
            graphics_context_set_stroke_width(ctx, 1);
            gpath_draw_outline(ctx, p);
        } else {
            // Day: black fill against lighter sky/back range
            graphics_context_set_fill_color(ctx, GColorBlack);
            gpath_draw_filled(ctx, p);
        }
#endif
        gpath_destroy(p);
    }
}

// Terrain / ground
static void init_flowers(GRect bounds) {
    int garden_start = bounds.size.w / 6;
    int garden_width = bounds.size.w * 2 / 3;
    for (int i = 0; i < MAX_FLOWERS; i++) {
        int seed = pseudo_rand(i * 3571);
        s_flowers[i].x      = garden_start + ((seed >> 4) % garden_width);
        s_flowers[i].height  = 0;
        s_flowers[i].bloomed = false;
    }
}

// Flower garden — grows with daily steps
static void draw_flowers(GContext *ctx, GRect bounds) {
    int ground_y = bounds.size.h * 60 / 100 + 3;
    for (int i = 0; i < MAX_FLOWERS; i++) {
        if (s_flowers[i].height <= 0) continue;
        int fx = s_flowers[i].x;
        int stem_base = ground_y + 2;
        int stem_top  = stem_base - s_flowers[i].height;
#ifdef PBL_COLOR
        graphics_context_set_stroke_color(ctx, flower_stem_color());
#else
        graphics_context_set_stroke_color(ctx, GColorBlack);
#endif
        graphics_context_set_stroke_width(ctx, 1);
        graphics_draw_line(ctx, GPoint(fx, stem_base), GPoint(fx + 1, stem_top));
        if (s_flowers[i].bloomed) {
#ifdef PBL_COLOR
            graphics_context_set_fill_color(ctx, flower_petal_color(i));
#else
            graphics_context_set_fill_color(ctx, GColorDarkGray);
#endif
            int r = 2;
            graphics_fill_circle(ctx, GPoint(fx + 1, stem_top), r);
            graphics_fill_rect(ctx, GRect(fx - 1, stem_top - r - 1, 4, 2), 0, GCornerNone);
            graphics_fill_rect(ctx, GRect(fx - 1, stem_top + r, 4, 2), 0, GCornerNone);
#ifdef PBL_COLOR
            graphics_context_set_fill_color(ctx, GColorYellow);
            graphics_fill_rect(ctx, GRect(fx, stem_top - 1, 2, 2), 0, GCornerNone);
#endif
        }
    }
}

static void draw_terrain(GContext *ctx, GRect bounds) {
    int sky_h = bounds.size.h * 60 / 100;
    int phase = get_phase(s_hour);

#ifdef PBL_COLOR
    graphics_context_set_fill_color(ctx, terrain_color(phase));
#else
    graphics_context_set_fill_color(ctx, (phase == PHASE_NIGHT) ? GColorDarkGray : GColorLightGray);
#endif

    // Rolling hills using overlapping circles
    int ground_y = sky_h;
    // Fill solid ground first
    graphics_fill_rect(ctx, GRect(0, ground_y + 5, bounds.size.w, bounds.size.h - ground_y - 5),
                       0, GCornerNone);

    // Gentle hills
    for (int i = 0; i < 4; i++) {
        int hx = bounds.size.w * (2 * i + 1) / 8;
        int hy = ground_y + 5;
        int hr = 30 + (i * 7) % 15;
        graphics_fill_circle(ctx, GPoint(hx, hy), hr);
    }


}

// Weather particles (rain drops or snowflakes)
static void draw_particles(GContext *ctx, GRect bounds) {
    if (s_weather_code != WEATHER_RAIN && s_weather_code != WEATHER_SNOW &&
        s_weather_code != WEATHER_STORM)
        return;

    bool is_snow = (s_weather_code == WEATHER_SNOW);
    int sky_h = bounds.size.h * 60 / 100;

    for (int i = 0; i < MAX_PARTICLES; i++) {
        // Deterministic "animation" driven by frame_counter
        int rx = (s_particles[i].x + s_frame_counter * (2 + i % 3)) % bounds.size.w;
        int ry = (s_particles[i].y + s_frame_counter * s_particles[i].speed) % sky_h;

#ifdef PBL_COLOR
        if (is_snow) {
            graphics_context_set_fill_color(ctx, GColorWhite);
            graphics_fill_circle(ctx, GPoint(rx, ry), s_particles[i].size);
        } else {
            graphics_context_set_stroke_color(ctx, GColorPictonBlue);
            graphics_context_set_stroke_width(ctx, 1);
            graphics_draw_line(ctx, GPoint(rx, ry), GPoint(rx - 1, ry + 4));
        }
#else
        {
            GColor bw_c = (get_phase(s_hour) == PHASE_NIGHT) ? GColorWhite : GColorDarkGray;
            if (is_snow) {
                graphics_context_set_fill_color(ctx, bw_c);
                graphics_fill_rect(ctx, GRect(rx, ry, 2, 2), 0, GCornerNone);
            } else {
                graphics_context_set_stroke_color(ctx, bw_c);
                graphics_draw_line(ctx, GPoint(rx, ry), GPoint(rx, ry + 3));
            }
        }
#endif
    }
}

// Shooting star animation (triggered by tap)
static void draw_shooting_star(GContext *ctx, GRect bounds) {
    if (!s_shooting_star_active) return;

    int sx = bounds.size.w - s_star_frame * (bounds.size.w / ANIM_TOTAL_FRAMES);
    int sy = 5 + s_star_frame * 2;
    int tail_len = 15;

#ifdef PBL_COLOR
    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_context_set_stroke_width(ctx, 2);
    graphics_draw_line(ctx, GPoint(sx, sy), GPoint(sx + tail_len, sy - tail_len / 2));
    // Bright head
    graphics_context_set_fill_color(ctx, GColorYellow);
    graphics_fill_circle(ctx, GPoint(sx, sy), 2);
#else
    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_draw_line(ctx, GPoint(sx, sy), GPoint(sx + tail_len, sy - tail_len / 2));
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_circle(ctx, GPoint(sx, sy), 1);
#endif
}

// Draw complication bar at bottom
static void draw_complication_bar(GContext *ctx, GRect bounds) {
    int bar_h = bounds.size.h * 18 / 100;
    int bar_y = bounds.size.h - bar_h;

#ifdef PBL_COLOR
    // Semi-dark bar
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, GRect(0, bar_y, bounds.size.w, bar_h), 0, GCornerNone);

    // Thin accent line at top of bar
    int phase = get_phase(s_hour);
    GColor accent = (phase == PHASE_NIGHT) ? GColorVividCerulean :
                    (phase == PHASE_DUSK)  ? GColorOrange : GColorMayGreen;
    graphics_context_set_stroke_color(ctx, accent);
    graphics_context_set_stroke_width(ctx, 1);
    graphics_draw_line(ctx, GPoint(4, bar_y), GPoint(bounds.size.w - 4, bar_y));


#else
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, GRect(0, bar_y, bounds.size.w, bar_h), 0, GCornerNone);
    // Border line
    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_draw_line(ctx, GPoint(0, bar_y), GPoint(bounds.size.w, bar_y));
#endif
}

// ───────── Master Draw Callback ─────────
// Battery indicator (top-right corner)
static void draw_battery(GContext *ctx, GRect bounds) {
    BatteryChargeState state = battery_state_service_peek();
    int pct = state.charge_percent;
    int bx = bounds.size.w - 20, by = 4;
    int bw = 14, bh = 7;
    // Outline
#ifdef PBL_COLOR
    graphics_context_set_stroke_color(ctx, GColorWhite);
#else
    graphics_context_set_stroke_color(ctx, (get_phase(s_hour) == PHASE_NIGHT) ? GColorWhite : GColorBlack);
#endif
    graphics_context_set_stroke_width(ctx, 1);
    graphics_draw_rect(ctx, GRect(bx, by, bw, bh));
    // Tip
#ifdef PBL_COLOR
    graphics_context_set_fill_color(ctx, GColorWhite);
#else
    graphics_context_set_fill_color(ctx, (get_phase(s_hour) == PHASE_NIGHT) ? GColorWhite : GColorBlack);
#endif
    graphics_fill_rect(ctx, GRect(bx + bw, by + 2, 2, 3), 0, GCornerNone);
    // Fill — red below 20%
#ifdef PBL_COLOR
    GColor fill_c = (pct <= 20) ? GColorRed : (state.is_charging ? GColorGreen : GColorYellow);
#else
    GColor fill_c = (get_phase(s_hour) == PHASE_NIGHT) ? GColorWhite : GColorBlack;
#endif
    int fill_w = (bw - 2) * pct / 100;
    if (fill_w < 1) fill_w = 1;
    graphics_context_set_fill_color(ctx, fill_c);
    graphics_fill_rect(ctx, GRect(bx + 1, by + 1, fill_w, bh - 2), 0, GCornerNone);
}

// Bluetooth disconnect indicator (top-left corner)
static void draw_bt_indicator(GContext *ctx, GRect bounds) {
    if (connection_service_peek_pebble_app_connection()) return;
    // Draw a small X in top-left
#ifdef PBL_COLOR
    graphics_context_set_stroke_color(ctx, GColorWhite);
#else
    graphics_context_set_stroke_color(ctx, (get_phase(s_hour) == PHASE_NIGHT) ? GColorWhite : GColorBlack);
#endif
    graphics_context_set_stroke_width(ctx, 1);
    graphics_draw_line(ctx, GPoint(4, 4), GPoint(10, 10));
    graphics_draw_line(ctx, GPoint(10, 4), GPoint(4, 10));
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);

    // 1. Sky background
    draw_sky(ctx, bounds);

    // 2. Stars (night/dusk only)
    draw_stars(ctx, bounds);

    // 3. Clouds
    draw_clouds(ctx, bounds);

    // 4. Celestial body (sun/moon) with HR pulse
    draw_celestial(ctx, bounds);

    // 5. Weather particles
    draw_particles(ctx, bounds);

    // 6. Shooting star
    draw_shooting_star(ctx, bounds);

    // 7. Mountains
    draw_mountains(ctx, bounds);

    // 8. Terrain / ground
    draw_terrain(ctx, bounds);

    // 9. Flowers (step-driven)
    draw_flowers(ctx, bounds);


    // 10. Complication bar
    draw_complication_bar(ctx, bounds);

    // 11. Battery indicator
    draw_battery(ctx, bounds);

    // 12. Bluetooth disconnect indicator
    draw_bt_indicator(ctx, bounds);
}

// ───────── Animation Timer ─────────
static void anim_timer_callback(void *data) {
    s_anim_frame++;
    s_anim_timer = NULL;

    if (s_shooting_star_active) {
        s_star_frame++;
        if (s_star_frame >= ANIM_TOTAL_FRAMES) {
            s_shooting_star_active = false;
            s_star_frame = 0;
            s_animating = false;
            layer_mark_dirty(s_canvas);
        }
    }

    if (s_animating) {
        layer_mark_dirty(s_canvas);
        s_anim_timer = app_timer_register(ANIM_DURATION_MS, anim_timer_callback, NULL);
    }
}

// ───────── Tap Handler (shooting star!) ─────────
static void tap_handler(AccelAxisType axis, int32_t direction) {
    if (s_animating) return;
    s_shooting_star_active = true;
    s_star_frame = 0;
    s_animating = true;
    s_anim_timer = app_timer_register(ANIM_DURATION_MS, anim_timer_callback, NULL);
}

// ───────── Time Update ─────────
static void update_time(struct tm *tick_time) {
    s_hour   = 12;
    s_minute = 0;
    s_month  = 5;

    // Time string
    if (clock_is_24h_style()) {
        strftime(s_time_buf, sizeof(s_time_buf), "%H:%M", tick_time);
    } else {
        strftime(s_time_buf, sizeof(s_time_buf), "%I:%M", tick_time);
        // Remove leading zero
        if (s_time_buf[0] == '0') {
            memmove(s_time_buf, s_time_buf + 1, strlen(s_time_buf));
        }
    }
    text_layer_set_text(s_time_layer, s_time_buf);

    // Date string
    strftime(s_date_buf, sizeof(s_date_buf), "%a %b %d", tick_time);
    text_layer_set_text(s_date_layer, s_date_buf);

    // White on dark backgrounds, black on light backgrounds
    int phase = get_phase(s_hour);
    GColor text_color = (phase == PHASE_NIGHT) ? GColorWhite : GColorBlack;
    text_layer_set_text_color(s_time_layer, text_color);
    text_layer_set_text_color(s_date_layer, text_color);
}

// ───────── Tick Handler ─────────
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    update_time(tick_time);

    s_frame_counter = (s_frame_counter + 1) % 10000;

    // Update particle positions deterministically
    GRect bounds = layer_get_bounds(window_get_root_layer(s_window));
    init_particles(bounds);

    // Refresh health
    update_health();

    // Request weather every WEATHER_REFRESH_MIN minutes
    if (tick_time->tm_min % WEATHER_REFRESH_MIN == 0) {
        DictionaryIterator *iter;
        AppMessageResult result = app_message_outbox_begin(&iter);
        if (result == APP_MSG_OK) {
            dict_write_uint8(iter, MESSAGE_KEY_REQUEST_WEATHER, 1);
            app_message_outbox_send();
        }
    }

    layer_mark_dirty(s_canvas);
}

// ───────── Weather AppMessage ─────────
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
    Tuple *temp_tuple = dict_find(iterator, MESSAGE_KEY_TEMPERATURE);
    Tuple *cond_tuple = dict_find(iterator, MESSAGE_KEY_CONDITIONS);
    Tuple *high_tuple = dict_find(iterator, MESSAGE_KEY_HIGH_TEMP);
    Tuple *low_tuple  = dict_find(iterator, MESSAGE_KEY_LOW_TEMP);
    Tuple *wind_tuple = dict_find(iterator, MESSAGE_KEY_WIND_SPEED);
    Tuple *wdir_tuple = dict_find(iterator, MESSAGE_KEY_WIND_DIR);
    Tuple *sr_tuple   = dict_find(iterator, MESSAGE_KEY_SUNRISE_MINS);
    Tuple *ss_tuple   = dict_find(iterator, MESSAGE_KEY_SUNSET_MINS);

    if (temp_tuple) { s_temperature = (int)temp_tuple->value->int32; s_weather_valid = true; }
    if (high_tuple) s_high_temp = (int)high_tuple->value->int32;
    if (low_tuple)  s_low_temp  = (int)low_tuple->value->int32;
    if (wind_tuple) s_wind_speed = (int)wind_tuple->value->int32;
    if (wdir_tuple) snprintf(s_wind_dir, sizeof(s_wind_dir), "%s", wdir_tuple->value->cstring);
    if (sr_tuple)   s_sunrise_mins = (int)sr_tuple->value->int32;
    if (ss_tuple)   s_sunset_mins  = (int)ss_tuple->value->int32;

    if (s_weather_valid) {
        snprintf(s_temp_buf, sizeof(s_temp_buf), "%d° H:%d L:%d %s %dmph",
                 s_temperature, s_high_temp, s_low_temp, s_wind_dir, s_wind_speed);
        text_layer_set_text(s_temp_layer, s_temp_buf);
    }

    if (cond_tuple) {
        const char *c = cond_tuple->value->cstring;
        if (strcmp(c, "Clear") == 0)           s_weather_code = WEATHER_CLEAR;
        else if (strcmp(c, "Cloudy") == 0)      s_weather_code = WEATHER_CLOUDY;
        else if (strcmp(c, "Rain") == 0)        s_weather_code = WEATHER_RAIN;
        else if (strcmp(c, "Showers") == 0)     s_weather_code = WEATHER_RAIN;
        else if (strcmp(c, "Snow") == 0)        s_weather_code = WEATHER_SNOW;
        else if (strcmp(c, "Fog") == 0)         s_weather_code = WEATHER_FOG;
        else if (strcmp(c, "T-Storm") == 0)     s_weather_code = WEATHER_STORM;
        else                                    s_weather_code = WEATHER_CLOUDY;
    }

    layer_mark_dirty(s_canvas);
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped: %d", reason);
}

// ───────── Window Lifecycle ─────────
static void window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    // Canvas layer (full screen)
    s_canvas = layer_create(bounds);
    layer_set_update_proc(s_canvas, canvas_update_proc);
    layer_add_child(window_layer, s_canvas);

    // ── Layout calculations ──
    int compl_bar_y = bounds.size.h - (bounds.size.h * 18 / 100);
    int sky_h = bounds.size.h * 60 / 100;

    // Time: in lower portion of sky, below the moon arc path
    int time_h = 42;
    int time_y = sky_h * 2 / 3 - time_h / 2;
    s_time_layer = text_layer_create(GRect(0, time_y, bounds.size.w, time_h + 6));
    text_layer_set_background_color(s_time_layer, GColorClear);
#ifdef PBL_COLOR
    text_layer_set_text_color(s_time_layer, GColorWhite);
#else
    text_layer_set_text_color(s_time_layer, GColorBlack);
#endif
    text_layer_set_font(s_time_layer,
        fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS));
    text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

    // Date: just below time
    int date_y = time_y + time_h + 2;
    s_date_layer = text_layer_create(GRect(0, date_y, bounds.size.w, 22));
    text_layer_set_background_color(s_date_layer, GColorClear);
#ifdef PBL_COLOR
    text_layer_set_text_color(s_date_layer, GColorWhite);
#else
    text_layer_set_text_color(s_date_layer, GColorBlack);
#endif
    text_layer_set_font(s_date_layer,
        fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(s_date_layer));

    // Temperature/wind bar and optional HR row
#if defined(PBL_PLATFORM_EMERY)
    s_has_hr = (health_service_metric_accessible(HealthMetricHeartRateBPM,
                   time_start_of_today(), time_start_of_today() + SECONDS_PER_DAY)
                   == HealthServiceAccessibilityMaskAvailable);
#endif
    int comp_text_y = s_has_hr ? compl_bar_y + 2 : compl_bar_y + 8;
    s_temp_layer = text_layer_create(GRect(0, comp_text_y, bounds.size.w, 14));
    text_layer_set_background_color(s_temp_layer, GColorClear);
    text_layer_set_text_color(s_temp_layer, GColorWhite);
    text_layer_set_font(s_temp_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    text_layer_set_text_alignment(s_temp_layer, GTextAlignmentCenter);
    text_layer_set_text(s_temp_layer, "--° H:-- L:-- -- --mph");
    layer_add_child(window_layer, text_layer_get_layer(s_temp_layer));
#if defined(PBL_PLATFORM_EMERY)
    if (s_has_hr) {
        s_hr_layer = text_layer_create(GRect(0, comp_text_y + 15, bounds.size.w, 14));
        text_layer_set_background_color(s_hr_layer, GColorClear);
#ifdef PBL_COLOR
        text_layer_set_text_color(s_hr_layer, GColorMelon);
#else
        text_layer_set_text_color(s_hr_layer, GColorWhite);
#endif
        text_layer_set_font(s_hr_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
        text_layer_set_text_alignment(s_hr_layer, GTextAlignmentCenter);
        text_layer_set_text(s_hr_layer, "-- bpm");
        layer_add_child(window_layer, text_layer_get_layer(s_hr_layer));
    }
#endif




    // ── Initialize scene ──
    init_stars(bounds);
    init_particles(bounds);
    init_flowers(bounds);
    update_health();

    // Set initial time
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    update_time(t);
}

static void window_unload(Window *window) {
    layer_destroy(s_canvas);
    text_layer_destroy(s_time_layer);
    text_layer_destroy(s_date_layer);
    text_layer_destroy(s_temp_layer);
#if defined(PBL_PLATFORM_EMERY)
    if (s_hr_layer) text_layer_destroy(s_hr_layer);
#endif


    if (s_anim_timer) {
        app_timer_cancel(s_anim_timer);
        s_anim_timer = NULL;
    }
}

// ───────── App Lifecycle ─────────
static void init(void) {
    s_window = window_create();
    window_set_background_color(s_window, GColorBlack);
    window_set_window_handlers(s_window, (WindowHandlers) {
        .load   = window_load,
        .unload = window_unload,
    });
    window_stack_push(s_window, true);

    // Tick timer — MINUTE_UNIT for battery efficiency
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

    // Tap for shooting star
    accel_tap_service_subscribe(tap_handler);

    // AppMessage for weather
    app_message_register_inbox_received(inbox_received_callback);
    app_message_register_inbox_dropped(inbox_dropped_callback);
    app_message_open(512, 64);
}

static void deinit(void) {
    accel_tap_service_unsubscribe();
    tick_timer_service_unsubscribe();
    window_destroy(s_window);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}
