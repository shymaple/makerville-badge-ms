
- `mkdir badge-workspace`
- `cd badge-workspace`
- `west init -m git@github.com:makerville/makerville-badge.git --mr master`
- `west update`
- `west blobs fetch hal_espressif`
- `cd makerville-badge`
- `make`
- `make flash monitor`
