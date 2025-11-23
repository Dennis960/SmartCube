import os

from OCP import IFSelect
from OCP.STEPCAFControl import STEPCAFControl_Reader
from OCP.TCollection import TCollection_ExtendedString
from OCP.TDataStd import TDataStd_Name
from OCP.TDF import TDF_ChildIterator, TDF_LabelSequence, TDF_Label
from OCP.TDocStd import TDocStd_Document
from OCP.TopoDS import TopoDS_Shape
from OCP.XCAFDoc import XCAFDoc_DocumentTool

from io import BytesIO
import cadquery as cq
from serializer import register
import pickle

register()

_path_to_script = os.path.dirname(os.path.abspath(__file__))
_pcb_folder = os.path.abspath(os.path.join(_path_to_script, "..", "..", "PCB"))
_footprints_folder = os.path.abspath(
    os.path.join(_pcb_folder, "lib", "CustomFootprints.pretty")
)
_output_folder = os.path.abspath(os.path.join(_path_to_script, "..", "models"))


def _convert_kicad_pcb(
    kicad_pcb_file: str,
    step_file: str,
):
    """
    Converts a KiCad PCB file to a STEP file using kicad-cli.
    """
    kicad_pcb_file = os.path.abspath(kicad_pcb_file)
    step_file = os.path.abspath(step_file)
    print("Converting " + kicad_pcb_file + " to " + step_file)
    kicad_cli_cmd = f'kicad-cli pcb export step "{kicad_pcb_file}" --drill-origin --no-dnp --subst-models -o "{step_file}"'
    print(f"Running command: {kicad_cli_cmd}")
    current_dir = os.getcwd()
    os.chdir(_footprints_folder)
    os.system(kicad_cli_cmd)
    os.chdir(current_dir)


def _get_kicad_pcb_step_file(
    kicad_pcb_name: str,
):
    """
    Returns the path to the STEP file for the given KiCad PCB name.
    """
    return os.path.join(_output_folder, f"{kicad_pcb_name}.step")


def _get_kicad_pcb_file(
    kicad_pcb_name: str,
):
    """
    Returns the path to the KiCad PCB file for the given KiCad PCB name.
    """
    return os.path.join(_pcb_folder, kicad_pcb_name, f"{kicad_pcb_name}.kicad_pcb")


def _get_kicad_pcb_pickle_file(
    kicad_pcb_name: str,
):
    """
    Returns the path to the pickle file for the given KiCad PCB name.
    """
    return os.path.join(_output_folder, f"{kicad_pcb_name}.pkl")


def _step_to_shapes_dict(
    step_file: str, pcb_part_name: str, full_name: str
) -> dict[str, cq.Shape]:
    """
    Loads the individual components from the step file as cq.Shape into a dictionary.\n

    :param step_file: Path to the step file.
    :param pcb_part_name: The part name of the PCB in the STEP file.

    :return: A dictionary of names and cq.Shape objects.
    """
    step_bytes = open(step_file, "rb").read()
    doc = TDocStd_Document(TCollection_ExtendedString("doc"))
    reader = STEPCAFControl_Reader()
    status = reader.Reader().ReadStream(step_file, BytesIO(step_bytes))
    if status != IFSelect.IFSelect_RetDone:
        raise Exception(f"Error reading file {step_file}")

    reader.Transfer(doc)
    shapeTool = XCAFDoc_DocumentTool.ShapeTool_s(doc.Main())

    shapes: dict[str, cq.Shape] = {}
    freeShapes = TDF_LabelSequence()
    shapeTool.GetFreeShapes(freeShapes)
    if freeShapes.Length() != 1:
        raise Exception(f"Expected 1 free shape, found {freeShapes.Length()}")
    board_shape = shapeTool.GetShape_s(freeShapes.Value(1))

    tdf_iterator = TDF_ChildIterator(freeShapes.Value(1))
    while tdf_iterator.More():
        label = tdf_iterator.Value()
        tdf_iterator.Next()
        if shapeTool.IsShape_s(label):
            shape = shapeTool.GetShape_s(label)

            refLabel = TDF_Label()
            nameAttr = TDataStd_Name()
            name = ""
            if shapeTool.GetReferredShape_s(label, refLabel):
                if refLabel.FindAttribute(TDataStd_Name.GetID_s(), nameAttr):
                    name = nameAttr.Get().ToExtString()
            newName = name
            if "PCB" in name:
                newName = pcb_part_name
            i = 1
            while newName in shapes:
                newName = name + f" ({i})"
                i += 1
            shapes[newName] = cq.Shape.cast(shape)
        else:
            print(f"Label {label} is not a shape")
    shapes[full_name] = cq.Shape.cast(board_shape)
    return shapes


