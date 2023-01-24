# Upgrading from aptdec 1.8.0 to 2.0.0

# `aptdec` (now `aptdec-cli`)

## Removed

- The `-d` (output directory) flag
- The `-g` (gamma) flag
- Map overlays/MCIR
- Reading raw PNG images

(Some of these features have been moved into `aptdec_post.py`)

## Changes

- "palettes" are now called "luts"
- Image type and effects are now full English words (separated by commas) instead of letters
- Output format of realtime images is now `CURRENT_TIME.png` instead of `CURRENT_TIME-r.png`
- Channel A/B images include telemetry

## Examples

| v1.8                                  | v2.0.0                                    |
|---------------------------------------|-------------------------------------------|
| `aptdec file.wav`                     | `aptdec-cli file.wav`                     |
| `aptdec -i rt file.wav`               | `aptdec-cli -i raw,thermal file.wav`      |
| `aptdec -i ab file.wav`               | `aptdec-cli -i a,b -e strip file.wav`     |
| `aptdec -i p -p palette.png file.wav` | `aptdec-cli -i lut -l lut.png file.wav`   |
| `aptdec -d out *.wav`                 | `mkdir out; cd out; aptdec-cli ../*.wav`  |


# `libapt` (now `libaptdec`)

To avoid confusion with `apt` (the Debian package manager) the name of the aptdec library has been changed to `libaptdec`.
