#include "clockgears_boot_splash.h"

#include "core/clockgears_version.h"
#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/math/math_funcs.h"
#include "core/os/mutex.h"
#include "core/os/os.h"
#include "core/os/thread.h"
#include "main/app_icon.gen.h"
#include "modules/modules_enabled.gen.h"
#include "servers/display/display_server.h"
#include "servers/rendering/rendering_server.h"

#ifdef WINDOWS_ENABLED
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#ifdef MODULE_CLOCKGEARS_ENABLED
#include "modules/clockgears/editor/clockgears_theme.h"
#endif

#ifdef TOOLS_ENABLED
#include "editor/themes/builtin_fonts.gen.h"
#else
#include "scene/theme/default_font.gen.h"
#endif

#include <ft2build.h>
#include FT_FREETYPE_H

#include <cmath>
#include <cstring>
#include <atomic>
#include <mutex>
#include <thread>

namespace {

inline float _clamp01(float v) {
	return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}

inline void _blend_pixel_rgba(uint8_t *p_dst, int p_w, int p_h, int p_x, int p_y, const Color &p_col, float p_alpha_mul = 1.0f) {
	if (p_x < 0 || p_y < 0 || p_x >= p_w || p_y >= p_h) {
		return;
	}
	const float sa = _clamp01(p_col.a * p_alpha_mul);
	if (sa <= 0.001f) {
		return;
	}
	uint8_t *px = p_dst + ((p_y * p_w + p_x) * 4);
	const float inv = 1.0f - sa;
	px[0] = (uint8_t)CLAMP((int)Math::round(px[0] * inv + (p_col.r * 255.0f) * sa), 0, 255);
	px[1] = (uint8_t)CLAMP((int)Math::round(px[1] * inv + (p_col.g * 255.0f) * sa), 0, 255);
	px[2] = (uint8_t)CLAMP((int)Math::round(px[2] * inv + (p_col.b * 255.0f) * sa), 0, 255);
	px[3] = (uint8_t)CLAMP((int)Math::round(MAX((float)px[3], sa * 255.0f)), 0, 255);
}

inline void _blend_pixel_additive_rgba(uint8_t *p_dst, int p_w, int p_h, int p_x, int p_y, const Color &p_col, float p_intensity = 1.0f) {
	if (p_x < 0 || p_y < 0 || p_x >= p_w || p_y >= p_h) {
		return;
	}
	const float sa = _clamp01(p_col.a * p_intensity);
	if (sa <= 0.001f) {
		return;
	}
	uint8_t *px = p_dst + ((p_y * p_w + p_x) * 4);
	const int add_r = (int)Math::round(p_col.r * 255.0f * sa);
	const int add_g = (int)Math::round(p_col.g * 255.0f * sa);
	const int add_b = (int)Math::round(p_col.b * 255.0f * sa);
	px[0] = (uint8_t)CLAMP((int)px[0] + add_r, 0, 255);
	px[1] = (uint8_t)CLAMP((int)px[1] + add_g, 0, 255);
	px[2] = (uint8_t)CLAMP((int)px[2] + add_b, 0, 255);
	px[3] = 255;
}

// Sprites de degradado radial y destello precalculados una sola vez (64x64) con mezcla aditiva (sin desenfoque ni shaders en tiempo real).
static constexpr int CG_GLOW_LUT_SIZE = 64;
static float cg_radial_glow_lut[CG_GLOW_LUT_SIZE * CG_GLOW_LUT_SIZE];
static float cg_head_flare_lut[CG_GLOW_LUT_SIZE * CG_GLOW_LUT_SIZE];
static bool cg_glow_luts_ready = false;

static void _ensure_glow_luts_ready() {
	if (cg_glow_luts_ready) {
		return;
	}
	const float half = (float)CG_GLOW_LUT_SIZE * 0.5f;
	for (int y = 0; y < CG_GLOW_LUT_SIZE; y++) {
		const float ny = ((float)y + 0.5f - half) / half;
		for (int x = 0; x < CG_GLOW_LUT_SIZE; x++) {
			const float nx = ((float)x + 0.5f - half) / half;
			const float r2 = nx * nx + ny * ny;
			const float r = sqrtf(r2);
			float radial = 0.0f;
			if (r < 1.0f) {
				const float u = 1.0f - r;
				radial = u * u * (3.0f - 2.0f * u) * expf(-1.85f * r2);
			}
			// Destello cruzado suave en la cabeza de avance
			const float ax = expf(-fabsf(ny) * 8.5f) * _clamp01(1.0f - fabsf(nx));
			const float ay = expf(-fabsf(nx) * 7.0f) * _clamp01(1.0f - fabsf(ny));
			const float core = (r < 1.0f) ? expf(-3.4f * r2) : 0.0f;
			cg_radial_glow_lut[y * CG_GLOW_LUT_SIZE + x] = _clamp01(radial);
			cg_head_flare_lut[y * CG_GLOW_LUT_SIZE + x] = _clamp01(core + (ax * 0.55f + ay * 0.45f) * _clamp01(1.0f - r * 0.85f));
		}
	}
	cg_glow_luts_ready = true;
}

static void _stamp_additive_glow_sprite(uint8_t *p_dst, int p_w, int p_h, float p_cx, float p_cy, float p_rx, float p_ry, const Color &p_col, float p_intensity, bool p_use_flare) {
	if (p_rx < 1.0f || p_ry < 1.0f || p_intensity <= 0.001f) {
		return;
	}
	_ensure_glow_luts_ready();
	const float *lut = p_use_flare ? cg_head_flare_lut : cg_radial_glow_lut;
	const int x0 = MAX(0, (int)floorf(p_cx - p_rx));
	const int y0 = MAX(0, (int)floorf(p_cy - p_ry));
	const int x1 = MIN(p_w - 1, (int)ceilf(p_cx + p_rx));
	const int y1 = MIN(p_h - 1, (int)ceilf(p_cy + p_ry));
	const float inv_rx = 1.0f / p_rx;
	const float inv_ry = 1.0f / p_ry;
	for (int y = y0; y <= y1; y++) {
		const float ny = ((float)y + 0.5f - p_cy) * inv_ry;
		if (ny <= -1.0f || ny >= 1.0f) {
			continue;
		}
		const int ly = CLAMP((int)((ny * 0.5f + 0.5f) * (float)(CG_GLOW_LUT_SIZE - 1)), 0, CG_GLOW_LUT_SIZE - 1);
		for (int x = x0; x <= x1; x++) {
			const float nx = ((float)x + 0.5f - p_cx) * inv_rx;
			if (nx <= -1.0f || nx >= 1.0f) {
				continue;
			}
			const int lx = CLAMP((int)((nx * 0.5f + 0.5f) * (float)(CG_GLOW_LUT_SIZE - 1)), 0, CG_GLOW_LUT_SIZE - 1);
			const float w = lut[ly * CG_GLOW_LUT_SIZE + lx] * p_intensity;
			if (w > 0.002f) {
				_blend_pixel_additive_rgba(p_dst, p_w, p_h, x, y, p_col, w);
			}
		}
	}
}

float _sd_rounded_box(float px, float py, float bx, float by, float bw, float bh, float r) {
	const float cx = bx + bw * 0.5f;
	const float cy = by + bh * 0.5f;
	const float hx = bw * 0.5f - r;
	const float hy = bh * 0.5f - r;
	const float qx = fabsf(px - cx) - hx;
	const float qy = fabsf(py - cy) - hy;
	const float ox = MAX(qx, 0.0f);
	const float oy = MAX(qy, 0.0f);
	return sqrtf(ox * ox + oy * oy) + MIN(MAX(qx, qy), 0.0f) - r;
}

float _dist_segment(float px, float py, float ax, float ay, float bx, float by) {
	const float vx = bx - ax;
	const float vy = by - ay;
	const float wx = px - ax;
	const float wy = py - ay;
	const float len2 = vx * vx + vy * vy;
	const float t = len2 > 1e-6f ? _clamp01((wx * vx + wy * vy) / len2) : 0.0f;
	const float dx = px - (ax + t * vx);
	const float dy = py - (ay + t * vy);
	return sqrtf(dx * dx + dy * dy);
}

void _draw_aa_line(uint8_t *p_dst, int p_w, int p_h, float ax, float ay, float bx, float by, float p_thickness, const Color &p_col) {
	const int x0 = MAX(0, (int)floorf(MIN(ax, bx) - p_thickness - 2.0f));
	const int y0 = MAX(0, (int)floorf(MIN(ay, by) - p_thickness - 2.0f));
	const int x1 = MIN(p_w - 1, (int)ceilf(MAX(ax, bx) + p_thickness + 2.0f));
	const int y1 = MIN(p_h - 1, (int)ceilf(MAX(ay, by) + p_thickness + 2.0f));
	const float half_t = p_thickness * 0.5f;
	for (int y = y0; y <= y1; y++) {
		for (int x = x0; x <= x1; x++) {
			const float d = _dist_segment((float)x + 0.5f, (float)y + 0.5f, ax, ay, bx, by) - half_t;
			if (d < 1.0f) {
				const float cov = _clamp01(1.0f - d);
				_blend_pixel_rgba(p_dst, p_w, p_h, x, y, p_col, cov);
			}
		}
	}
}

void _draw_aa_ring(uint8_t *p_dst, int p_w, int p_h, float cx, float cy, float radius, float thickness, const Color &p_col) {
	const float half_t = thickness * 0.5f;
	const int x0 = MAX(0, (int)floorf(cx - radius - half_t - 2.0f));
	const int y0 = MAX(0, (int)floorf(cy - radius - half_t - 2.0f));
	const int x1 = MIN(p_w - 1, (int)ceilf(cx + radius + half_t + 2.0f));
	const int y1 = MIN(p_h - 1, (int)ceilf(cy + radius + half_t + 2.0f));
	for (int y = y0; y <= y1; y++) {
		for (int x = x0; x <= x1; x++) {
			const float dx = (float)x + 0.5f - cx;
			const float dy = (float)y + 0.5f - cy;
			const float d = fabsf(sqrtf(dx * dx + dy * dy) - radius) - half_t;
			if (d < 1.0f) {
				_blend_pixel_rgba(p_dst, p_w, p_h, x, y, p_col, _clamp01(1.0f - d));
			}
		}
	}
}

void _draw_aa_disc(uint8_t *p_dst, int p_w, int p_h, float cx, float cy, float radius, const Color &p_col) {
	const int x0 = MAX(0, (int)floorf(cx - radius - 2.0f));
	const int y0 = MAX(0, (int)floorf(cy - radius - 2.0f));
	const int x1 = MIN(p_w - 1, (int)ceilf(cx + radius + 2.0f));
	const int y1 = MIN(p_h - 1, (int)ceilf(cy + radius + 2.0f));
	for (int y = y0; y <= y1; y++) {
		for (int x = x0; x <= x1; x++) {
			const float dx = (float)x + 0.5f - cx;
			const float dy = (float)y + 0.5f - cy;
			const float d = sqrtf(dx * dx + dy * dy) - radius;
			if (d < 1.0f) {
				_blend_pixel_rgba(p_dst, p_w, p_h, x, y, p_col, _clamp01(1.0f - d));
			}
		}
	}
}

void _draw_ornament_gear(uint8_t *p_dst, int p_w, int p_h, float cx, float cy, float r_outer, float r_inner, int teeth, float phase_rad, float stroke_w, const Color &p_col, float p_fade_y_limit = 9999.0f) {
	const int x0 = MAX(0, (int)floorf(cx - r_outer - 3.0f));
	const int y0 = MAX(0, (int)floorf(cy - r_outer - 3.0f));
	const int x1 = MIN(p_w - 1, (int)ceilf(cx + r_outer + 3.0f));
	const int y1 = MIN(p_h - 1, (int)ceilf(cy + r_outer + 3.0f));
	const float r_root = r_outer * 0.84f;
	const float tooth_h = r_outer - r_root;
	const int spokes = 5;

	for (int y = y0; y <= y1; y++) {
		if ((float)y > p_fade_y_limit) {
			continue;
		}
		float edge_fade = 1.0f;
		if (p_fade_y_limit < 9000.0f && (float)y > p_fade_y_limit - 32.0f) {
			edge_fade = _clamp01((p_fade_y_limit - (float)y) / 32.0f);
		}
		for (int x = x0; x <= x1; x++) {
			const float dx = (float)x + 0.5f - cx;
			const float dy = (float)y + 0.5f - cy;
			const float dist = sqrtf(dx * dx + dy * dy);
			if (dist > r_outer + 2.0f || dist < r_inner - stroke_w - 2.0f) {
				continue;
			}
			const float ang = atan2f(dy, dx) + phase_rad;
			const float s = sinf(ang * (float)teeth);
			const float tooth_profile = _clamp01((s + 0.25f) * 2.4f);
			const float rim_r = r_root + tooth_h * tooth_profile;

			// Contorno dentado exterior + anillos internos + radios
			const float d_outer = fabsf(dist - rim_r) - stroke_w * 0.5f;
			const float d_mid = fabsf(dist - r_root * 0.72f) - stroke_w * 0.45f;
			const float d_hub = fabsf(dist - r_inner) - stroke_w * 0.45f;
			float d_spoke = 99.0f;
			if (dist > r_inner && dist < r_root * 0.72f) {
				const float spoke_s = fabsf(sinf(ang * (float)spokes * 0.5f));
				d_spoke = spoke_s * dist - stroke_w * 0.55f;
			}
			const float d_min = MIN(MIN(d_outer, d_mid), MIN(d_hub, d_spoke));
			if (d_min < 1.0f) {
				_blend_pixel_rgba(p_dst, p_w, p_h, x, y, p_col, _clamp01(1.0f - d_min) * edge_fade);
			}
		}
	}
}

void _draw_circuit_trace(uint8_t *p_dst, int p_w, int p_h, const Vector<Vector2> &p_pts, float p_thick, const Color &p_col, bool p_start_via, bool p_end_via) {
	for (int i = 0; i + 1 < p_pts.size(); i++) {
		_draw_aa_line(p_dst, p_w, p_h, p_pts[i].x, p_pts[i].y, p_pts[i + 1].x, p_pts[i + 1].y, p_thick, p_col);
	}
	if (p_start_via && !p_pts.is_empty()) {
		_draw_aa_ring(p_dst, p_w, p_h, p_pts[0].x, p_pts[0].y, 3.2f, 1.3f, Color(p_col.r, p_col.g, p_col.b, p_col.a * 1.35f));
	}
	if (p_end_via && p_pts.size() >= 2) {
		const Vector2 end_p = p_pts[p_pts.size() - 1];
		_draw_aa_ring(p_dst, p_w, p_h, end_p.x, end_p.y, 3.4f, 1.35f, Color(p_col.r, p_col.g, p_col.b, p_col.a * 1.45f));
	}
}

struct SplashFontCache {
	FT_Library library = nullptr;
	FT_Face face = nullptr;
	bool ready = false;

