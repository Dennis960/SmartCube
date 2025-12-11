import ocp_vscode
import cadquery as cq
from loader import get_kicad_pcbs_as_shapes_dicts, shapes_dict_to_cq_object
from debug import debug_show, debug_show_no_exit
from pcb import make_offset_shape
import os


def build_octahedron(
    size: float, cq_object: cq.Workplane = cq.Workplane("XY")
) -> cq.Workplane:
    v = [
        (size, 0, 0),
        (-size, 0, 0),
        (0, size, 0),
        (0, -size, 0),
        (0, 0, size),
        (0, 0, -size),
    ]
    faces = [
        [0, 2, 4],
        [2, 1, 4],
        [1, 3, 4],
        [3, 0, 4],
        [0, 5, 2],
        [2, 5, 1],
        [1, 5, 3],
        [3, 5, 0],
    ]
    cq_faces: list[cq.Face] = []
    for f in faces:
        polygon = cq.Wire.makePolygon([v[f[0]], v[f[1]], v[f[2]], v[f[0]]])
        tri = cq.Face.makeFromWires(polygon)
        cq_faces.append(tri)
    cq_shell = cq.Shell.makeShell(cq_faces)
    cq_solid = cq.Solid.makeSolid(cq_shell)
    return cq_object.add(cq_solid)


# fmt: off
# ----------- Constants
KICAD_PCB_NAMES = [
    "Module",
    "PowerSupply",
    "PogoConnector",
]

PRINTER_MIN_OUTER_WALL_WIDTH = 0.42

PCB_PART_NAME = "PCB"
FULL_PCB_NAME = "FullBoard"
PCB_THICKNESS = 1.6

WALL_THICKNESS = 1.5
"""Typical wall thickness for 3D printed parts."""
PCB_TOLERANCE = 0.1
"""Tolerance to apply in all directions around the PCB to ensure it fits into the box."""
TOLERANCE = 0.15
"""Tolerance to apply in all directions when combining two 3d printed parts."""
MOUSE_BITES_TOLERANCE = 0.5
"""Tolerance to apply at the pogo connector PCB mouse bites to ensure it fits well."""
MOUSE_BITES_WIDTH = 6
"""Width of the mouse bites on the pogo connector PCB."""

BOX_HOURGLASS_OFFSET = 0.4
"""
The box will be created as an hourglass shape.
This is to prevent part of the box bending outwards after assembly which causes loose contacts.
The offset is applied on every side.
"""

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
    POGO_PIN_LENGTH - (POGO_PIN_TARGET_COMPRESSION_PERCENTAGE * POGO_PIN_MAX_COMPRESSION)
)
"""Length of the pogo pin when compressed to the target percentage."""
POGO_PIN_SPACING = 3
"""Spacing between pogo pins."""
NUMBER_OF_POGO_PINS = 6

MAGNET_DIAMETER = 10.0 - 0.1  # Slightly smaller for a tighter fit
MAGNET_THICKNESS = 2.55
MAGNET_DISTANCE = 4 * PRINTER_MIN_OUTER_WALL_WIDTH  # Has to be at least twice the outer wall width of slicer
"""Distance between two magnets when two boxes are connected."""
MAGNET_POGO_CONNECTOR_DISTANCE = 0.5
"""Distance between the edge of the magnet and the edge of the pogo connector pcb."""
MAGNET_EXTRA_SPACING_VERTICAL = 4
"""Extra spacing between magnets in vertical direction."""
MAGNET_EXTRA_SPACING_HORIZONTAL = 1.5
"""Extra spacing from magnets to walls in horizontal direction."""

BOX_WALL_THICKNESS = WALL_THICKNESS

MODULE_PILLAR_DIAMETER = 3.5
"""Diameter of the pillars that hold the module PCB inside the box."""

CLIP_CONNECTOR_THICKNESS = 1.2
"""Thickness of the clipping connectors on the module pillars."""
CLIP_CONNECTOR_OFFSET_Z = 0.1
"""Offset in z direction of the clipping connectors for a better fit."""
CLIP_CONNECTOR_OFFSET = 1.4
"""Offset in xy direction (tune this value until it fits well)"""
CLIP_CONNECTOR_TOLERANCE = 0.1
"""Tolerance to apply to the clipping connectors for a better fit."""

