
- `mkdir badge-workspace`
- `cd badge-workspace`
- `west init -m git@github.com:makerville/makerville-badge.git --mr master`
- `west update`
- `west blobs fetch hal_espressif`
- `cd makerville-badge`
- `make`
- `make flash monitor`

![mBadge25_TOP](/hw_pcb/mbadge25_v2/renders/mbadge25_17.png)

![mBadge25_BOT](/hw_pcb/mbadge25_v2/renders/mbadge25_19.png)