	bool ensure_ready() {
		if (ready) {
			return face != nullptr;
		}
		ready = true;
		if (FT_Init_FreeType(&library) != 0 || !library) {
			return false;
		}
#ifdef TOOLS_ENABLED
		const FT_Byte *font_ptr = (const FT_Byte *)_font_Inter_Regular;
		const FT_Long font_sz = (FT_Long)_font_Inter_Regular_size;
#else
		const FT_Byte *font_ptr = (const FT_Byte *)_font_OpenSans_SemiBold;
		const FT_Long font_sz = (FT_Long)_font_OpenSans_SemiBold_size;
#endif
		if (FT_New_Memory_Face(library, font_ptr, font_sz, 0, &face) != 0) {
			face = nullptr;
			return false;
		}
		FT_Select_Charmap(face, FT_ENCODING_UNICODE);
		return true;
	}

	int measure_text_width(const String &p_text, int p_px_size, float p_tracking) {
		if (!ensure_ready() || p_text.is_empty()) {
			return 0;
		}
		FT_Set_Pixel_Sizes(face, 0, (FT_UInt)MAX(8, p_px_size));
		float pen_x = 0.0f;
		FT_UInt prev_glyph = 0;
		const bool use_kerning = FT_HAS_KERNING(face) != 0;
		for (int i = 0; i < p_text.length(); i++) {
			const char32_t cp = p_text[i];
			const FT_UInt gidx = FT_Get_Char_Index(face, cp);
			if (use_kerning && prev_glyph && gidx) {
				FT_Vector kern;
				if (FT_Get_Kerning(face, prev_glyph, gidx, FT_KERNING_DEFAULT, &kern) == 0) {
					pen_x += (float)(kern.x >> 6);
				}
			}
			if (FT_Load_Glyph(face, gidx, FT_LOAD_DEFAULT) == 0) {
				pen_x += (float)(face->glyph->advance.x >> 6);
			}
			if (i + 1 < p_text.length()) {
				pen_x += p_tracking;
			}
			prev_glyph = gidx;
		}
		return (int)ceilf(pen_x);
	}

