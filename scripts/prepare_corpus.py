#!/usr/bin/env python3
"""Prepare RayVault seed corpus files (binary RVP packages).

This script only writes corpus/*.rvp and fuzz/corpus/** seed inputs.
It does not generate C/C++ sources.
"""

from __future__ import annotations

import os
import struct
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def u16(v: int) -> bytes:
    return struct.pack("<H", v & 0xFFFF)


def u32(v: int) -> bytes:
    return struct.pack("<I", v & 0xFFFFFFFF)


def u64(v: int) -> bytes:
    return struct.pack("<Q", v & 0xFFFFFFFFFFFFFFFF)


def f32(v: float) -> bytes:
    return struct.pack("<f", v)


def crc32(data: bytes) -> int:
    return zlib.crc32(data) & 0xFFFFFFFF


TAG = {
    "SNAM": 0x4D414E53,
    "ROUT": 0x54554F52,
    "INST": 0x54534E49,
    "CLBR": 0x52424C43,
    "WIND": 0x444E4957,
    "WAVE": 0x45564157,
    "MARK": 0x4B52414D,
    "LINK": 0x4B4E494C,
    "NOTE": 0x45544F4E,
    "SUMM": 0x4D4D5553,
}


def lp_str(s: str) -> bytes:
    b = s.encode("utf-8")
    out = u16(len(b)) + b
    if len(b) & 1:
        out += b"\x00"
    return out


def build_snam(names: list[tuple[int, str]]) -> bytes:
    body = u32(0x314D414E) + u32(len(names))
    for nid, s in names:
        body += u32(nid) + lp_str(s)
    return body


def build_rout(routes: list[dict]) -> bytes:
    body = u32(0x31544F52) + u32(len(routes))
    for r in routes:
        body += (
            u32(r["id"])
            + u32(r["site"])
            + u32(r["far"])
            + f32(r["km"])
            + u32(r.get("fibers", 12))
            + u32(r.get("flags", 0))
        )
    return body


def build_inst(items: list[dict]) -> bytes:
    body = u32(0x3154534E) + u32(len(items))
    for r in items:
        body += (
            u32(r["id"])
            + u32(r["model"])
            + u32(r["serial"])
            + u16(r.get("wl", 1550))
            + u16(r.get("pulse", 100))
            + f32(r.get("dr", 38.0))
            + u32(r.get("flags", 0))
        )
    return body


def build_clbr(items: list[dict]) -> bytes:
    body = u32(0x31424C43) + u32(len(items))
    for r in items:
        body += (
            u32(r["id"])
            + u32(r["inst"])
            + u64(r.get("ts", 1700000000))
            + f32(r.get("n", 1.4681))
            + f32(r.get("bs", -81.0))
            + f32(r.get("sp", 0.02))
            + u32(r.get("flags", 0))
        )
    return body


def build_wind(items: list[dict]) -> bytes:
    body = u32(0x31444E49) + u32(len(items))
    for r in items:
        body += (
            u32(r["id"])
            + u32(r["route"])
            + u32(r["inst"])
            + u32(r["calib"])
            + u32(r["wave"])
            + f32(r.get("pulse", 100.0))
            + f32(r.get("range", 10.0))
            + f32(r.get("res", 0.5))
            + u64(r.get("ts", 1700001000))
        )
    return body


def build_wave(blocks: list[dict]) -> bytes:
    body = u32(0x31455641) + u32(len(blocks))
    for b in blocks:
        samples = b["samples"]
        raw = b"".join(struct.pack("<h", int(v)) for v in samples)
        body += (
            u32(b["id"])
            + u32(b["window"])
            + u32(len(samples))
            + f32(b.get("start", 0.0))
            + f32(b.get("step", 0.5))
            + u16(b.get("flags", 0))
            + u16(crc32(raw) & 0xFFFF)
            + raw
        )
        if len(raw) & 1:
            body += b"\x00"
    return body


def build_mark(items: list[dict]) -> bytes:
    body = u32(0x314B524D) + u32(len(items))
    for r in items:
        body += (
            u32(r["id"])
            + u32(r["window"])
            + u32(r["label"])
            + f32(r.get("dist", 1000.0))
            + f32(r.get("loss", 0.1))
            + f32(r.get("refl", -55.0))
            + u16(r.get("kind", 1))
            + u16(r.get("sev", 20))
            + u32(r.get("flags", 0))
        )
    return body


def build_link(items: list[dict]) -> bytes:
    body = u32(0x314B4E4C) + u32(len(items))
    for r in items:
        body += (
            u32(r["id"])
            + u32(r["fk"])
            + u32(r["fi"])
            + u32(r["tk"])
            + u32(r["ti"])
            + u32(r.get("flags", 0))
        )
    return body


