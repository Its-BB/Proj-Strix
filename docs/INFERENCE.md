# Inference guide

Run the near-edge STRIX runtime after setup.

## Prerequisites

- Python 3.10+
- `pip install -r droneai/requirements.txt`
- `droneai/weapon_detection_custom.pt` (fine-tuned YOLOv8n; not in repo)
- Optional: live ESP32-CAM at `http://<device-ip>/stream`, or a local video file per `config.yaml`

## Configure

Edit `droneai/config.yaml`:

```yaml
# Example — set your stream endpoint
stream_url: "http://192.168.1.100/stream"
```

Adjust detection thresholds and branch flags as documented in the file comments.

## Run

```bash
cd droneai
python app.py
```

Alternative modular entry:

```bash
python detection_system.py
```

## Expected behavior

- Frames are ingested from the configured source
- `weapon_detector.py` runs the composed branch workflow
- `alert_system.py` enqueues cooldown-gated alerts and may write JSON logs under `detections/`

## Offline / no hardware

For benchmarks without a camera, use evaluation scripts with `STRIX_DATASET_ROOT` (see [REPRODUCIBILITY.md](REPRODUCIBILITY.md)) or the MJPEG surrogate in `experiments/run_final_push.py`.

## Troubleshooting

| Issue | Check |
| --- | --- |
| Missing weights | Place `weapon_detection_custom.pt` in `droneai/` |
| Stream timeout | Ping ESP32 IP; open `/stream` in a browser |
| Slow FPS | Disable branches in config; use single-pass mode |
