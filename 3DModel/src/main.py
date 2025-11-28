import ocp_vscode
import cadquery as cq
from loader import get_kicad_pcbs_as_shapes_dicts, shapes_dict_to_cq_object
from debug import debug_show, debug_show_no_exit
from pcb import make_offset_shape
import os

# fmt: off
# ----------- Constants
KICAD_PCB_NAMES = [
    "Module",
    "PowerSupply",
    "PogoConnector",
]

PCB_PART_NAME = "PCB"
FULL_PCB_NAME = "FullBoard"
PCB_THICKNESS = 1.6

WALL_THICKNESS = 1
"""Typical wall thickness for 3D printed parts."""
PCB_TOLERANCE = 0.1
"""Tolerance to apply in all directions around the PCB to ensure it fits into the box."""

POGO_PIN_OFFSET = 2.3
"""Distance from the center of the pogo connecter to the center of the pogo pins."""

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
MAGNET_DISTANCE = 4 * 0.42 # Has to be at least twice the outer wall width of slicer
"""Distance between two magnets when two boxes are connected."""
MAGNET_SPACING = 3
"""Spacing between the two magnets, measured from the edges of the magnets."""
MAGNET_POGO_CONNECTOR_DISTANCE = 0.5
"""Distance between the edge of the magnet and the edge of the pogo connector pcb."""
MAGNET_HOLDER_COVER_PERCENTAGE = 0.1
"""Percentage of the magnet diameter that should be covered by the magnet holder."""
MAGNET_HOLDER_COVER_THICKNESS = WALL_THICKNESS
"""Thickness of the magnet holder cover."""

BOX_FILLET = 5.0
BOX_BE_A_CUBE = False
"""When True, the box will be a perfect cube. When False, the size of the box in positive and negative z direction will depend on the components inside."""

MODULE_PILLAR_DIAMETER = 8.0
"""Diameter of the pillars that hold the module PCB inside the box."""

USB_C_CONNECTOR_WIDTH = 9
USB_C_CONNECTOR_HEIGHT = 3.3
USB_C_CONNECTOR_OFFSET_FROM_PCB = 0.1
"""Distance from the bottom of the USB-C connecter to the top of the PCB."""
USB_C_CONNECTOR_FILLET = 1.2
USB_C_CONNECTOR_DEPTH = 7.5
USB_C_CONNECTOR_OVERHANG = 1.3
"""How much the USB-C connector extends beyond the edge of the PCB"""

# ----------- Load PCBs
shapes_dicts = get_kicad_pcbs_as_shapes_dicts(
    kicad_pcb_names=KICAD_PCB_NAMES,
    pcb_part_name=PCB_PART_NAME,
    full_name=FULL_PCB_NAME,
)
module_shapes_dict = shapes_dicts["Module"]
power_supply_shapes_dict = shapes_dicts["PowerSupply"]
pogo_connector_shapes_dict = shapes_dicts["PogoConnector"]

cq_pogo_connector = shapes_dict_to_cq_object(pogo_connector_shapes_dict)
cq_power_supply = shapes_dict_to_cq_object(power_supply_shapes_dict)
cq_module = shapes_dict_to_cq_object(module_shapes_dict)

cq_module_pcb = module_shapes_dict[PCB_PART_NAME]
module_pcb_bounds = cq_module_pcb.BoundingBox()
module_length = module_pcb_bounds.xlen
box_length = round(module_length + POGO_PIN_LENGTH_COMPRESSED, 2)
"""Length of a side of the box on the xy plane."""

cq_power_supply_pcb = power_supply_shapes_dict[PCB_PART_NAME]
power_supply_pcb_bounds = cq_power_supply_pcb.BoundingBox()
power_supply_length = power_supply_pcb_bounds.xlen

module_max_z = module_pcb_bounds.zmax
for shape in module_shapes_dict.values():
    bounds = shape.BoundingBox()
    if bounds.zmax > module_max_z:
        module_max_z = bounds.zmax
power_supply_max_z = power_supply_pcb_bounds.zmax
for shape in power_supply_shapes_dict.values():
    bounds = shape.BoundingBox()
    if bounds.zmax > power_supply_max_z:
        power_supply_max_z = bounds.zmax

# ----------- Pogo Connectors and Magnets
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

