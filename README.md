# ESP32-S3 Basic RGB Strip Controller

PlatformIO/Arduino firmware for a 4-wire, 5 V common-positive RGB LED strip.
The detected controller is an ESP32-S3 with 16 MB flash and 8 MB PSRAM.

## Hardware

The LED strip must be driven through three external 3.3 V-compatible,
logic-level N-channel MOSFETs. The ESP32 GPIO pins cannot power the strip.

| Signal | Connection |
| --- | --- |
| Strip `5V+` | External regulated 5 V supply positive |
| Strip `R` | Red MOSFET drain |
| Strip `G` | Green MOSFET drain |
| Strip `B` | Blue MOSFET drain |
| MOSFET sources | External supply ground |
| ESP32 GPIO 4 | Blue MOSFET gate through 100-220 ohm resistor |
| ESP32 GPIO 5 | Red MOSFET gate through 100-220 ohm resistor |
| ESP32 GPIO 6 | Green MOSFET gate through 100-220 ohm resistor |
| ESP32 GPIO 7 | Segment 2 blue MOSFET gate through 100-220 ohm resistor |
| ESP32 GPIO 8 | Segment 2 red MOSFET gate through 100-220 ohm resistor |
| ESP32 GPIO 9 | Segment 2 green MOSFET gate through 100-220 ohm resistor |
| ESP32 GND | External supply ground |

Each strip segment uses its own three MOSFETs. Add a 10 kilohm pull-down
resistor between every MOSFET gate and ground. This
keeps the strip off while the ESP32 is starting. Do not connect external 5 V to
an ESP32 3.3 V pin.

Choose the power supply and MOSFET current ratings for the strip's maximum
load. Full white activates all three channels and is normally the highest-load
condition.

## Dashboard

The ESP32 creates and hosts its own local Wi-Fi network; an internet connection
is not required.

1. Join Wi-Fi network `ESP32-RGB`.
2. Enter password `rgb-control`.
3. Open `http://192.168.4.1` if the dashboard does not open automatically.

The dashboard provides dedicated Segment 1 and Segment 2 selectors. Each
segment can have its own color and brightness, or the **Link segments** switch
can keep them synchronized. It also includes color presets and four effects:
solid, pulse, color cycle, and alternating segments. Effect speed is adjustable.
When segments are unlinked, the dashboard retains each segment's slider and
color values while switching between the two selectors.
The onboard NeoPixel on GPIO 48 mirrors the currently selected segment's live
output, including its brightness and animation frames. During the alternating
effect, the dashboard selector and onboard NeoPixel switch between Segment 1
and Segment 2 in sync with the strip outputs.

Settings are saved to the ESP32 only after changes settle, reducing flash wear,
and are restored after a restart. Effects are non-blocking, so dashboard and
serial-debug responsiveness continue while animations run. PWM runs at 5 kHz
with 8-bit brightness control. Change GPIO or PWM settings in
`include/rgb_config.h`; change the local network name or password near the top
of `src/main.cpp`.

## Build and upload

Connect the board at `/dev/cu.usbmodem2101`, then run:

```sh
pio run
pio run --target upload
pio device monitor
```

The upload command replaces the firmware currently stored on the board.

## Live serial debugging

Open the serial monitor after uploading:

```sh
pio device monitor
```

The controller emits newline-delimited JSON every 500 milliseconds and
immediately after every dashboard state change. Each record includes uptime,
connected dashboard clients, free heap memory, link and effect state, onboard
NeoPixel state, both
segments' selected settings, and the live GPIO PWM values. During an effect,
these live values show the animation changing in real time.

Example:

```json
{"type":"telemetry","linked":false,"effect":"pulse","segment_1":{"rgb":[124,92,255],"brightness":180,"outputs":{"R(GPIO5)":52,"G(GPIO6)":38,"B(GPIO4)":107}},"segment_2":{"rgb":[53,185,255],"brightness":140,"outputs":{"R(GPIO8)":17,"G(GPIO9)":59,"B(GPIO7)":82}}}
```

Segment 1 uses GPIO 5/6/4 for red/green/blue, and segment 2 uses GPIO 8/9/7.

Change `DEBUG_ENABLED` or `DEBUG_INTERVAL_MS` in `include/rgb_config.h` to
set the initial default or adjust its frequency. The **Enable serial debugging**
checkbox in the dashboard turns the stream on or off at runtime and remembers
the selection across reboots.
