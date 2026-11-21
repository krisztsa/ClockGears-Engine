#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cwchar>
#include <string>
#include <vector>

#pragma pack(push, 1)
struct GRPICONDIR {
	uint16_t idReserved;
	uint16_t idType;
	uint16_t idCount;
};
struct GRPICONDIRENTRY {
	uint8_t bWidth;
	uint8_t bHeight;
	uint8_t bColorCount;
	uint8_t bReserved;
	uint16_t wPlanes;
	uint16_t wBitCount;
	uint32_t dwBytesInRes;
	uint16_t nID;
};
#pragma pack(pop)

int main(int argc, char **argv) {
	if (argc < 2) {
		std::fprintf(stderr, "Uso: cg_verify_exe_res <exe_path>\n");
		return 1;
	}
	int wlen = MultiByteToWideChar(CP_UTF8, 0, argv[1], -1, nullptr, 0);
	std::wstring wpath(wlen, 0);
	MultiByteToWideChar(CP_UTF8, 0, argv[1], -1, &wpath[0], wlen);

	HMODULE mod = LoadLibraryExW(wpath.c_str(), nullptr, LOAD_LIBRARY_AS_DATAFILE);
	if (!mod) {
		std::fprintf(stderr, "No se pudo cargar PE: %s (err=%lu)\n", argv[1], GetLastError());
		return 2;
	}

	HRSRC rgrp = FindResourceW(mod, L"GODOT_ICON", MAKEINTRESOURCEW(14)); // RT_GROUP_ICON = 14
	if (!rgrp) {
		std::fprintf(stderr, "RT_GROUP_ICON 'GODOT_ICON' no encontrado\n");
		FreeLibrary(mod);
		return 3;
	}
	HGLOBAL hg = LoadResource(mod, rgrp);
	const uint8_t *raw = (const uint8_t *)LockResource(hg);
	DWORD rsz = SizeofResource(mod, rgrp);
	if (!raw || rsz < sizeof(GRPICONDIR)) {
		FreeLibrary(mod);
		return 4;
	}
	const GRPICONDIR *dir = (const GRPICONDIR *)raw;
	std::printf("EXE_ICON_COUNT=%u\n", (unsigned)dir->idCount);
	std::printf("EXE_ICON_SIZES=");
	for (uint16_t i = 0; i < dir->idCount; i++) {
		const GRPICONDIRENTRY *e = (const GRPICONDIRENTRY *)(raw + sizeof(GRPICONDIR) + i * sizeof(GRPICONDIRENTRY));
		int w = (e->bWidth == 0) ? 256 : e->bWidth;
		int h = (e->bHeight == 0) ? 256 : e->bHeight;
		std::printf("%dx%d(%ub)%s", w, h, (unsigned)e->dwBytesInRes, (i + 1 < dir->idCount) ? "," : "\n");
	}

	// Inspeccionar VS_VERSION_INFO si existe
	DWORD ver_handle = 0;
	DWORD ver_sz = GetFileVersionInfoSizeW(wpath.c_str(), &ver_handle);
	if (ver_sz > 0) {
		std::vector<uint8_t> vbuf(ver_sz);
		if (GetFileVersionInfoW(wpath.c_str(), 0, ver_sz, vbuf.data())) {
			VS_FIXEDFILEINFO *ffi = nullptr;
			UINT ffi_len = 0;
			if (VerQueryValueW(vbuf.data(), L"\\", (LPVOID *)&ffi, &ffi_len) && ffi) {
				std::printf("FILEVERSION=%u.%u.%u.%u\n",
					HIWORD(ffi->dwFileVersionMS), LOWORD(ffi->dwFileVersionMS),
					HIWORD(ffi->dwFileVersionLS), LOWORD(ffi->dwFileVersionLS));
				std::printf("PRODUCTVERSION=%u.%u.%u.%u\n",
					HIWORD(ffi->dwProductVersionMS), LOWORD(ffi->dwProductVersionMS),
					HIWORD(ffi->dwProductVersionLS), LOWORD(ffi->dwProductVersionLS));
			}
			const char *keys[5] = { "ProductName", "CompanyName", "FileDescription", "LegalCopyright", "FileVersion" };
			for (const char *k : keys) {
				wchar_t q[128];
				std::swprintf(q, 128, L"\\StringFileInfo\\040904b0\\%S", k);
				LPWSTR val = nullptr;
				UINT vlen = 0;
				if (VerQueryValueW(vbuf.data(), q, (LPVOID *)&val, &vlen) && val) {
					int ulen = WideCharToMultiByte(CP_UTF8, 0, val, -1, nullptr, 0, nullptr, nullptr);
					std::string uval(ulen > 0 ? ulen - 1 : 0, 0);
					if (ulen > 1) WideCharToMultiByte(CP_UTF8, 0, val, -1, &uval[0], ulen, nullptr, nullptr);
					std::printf("%s=%s\n", k, uval.c_str());
				}
			}
		}
	}

	FreeLibrary(mod);
	return 0;
}
