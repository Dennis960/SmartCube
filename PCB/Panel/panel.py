from kikit import panelize_ui_impl as ki
from kikit.fab import jlcpcb
from kikit.units import mm, deg
from kikit.panelize import Panel, BasicGridPosition, Origin, fromDegrees, Substrate
from pcbnewTransition.pcbnew import LoadBoard, VECTOR2I
from pcbnewTransition import pcbnew
from itertools import chain
from shapely.geometry import box, GeometryCollection, LineString, MultiLineString

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

def get_pogo_hspace(row, col):
    if col == 0:
        return 0 if row % 2 == 0 else int(0.5 * (pogo_big_hspace - pogo_small_hspace))
    if row % 2 == 0:
        return pogo_big_hspace if col % 2 == 1 else pogo_small_hspace
    return pogo_small_hspace if col % 2 == 1 else pogo_big_hspace
def get_pogo_total_hspace(row, col):
    if col == 0:
        return get_pogo_hspace(row, col)
    return get_pogo_hspace(row, col) + get_pogo_total_hspace(row, col - 1)

def createModulePartitionLine(substrate: Substrate, first_row: bool, last_row: bool,
                                first_col: bool, last_col: bool) -> None:
    # Generate partition lines around the module, with the size of the module's bounding box
    # Only use lines on sides that are not on the edge of the panel
    exterior = substrate.exterior()
    partition_lines: list[LineString] = []
    minx, miny, maxx, maxy = exterior.bounds
    if not first_col:
        partition_lines.append(LineString([(minx - hspace//2, miny), (minx - hspace//2, maxy)]))
    else:
        partition_lines.append(LineString([(minx - hspace, miny), (minx - hspace, maxy)]))
    if not last_col:
        partition_lines.append(LineString([(maxx + hspace//2, miny), (maxx + hspace//2, maxy)]))
    else:
        partition_lines.append(LineString([(maxx + hspace, miny), (maxx + hspace, maxy)]))
    if not first_row:
        partition_lines.append(LineString([(minx, miny - vspace//2), (maxx, miny - vspace//2)]))
    if not last_row:
        partition_lines.append(LineString([(minx, maxy + vspace//2), (maxx, maxy + vspace//2)]))
    substrate.partitionLine = GeometryCollection(partition_lines)

def createConnectorPartitionLine(substrate: Substrate, row: int, col: int) -> None:
    # Create partition lines to the left and right of the pogo connector
    hspace_left = get_pogo_hspace(row, col)
    hspace_right = get_pogo_hspace(row, col + 1)
    if col == pogo_columns - 1:
        hspace_right = hspace_right//2 - pogo_small_hspace//2 + hspace
    exterior = substrate.exterior()
    partition_lines: list[LineString] = []
    minx, miny, maxx, maxy = exterior.bounds
    leny = maxy - miny
    miny += leny // 4
    maxy -= leny // 4
    if col == 0:
        partition_lines.append(LineString([(minx - hspace_left - hspace, miny), (minx - hspace_left - hspace, maxy)]))
    else:
        partition_lines.append(LineString([(minx - hspace_left//2, miny), (minx - hspace_left//2, maxy)]))
    if col == pogo_columns - 1:
        partition_lines.append(LineString([(maxx + hspace_right, miny), (maxx + hspace_right, maxy)]))
    else:
        partition_lines.append(LineString([(maxx + hspace_right//2, miny), (maxx + hspace_right//2, maxy)]))
    substrate.partitionLine = GeometryCollection(partition_lines)

module_positions: list[tuple[int, int, int, int]] = []
for col in range(module_columns):
    for row in range(module_rows):
        x = panelOrigin.x + col * (module_w + hspace) + module_w // 2
        y = panelOrigin.y + row * (module_h + vspace) + module_h // 2
        module_positions.append((x, y, row, col))

for i, (x, y, row, col) in enumerate(module_positions):
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
        substrate = panel.substrates[-1]
        first_row = (row == 0)
        last_row = (row == module_rows - 1)
        first_col = (col == 0)
        last_col = (col == module_columns - 1)
        createModulePartitionLine(substrate, first_row, last_row, first_col, last_col)

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
pogo_positions: list[tuple[int, int, int, int, bool]] = []
for col in range(pogo_columns):
    for row in range(pogo_rows):
        total_hspace = get_pogo_total_hspace(row, col)
             
        x = pogo_origin_x + col * pogo_w + total_hspace + pogo_w // 2
        y = pogo_origin_y + row * (pogo_h + pogo_vspace) + pogo_h // 2
        flip = (col % 2 == 1) ^ (row % 2 == 0)
        pogo_positions.append((x, y, row, col, flip))

for (x, y, row, col, flip) in pogo_positions:
        panel.appendBoard(
            pogo_connector_path,
            VECTOR2I(x, y),
            origin=Origin.Center,
            sourceArea=source_area_pogo_connector,
            inheritDrc=False,
            rotationAngle=fromDegrees(180) if flip else fromDegrees(0),
            refRenamer=lambda board_id, ref: f"POGO_{board_id}_{ref}"
        )
        substrate = panel.substrates[-1]
        createConnectorPartitionLine(substrate, row, col)

# Collect set of newly added boards
substrates = panel.substrates[substrateCount:]

# Prepare frame and partition
framingSubstrates = ki.dummyFramingSubstrate(substrates, preset)
# panel.buildPartitionLineFromBB(framingSubstrates)
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

# Product description:
"""
HS Code 85437090 Prototype modular LED cube PCB assembly with connectors, LEDs, and microcontroller; for testing and development only, not a finished product.
"""