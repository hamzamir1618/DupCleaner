# dupcleaner

**Status: early development**

dupcleaner scans a drive, finds exact and near-duplicate files/photos, and reports reclaimable space before deleting anything.

## Planned Features
- [ ] Scan directory trees
- [ ] Detect exact duplicates via hashing
- [ ] Detect near-duplicate photos (perceptual hashing)
- [ ] Report reclaimable space
- [ ] Safe deletion capabilities
- [ ] GUI frontend
- [ ] CLI frontend

## Build Instructions

```bash
cmake -S . -B build
cmake --build build
```
