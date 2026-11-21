#include "clockgears_boot_splash.h"

#include "core/clockgears_version.h"
#include "core/config/project_settings.h"
#include "core/math/math_funcs.h"
#include "core/os/os.h"
#include "main/app_icon.gen.h"
#include "modules/modules_enabled.gen.h"
#include "servers/display/display_server.h"
#include "servers/rendering/rendering_server.h"

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

void _build_static_base(Vector<uint8_t> &r_buf, const ClockGearsBootSplash::Palette &p_pal, ClockGearsBootSplash::LayoutMetrics &r_metrics) {
	const int W = ClockGearsBootSplash::SPLASH_WIDTH;
	const int H = ClockGearsBootSplash::SPLASH_HEIGHT;
	r_buf.resize(W * H * 4);
	uint8_t *dst = r_buf.ptrw();

	// 1. Fondo con degradado suave vertical y brillo radial calido central.
	const float halo_cx = 400.0f;
	const float halo_cy = 148.0f;
	const float halo_inv_r = 1.0f / 145.0f;
	for (int y = 0; y < H; y++) {
		const float vt = (float)y / (float)(H - 1);
		const Color row_bg = p_pal.bg_top.lerp(p_pal.bg_bottom, vt);
		for (int x = 0; x < W; x++) {
			const float dx = ((float)x + 0.5f - halo_cx) * halo_inv_r;
			const float dy = ((float)y + 0.5f - halo_cy) * halo_inv_r;
			const float d2 = dx * dx + dy * dy;
			const float halo = expf(-2.65f * d2) * p_pal.halo_amber.a;
			const float c_r = _clamp01(row_bg.r + p_pal.halo_amber.r * halo);
			const float c_g = _clamp01(row_bg.g + p_pal.halo_amber.g * halo);
			const float c_b = _clamp01(row_bg.b + p_pal.halo_amber.b * halo);
			uint8_t *px = dst + ((y * W + x) * 4);
			px[0] = (uint8_t)Math::round(c_r * 255.0f);
			px[1] = (uint8_t)Math::round(c_g * 255.0f);
			px[2] = (uint8_t)Math::round(c_b * 255.0f);
			px[3] = 255;
		}
	}

	// Anillos concentricos tenues alrededor del halo del icono.
	_draw_aa_ring(dst, W, H, halo_cx, halo_cy, 84.0f, 1.0f, Color(p_pal.accent_amber.r, p_pal.accent_amber.g, p_pal.accent_amber.b, 0.16f));
	_draw_aa_ring(dst, W, H, halo_cx, halo_cy, 114.0f, 1.0f, Color(p_pal.accent_amber.r, p_pal.accent_amber.g, p_pal.accent_amber.b, 0.07f));

	// 2. Decoracion de engranajes y trazos de circuito en baja opacidad en las esquinas.
	const Color orn_soft = p_pal.ornament_copper;
	const Color orn_subtle(orn_soft.r, orn_soft.g, orn_soft.b, orn_soft.a * 0.72f);

	// Esquina superior izquierda
	_draw_ornament_gear(dst, W, H, 36.0f, 36.0f, 76.0f, 20.0f, 12, 0.25f, 1.3f, orn_soft);
	_draw_circuit_trace(dst, W, H, { Vector2(106, 26), Vector2(168, 26), Vector2(190, 48), Vector2(238, 48) }, 1.2f, orn_subtle, false, true);
	_draw_circuit_trace(dst, W, H, { Vector2(26, 108), Vector2(26, 152), Vector2(46, 172), Vector2(46, 212) }, 1.2f, orn_subtle, false, true);

	// Esquina superior derecha
	_draw_ornament_gear(dst, W, H, 770.0f, 44.0f, 64.0f, 16.0f, 10, 0.65f, 1.2f, Color(orn_soft.r, orn_soft.g, orn_soft.b, orn_soft.a * 0.75f));
	_draw_circuit_trace(dst, W, H, { Vector2(772, 106), Vector2(772, 154), Vector2(752, 174), Vector2(752, 220) }, 1.2f, orn_subtle, false, true);

	// Esquina inferior izquierda (sobre el area de barra para cero interferencia)
	_draw_ornament_gear(dst, W, H, 34.0f, 356.0f, 52.0f, 14.0f, 10, 0.40f, 1.15f, Color(orn_soft.r, orn_soft.g, orn_soft.b, orn_soft.a * 0.70f), 382.0f);
	_draw_circuit_trace(dst, W, H, { Vector2(84, 346), Vector2(142, 346), Vector2(162, 326), Vector2(194, 326) }, 1.15f, orn_subtle, true, true);

	// Esquina inferior derecha
	_draw_circuit_trace(dst, W, H, { Vector2(758, 352), Vector2(704, 352), Vector2(684, 332), Vector2(648, 332) }, 1.15f, orn_subtle, true, true);

	// 3. Icono central (~128 px sin filtro suave, 1:1 desde app_icon_png).
	Ref<Image> icon = memnew(Image(app_icon_png));
	if (icon.is_valid() && !icon->is_empty()) {
		if (icon->get_format() != Image::FORMAT_RGBA8) {
			icon->convert(Image::FORMAT_RGBA8);
		}
		if (icon->get_width() != 128 || icon->get_height() != 128) {
			icon->resize(128, 128, Image::INTERPOLATE_NEAREST);
		}
		const int ix0 = (W - 128) / 2;
		const int iy0 = 84;
		r_metrics.icon_rect = Rect2i(ix0, iy0, 128, 128);
		const uint8_t *isrc = icon->get_data().ptr();
		for (int sy = 0; sy < 128; sy++) {
			for (int sx = 0; sx < 128; sx++) {
				const uint8_t *sp = isrc + ((sy * 128 + sx) * 4);
				if (sp[3] > 0) {
					const Color c((float)sp[0] / 255.0f, (float)sp[1] / 255.0f, (float)sp[2] / 255.0f, (float)sp[3] / 255.0f);
					_blend_pixel_rgba(dst, W, H, ix0 + sx, iy0 + sy, c, 1.0f);
				}
			}
		}
	}

	// 4. Tipografías del tema: pastilla superior derecha v0.0.4 ALPHA + titulos centrados.
	SplashFontCache &fc = get_font_cache();

	const String ver_txt = String::utf8(CLOCKGEARS_SPLASH_VERSION);
	const int ver_w = fc.measure_text_width(ver_txt, 11, 0.8f);
	const int pill_w = ver_w + 20;
	const int pill_h = 22;
	const int pill_x = W - 22 - pill_w;
	const int pill_y = 20;
	r_metrics.badge_rect = Rect2i(pill_x, pill_y, pill_w, pill_h);
	_draw_filled_rounded_rect(dst, W, H, (float)pill_x, (float)pill_y, (float)pill_w, (float)pill_h, 4.0f,
			Color(p_pal.badge_bg.r, p_pal.badge_bg.g, p_pal.badge_bg.b, 0.88f),
			Color(p_pal.card_border.r, p_pal.card_border.g, p_pal.card_border.b, 0.62f), 1.0f);
	(void)fc.draw_text(dst, W, H, ver_txt, pill_x + 10, pill_y + 15, 11, p_pal.accent_amber, 0.8f, true);

	const String title_txt = String::utf8("CLOCKGEARS ENGINE");
	const int title_w = fc.measure_text_width(title_txt, 27, 2.0f);
	r_metrics.title_rect = fc.draw_text(dst, W, H, title_txt, (W - title_w) / 2, 248, 27, p_pal.accent_amber, 2.0f, true);

	const String sub_txt = String::utf8("NEXT-GEN RPG CREATION SYSTEM");
	const int sub_w = fc.measure_text_width(sub_txt, 11, 2.8f);
	r_metrics.subtitle_rect = fc.draw_text(dst, W, H, sub_txt, (W - sub_w) / 2, 274, 11, p_pal.text_secondary, 2.8f, false);

	// Linea separadora cobriza sutil centrada.
	for (int x = 305; x <= 495; x++) {
		const float t = 1.0f - fabsf((float)x - 400.0f) / 95.0f;
		_blend_pixel_rgba(dst, W, H, x, 292, p_pal.card_border, 0.45f * t);
	}

	const String cred_txt = String::utf8("Created by Kris'ZtS (ClockGears Studio)");
	const int cred_w = fc.measure_text_width(cred_txt, 12, 0.3f);
	const Color cred_col = p_pal.text_primary.lerp(p_pal.text_secondary, 0.35f);
	r_metrics.credits_rect = fc.draw_text(dst, W, H, cred_txt, (W - cred_w) / 2, 316, 12, cred_col, 0.3f, false);
}

