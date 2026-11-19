# This needs to be called on reboot to write own IP address to /mem/able-live.ip
/home/pi/able-live/tailscale/write_ip able-live >/home/pi/init.log 2>&1
/home/pi/gnius-live/gnius-live/gnius-live >/home/pi/gnius-live.log 2>&1
