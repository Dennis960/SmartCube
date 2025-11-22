parts=(
  C2826552  # Pogo pin
  C492414   # Pin header angled
  C2843785  # RGB Led WS2812B
  C7469098  # PY32F002BF15P6TU Microcontroller
  C720477   # Tactile Switch (push button)
)

JLC2KiCadLib "${parts[@]}" \
-dir lib \
-model_dir packages3d \
-footprint_lib CustomFootprints.pretty \
-symbol_lib_dir symbol \
-symbol_lib Custom_Parts
