# SmartCube

SmartCube is a modular, connectable LED cube platform. Each module is a small PCB containing addressable LEDs and sensors; modules mechanically and electrically connect using pogo pins and neodymium magnets. This monorepo contains KiCad PCB sources, a CadQuery 3D-model generator (that uses KiCad STEP exports), and firmware for the microcontrollers used across modules.

<!-- ![SmartCube](assets/cube.png)

TODO: Add image, once the cubes arrive -->

## Key features

- Modular PCB-based cube modules with addressable LEDs and Hall sensors
- Electrical connection via pogo pins; mechanical coupling via neodymium magnets
- ESP32 used as a power/aggregation/controller cube
- PY32 microcontrollers used for individual LED modules
- CadQuery scripts that generate printable enclosures using KiCad STEP references

## Project Structure

- [PCB](./PCB): KiCad projects for the LED module, power supply module and pogo pin connectors, as well as helper scripts for panelization.

- [3DModel](./3DModel): CadQuery scripts to generate 3D models of the cube modules and enclosures, using KiCad STEP exports as references.

- [Firmware](./Firmware): CMake-based firmware for the PY32 microcontrollers used in the LED modules, with support for flashing via ST-Link and esp-idf-based firmware for the ESP32 power/controller module.

A devcontainer exists for development for the 3DModel and panelization.
