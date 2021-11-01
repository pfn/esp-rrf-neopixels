# Overview

This project is designed to handle NeoPixel control on an ESP8266 (Wemos D1 mini) and offload it from RepRapFirmware.

<iframe width="560" height="315" src="https://www.youtube.com/embed/OXgH2lOdC04" title="YouTube video player" frameborder="0" allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture" allowfullscreen></iframe>

## Features

* WiFi-enabled
* WiFi captive-portal for SSID/PSK configuration
* OTA configuration
* WiFi connectivity indicator (pixel 0 blinks when disconnected)
* Animations:
  * Boot animations
  * Printer connection status (ESP to printer)
  * Printer status (idle, paused, firmware updating, busy)
  * Printing status (file progress 0-100%)
  * Heater status (heating, cooling, maintaining, off)
  * Fan status (0-100% duty cycle)

### Coming Soon

* Web UI for configuration, status, diagnostics, hooks, etc.
* Custom hooks (triggering updates to an HTTP/mqtt endpoint, etc.)
* Lua scripting
* `M408` parsing support for BTT TFT users in spliced/passthrough mode

## Setup

### Prequisites

* ESP8266 dev board (Wemos D1 mini is my preferred option)
* One or more neopixel rings, strips, etc. of your choice
* VSCode + PlatformIO

### Building

TODO

### Flashing

TODO more clarity

* Flash the filesystem, and the firmware image to the ESP device using the `d1_mini` environment. Subsequent updates can be done over the air (WiFi) using the `d1_mini_OTA` environment

### WiFi

TODO more clarity

1. First boot will launch a WiFi captive portal (navigate to any uncached address in a browser) using the AP name `rrf-neopixel`
2. Configure SSID/PSK as necessary in the portal and save
3. Optionally change the hostname from `rrf-neopixel` if desired

