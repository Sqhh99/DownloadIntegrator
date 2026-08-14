# YOLOs-CPP (vendored)

Header-only YOLO inference helpers used by the cover extractor.

- Upstream: https://github.com/Geekgineer/YOLOs-CPP
- License: MIT (see upstream `LICENSE`)
- What is vendored: the `yolos/` and `tools/` headers needed to load `game-cover-v2.onnx`

This tree is a snapshot, not a git submodule. When updating:

1. Copy the required headers from a tagged upstream revision.
2. Record the upstream tag or commit in this file.
3. Note any local edits below.

Current snapshot: headers present in this directory as of the ONNX cover-extractor work. No additional local API changes are documented beyond what the app's `CoverExtractor` calls.
