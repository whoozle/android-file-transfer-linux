#!/usr/bin/python3

# -*- coding: utf-8 -*-
"""
hierarchical prompt usage example
"""
from __future__ import print_function, unicode_literals
from PyInquirer import style_from_dict, Token, prompt

import aftl

def get_storage_prompt(session, s):
	info = session.get_storage_info(s)
	name = info.StorageDescription
	if not name:
		name = repr(s)
	return { 'name': name, 'value': s }

def get_storage(session):
	storage_prompt = {
		'type': 'list',
		'name': 'storage',
		'message': 'Select storage',
		'choices': list(map(lambda s: get_storage_prompt(session, s), session.get_storage_ids())) + [{'name': 'Quit', 'value': None}]
	}
	answers = prompt(storage_prompt)
	return answers.get('storage')

def get_object_prompt(session, object):
	info = session.get_object_info(object)
	name = info.Filename
	if info.ObjectFormat == aftl.ObjectFormat.Association:
		name += '/'
	return {'name': name, 'value': (object, info.ObjectFormat) }

def get_object(session, storage, parent):
	objects = session.get_object_handles(storage, aftl.ObjectFormat.Any, parent)
	object_prompt = {
		'type': 'list',
		'name': 'object',
		'message': 'Browse objects',
		'choices': [{'name': '..', 'value': (None, None) }] + \
			list(map(lambda o: get_object_prompt(session, o), objects))
	}
	answers = prompt(object_prompt)
	return answers.get('object')


def main():
	session = aftl.Device.find_first().open_session()
	while True:
		storage = get_storage(session)
		if storage is None:
			break

		path = [aftl.Session.Root]
		while path:
			object, type = get_object(session, storage, path[-1])
			if type == aftl.ObjectFormat.Association:
				path.append(object)
				continue

			if object is None:
				path.pop()


if __name__ == '__main__':
	main()
