#!/usr/bin/env python3
"""
Build-time Kenney Nature Kit importer for Aether 3D.

The source pack is fetched from Kenney's official distribution URL and verified
against its published SHA-256. The selected GLB files are converted into one
small, vertex-colored C++ mesh so the Android runtime does not need a general
purpose glTF loader just to render the showcase world.

Kenney Nature Kit is CC0.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
import struct
import tempfile
import urllib.request
import zipfile
from pathlib import Path
from typing import Any

URL = "https://kenney.nl/media/pages/assets/nature-kit/37ac38a37b-1677698939/kenney_nature-kit.zip"
SHA256 = "fa7974a0d342bfe63c38664ba9f8ec1a4aab8ea25f099bdc56870e33588c4d9d"

# Small, representative subset. The full pack stays at Kenney; only geometry
# needed by the mobile showcase scene is baked into the APK.
MODELS = {
    "tree-oak": ("tree_oak.glb", 4.6),
    "tree-default": ("tree_default.glb", 4.2),
    "tree-pinedefaulta": ("tree_pineDefaultA.glb", 4.8),
    "tree-pineroundb": ("tree_pineRoundB.glb", 4.5),
    "plant-bushdetailed": ("plant_bushDetailed.glb", 3.8),
    "rock-largea": ("rock_largeA.glb", 2.8),
    "rock-smalla": ("rock_smallA.glb", 2.4),
    "grass-large": ("grass_large.glb", 3.0),
    "flower-purplea": ("flower_purpleA.glb", 2.8),
    "flower-redb": ("flower_redB.glb", 2.8),
    "mushroom-redgroup": ("mushroom_redGroup.glb", 2.6),
    "log-stack": ("log_stack.glb", 2.8),
}

# x, z, model key, yaw radians, scale multiplier.
INSTANCES = [
    (-16.0, -13.0, "tree-oak", 0.20, 1.00),
    (-11.5, -15.5, "tree-default", -0.40, 0.95),
    (-5.5, -16.5, "tree-pineroundb", 0.70, 1.05),
    (2.0, -16.0, "tree-pinedefaulta", -0.10, 1.10),
    (9.0, -15.0, "tree-oak", 0.55, 0.95),
    (15.5, -12.0, "tree-default", -0.20, 1.00),
    (-17.0, -4.0, "tree-pinedefaulta", 1.00, 1.00),
    (17.0, -3.5, "tree-pineroundb", -0.75, 1.00),
    (-17.0, 5.0, "tree-oak", -0.40, 1.05),
    (17.0, 6.0, "tree-default", 0.25, 1.05),
    (-15.0, 13.0, "tree-pineroundb", -0.60, 0.95),
    (-8.0, 15.5, "tree-oak", 0.30, 1.00),
    (0.0, 16.0, "tree-default", -0.25, 1.05),
    (8.0, 15.0, "tree-pinedefaulta", 0.60, 1.00),
    (15.0, 12.5, "tree-oak", -0.35, 0.95),

    (-12.0, -8.0, "plant-bushdetailed", 0.2, 1.0),
    (-8.0, -11.0, "plant-bushdetailed", 1.3, 0.9),
    (7.0, -10.5, "plant-bushdetailed", -0.6, 1.0),
    (12.0, -7.5, "plant-bushdetailed", 0.8, 0.95),
    (-13.0, 7.0, "plant-bushdetailed", -0.4, 1.0),
    (-8.5, 10.0, "plant-bushdetailed", 0.7, 0.9),
    (8.0, 9.5, "plant-bushdetailed", -1.1, 1.0),
    (12.5, 6.5, "plant-bushdetailed", 0.3, 1.0),

    (-10.0, -4.0, "rock-largea", 0.4, 1.0),
    (-3.5, -12.0, "rock-largea", -0.8, 0.8),
    (4.0, -13.0, "rock-largea", 1.2, 0.9),
    (10.5, -3.0, "rock-largea", -0.3, 0.85),
    (-11.0, 4.0, "rock-largea", 0.8, 0.9),
    (10.0, 4.5, "rock-largea", -1.0, 0.95),
    (-4.5, 12.0, "rock-largea", 0.2, 0.75),
    (5.5, 11.5, "rock-largea", -0.6, 0.8),

    (-7.0, -6.0, "rock-smalla", 0.1, 1.0),
    (-2.0, -8.5, "rock-smalla", 1.0, 0.8),
    (3.5, -7.0, "rock-smalla", -0.4, 1.1),
    (8.0, -5.0, "rock-smalla", 0.6, 0.9),
    (-7.0, 5.5, "rock-smalla", -1.0, 0.9),
    (3.0, 6.0, "rock-smalla", 0.3, 1.0),
    (7.0, 12.0, "rock-smalla", 1.4, 0.8),
    (-2.0, 11.0, "rock-smalla", -0.7, 0.9),

    (-13.0, -6.5, "grass-large", 0.2, 1.0),
    (-9.0, -2.0, "grass-large", -0.5, 0.9),
    (-6.0, 2.0, "grass-large", 1.1, 0.8),
    (6.0, -2.0, "grass-large", 0.5, 1.0),
    (12.0, 1.5, "grass-large", -0.2, 0.9),
    (9.0, 7.0, "grass-large", 0.8, 1.0),
    (-10.0, 8.0, "grass-large", -1.1, 0.9),
    (1.0, 10.0, "grass-large", 0.4, 0.9),

    (-5.0, -4.5, "flower-purplea", 0.0, 1.0),
    (-4.0, -3.0, "flower-redb", 0.8, 0.9),
    (5.0, -4.0, "flower-purplea", -0.5, 1.1),
    (5.5, 3.0, "flower-redb", 1.0, 1.0),
    (-4.5, 4.5, "flower-purplea", -1.0, 0.9),
    (2.0, 4.0, "flower-redb", 0.2, 1.0),
    (-1.0, -11.5, "mushroom-redgroup", 0.4, 1.0),
    (11.0, 10.0, "mushroom-redgroup", -0.7, 0.9),
    (-12.0, 10.0, "mushroom-redgroup", 0.6, 1.0),

    (-3.0, 2.0, "log-stack", 0.25, 0.9),
    (4.0, 2.5, "log-stack", -0.9, 1.0),
]


def terrain(x: float, z: float) -> float:
    return (
        -1.05
        + 0.28 * math.sin(x * 0.24) * math.cos(z * 0.19)
        + 0.11 * math.sin(x * 0.61 + z * 0.37)
        + 0.06 * math.cos(z * 0.83 - x * 0.17)
    )


def quat_matrix(q: list[float]) -> list[float]:
    x, y, z, w = q
    return [
        1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w), 0,
        2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w), 0,
        2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y), 0,
        0, 0, 0, 1,
    ]


def mat_mul(a: list[float], b: list[float]) -> list[float]:
    out = [0.0] * 16
    for c in range(4):
        for r in range(4):
            out[c * 4 + r] = sum(a[k * 4 + r] * b[c * 4 + k] for k in range(4))
    return out


def node_matrix(node: dict[str, Any]) -> list[float]:
    if "matrix" in node:
        return [float(x) for x in node["matrix"]]
    t = node.get("translation", [0, 0, 0])
    r = quat_matrix([float(x) for x in node.get("rotation", [0, 0, 0, 1])])
    s = node.get("scale", [1, 1, 1])
    r[0] *= s[0]; r[1] *= s[0]; r[2] *= s[0]
    r[4] *= s[1]; r[5] *= s[1]; r[6] *= s[1]
    r[8] *= s[2]; r[9] *= s[2]; r[10] *= s[2]
    r[12], r[13], r[14] = t
    return r


def transform_point(m: list[float], p: list[float]) -> list[float]:
    return [
        m[0] * p[0] + m[4] * p[1] + m[8] * p[2] + m[12],
        m[1] * p[0] + m[5] * p[1] + m[9] * p[2] + m[13],
        m[2] * p[0] + m[6] * p[1] + m[10] * p[2] + m[14],
    ]


def transform_dir(m: list[float], p: list[float]) -> list[float]:
    x = m[0] * p[0] + m[4] * p[1] + m[8] * p[2]
    y = m[1] * p[0] + m[5] * p[1] + m[9] * p[2]
    z = m[2] * p[0] + m[6] * p[1] + m[10] * p[2]
    l = math.sqrt(x * x + y * y + z * z)
    if l < 1e-8:
        return [0.0, 1.0, 0.0]
    return [x / l, y / l, z / l]


def parse_glb(path: Path) -> tuple[dict[str, Any], bytes]:
    raw = path.read_bytes()
    if raw[:4] != b"glTF":
        raise RuntimeError(f"{path}: not a GLB")
    version, length = struct.unpack_from("<II", raw, 4)
    if version != 2 or length > len(raw):
        raise RuntimeError(f"{path}: unsupported GLB version/length")
    off = 12
    json_chunk = None
    bin_chunk = b""
    while off + 8 <= length:
        chunk_len, chunk_type = struct.unpack_from("<II", raw, off)
        off += 8
        chunk = raw[off:off + chunk_len]
        off += chunk_len
        if chunk_type == 0x4E4F534A:
            json_chunk = chunk.rstrip(b" 	\r\n\x00")
        elif chunk_type == 0x004E4942:
            bin_chunk = chunk
    if json_chunk is None:
        raise RuntimeError(f"{path}: missing JSON chunk")
    return json.loads(json_chunk.decode("utf-8")), bin_chunk


COMPONENT = {
    5120: ("b", 1, True),
    5121: ("B", 1, False),
    5122: ("h", 2, True),
    5123: ("H", 2, False),
    5125: ("I", 4, False),
    5126: ("f", 4, False),
}
TYPE_COUNT = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4}


def accessor_values(doc: dict[str, Any], binary: bytes, index: int) -> list[list[float]]:
    acc = doc["accessors"][index]
    n = int(acc["count"])
    comps = TYPE_COUNT[acc["type"]]
    fmt, size, signed = COMPONENT[int(acc["componentType"])]
    if "bufferView" not in acc:
        return [[0.0] * comps for _ in range(n)]
    view = doc["bufferViews"][int(acc["bufferView"])]
    base = int(view.get("byteOffset", 0)) + int(acc.get("byteOffset", 0))
    stride = int(view.get("byteStride", comps * size))
    normalized = bool(acc.get("normalized", False))
    out: list[list[float]] = []
    for i in range(n):
        vals = struct.unpack_from("<" + fmt * comps, binary, base + i * stride)
        row: list[float] = []
        for v in vals:
            if normalized:
                if signed:
                    v = max(-1.0, float(v) / ((1 << (size * 8 - 1)) - 1))
                else:
                    v = float(v) / ((1 << (size * 8)) - 1)
            row.append(float(v))
        out.append(row)
    return out


def index_values(doc: dict[str, Any], binary: bytes, index: int) -> list[int]:
    acc = doc["accessors"][index]
    if acc["type"] != "SCALAR":
        raise RuntimeError("index accessor is not scalar")
    fmt, size, _ = COMPONENT[int(acc["componentType"])]
    view = doc["bufferViews"][int(acc["bufferView"])]
    base = int(view.get("byteOffset", 0)) + int(acc.get("byteOffset", 0))
    return [int(struct.unpack_from("<" + fmt, binary, base + i * size)[0]) for i in range(int(acc["count"]))]


def load_model(path: Path) -> tuple[list[tuple[float, ...]], list[int]]:
    doc, binary = parse_glb(path)
    materials = doc.get("materials", [])
    meshes = doc.get("meshes", [])
    nodes = doc.get("nodes", [])
    scenes = doc.get("scenes", [])
    scene_index = int(doc.get("scene", 0))
    roots = scenes[scene_index].get("nodes", []) if scenes else list(range(len(nodes)))

    vertices: list[tuple[float, ...]] = []
    indices: list[int] = []

    def visit(node_index: int, parent: list[float]) -> None:
        node = nodes[node_index]
        world = mat_mul(parent, node_matrix(node))
        if "mesh" in node:
            mesh = meshes[int(node["mesh"])]
            for prim in mesh.get("primitives", []):
                if int(prim.get("mode", 4)) != 4:
                    continue
                attrs = prim.get("attributes", {})
                if "POSITION" not in attrs:
                    continue
                positions = accessor_values(doc, binary, int(attrs["POSITION"]))
                normals = accessor_values(doc, binary, int(attrs["NORMAL"])) if "NORMAL" in attrs else [[0, 1, 0]] * len(positions)
                colors = accessor_values(doc, binary, int(attrs["COLOR_0"])) if "COLOR_0" in attrs else None
                mat_color = [1.0, 1.0, 1.0, 1.0]
                mi = prim.get("material")
                if mi is not None and int(mi) < len(materials):
                    pbr = materials[int(mi)].get("pbrMetallicRoughness", {})
                    mat_color = pbr.get("baseColorFactor", mat_color)
                base = len(vertices)
                for i, p in enumerate(positions):
                    wp = transform_point(world, p)
                    wn = transform_dir(world, normals[i])
                    vc = colors[i] if colors is not None else mat_color
                    c = [max(0.0, min(1.0, float(vc[j]) * float(mat_color[j]))) for j in range(3)]
                    vertices.append((wp[0], wp[1], wp[2], wn[0], wn[1], wn[2], c[0], c[1], c[2]))
                if "indices" in prim:
                    local = index_values(doc, binary, int(prim["indices"]))
                else:
                    local = list(range(len(positions)))
                indices.extend(base + int(i) for i in local)
        for child in node.get("children", []):
            visit(int(child), world)

    ident = [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]
    for root in roots:
        visit(int(root), ident)
    return vertices, indices


def download_and_extract(root: Path) -> Path:
    archive = root / "kenney_nature-kit.zip"
    if not archive.exists():
        print("Downloading Kenney Nature Kit...")
        with urllib.request.urlopen(URL, timeout=60) as response, archive.open("wb") as out:
            while True:
                chunk = response.read(1024 * 1024)
                if not chunk:
                    break
                out.write(chunk)
    digest = hashlib.sha256(archive.read_bytes()).hexdigest()
    if digest != SHA256:
        raise RuntimeError(f"Kenney archive SHA-256 mismatch: {digest} != {SHA256}")
    extract = root / "kenney_nature-kit"
    if not extract.exists():
        with zipfile.ZipFile(archive) as zf:
            zf.extractall(extract)
    return extract


def find_model(extract: Path, filename: str) -> Path:
    matches = list(extract.rglob(filename))
    if not matches:
        raise FileNotFoundError(filename)
    return matches[0]


def fmt(v: float) -> str:
    if abs(v) < 0.0000005:
        v = 0.0
    return f"{v:.6f}f"


def generate(output: Path, asset_root: Path) -> None:
    with tempfile.TemporaryDirectory(prefix="aether-kenney-") as td:
        root = Path(td)
        extract = download_and_extract(root)
        cache: dict[str, tuple[list[tuple[float, ...]], list[int]]] = {}
        for key, (filename, _) in MODELS.items():
            cache[key] = load_model(find_model(extract, filename))
            print(f"Kenney {key}: {len(cache[key][0])} vertices, {len(cache[key][1])} indices")

        combined_v: list[tuple[float, ...]] = []
        combined_i: list[int] = []
        for x, z, key, yaw, scale_mul in INSTANCES:
            source_v, source_i = cache[key]
            base = len(combined_v)
            scale = MODELS[key][1] * scale_mul
            cy, sy = math.cos(yaw), math.sin(yaw)
            ground = terrain(x, z)
            for p in source_v:
                px, py, pz = p[0] * scale, p[1] * scale, p[2] * scale
                rx = px * cy - pz * sy
                rz = px * sy + pz * cy
                nx, ny, nz = p[3], p[4], p[5]
                rnx = nx * cy - nz * sy
                rnz = nx * sy + nz * cy
                combined_v.append((x + rx, ground + py, z + rz, rnx, ny, rnz, p[6], p[7], p[8]))
            combined_i.extend(base + i for i in source_i)

        if len(combined_v) >= 65536:
            raise RuntimeError(f"Kenney world has {len(combined_v)} vertices; uint16 index path would overflow")
        output.parent.mkdir(parents=True, exist_ok=True)
        with output.open("w", encoding="utf-8") as f:
            f.write("// Generated by tools/generate_kenney_world.py; do not edit.\n")
            f.write("#pragma once\n#include <cstdint>\n\nnamespace aether_kenney {\n")
            f.write("struct Vertex { float px,py,pz,nx,ny,nz,r,g,b; };\n")
            f.write(f"static constexpr uint32_t kVertexCount = {len(combined_v)}u;\n")
            f.write(f"static constexpr uint32_t kIndexCount = {len(combined_i)}u;\n")
            f.write("static constexpr Vertex kVertices[] = {\n")
            for p in combined_v:
                f.write("    {" + ",".join(fmt(x) for x in p) + "},\n")
            f.write("};\nstatic constexpr uint16_t kIndices[] = {\n")
            for i in range(0, len(combined_i), 12):
                f.write("    " + ",".join(str(int(x)) for x in combined_i[i:i+12]) + ",\n")
            f.write("};\n}\n")
    print(f"Wrote {output}")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True)
    parser.add_argument("--asset-root", default="app/src/main/assets/kenney")
    args = parser.parse_args()
    output = Path(args.output)
    asset_root = Path(args.asset_root)
    asset_root.mkdir(parents=True, exist_ok=True)
    (asset_root / "README.txt").write_text(
        "Kenney Nature Kit 2.1 subset used by the Aether 3D showcase world.\n"
        "Source: https://kenney.nl/assets/nature-kit\n"
        "License: Creative Commons CC0 1.0.\n"
        "The build fetches and verifies the official archive before generating mesh data.\n",
        encoding="utf-8",
    )
    generate(output, asset_root)


if __name__ == "__main__":
    main()
