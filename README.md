# AV Badge 2024

The Aerospace Village badge for DC32 truly is both a feat of engineering and a work of art. The headlining feature of the badge is its ability to natively receive and display nearby aircraft using the ADS-B signals that most aircraft transmit. What normally takes specialized components and an SDR to receive the 1090 MHz signal, this badge uses some hardware hacks and clever trickery to make regular components do the work.
​
But this badge doesn't just display aircraft, it's also an entire linux single-board computer with built-in Wi-Fi, GPS, dual-core processor, 128MB DDR3 RAM, and 8GB of eMMC storage. Want to hack it? Go ahead and connect with SSH or plug in a keyboard to the USB port and open up a terminal. It even has a microSD slot.
​
### What else can the badge do?
- Fully integrated ADS-B receiver with onboard PCB antenna and optional external antenna
- GPS with own-ship position on a moving map
​- Expose the Dump1090 data to the network over Wi-Fi or USB Ethernet
- Video playback
- Video game emulation
- Expandable storage if you provide your own microSD card
- Replaceable 18650 battery with battery state of charge readout and fast charging via USB-C PD
- USB-C Dual Role Port
- And of course, lots of blinky lights and an SAO connector that supports I2C, UART, CAN Bus, and more
- Anything else you program it to do, it runs Linux after all :)

### YouTube videos on development:
- HelicoptersofDC "behind the badge" chat - [link](https://youtu.be/6bwVIX6AgdQ)
- DC32 presentation on the badge - [link](https://youtube.com/watch?v=dDFtkjYx0V8)

### Get in touch:
- Aerospace Village website - [https://www.aerospacevillage.org/](https://www.aerospacevillage.org/)
- Discord - [https://discord.gg/gV4EWuk](https://discord.gg/gV4EWuk)
- X/Twitter - [https://x.com/secureaerospace](https://x.com/secureaerospace)

### Links to cases:
- Case with stand - [https://github.com/LoganSound/AerospaceBadge2024_AerodynamicCase](https://github.com/LoganSound/AerospaceBadge2024_AerodynamicCase)
- Travel case - [https://www.printables.com/model/1046877-case-for-aerospace-village-badge-dc32](https://www.printables.com/model/1046877-case-for-aerospace-village-badge-dc32)
- Bumper case - [https://cad.onshape.com/documents/afde437b5cff13e66e64d5b8/w/c67f180588a5d683aad517e6/e/278043b948701274f9d3ab34?renderMode=0&uiState=672fa746f270b81d2e00e4de](https://cad.onshape.com/documents/afde437b5cff13e66e64d5b8/w/c67f180588a5d683aad517e6/e/278043b948701274f9d3ab34?renderMode=0&uiState=672fa746f270b81d2e00e4de)
- Bezel - [hardware/bezel.stl](hardware/bezel.stl)

### Software releases
- v1.1 - [https://github.com/AerospaceVillage/avBadge_2024/releases/tag/v1.1](https://github.com/AerospaceVillage/avBadge_2024/releases/tag/v1.1)
- v2.0 - [https://github.com/AerospaceVillage/avBadge_2024/releases/tag/v2.0](https://github.com/AerospaceVillage/avBadge_2024/releases/tag/v2.0)

### Tutorials and FAQ
- How to create a QT widget for the AV Badge - [https://github.com/AerospaceVillage/avBadge_2024/tree/main/Tutorials-FAQ/AV%20Badge%20Development%20Tutorial](https://github.com/AerospaceVillage/avBadge_2024/tree/main/Tutorials-FAQ/AV%20Badge%20Development%20Tutorial)
- How to build WingletOS - [https://github.com/AerospaceVillage/avBadge_2024/tree/main/Tutorials-FAQ/building_WingletOS.md](https://github.com/AerospaceVillage/avBadge_2024/tree/main/Tutorials-FAQ/building_WingletOS.md)
- FAQ - [https://github.com/AerospaceVillage/avBadge_2024/tree/main/Tutorials-FAQ/FAQ.md](https://github.com/AerospaceVillage/avBadge_2024/tree/main/Tutorials-FAQ/FAQ.md)
