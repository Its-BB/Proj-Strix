# Demo assets

Static examples from the reference evaluation run are included in this folder:

- `detection_example.jpg` — Ultralytics validation batch with predictions
- `ablation_latency.png` — Mean latency vs. enabled pipeline branches
- `ablation_pr_scatter.png` — Precision–recall trade-off across ablation configs

## Optional video

A short screen recording helps reviewers understand the live pipeline. To add one:

1. Run `droneai/app.py` against a live or recorded MJPEG source.
2. Record 30–60 seconds showing stream ingest, a detection overlay, and an alert log entry.
3. Save as `docs/demo/demo.mp4` (H.264, &lt; 20 MB recommended).
4. Link it from the root README:

   ```markdown
   ## Demo video
   https://github.com/Its-BB/Proj-Strix/blob/main/docs/demo/demo.mp4
   ```

Video is optional; the static images above are sufficient for repository review.
