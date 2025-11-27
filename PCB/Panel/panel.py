from kikit import panelize_ui_impl as ki
from kikit.fab import jlcpcb
from kikit.units import mm, deg
from kikit.panelize import Panel, BasicGridPosition, Origin, fromDegrees
from pcbnewTransition.pcbnew import LoadBoard, VECTOR2I
from pcbnewTransition import pcbnew
from itertools import chain
from shapely.geometry import box

import os

path_to_script = os.path.dirname(os.path.abspath(__file__))
# Custom config
# fmt: off
module_path = os.path.abspath(os.path.join(path_to_script, "../Module/Module.kicad_pcb"))
power_supply_path = os.path.abspath(os.path.join(path_to_script, "../PowerSupply/PowerSupply.kicad_pcb"))
pogo_connector_path = os.path.abspath(os.path.join(path_to_script, "../PogoConnector/PogoConnector.kicad_pcb"))
output_path = os.path.abspath(os.path.join(path_to_script, "SmartCubePanel.kicad_pcb"))
# KiKit Panel Config (Only deviations from default)

source = {
    "tolerance": "100mm"
}
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
preset = ki.obtainPreset([], source=source, layout=layout, tabs=tabs, cuts=cuts, framing=framing,
                         tooling=tooling, fiducials=fiducials, text=text, post=post)


# Adjusted `panelize_ui#doPanelization`

# Prepare
module = LoadBoard(module_path)
power_supply = LoadBoard(power_supply_path)
pogo_connector = LoadBoard(pogo_connector_path)
panel = Panel(output_path)


panel.inheritDesignSettings(module)
panel.inheritProperties(module)
panel.inheritTitleBlock(module)


# Manually build layout. Inspired by `panelize_ui_impl#buildLayout`
source_area_module = ki.readSourceArea(preset["source"], module)
source_area_power_supply = ki.readSourceArea(preset["source"], power_supply)
source_area_pogo_connector = ki.readSourceArea(preset["source"], pogo_connector)

# Store number of previous boards (probably 0)
substrateCount = len(panel.substrates)

# ----------------------------
# Setup
# ----------------------------
panelOrigin = VECTOR2I(0, 0)
hspace = 2 * mm
vspace = 2 * mm

# sizes
module_w = int(40.729*mm)
module_h = int(40.729*mm)
pogo_w = 8*mm
pogo_h = 20*mm
pogo_small_hspace = 2*mm
pogo_big_hspace = 4*mm
pogo_vspace = -4*mm

# Configurable column layout: list of (type, count) tuples
# Supported types: "module", "pogo"
module_columns = 2
module_rows = 3
pogo_columns = 3
pogo_rows = 8

separator_rail_thickness = 5*mm

module_positions: list[tuple[int, int]] = []
for col in range(module_columns):
    for row in range(module_rows):
        x = panelOrigin.x + col * (module_w + hspace) + module_w // 2
        y = panelOrigin.y + row * (module_h + vspace) + module_h // 2
        module_positions.append((x, y))

for i, (x, y) in enumerate(module_positions):
        if i == 0:
            # place power supply at top-left corner
            panel.appendBoard(
                power_supply_path,
                VECTOR2I(x, y),
                origin=Origin.Center,
                sourceArea=source_area_power_supply,
                inheritDrc=False,
                rotationAngle=fromDegrees(45),
                refRenamer=lambda board_id, ref: f"PSU_{board_id}_{ref}"
            )
        else:
            panel.appendBoard(
                module_path,
                VECTOR2I(x, y),
                origin=Origin.Center,
                sourceArea=source_area_module,
                inheritDrc=False,
                rotationAngle=fromDegrees(45),
                refRenamer=lambda board_id, ref: f"M_{board_id}_{ref}"
            )

# Calculate the widths of all columns
module_total_width = module_columns * module_w + (module_columns - 1) * hspace
module_total_height = module_rows * module_h + (module_rows - 1) * vspace

# Size of the middle rail: width 5mm, height = module height
# Positioned to the right of the modules, with hspace gap
minx = panelOrigin.x + module_total_width + hspace
maxx = minx + separator_rail_thickness
miny = panelOrigin.y
maxy = panelOrigin.y + module_total_height

# Add a divider rail between modules and connectors
panel.appendSubstrate(box(minx, miny, maxx, maxy))

pogo_origin_x = panelOrigin.x + module_total_width + hspace + separator_rail_thickness + hspace
pogo_max_height = pogo_rows * pogo_h + (pogo_rows - 1) * pogo_vspace
pogo_origin_y = panelOrigin.y + (module_total_height - pogo_max_height) // 2
# Pogo connector positions
def pattern_value(r, c, small, big):
    # Column 0
    if c == 0:
        return 0 if r % 2 == 0 else int(0.5 * (big - small))
    
    # Even rows: odd→big, even→small
    if r % 2 == 0:
        return big if c % 2 == 1 else small
    
    # Odd rows: even→small, odd→big (same logic, but after col0 it's identical)
    return small if c % 2 == 1 else big

def pattern_value_recursive(row, col, s, b):
    if col == 0:
        return pattern_value(row, col, s, b)
    return pattern_value(row, col, s, b) + pattern_value_recursive(row, col - 1, s, b)

pogo_positions: list[tuple[int, int, bool]] = []
for col in range(pogo_columns):
    for row in range(pogo_rows):
        total_hspace = pattern_value_recursive(row, col, pogo_small_hspace, pogo_big_hspace)
             
        x = pogo_origin_x + col * pogo_w + total_hspace + pogo_w // 2
        y = pogo_origin_y + row * (pogo_h + pogo_vspace) + pogo_h // 2
        flip = (col % 2 == 1) ^ (row % 2 == 0)
        pogo_positions.append((x, y, flip))

for (x, y, flip) in pogo_positions:
        panel.appendBoard(
            pogo_connector_path,
            VECTOR2I(x, y),
            origin=Origin.Center,
            sourceArea=source_area_pogo_connector,
            inheritDrc=False,
            rotationAngle=fromDegrees(180) if flip else fromDegrees(0),
            refRenamer=lambda board_id, ref: f"POGO_{board_id}_{ref}"
        )

# Collect set of newly added boards
substrates = panel.substrates[substrateCount:]

# Prepare frame and partition
framingSubstrates = ki.dummyFramingSubstrate(substrates, preset)
panel.buildPartitionLineFromBB(framingSubstrates)
# TODO: partition lines are note created correctly
panel.debugRenderPartitionLines()
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