Once configured and rebooted, the device should come online and be available at `http://rrf-neopixel.local/` (or whatever you've set for HOSTNAME`.local`). However there is no WebUI yet so `/` will result in a 404, see [HTTP end-points](#http-end-points) below.

### Wiring

* D8 = TX => RRF UART.rx
  * If using a PanelDue on Duet2Wifi, D8 can be left disconnected. The PanelDue will send M409 to the printer for us.
* D7 = RX => RRF UART.tx
  * There is an option here for PanelDue connectivity, wiring of the PanelDue can be spliced in a T such that RRF TX splits and connects to the PanelDue RX and ESP D7, *or* RRF TX can connect to ESP D7, and ESP D4 can connect to PanelDue RX.
* NeoPixels
  * Default config.json:
    * D3 (GPIO0) = NeoPixel 1 Data-In
    * D2 (GPIO4) = NeoPixel 2 Data-In
    * D1 (GPIO5) = NeoPixel 3 Data-In
  * The pin assignment is configurable, and up to 6 NeoPixel devices are allowed. NeoPixel entries can also be deleted out of the config.json to free up pins for other purposes.
  * Having individual neopixel devices per pin makes wiring more flexible for "remote" locations in the printer without having to daisy-chain pixels together. Future enhancements could allow better daisy-chain support.
* GND, 5V should be obvious, splice/T these as necessary to send power to the appropriate neopixels

### Configuration

The default configuration looks like so:

```json
{
    "neopixels": [
        {
            "pin": 0,
            "startup_color": "#00ffff",
            "display_item": 0,
            "type": 82,
            "offset": 0,
            "count": 16,
            "brightness": 8,
            "reverse": true,
            "temp_base": 20
        },
        ... snip ... 
    ],

    "colors": {
        "state": {
            "starting": "#0000ff",
            "updating": "#ffff00",
            "paused": "#8000a0",
            "changingTool": "#ffff00",
            "busy": "#ffff00",
            "halted": "#ffaa00",
            "idle": "#ffffff"
        },
        "heater": {
            "heating": "#ff0000",
            "cooling": "#0000ff",
            "secondary": "#ff4040"
        },
        "fan": {
            "active": "#80ff80",
            "secondary": "#007777"
        },
        "printing": {
            "done": "#00ff00",
            "secondary": "#40ff40"
        }
    },
    "swap_serial": true,
    "query_interval": 1001
}
```

* `neopixels`: contains an array of neopixel configuration elements (0-6 elements)
* `neopixels[INDEX].pin`: data pin assignment, required, if not present will cause the element to be ignored
* `neopixels[INDEX}.startup_color`: startup animation color, first color of the animation, if not present will default to whatever happens to be in memory at this location, syntax is hex `#RRGGBB`
* `neopixels[INDEX].display_item`: printer item to display
  * `0` = highest priority item to render:
    * a heating heater
    * a faulted heater
    * printing, pause/resume, busy
    * first active heater
    * first standby heater
    * first heater off but still warm
    * first fan running
    * remaining printer statuses
  * `0x10` means the lower 3 bits specifies a FAN
    * example: any fan = `0x10 | 0x7` = 23
    * example: fan1 = `0x10 | 0x1` = 17
  * `0x8` means the lower 3 bits specifies a HEATER
    * example: heater2 = `0x8 | 0x2` = 10
  * `0x7` means display first active 0-6
  * `0x20` set as a flag with neither of `0x10` or `0x8` means display printer status *only*
  * Anything else not matching the above rules will hit the `0x20` case
* `neopixels[INDEX].type`: specifies neopixel type to the Adafruit library, default is `NEO_GRB`=`82`
  * other common values would be `NEO_RGB`=`6`, or with 400KHz signalling + `256` rather than the default 800KHz signalling
* `neopixels[INDEX].offset`: allows setting the starting point of animations wherever you would like (allows mis-aligned placement of multiple rings, etc). Default=`0`
* `neopixels[INDEX].count`: number of pixels in this device, default=`16`
* `neopixels[INDEX].brightness`: brightness for normal operation, range 0-255 [0 would be off], default=`8`,
* `neopixels[INDEX].reverse`: whether to reverse pixel index ordering, adafruit 16 pixel rings have reversed ordering, set this `true` to reverse it, default=`true`
* `neopixels[INDEX].temp_base`: temperature point to consider `0` pixels to light up in the progress indicator, default=`20`
* `colors`: contains various color configuration values, experiment as desired, no defaults, syntax is hex `#RRGGBB`
* `swap_serial`: move UART pins off of RX/TX onto `D7`/`D8` if `true`, default=`true` (this resovles the issue with the CH340C USB-UART cross-talking onto RX)
* `query_interval`: sets the time period, in milliseconds, between `M409` requests to RRF, default=`1000`

### HTTP end-points

* `/wificonfig`: replace the controller webserver with the WiFiManager WiFi configuration web server. Once replaced, navigate to `/` to re-configure WiFi SSID and PSK
* `/config.json`: `GET` to retrieve the configuration, `POST` to upload a new configuration
  * Using curl: `curl -F 'file=@config.json' http://rrf-neopixel.local/config.json`
* `/rx`: diagnostic endpoint, shows the last received buffer from RRF
* `/debuglog`: diagnostic endpoint, shows the output of `DEBUG` statements
* `/reset`: reboot the ESP

## Background

Prior to RepRapFirmware 3.0, there was no mechanism to directly control NeoPixels and several users of BLV printers took it upon themselves to develop Arduino sketches that would read status from RRF and process the information into visual indicators that could be rendered on neopixel rings. Subsequently, with the release of RRF 3.0 and newer versions, neopixel control is now available directly from the firmware, but it has limitations. Directly manipulating neopixels from RRF requires cpu time as the LED control requires an 800khz signal, and the control commands themselves are rudimentary with `M150` being able to specify RGB values, plus offset and a number of elements to fill. Animations and complex patterns would be difficult to implement directly from gcode in addition to the timing and processor utilization required. Printing would be most affected by this as gcode being sent during a job has the potential to cause movements to pause.

Problems and limitations of running on Arduino lead to the creation of this project.

* The CH340C USB-UART on Arduino poses a serious problem: it always drives the atmega RX pin high because it is tied to its TX output pin. Often, this results in other electronics having to sink a 5V signal (potentially catastrophically), and it, possibly, prevents other TX sources from driving LOW in order to send serial data.
* All configuration is in source-code, that means any desirable changes need to be flashed manually over USB and is often cumbersome.