#pragma once

#include "core/math/color.h"
#include "core/object/class_db.h"
#include "core/object/object.h"
#include "core/templates/hash_map.h"
#include "scene/resources/texture.h"

class ClockGearsIcons : public Object {
	GDCLASS(ClockGearsIcons, Object);

private:
	static ClockGearsIcons *singleton;
	mutable HashMap<String, Ref<Texture2D>> icon_cache;
	mutable HashMap<String, String> svg_sources;
	mutable Ref<Texture2D> fallback_icon;

	void _register_builtin_svgs();
	Ref<Texture2D> _get_fallback_icon() const;
	Ref<Texture2D> _rasterize_svg(const String &p_svg, float p_scale = 1.0f) const;

protected:
	static void _bind_methods();

public:
	static ClockGearsIcons *get_singleton() { return singleton; }

	Ref<Texture2D> get_icon(const String &p_name, float p_scale = 1.0f) const;
	Ref<Texture2D> get_db_category_icon(int p_category, float p_scale = 1.0f) const;
	Ref<Texture2D> get_command_category_icon(const String &p_category, float p_scale = 1.0f) const;
	Color get_command_category_color(const String &p_category) const;

	void clear_cache();

	ClockGearsIcons();
	~ClockGearsIcons();
};
