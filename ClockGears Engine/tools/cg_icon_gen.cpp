#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "png.h"

struct ImageRGBA {
	int w = 0;
	int h = 0;
	std::vector<uint8_t> data; // RGBA8 top-to-bottom
};

static bool load_png(const char *path, ImageRGBA &out) {
	FILE *fp = std::fopen(path, "rb");
	if (!fp) {
		std::fprintf(stderr, "Error abriendo PNG: %s\n", path);
		return false;
	}
	png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	png_infop info = png ? png_create_info_struct(png) : nullptr;
	if (!png || !info || setjmp(png_jmpbuf(png))) {
		if (png) png_destroy_read_struct(&png, info ? &info : nullptr, nullptr);
		std::fclose(fp);
		return false;
	}
	png_init_io(png, fp);
	png_read_info(png, info);

	out.w = (int)png_get_image_width(png, info);
	out.h = (int)png_get_image_height(png, info);
	png_byte color_type = png_get_color_type(png, info);
	png_byte bit_depth = png_get_bit_depth(png, info);

	if (bit_depth == 16) png_set_strip_16(png);
	if (color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png);
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) png_set_expand_gray_1_2_4_to_8(png);
	if (png_get_valid(png, info, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png);
	if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
	if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png);

	png_read_update_info(png, info);
	out.data.resize((size_t)out.w * out.h * 4);
	std::vector<png_bytep> rows(out.h);
	for (int y = 0; y < out.h; y++) {
		rows[y] = out.data.data() + (size_t)y * out.w * 4;
	}
	png_read_image(png, rows.data());
	png_destroy_read_struct(&png, &info, nullptr);
	std::fclose(fp);
	return true;
}

