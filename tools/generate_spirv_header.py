from pathlib import Path
import struct

root = Path(__file__).resolve().parents[1]
out = root / "app/src/main/cpp/generated/shaders_spv.h"
out.parent.mkdir(parents=True, exist_ok=True)
items = [("vert","app/src/main/shaders/aether.vert.spv"),("frag","app/src/main/shaders/aether.frag.spv"),("comp","app/src/main/shaders/aether.comp.spv")]
with out.open("w", encoding="utf-8") as f:
    f.write("#pragma once\n#include <cstdint>\nnamespace aether_shader_spv {\n")
    for name, rel in items:
        data = Path(root / rel).read_bytes()
        if len(data) % 4: raise SystemExit(f"{rel} is not aligned to 32-bit words")
        words = struct.unpack("<" + "I" * (len(data)//4), data)
        f.write(f"static const uint32_t {name}[] = {{\n")
        for i in range(0, len(words), 8):
            f.write("    " + ", ".join(f"0x{x:08x}u" for x in words[i:i+8]) + ",\n")
        f.write("};\n")
        f.write(f"static constexpr size_t {name}_words = sizeof({name}) / sizeof(uint32_t);\n")
    f.write("}\n")
