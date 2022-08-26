# vkblam [![GitHub license](https://img.shields.io/badge/license-MIT-blue.svg)](https://raw.githubusercontent.com/Wunkolo/vkblam/main/LICENSE)

**vkblam** is a re-implementation of the Halo 1 blam engine's graphics library in vulkan.

Currently this is highly experimental and only generates static images and not a real-time interactable window.

Derelict|Prisoner
-|-
![](media/carousel.png) | ![](media/prisoner.png)

# Build guide
```
$ git clone https://github.com/Wunkolo/vkblam && cd vkblam
$ git clone https://github.com/vector-of-bool/cmrc external/cmrc
$ git clone https://github.com/mandreyel/mio external/mio
$ git clone https://github.com/g-truc/glm external/glm
$ mkdir _build && cd _build
$ cmake ..
$ make -j$(nproc) 
```

# Acknowledgements
* [Reclaimers](https://c20.reclaimers.net/)
* [Assembly](https://github.com/XboxChaos/Assembly)
* [Sparkedit](https://github.com/HaloMods/SparkEdit)
* [Swordedit](https://github.com/ChadSki/Swordedit)
* [Prometheus](https://github.com/HaloMods/Prometheus)