	Rect2i draw_text(uint8_t *p_dst, int p_w, int p_h, const String &p_text, int p_x, int p_baseline_y, int p_px_size, const Color &p_col, float p_tracking = 0.0f, bool p_faux_bold = false) {
		if (!ensure_ready() || p_text.is_empty()) {
			return Rect2i();
		}
		FT_Set_Pixel_Sizes(face, 0, (FT_UInt)MAX(8, p_px_size));
		float pen_x = (float)p_x;
		FT_UInt prev_glyph = 0;
		const bool use_kerning = FT_HAS_KERNING(face) != 0;
		int min_x = p_x;
		int min_y = p_baseline_y - p_px_size;
		int max_x = p_x;
		int max_y = p_baseline_y + 4;

		for (int i = 0; i < p_text.length(); i++) {
			const char32_t cp = p_text[i];
			const FT_UInt gidx = FT_Get_Char_Index(face, cp);
			if (use_kerning && prev_glyph && gidx) {
				FT_Vector kern;
				if (FT_Get_Kerning(face, prev_glyph, gidx, FT_KERNING_DEFAULT, &kern) == 0) {
					pen_x += (float)(kern.x >> 6);
				}
			}
			if (FT_Load_Glyph(face, gidx, FT_LOAD_RENDER | FT_LOAD_TARGET_LIGHT) == 0) {
				const FT_GlyphSlot slot = face->glyph;
				const FT_Bitmap &bmp = slot->bitmap;
				const int gx = (int)Math::round(pen_x) + slot->bitmap_left;
				const int gy = p_baseline_y - slot->bitmap_top;
				const int passes = p_faux_bold ? 2 : 1;
				for (int pass = 0; pass < passes; pass++) {
					const float pass_mul = (pass == 0) ? 1.0f : 0.60f;
					for (unsigned int row = 0; row < bmp.rows; row++) {
						const uint8_t *src_row = bmp.buffer + row * bmp.pitch;
						for (unsigned int col = 0; col < bmp.width; col++) {
							const float cov = (float)src_row[col] / 255.0f;
							if (cov > 0.005f) {
								_blend_pixel_rgba(p_dst, p_w, p_h, gx + (int)col + pass, gy + (int)row, p_col, cov * pass_mul);
							}
						}
					}
				}
				min_x = MIN(min_x, gx);
				min_y = MIN(min_y, gy);
				max_x = MAX(max_x, gx + (int)bmp.width + (p_faux_bold ? 1 : 0));
				max_y = MAX(max_y, gy + (int)bmp.rows);
				pen_x += (float)(slot->advance.x >> 6) + p_tracking;
			}
			prev_glyph = gidx;
		}
		return Rect2i(min_x, min_y, MAX(0, max_x - min_x), MAX(0, max_y - min_y));
	}
};

SplashFontCache &get_font_cache() {
	static SplashFontCache cache;
	return cache;
}

void _draw_filled_rounded_rect(uint8_t *p_dst, int p_w, int p_h, float rx, float ry, float rw, float rh, float radius, const Color &p_fill, const Color &p_border, float p_border_w = 1.0f) {
	const int x0 = MAX(0, (int)floorf(rx - 2.0f));
	const int y0 = MAX(0, (int)floorf(ry - 2.0f));
	const int x1 = MIN(p_w - 1, (int)ceilf(rx + rw + 2.0f));
	const int y1 = MIN(p_h - 1, (int)ceilf(ry + rh + 2.0f));
	for (int y = y0; y <= y1; y++) {
		for (int x = x0; x <= x1; x++) {
			const float d = _sd_rounded_box((float)x + 0.5f, (float)y + 0.5f, rx, ry, rw, rh, radius);
			if (d < 1.0f) {
				const float fill_cov = _clamp01(1.0f - d);
				_blend_pixel_rgba(p_dst, p_w, p_h, x, y, p_fill, fill_cov);
				if (p_border.a > 0.005f && p_border_w > 0.0f) {
					const float db = fabsf(d + p_border_w * 0.5f) - p_border_w * 0.5f;
					if (db < 1.0f) {
						_blend_pixel_rgba(p_dst, p_w, p_h, x, y, p_border, _clamp01(1.0f - db));
					}
				}
			}
		}
	}
}

void _draw_inner_copper_frame(uint8_t *p_dst, int p_w, int p_h, const Rect2i &p_frame, float p_radius, float p_border_w, const Color &p_col) {
	const float rx = (float)p_frame.position.x;
	const float ry = (float)p_frame.position.y;
	const float rw = (float)p_frame.size.x;
	const float rh = (float)p_frame.size.y;
	const int band = MAX(3, (int)ceilf(p_radius + p_border_w + 2.0f));
	const int x0 = MAX(0, p_frame.position.x - 2);
	const int y0 = MAX(0, p_frame.position.y - 2);
	const int x1 = MIN(p_w - 1, p_frame.get_end().x + 2);
	const int y1 = MIN(p_h - 1, p_frame.get_end().y + 2);

	for (int y = y0; y <= y1; y++) {
		const bool in_v_band = (y <= p_frame.position.y + band) || (y >= p_frame.get_end().y - band);
		for (int x = x0; x <= x1; x++) {
			if (!in_v_band && x > p_frame.position.x + band && x < p_frame.get_end().x - band) {
				x = p_frame.get_end().x - band - 1;
				continue;
			}
			const float d = _sd_rounded_box((float)x + 0.5f, (float)y + 0.5f, rx, ry, rw, rh, p_radius);
			const float db = fabsf(d + p_border_w * 0.5f) - p_border_w * 0.5f;
			if (db < 1.0f) {
				_blend_pixel_rgba(p_dst, p_w, p_h, x, y, p_col, _clamp01(1.0f - db));
			}
		}
	}
}

void _build_static_base(Vector<uint8_t> &r_buf, int W, int H, float p_dpi_scale, const ClockGearsBootSplash::Palette &p_pal, ClockGearsBootSplash::LayoutMetrics &r_metrics) {
	r_buf.resize(W * H * 4);
	uint8_t *dst = r_buf.ptrw();

	// Escala proporcional sobre una base de 1920x1080 (en pantallas 16:10 o ultrapanorámicas extiende el fondo sin estirar).
	const float s = MIN((float)W / (float)ClockGearsBootSplash::BASE_DESIGN_WIDTH, (float)H / (float)ClockGearsBootSplash::BASE_DESIGN_HEIGHT);
	r_metrics.layout_scale = s;
	r_metrics.card_rect = Rect2i(0, 0, W, H);

	// Tamaño del icono en múltiplo ENTERO de 64/128 px (128, 192 o 256 px según pantalla y escala de Windows) con filtro nearest.
	const float eff_h = (float)H * MAX(1.0f, p_dpi_scale * 0.85f);
	int icon_px = 128;
	if (H >= 1800 || eff_h >= 1950.0f) {
		icon_px = 256;
	} else if (H >= 1000) {
		icon_px = 192;
	}
	r_metrics.icon_scale_multiple = icon_px / 64;

	const int ix0 = (W - icon_px) / 2;
	const int iy0 = (int)Math::round((float)H * 0.20f);
	r_metrics.icon_rect = Rect2i(ix0, iy0, icon_px, icon_px);

	// 1. Fondo a pantalla completa con degradado vertical rápido y halo radial cálido centrado en el icono.
	const float halo_cx = (float)W * 0.5f;
	const float halo_cy = (float)iy0 + (float)icon_px * 0.5f;
	const float halo_r = 250.0f * s;
	const float halo_inv_r = 1.0f / MAX(32.0f, halo_r);
	const int hx0 = MAX(0, (int)floorf(halo_cx - halo_r * 1.55f));
	const int hx1 = MIN(W - 1, (int)ceilf(halo_cx + halo_r * 1.55f));
	const int hy0 = MAX(0, (int)floorf(halo_cy - halo_r * 1.55f));
	const int hy1 = MIN(H - 1, (int)ceilf(halo_cy + halo_r * 1.55f));

	for (int y = 0; y < H; y++) {
		const float vt = (float)y / (float)MAX(1, H - 1);
		const Color row_bg = p_pal.bg_top.lerp(p_pal.bg_bottom, vt);
		const uint8_t br = (uint8_t)Math::round(_clamp01(row_bg.r) * 255.0f);
		const uint8_t bg = (uint8_t)Math::round(_clamp01(row_bg.g) * 255.0f);
		const uint8_t bb = (uint8_t)Math::round(_clamp01(row_bg.b) * 255.0f);
		const uint32_t row_packed = (uint32_t)br | ((uint32_t)bg << 8) | ((uint32_t)bb << 16) | 0xFF000000u;

		uint32_t *row_ptr32 = reinterpret_cast<uint32_t *>(dst + (size_t)y * W * 4);
		for (int x = 0; x < W; x++) {
			row_ptr32[x] = row_packed;
		}
		if (y >= hy0 && y <= hy1) {
			const float dy = ((float)y + 0.5f - halo_cy) * halo_inv_r;
			const float dy2 = dy * dy;
			for (int x = hx0; x <= hx1; x++) {
				const float dx = ((float)x + 0.5f - halo_cx) * halo_inv_r;
				const float d2 = dx * dx + dy2;
				if (d2 < 2.35f) {
					const float halo = expf(-2.45f * d2) * p_pal.halo_amber.a;
					uint8_t *px = dst + (((size_t)y * W + x) * 4);
					px[0] = (uint8_t)Math::round(_clamp01(row_bg.r + p_pal.halo_amber.r * halo) * 255.0f);
					px[1] = (uint8_t)Math::round(_clamp01(row_bg.g + p_pal.halo_amber.g * halo) * 255.0f);
					px[2] = (uint8_t)Math::round(_clamp01(row_bg.b + p_pal.halo_amber.b * halo) * 255.0f);
				}
			}
		}
	}

	// Anillos concéntricos tenues alrededor del halo del icono.
	const float ring_th = MAX(1.0f, 1.3f * s);
	_draw_aa_ring(dst, W, H, halo_cx, halo_cy, (float)icon_px * 0.72f, ring_th, Color(p_pal.accent_amber.r, p_pal.accent_amber.g, p_pal.accent_amber.b, 0.18f));
	_draw_aa_ring(dst, W, H, halo_cx, halo_cy, (float)icon_px * 0.98f, ring_th, Color(p_pal.accent_amber.r, p_pal.accent_amber.g, p_pal.accent_amber.b, 0.09f));
	_draw_aa_ring(dst, W, H, halo_cx, halo_cy, (float)icon_px * 1.24f, ring_th, Color(p_pal.accent_amber.r, p_pal.accent_amber.g, p_pal.accent_amber.b, 0.045f));

	// 2. Engranajes y trazos de circuito más grandes y numerosos anclados a las esquinas y los bordes.
	const Color orn_soft = p_pal.ornament_copper;
	const Color orn_mid(orn_soft.r, orn_soft.g, orn_soft.b, orn_soft.a * 0.80f);
	const Color orn_subtle(orn_soft.r, orn_soft.g, orn_soft.b, orn_soft.a * 0.68f);
	const float stroke_main = MAX(1.1f, 1.8f * s);
	const float stroke_sub = MAX(1.0f, 1.45f * s);

	// 8 engranajes en las 4 esquinas y 4 bordes.
	_draw_ornament_gear(dst, W, H, 76.0f * s, 76.0f * s, 155.0f * s, 42.0f * s, 14, 0.22f, stroke_main, orn_soft);
	_draw_ornament_gear(dst, W, H, 242.0f * s, 56.0f * s, 74.0f * s, 20.0f * s, 10, 0.55f, stroke_sub, orn_mid);

	_draw_ornament_gear(dst, W, H, (float)W - 82.0f * s, 82.0f * s, 144.0f * s, 38.0f * s, 12, 0.65f, stroke_main, orn_mid);
	_draw_ornament_gear(dst, W, H, (float)W - 60.0f * s, 238.0f * s, 70.0f * s, 18.0f * s, 10, 0.15f, stroke_sub, orn_subtle);

	_draw_ornament_gear(dst, W, H, 80.0f * s, (float)H - 80.0f * s, 140.0f * s, 36.0f * s, 12, 0.40f, stroke_main, orn_mid);
	_draw_ornament_gear(dst, W, H, 28.0f * s, (float)H * 0.48f, 86.0f * s, 22.0f * s, 12, 0.10f, stroke_sub, orn_subtle);

	_draw_ornament_gear(dst, W, H, (float)W - 80.0f * s, (float)H - 80.0f * s, 140.0f * s, 36.0f * s, 12, 0.30f, stroke_main, orn_mid);
	_draw_ornament_gear(dst, W, H, (float)W - 28.0f * s, (float)H * 0.52f, 86.0f * s, 22.0f * s, 12, 0.50f, stroke_sub, orn_subtle);
	r_metrics.ornament_gear_count = 8;

	// 10 trazos de circuito anclados a esquinas y bordes laterales/superiores/inferiores.
	_draw_circuit_trace(dst, W, H, { Vector2(210 * s, 54 * s), Vector2(360 * s, 54 * s), Vector2(402 * s, 96 * s), Vector2(530 * s, 96 * s) }, stroke_sub, orn_subtle, false, true);
	_draw_circuit_trace(dst, W, H, { Vector2(54 * s, 215 * s), Vector2(54 * s, 340 * s), Vector2(96 * s, 382 * s), Vector2(96 * s, 470 * s) }, stroke_sub, orn_subtle, false, true);
	_draw_circuit_trace(dst, W, H, { Vector2(142 * s, 152 * s), Vector2(236 * s, 152 * s), Vector2(276 * s, 192 * s), Vector2(368 * s, 192 * s) }, stroke_sub, orn_subtle, true, true);

	_draw_circuit_trace(dst, W, H, { Vector2(W - 216 * s, 60 * s), Vector2(W - 368 * s, 60 * s), Vector2(W - 410 * s, 102 * s), Vector2(W - 535 * s, 102 * s) }, stroke_sub, orn_subtle, false, true);
	_draw_circuit_trace(dst, W, H, { Vector2(W - 54 * s, 220 * s), Vector2(W - 54 * s, 348 * s), Vector2(W - 96 * s, 390 * s), Vector2(W - 96 * s, 475 * s) }, stroke_sub, orn_subtle, false, true);

	_draw_circuit_trace(dst, W, H, { Vector2(96 * s, H * 0.56f), Vector2(178 * s, H * 0.56f), Vector2(214 * s, H * 0.60f), Vector2(310 * s, H * 0.60f) }, stroke_sub, orn_subtle, true, true);
	_draw_circuit_trace(dst, W, H, { Vector2(W - 96 * s, H * 0.56f), Vector2(W - 178 * s, H * 0.56f), Vector2(W - 214 * s, H * 0.60f), Vector2(W - 310 * s, H * 0.60f) }, stroke_sub, orn_subtle, true, true);

	_draw_circuit_trace(dst, W, H, { Vector2(198 * s, H - 72 * s), Vector2(348 * s, H - 72 * s), Vector2(388 * s, H - 112 * s), Vector2(492 * s, H - 112 * s) }, stroke_sub, orn_subtle, true, true);
	_draw_circuit_trace(dst, W, H, { Vector2(68 * s, H - 205 * s), Vector2(68 * s, H - 305 * s), Vector2(112 * s, H - 349 * s), Vector2(196 * s, H - 349 * s) }, stroke_sub, orn_subtle, false, true);
	_draw_circuit_trace(dst, W, H, { Vector2(W - 198 * s, H - 72 * s), Vector2(W - 348 * s, H - 72 * s), Vector2(W - 388 * s, H - 112 * s), Vector2(W - 492 * s, H - 112 * s) }, stroke_sub, orn_subtle, true, true);
	r_metrics.circuit_trace_count = 10;

	// 3. Marco cobrizo fino interior separado del borde de la pantalla (24 px a 1080p, con esquinas suaves).
	const int frame_inset = MAX(10, (int)Math::round(24.0f * s));
	const int frame_corner = MAX(4, (int)Math::round((float)p_pal.corner_radius * s));
	const float frame_bw = MAX(1.0f, Math::round((float)p_pal.border_width * s));
	r_metrics.frame_inset = frame_inset;
	r_metrics.corner_radius = p_pal.corner_radius;
	r_metrics.inner_frame_rect = Rect2i(frame_inset, frame_inset, W - frame_inset * 2, H - frame_inset * 2);
	_draw_inner_copper_frame(dst, W, H, r_metrics.inner_frame_rect, (float)frame_corner, frame_bw, p_pal.card_border);

	// 4. Icono central en múltiplo entero con filtro nearest.
	Ref<Image> icon = memnew(Image(app_icon_png));
	if (icon.is_valid() && !icon->is_empty()) {
		if (icon->get_format() != Image::FORMAT_RGBA8) {
			icon->convert(Image::FORMAT_RGBA8);
		}
		if (icon->get_width() != icon_px || icon->get_height() != icon_px) {
			icon->resize(icon_px, icon_px, Image::INTERPOLATE_NEAREST);
		}
		const uint8_t *isrc = icon->get_data().ptr();
		for (int sy = 0; sy < icon_px; sy++) {
			for (int sx = 0; sx < icon_px; sx++) {
				const uint8_t *sp = isrc + (((size_t)sy * icon_px + sx) * 4);
				if (sp[3] > 0) {
					const Color c((float)sp[0] / 255.0f, (float)sp[1] / 255.0f, (float)sp[2] / 255.0f, (float)sp[3] / 255.0f);
					_blend_pixel_rgba(dst, W, H, ix0 + sx, iy0 + sy, c, 1.0f);
				}
			}
		}
	}

	// 5. Tipografías escaladas directamente en tamaño entero de píxel (nunca desenfocadas).
	SplashFontCache &fc = get_font_cache();

	// Etiqueta "v0.0.4 ALPHA" arriba a la derecha dentro del marco interior.
	const String ver_txt = String::utf8(CLOCKGEARS_SPLASH_VERSION);
	const int ver_font_px = MAX(11, (int)Math::round(15.0f * s));
	const float ver_track = 1.0f * s;
	const int ver_w = fc.measure_text_width(ver_txt, ver_font_px, ver_track);
	const int pill_pad_x = MAX(10, (int)Math::round(16.0f * s));
	const int pill_w = ver_w + pill_pad_x * 2;
	const int pill_h = MAX(22, (int)Math::round(32.0f * s));
	const int pill_margin = MAX(14, (int)Math::round(24.0f * s));
	const int pill_x = r_metrics.inner_frame_rect.get_end().x - pill_margin - pill_w;
	const int pill_y = r_metrics.inner_frame_rect.position.y + pill_margin;
	r_metrics.badge_rect = Rect2i(pill_x, pill_y, pill_w, pill_h);
	_draw_filled_rounded_rect(dst, W, H, (float)pill_x, (float)pill_y, (float)pill_w, (float)pill_h, MAX(4.0f, 5.0f * s),
			Color(p_pal.badge_bg.r, p_pal.badge_bg.g, p_pal.badge_bg.b, 0.90f),
			Color(p_pal.card_border.r, p_pal.card_border.g, p_pal.card_border.b, 0.68f), frame_bw);
	const int ver_baseline = pill_y + (pill_h + ver_font_px) / 2 - MAX(1, (int)Math::round(1.5f * s));
	(void)fc.draw_text(dst, W, H, ver_txt, pill_x + pill_pad_x, ver_baseline, ver_font_px, p_pal.accent_amber, ver_track, true);

	// Títulos y créditos centrados debajo del icono.
	const String title_txt = String::utf8("CLOCKGEARS ENGINE");
	const int title_font_px = MAX(22, (int)Math::round(42.0f * s));
	const float title_track = 2.8f * s;
	const int title_w = fc.measure_text_width(title_txt, title_font_px, title_track);
	const int title_y = iy0 + icon_px + MAX(34, (int)Math::round(58.0f * s));
	r_metrics.title_rect = fc.draw_text(dst, W, H, title_txt, (W - title_w) / 2, title_y, title_font_px, p_pal.accent_amber, title_track, true);

	const String sub_txt = String::utf8("NEXT-GEN RPG CREATION SYSTEM");
	const int sub_font_px = MAX(10, (int)Math::round(15.0f * s));
	const float sub_track = 3.6f * s;
	const int sub_w = fc.measure_text_width(sub_txt, sub_font_px, sub_track);
	const int sub_y = title_y + MAX(22, (int)Math::round(36.0f * s));
	r_metrics.subtitle_rect = fc.draw_text(dst, W, H, sub_txt, (W - sub_w) / 2, sub_y, sub_font_px, p_pal.text_secondary, sub_track, false);

	// Línea separadora cobriza sutil centrada.
	const int sep_half_w = MAX(80, (int)Math::round(145.0f * s));
	const int sep_y = sub_y + MAX(16, (int)Math::round(26.0f * s));
	const int cx_mid = W / 2;
	for (int x = cx_mid - sep_half_w; x <= cx_mid + sep_half_w; x++) {
		const float t = 1.0f - fabsf((float)(x - cx_mid)) / (float)sep_half_w;
		_blend_pixel_rgba(dst, W, H, x, sep_y, p_pal.card_border, 0.48f * t);
	}

	const String cred_txt = String::utf8("Created by Kris'ZtS (ClockGears Studio)");
	const int cred_font_px = MAX(11, (int)Math::round(17.0f * s));
	const float cred_track = 0.4f * s;
	const int cred_w = fc.measure_text_width(cred_txt, cred_font_px, cred_track);
	const int cred_y = sep_y + MAX(22, (int)Math::round(36.0f * s));
	const Color cred_col = p_pal.text_primary.lerp(p_pal.text_secondary, 0.35f);
	r_metrics.credits_rect = fc.draw_text(dst, W, H, cred_txt, (W - cred_w) / 2, cred_y, cred_font_px, cred_col, cred_track, false);
}

static bool cg_splash_base_valid = false;
static bool cg_splash_base_brass = false;
static int cg_splash_base_w = 0;
static int cg_splash_base_h = 0;
static float cg_splash_base_dpi = 1.0f;
static Vector<uint8_t> cg_splash_base_bytes;
static ClockGearsBootSplash::LayoutMetrics cg_splash_cached_metrics;
static std::recursive_mutex cg_splash_render_mutex;

// Pool fijo reutilizable de partículas pixel-art (máximo 120 activas en total: 20 motas de ambiente + 100 chispas de la cabeza).
// Cero reservas ni liberaciones de memoria por fotograma.
struct SplashParticle {
	bool active = false;
	bool ambient = false;
	float x = 0.0f;
	float y = 0.0f;
	float vx = 0.0f;
	float vy = 0.0f;
	float age = 0.0f;
	float lifetime = 1.0f;
	int base_size_px = 3; // 2 a 4 px escalados por el píxel base entero de la pantalla
	uint8_t tone_idx = 0; // 0 = ámbar, 1 = dorado claro, 2 = cobrizo
	float base_alpha = 0.85f;
};

static SplashParticle cg_particle_pool[ClockGearsBootSplash::MAX_PARTICLES];
static bool cg_particles_initialized = false;
static uint32_t cg_particle_rng_state = 0xC6E00004u;
static int cg_particle_cap = ClockGearsBootSplash::MAX_PARTICLES;
static float cg_particle_spawn_accum = 0.0f;
static float cg_particle_last_progress = -2.0f;
static bool cg_particle_burst_done = false;
static int cg_particle_screen_w = ClockGearsBootSplash::BASE_DESIGN_WIDTH;
static int cg_particle_screen_h = ClockGearsBootSplash::BASE_DESIGN_HEIGHT;
static int cg_active_total_count = 0;
static int cg_active_spark_count = 0;
static int cg_active_ambient_count = 0;
static uint64_t cg_particle_total_spawns = 0;

static inline uint32_t _cg_rng_next() {
	uint32_t x = cg_particle_rng_state ? cg_particle_rng_state : 0xC6E00004u;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	cg_particle_rng_state = x;
	return x;
}

static inline float _cg_rng_float01() {
	return (float)(_cg_rng_next() & 0x00FFFFFFu) / 16777215.0f;
}

static inline float _cg_rng_range(float p_min, float p_max) {
	return p_min + (p_max - p_min) * _cg_rng_float01();
}

static void _cg_recount_particles() {
	int total = 0;
	int sparks = 0;
	int ambient = 0;
	for (int i = 0; i < ClockGearsBootSplash::MAX_PARTICLES; i++) {
		if (!cg_particle_pool[i].active) {
			continue;
		}
		total++;
		if (cg_particle_pool[i].ambient) {
			ambient++;
		} else {
			sparks++;
		}
	}
	cg_active_total_count = total;
	cg_active_spark_count = sparks;
	cg_active_ambient_count = ambient;
}

static void _cg_init_ambient_mote(SplashParticle &r_p, int p_w, int p_h, float p_s, bool p_random_age) {
	r_p.active = true;
	r_p.ambient = true;
	r_p.x = _cg_rng_range(32.0f * p_s, (float)p_w - 32.0f * p_s);
	r_p.y = _cg_rng_range(32.0f * p_s, (float)p_h - 32.0f * p_s);
	r_p.vx = _cg_rng_range(-9.0f * p_s, 9.0f * p_s);
	r_p.vy = _cg_rng_range(-14.0f * p_s, -4.5f * p_s);
	r_p.lifetime = _cg_rng_range(3.2f, 6.4f);
	r_p.age = p_random_age ? _cg_rng_range(0.0f, r_p.lifetime * 0.85f) : 0.0f;
	r_p.base_size_px = 2 + (int)(_cg_rng_next() % 2); // 2..3 px para motas suaves
	r_p.tone_idx = (uint8_t)(_cg_rng_next() % 3);
	r_p.base_alpha = _cg_rng_range(0.18f, 0.34f); // más tenues que las chispas
}

static bool _cg_spawn_head_spark(float p_head_x, float p_head_y, float p_s, bool p_burst) {
	_cg_recount_particles();
	if (cg_active_total_count >= cg_particle_cap || cg_active_spark_count >= ClockGearsBootSplash::MAX_HEAD_SPARKS) {
		return false;
	}
	for (int i = ClockGearsBootSplash::MAX_AMBIENT_MOTES; i < ClockGearsBootSplash::MAX_PARTICLES; i++) {
		if (!cg_particle_pool[i].active) {
			SplashParticle &sp = cg_particle_pool[i];
			sp.active = true;
			sp.ambient = false;
			sp.x = p_head_x + _cg_rng_range(-6.0f * p_s, 6.0f * p_s);
			sp.y = p_head_y + _cg_rng_range(-4.0f * p_s, 4.0f * p_s);
			// Suben y se desvían ligeramente con gravedad suave
			sp.vx = p_burst ? _cg_rng_range(-115.0f * p_s, 115.0f * p_s) : _cg_rng_range(-42.0f * p_s, 36.0f * p_s);
			sp.vy = p_burst ? _cg_rng_range(-135.0f * p_s, -32.0f * p_s) : _cg_rng_range(-88.0f * p_s, -26.0f * p_s);
			sp.age = 0.0f;
			sp.lifetime = _cg_rng_range(0.6f, 1.4f);
			sp.base_size_px = 2 + (int)(_cg_rng_next() % 3); // 2 a 4 px
			sp.tone_idx = (uint8_t)(_cg_rng_next() % 3);
			sp.base_alpha = _cg_rng_range(0.72f, 0.98f);
			cg_active_total_count++;
			cg_active_spark_count++;
			cg_particle_total_spawns++;
			return true;
		}
	}
	return false;
}

static void _cg_draw_pixel_art_particles(uint8_t *p_dst, int p_w, int p_h, float p_s, const ClockGearsBootSplash::Palette &p_pal, bool p_ambient_pass, float p_breath) {
	const int int_scale = MAX(1, (int)Math::round(p_s));
	const Color tones[3] = {
		p_pal.accent_amber,
		p_pal.progress_fill.lightened(0.45f),
		Color(p_pal.card_border.r, p_pal.card_border.g, p_pal.card_border.b, 1.0f).lightened(0.15f)
	};

	for (int i = 0; i < ClockGearsBootSplash::MAX_PARTICLES; i++) {
		const SplashParticle &pt = cg_particle_pool[i];
		if (!pt.active || pt.ambient != p_ambient_pass) {
			continue;
		}
		const float life_t = _clamp01(pt.age / MAX(0.05f, pt.lifetime));
		float fade = 1.0f;
		int sq_base = pt.base_size_px;
		if (pt.ambient) {
			// Entrada y salida suave para las motas de ambiente
			fade = sinf(life_t * (float)Math::PI) * pt.base_alpha * p_breath;
		} else {
			// Las chispas se desvanecen y encogen al final de su vida (0.6..1.4 s)
			const float end_t = _clamp01(1.0f - life_t);
			fade = (end_t * end_t) * pt.base_alpha * p_breath;
			if (life_t > 0.72f) {
				sq_base = MAX(1, pt.base_size_px - 2);
			} else if (life_t > 0.45f) {
				sq_base = MAX(2, pt.base_size_px - 1);
			}
		}
		if (fade <= 0.005f) {
			continue;
		}
		// Cuadrados pixel-art (2..4 px escalados por el múltiplo entero del píxel base, alineados a rejilla nearest) con mezcla aditiva.
		const int px_size = sq_base * int_scale;
		const int x0 = ((int)Math::round(pt.x) / int_scale) * int_scale - (px_size / 2);
		const int y0 = ((int)Math::round(pt.y) / int_scale) * int_scale - (px_size / 2);
		const Color &col = tones[pt.tone_idx % 3];
		for (int yy = 0; yy < px_size; yy++) {
			for (int xx = 0; xx < px_size; xx++) {
				_blend_pixel_additive_rgba(p_dst, p_w, p_h, x0 + xx, y0 + yy, col, fade);
			}
		}
	}
}

// Estado del ciclo de vida de la ventana propia de carga y trazas por evento (nunca por fotograma).
static bool cg_splash_window_open = false;
static bool cg_splash_was_opened = false;
static bool cg_splash_is_independent = false;
static bool cg_splash_has_parent = false;
static bool cg_splash_is_borderless = false;
static bool cg_splash_has_titlebar = false;
static bool cg_splash_has_taskbar_btn = false;
static bool cg_splash_ready_logged = false;
static bool cg_splash_first_frame_logged = false;
static bool cg_splash_close_fade_logged = false;
static bool cg_splash_watchdog_triggered = false;
static String cg_splash_closed_reason;
static uint64_t cg_splash_open_msec = 0;
static uint64_t cg_splash_first_frame_elapsed_msec = 0;
static uint64_t cg_splash_watchdog_timeout_ms = ClockGearsBootSplash::WATCHDOG_DEFAULT_TIMEOUT_MS;
static Ref<Image> cg_splash_last_card;
static Vector<String> cg_splash_pending_logs;
static bool cg_splash_log_file_initialized = false;

#ifdef WINDOWS_ENABLED
static HWND cg_splash_hwnd = nullptr;
static bool cg_splash_wndclass_registered = false;
static constexpr UINT_PTR CG_SPLASH_WATCHDOG_TIMER_ID = 0xC6E1;
static std::thread cg_splash_anim_thread;
static std::atomic<bool> cg_splash_anim_running{ false };
static std::atomic<int> cg_splash_anim_frames_drawn{ 0 };
static std::mutex cg_splash_stage_mutex;
static float cg_splash_live_progress = 0.0f;
static String cg_splash_live_status = String::utf8("Iniciando motor...");
static bool cg_splash_live_completed = false;
static bool cg_splash_live_brass = false;
static Vector2i cg_splash_live_mon_sz = Vector2i(ClockGearsBootSplash::BASE_DESIGN_WIDTH, ClockGearsBootSplash::BASE_DESIGN_HEIGHT);
static float cg_splash_live_dpi_scale = 1.0f;
static float cg_splash_last_render_ms = 2.0f;

static void _cg_pump_splash_hwnd_messages() {
	if (!cg_splash_hwnd) {
		return;
	}
	MSG msg;
	while (PeekMessageW(&msg, cg_splash_hwnd, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
}

static bool _cg_update_layered_splash(const Ref<Image> &p_card, uint8_t p_global_alpha) {
	if (!cg_splash_hwnd || p_card.is_null() || p_card->is_empty()) {
		return false;
	}
	const int w = p_card->get_width();
	const int h = p_card->get_height();
	if (w <= 0 || h <= 0) {
		return false;
	}

	HDC hdc_screen = GetDC(nullptr);
	if (!hdc_screen) {
		return false;
	}
	HDC hdc_mem = CreateCompatibleDC(hdc_screen);
	if (!hdc_mem) {
		ReleaseDC(nullptr, hdc_screen);
		return false;
	}

	BITMAPINFO bmi;
	memset(&bmi, 0, sizeof(bmi));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = w;
	bmi.bmiHeader.biHeight = -h; // Top-down DIB
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	void *dib_bits = nullptr;
	HBITMAP hbm = CreateDIBSection(hdc_screen, &bmi, DIB_RGB_COLORS, &dib_bits, nullptr, 0);
	if (!hbm || !dib_bits) {
		if (hbm) {
			DeleteObject(hbm);
		}
		DeleteDC(hdc_mem);
		ReleaseDC(nullptr, hdc_screen);
		return false;
	}

	// Convertir RGBA8 -> BGRA8 premultiplicado para UpdateLayeredWindow (AC_SRC_ALPHA).
	const uint8_t *src = p_card->get_data().ptr();
	uint8_t *dst = reinterpret_cast<uint8_t *>(dib_bits);
	const int total_px = w * h;
	for (int i = 0; i < total_px; i++) {
		const uint8_t r = src[i * 4 + 0];
		const uint8_t g = src[i * 4 + 1];
		const uint8_t b = src[i * 4 + 2];
		const uint8_t a = src[i * 4 + 3];
		dst[i * 4 + 0] = (uint8_t)((b * a + 127) / 255);
		dst[i * 4 + 1] = (uint8_t)((g * a + 127) / 255);
		dst[i * 4 + 2] = (uint8_t)((r * a + 127) / 255);
		dst[i * 4 + 3] = a;
	}

	HGDIOBJ old_bmp = SelectObject(hdc_mem, hbm);
	RECT wnd_rect;
	GetWindowRect(cg_splash_hwnd, &wnd_rect);
	POINT pt_dst = { wnd_rect.left, wnd_rect.top };
	SIZE sz_wnd = { w, h };
	POINT pt_src = { 0, 0 };

	BLENDFUNCTION bf;
	bf.BlendOp = AC_SRC_OVER;
	bf.BlendFlags = 0;
	bf.SourceConstantAlpha = p_global_alpha;
	bf.AlphaFormat = AC_SRC_ALPHA;

	BOOL ok = UpdateLayeredWindow(cg_splash_hwnd, hdc_screen, &pt_dst, &sz_wnd, hdc_mem, &pt_src, 0, &bf, ULW_ALPHA);

	SelectObject(hdc_mem, old_bmp);
	DeleteObject(hbm);
	DeleteDC(hdc_mem);
	ReleaseDC(nullptr, hdc_screen);
	return ok != FALSE;
}

static void _cg_stop_splash_anim_thread() {
	cg_splash_anim_running.store(false);
	if (cg_splash_anim_thread.joinable()) {
		if (cg_splash_anim_thread.get_id() != std::this_thread::get_id()) {
			cg_splash_anim_thread.join();
		} else {
			cg_splash_anim_thread.detach();
		}
	}
}

static void _cg_start_splash_anim_thread() {
	_cg_stop_splash_anim_thread();
	if (!cg_splash_hwnd) {
		return;
	}
	cg_splash_anim_running.store(true);
	cg_splash_anim_thread = std::thread([]() {
		uint64_t prev_ticks = GetTickCount64();
		while (cg_splash_anim_running.load()) {
			Sleep(28); // ~35 FPS constantes en segundo plano mientras el hilo principal carga etapas pesadas
			if (!cg_splash_anim_running.load() || !cg_splash_hwnd) {
				break;
			}
			const uint64_t now_ticks = GetTickCount64();
			const float dt = CLAMP((float)(now_ticks - prev_ticks) * 0.001f, 0.010f, 0.100f);
			prev_ticks = now_ticks;

			float prog = 0.0f;
			String st_txt;
			bool comp = false;
			bool brass = false;
			Vector2i mon_sz;
			float dpi_s = 1.0f;
			float last_ms = 2.0f;
			{
				std::lock_guard<std::mutex> lk(cg_splash_stage_mutex);
				prog = cg_splash_live_progress;
				st_txt = cg_splash_live_status;
				comp = cg_splash_live_completed;
				brass = cg_splash_live_brass;
				mon_sz = cg_splash_live_mon_sz;
				dpi_s = cg_splash_live_dpi_scale;
				last_ms = cg_splash_last_render_ms;
			}

			const uint64_t r0 = GetTickCount64();
			const float spin_deg = (float)((now_ticks * 180ULL / 1000ULL) % 360ULL);
			const float anim_sec = (float)now_ticks * 0.001f;
			ClockGearsBootSplash::step_particles(dt, prog, comp, mon_sz, brass, last_ms);
			Ref<Image> frame = ClockGearsBootSplash::render_card(prog, st_txt, spin_deg, comp, brass, nullptr, mon_sz, dpi_s, anim_sec);
			if (cg_splash_anim_running.load() && cg_splash_hwnd && frame.is_valid()) {
				_cg_update_layered_splash(frame, 255);
				cg_splash_anim_frames_drawn.fetch_add(1);
			}
			const float elapsed_render = (float)(GetTickCount64() - r0);
			{
				std::lock_guard<std::mutex> lk(cg_splash_stage_mutex);
				cg_splash_last_card = frame;
				cg_splash_last_render_ms = elapsed_render;
			}
		}
	});
}

static void _cg_destroy_splash_hwnd() {
	_cg_stop_splash_anim_thread();
	if (cg_splash_hwnd) {
		KillTimer(cg_splash_hwnd, CG_SPLASH_WATCHDOG_TIMER_ID);
		DestroyWindow(cg_splash_hwnd);
		cg_splash_hwnd = nullptr;
	}
	cg_splash_window_open = false;
}

static LRESULT CALLBACK _cg_splash_wndproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
		case WM_TIMER: {
			if (wParam == CG_SPLASH_WATCHDOG_TIMER_ID) {
				ClockGearsBootSplash::poll_watchdog();
				return 0;
			}
		} break;
		case WM_NCHITTEST: {
			return HTCLIENT;
		}
		case WM_CLOSE: {
			_cg_destroy_splash_hwnd();
			return 0;
		}
	}
	return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}
#endif // WINDOWS_ENABLED

} // namespace

