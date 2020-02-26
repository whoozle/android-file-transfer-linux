#!/usr/bin/python3

#!/usr/bin/env python

import os
import os.path
import re

guard_re = re.compile(r'#ifndef\s+(\w+)\n#define\s+(\w+)', re.M | re.S)
base_dir = os.path.dirname(os.path.realpath(__file__))
clean_re = re.compile(r'[^\w]')
print(base_dir)

for root, dirs, files in os.walk('.'):
	root = os.path.relpath(root, base_dir)
	if (root[0] == '.' and len(root) > 1) or root.startswith("build"):
		dirs[:] = []
		continue
	if root == "mtp/backend/linux/usb/linux":
		continue

	for file in files:
		name, ext = os.path.splitext(file)
		if ext != '.h' or name == "arg_lexer.l" or name.endswith(".values"):
			continue

		fname = os.path.join(root, file)
		with open(fname) as f:
			data = f.read()

		guard = 'AFTL_' + clean_re.sub('_', os.path.relpath(os.path.join(root, file), base_dir)).upper()

		def replace_guard(m):
			return "#ifndef %s\n#define %s" %(guard, guard)

		data, n = guard_re.subn(replace_guard, data, count=1)
		if n == 0:
			print("missing guard in %s" %file)
		else:
			with open(fname, 'w') as f:
				f.write(data)