USB_C_CONNECTOR_WIDTH = 9
USB_C_CONNECTOR_HEIGHT = 3.3
USB_C_CONNECTOR_OFFSET_FROM_PCB = 0.1
"""Distance from the bottom of the USB-C connecter to the top of the PCB."""
USB_C_CONNECTOR_FILLET = 1.2
USB_C_CONNECTOR_DEPTH = 7.5
USB_C_CONNECTOR_OVERHANG = 1.3
"""How much the USB-C connector extends beyond the edge of the PCB"""

ESP_MARGIN = 0.4
"""Margin to add around the ESP32 for the cutout."""

BOX_CHAMFER = 1
"""Size of the chamfer on the box edges."""

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

# ----------- Pogo Connectors, Magnets and their Holes and Holders
cq_pogo_connectors: list[cq.Workplane] = []
cq_pogo_pin_holes: list[cq.Workplane] = []
"""List of holes to later cut out of the box for the pogo pins."""
cq_pogo_connector_holes: list[cq.Workplane] = []
"""List of holes to later cut out of the box for the pogo connectors."""
cq_magnet_holes: list[cq.Workplane] = []
"""List of holes to later cut out of the box for the magnets."""
cq_magnets: list[cq.Workplane] = []
"""List of magnets to be placed inside the box."""
cq_magnet_holders: list[cq.Workplane] = []
"""List of magnet holders to be placed inside the box."""
cq_pogo_connector_holders: list[cq.Workplane] = []
"""List of pogo connector holders to be placed inside the box."""

############# Pogo Connectors
def transform_pogo_connector(cq_obj: cq.Workplane) -> cq.Workplane:
    """Position an object to align with the pogo connector placement on the module PCB."""
    return (
        cq_obj
        .rotate((0, 0, 0), (0, 1, 0), 90)
        .translate((
            0.5 * module_length - PCB_THICKNESS,
            0,
            0,
        ))
    )
cq_pogo_connector_pcb = pogo_connector_shapes_dict[PCB_PART_NAME]
pogo_pin_positions = [
    (0, (i-NUMBER_OF_POGO_PINS / 2 + 0.5) * POGO_PIN_SPACING) for i in range(NUMBER_OF_POGO_PINS)
]
cq_pogo_pin_hole = (
    cq.Workplane()
    .pushPoints(pogo_pin_positions)
    .eachpoint(
        cq.Workplane()
        .circle(0.5 * POGO_PIN_DIAMETER)
        .extrude(0.5 * POGO_PIN_LENGTH_COMPRESSED)
    )
    .translate((POGO_PIN_OFFSET, 0, PCB_THICKNESS))
)
pogo_connector_bounds = cq_pogo_connector_pcb.BoundingBox()
cq_pogo_pin_pcb_with_tolerance = (
    make_offset_shape(
        cq.Workplane(cq_pogo_connector_pcb),
        cq.Vector(PCB_TOLERANCE, PCB_TOLERANCE, PCB_TOLERANCE)
    )
    .copyWorkplane(cq.Workplane(origin=(0, 0, -PCB_TOLERANCE)))
    .box(pogo_connector_bounds.xlen + 2 * (MOUSE_BITES_TOLERANCE), MOUSE_BITES_WIDTH, PCB_THICKNESS + 2 * PCB_TOLERANCE, centered=(True, True, False))
    .copyWorkplane(cq.Workplane(origin=(0, 0, -PCB_TOLERANCE)))
    .pushPoints([(-0.15, 5.15), (-0.15, -5.15)]) # Hack to remove the holes
    .rect(0.9, 0.9)
    .extrude(PCB_THICKNESS + 2 * PCB_TOLERANCE)
)

############# Magnets
magnet_translation_z_down = 0.5 * pogo_connector_bounds.xlen + MAGNET_POGO_CONNECTOR_DISTANCE + 0.5 * MAGNET_DIAMETER + PCB_TOLERANCE
magnet_translation_z_up = PCB_THICKNESS + MAGNET_POGO_CONNECTOR_DISTANCE + 0.5 * MAGNET_DIAMETER + PCB_TOLERANCE
magnet_translation_z_down += 0.5 * MAGNET_EXTRA_SPACING_VERTICAL
magnet_translation_z_up += 0.5 * MAGNET_EXTRA_SPACING_VERTICAL