void ClockGearsBootSplash::log_event(const String &p_event_line) {
	const uint64_t ticks = OS::get_singleton() ? OS::get_singleton()->get_ticks_msec() : 0;
	const String formatted = vformat("[BootSplash t=%dms] %s", (int)ticks, p_event_line);
	print_line(formatted);

	cg_splash_pending_logs.push_back(formatted);
	if (!OS::get_singleton()) {
		return;
	}
	const String user_dir = OS::get_singleton()->get_user_data_dir();
	if (user_dir.is_empty() || !DirAccess::exists(user_dir)) {
		return;
	}
	const String log_path = user_dir.path_join("boot_splash_trace.log");
	Ref<FileAccess> f = FileAccess::open(log_path, cg_splash_log_file_initialized ? FileAccess::READ_WRITE : FileAccess::WRITE);
	if (f.is_null()) {
		return;
	}
	if (cg_splash_log_file_initialized) {
		f->seek_end();
	}
	for (int i = 0; i < cg_splash_pending_logs.size(); i++) {
		f->store_line(cg_splash_pending_logs[i]);
	}
	f->flush();
	cg_splash_pending_logs.clear();
	cg_splash_log_file_initialized = true;
}

Vector2i ClockGearsBootSplash::get_primary_monitor_size(int *r_pos_x, int *r_pos_y, float *r_dpi_scale) {
	int pos_x = 0;
	int pos_y = 0;
	int mon_w = BASE_DESIGN_WIDTH;
	int mon_h = BASE_DESIGN_HEIGHT;
	float dpi_scale = 1.0f;
#ifdef WINDOWS_ENABLED
	POINT pt_zero = { 0, 0 };
	HMONITOR hMon = MonitorFromPoint(pt_zero, MONITOR_DEFAULTTOPRIMARY);
	MONITORINFO mi;
	memset(&mi, 0, sizeof(mi));
	mi.cbSize = sizeof(MONITORINFO);
	if (hMon && GetMonitorInfoW(hMon, &mi)) {
		pos_x = mi.rcMonitor.left;
		pos_y = mi.rcMonitor.top;
		mon_w = MAX(640, (int)(mi.rcMonitor.right - mi.rcMonitor.left));
		mon_h = MAX(400, (int)(mi.rcMonitor.bottom - mi.rcMonitor.top));
	}
	HDC hdc = GetDC(nullptr);
	if (hdc) {
		const int dpi_x = GetDeviceCaps(hdc, LOGPIXELSX);
		if (dpi_x > 0) {
			dpi_scale = CLAMP((float)dpi_x / 96.0f, 1.0f, 3.0f);
		}
		ReleaseDC(nullptr, hdc);
	}
#endif
	if (r_pos_x) {
		*r_pos_x = pos_x;
	}
	if (r_pos_y) {
		*r_pos_y = pos_y;
	}
	if (r_dpi_scale) {
		*r_dpi_scale = dpi_scale;
	}
	return Vector2i(mon_w, mon_h);
}

