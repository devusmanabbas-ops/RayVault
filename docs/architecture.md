# Architecture overview

RayVault is organized as a library with a public session-oriented API and a
set of cooperating internal subsystems.

## Subsystems

1. **Public API** (`include/rayvault`, `src/rayvault.c`) — package/session
   lifecycle, queries, streams, export.
2. **Arena allocator** — bump allocator for parse tables and interned strings.
   Callers rely on pointer stability until reset/destroy.
3. **Binary reader** — loads RVP bytes, parses the file header and section
   directory, verifies optional section CRCs.
4. **Section decoders** — SNAM, ROUT, INST, CLBR, WIND, WAVE, MARK, LINK,
   NOTE, SUMM payload parsers.
5. **Incremental parser** — phased decode across sections into a `rv_parsed`
   bundle allocated from the arena.
6. **Name dictionary** — hash lookup over interned SNAM entries.
7. **Index builder** — secondary maps for windows, waves, routes, markers;
   borrows interned labels and record pointers.
8. **Cross-reference resolver** — checks WIND→WAVE/ROUT/INST and MARK→WIND.
9. **Validation / normalization** — header consistency, SUMM counts, physical
   ranges, severity clamps.
10. **Wave cache (LRU)** — borrows sample slices from WAVE tables; generation
    counters track invalidation.
11. **Stream / cursor** — ordered window walks and random seeks over cache.
12. **Derived summary / analytics** — plant health, loss histograms, event
    classification.
13. **Export / builder** — rewrite RVP images from live tables.
14. **Checkpoint** — snapshot derived state for restore across rebuilds.
15. **Repair / legacy migration** — recover SUMM, drop dangling links, bump
    v1 packages to v2 backing stores.
16. **Diagnostics / config / plugins** — error lists, tunables, observation hooks.

## Lifecycle

```
open package → parse sections → (optional) build index → warm cache
     → query / stream / export → repair or migrate → rebind → close
```

Several later stages borrow pointers established earlier. Session rebind and
export-after-rebuild paths intentionally keep derived snapshots available so
monitoring UIs can continue serving cached views while indexes refresh.
