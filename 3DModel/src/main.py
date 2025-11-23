import ocp_vscode
import cadquery as cq
from loader import get_kicad_pcbs_as_shapes_dicts, shapes_dict_to_cq_object
from debug import debug_show, debug_show_no_exit

# fmt: off
# ----------- Constants
KICAD_PCB_NAMES = [
    "Module",
    "PogoConnector",
    "ShieldHallSensor",
]

PCB_PART_NAME = "PCB"
FULL_PCB_NAME = "FullBoard"
PCB_THICKNESS = 1.6

ANGLED_PIN_HEADER_HEIGHT = 2.5
"""Height of the plastic part of the angled pin header on the module in z direction"""
POGO_CONNECTOR_HOLES_OFFSET = 1.5
"""Distance from the center of the pogo connecter to its holes where the angled pin header is inserted."""
POGO_PIN_OFFSET = 1.5
"""Distance from the center of the pogo connecter to the center of the pogo pins."""
SHIELD_OFFSET_Z = 7.3
"""Offset in Z direction for the shield hall sensor PCB."""

POGO_PIN_DIAMETER = 2.0
POGO_PIN_LENGTH = 3.0
"""Length of the pogo pin, when not compressed."""
POGO_PIN_MAX_COMPRESSION = 1.0
"""Maximum compression length for pogo pins, meaning how far the pin can be pushed in."""
POGO_PIN_TARGET_COMPRESSION_PERCENTAGE = 0.6
"""Target compression percentage for pogo pins. 60% compression is recommended."""
POGO_PIN_LENGTH_COMPRESSED = (
    POGO_PIN_LENGTH - POGO_PIN_MAX_COMPRESSION
) + POGO_PIN_TARGET_COMPRESSION_PERCENTAGE * POGO_PIN_MAX_COMPRESSION
"""Length of the pogo pin when compressed to the target percentage."""
POGO_PIN_SPACING = 3
"""Spacing between pogo pins."""
NUMBER_OF_POGO_PINS = 6

MAGNET_DIAMETER = 10.0
MAGNET_THICKNESS = 2.7
MAGNET_DISTANCE = 0.4
"""Distance between two magnets when two boxes are connected."""
MAGNET_SPACING = 3
"""Spacing between the two magnets, measured from the edges of the magnets."""
MAGNET_POGO_CONNECTOR_DISTANCE = 0.5
"""Distance between the edge of the magnet and the edge of the pogo connector pcb."""

BOX_FILLET = 5.0

WALL_THICKNESS = 1
"""Typical wall thickness for 3D printed parts."""
PCB_TOLERANCE = 0.1
"""Tolerance to apply in all directions around the PCB to ensure it fits into the box."""

MODULE_PILLAR_DIAMETER = 6.0
"""Diameter of the pillars that hold the module PCB inside the box."""

# ----------- Load PCBs
shapes_dicts = get_kicad_pcbs_as_shapes_dicts(
    kicad_pcb_names=KICAD_PCB_NAMES,
    pcb_part_name=PCB_PART_NAME,
    full_name=FULL_PCB_NAME,
)
module_shapes_dict = shapes_dicts["Module"]
pogo_connector_shapes_dict = shapes_dicts["PogoConnector"]
shield_hall_sensor_shapes_dict = shapes_dicts["ShieldHallSensor"]

cq_pogo_connector = shapes_dict_to_cq_object(pogo_connector_shapes_dict)
cq_module = shapes_dict_to_cq_object(module_shapes_dict)
cq_shield_hall_sensor = shapes_dict_to_cq_object(shield_hall_sensor_shapes_dict)

# ----------- Pogo Connectors and Magnets
############# Get Pogo Connector Positions
shield_bounds = shield_hall_sensor_shapes_dict[FULL_PCB_NAME].BoundingBox()
shield_height = shield_bounds.zmax

cq_module_pcb = module_shapes_dict[PCB_PART_NAME]
module_bounds = cq_module_pcb.BoundingBox()
module_length = module_bounds.xlen
box_length = round(module_length + POGO_PIN_LENGTH_COMPRESSED + 2 * PCB_THICKNESS, 2)
"""Length of a side of the box on the xy plane."""