bool ClockGearsBootSplash::open_window(int p_screen) {
	(void)p_screen;
	if (cg_splash_window_open) {
		return true;
	}
	cg_splash_ready_logged = false;
	cg_splash_first_frame_logged = false;
	cg_splash_close_fade_logged = false;
	cg_splash_watchdog_triggered = false;
	cg_splash_closed_reason = String();
	cg_splash_open_msec = OS::get_singleton() ? OS::get_singleton()->get_ticks_msec() : 0;
	if (OS::get_singleton() && OS::get_singleton()->has_environment("CG_SPLASH_WATCHDOG_MS")) {
		const int env_ms = OS::get_singleton()->get_environment("CG_SPLASH_WATCHDOG_MS").to_int();
		if (env_ms >= 100) {
			cg_splash_watchdog_timeout_ms = (uint64_t)env_ms;
		}
	}

	int pos_x = 0;
	int pos_y = 0;
	float dpi_scale = 1.0f;
	const Vector2i mon_sz = get_primary_monitor_size(&pos_x, &pos_y, &dpi_scale);

#ifdef WINDOWS_ENABLED
	HINSTANCE hInst = GetModuleHandleW(nullptr);
	if (!cg_splash_wndclass_registered) {
		WNDCLASSEXW wc;
		memset(&wc, 0, sizeof(wc));
		wc.cbSize = sizeof(WNDCLASSEXW);
		wc.lpfnWndProc = _cg_splash_wndproc;
		wc.hInstance = hInst;
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wc.lpszClassName = L"ClockGearsBootSplashWindow";
		if (RegisterClassExW(&wc) != 0 || GetLastError() == ERROR_CLASS_ALREADY_EXISTS) {
			cg_splash_wndclass_registered = true;
		}
	}

	const DWORD dwStyle = WS_POPUP;
	const DWORD dwExStyle = WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST;
	cg_splash_hwnd = CreateWindowExW(
			dwExStyle,
			L"ClockGearsBootSplashWindow",
			L"ClockGears Engine Splash",
			dwStyle,
			pos_x, pos_y, mon_sz.x, mon_sz.y,
			nullptr, // Ventana propia a pantalla completa del monitor principal, nunca embebida en el editor.
			nullptr,
			hInst,
			nullptr);

	if (cg_splash_hwnd) {
		const LONG actual_style = GetWindowLongW(cg_splash_hwnd, GWL_STYLE);
		const LONG actual_exstyle = GetWindowLongW(cg_splash_hwnd, GWL_EXSTYLE);
		const HWND actual_parent = GetParent(cg_splash_hwnd);

		cg_splash_window_open = true;
		cg_splash_was_opened = true;
		cg_splash_is_independent = (actual_parent == nullptr && (actual_style & WS_CHILD) == 0);
		cg_splash_has_parent = (actual_parent != nullptr);
		cg_splash_is_borderless = ((actual_style & WS_CAPTION) == 0 && (actual_style & WS_THICKFRAME) == 0 && (actual_style & WS_POPUP) != 0);
		cg_splash_has_titlebar = ((actual_style & WS_CAPTION) != 0);
		cg_splash_has_taskbar_btn = ((actual_exstyle & WS_EX_TOOLWINDOW) == 0 || (actual_exstyle & WS_EX_APPWINDOW) != 0);

		SetTimer(cg_splash_hwnd, CG_SPLASH_WATCHDOG_TIMER_ID, 200, nullptr);
		ShowWindow(cg_splash_hwnd, SW_SHOWNOACTIVATE);
		{
			std::lock_guard<std::mutex> lk(cg_splash_stage_mutex);
			cg_splash_live_mon_sz = mon_sz;
			cg_splash_live_dpi_scale = dpi_scale;
		}
		cg_splash_anim_frames_drawn.store(0);
		_cg_start_splash_anim_thread();

		log_event(vformat("event=created window_type=\"Win32FullscreenPopup\" parent=\"%s\" borderless=%s titlebar=%s taskbar_button=%s size=%dx%d pos=(%d,%d) dpi_scale=%.2f",
				cg_splash_has_parent ? "HWND" : "NONE",
				cg_splash_is_borderless ? "true" : "false",
				cg_splash_has_titlebar ? "true" : "false",
				cg_splash_has_taskbar_btn ? "true" : "false",
				mon_sz.x, mon_sz.y, pos_x, pos_y, dpi_scale));
		return true;
	}
#endif
	// Respaldo no-Windows o headless
	cg_splash_window_open = true;
	cg_splash_was_opened = true;
	cg_splash_is_independent = true;
	cg_splash_has_parent = false;
	cg_splash_is_borderless = true;
	cg_splash_has_titlebar = false;
	cg_splash_has_taskbar_btn = false;
	log_event(vformat("event=created window_type=\"FullscreenPopup\" parent=\"NONE\" borderless=true titlebar=false taskbar_button=false size=%dx%d", mon_sz.x, mon_sz.y));
	return true;
}

