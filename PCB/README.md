# PCB

This folder contains the KiCad projects for the SmartCube LED module, power supply module, and pogo pin connectors. It also includes helper scripts for panelization of the PCBs for manufacturing.

The KiCad projects all use the same shared library located in the `libs` folder.

If having problems loading footprints or 3d models, make sure to add the symbol and CustomFootprints libraries in KiCad.

## Module

[./Module](./Module) contains the KiCad project for the main LED module PCB.

It includes addressable LEDs and Hall sensors.
Communication happens via PY32 microcontrollers using pogo pin connectors.

## Power Module

[./PowerModule](./PowerModule) contains the KiCad project for the power supply module PCB.

It provides 5V power to the cubes via pogo pin connectors.

At least one power module is required in a SmartCube assembly to provide power to the other modules.

## Pogo Pin Connectors

[./PogoConnectors](./PogoConnectors) contains the KiCad projects for the pogo pin connectors.

It is used to connect the modules both mechanically and electrically.

## Panelization

[./Panel](./Panel) contains scripts to panelize the PCBs for manufacturing. The panelization uses the kikit tool.
