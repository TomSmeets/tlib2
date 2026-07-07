import os
src_path = 'src'
copyright = "// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>\n"

# [mod][file] -> [dep]
deps = {}
for mod_name in os.listdir(src_path):
    mod_path = f'{src_path}/{mod_name}'
    deps[mod_name] = {}
    for file_name in os.listdir(mod_path):
        file_path = f'{mod_path}/{file_name}'
        if file_name.endswith(".h") or file_name.endswith(".c"):
            deps[mod_name][file_name] = []
            with open(file_path, 'r') as f:
                 lines = f.readlines()
            for l in lines:
                if l.startswith('#include \"'):
                    dep = l[10:-2]
                    deps[mod_name][file_name].append(dep)

# [mod][file] -> [dep]
# print(deps)

print("digraph {")
print("overlap = false;")
for mod,files in deps.items():
    print(f"subgraph cluster_{mod} {{")
    print(f"label = {mod}")
    for file,deps2 in files.items():
        print(f'"{file}";')
    print("}")

print(f"subgraph cluster_modules {{")
print(f"label = Modules")
for mod,files in deps.items():
    print(f'"{mod}";')
print("}")

for mod,files in deps.items():
    for file,deps2 in files.items():
        for dep2 in deps2:
            owner = [mod for mod,files in deps.items() if dep2 in files]
            if owner == [] or owner[0] == mod:
                print(f'"{file}" -> "{dep2}";')
            else:
                print(f'"{file}" -> "{dep2}"[color=gray];')
                # print(f"{mod} -> {owner[0]};")
print("}")

# for mod_name in os.listdir(src_path):
#     mod_path = f'{src_path}/{mod_name}'
#     for file_name in os.listdir(mod_path):
#         file_path = f'{mod_path}/{file_name}'
#         if file_name.endswith(".h") or file_name.endswith(".c"):
#             print(mod_name, file_name)
#             with open(file_path, 'r') as f:
#                  lines = f.readlines()

#             # if lines[0] != copyright:
#             #     print(f"Invalid copyright: {file_path}")
#             #     lines[0] = copyright
#             #     with open(file_path, 'w') as f:
#             #          f.writelines(lines)
#             # print(lines[0])

#             for l in lines:
#                 if l.startswith('#include \"'):
#                     dep = l[10:]
#                     print(dep)
                    