bool ClockGearsBootSplash::is_window_open() {
	return cg_splash_window_open;
}

void ClockGearsBootSplash::mark_ready() {
	present_stage(TOTAL_STAGES, TOTAL_STAGES, String::utf8("Listo"), true);
	if (!cg_splash_ready_logged) {
		cg_splash_ready_logged = true;
		log_event("event=ready status=\"Listo\" progress=100%");
	}
}

void ClockGearsBootSplash::notify_first_frame_drawn(const String &p_context) {
	if (!cg_splash_first_frame_logged) {
		cg_splash_first_frame_logged = true;
		const uint64_t now = OS::get_singleton() ? OS::get_singleton()->get_ticks_msec() : 0;
		const uint64_t elapsed = (now >= cg_splash_open_msec) ? (now - cg_splash_open_msec) : 0;
		cg_splash_first_frame_elapsed_msec = elapsed;
		log_event(vformat("event=first_frame_drawn context=\"%s\" elapsed_ms=%d", p_context, (int)elapsed));
	}
	if (cg_splash_window_open) {
		close_with_fade("first_frame_drawn");
	}
}

void ClockGearsBootSplash::close_with_fade(const String &p_reason) {
	if (!cg_splash_window_open) {
		return;
	}
	cg_splash_closed_reason = p_reason;
	if (!cg_splash_close_fade_logged) {
		cg_splash_close_fade_logged = true;
		log_event(vformat("event=close_fade reason=\"%s\" fade_steps=5", p_reason));
	}

#ifdef WINDOWS_ENABLED
	_cg_stop_splash_anim_thread();
	if (cg_splash_hwnd && cg_splash_last_card.is_valid()) {
		const uint8_t alphas[5] = { 200, 145, 95, 45, 0 };
		for (int i = 0; i < 5; i++) {
			_cg_update_layered_splash(cg_splash_last_card, alphas[i]);
			_cg_pump_splash_hwnd_messages();
			if (OS::get_singleton()) {
				OS::get_singleton()->delay_usec(16000);
			}
		}
	}
	_cg_destroy_splash_hwnd();
#else
	cg_splash_window_open = false;
#endif
}

void ClockGearsBootSplash::close_on_error(const String &p_error_reason) {
	if (!cg_splash_window_open) {
		return;
	}
	cg_splash_closed_reason = "error:" + p_error_reason;
	log_event(vformat("ERROR event=closed_on_error reason=\"%s\"", p_error_reason));
#ifdef WINDOWS_ENABLED
	_cg_destroy_splash_hwnd();
#else
	cg_splash_window_open = false;
#endif
}

void ClockGearsBootSplash::poll_watchdog() {
	if (!cg_splash_window_open) {
		return;
	}
#ifdef WINDOWS_ENABLED
	_cg_pump_splash_hwnd_messages();
#endif
	const uint64_t now = OS::get_singleton() ? OS::get_singleton()->get_ticks_msec() : 0;
	if (now >= cg_splash_open_msec && (now - cg_splash_open_msec) >= cg_splash_watchdog_timeout_ms) {
		cg_splash_watchdog_triggered = true;
		const uint64_t elapsed = now - cg_splash_open_msec;
		log_event(vformat("WARN event=watchdog_timeout_close elapsed_ms=%d limit_ms=%d msg=\"Seguro anti-bloqueo: pantalla de carga cerrada automáticamente tras superar el tiempo límite\"",
				(int)elapsed, (int)cg_splash_watchdog_timeout_ms));
		close_with_fade("watchdog_timeout_15s");
	}
}

void ClockGearsBootSplash::set_watchdog_timeout_ms(uint64_t p_timeout_ms) {
	cg_splash_watchdog_timeout_ms = MAX((uint64_t)100, p_timeout_ms);
}

Dictionary ClockGearsBootSplash::get_last_lifecycle_state() {
	LayoutMetrics m;
	const bool is_brass = is_saved_brass_theme();
	float dpi_scale = 1.0f;
	const Vector2i mon_sz = get_primary_monitor_size(nullptr, nullptr, &dpi_scale);
	(void)render_card(1.0f, String::utf8("Listo"), 0.0f, true, is_brass, &m, mon_sz, dpi_scale);
	const bool badge_unclipped = (m.badge_rect.position.x >= 0 && m.badge_rect.position.y >= m.inner_frame_rect.position.y &&
			m.badge_rect.get_end().x <= mon_sz.x && m.badge_rect.get_end().y <= mon_sz.y);

	Dictionary d;
	d["window_open"] = cg_splash_window_open;
	d["was_opened"] = cg_splash_was_opened;
	d["is_independent_window"] = cg_splash_is_independent;
	d["has_parent"] = cg_splash_has_parent;
	d["is_borderless"] = cg_splash_is_borderless;
	d["has_titlebar"] = cg_splash_has_titlebar;
	d["has_taskbar_button"] = cg_splash_has_taskbar_btn;
	d["ready_logged"] = cg_splash_ready_logged;
	d["first_frame_logged"] = cg_splash_first_frame_logged;
	d["close_fade_logged"] = cg_splash_close_fade_logged;
	d["watchdog_triggered"] = cg_splash_watchdog_triggered;
	d["closed_reason"] = cg_splash_closed_reason;
	d["version_badge_rect"] = m.badge_rect;
	d["version_badge_unclipped"] = badge_unclipped;
	d["monitor_size"] = mon_sz;
	d["inner_frame_rect"] = m.inner_frame_rect;
	d["first_frame_elapsed_ms"] = (int)cg_splash_first_frame_elapsed_msec;
#ifdef WINDOWS_ENABLED
	d["anim_frames_drawn"] = cg_splash_anim_frames_drawn.load();
#else
	d["anim_frames_drawn"] = 0;
#endif
	if (OS::get_singleton() && !OS::get_singleton()->get_user_data_dir().is_empty()) {
		d["trace_log_path"] = OS::get_singleton()->get_user_data_dir().path_join("boot_splash_trace.log");
	} else {
		d["trace_log_path"] = String();
	}
	return d;
}

bool ClockGearsBootSplash::is_saved_brass_theme() {
	if (ProjectSettings::get_singleton() && ProjectSettings::get_singleton()->has_setting("clockgears/editor/theme_id")) {
		const String tid = String(ProjectSettings::get_singleton()->get_setting("clockgears/editor/theme_id")).strip_edges().to_lower();
		return (tid == "brass" || tid == "laton" || tid == String::utf8("latón"));
	}
	return false;
}

static bool cg_splash_glow_override_active = false;
static bool cg_splash_glow_override_value = true;

void ClockGearsBootSplash::set_glow_effects_override(bool p_has_override, bool p_enabled) {
	cg_splash_glow_override_active = p_has_override;
	cg_splash_glow_override_value = p_enabled;
}

bool ClockGearsBootSplash::are_glow_effects_enabled() {
	if (cg_splash_glow_override_active) {
		return cg_splash_glow_override_value;
	}
	if (OS::get_singleton() && OS::get_singleton()->has_environment("CG_SPLASH_GLOW_EFFECTS")) {
		const String env_v = OS::get_singleton()->get_environment("CG_SPLASH_GLOW_EFFECTS").strip_edges().to_lower();
		return !(env_v == "0" || env_v == "false" || env_v == "off" || env_v == "no");
	}
	if (ProjectSettings::get_singleton()) {
		if (ProjectSettings::get_singleton()->has_setting("clockgears/editor/glow_effects")) {
			return (bool)ProjectSettings::get_singleton()->get_setting("clockgears/editor/glow_effects");
		}
		if (ProjectSettings::get_singleton()->has_setting("clockgears/editor/appearance/glow_effects")) {
			return (bool)ProjectSettings::get_singleton()->get_setting("clockgears/editor/appearance/glow_effects");
		}
		if (ProjectSettings::get_singleton()->has_setting("clockgears/appearance/glow_effects")) {
			return (bool)ProjectSettings::get_singleton()->get_setting("clockgears/appearance/glow_effects");
		}
	}
	return true;
}

float ClockGearsBootSplash::compute_breathing_intensity(float p_time_sec) {
	// Respiración suave entre 85 % (0.85) y 100 % (1.00) con un ciclo de 2 s.
	const float phase = p_time_sec * ((float)Math::TAU / 2.0f);
	return CLAMP(0.925f + 0.075f * sinf(phase), 0.85f, 1.00f);
}

void ClockGearsBootSplash::reset_particles(uint32_t p_seed, const Vector2i &p_screen_size, bool p_brass) {
	std::lock_guard<std::recursive_mutex> lk(cg_splash_render_mutex);
	(void)p_brass;
	const int W = MAX(320, p_screen_size.x);
	const int H = MAX(200, p_screen_size.y);
	const float s = MIN((float)W / (float)BASE_DESIGN_WIDTH, (float)H / (float)BASE_DESIGN_HEIGHT);

	cg_particle_rng_state = (p_seed != 0) ? p_seed : 0xC6E00004u;
	cg_particle_cap = MAX_PARTICLES;
	cg_particle_spawn_accum = 0.0f;
	cg_particle_last_progress = -2.0f;
	cg_particle_burst_done = false;
	cg_particle_screen_w = W;
	cg_particle_screen_h = H;
	cg_particle_total_spawns = 0;

	for (int i = 0; i < MAX_PARTICLES; i++) {
		cg_particle_pool[i] = SplashParticle();
	}

	if (!are_glow_effects_enabled()) {
		cg_particles_initialized = true;
		_cg_recount_particles();
		return;
	}

	// Sembrar hasta 20 motas doradas flotando despacio por toda la pantalla
	for (int i = 0; i < MAX_AMBIENT_MOTES; i++) {
		_cg_init_ambient_mote(cg_particle_pool[i], W, H, s, true);
	}
	cg_particles_initialized = true;
	_cg_recount_particles();
}

