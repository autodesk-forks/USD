import sys

file_name = sys.argv[1]
new_file_name = sys.argv[2]
with open(file_name, 'r') as f:
    content = f.read()

with open(new_file_name, 'w') as f:
    f.write('#!/usr/bin/env node\n' + content)