def get_kicad_pcb_modification_time(kicad_pcb_name: str) -> float:
    kicad_pcb_file = _get_kicad_pcb_file(kicad_pcb_name)
    return os.path.getmtime(kicad_pcb_file)


def save_to_pickle(shapes_dict: dict[str, cq.Shape], kicad_pcb_name: str):
    pickle_file = _get_kicad_pcb_pickle_file(kicad_pcb_name)
    modification_time = get_kicad_pcb_modification_time(kicad_pcb_name)
    with open(pickle_file, "wb") as f:
        pickle.dump((modification_time, shapes_dict), f)


def load_from_pickle(filename: str) -> tuple[float, dict[str, cq.Shape]]:
    with open(filename, "rb") as f:
        (modification_time, shapes_dict_cq_shape) = pickle.load(f)
    return modification_time, shapes_dict_cq_shape


def get_kicad_pcbs_as_shapes_dicts(
    kicad_pcb_names: list[str],
    pcb_part_name: str = "PCB",
    full_name: str = "FullBoard",
):
    """
    Loads the KiCad PCBs as cadquery shapes dictionaries.
    :param kicad_pcb_names: List of KiCad PCB names to load.
    :param pcb_part_name: The part name of the PCB in the STEP file.
    :return: A dictionary of KiCad PCB names and their corresponding cadquery shapes dictionaries.
    """
    shapes_dicts: dict[str, dict[str, cq.Shape]] = {}
    for kicad_pcb_name in kicad_pcb_names:
        pickle_file = _get_kicad_pcb_pickle_file(kicad_pcb_name)
        is_kicad_pcb_pickle_outdated = True
        if os.path.exists(pickle_file):
            try:
                print(f"Loading {kicad_pcb_name} from pickle file {pickle_file}")
                modification_time, shapes_dict = load_from_pickle(pickle_file)
                is_kicad_pcb_pickle_outdated = (
                    modification_time != get_kicad_pcb_modification_time(kicad_pcb_name)
                )
                shapes_dicts[kicad_pcb_name] = shapes_dict
            except Exception as e:
                print(f"Error loading pickle file {pickle_file}: {e}")
                is_kicad_pcb_pickle_outdated = True
        if is_kicad_pcb_pickle_outdated:
            print(f"KiCad PCB pickle file {pickle_file} is outdated or does not exist.")
            kicad_pcb_file = _get_kicad_pcb_file(kicad_pcb_name)
            step_file = _get_kicad_pcb_step_file(kicad_pcb_name)
            _convert_kicad_pcb(kicad_pcb_file, step_file)
            shapes_dict = _step_to_shapes_dict(step_file, pcb_part_name, full_name)
            shapes_dicts[kicad_pcb_name] = shapes_dict
            save_to_pickle(shapes_dicts[kicad_pcb_name], kicad_pcb_name)

    return shapes_dicts


def shapes_dict_to_cq_object(shapes_dict: dict[str, cq.Shape]) -> cq.Workplane:
    """
    Converts a shapes dictionary to a cadquery Workplane object by combining all shapes.

    :param shapes_dict: Dictionary of names and cadquery Shape objects.

    :return: A cadquery Workplane object containing all shapes.
    """
    cq_object = cq.Workplane("XY")
    for name, shape in shapes_dict.items():
        cq_object = cq_object.add(shape)
    return cq_object
