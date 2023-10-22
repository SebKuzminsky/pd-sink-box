# Introduction

This is a device that makes available the electrical power from a USB
PD power source, such as most cheap modern phone chargers.

A USB PD source offers one or more voltages (5V, 9V, 12V, 15V, 18V,
and 20V), each with its own current limit.

This device shows what voltages are offered by the connected USB PD
source (and their advertised max currents), and lets the user choose one.
The specified voltage then appears on the screw terminals.


# Design

This device is based on the HUSB238 USB PD Sink chip and an RP2040
microcontroller.

The user interface is a small color screen and a rotary/push-button knob.

The device also includes a Mini360 buck converter to power the control
electronics and the screen from whatever voltage the USB PD source is
currently supplying.


## Bill of materials

HUSB238:
    <https://www.adafruit.com/product/5807>
    <https://en.hynetek.com/2421.html>

RP2040 (Raspberry Pi Pico):
    <https://www.raspberrypi.com/products/raspberry-pi-pico/>

Mini360:
    <https://components101.com/modules/mini360-dc-dc-buck-converter-module>
    <https://www.ebay.com/itm/132416658988>

1.14 inch color LCD display module, 240x135:
    <https://www.waveshare.com/1.14inch-lcd-module.htm>
    <https://www.waveshare.com/wiki/1.14inch_LCD_Module>

Rotary/push-button knob:
    <https://www.adafruit.com/product/377>

Panel mount screw terminals:
    <https://www.ebay.com/itm/301753692259>
    <https://www.ebay.com/itm/302724734282>

Custom 3d printed enclosure (FCStd & STLs in this repo)

Misc fasteners (see below)

Misc hookup wire, mostly 24 AWG stranded or similar, plus a short length
of red & black in 18 AWG.


# Enclosure

The enclosure is 3d printed from the FreeCAD file in this repo.
The plastic parts include standoffs with holes that I drill to size and
tap M2.  This works surprisingly well.

I think the #4-40 flat head screws i have will work well for mounting
the screen but we'll see.


## Design

The enclosure is designed to make the device easier to assemble.  You can
have all the the electronics assembled and mounted, but have the case
off so everything is visible and accessible.

The stack looks like this, starting at the bottom:

1.  The "base-plate" holds the HUSB238 and Mini360.  It has standoffs
    for the mid-plate.

2.  The "mid-plate" is mounted on the standoffs of the base-plate.
    It has standoffs for mounting the Pico, and standoffs for the screen.
    There are slots in the mid-plate that allow easy wiring from below
    (power from the Mini360 and I2C to the HUSB238).

3.  The "lid" or "case" is an inverted bowl that sits on top of the
    mid-plate.  The screen and the encoder knob both mount to the lid.
    The screws that attach the screen to the lid go into the standoffs
    on the mid-plate.  The encoder knob is panel-mounted to the lid with
    its own nut and washer.