void ClockGearsBootSplash::step_particles(float p_dt, float p_progress, bool p_completed, const Vector2i &p_screen_size, bool p_brass, float p_last_frame_ms) {
	std::lock_guard<std::recursive_mutex> lk(cg_splash_render_mutex);
	(void)p_brass;
	const int W = MAX(320, p_screen_size.x);
	const int H = MAX(200, p_screen_size.y);
	const float s = MIN((float)W / (float)BASE_DESIGN_WIDTH, (float)H / (float)BASE_DESIGN_HEIGHT);
	const float dt = CLAMP(p_dt, 0.001f, 0.10f);

	if (!are_glow_effects_enabled()) {
		for (int i = 0; i < MAX_PARTICLES; i++) {
			cg_particle_pool[i].active = false;
		}
		_cg_recount_particles();
		return;
	}

	if (!cg_particles_initialized || cg_particle_screen_w != W || cg_particle_screen_h != H) {
		reset_particles(cg_particle_rng_state, Vector2i(W, H), p_brass);
	}

	// Si el fotograma tarda más de 16,7 ms, reduce automáticamente la cantidad máxima de partículas activas.
	if (p_last_frame_ms > FRAME_BUDGET_MS) {
		cg_particle_cap = MAX(24, cg_particle_cap - 12);
		_cg_recount_particles();
		for (int i = MAX_PARTICLES - 1; i >= MAX_AMBIENT_MOTES && cg_active_total_count > cg_particle_cap; i--) {
			if (cg_particle_pool[i].active) {
				cg_particle_pool[i].active = false;
				cg_active_total_count--;
				cg_active_spark_count--;
			}
		}
	} else if (p_last_frame_ms < FRAME_BUDGET_MS * 0.70f && cg_particle_cap < MAX_PARTICLES) {
		cg_particle_cap = MIN(MAX_PARTICLES, cg_particle_cap + 2);
	}

	// Actualizar motas de ambiente (slots 0..MAX_AMBIENT_MOTES-1) y chispas (slots MAX_AMBIENT_MOTES..MAX_PARTICLES-1)
	const float gravity = 32.0f * s; // gravedad suave
	for (int i = 0; i < MAX_PARTICLES; i++) {
		SplashParticle &pt = cg_particle_pool[i];
		if (!pt.active) {
			if (i < MAX_AMBIENT_MOTES && cg_active_total_count < cg_particle_cap) {
				_cg_init_ambient_mote(pt, W, H, s, false);
				cg_active_total_count++;
				cg_active_ambient_count++;
			}
			continue;
		}
		pt.age += dt;
		if (pt.age >= pt.lifetime) {
			if (pt.ambient) {
				_cg_init_ambient_mote(pt, W, H, s, false);
			} else {
				pt.active = false;
			}
			continue;
		}
		pt.x += pt.vx * dt;
		pt.y += pt.vy * dt;
		if (pt.ambient) {
			// Deriva senoidal lenta y suave por toda la pantalla
			pt.x += sinf(pt.age * 1.4f + (float)i) * 4.0f * s * dt;
			if (pt.y < 16.0f * s) {
				pt.y = (float)H - 24.0f * s;
			}
			if (pt.x < 16.0f * s) {
				pt.x = (float)W - 24.0f * s;
			} else if (pt.x > (float)W - 16.0f * s) {
				pt.x = 24.0f * s;
			}
		} else {
			pt.vy += gravity * dt;
		}
	}
	_cg_recount_particles();

	// Calcular posición actual de la cabeza de avance de la barra de progreso
	const int bar_w = MAX(200, (int)Math::round((float)W * 0.40f));
	const int bar_h = MAX(6, (int)Math::round(10.0f * s));
	const int bar_x = (W - bar_w) / 2;
	const int bar_y = (int)Math::round((float)H * 0.79f);
	float head_x = (float)bar_x;
	const float head_y = (float)bar_y + (float)bar_h * 0.5f;
	if (!p_completed && p_progress < 0.0f) {
		const float seg_w = MAX((float)bar_h * 3.0f, (float)bar_w * 0.28f);
		const float travel = MAX(1.0f, (float)bar_w - seg_w);
		const float anim_t = OS::get_singleton() ? ((float)OS::get_singleton()->get_ticks_msec() * 0.001f) : 0.0f;
		const float sweep = 0.5f + 0.5f * sinf(anim_t * ((float)Math::TAU / 1.8f));
		head_x = (float)bar_x + travel * sweep + seg_w * 0.5f;
	} else {
		const float frac = p_completed ? 1.0f : _clamp01(p_progress);
		const float seg_w = (frac > 0.001f) ? MAX((float)bar_h, (float)bar_w * frac) : 0.0f;
		head_x = (float)bar_x + seg_w;
	}

	// Emisión desde la cabeza de avance: la emisión sube un poco cuando la barra avanza
	float emit_rate = 24.0f; // chispas por segundo base
	if (cg_particle_last_progress >= 0.0f && p_progress > cg_particle_last_progress + 0.001f) {
		const float delta_prog = _clamp01(p_progress - cg_particle_last_progress);
		emit_rate += 38.0f + delta_prog * 140.0f;
		// Ráfaga suave inmediata al avanzar de etapa
		const int step_sparks = MIN(8, (int)Math::round(2.0f + delta_prog * 20.0f));
		for (int k = 0; k < step_sparks; k++) {
			_cg_spawn_head_spark(head_x, head_y, s, false);
		}
	}
	cg_particle_last_progress = p_progress;

	cg_particle_spawn_accum += emit_rate * dt;
	while (cg_particle_spawn_accum >= 1.0f) {
		cg_particle_spawn_accum -= 1.0f;
		if (!_cg_spawn_head_spark(head_x, head_y, s, false)) {
			cg_particle_spawn_accum = 0.0f;
			break;
		}
	}

	// Al llegar al 100 % ("Listo"), ráfaga de partículas a lo largo de la barra y la cabeza
	if (p_completed && !cg_particle_burst_done) {
		cg_particle_burst_done = true;
		for (int k = 0; k < 28; k++) {
			const float bx = (float)bar_x + (float)bar_w * _cg_rng_range(0.35f, 1.0f);
			_cg_spawn_head_spark(bx, head_y, s, true);
		}
	}
	_cg_recount_particles();
}

Dictionary ClockGearsBootSplash::get_particle_state() {
	std::lock_guard<std::recursive_mutex> lk(cg_splash_render_mutex);
	_cg_recount_particles();
	Dictionary d;
	d["active_total"] = cg_active_total_count;
	d["active_sparks"] = cg_active_spark_count;
	d["active_ambient"] = cg_active_ambient_count;
	d["particle_cap"] = cg_particle_cap;
	d["max_particles"] = (int)MAX_PARTICLES;
	d["max_ambient_motes"] = (int)MAX_AMBIENT_MOTES;
	d["pool_capacity"] = (int)MAX_PARTICLES;
	d["total_spawns"] = (uint64_t)cg_particle_total_spawns;
	d["glow_enabled"] = are_glow_effects_enabled();
	d["burst_done"] = cg_particle_burst_done;
	return d;
}

ClockGearsBootSplash::Palette ClockGearsBootSplash::get_active_palette(bool p_force_brass, bool p_override_brass) {
	const bool use_brass = p_override_brass ? p_force_brass : is_saved_brass_theme();
	Palette out;
#ifdef MODULE_CLOCKGEARS_ENABLED
	ClockGearsTheme::SplashPalette tp = ClockGearsTheme::get_splash_palette(use_brass);
	out.bg_top = tp.bg_top;
	out.bg_bottom = tp.bg_bottom;
	out.card_border = tp.card_border;
	out.accent_amber = tp.accent_amber;
	out.halo_amber = tp.halo_amber;
	out.ornament_copper = tp.ornament_copper;
	out.text_primary = tp.text_primary;
	out.text_secondary = tp.text_secondary;
	out.badge_bg = tp.badge_bg;
	out.progress_track = tp.progress_track;
	out.progress_fill = tp.progress_fill;
	out.corner_radius = tp.corner_radius;
	out.border_width = tp.border_width;
#else
	out.bg_top = Color::html("#1e2128");
	out.bg_bottom = Color::html("#14161c");
	out.card_border = Color(0.88f, 0.63f, 0.25f, 0.65f);
	out.accent_amber = Color::html("#e0a040");
	out.halo_amber = Color(0.88f, 0.63f, 0.25f, 0.28f);
	out.ornament_copper = Color(0.88f, 0.63f, 0.25f, 0.14f);
	out.text_primary = Color::html("#e6e6e6");
	out.text_secondary = Color::html("#9aa0ab");
	out.badge_bg = Color::html("#23262e");
	out.progress_track = Color::html("#13151b");
	out.progress_fill = Color::html("#e0a040");
	out.corner_radius = 6;
	out.border_width = 1;
#endif
	return out;
}

Color ClockGearsBootSplash::get_active_clear_color() {
	return get_active_palette(false, false).bg_bottom;
}

Vector<Vector2> ClockGearsBootSplash::get_rounded_window_polygon(int p_width, int p_height, int p_radius) {
	const int r = (p_radius > 0) ? p_radius : get_active_palette().corner_radius;
	Vector<Vector2> poly;
	const int segs = 8;
	const float fw = (float)p_width;
	const float fh = (float)p_height;
	const float fr = (float)r;

	// Esquina superior derecha -> inferior derecha -> inferior izquierda -> superior izquierda
	const float cxs[4] = { fw - fr, fw - fr, fr, fr };
	const float cys[4] = { fr, fh - fr, fh - fr, fr };
	const float a0s[4] = { -1.5707963f, 0.0f, 1.5707963f, 3.14159265f };

	for (int c = 0; c < 4; c++) {
		for (int s = 0; s <= segs; s++) {
			const float a = a0s[c] + (1.5707963f * (float)s) / (float)segs;
			poly.push_back(Vector2(cxs[c] + cosf(a) * fr, cys[c] + sinf(a) * fr));
		}
	}
	return poly;
}