static bool cg_splash_base_valid = false;
static bool cg_splash_base_brass = false;
static Vector<uint8_t> cg_splash_base_bytes;
static ClockGearsBootSplash::LayoutMetrics cg_splash_cached_metrics;

} // namespace

bool ClockGearsBootSplash::is_saved_brass_theme() {
	if (ProjectSettings::get_singleton() && ProjectSettings::get_singleton()->has_setting("clockgears/editor/theme_id")) {
		const String tid = String(ProjectSettings::get_singleton()->get_setting("clockgears/editor/theme_id")).strip_edges().to_lower();
		return (tid == "brass" || tid == "laton" || tid == String::utf8("latón"));
	}
	return false;
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

Ref<Image> ClockGearsBootSplash::render_card(float p_progress, const String &p_status, float p_spinner_deg, bool p_completed, bool p_brass, LayoutMetrics *r_metrics) {
	const Palette pal = get_active_palette(p_brass, true);
	if (!cg_splash_base_valid || cg_splash_base_brass != p_brass || cg_splash_base_bytes.size() != SPLASH_WIDTH * SPLASH_HEIGHT * 4) {
		cg_splash_cached_metrics = LayoutMetrics();
		cg_splash_cached_metrics.card_rect = Rect2i(0, 0, SPLASH_WIDTH, SPLASH_HEIGHT);
		cg_splash_cached_metrics.corner_radius = pal.corner_radius;
		_build_static_base(cg_splash_base_bytes, pal, cg_splash_cached_metrics);
		cg_splash_base_brass = p_brass;
		cg_splash_base_valid = true;
	}

	Vector<uint8_t> frame_bytes = cg_splash_base_bytes;
	uint8_t *dst = frame_bytes.ptrw();
	LayoutMetrics metrics = cg_splash_cached_metrics;

	// 5. Zona inferior: texto de fase a la izquierda, marca + indicador animado a la derecha, barra ambar fina debajo.
	SplashFontCache &fc = get_font_cache();
	const int pad_x = 28;
	const int status_baseline = 403;
	const int bar_x = pad_x;
	const int bar_y = 416;
	const int bar_w = SPLASH_WIDTH - pad_x * 2;
	const int bar_h = 5;

	const Color status_col = p_completed ? pal.accent_amber : pal.text_primary;
	metrics.status_rect = fc.draw_text(dst, SPLASH_WIDTH, SPLASH_HEIGHT, p_status, pad_x, status_baseline, 12, status_col, 0.2f, p_completed);

	// Indicador animado / estado abajo a la derecha.
	const float spin_cx = (float)(SPLASH_WIDTH - pad_x - 8);
	const float spin_cy = 398.5f;
	const float spin_r = 6.2f;
	if (p_completed) {
		_draw_aa_ring(dst, SPLASH_WIDTH, SPLASH_HEIGHT, spin_cx, spin_cy, spin_r, 1.5f, pal.accent_amber);
		_draw_aa_disc(dst, SPLASH_WIDTH, SPLASH_HEIGHT, spin_cx, spin_cy, 2.5f, pal.accent_amber);
	} else {
		_draw_aa_ring(dst, SPLASH_WIDTH, SPLASH_HEIGHT, spin_cx, spin_cy, spin_r, 1.4f, Color(pal.accent_amber.r, pal.accent_amber.g, pal.accent_amber.b, 0.25f));
		const float rad0 = Math::deg_to_rad(p_spinner_deg);
		const int arc_segs = 18;
		const float arc_span = 2.6f;
		for (int s = 0; s < arc_segs; s++) {
			const float t0 = rad0 + (arc_span * (float)s) / (float)arc_segs;
			const float t1 = rad0 + (arc_span * (float)(s + 1)) / (float)arc_segs;
			const float fade = (float)(s + 1) / (float)arc_segs;
			_draw_aa_line(dst, SPLASH_WIDTH, SPLASH_HEIGHT,
					spin_cx + cosf(t0) * spin_r, spin_cy + sinf(t0) * spin_r,
					spin_cx + cosf(t1) * spin_r, spin_cy + sinf(t1) * spin_r,
					1.6f, Color(pal.accent_amber.r, pal.accent_amber.g, pal.accent_amber.b, fade));
		}
	}

	const String right_txt = String::utf8("ClockGears Engine");
	const int right_w = fc.measure_text_width(right_txt, 11, 0.2f);
	const int right_x = (int)spin_cx - 14 - right_w;
	metrics.right_brand_rect = fc.draw_text(dst, SPLASH_WIDTH, SPLASH_HEIGHT, right_txt, right_x, status_baseline, 11, pal.text_secondary, 0.2f, false);

	// Barra de progreso fina ambar.
	metrics.progress_bar_rect = Rect2i(bar_x, bar_y, bar_w, bar_h);
	_draw_filled_rounded_rect(dst, SPLASH_WIDTH, SPLASH_HEIGHT, (float)bar_x, (float)bar_y, (float)bar_w, (float)bar_h, 2.5f,
			pal.progress_track, Color(pal.card_border.r, pal.card_border.g, pal.card_border.b, 0.28f), 1.0f);

	const float frac = _clamp01(p_progress);
	if (frac > 0.001f) {
		const float fill_w = MAX(5.0f, (float)bar_w * frac);
		const Color fill_hi = pal.progress_fill.lightened(0.18f);
		for (int y = bar_y; y < bar_y + bar_h; y++) {
			for (int x = bar_x; x < (int)ceilf((float)bar_x + fill_w); x++) {
				const float d = _sd_rounded_box((float)x + 0.5f, (float)y + 0.5f, (float)bar_x, (float)bar_y, fill_w, (float)bar_h, 2.5f);
				if (d < 1.0f) {
					const float cov = _clamp01(1.0f - d);
					const Color bar_c = (y == bar_y) ? fill_hi : pal.progress_fill;
					_blend_pixel_rgba(dst, SPLASH_WIDTH, SPLASH_HEIGHT, x, y, bar_c, cov);
				}
			}
		}
	}

	// 6. Esquinas suavemente redondeadas y borde cobrizo fino de 1 px sobre toda la tarjeta.
	const float r_card = (float)pal.corner_radius;
	const float bw_card = (float)pal.border_width;
	for (int y = 0; y < SPLASH_HEIGHT; y++) {
		for (int x = 0; x < SPLASH_WIDTH; x++) {
			// Salto rapido del interior seguro
			if (x >= (int)r_card + 3 && x < SPLASH_WIDTH - (int)r_card - 3 &&
					y >= (int)r_card + 3 && y < SPLASH_HEIGHT - (int)r_card - 3) {
				continue;
			}
			const float d = _sd_rounded_box((float)x + 0.5f, (float)y + 0.5f, 0.0f, 0.0f, (float)SPLASH_WIDTH, (float)SPLASH_HEIGHT, r_card);
			uint8_t *px = dst + ((y * SPLASH_WIDTH + x) * 4);
			if (d >= 0.6f) {
				// Fuera de la esquina redondeada: transparente para la tarjeta.
				px[3] = 0;
			} else if (d > -bw_card - 0.8f) {
				const float outer_cov = _clamp01(0.6f - d);
				const float border_cov = _clamp01(1.0f - fabsf(d + bw_card * 0.5f)) * outer_cov;
				_blend_pixel_rgba(dst, SPLASH_WIDTH, SPLASH_HEIGHT, x, y, pal.card_border, border_cov);
				px[3] = (uint8_t)Math::round(outer_cov * 255.0f);
			}
		}
	}

	if (r_metrics) {
		*r_metrics = metrics;
	}

	Ref<Image> img = Image::create_from_data(SPLASH_WIDTH, SPLASH_HEIGHT, false, Image::FORMAT_RGBA8, frame_bytes);
	return img;
}

void ClockGearsBootSplash::present_stage(int p_stage, int p_total_stages, const String &p_status, bool p_completed) {
	RenderingServer *rs = RenderingServer::get_singleton();
	if (!rs) {
		return;
	}
	const int total = MAX(1, p_total_stages);
	const int stage = CLAMP(p_stage, 0, total);
	const float progress = p_completed ? 1.0f : ((float)stage / (float)total);
	const int pct = (int)Math::round(progress * 100.0f);
	const String full_status = p_completed ? p_status : vformat(String::utf8("%s (%d %%)"), p_status, pct);
	const float spinner_deg = (float)(stage * 64 + (int)(OS::get_singleton()->get_ticks_msec() % 360));
	const bool is_brass = is_saved_brass_theme();
	const Palette pal = get_active_palette(is_brass, true);

	Ref<Image> card = render_card(progress, full_status, spinner_deg, p_completed, is_brass);
	if (card.is_valid()) {
		rs->set_boot_image_with_stretch(card, pal.bg_bottom, RSE::SPLASH_STRETCH_MODE_DISABLED, false);
	}
	if (DisplayServer::get_singleton()) {
		DisplayServer::get_singleton()->process_events();
	}
}