magnets_max_z = magnet_translation_z_up + 0.5 * MAGNET_DIAMETER
"""Final global max z position of the top of the top magnets inside the box."""
magnets_min_z = -magnet_translation_z_down - 0.5 * MAGNET_DIAMETER
"""Final global min z position of the bottom of the bottom magnets inside the box."""
magnet_translation_y = 0.5 * (box_length - MAGNET_DIAMETER) - BOX_WALL_THICKNESS - MAGNET_THICKNESS - MAGNET_EXTRA_SPACING_HORIZONTAL
magnet_positions = [
    (magnet_translation_y, magnet_translation_z_up),
    (-magnet_translation_y, magnet_translation_z_up),
    (magnet_translation_y, -magnet_translation_z_down),
    (-magnet_translation_y, -magnet_translation_z_down),
]
cq_magnet = (
    cq.Workplane("YZ")
    .pushPoints(magnet_positions)
    .circle(0.5 * MAGNET_DIAMETER)
    .extrude(-MAGNET_THICKNESS)
    .translate((
        0.5 * box_length - 0.5 * MAGNET_DISTANCE,
        0,
        0,
    ))
)
cq_magnet_hole = cq_magnet

module_max_z = max(module_max_z, power_supply_max_z, magnets_max_z)
box_height = module_max_z + BOX_WALL_THICKNESS
"""Height of the box in positive z direction."""
box_depth = -(magnets_min_z - BOX_WALL_THICKNESS)

############# Holders for the magnets
magnet_holder_length = 2 * BOX_WALL_THICKNESS + MAGNET_DIAMETER + MAGNET_THICKNESS + MAGNET_EXTRA_SPACING_HORIZONTAL
magnet_holder_height = MAGNET_DIAMETER + 3 * WALL_THICKNESS
magnet_holder_thickness = 0.5 * POGO_PIN_LENGTH_COMPRESSED + PCB_THICKNESS
cq_magnet_holder = (
    cq.Workplane("YZ")
    .box(
        magnet_holder_length,
        magnet_holder_height,
        magnet_holder_thickness,
        centered=(True, True, False),
    )
    .edges("|X or >X")
    .chamfer(BOX_CHAMFER)
)
magnet_holder_translation_x = 0.5 * box_length - magnet_holder_thickness
magnet_holder_translation_y = 0.5 * box_length - 0.5 * magnet_holder_length
magnet_holder_translation_z_up = magnet_translation_z_up + 0.5 * WALL_THICKNESS
magnet_holder_translation_z_down = magnet_translation_z_down - 0.5 * WALL_THICKNESS
cq_magnet_holder_tr = (
    cq_magnet_holder
    .translate((
        magnet_holder_translation_x, magnet_holder_translation_y, magnet_holder_translation_z_up
    ))
)
cq_magnet_holder_br = (
    cq_magnet_holder
    .translate((
        magnet_holder_translation_x, magnet_holder_translation_y, -magnet_holder_translation_z_down
    ))
)
cq_magnet_holder_tl = (
    cq_magnet_holder
    .translate((
        magnet_holder_translation_x, -magnet_holder_translation_y, magnet_holder_translation_z_up
    ))
)
cq_magnet_holder_bl = (
    cq_magnet_holder
    .translate((
        magnet_holder_translation_x, -magnet_holder_translation_y, -magnet_holder_translation_z_down
    ))
)

############# Holders for the pogo connectors
pogo_connector_holder_height = box_depth
pogo_connector_holder_width = pogo_connector_bounds.ylen + 2 * WALL_THICKNESS
pogo_connector_holder_thickness = PCB_THICKNESS

cq_pogo_connector_holder = (
    cq.Workplane()
    .box(
        pogo_connector_holder_thickness,
        pogo_connector_holder_width,
        pogo_connector_holder_height,
        centered=(False, True, False),
    )
    .translate((
        0.5 * module_length - PCB_THICKNESS, 0, -POGO_PIN_OFFSET - pogo_connector_holder_height
    ))
)

