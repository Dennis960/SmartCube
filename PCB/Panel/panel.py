from kikit import panelize_ui_impl as ki
from kikit.fab import jlcpcb
from kikit.units import mm, deg
from kikit.panelize import Panel, BasicGridPosition, Origin
from pcbnewTransition.pcbnew import LoadBoard, VECTOR2I
from pcbnewTransition import pcbnew
from itertools import chain
import os

path_to_script = os.path.dirname(os.path.abspath(__file__))
# Custom config
# fmt: off
module_path = os.path.abspath(os.path.join(path_to_script, "../Module/Module.kicad_pcb"))
shield_hall_sensor_path = os.path.abspath(os.path.join(path_to_script, "../ShieldHallSensor/ShieldHallSensor.kicad_pcb"))
pogo_connector_path = os.path.abspath(os.path.join(path_to_script, "../PogoConnector/PogoConnector.kicad_pcb"))
output_path = os.path.abspath(os.path.join(path_to_script, "SmartCubePanel.kicad_pcb"))
# KiKit Panel Config (Only deviations from default)

layout = {
    "hspace": "2mm",
    "vspace": "2mm"
}
tabs = {
    "type": "annotation",
    "vwidth": "5mm",
    "hwidth": "5mm",
    "footprint": "kikit:Tab"
}
cuts = {
    "type": "mousebites",
    "drill": "0.6mm",
    "spacing": "1mm",
    "offset": "0mm"
}
framing = {
    "type": "railslr"
}
tooling = {
    "type": "4hole",
    "hoffset": "2.5mm",
    "voffset": "2.5mm",
    "size": "1.152mm"
}
fiducials = {
    "type": "4fid",
    "hoffset": "3.85mm",
    "voffset": "8mm",
    "opening": "1mm"
}
text = {
    "type": "simple",
    "hoffset": "2mm",
    "hjustify": "left",
    "orientation": "90deg",
    "text": "JLCJLCJLCJLC",
    "anchor": "ml"
}
post = {
    "dimensions": "True"
}

# Obtain full config by combining above with default
preset = ki.obtainPreset([], layout=layout, tabs=tabs, cuts=cuts, framing=framing,
                         tooling=tooling, fiducials=fiducials, text=text, post=post)


# Adjusted `panelize_ui#doPanelization`

# Prepare
module = LoadBoard(module_path)
shield_hall_sensor = LoadBoard(shield_hall_sensor_path)
pogo_connector = LoadBoard(pogo_connector_path)
panel = Panel(output_path)


panel.inheritDesignSettings(module)
panel.inheritProperties(module)
panel.inheritTitleBlock(module)


# Manually build layout. Inspired by `panelize_ui_impl#buildLayout`
source_area_module = ki.readSourceArea(preset["source"], module)
source_area_shield_hall_sensor = ki.readSourceArea(preset["source"], shield_hall_sensor)
source_area_pogo_connector = ki.readSourceArea(preset["source"], pogo_connector)

# Store number of previous boards (probably 0)
substrateCount = len(panel.substrates)

# ----------------------------
# Setup
# ----------------------------
panelOrigin = VECTOR2I(0, 0)
hspace = 0 * mm
vspace = 0 * mm

# sizes (used to vertically center shields)
module_w = source_area_module.GetWidth()
module_h = source_area_module.GetHeight()
shield_w = source_area_shield_hall_sensor.GetWidth()
shield_h = source_area_shield_hall_sensor.GetHeight()
pogo_w = source_area_pogo_connector.GetWidth()
pogo_h = source_area_pogo_connector.GetHeight()

assert module_w == shield_w, "Module and shield widths must be equal for checkerboard layout"
assert module_h == shield_h, "Module and shield heights must be equal for checkerboard layout"

module_positions = []
module_positions_y = []
for col in range(3):
    x = panelOrigin.x + col * (module_w + hspace) + module_w // 2
    for row in range(4):
        y = panelOrigin.y + row * (module_h + vspace) + module_h // 2
        module_positions.append((row, col, x, y))
        if not y in module_positions_y:
            module_positions_y.append(y)

module_max_x = max(x for (row, col, x, y) in module_positions) + module_w // 2

pogo_positions = []
for col in range(6):
    for y in module_positions_y:
        x = module_max_x + hspace + col * (pogo_w + hspace) + pogo_w // 2
        pogo_positions.append((x, y))

for (row, col, x, y) in module_positions:
        if (row + col) % 2 == 0:
            panel.appendBoard(
                module_path,
                VECTOR2I(x, y),
                origin=Origin.Center,
                sourceArea=source_area_module,
                inheritDrc=False
            )
        else:
            # vertically center shield relative to module
            panel.appendBoard(
                shield_hall_sensor_path,
                VECTOR2I(x, y),
                origin=Origin.Center,
                sourceArea=source_area_shield_hall_sensor,
                inheritDrc=False
            )

for (x, y) in pogo_positions:
        panel.appendBoard(
            pogo_connector_path,
            VECTOR2I(x, y),
            origin=Origin.Center,
            sourceArea=source_area_pogo_connector,
            inheritDrc=False
        )

# Collect set of newly added boards
substrates = panel.substrates[substrateCount:]

# Prepare frame and partition
framingSubstrates = ki.dummyFramingSubstrate(substrates, preset)
panel.buildPartitionLineFromBB(framingSubstrates)
backboneCuts = ki.buildBackBone(preset["layout"], panel, substrates, preset)


# --------------------- Continue doPanelization

tabCuts = ki.buildTabs(preset, panel, substrates, framingSubstrates)

frameCuts = ki.buildFraming(preset, panel)


ki.buildTooling(preset, panel)
ki.buildFiducials(preset, panel)
for textSection in ["text", "text2", "text3", "text4"]:
    ki.buildText(preset[textSection], panel)
ki.buildPostprocessing(preset["post"], panel)

ki.makeTabCuts(preset, panel, tabCuts)
ki.makeOtherCuts(preset, panel, chain(backboneCuts, frameCuts))


ki.buildCopperfill(preset["copperfill"], panel)

ki.setStackup(preset["source"], panel)
ki.setPageSize(preset["page"], panel, module)
ki.positionPanel(preset["page"], panel)

ki.runUserScript(preset["post"], panel)

ki.buildDebugAnnotation(preset["debug"], panel)


footprints = panel.board.Footprints()
for footprint in footprints:
    reference = footprint.GetReference()
    if "KiKit_" in str(reference):
        footprint.SetExcludedFromBOM(True)
        footprint.SetExcludedFromPosFiles(True)


panel.save(reconstructArcs=preset["post"]["reconstructarcs"],
           refillAllZones=preset["post"]["refillzones"])
