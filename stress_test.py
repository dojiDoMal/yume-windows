import json
import math

NUM_OBJECTS = 8000  # altere aqui

with open("scene.scn", "r") as f:
    scene = json.load(f)

mesh_obj = next(o for o in scene["worldObjects"] if any(c["type"] == "MESH_RENDERER" for c in o["components"]))

# Remove o original e adiciona N cópias em grid
scene["worldObjects"] = [o for o in scene["worldObjects"] if not any(c["type"] == "MESH_RENDERER" for c in o["components"])]

cols = int(math.ceil(math.sqrt(NUM_OBJECTS)))
for i in range(NUM_OBJECTS):
    import copy
    obj = copy.deepcopy(mesh_obj)
    obj["transform"]["position"] = [
        (i % cols) * 2.5,
        0.0,
        (i // cols) * 2.5
    ]
    scene["worldObjects"].append(obj)

with open("scene.scn", "w") as f:
    json.dump(scene, f, indent=2)

print(f"{NUM_OBJECTS} objetos adicionados.")