def build_note(items: list[dict]) -> bytes:
    body = u32(0x3145544E) + u32(len(items))
    for r in items:
        body += (
            u32(r["id"])
            + u32(r["tk"])
            + u32(r["ti"])
            + u32(r["text"])
            + u32(r.get("flags", 0))
        )
    return body


def build_summ(counts: dict) -> bytes:
    return (
        u32(0x314D4D55)
        + u32(counts.get("routes", 0))
        + u32(counts.get("windows", 0))
        + u32(counts.get("markers", 0))
        + u32(counts.get("waves", 0))
        + u32(counts.get("names", 0))
        + u32(0)
        + u64(0)
        + u32(0)
    )


def pack_rvp(sections: list[tuple[str, bytes]], major=2, minor=3, feature=0) -> bytes:
    dir_offset = 64
    nsec = len(sections)
    payloads = b"".join(body for _, body in sections)
    payload_off = dir_offset + nsec * 24
    directory = b""
    off = payload_off
    for tag, body in sections:
        directory += u32(TAG[tag]) + u32(0) + u64(off) + u32(len(body)) + u32(crc32(body))
        off += len(body)
    total = off
    hdr = bytearray(64)
    hdr[0:4] = b"RVPK"
    hdr[4:6] = u16(major)
    hdr[6:8] = u16(minor)
    hdr[8:12] = u32(0)
    # crc later
    hdr[16:24] = u64(1710000000)
    hdr[24:28] = u32(nsec)
    hdr[28:32] = u32(dir_offset)
    hdr[32:40] = u64(total)
    hdr[40:44] = u32(feature)
    hdr[12:16] = u32(crc32(bytes(hdr)))
    return bytes(hdr) + directory + payloads


