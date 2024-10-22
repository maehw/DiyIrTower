# README

Can be used as a replacement of the original LEGO IR serial tower with programs and platforms such as

* [WebPBrick](https://www.webpbrick.com): web-based NQC IDE (see also: [github.com/maehw/WebPBrick/](https://github.com/maehw/WebPBrick/))
* [Bricx Command Center (BricxCC)](https://bricxcc.sourceforge.net/): 32-bit Windows IDE
* `firmdl3` command line download tool
* probably other software

to **load user programs and firmware** on the yellow LEGO Mindstorms RCX programmable brick.

![RCX firmware download setup](./doc/media/setup.jpg)

## Hardware

### Pin Assignment

Pin Assignment Table for the ATtiny13A:

| IC Pin | Pin    | Purpose              | Direction | Side of IC | 
|--------|--------|----------------------|-----------|------------|
| 1      | PB5    | Reset                | Unused    | Left       |
| 2      | PB3    | IR_LED_TX_PIN        | Output    | Left       |
| 3      | PB4    | PHOTO_RX_PIN         | Input     | Left       |
| 4      | GND    | Ground               | -         | Left       |
| 5      | PB0    | UART_RX_PIN          | Output    | Right      |
| 6      | PB1    | UART_TX_PIN          | Input     | Right      |
| 7      | PB2    | IR_ACTIVITY_LED_PIN  | Output    | Right      |
| 8      | VCC    | Supply Voltage (VCC) | -         | Right      |

### Parts

- ATtiny13A as microcontroller (calibrated internal 9.6 MHz oscillator as clock source; other uCs such as the ATtiny85 _may_ work as they have more SRAM and more flash memory, but timing values will need to be adjusted)
- Vishay Semiconductors TSOP4338 38 kHz infrared receiver (others 38 kHz infrared receivers _may_ work)
- Vishay Semiconductors TSAL6200 940 nm infrared TX LED (others 940 nm infrared TX LEDs _may_ work)

### Breadboard or PCB

tbc

### Firmware

Find the productive firmware in folder `./firmware`.

To build, run:

```shell
avr-gcc -g -Wall -Os -mmcu=attiny13 -o main.elf main.c
```

Want to check if it will it fit into the internal flash?

```shell
avr-size main.elf
   text	   data	    bss	    dec	    hex	filename
    776	      0	      7	    783	    30f	main.elf
```

Make sure to burn the fuses so that
- the calibrated internal 9.6 MHz oscillator is selected
- clock pre-scaler set to 1 (device is shipped with CKDIV8 programmed).

Reading fuses with avrdude (and in this case an `ehajo-isp` in-system programmer):
 
```
avrdude -c ehajo-isp -p t13a -U hfuse:r:-:h -U lfuse:r:-:h 
```

Flash the firmware:

```
avrdude -c ehajo-isp -p t13a -U flash:w:main.elf:e
```

### Disclaimer

LEGO® is a trademark of the LEGO Group of companies which does not sponsor, authorize or endorse this project.
