librem README
=============


librem is a Audio and video processing media library

- Copyright (C) 2010 - 2019 Creytiv.com
- Copyright (C) 2020 - 2021 Baresip Foundation (https://github.com/baresip)

[![Build](https://github.com/baresip/rem/actions/workflows/build.yml/badge.svg)](https://github.com/baresip/rem/actions/workflows/build.yml)


## Features

* Audio buffer
* Audio sample format conversion
* Audio file reader/writer
* Audio mixer
* Audio resampler
* Audio tone generator
* Audio codec (G.711)
* DTMF decoder
* Video mixer
* Video pixel converter
* FIR-filter


## Building

librem is using GNU makefiles, and [libre](https://github.com/baresip/re)
must be installed before building.


### Build with default options

```
$ make
$ sudo make install
$ sudo ldconfig
```

## License

The librem project is using the BSD license.


## Contributing

Patches can sent via Github
[Pull-Requests](https://github.com/baresip/rem/pulls)


## Modules
```
Audio Modules:

  name:     status:       description:

* aac       unstable      Advanced Audio Coding (AAC)
* au        testing       Base audio types
* aubuf     testing       Audio buffer
* auconv    unstable      Audio sample format conversion
* aufile    testing       Audio file reader/writer
* aumix     unstable      Audio mixer
* auresamp  unstable      Audio resampler
* autone    testing       Tone/DTMF generator
* dtmf      unstable      DTMF decoder
* g711      stable        G.711 audio codec




Video Modules:

  name:     status:       description:

* avc       unstable      Advanced Video Coding (AVC)
* h264      unstable      H.264 header parser
* vid       testing       Base video types
* vidconv   testing       Colorspace conversion and scaling
* vidmix    unstable      Video mixer




Generic modules:

* dsp       testing       DSP routines
* flv       unstable      Flash Video File Format
* fir       unstable      FIR (Finite Impulse Response) filter
* goertzel  unstable      Goertzel Algorithm
```




## Specifications:

* ITU-T G.711 Appendix I and Appendix II


## Supported platforms

Same as [libre](https://github.com/baresip/re)


## Related projects

* [libre](https://github.com/baresip/re)
* [retest](https://github.com/baresip/retest)
* [baresip](https://github.com/baresip/baresip)



## References

https://github.com/baresip