def samples(n: int, base: int = -2000) -> list[int]:
    return [base + (i % 40) - (i // 100) for i in range(n)]


def write(path: Path, data: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)
    print(f"wrote {path} ({len(data)} bytes)")


def small_valid() -> bytes:
    names = [(1, "CO-A"), (2, "POP-B"), (3, "OTDR"), (4, "SN1"), (5, "splice")]
    sections = [
        ("SNAM", build_snam(names)),
        ("ROUT", build_rout([{"id": 1, "site": 1, "far": 2, "km": 4.5}])),
        ("INST", build_inst([{"id": 1, "model": 3, "serial": 4}])),
        ("CLBR", build_clbr([{"id": 1, "inst": 1}])),
        ("WIND", build_wind([{"id": 1, "route": 1, "inst": 1, "calib": 1, "wave": 1}])),
        ("WAVE", build_wave([{"id": 1, "window": 1, "samples": samples(32)}])),
        ("MARK", build_mark([{"id": 1, "window": 1, "label": 5, "dist": 1200.0}])),
        ("LINK", build_link([{"id": 1, "fk": 1, "fi": 1, "tk": 2, "ti": 1}])),
        ("SUMM", build_summ({"routes": 1, "windows": 1, "markers": 1, "waves": 1, "names": 5})),
    ]
    return pack_rvp(sections)


def medium_valid() -> bytes:
    names = [
        (1, "Hub-North"),
        (2, "Node-7"),
        (3, "Node-8"),
        (4, "OTDR-X"),
        (5, "SN-88"),
        (6, "splice"),
        (7, "reflect"),
        (8, "note-text"),
    ]
    waves = []
    winds = []
    marks = []
    for i in range(1, 5):
        winds.append({"id": i, "route": 1 if i < 3 else 2, "inst": 1, "calib": 1, "wave": i, "range": 8.0})
        waves.append({"id": i, "window": i, "samples": samples(128, -1800 - i * 10)})
        marks.append({"id": i, "window": i, "label": 6 if i % 2 else 7, "dist": 500.0 * i, "loss": 0.05 * i, "kind": 1 if i % 2 else 2, "sev": 10 * i})
    sections = [
        ("SNAM", build_snam(names)),
        ("ROUT", build_rout([
            {"id": 1, "site": 1, "far": 2, "km": 8.2, "fibers": 24},
            {"id": 2, "site": 1, "far": 3, "km": 3.1, "fibers": 12},
        ])),
        ("INST", build_inst([{"id": 1, "model": 4, "serial": 5, "wl": 1310}])),
        ("CLBR", build_clbr([{"id": 1, "inst": 1}])),
        ("WIND", build_wind(winds)),
        ("WAVE", build_wave(waves)),
        ("MARK", build_mark(marks)),
        ("LINK", build_link([
            {"id": 1, "fk": 1, "fi": 1, "tk": 2, "ti": 1},
            {"id": 2, "fk": 1, "fi": 2, "tk": 2, "ti": 3},
        ])),
        ("NOTE", build_note([{"id": 1, "tk": 1, "ti": 1, "text": 8}])),
        ("SUMM", build_summ({"routes": 2, "windows": 4, "markers": 4, "waves": 4, "names": 8})),
    ]
    return pack_rvp(sections)


def large_structured() -> bytes:
    names = [(i, f"site-{i}") for i in range(1, 21)]
    names += [(100, "OTDR-Pro"), (101, "SN-9001"), (102, "splice"), (103, "bend")]
    routes = [{"id": i, "site": i, "far": (i % 20) + 1, "km": 2.0 + i * 0.3, "fibers": 12} for i in range(1, 9)]
    winds = []
    waves = []
    marks = []
    for i in range(1, 13):
        rid = ((i - 1) % 8) + 1
        winds.append({"id": i, "route": rid, "inst": 1, "calib": 1, "wave": i, "range": 12.0, "res": 0.25})
        waves.append({"id": i, "window": i, "samples": samples(256, -2200)})
        marks.append({"id": i, "window": i, "label": 102 if i % 3 else 103, "dist": 100.0 * i, "loss": 0.02 * i, "kind": (i % 5) + 1, "sev": 5 * i})
    sections = [
        ("SNAM", build_snam(names)),
        ("ROUT", build_rout(routes)),
        ("INST", build_inst([{"id": 1, "model": 100, "serial": 101, "wl": 1550, "dr": 40.0}])),
        ("CLBR", build_clbr([{"id": 1, "inst": 1, "ts": 1600000000}])),
        ("WIND", build_wind(winds)),
        ("WAVE", build_wave(waves)),
        ("MARK", build_mark(marks)),
        ("LINK", build_link([{"id": i, "fk": 1, "fi": ((i - 1) % 8) + 1, "tk": 2, "ti": i} for i in range(1, 13)])),
        ("SUMM", build_summ({"routes": 8, "windows": 12, "markers": 12, "waves": 12, "names": 24})),
    ]
    return pack_rvp(sections)


def legacy_v1() -> bytes:
    data = small_valid()
    # rewrite major version to 1
    buf = bytearray(data)
    buf[4:6] = u16(1)
    buf[6:8] = u16(0)
    # recompute header crc
    tmp = bytearray(buf[:64])
    tmp[12:16] = b"\x00\x00\x00\x00"
    buf[12:16] = u32(crc32(bytes(tmp)))
    return bytes(buf)


def optional_notes() -> bytes:
    base = medium_valid()
    return base  # already includes NOTE


def multi_window_stream() -> bytes:
    names = [(1, "A"), (2, "B"), (3, "M"), (4, "S"), (5, "lab")]
    winds = [{"id": i, "route": 1, "inst": 1, "calib": 1, "wave": i} for i in range(1, 7)]
    waves = [{"id": i, "window": i, "samples": samples(80, -1500 - i)} for i in range(1, 7)]
    sections = [
        ("SNAM", build_snam(names)),
        ("ROUT", build_rout([{"id": 1, "site": 1, "far": 2, "km": 6.0}])),
        ("INST", build_inst([{"id": 1, "model": 3, "serial": 4}])),
        ("CLBR", build_clbr([{"id": 1, "inst": 1}])),
        ("WIND", build_wind(winds)),
        ("WAVE", build_wave(waves)),
        ("MARK", build_mark([{"id": 1, "window": 3, "label": 5}])),
        ("LINK", build_link([{"id": 1, "fk": 1, "fi": 1, "tk": 2, "ti": 1}])),
        ("SUMM", build_summ({"routes": 1, "windows": 6, "markers": 1, "waves": 6, "names": 5})),
    ]
    return pack_rvp(sections)


def main() -> None:
    shared = ROOT / "corpus"
    write(shared / "small_span.rvp", small_valid())
    write(shared / "medium_plant.rvp", medium_valid())
    write(shared / "large_ring.rvp", large_structured())
    write(shared / "legacy_v1_span.rvp", legacy_v1())
    write(shared / "with_notes.rvp", optional_notes())
    write(shared / "stream_windows.rvp", multi_window_stream())

    harnesses = [
        "package_session_fuzzer",
        "span_query_fuzzer",
        "wave_stream_fuzzer",
        "dataset_export_fuzzer",
        "repair_reopen_fuzzer",
    ]
    mapping = {
        "package_session_fuzzer": ["small_span.rvp", "medium_plant.rvp", "legacy_v1_span.rvp"],
        "span_query_fuzzer": ["medium_plant.rvp", "large_ring.rvp", "with_notes.rvp"],
        "wave_stream_fuzzer": ["stream_windows.rvp", "medium_plant.rvp", "large_ring.rvp"],
        "dataset_export_fuzzer": ["small_span.rvp", "medium_plant.rvp", "with_notes.rvp"],
        "repair_reopen_fuzzer": ["legacy_v1_span.rvp", "medium_plant.rvp", "stream_windows.rvp"],
    }
    for h in harnesses:
        d = ROOT / "fuzz" / "corpus" / h
        for name in mapping[h]:
            write(d / name, (shared / name).read_bytes())


if __name__ == "__main__":
    main()
