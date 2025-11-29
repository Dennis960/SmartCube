# 3DModel

Generate and export 3D models of the SmartCube modules and enclosures using CadQuery scripts.

It is recommended to use the provided devcontainer for development, which includes all necessary dependencies.

Run

```bash
python main.py
```

inside the src folder to generate the models. The generated STEP files will be saved in the output folder.

## Troubleshooting

In case there is an issue with loading the kicad STEP files, delete the contents of the models folder and re-run the script to regenerate them.