############# Position Pogo Connectors
cq_pogo_connectors: list[cq.Workplane] = []
cq_pogo_pin_holes: list[cq.Workplane] = []
"""List of holes to later cut out of the box for the pogo pins."""
cq_pogo_connector_holes: list[cq.Workplane] = []
"""List of holes to later cut out of the box for the pogo connectors."""
cq_magnet_holes: list[cq.Workplane] = []
"""List of holes to later cut out of the box for the magnets."""
cq_magnets: list[cq.Workplane] = []
"""List of magnets to be placed inside the box."""

pogo_connector_translation = PCB_THICKNESS + 0.5 * ANGLED_PIN_HEADER_HEIGHT - POGO_CONNECTOR_HOLES_OFFSET
def transform_pogo_connector(cq_obj: cq.Workplane) -> cq.Workplane:
    """Position an object to align with the pogo connector placement on the module PCB."""
    return (
        cq_obj
        .rotate((0, 0, 0), (0, 1, 0), 90)
        .translate((
            0.5 * module_length,
            0,
            pogo_connector_translation,
        ))
    )
cq_pogo_connector_pcb = pogo_connector_shapes_dict[PCB_PART_NAME]
pogo_pin_positions = [
    (0, (i-NUMBER_OF_POGO_PINS / 2 + 0.5) * POGO_PIN_SPACING) for i in range(NUMBER_OF_POGO_PINS)
]
cq_pogo_pin_hole = (
    cq.Workplane()
    .pushPoints(pogo_pin_positions)
    .circle(0.5 * POGO_PIN_DIAMETER)
    .extrude(POGO_PIN_LENGTH)
    .translate((POGO_PIN_OFFSET, 0, PCB_THICKNESS))
)
pogo_connector_bounds = cq_pogo_connector_pcb.BoundingBox()
cq_pogo_pin_pcb_with_tolerance = (
    cq.Workplane()
    .box(
        pogo_connector_bounds.xlen + 2 * PCB_TOLERANCE,
        pogo_connector_bounds.ylen + 2 * PCB_TOLERANCE,
        pogo_connector_bounds.zlen + 2 * PCB_TOLERANCE,
    )
    .translate((0, 0, 0.5 * PCB_THICKNESS))
)

magnet_translation_x = 0.5 * pogo_connector_bounds.xlen + MAGNET_POGO_CONNECTOR_DISTANCE + 0.5 * MAGNET_DIAMETER + PCB_TOLERANCE
magnet_positions = [
    (magnet_translation_x, 0.5 * (MAGNET_SPACING + MAGNET_DIAMETER)),
    (magnet_translation_x, -0.5 * (MAGNET_SPACING + MAGNET_DIAMETER)),
]
cq_magnet = (
    cq.Workplane()
    .pushPoints(magnet_positions)
    .circle(0.5 * MAGNET_DIAMETER)
    .extrude(-MAGNET_THICKNESS)
    .rotate((0, 0, 0), (0, 1, 0), 90)
    .translate((
        0.5 * box_length - 0.5 * MAGNET_DISTANCE,
        0,
        pogo_connector_translation,
    ))
)
cq_magnet_hole = (
    cq.Workplane()
    .pushPoints(magnet_positions)
    .circle(0.5 * MAGNET_DIAMETER)
    .extrude(-3 * MAGNET_THICKNESS)
    .rotate((0, 0, 0), (0, 1, 0), 90)
    .translate((
        0.5 * box_length - 0.5 * MAGNET_DISTANCE,
        0,
        pogo_connector_translation,
    ))
)