pogo_connector_translation = PCB_TOLERANCE
def transform_pogo_connector(cq_obj: cq.Workplane) -> cq.Workplane:
    """Position an object to align with the pogo connector placement on the module PCB."""
    return (
        cq_obj
        .rotate((0, 0, 0), (0, 1, 0), 90)
        .translate((
            0.5 * module_length - PCB_THICKNESS,
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
    .extrude(-MAGNET_THICKNESS)
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

pogo_pin_center_z = -POGO_PIN_OFFSET + pogo_connector_translation
"""Final global z position of the center of the pogo pins."""

# ----------- Box
box_wall_thickness = WALL_THICKNESS
box_height = 0.5 * box_length
"""Height of the box in positive z direction."""
box_depth = 0.5 * box_length
"""Depth of the box in negative z direction."""
if not BOX_BE_A_CUBE:
    module_max_z = max(module_max_z, power_supply_max_z)
    box_height = module_max_z + 2 * PCB_TOLERANCE + BOX_FILLET + box_wall_thickness
    box_depth = magnet_translation_x + 0.5 * MAGNET_DIAMETER + BOX_FILLET + box_wall_thickness
cq_box_original = (
    cq.Workplane().box(
        box_length,
        box_length,
        box_height + box_depth,
        centered=(True, True, False),
    )
    .translate((0, 0, -box_depth))
    .edges()
    .chamfer(BOX_FILLET)
)
cq_box = cq_box_original.shell(-box_wall_thickness)

############# Holders for the magnets
magnet_center_z = -magnet_translation_x + pogo_connector_translation
"""Final global z position of the center of the magnets inside the box."""
magnet_holder_height = box_depth + magnet_center_z + 0.5 * MAGNET_DIAMETER + MAGNET_POGO_CONNECTOR_DISTANCE
magnet_holder_width = box_length
magnet_holder_cover_height = box_depth + magnet_center_z - 0.5 * MAGNET_DIAMETER + MAGNET_HOLDER_COVER_PERCENTAGE * MAGNET_DIAMETER

cq_magnet_holder = (
    cq.Workplane()
    .box(
        magnet_holder_width,
        MAGNET_THICKNESS,
        magnet_holder_height,
        centered=(True, True, False),
    )
    .translate((
        0, 0.5 * box_length - 0.5 * MAGNET_DISTANCE -0.5 * MAGNET_THICKNESS, -box_depth
    ))
    .intersect(cq_box_original)
    .cut(cq_box)
)
cq_magnet_holder_cover = (
    cq.Workplane()
    .box(
        magnet_holder_width,
        MAGNET_HOLDER_COVER_THICKNESS,
        magnet_holder_cover_height,
        centered=(True, True, False),
    )
    .translate((
        0, 0.5 * box_length - 0.5 * MAGNET_DISTANCE - MAGNET_THICKNESS -0.5 * MAGNET_HOLDER_COVER_THICKNESS, -box_depth
    ))
    .intersect(cq_box_original)
    .cut(cq_box)
)

for angle in [0, 90, 180, 270]:
    cq_magnet_holder_rotated = cq_magnet_holder.rotate((0, 0, 0), (0, 0, 1), angle)
    cq_magnet_holder_cover_rotated = cq_magnet_holder_cover.rotate((0, 0, 0), (0, 0, 1), angle)
    cq_box = cq_box.union(cq_magnet_holder_rotated)
    # cq_box = cq_box.union(cq_magnet_holder_cover_rotated) # TODO: cover currently not needed

############# Holders for the pogo connectors
pogo_connector_holder_height = box_length - magnet_holder_height
pogo_connector_holder_width = box_length
pogo_connector_holder_thickness = PCB_THICKNESS

cq_pogo_connector_holder = (
    cq.Workplane()
    .box(
        pogo_connector_holder_width,
        pogo_connector_holder_thickness,
        pogo_connector_holder_height,
        centered=(True, False, False),
    )
    .translate((
        0, 0.5 * box_length - box_wall_thickness - pogo_connector_holder_thickness, -box_depth + magnet_holder_height
    ))
    .intersect(cq_box_original)
    .cut(cq_box)
)

for angle in [0, 90, 180, 270]:
    cq_pogo_connector_holder_rotated = cq_pogo_connector_holder.rotate((0, 0, 0), (0, 0, 1), angle)
    cq_box = cq_box.union(cq_pogo_connector_holder_rotated)

############# USB-C Connector Cutout
cq_usb_c_connector = (
    cq.Workplane()
    .box(
        USB_C_CONNECTOR_DEPTH,
        USB_C_CONNECTOR_WIDTH,
        USB_C_CONNECTOR_HEIGHT,
        centered=(False, True, False),
    )
    .edges("X")
    .fillet(USB_C_CONNECTOR_FILLET)
    .translate((
        -0.5 * module_length - USB_C_CONNECTOR_OVERHANG, 0, PCB_THICKNESS + USB_C_CONNECTOR_OFFSET_FROM_PCB
    ))
)

def finish_box(cq_box: cq.Workplane, is_power_supply: bool) -> tuple[cq.Workplane, cq.Workplane]:
    """Finish editing the box, extracted to a function to work for the power supply box as well."""

    ############# Cut Holes
    if is_power_supply:
        # Only cut holes on the right side
        cq_box = cq_box.cut(cq_pogo_pin_holes[1])
        cq_box = cq_box.cut(cq_pogo_connector_holes[1])
        cq_box = cq_box.cut(cq_magnet_holes[1])
        # Cut the USB-C connector hole
        cq_box = cq_box.cut(cq_usb_c_connector)
    else:
        for cq_pogo_pin_hole in cq_pogo_pin_holes:
            cq_box = cq_box.cut(cq_pogo_pin_hole)
        for cq_pogo_connector_hole in cq_pogo_connector_holes:
            cq_box = cq_box.cut(cq_pogo_connector_hole)
        for cq_magnet_hole in cq_magnet_holes:
            cq_box = cq_box.cut(cq_magnet_hole)

    ############# Split the box into two halves that can be clipped together
    cq_split_body_bottom = (
        cq.Workplane()
        .box(box_length, box_length, box_depth + pogo_pin_center_z, centered=(True, True, False))
        .translate((0, 0, -box_depth))
        .edges("|Z")
        .chamfer(BOX_FILLET)
    )
    cq_split_body_bottom = (
        cq.Workplane()
        .add(
            cq_split_body_bottom
            .faces(">Z")
        )
        .add(
            cq_split_body_bottom
            .translate((0, 0, box_wall_thickness))
            .faces(">Z")
            .wires()
            .toPending()
            .offset2D(-box_wall_thickness, kind="intersection")
        )
        .wires()
        .toPending()
        .loft()
        .add(cq_split_body_bottom)
    )
    cq_split_body_top = (
        cq_box_original
        .cut(cq_split_body_bottom)
    )

    cq_box_top = (
        cq_box
        .cut(cq_split_body_bottom)
    )
    cq_box_bottom = (
        cq_box
        .cut(cq_split_body_top)
    )
    # TODO: add clipping mechanism

    ############# Module PCB Slot
    cq_module_pcb_with_tolerance = make_offset_shape(cq.Workplane(cq_module_pcb), cq.Vector(PCB_TOLERANCE, PCB_TOLERANCE, PCB_TOLERANCE))
    cq_box_top = (
        cq_box_top
        .cut(
            cq_module_pcb_with_tolerance
            .faces(">Z")
            .wires().toPending()
            .extrude(-(box_height + box_depth))
        )
    )

    ############# Module Pillars
    module_pillar_height = box_depth - box_wall_thickness - PCB_TOLERANCE + PCB_THICKNESS
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
        .rect(0.5 * MODULE_PILLAR_DIAMETER, 0.5 * MODULE_PILLAR_DIAMETER)
        .extrude(-module_pillar_height)
        .translate((
            0,
            0,
            -PCB_TOLERANCE + PCB_THICKNESS,
        ))
        .intersect(cq_box_original)
        .cut(cq_box)
        .cut(cq_module_pcb_with_tolerance)
    )
    cq_box_bottom = cq_box_bottom.union(cq_module_pillar)
    return cq_box_top, cq_box_bottom

cq_box_top, cq_box_bottom = finish_box(cq_box, is_power_supply=False)
cq_power_supply_box_top, cq_power_supply_box_bottom = finish_box(cq_box, is_power_supply=True)

# ----------- Show Result
full_cube: dict[str, cq.Workplane] = {
    "Module": cq_module,
    "Pogo Connector Top": cq_pogo_connectors[0],
    "Pogo Connector Right": cq_pogo_connectors[1],
    "Pogo Connector Bottom": cq_pogo_connectors[2],
    "Pogo Connector Left": cq_pogo_connectors[3],
    "Box Top": cq_box_top,
    "Box Bottom": cq_box_bottom,
    **{f"Magnet {i+1}": cq_magnets[i] for i in range(len(cq_magnets))},
    "USB-C Connector": cq_usb_c_connector,
}
full_power_supply_cube: dict[str, cq.Workplane] = {
    "Power Supply": cq_power_supply,
    "Power Supply Pogo Connector Right": cq_pogo_connectors[1],
    "Power Supply Box Top": cq_power_supply_box_top,
    "Power Supply Box Bottom": cq_power_supply_box_bottom,
    "Power Supply Magnet 1": cq_magnets[1],
}
for name, cq_object in full_power_supply_cube.items():
    full_power_supply_cube[name] = cq_object.translate((
        -box_length, 0, 0
    ))
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

cq_power_supply_cube = cq.Workplane()
for value in full_power_supply_cube.values():
    cq_power_supply_cube = cq_power_supply_cube.add(value)
cq_power_supply_cube = cq_power_supply_cube.translate((-box_length, 0, 0))

ocp_vscode.show(
    *[*full_cube.values(), *full_power_supply_cube.values(), cq_full_cube_2],
    names=list(full_cube.keys()) + list(full_power_supply_cube.keys()) + ["Full Cube 2"],
)

# ----------- Save Result
output_folder = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "output"))
cq.Assembly(cq_pogo_connectors[0]).export(os.path.join(output_folder, "Pogo_Connector.stl"))
cq.Assembly(cq_module).export(os.path.join(output_folder, "Module.stl"))
cq.Assembly(cq_power_supply).export(os.path.join(output_folder, "Power_Supply.stl"))
cq.Assembly(cq_box_top).export(os.path.join(output_folder, "Box_Top.stl"))
cq.Assembly(cq_box_bottom).export(os.path.join(output_folder, "Box_Bottom.stl"))
cq.Assembly(cq_power_supply_box_top).export(os.path.join(output_folder, "Power_Supply_Box_Top.stl"))
cq.Assembly(cq_power_supply_box_bottom).export(os.path.join(output_folder, "Power_Supply_Box_Bottom.stl"))

# TODO: Design the box so the magnet holder cover is part of the top box and magnets are not inserted as tight fit but a bit more loose and then held in place by the magnet holder cover
