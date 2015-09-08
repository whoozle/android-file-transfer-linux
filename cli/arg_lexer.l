%option noyywrap nounput
%option reentrant
%option 8bit
%option nounistd
%option batch
%option never-interactive
%option prefix="args_"

%top {
#	include <cli/Tokenizer.h>
}

%{
#	include <stdio.h>
#	include <stdlib.h>
#	include <iostream>
#	include <sstream>

#	define LVAL (yylval_param)
#	define PARSER (reinterpret_cast<cli::Tokenizer *>(yyget_extra(yyscanner)))
#	define YY_INPUT(buf,result,max_size) result = PARSER->Input(buf, max_size);

static void yy_fatal_error(const char*, yyscan_t);
void (*hide_warning)(const char*, yyscan_t) = &yy_fatal_error;

%}

%x STRING

%%

\"											{ BEGIN(STRING); }

<STRING>\"									{ BEGIN(INITIAL); }
<STRING>[^\\\"]+							{ PARSER->Write(yytext, yyleng); }

[[:space:]]+								{ PARSER->NextArgument(); }
.											{ PARSER->Write(yytext, yyleng); }
%%