############ Assemble all pogo connectors, magnets, holes and holders in all four orientations
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
    for cq_magnet_holder in [cq_magnet_holder_tr, cq_magnet_holder_br, cq_magnet_holder_tl, cq_magnet_holder_bl]:
        cq_magnet_holders.append(
            cq_magnet_holder
            .rotate((0, 0, 0), (0, 0, 1), angle)
        )
    cq_pogo_connector_holders.append(
        cq_pogo_connector_holder
        .rotate((0, 0, 0), (0, 0, 1), angle)
    )

# ----------- Box
"""Depth of the box in negative z direction."""
cq_box_original = (
    cq.Workplane().box(
        box_length - 2 * BOX_HOURGLASS_OFFSET,
        box_length - 2 * BOX_HOURGLASS_OFFSET,
        box_height + box_depth,
        centered=(True, True, False),
    )
    .translate((0, 0, -box_depth))
    .edges()
    .chamfer(BOX_CHAMFER)
)
cq_box_full = (
    cq.Workplane().box(
        box_length,
        box_length,
        box_height + box_depth,
        centered=(True, True, False),
    )
    .translate((0, 0, -box_depth))
    .edges()
    .chamfer(BOX_CHAMFER)
)
cq_box = cq_box_original.shell(-BOX_WALL_THICKNESS)
cq_box_with_tolerance = cq_box_original.shell(-(BOX_WALL_THICKNESS + TOLERANCE))

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

############# ESP-32 Connector Cutout
esp32_bounds = None
for name, shape in power_supply_shapes_dict.items():
    if "ESP32" in name:
        esp32_bounds = shape.BoundingBox()
        break
assert esp32_bounds is not None, "ESP32 shape not found in power supply PCB shapes."
cq_esp32 = (
    cq.Workplane()
    .box(
        esp32_bounds.xlen + 2 * ESP_MARGIN,
        esp32_bounds.ylen + 2 * ESP_MARGIN,
        esp32_bounds.zlen + 2 * ESP_MARGIN,
    )
    .translate(esp32_bounds.center)
)

