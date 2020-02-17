#!/usr/bin/env python

import os
import os.path
import re

license = """/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015-2020  Vladimir Menshakov

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2.1 of the License,
    or (at your option) any later version.

    This library is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this library; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
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
