parts=(
  # C2988082  # Pogo pin
  # C492414   # Pin header angled
  # C2843785  # RGB Led WS2812B
  # C7469098  # PY32F002BF15P6TU Microcontroller
  # C720477   # Tactile Switch (push button)
  # C7420984  # Linear Hall Effect Sensor
  # C2922787  # RGB Led SK6812-MINI-HS
  # C2883469  # 1x1 Pin Header Female
  # C5736265  # ESP32-C6-Mini-1-N4
  # C2765186  # USB Type C Connector
  # C6186     # Linear Voltage Regulator 5V -> 3.3V
  # C168855   # Bidirectional Logic Level Converter
)

JLC2KiCadLib "${parts[@]}" \
-dir lib \
-model_dir packages3d \
-footprint_lib CustomFootprints.pretty \
-symbol_lib_dir symbol \
-symbol_lib Custom_Parts
