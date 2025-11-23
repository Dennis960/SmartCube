import cadquery as cq
from typing import Any


def debug_show_no_exit(
    *objs: cq.Workplane | Any,
    **kobjs: cq.Workplane | Any,
):
    """
    Show a cq object in the cadquery viewer
    """
    import ocp_vscode

    def to_cq_object(obj: cq.Workplane):
        if isinstance(obj, cq.Workplane):
            return obj
        else:
            raise TypeError(f"Unsupported type: {type(obj)}")

    for obj in objs:
        ocp_vscode.show_object(to_cq_object(obj))
    for key, obj in kobjs.items():
        ocp_vscode.show_object(to_cq_object(obj), name=key)


def debug_show(*objs: cq.Workplane | Any, **kobjs: cq.Workplane | Any) -> None:
    """
    Show a cq object in the cadquery viewer and exit
    """
    debug_show_no_exit(*objs, **kobjs)
    exit()
