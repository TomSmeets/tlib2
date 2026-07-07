import os

copyright = "// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>"
src_path = 'src'
for mod_name in os.listdir(src_path):
    mod_path = f'{src_path}/{mod_name}'
    for file_name in os.listdir(mod_path):
        file_path = f'{mod_path}/{file_name}'
        if file_name.endswith(".h") or file_name.endswith(".c"):
            print(mod_name, file_name)
            with open(file_path, 'r') as f:
                 lines = f.readlines()

            if lines[0] != copyright:
                print(f"Invalid copyright: {file_path}")
            # print(lines[0])
