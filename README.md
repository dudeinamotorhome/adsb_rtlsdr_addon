# ADSB_RTLSDR_ADDON

A Node.js native addon to decode aircraft ADS-B transmissions using a RTL-SDR USB receiver.

Built on a BeagleBoneBlack, Debian 8.5, Kernel 4.4-9-ti-r25, librtlsdr 0.5.3-3, Node v6.3.0, node-gyp v3.4.0

## Prerequisites

### librtlsdr

You will need to install librtlsdr-dev:

`apt-get install librtlsdr-dev`

This also installs librtlsdr0 for me.  For some reason for me it installed a kernel module I could not remove manually.  It also installed something to blacklist it in `/etc/modprobe.d/rtl-sdr-blacklist.conf`, so rebooting stopped it from loading and I could then access the radio.

There are also a set of rtl-sdr apps that you can use to test your radio that I found useful for development:

`apt-get install rtl-sdr`

You can run `rtl_test -t` to test out your radio.

### Node 6.x

The BeagleBone debian image I was using has an old version of node.  To update:

`curl -sL https://deb.nodesource.com/setup_6.x | sudo -E bash -`

`apt-get install nodejs`

### node-gyp

You'll need node-gyp to compile the addon:

`npm install node-gyp -g`

## Building

To build:

`node-gyp configure`

`node-gyp build`

Note that it dynamically links with librtlsdr.so, so that will need to be available in your ldconfig path.

## Node.js API

| Function | Description | Return Value | Arguments |
|:---------|:------------|:-------------|:----------|
| `getRadioList()` | Returns a list of all radios librtlsdr detects, using a combination of `rtlsdr_get_device_count()` and `rtlsdr_get_device_usb_strings()`.| An array of objects, with each object containing: `id`, `vendor`, `product`, `serial`.  If no devices are found the list will be empty.| None. |
| `initRadioByID(Int32, Boolean)` | Initializes specified radio, setting gain and frequency values. | Boolean - True if radio is initialized, otherwise false. | Int32 for the radio id; Boolean for enabling autogain (if false it will use the max available gain). |
| `getGainSettings()` | Returns the gain settings on the radio. | An array of objects: `autoGainEnabled`, `gainSettingInTenths_dB`, `deviceGainIn_dB`. | None. |
| `getFreqSettings()` | Returns the frequency settings on the radio. | An array of objects: `freqSettingsInHz`, `deviceFreqInHz`, `deviceFreqCorrectionInPPM`. | None. |
| `closeRadio()` | Closes the radio device. | None. | None. |

## License

TBD.

## Author

Jeff Powers, powerjm@gmail.com