Ref<Image> ClockGearsBootSplash::render_card(float p_progress, const String &p_status, float p_spinner_deg, bool p_completed, bool p_brass, LayoutMetrics *r_metrics, const Vector2i &p_screen_size, float p_dpi_scale, float p_anim_time_sec) {
	std::lock_guard<std::recursive_mutex> lk(cg_splash_render_mutex);
	const int W = MAX(320, p_screen_size.x);
	const int H = MAX(200, p_screen_size.y);
	const float s = MIN((float)W / (float)BASE_DESIGN_WIDTH, (float)H / (float)BASE_DESIGN_HEIGHT);
	const Palette pal = get_active_palette(p_brass, true);

	if (!cg_splash_base_valid || cg_splash_base_brass != p_brass || cg_splash_base_w != W || cg_splash_base_h != H ||
			!Math::is_equal_approx(cg_splash_base_dpi, p_dpi_scale) || cg_splash_base_bytes.size() != W * H * 4) {
		cg_splash_cached_metrics = LayoutMetrics();
		_build_static_base(cg_splash_base_bytes, W, H, p_dpi_scale, pal, cg_splash_cached_metrics);
		cg_splash_base_brass = p_brass;
		cg_splash_base_w = W;
		cg_splash_base_h = H;
		cg_splash_base_dpi = p_dpi_scale;
		cg_splash_base_valid = true;
	}

	Vector<uint8_t> frame_bytes = cg_splash_base_bytes;
	uint8_t *dst = frame_bytes.ptrw();
	LayoutMetrics metrics = cg_splash_cached_metrics;

	const float anim_t = (p_anim_time_sec >= 0.0f)
			? p_anim_time_sec
			: (OS::get_singleton() ? ((float)OS::get_singleton()->get_ticks_msec() * 0.001f) : 0.0f);
	const bool glow_enabled = are_glow_effects_enabled();
	const float breath = glow_enabled ? compute_breathing_intensity(anim_t) : 1.0f;
	const bool indeterminate = (!p_completed && p_progress < 0.0f);

	metrics.glow_enabled = glow_enabled;
	metrics.glow_breath_factor = breath;
	metrics.indeterminate = indeterminate;

	if (glow_enabled) {
		if (!cg_particles_initialized || cg_particle_screen_w != W || cg_particle_screen_h != H || cg_active_total_count == 0) {
			reset_particles(cg_particle_rng_state, Vector2i(W, H), p_brass);
			step_particles(0.033f, p_progress, p_completed, Vector2i(W, H), p_brass, 2.0f);
		}
		// 20 motas doradas flotando despacio por toda la pantalla
		_cg_draw_pixel_art_particles(dst, W, H, s, pal, true, breath);
	} else {
		_cg_recount_particles();
	}

	// Bloque de la barra de progreso centrado en el tercio inferior, con un ancho de ~40 % de la pantalla.
	SplashFontCache &fc = get_font_cache();
	const int bar_w = MAX(200, (int)Math::round((float)W * 0.40f));
	const int bar_h = MAX(6, (int)Math::round(10.0f * s));
	const int bar_x = (W - bar_w) / 2;
	const int bar_y = (int)Math::round((float)H * 0.79f);
	const int status_font_px = MAX(11, (int)Math::round(15.0f * s));
	const int brand_font_px = MAX(10, (int)Math::round(14.0f * s));
	const int status_baseline = bar_y - MAX(10, (int)Math::round(14.0f * s));

	const Color status_col = p_completed ? pal.accent_amber : pal.text_primary;
	metrics.status_rect = fc.draw_text(dst, W, H, p_status, bar_x, status_baseline, status_font_px, status_col, 0.25f * s, p_completed);

	// Indicador giratorio y marca "ClockGears Engine" encima de la barra a la derecha.
	const float spin_r = MAX(5.0f, 7.5f * s);
	const float spin_cx = (float)(bar_x + bar_w) - spin_r - 2.0f * s;
	const float spin_cy = (float)status_baseline - (float)brand_font_px * 0.36f;
	const float spin_th = MAX(1.3f, 1.8f * s);
	if (p_completed) {
		_draw_aa_ring(dst, W, H, spin_cx, spin_cy, spin_r, spin_th, pal.accent_amber);
		_draw_aa_disc(dst, W, H, spin_cx, spin_cy, MAX(2.2f, 3.2f * s), pal.accent_amber);
	} else {
		_draw_aa_ring(dst, W, H, spin_cx, spin_cy, spin_r, spin_th * 0.85f, Color(pal.accent_amber.r, pal.accent_amber.g, pal.accent_amber.b, 0.25f));
		const float rad0 = Math::deg_to_rad(p_spinner_deg);
		const int arc_segs = 18;
		const float arc_span = 2.6f;
		for (int seg = 0; seg < arc_segs; seg++) {
			const float t0 = rad0 + (arc_span * (float)seg) / (float)arc_segs;
			const float t1 = rad0 + (arc_span * (float)(seg + 1)) / (float)arc_segs;
			const float fade = (float)(seg + 1) / (float)arc_segs;
			_draw_aa_line(dst, W, H,
					spin_cx + cosf(t0) * spin_r, spin_cy + sinf(t0) * spin_r,
					spin_cx + cosf(t1) * spin_r, spin_cy + sinf(t1) * spin_r,
					spin_th, Color(pal.accent_amber.r, pal.accent_amber.g, pal.accent_amber.b, fade));
		}
	}

	const String right_txt = String::utf8("ClockGears Engine");
	const int right_w = fc.measure_text_width(right_txt, brand_font_px, 0.25f * s);
	const int right_x = (int)Math::round(spin_cx - spin_r - MAX(8.0f, 10.0f * s)) - right_w;
	metrics.right_brand_rect = fc.draw_text(dst, W, H, right_txt, right_x, status_baseline, brand_font_px, pal.text_secondary, 0.25f * s, false);

	// Barra de progreso redondeada (10 px de alto a 1080p) con degradado horizontal de ámbar a dorado claro, halo exterior y cabeza con destello.
	const float bar_rad = (float)bar_h * 0.5f;
	metrics.progress_bar_rect = Rect2i(bar_x, bar_y, bar_w, bar_h);
	_draw_filled_rounded_rect(dst, W, H, (float)bar_x, (float)bar_y, (float)bar_w, (float)bar_h, bar_rad,
			pal.progress_track, Color(pal.card_border.r, pal.card_border.g, pal.card_border.b, 0.36f), MAX(1.0f, s));

	float seg_x0 = (float)bar_x;
	float seg_w = 0.0f;
	if (indeterminate) {
		// Modo indeterminado: un segmento de brillo recorre la barra de lado a lado sin inventar un porcentaje.
		seg_w = MAX((float)bar_h * 3.0f, (float)bar_w * 0.28f);
		const float travel = MAX(1.0f, (float)bar_w - seg_w);
		const float sweep = 0.5f + 0.5f * sinf(anim_t * ((float)Math::TAU / 1.8f));
		seg_x0 = (float)bar_x + travel * sweep;
		metrics.head_x = seg_x0 + seg_w * 0.5f;
		metrics.head_y = (float)bar_y + (float)bar_h * 0.5f;
	} else {
		const float frac = p_completed ? 1.0f : _clamp01(p_progress);
		if (frac > 0.001f) {
			seg_w = MAX((float)bar_h, (float)bar_w * frac);
		}
		seg_x0 = (float)bar_x;
		metrics.head_x = seg_x0 + seg_w;
		metrics.head_y = (float)bar_y + (float)bar_h * 0.5f;
	}

	if (seg_w > 0.5f) {
		const Color col_amber = pal.progress_fill;
		const Color col_gold_light = pal.progress_fill.lightened(0.45f);
		const Color col_head_bright = pal.progress_fill.lightened(0.68f);

		// Halo exterior ámbar alrededor de la parte llena (mezcla aditiva con respiración 85%..100% en ciclo de 2 s).
		if (glow_enabled) {
			const float halo_pad_x = MAX(12.0f, 22.0f * s);
			const float halo_pad_y = MAX(10.0f, 18.0f * s);
			const int hx0 = MAX(0, (int)floorf(seg_x0 - halo_pad_x));
			const int hx1 = MIN(W - 1, (int)ceilf(seg_x0 + seg_w + halo_pad_x));
			const int hy0 = MAX(0, (int)floorf((float)bar_y - halo_pad_y));
			const int hy1 = MIN(H - 1, (int)ceilf((float)(bar_y + bar_h) + halo_pad_y));
			const float inv_pad = 1.0f / halo_pad_y;

			for (int y = hy0; y <= hy1; y++) {
				for (int x = hx0; x <= hx1; x++) {
					const float d = _sd_rounded_box((float)x + 0.5f, (float)y + 0.5f, seg_x0, (float)bar_y, seg_w, (float)bar_h, bar_rad);
					if (d > -1.0f && d < halo_pad_y) {
						const float nd = MAX(0.0f, d) * inv_pad;
						const float falloff = expf(-3.1f * nd * nd) * (1.0f - nd);
						const float ht = _clamp01(((float)x - seg_x0) / MAX(1.0f, seg_w));
						const Color hcol = col_amber.lerp(col_gold_light, ht * 0.7f);
						_blend_pixel_additive_rgba(dst, W, H, x, y, hcol, falloff * 0.42f * breath);
					}
				}
			}
		}

		// Relleno de la barra con degradado horizontal de ámbar a dorado claro + brillo superior sutil.
		const int fx0 = MAX(0, (int)floorf(seg_x0 - 1.0f));
		const int fx1 = MIN(W - 1, (int)ceilf(seg_x0 + seg_w + 1.0f));
		for (int y = bar_y; y < bar_y + bar_h; y++) {
			const float row_t = (float)(y - bar_y) / (float)MAX(1, bar_h - 1);
			const float v_highlight = (1.0f - row_t) * 0.16f;
			for (int x = fx0; x <= fx1; x++) {
				const float d = _sd_rounded_box((float)x + 0.5f, (float)y + 0.5f, seg_x0, (float)bar_y, seg_w, (float)bar_h, bar_rad);
				if (d < 1.0f) {
					const float ht = _clamp01(((float)x + 0.5f - seg_x0) / MAX(1.0f, seg_w));
					Color px_col = col_amber.lerp(col_gold_light, ht).lightened(v_highlight * breath);
					if (indeterminate) {
						// Suavizar extremos del segmento flotante indeterminado
						const float edge_fade = sinf(ht * (float)Math::PI);
						px_col = col_amber.lerp(col_head_bright, edge_fade);
					}
					_blend_pixel_rgba(dst, W, H, x, y, px_col, _clamp01(1.0f - d));
				}
			}
		}

		// Cabeza de avance más brillante con pequeño destello precalculado (mezcla aditiva).
		if (glow_enabled) {
			const float flare_rx = MAX(14.0f, 26.0f * s);
			const float flare_ry = MAX(10.0f, 18.0f * s);
			_stamp_additive_glow_sprite(dst, W, H, metrics.head_x, metrics.head_y, flare_rx, flare_ry, col_head_bright, 0.72f * breath, true);
			_stamp_additive_glow_sprite(dst, W, H, metrics.head_x, metrics.head_y, flare_rx * 0.45f, flare_ry * 0.45f, Color(1.0f, 0.96f, 0.84f, 1.0f), 0.55f * breath, false);

			// Al llegar al 100 % ("Listo"), destello breve a lo largo de toda la barra.
			if (p_completed) {
				const int flash_stamps = 9;
				for (int i = 0; i < flash_stamps; i++) {
					const float u = (float)i / (float)(flash_stamps - 1);
					const float cx = seg_x0 + seg_w * u;
					_stamp_additive_glow_sprite(dst, W, H, cx, metrics.head_y, flare_rx * 1.45f, flare_ry * 1.15f, col_head_bright, 0.28f * breath, false);
				}
			}

			// Chispas pixel-art (cuadrados 2..4 px con filtro nearest y mezcla aditiva) saliendo de la cabeza de avance.
			_cg_draw_pixel_art_particles(dst, W, H, s, pal, false, breath);
		}
	}

	_cg_recount_particles();
	metrics.active_particle_count = glow_enabled ? cg_active_total_count : 0;
	metrics.active_spark_count = glow_enabled ? cg_active_spark_count : 0;
	metrics.active_ambient_count = glow_enabled ? cg_active_ambient_count : 0;
	metrics.particle_cap = cg_particle_cap;
	metrics.pool_capacity = MAX_PARTICLES;

	if (r_metrics) {
		*r_metrics = metrics;
	}

	Ref<Image> img = Image::create_from_data(W, H, false, Image::FORMAT_RGBA8, frame_bytes);
	return img;
}

void ClockGearsBootSplash::present_stage(int p_stage, int p_total_stages, const String &p_status, bool p_completed) {
	const int total = MAX(1, p_total_stages);
	const bool indeterminate = (!p_completed && p_stage < 0);
	const int stage = indeterminate ? 0 : CLAMP(p_stage, 0, total);
	const float progress = p_completed ? 1.0f : (indeterminate ? -1.0f : ((float)stage / (float)total));
	const int pct = (int)Math::round(MAX(0.0f, progress) * 100.0f);
	const String full_status = (p_completed || indeterminate) ? p_status : vformat(String::utf8("%s (%d %%)"), p_status, pct);
	const float spinner_deg = (float)(stage * 64 + (int)((OS::get_singleton() ? OS::get_singleton()->get_ticks_msec() : 0) % 360));
	const bool is_brass = is_saved_brass_theme();

	float dpi_scale = 1.0f;
	const Vector2i mon_sz = get_primary_monitor_size(nullptr, nullptr, &dpi_scale);

#ifdef WINDOWS_ENABLED
	{
		std::lock_guard<std::mutex> lk(cg_splash_stage_mutex);
		cg_splash_live_progress = progress;
		cg_splash_live_status = full_status;
		cg_splash_live_completed = p_completed;
		cg_splash_live_brass = is_brass;
		cg_splash_live_mon_sz = mon_sz;
		cg_splash_live_dpi_scale = dpi_scale;
	}
#endif

	if (!cg_splash_window_open && !p_completed) {
		open_window(0);
	}

#ifdef WINDOWS_ENABLED
	// Si el hilo de animación en segundo plano ya está dibujando a ~35 FPS y aún no es la etapa final "Listo",
	// dejamos que el hilo de animación renderice el fotograma para no añadir latencia al hilo principal.
	if (cg_splash_anim_running.load() && cg_splash_anim_frames_drawn.load() > 0 && !p_completed) {
		_cg_pump_splash_hwnd_messages();
		poll_watchdog();
		return;
	}
#endif

	step_particles(0.033f, progress, p_completed, mon_sz, is_brass, 2.0f);
	Ref<Image> card = render_card(progress, full_status, spinner_deg, p_completed, is_brass, nullptr, mon_sz, dpi_scale);
	{
#ifdef WINDOWS_ENABLED
		std::lock_guard<std::mutex> lk(cg_splash_stage_mutex);
#endif
		cg_splash_last_card = card;
	}

#ifdef WINDOWS_ENABLED
	if (cg_splash_hwnd && card.is_valid()) {
		_cg_update_layered_splash(card, 255);
		_cg_pump_splash_hwnd_messages();
	}
#endif
	poll_watchdog();
}
