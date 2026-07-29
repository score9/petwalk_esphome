# petwalk_esphome

ESPHome external component for passively reading the serial display/LED bus between a petWALK mainboard and its front panel.

The observed bus uses:

- DATA
- active-low CLOCK (idle HIGH)
- LATCH/STROBE
- 56 bits per frame
- approximately 100 kHz clock
- approximately one frame every 5 ms

The ESP only listens. It must never drive the three bus lines.

## Wiring

Connect the ESP in parallel with the front panel:

| petWALK signal | ESP |
|---|---|
| DATA | input GPIO |
| CLOCK | input GPIO |
| LATCH/STROBE | input GPIO |
| GND | GND |

Verify the bus voltage before connecting it. ESP8266 and ESP32 GPIOs are not 5 V tolerant. If the petWALK bus uses 5 V, use a suitable level shifter or resistor divider on every signal.

## Installation from GitHub

```yaml
external_components:
  - source: github://YOUR_GITHUB_NAME/petwalk_esphome
    components: [petwalk_esphome]
    refresh: 1h
```

## Example

Bit numbers are **1-based in transmission order**. `bit: 1` is the first DATA value sampled after the configured LATCH edge.

```yaml
petwalk_esphome:
  id: petwalk_bus
  data_pin:
    number: D5
    mode: INPUT
  clock_pin:
    number: D6
    mode: INPUT
  latch_pin:
    number: D7
    mode: INPUT
  frame_bits: 56
  clock_edge: FALLING
  latch_edge: RISING
  debug_frames: false

binary_sensor:
  - platform: petwalk_esphome
    petwalk_esphome_id: petwalk_bus
    bit: 6
    active_low: true
    name: "Petwalk ON"

  - platform: petwalk_esphome
    petwalk_esphome_id: petwalk_bus
    bit: 9
    active_low: true
    name: "Petwalk Ausgangskontrolle"

  - platform: petwalk_esphome
    petwalk_esphome_id: petwalk_bus
    bit: 11
    active_low: true
    name: "Petwalk Eingangskontrolle"

  - platform: petwalk_esphome
    petwalk_esphome_id: petwalk_bus
    bit: 23
    active_low: true
    name: "Petwalk Tür"
    device_class: door
```

Each sensor only publishes when its logical state changes. `active_low: true` means a raw bus value of `0` is reported as ON.

## Options

### Hub

- `data_pin` — required input GPIO
- `clock_pin` — required input GPIO
- `latch_pin` — required input GPIO
- `frame_bits` — default `56`, range 1–64
- `clock_edge` — `FALLING` by default; change to `RISING` if required
- `latch_edge` — `RISING` by default; change to `FALLING` if required
- `debug_frames` — logs complete frames; keep disabled in normal operation because the bus sends about 200 frames/s

### Binary sensor

- `petwalk_esphome_id` — hub ID
- `bit` — 1-based bit number
- `active_low` — default `true`
- all standard ESPHome binary sensor options are supported

## Notes

The decoder uses GPIO interrupts and double-buffered frames. Home Assistant receives updates only when a selected bit changes.
