# Todo

Move the screen GPIOs fmor 3, 4, 5 to something like 10, 11, 12 to make
the wiring cleaner, and make it easier to reach the BOOTSEL button when
everything is wired up.

FIXME: The slot in the mid-plate is too narrow for the Schottky diode.
That's fine if we put the diode close to the Mini360 instead of close to
the Pico, or leave the diode on the Pico side and give it a longer tail.

FIXME: The screw terminal needs to be attached to the internal wires
and screwed to the enclosure after the lid/case is put on the stack.
Maybe instead of soldering the internal wires to the screw terminal I'll
try crimped female spade connectors?

FIXME: screen rotation changes the width/height, but the drawing doesn't compensate

FIXME: log scale for dimming the backlight

pio encoder

hmm, redrawing the main screen all the time crashes it

    i2c errors talking with the husb238 :-(

    looks like maybe bit flips in the communications?  sometimes i'd
    read 9V 2.25A, unlike the normal 9v 2.00A

Try switching from a Raspberry Pi Pico to a Waveshare RP2040 Zero, and
moving the screw terminal up one floor, and making the enclosure shorter.

Or try making my own board, optimized for this application.

    USB-C PD input connector / HUSB238 / RP2040 / Micro360 / Screw terminals

    maybe connectors for the knob & screen?

    extra USB connector for programming the RP2040, or can i share the
    USB-PD one?

    bootselect button near the USB connector, so it's reachable while
    the board is installed in the enclosure

    Maybe better to wait until a full PD/PPS solution