static bool save_png_bytes(const ImageRGBA &img, std::vector<uint8_t> &out_bytes) {
	png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	png_infop info = png ? png_create_info_struct(png) : nullptr;
	if (!png || !info || setjmp(png_jmpbuf(png))) {
		if (png) png_destroy_write_struct(&png, info ? &info : nullptr);
		return false;
	}
	out_bytes.clear();
	png_set_write_fn(png, &out_bytes,
		[](png_structp p, png_bytep data, png_size_t len) {
			auto *vec = reinterpret_cast<std::vector<uint8_t> *>(png_get_io_ptr(p));
			vec->insert(vec->end(), data, data + len);
		},
		[](png_structp) {});
	png_set_IHDR(png, info, img.w, img.h, 8, PNG_COLOR_TYPE_RGBA,
		PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_write_info(png, info);
	std::vector<png_bytep> rows(img.h);
	for (int y = 0; y < img.h; y++) {
		rows[y] = const_cast<png_bytep>(img.data.data() + (size_t)y * img.w * 4);
	}
	png_write_image(png, rows.data());
	png_write_end(png, nullptr);
	png_destroy_write_struct(&png, &info);
	return true;
}

static bool write_png_file(const char *path, const ImageRGBA &img) {
	std::vector<uint8_t> buf;
	if (!save_png_bytes(img, buf)) return false;
	FILE *fp = std::fopen(path, "wb");
	if (!fp) return false;
	size_t written = std::fwrite(buf.data(), 1, buf.size(), fp);
	std::fclose(fp);
	return written == buf.size();
}

static ImageRGBA scale_nearest(const ImageRGBA &src, int dst_w, int dst_h) {
	ImageRGBA dst;
	dst.w = dst_w;
	dst.h = dst_h;
	dst.data.resize((size_t)dst_w * dst_h * 4);
	for (int y = 0; y < dst_h; y++) {
		int sy = std::min(src.h - 1, (y * src.h) / dst_h);
		for (int x = 0; x < dst_w; x++) {
			int sx = std::min(src.w - 1, (x * src.w) / dst_w);
			const uint8_t *sp = &src.data[((size_t)sy * src.w + sx) * 4];
			uint8_t *dp = &dst.data[((size_t)y * dst_w + x) * 4];
			dp[0] = sp[0];
			dp[1] = sp[1];
			dp[2] = sp[2];
			dp[3] = sp[3];
		}
	}
	return dst;
}

static int color_luma(uint32_t rgba) {
	int r = (rgba >> 24) & 0xFF;
	int g = (rgba >> 16) & 0xFF;
	int b = (rgba >> 8) & 0xFF;
	return (299 * r + 587 * g + 114 * b) / 1000;
}

static int color_chroma(uint32_t rgba) {
	int r = (rgba >> 24) & 0xFF;
	int g = (rgba >> 16) & 0xFF;
	int b = (rgba >> 8) & 0xFF;
	return std::max({r, g, b}) - std::min({r, g, b});
}

// Crisp pixel-art resampling: integer multiples use exact nearest; reductions (16, 24) use palette-mode sampling
// with contrast/center tie-break so 1px copper frames, gears, and rivets remain sharp without blur.
static ImageRGBA resize_pixel_art(const ImageRGBA &src, int dst_size) {
	if (dst_size >= src.w && dst_size >= src.h && (dst_size % src.w) == 0 && (dst_size % src.h) == 0) {
		return scale_nearest(src, dst_size, dst_size);
	}
	if (dst_size > src.w && dst_size > src.h) {
		// e.g. 32 -> 48 (1.5x upscale): nearest-neighbor keeps exact palette colors without blur
		return scale_nearest(src, dst_size, dst_size);
	}

	ImageRGBA dst;
	dst.w = dst_size;
	dst.h = dst_size;
	dst.data.assign((size_t)dst_size * dst_size * 4, 0);

	for (int dy = 0; dy < dst_size; dy++) {
		int y0 = (dy * src.h) / dst_size;
		int y1 = std::max(y0 + 1, ((dy + 1) * src.h) / dst_size);
		y1 = std::min(src.h, y1);
		for (int dx = 0; dx < dst_size; dx++) {
			int x0 = (dx * src.w) / dst_size;
			int x1 = std::max(x0 + 1, ((dx + 1) * src.w) / dst_size);
			x1 = std::min(src.w, x1);

			int total = 0;
			int opaque_cnt = 0;
			std::map<uint32_t, int> freq;
			int cx = (x0 + x1) / 2;
			int cy = (y0 + y1) / 2;
			uint32_t center_rgba = 0;
			int sum_a = 0;

			for (int sy = y0; sy < y1; sy++) {
				for (int sx = x0; sx < x1; sx++) {
					const uint8_t *p = &src.data[((size_t)sy * src.w + sx) * 4];
					total++;
					sum_a += p[3];
					uint32_t key = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
					if (sx == cx && sy == cy) center_rgba = key;
					if (p[3] >= 64) {
						opaque_cnt++;
						freq[key]++;
					}
				}
			}

			uint8_t *dp = &dst.data[((size_t)dy * dst_size + dx) * 4];
			if (opaque_cnt * 2 < total) {
				dp[0] = dp[1] = dp[2] = dp[3] = 0;
			} else {
				uint32_t best_key = center_rgba;
				int best_score = -1;
				for (const auto &kv : freq) {
					// Score = frequency * 100 + slight preference for saturated copper/bright gleam + center pixel
					int score = kv.second * 100 + (color_chroma(kv.first) > 25 ? 20 : 0) + (kv.first == center_rgba ? 15 : 0);
					if (score > best_score || (score == best_score && color_luma(kv.first) > color_luma(best_key))) {
						best_score = score;
						best_key = kv.first;
					}
				}
				dp[0] = (best_key >> 24) & 0xFF;
				dp[1] = (best_key >> 16) & 0xFF;
				dp[2] = (best_key >> 8) & 0xFF;
				dp[3] = (uint8_t)std::min(255, std::max((int)(best_key & 0xFF), sum_a / total));
			}
		}
	}
	return dst;
}

static void append_u16(std::vector<uint8_t> &b, uint16_t v) {
	b.push_back(v & 0xFF);
	b.push_back((v >> 8) & 0xFF);
}

static void append_u32(std::vector<uint8_t> &b, uint32_t v) {
	b.push_back(v & 0xFF);
	b.push_back((v >> 8) & 0xFF);
	b.push_back((v >> 16) & 0xFF);
	b.push_back((v >> 24) & 0xFF);
}

static std::vector<uint8_t> make_ico_dib(const ImageRGBA &img) {
	std::vector<uint8_t> out;
	const int w = img.w;
	const int h = img.h;
	const int xor_bytes = w * h * 4;
	const int and_stride = ((w + 31) / 32) * 4;
	const int and_bytes = and_stride * h;

	append_u32(out, 40);
	append_u32(out, (uint32_t)w);
	append_u32(out, (uint32_t)(h * 2));
	append_u16(out, 1);
	append_u16(out, 32);
	append_u32(out, 0);
	append_u32(out, (uint32_t)(xor_bytes + and_bytes));
	append_u32(out, 0);
	append_u32(out, 0);
	append_u32(out, 0);
	append_u32(out, 0);

	for (int y = h - 1; y >= 0; y--) {
		for (int x = 0; x < w; x++) {
			const uint8_t *p = &img.data[((size_t)y * w + x) * 4];
			out.push_back(p[2]);
			out.push_back(p[1]);
			out.push_back(p[0]);
			out.push_back(p[3]);
		}
	}
	for (int y = h - 1; y >= 0; y--) {
		for (int b = 0; b < and_stride; b++) {
			uint8_t byte_val = 0;
			for (int bit = 0; bit < 8; bit++) {
				int x = b * 8 + bit;
				if (x < w) {
					uint8_t a = img.data[((size_t)y * w + x) * 4 + 3];
					if (a < 128) byte_val |= (uint8_t)(0x80 >> bit);
				}
			}
			out.push_back(byte_val);
		}
	}
	return out;
}

static bool write_ico(const char *path, const std::vector<ImageRGBA> &imgs) {
	std::vector<std::vector<uint8_t>> payloads(imgs.size());
	for (size_t i = 0; i < imgs.size(); i++) {
		if (imgs[i].w >= 256 || imgs[i].h >= 256) {
			if (!save_png_bytes(imgs[i], payloads[i])) return false;
		} else {
			payloads[i] = make_ico_dib(imgs[i]);
		}
	}

	std::vector<uint8_t> ico;
	append_u16(ico, 0);
	append_u16(ico, 1);
	append_u16(ico, (uint16_t)imgs.size());

	uint32_t offset = 6 + (uint32_t)imgs.size() * 16;
	for (size_t i = 0; i < imgs.size(); i++) {
		ico.push_back(imgs[i].w >= 256 ? 0 : (uint8_t)imgs[i].w);
		ico.push_back(imgs[i].h >= 256 ? 0 : (uint8_t)imgs[i].h);
		ico.push_back(0);
		ico.push_back(0);
		append_u16(ico, 1);
		append_u16(ico, 32);
		append_u32(ico, (uint32_t)payloads[i].size());
		append_u32(ico, offset);
		offset += (uint32_t)payloads[i].size();
	}
	for (size_t i = 0; i < payloads.size(); i++) {
		ico.insert(ico.end(), payloads[i].begin(), payloads[i].end());
	}

	FILE *fp = std::fopen(path, "wb");
	if (!fp) return false;
	size_t w = std::fwrite(ico.data(), 1, ico.size(), fp);
	std::fclose(fp);
	return w == ico.size();
}

// 5x7 crisp pixel font for preview labels ("16", "24", "32", "48", "64", "128", "256")
static void draw_digit_char(ImageRGBA &canvas, int ox, int oy, char ch, uint8_t r, uint8_t g, uint8_t b) {
	static const uint8_t glyphs[12][7] = {
		{0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}, // 0
		{0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}, // 1
		{0x0E,0x11,0x01,0x06,0x08,0x10,0x1F}, // 2
		{0x0E,0x11,0x01,0x06,0x01,0x11,0x0E}, // 3
		{0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}, // 4
		{0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E}, // 5
		{0x06,0x08,0x10,0x1E,0x11,0x11,0x0E}, // 6
		{0x1F,0x01,0x02,0x04,0x08,0x08,0x08}, // 7
		{0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E}, // 8
		{0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C}, // 9
		{0x00,0x00,0x1E,0x11,0x1E,0x10,0x10}, // p
		{0x00,0x00,0x11,0x0A,0x04,0x0A,0x11}, // x
	};
	int gi = -1;
	if (ch >= '0' && ch <= '9') gi = ch - '0';
	else if (ch == 'p') gi = 10;
	else if (ch == 'x') gi = 11;
	if (gi < 0) return;

	for (int row = 0; row < 7; row++) {
		for (int col = 0; col < 5; col++) {
			if (glyphs[gi][row] & (1 << (4 - col))) {
				for (int sy = 0; sy < 2; sy++) {
					for (int sx = 0; sx < 2; sx++) {
						int px = ox + col * 2 + sx;
						int py = oy + row * 2 + sy;
						if (px >= 0 && px < canvas.w && py >= 0 && py < canvas.h) {
							uint8_t *dp = &canvas.data[((size_t)py * canvas.w + px) * 4];
							dp[0] = r; dp[1] = g; dp[2] = b; dp[3] = 255;
						}
					}
				}
			}
		}
	}
}

static void draw_label(ImageRGBA &canvas, int cx, int y, const std::string &txt) {
	int w = (int)txt.size() * 12;
	int x = cx - w / 2;
	for (size_t i = 0; i < txt.size(); i++) {
		draw_digit_char(canvas, x + (int)i * 12, y, txt[i], 246, 236, 217);
	}
}

static void blit_checkerboard_and_image(ImageRGBA &canvas, int ox, int oy, const ImageRGBA &img) {
	for (int y = -2; y < img.h + 2; y++) {
		for (int x = -2; x < img.w + 2; x++) {
			int cx = ox + x;
			int cy = oy + y;
			if (cx < 0 || cy < 0 || cx >= canvas.w || cy >= canvas.h) continue;
			uint8_t *dp = &canvas.data[((size_t)cy * canvas.w + cx) * 4];
			if (x < 0 || y < 0 || x >= img.w || y >= img.h) {
				dp[0] = 201; dp[1] = 146; dp[2] = 62; dp[3] = 255;
			} else {
				bool dark = (((x / 4) + (y / 4)) & 1) == 0;
				uint8_t bg_r = dark ? 24 : 34;
				uint8_t bg_g = dark ? 26 : 36;
				uint8_t bg_b = dark ? 32 : 44;
				const uint8_t *sp = &img.data[((size_t)y * img.w + x) * 4];
				float a = sp[3] / 255.0f;
				dp[0] = (uint8_t)std::round(sp[0] * a + bg_r * (1.0f - a));
				dp[1] = (uint8_t)std::round(sp[1] * a + bg_g * (1.0f - a));
				dp[2] = (uint8_t)std::round(sp[2] * a + bg_b * (1.0f - a));
				dp[3] = 255;
			}
		}
	}
}

static bool write_crisp_svg(const char *path, const ImageRGBA &src) {
	std::ostringstream ss;
	ss << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"128\" height=\"128\" viewBox=\"0 0 "
	   << src.w << " " << src.h << "\" shape-rendering=\"crispEdges\">";
	for (int y = 0; y < src.h; y++) {
		int x = 0;
		while (x < src.w) {
			const uint8_t *p = &src.data[((size_t)y * src.w + x) * 4];
			if (p[3] == 0) {
				x++;
				continue;
			}
			int run = 1;
			while (x + run < src.w) {
				const uint8_t *q = &src.data[((size_t)y * src.w + x + run) * 4];
				if (q[0] == p[0] && q[1] == p[1] && q[2] == p[2] && q[3] == p[3]) {
					run++;
				} else {
					break;
				}
			}
			char hex[10];
			std::snprintf(hex, sizeof(hex), "#%02x%02x%02x", p[0], p[1], p[2]);
			ss << "<path fill=\"" << hex << "\"";
			if (p[3] < 255) {
				char al[16];
				std::snprintf(al, sizeof(al), "%.3f", p[3] / 255.0);
				ss << " fill-opacity=\"" << al << "\"";
			}
			ss << " d=\"M" << x << " " << y << "h" << run << "v1h-" << run << "z\"/>";
			x += run;
		}
	}
	ss << "</svg>";
	std::ofstream ofs(path, std::ios::binary);
	if (!ofs) return false;
	ofs << ss.str();
	return ofs.good();
}

static bool patch_splash_with_icon(const char *src_splash_path, const char *dst_splash_path, const ImageRGBA &icon_128) {
	ImageRGBA splash;
	if (!load_png(src_splash_path, splash)) return false;
	// Reemplaza la zona central antigua (~160x160 alrededor del centro horizontal, y=100..270) con el nuevo icono 128x128 nítido
	int cx = (splash.w - icon_128.w) / 2;
	int cy = 104;
	// Limpiar primero el rectángulo anterior con el color de fondo del píxel superior de splash
	uint8_t bg_r = splash.data[0], bg_g = splash.data[1], bg_b = splash.data[2];
	for (int y = 92; y < 276 && y < splash.h; y++) {
		for (int x = splash.w / 2 - 110; x < splash.w / 2 + 110; x++) {
			if (x >= 0 && x < splash.w) {
				uint8_t *dp = &splash.data[((size_t)y * splash.w + x) * 4];
				dp[0] = bg_r; dp[1] = bg_g; dp[2] = bg_b; dp[3] = 255;
			}
		}
	}
	for (int y = 0; y < icon_128.h; y++) {
		for (int x = 0; x < icon_128.w; x++) {
			int dx = cx + x;
			int dy = cy + y;
			if (dx < 0 || dy < 0 || dx >= splash.w || dy >= splash.h) continue;
			const uint8_t *sp = &icon_128.data[((size_t)y * icon_128.w + x) * 4];
			uint8_t *dp = &splash.data[((size_t)dy * splash.w + dx) * 4];
			float a = sp[3] / 255.0f;
			dp[0] = (uint8_t)std::round(sp[0] * a + dp[0] * (1.0f - a));
			dp[1] = (uint8_t)std::round(sp[1] * a + dp[1] * (1.0f - a));
			dp[2] = (uint8_t)std::round(sp[2] * a + dp[2] * (1.0f - a));
			dp[3] = 255;
		}
	}
	return write_png_file(dst_splash_path, splash);
}

int main(int argc, char **argv) {
	if (argc < 2) {
		std::fprintf(stderr, "Uso: cg_icon_gen <input.png> [out.ico] [out_preview.png] [out_128.png] [out_256.png] [out_svg] [out_splash.png]\n");
		return 1;
	}
	ImageRGBA src;
	if (!load_png(argv[1], src)) {
		return 2;
	}
	std::printf("ORIGINAL=%dx%d SQUARE=%s\n", src.w, src.h, (src.w == src.h) ? "SI" : "NO");

	if (argc >= 3) {
		const int sizes[7] = { 16, 24, 32, 48, 64, 128, 256 };
		std::vector<ImageRGBA> variants;
		for (int sz : sizes) {
			variants.push_back(resize_pixel_art(src, sz));
		}
		if (!write_ico(argv[2], variants)) {
			std::fprintf(stderr, "Error escribiendo ICO: %s\n", argv[2]);
			return 3;
		}
		std::printf("ICO_WRITTEN=%s (sizes=16,24,32,48,64,128,256)\n", argv[2]);

		if (argc >= 4) {
			int total_w = 24;
			for (int sz : sizes) total_w += std::max(sz, 48) + 22;
			int total_h = 256 + 72;
			ImageRGBA sheet;
			sheet.w = total_w;
			sheet.h = total_h;
			sheet.data.resize((size_t)total_w * total_h * 4);
			for (int i = 0; i < total_w * total_h; i++) {
				sheet.data[i * 4 + 0] = 23;
				sheet.data[i * 4 + 1] = 21;
				sheet.data[i * 4 + 2] = 18;
				sheet.data[i * 4 + 3] = 255;
			}
			int cur_x = 20;
			for (size_t i = 0; i < variants.size(); i++) {
				int slot_w = std::max(variants[i].w, 48);
				int ox = cur_x + (slot_w - variants[i].w) / 2;
				int oy = (256 - variants[i].h) / 2 + 16;
				blit_checkerboard_and_image(sheet, ox, oy, variants[i]);
				draw_label(sheet, cur_x + slot_w / 2, 286, std::to_string(sizes[i]) + "px");
				cur_x += slot_w + 22;
			}
			if (!write_png_file(argv[3], sheet)) return 4;
			std::printf("PREVIEW_WRITTEN=%s (%dx%d)\n", argv[3], sheet.w, sheet.h);
		}
		if (argc >= 5 && !write_png_file(argv[4], variants[5])) return 5;
		if (argc >= 6 && !write_png_file(argv[5], variants[6])) return 6;
		if (argc >= 7 && !write_crisp_svg(argv[6], src)) return 7;
		if (argc >= 8 && !patch_splash_with_icon(argv[7], argv[7], variants[5])) return 8;
		if (argc >= 9 && !patch_splash_with_icon(argv[8], argv[8], variants[5])) return 9;
	}
	return 0;
}
