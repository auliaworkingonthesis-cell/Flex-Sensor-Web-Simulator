import bpy
import math
import os
import sys

source = sys.argv[sys.argv.index("--") + 1]
target = sys.argv[sys.argv.index("--") + 2]

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=source)

for obj in list(bpy.context.scene.objects):
    if obj.type in {"CAMERA", "LIGHT"} or obj.name == "Cube":
        bpy.data.objects.remove(obj, do_unlink=True)

body = bpy.data.materials.new("SCARA Body")
body.diffuse_color = (0.045, 0.12, 0.14, 1)
body.metallic = 0.18
body.roughness = 0.54

accent = bpy.data.materials.new("SCARA Accent")
accent.diffuse_color = (0.08, 0.72, 0.78, 1)
accent.metallic = 0.16
accent.roughness = 0.42

active = bpy.data.materials.new("SCARA Active")
active.diffuse_color = (0.72, 0.025, 0.025, 1)
active.metallic = 0.12
active.roughness = 0.48

servo = bpy.data.materials.new("Servo")
servo.diffuse_color = (0.055, 0.065, 0.075, 1)
servo.metallic = 0.08
servo.roughness = 0.68

for obj in bpy.context.scene.objects:
    if obj.type != "MESH":
        continue
    obj.data.materials.clear()
    parent_name = obj.parent.name.lower() if obj.parent else ""
    ancestry = parent_name
    parent = obj.parent
    while parent:
        ancestry += " " + parent.name.lower()
        parent = parent.parent
    if "rcxaz" in parent_name:
        obj.data.materials.append(servo)
    elif "cremagliera" in ancestry:
        obj.data.materials.append(accent)
    elif any(name in ancestry for name in ("pignone", "assieme_pinza", "frame_guida", "braccio", "tappo_cremagliera")):
        obj.data.materials.append(active)
    else:
        obj.data.materials.append(body)

os.makedirs(os.path.dirname(target), exist_ok=True)
bpy.ops.export_scene.gltf(
    filepath=target,
    export_format="GLB",
    export_apply=True,
    export_materials="EXPORT",
    export_cameras=False,
    export_lights=False,
)
