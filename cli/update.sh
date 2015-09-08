#!/bin/sh

flex -L -F -o arg_lexer.l.cpp --header-file=arg_lexer.l.h arg_lexer.l
