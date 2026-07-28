# RVP file format

RayVault Package (RVP) is a little-endian binary container for fiber
monitoring datasets. Current production version is **2.3**. Version **1.x**
packages are accepted when `RV_OPEN_ALLOW_LEGACY` is set.

## File header (64 bytes)

| Offset | Size | Field |
|--------|------|-------|
| 0 | 4 | Magic `RVPK` |
| 4 | 2 | Major version |
| 6 | 2 | Minor version |
| 8 | 4 | Flags |
| 12 | 4 | Header CRC32 (header with this field zeroed) |
| 16 | 8 | Created (unix seconds) |
| 24 | 4 | Section count |
| 28 | 4 | Directory offset |
| 32 | 8 | Total size |
| 40 | 4 | Feature bits |
| 44 | 4 | Reserved |
| 48 | 8 | Reserved |

## Section directory

Each entry is 24 bytes:

| Offset | Size | Field |
|--------|------|-------|
| 0 | 4 | Tag (fourcc) |
| 4 | 4 | Flags |
| 8 | 8 | Absolute payload offset |
| 16 | 4 | Payload length |
| 20 | 4 | Payload CRC32 (0 = skip) |

## Section tags

| Tag | Purpose |
|-----|---------|
| SNAM | Interned site / label name pool |
| ROUT | Fiber route table |
| INST | Instrument / probe definitions |
| CLBR | Calibration records |
| WIND | Acquisition windows |
| WAVE | Waveform sample payloads |
| MARK | Event / marker annotations |
| LINK | Cross-reference linkages |
| NOTE | Optional free-form notes |
| SUMM | Footer summary counts / checksums |

A meaningful package normally includes SNAM, ROUT, INST, CLBR, WIND, and WAVE
together. MARK/LINK/NOTE/SUMM are optional but expected in plant archives.

## Cross references

- ROUT and INST name ids resolve through SNAM
- WIND references ROUT, INST, CLBR, and WAVE block ids
- MARK references WIND and optional SNAM labels
- LINK endpoints reference route/window/marker/wave ids
- SUMM counts should agree with decoded table sizes when present

Section order in the directory is not required to match logical dependency
order, but consumers decode names before resolving label strings.