def finish_box(cq_box: cq.Workplane, is_power_supply: bool) -> tuple[cq.Workplane, cq.Workplane]:
    """Finish editing the box, extracted to a function to work for the power supply box as well."""

    ############# Cut Holes and add Holders for Magnets and Pogo Connectors
    if is_power_supply:
        for cq_magnet_holder in cq_magnet_holders[4:8]:
            cq_box = cq_box.union(cq_magnet_holder.intersect(cq_box_full).cut(cq_box))
        cq_box = cq_box.union(cq_pogo_connector_holders[1].intersect(cq_box_original).cut(cq_box))
        # Only cut holes on the right side
        cq_box = cq_box.cut(cq_pogo_pin_holes[1])
        cq_box = cq_box.cut(cq_pogo_connector_holes[1])
        cq_box = cq_box.cut(cq_magnet_holes[1])
        # Cut the USB-C connector hole
        cq_box = cq_box.cut(cq_usb_c_connector)
    else:
        for cq_magnet_holder in cq_magnet_holders:
            cq_box = cq_box.union(cq_magnet_holder.intersect(cq_box_full).cut(cq_box))
        for cq_pogo_connector_holder in cq_pogo_connector_holders:
            cq_box = cq_box.union(cq_pogo_connector_holder.intersect(cq_box_original).cut(cq_box))
        for cq_pogo_pin_hole in cq_pogo_pin_holes:
            cq_box = cq_box.cut(cq_pogo_pin_hole)
        for cq_pogo_connector_hole in cq_pogo_connector_holes:
            cq_box = cq_box.cut(cq_pogo_connector_hole)
        for cq_magnet_hole in cq_magnet_holes:
            cq_box = cq_box.cut(cq_magnet_hole)

    ############# Split the box into two halves that can be clipped together
    def get_cq_split_body_bottom(tolerance: float = 0) -> cq.Workplane:
        cq_split_body_bottom = (
            cq.Workplane()
            .box(box_length, box_length, box_depth - POGO_PIN_OFFSET - 0.25 * POGO_PIN_DIAMETER, centered=(True, True, False))
            .translate((0, 0, -box_depth))
            .edges("|Z or <Z")
            .chamfer(BOX_CHAMFER)
        )
        cq_split_body_bottom = (
            cq.Workplane()
            .add(
                cq_split_body_bottom
                .faces(">Z")
                .wires()
                .toPending()
                .offset2D(-PRINTER_MIN_OUTER_WALL_WIDTH - BOX_HOURGLASS_OFFSET - tolerance, kind="intersection")
                # Small hack to get the wires (idk why offset2D alone does not work here)
                .extrude(1)
                .faces(">Z")
                .translate((0, 0, -1))
            )
            .add(
                cq_split_body_bottom
                .translate((0, 0, BOX_WALL_THICKNESS))
                .faces(">Z")
                .wires()
                .toPending()
                .offset2D(-PRINTER_MIN_OUTER_WALL_WIDTH - BOX_HOURGLASS_OFFSET - tolerance - BOX_WALL_THICKNESS, kind="intersection")
            )
            .wires()
            .toPending()
            .loft(ruled=True)
            .add(cq_split_body_bottom)
        )
        return cq_split_body_bottom
    cq_split_body_top = (
        cq_box_full
        .cut(get_cq_split_body_bottom(TOLERANCE))
    )

    cq_box_top = (
        cq_box
        .cut(get_cq_split_body_bottom())
    )
    cq_box_bottom = (
        cq_box
        .cut(cq_split_body_top)
    )

    cq_box_top_with_tolerance = (
        cq_box_with_tolerance
        .cut(get_cq_split_body_bottom(TOLERANCE))
    )

    ############# Module PCB Slot
    if is_power_supply:
        cq_module_pcb_with_tolerance = make_offset_shape(cq.Workplane(cq_power_supply_pcb), cq.Vector(PCB_TOLERANCE, PCB_TOLERANCE, PCB_TOLERANCE))
    else:
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

    if is_power_supply:
        cq_box_top = cq_box_top.cut(
            cq_esp32
            .faces(">Z")
            .wires().toPending()
            .extrude(-(box_height + box_depth))
        )

    ############# Module Pillars
    module_pillar_height = box_depth - BOX_WALL_THICKNESS - PCB_TOLERANCE + PCB_THICKNESS
    module_pillar_translation = 0.5 * box_length - BOX_WALL_THICKNESS - 0.5 * MODULE_PILLAR_DIAMETER
    module_pillar_positions = [
        (module_pillar_translation, module_pillar_translation),
        (module_pillar_translation, -module_pillar_translation),
        (-module_pillar_translation, module_pillar_translation),
        (-module_pillar_translation, -module_pillar_translation),
    ]
    cq_module_pillar = (
        cq.Workplane()
        .pushPoints(module_pillar_positions)
        .eachpoint(
            cq.Workplane()
            .rect(MODULE_PILLAR_DIAMETER, MODULE_PILLAR_DIAMETER)
            .extrude(-module_pillar_height)
            .rotate((0, 0, 0), (0, 0, 1), 45)
        )
        .translate((
            0,
            0,
            -PCB_TOLERANCE + PCB_THICKNESS,
        ))
        .intersect(cq_box_original)
        .cut(cq_box)
        .cut(cq_box_top_with_tolerance)
        .cut(cq_module_pcb_with_tolerance)
    )
    cq_box_bottom = cq_box_bottom.union(cq_module_pillar)

    ############# Clipping on the Module Pillars
    clip_connector_translation = module_pillar_translation + CLIP_CONNECTOR_OFFSET
    clip_connector_positions = [
        (clip_connector_translation, clip_connector_translation),
        (clip_connector_translation, -clip_connector_translation),
        (-clip_connector_translation, clip_connector_translation),
        (-clip_connector_translation, -clip_connector_translation),
    ]
    def build_clip_connector(tolerance: float = 0) -> cq.Workplane:
        return (
            cq.Workplane()
            .pushPoints(clip_connector_positions)
            .eachpoint(
                build_octahedron(CLIP_CONNECTOR_THICKNESS + tolerance)
                .rotate((0, 0, 0), (0, 0, 1), 45)
            )
            .intersect(cq_box_original)
        )
    cq_clip_connector = build_clip_connector().translate((0, 0, CLIP_CONNECTOR_OFFSET_Z))
    cq_clip_connector_with_tolerance = build_clip_connector(CLIP_CONNECTOR_TOLERANCE)
    cq_box_top = cq_box_top.union(cq_clip_connector)
    cq_box_bottom = cq_box_bottom.cut(cq_clip_connector_with_tolerance)

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
