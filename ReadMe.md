# ESP32 Mortar Controller

## Components
- 6V-24V Battery
- Buck Converter ~1A
- ESP32 (DEVKITV1)
- Illuminated Button
- 2 potentiometers
- Relay or MOSFET
- Lora module (optional)

### ESP32

Have [this ESP32](https://www.circuitstate.com/pinouts/doit-esp32-devkit-v1-wifi-development-board-pinout-diagram-and-reference/)

<img src="./Misc/ESP32-DevKit-V1-Pinout.png" alt="KeySizes" width="800"/> 

Some pins should be left floating during boot, this pins can be used but will only use them if nothing left.

### Illiminated push button 
Have [this button: Ring, 5V, Momentary, Blue](https://www.aliexpress.com/item/4000032282063.html) 
Pin functions are marked on switch. Will use C (common) and NO (Normally open)

Use + and - for LED, note there is already a resistor inside so no current limiting resistor is needed.
Current is around 2mA so can be driven from GPIO pin.


## Plan



## Pinout
<img src="./Misc/Pinout.png" alt="KeySizes" width="800"/> 


## Software

  Use board type `DOIT ESP32 DEVKIT V1`

  Use [external interrupts](https://microcontrollerslab.com/esp32-external-interrupts-tutorial-arduino-ide/) for limit switch and ARM.  
  ARM will toggle the state so will need to account for switch bouncing, a simple delay could work.