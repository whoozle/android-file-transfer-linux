#!/usr/bin/env python

import os
import os.path
import re

license = """/*
MIT License

Copyright (c) 2008-2019 Vladimir Menshakov

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

"""

license_re = re.compile(r'^/\*.*?\*/\s+', re.M | re.S)

for root, dirs, files in os.walk('.'):
	if root == "./mtp/backend/linux/usb/linux":
		continue

	for file in files:
		name, ext = os.path.splitext(file)
		if ext != '.h' and ext != '.cpp':
			continue
		if name == "arg_lexer.l":
			continue

		fname = os.path.join(root, file)
		with open(fname) as f:
			data = f.read()

		if license_re.match(data):
			data = license_re.sub(license, data, 1)
		else:
			data = license + data

		with open(fname, 'w') as f:
			f.write(data)
