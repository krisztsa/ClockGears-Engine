import os
import methods


def make_png_icons_header(target, source, env):
    entries = []
    seen = set()
    for src in sorted(source, key=lambda n: os.path.basename(str(n))):
        path = str(src)
        name = os.path.splitext(os.path.basename(path))[0]
        if name in seen:
            continue
        seen.add(name)
        buf = methods.get_buffer(path)
        entries.append((name, buf))

    with methods.generated_wrapper(str(target[0])) as file:
        file.write("#include <cstddef>\n\n")
        for name, buf in entries:
            file.write(f"inline constexpr unsigned char _cg_png_{name}[] = {{\n\t{methods.format_buffer(buf, 1)}\n}};\n\n")

        file.write("""\
struct ClockGearsEmbeddedPngIcon {
	const char *name;
	const unsigned char *data;
	size_t size;
};

""")
        file.write(f"inline constexpr int CLOCKGEARS_PNG_ICON_COUNT = {len(entries)};\n")
        file.write("inline constexpr ClockGearsEmbeddedPngIcon CLOCKGEARS_PNG_ICONS[] = {\n")
        for name, _ in entries:
            file.write(f'\t{{ "{name}", _cg_png_{name}, sizeof(_cg_png_{name}) }},\n')
        file.write("};\n")
