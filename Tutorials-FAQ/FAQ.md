# Aerospace Village 2024 badge - FAQ

## GPS
### Lock
- make sure you have a full charge and put your badge somewhere it's likely to get a lock
- Run gpsmon to verify badge is receiving GPS signal
- using gpsctl to change to NMEA mode first might make this easier to read
- switch back to binary mode after verification
- just switching back to binary mode resolved my failure to get GPS lock
### Cache
- In the badge settings you can manually enter lat/long coords
- these will remain in place until a GPS lock is established
### Flashing
- flashing badge may resolve GPS issues, but is no guarantee
- manually set GPS coordinates to something far away from your actual position before flashing

## ADS-B
- The badge only collect 1090 MHz ADS-B, not 978 MHz. This is a hardware design choice and not a software configuration.
- You won't see any traffic on the radar display if the badge coordinates aren't correct. So if no GPS position you'll need to manually enter the position. That will remain in place until a GPS position is in locked.

## External Antenna
- When you solder the external antenna on, be careful not to knock off the tiny capacitor. If you do knock it off, in a pinch you can actually short across that cap as long as you’re careful to not apply DC bias to the port and use an antenna with decent out-of-band rejection. The main purpose of that cap is to block any DC voltage (power) that might show up on the SMA, so only AC-coupled RF signals can pass to protect the sensitive downstream parts. It’s also there as a sort of pre-filter to block any potential low frequency high power signals from entering like AM/FM broadcast radio if you attach an antenna that receives that (out-of-band for ADSB)
- If you attach an external antenna, you need to switch to it in the settings menu

## Connecting to the badge
## Ethernet over USB-C
- Windows 11 doesn't appear to support ethernet over USB-C. Unsure about Windows 10.
- Booted up Linux and this worked without issue. The Badge will issue a DHCP address.
- ssh root@192.168.100.1
### MacOS
USB connections to MacOS sometimes have problems because of the BMS
- Use a USB-C to USB-A cable
- Turn off the badge and turn it back on. Plug it into a power adaptor for a moment (preferably with a USB-A to USB-C cable) and unplug it. Then connect it to your Mac by a USB-C to USB-C cable. The BMS tries to do negotiation on power with the MAC but doesn’t do USB-C PD, so the BMS gets into a weird state. If you use USB-A to USB-C, the guess is that the BMS gets put into a different state where it doesn’t try to do PD negotiation
- Another thing to check is System Information under USB if it has connected as an Ethernet device correctly.

## Connecting to WiFi
- The internal keyboard doesn't have an underscore character. You'll need to configure it via terminal if you need the underscore.
- You can SSH into the badge and set up WiFi manually with wpa_supplicant cli tools or the config file.
- 2.4 GHz only

## Ethernet over WiFi
- remount the file system as read-write (see the readme file in /)
- set a password for the root account
- configure sshd_config to permitrootlogin yes
- there is an option to allow wireless SSH access with no password - don't be an idiot

## Connecting a keyboard
- Connecting a sometimes has issues. Unfortunately that's a bug with the allwinner otg Linux kernel driver... on boot up there's like a 60% chance the OTG controller fails to enable host mode and you'll need to reboot it to try again- one of those things that should be fixed eventually, but with allwinner not actually documenting their OTG peripheral it makes it a bit tricky to debug...

## Battery Reset
- You really shouldn't have to do this. There's a reset button on the back.
- [!] DANGER CLOSE [!]
- pull the battery for a few minutes
- when replacing the battery do not reverse the polarity or you will brick your badge
- **if you do this everyone will laugh at you and you will bring shame on your family name**