You can unscrew the screen screws and the nut on the encoder knob and
take the lid off, and run the whole thing without the case.  You can
attach the screen to its standoffs on the mid-plate without the lid
(though the encoder knob has nowhere other than the lid to mount to,
so it's free floating in this configuration).


## Slicer

I configured my slicer to print 5 perimeters, to make the standoffs
stronger and more able to take a tap.

I configured my slicer to print the first layer at elevated temperatures,
to aid with print-bed adhesion and avoid corner lifting: 210°C nozzle,
65°C bed.  The rest of the part is printed at 190°C/60°C.  This is
working great.


## Fasteners

screen to lid: 4x #4-40 flat head FIXME: switch to m2.5 for consistency?

pico to mid-plate: 4x m2-0.40x6 button head or pan head

mid-plate to case: 4x m2-0.40x6 button head or pan head

husb238 to bottom-plate: 2x m2-0.40x4 button head or pan head

bottom-plate to case: FIXME
    4x m2-0.40x6 philips flat head (3.5-3.8 mm diameter head)
    4x m2-0.40x6 socket flat head (3.7-4 mm diameter head)
    4x m2-0.40x6 philips pan or button head (3.5-3.8 mm diameter head)
    what heads will fit?  do i need to move the holes in from the edge?

screw terminal to case: 2x SHCS m2x6


## Todo

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

Needs a name

    piddly-power (play on PD, piddly means small)

    trigger-happy (because it's a USB-PD trigger, but sounds a bit violent)

    power-trip (because it's a portable power supply)




# Assembly


## Enclosure prep

Drill the holes to be tapped M2 with #53 (0.059 inch, 1.499 mm).

    6x on the Base Plate

    5x on the Mid Plate

    2x on the Lid

Tap them M2.  Only the HUSB238 holes are any challenge, since for
aesthetic reasons they are blind.

Drill the M2 clearance holes with #44 (0.086inch, 2.184mm).

    4x on the Mid Plate

    3x on the Mid Plate End Cap

Drill the holes to be tapped #4-40, on the screen standoffs #43 (0.089
inch, 2.261 mm).

Tap the #4-40 holes on the screen standoffs.

Drill the #4-40 clearance holes in the lid #30 (0.129 inch, 3.264 mm).


## Base plate

Solder the I2C leads on the HUSB238.  Blue for SDA, yellow for SCK.
These are about 20 mm long.

Solder power leads to the HUSB238 board, one pair long to go to the
screw terminal and one pair short to go to the Mini360.

Solder the short power leads from the HUSB238 to the input points on
the Mini360.

Solder power output leads to the Mini360, including a Schottky diode on
the +VOUT line, but *do not* connect them anywhere yet.  Note: the diode
does not fit through the slot in the mid-plate, so ensure there's an
adequate lead on the Pico side.

Mount the HUSB238 and Mini360 to the base-plate.

Connect the HUSB238 to power and adjust the pot on the Mini360 to get
about 3.3V after the diode.


## Mid plate

Mount the Pico on its standoffs on the mid-plate.  The USB connector and
BOOTSEL button should be on the side that's *not* covered by the display.

Mount the mid-plate on its standoffs on the base-plate.

Route the 3.3V and GND output lines from the Mini360 through the slots
in the mid-plate to pins 39 (VSYS) an 38 (GND) and solder them in place.

Solder leads between the rotary encoder knob and the Pico.

    The top of the square body of the rotary encoder will be about level
    with the tops of the screen standoffs, so the leads should be about
    15 mm long.

    Pico GPIO | Pico Pin | Knob pin
    -----------+----------+-----------
    GPIO 0    | 1        | A
    GND       | 3        | C (center on 3-pin side)
    GPIO 1    | 2        | B
    -----------+----------+-----------
    GND       |          | either pin on 2-pin side, jumper to C (GND pin) on other side of knob
    GPIO 2    | 4        | other pin on 2-pin side
    -----------+----------+-----------

    If the side with the three pins is facing you, with the shaft up, then
    the pins are A, C, B.

    Connect one of the two ground pins on the knob to pin 3 (GND) on the Pico.
    Jumper the two grounds together, but run the jumper wire as close to the
    encoder body as possible so it clears the micro-USB connector on the Pico.

    When stripping the wire to solder to the pico, very short exposed
    conductors are sufficient, 2mm or so.

Solder the I2C leads from the HUSB238 to the Pico:

    Name | Pico pin | Direction | HUSB238 | Color
    ------+---------+-----------+---------+-------
    SDA  | 21       | <->       | SDA     | blue
    SCL  | 22       |  ->       | SCL     | yellow

Temporarily mount the display on its standoffs.

Solder leads between the display and the Pico:

 Pico Name          | Pico Pin | Direction | Display
--------------------+----------+-----------+--------
 3.3V               | 36       |  ->       | VCC
 GND                | 23       | <->       | GND
 spi1 TX (GPIO 15)  | 20       |  ->       | DIN
 spi1 SCK (GPIO 14) | 19       |  ->       | CLK
 GPIO 3             | 5        |  ->       | CS
 GPIO 5             | 7        |  ->       | D/C
 GPIO 4             | 6        |  ->       | RST
 PWM 4B (GPIO 9)    | 12       |  ->       | BL


## Lid

Remove the temporary screws holding the screen on its standoffs.

Remove the nut & washer from the rotary encoder knob.

Slide the lid carefully over the main assembly.

    The rotary encoder knob should come up through its hole.  Attach it
    with its washer and nut.

    Align the mounting holes in the screen with the holes in the lid
    and in the standoffs, and screw it securely into place.




# I2C

The RP2040 on the Pico talks to the HUSB238 via I²C.  It uses i2c
instance number 0, with SCL on GPIO 17 (pin 22) and SDA on GPIO 16
(pin 21).


# SPI

The RP2040 on the Pico talks SPI to the ST7735 TFT LCD screen.  It uses
spi instance number 1, with SCK on GPIO 14 (pin 19) and TX on GPIO 15
(pin 20).


# UI

The UI will show what voltage/max-current options are available, and
let the user select one.

It'd be cool if it also showed the current draw of the load, but the
HUSB238 can't tell you that, it'd be extra circuitry I'd have to add.


## Display

1.14 inch color LCD display module, 240x135:
    <https://www.waveshare.com/1.14inch-lcd-module.htm>
    <https://www.waveshare.com/wiki/1.14inch_LCD_Module>

Driver library:
    <https://github.com/tuupola/hagl>
    <https://github.com/tuupola/hagl_pico_mipi>


## Rotary encoder knob

This is a good rotary encoder for an embedded UI:
<https://www.adafruit.com/product/377>

The manufacturer is Bourns, the part number is PEC11-4215F-S24

<https://www.digikey.com/en/products/detail/bourns-inc/PEC11R-4215F-S0024/4499665>

The knob itself is an Eagle 450-BA161

Rotates with gray-code encoder and detents, plus push-to-click.

I can maybe save $0.50 - $1 per unit (encoder + knob) by buying from
Mouser instead of Adafruit, doesn't seem worth it.




# Todo

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

Handle non-PD power sources better:

    husb238 found!
    PD_STATUS0: 0x00
        PD source providing -1 V
        PD source max current 0.50 A
    PD_STATUS1: 0x4c
        CC_DIR: CC2 is connected to CC
        ATTACH: unattached mode
        PD response: success
        5V contract voltage: 5V
        5V contract max current: 0.00 A
    SRC_PDO_5V: 0x00 (not detected, -1.00A max)
    SRC_PDO_9V: 0x00 (not detected, -1.00A max)
    SRC_PDO_12V: 0x00 (not detected, -1.00A max)
    SRC_PDO_15V: 0x00 (not detected, -1.00A max)
    SRC_PDO_18V: 0x00 (not detected, -1.00A max)
    SRC_PDO_20V: 0x00 (not detected, -1.00A max)
    SRC_PDO: 0x00
        no PDO selected
    GO_COMMAND: 0x00
    PD contract: -1V 0.50A

    and if i turn the knob it loops forever over the PDOs looking for
    one that's available

When connected to a PD source:

    husb238 found!
    PD_STATUS0: 0x13
        PD source providing 5 V
        PD source max current 1.25 A
    PD_STATUS1: 0x4f
        CC_DIR: CC2 is connected to CC
        ATTACH: unattached mode
        PD response: success
        5V contract voltage: 5V
        5V contract max current: 3.00 A
    SRC_PDO_5V: 0x8a (detected, 3.00A max)
    SRC_PDO_9V: 0x8a (detected, 3.00A max)
    SRC_PDO_12V: 0x8a (detected, 3.00A max)
    SRC_PDO_15V: 0x8a (detected, 3.00A max)
    SRC_PDO_18V: 0x00 (not detected, -1.00A max)
    SRC_PDO_20V: 0x8a (detected, 3.00A max)
    SRC_PDO: 0x00
        no PDO selected
    GO_COMMAND: 0x00

    PD_STATUS0 voltage == 0 indicates a non-PD source



# Old

The HUSB238 output power goes to a screw terminal block for the user to
use, and also to a small buck converter that powers the Pico via VSYS
(protected by a 1N5817 schottky diode).  The HUSB238 can't provide power
below 5V, so i just need a buck converter, never a boost converter.
This way I can still plug the Pico into a computer via USB for
development/debugging and "UI-over-serial".

From the Pico W Datasheet:

    "Raspberry Pi Pico W uses an on-board buck-boost SMPS which is able
    to generate the required 3.3V (to power RP2040 and external circuitry)
    from a wide range of input voltages (~1.8 to 5.5V)."

    "If the USB port is not going to be used, it is safe to power Pico
    W by connecting VSYS to your preferred power source (in the range
    ~1.8V to 5.5V)."

The Pico W has a RT6154 buck-boost SMPS onboard:
<https://www.richtek.com/Products/Switching%20Regulators/Buck-Boost%20Converter/RT6154ART6154B?sc_lang=en&specid=RT6154A/RT6154B>

This i2c programmable buck-boost converter looks super interesting:
<https://www.monolithicpower.com/en/mp2651.html>.  Maybe one for Vout
and one to run the Pico?

The Waveshare "RP2040 Plus" <https://www.waveshare.com/rp2040-plus.htm>
uses the MP28164 "high efficiency DC-DC buck-boost chip".  And also a
TPS63000, says the wiki <https://www.waveshare.com/wiki/RP2040-Plus>?


# Features

The basic feature is to provide power from a USB PD source to a pair of
screw terminals, and let the user select the PDO (voltage and max current)
from what the source offers.

An actual voltage and current measurement on the output would be neat,
maybe using <https://www.adafruit.com/product/904>.

USB-PD does not give fine grained control over voltage and current,
but I think USB PPS does?
