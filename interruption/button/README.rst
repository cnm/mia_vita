Changes to support button
=========================

linuxrc-sdroot-readonly - Changed boot script to put ATX pin number 14 to 0 (to hold the system on)

It turns off green light and turns on the red one when the user can remove the finger from the button. After a few seconds, it turns the red led on again.

Installation
============

Just replace the linuxrc-sdroot-readonly script in the device (initrd partition).