cq_pogo_connector_transformed = transform_pogo_connector(cq_pogo_connector)
cq_pogo_pin_hole_transformed = transform_pogo_connector(cq_pogo_pin_hole)
cq_pogo_pin_pcb_with_tolerance_transformed = transform_pogo_connector(cq_pogo_pin_pcb_with_tolerance)
for angle in [90, 0, 270, 180]:  # Top, Right, Bottom, Left
    cq_pogo_connectors.append(
        cq_pogo_connector_transformed
        .rotate((0, 0, 0), (0, 0, 1), angle)
    )
    cq_pogo_pin_holes.append(
        cq_pogo_pin_hole_transformed
        .rotate((0, 0, 0), (0, 0, 1), angle)
    )
    cq_pogo_connector_holes.append(
        cq_pogo_pin_pcb_with_tolerance_transformed
        .rotate((0, 0, 0), (0, 0, 1), angle)
    )
    cq_magnets.append(
        cq_magnet
        .rotate((0, 0, 0), (0, 0, 1), angle)
    )
    cq_magnet_holes.append(
        cq_magnet_hole
        .rotate((0, 0, 0), (0, 0, 1), angle)
    )
# ----------- Shield Hall Sensor Position
cq_shield_hall_sensor = cq_shield_hall_sensor.translate((0, 0, SHIELD_OFFSET_Z))

# ----------- Box
box_wall_thickness = WALL_THICKNESS
box_height = SHIELD_OFFSET_Z + shield_height + PCB_TOLERANCE + box_wall_thickness # 0.5 * box_length
"""Height of the box in positive z direction."""
box_depth = magnet_translation_x - pogo_connector_translation + 0.5 * MAGNET_DIAMETER + BOX_FILLET + box_wall_thickness # 0.5 * box_length
"""Depth of the box in negative z direction."""
cq_box_original = (
    cq.Workplane().box(
        box_length,
        box_length,
        box_height + box_depth,
        centered=(True, True, False),
    )
    .translate((0, 0, -box_depth))
    .edges()
    .fillet(BOX_FILLET)
)
cq_box = cq_box_original.shell(-box_wall_thickness)
for cq_pogo_pin_hole in cq_pogo_pin_holes:
    cq_box = cq_box.cut(cq_pogo_pin_hole)
for cq_pogo_connector_hole in cq_pogo_connector_holes:
    cq_box = cq_box.cut(cq_pogo_connector_hole)
for cq_magnet_hole in cq_magnet_holes:
    cq_box = cq_box.cut(cq_magnet_hole)


############# Holders for the magnets and pogo connector PCBs


############# Module Pillars
module_pillar_height = box_depth - box_wall_thickness - PCB_TOLERANCE
module_pillar_translation = 0.5 * box_length - box_wall_thickness - 0.5 * MODULE_PILLAR_DIAMETER
module_pillar_positions = [
    (module_pillar_translation, module_pillar_translation),
    (module_pillar_translation, -module_pillar_translation),
    (-module_pillar_translation, module_pillar_translation),
    (-module_pillar_translation, -module_pillar_translation),
]
cq_module_pillar = (
    cq.Workplane()
    .pushPoints(module_pillar_positions)
    .circle(0.5 * MODULE_PILLAR_DIAMETER)
    .extrude(-module_pillar_height)
    .translate((
        0,
        0,
        -PCB_TOLERANCE,
    ))
    .intersect(cq_box_original) # Hacky, but gets the job done
)
cq_box = cq_box.union(cq_module_pillar)

# ----------- Show Result
full_cube: dict[str, cq.Workplane] = {
    "Module": cq_module,
    "Pogo Connector Top": cq_pogo_connectors[0],
    "Pogo Connector Right": cq_pogo_connectors[1],
    "Pogo Connector Bottom": cq_pogo_connectors[2],
    "Pogo Connector Left": cq_pogo_connectors[3],
    "Shield Hall Sensor": cq_shield_hall_sensor,
    "Box": cq_box,
    **{f"Magnet {i+1}": cq_magnets[i] for i in range(len(cq_magnets))},
}
cq_full_cube = cq.Workplane()
for value in full_cube.values():
    cq_full_cube = cq_full_cube.add(value)

full_cube_2 = {
    f"{name}_2]": cq_object.translate((
        0, box_length, 0
    ))
    for name, cq_object in full_cube.items()
}
cq_full_cube_2 = cq_full_cube.translate((0, box_length, 0))

ocp_vscode.show(
    *[*full_cube.values(), cq_full_cube_2],
    names=list(full_cube.keys()) + ["Full Cube 2"],
)
