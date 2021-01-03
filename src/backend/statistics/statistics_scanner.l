%{
#include "postgres.h"

#include "utils/builtins.h"
#include "parser/scansup.h"

/* Handle to the buffer that the lexer uses internally */
static YY_BUFFER_STATE scanbufhandle;
%}

%option noyywrap
%option prefix="statistic_yy"

dquote			\"
xdstart			{dquote}
xdstop			{dquote}

digit			[0-9]+

%%

[ \t\n]		;
"{"			{ return '{'; }
"}"			{ return '}'; }
","			{ return ','; }
"\""		{ return '"'; }
":"			{ return ':'; }

{digit}+		{
					yylval.uintval = strtoul(yytext, NULL, 10);
					return UCONST;
				}

%%

void
yyerror(const char *message)
{
	ereport(ERROR,
			(errcode(ERRCODE_SYNTAX_ERROR),
			 errmsg_internal("%s", message)));
}

void
statistics_scanner_init(const char *str)
{
	Size		slen = strlen(str);
	char	   *scanbuf;

	/*
	 * Might be left over after ereport()
	 */
	if (YY_CURRENT_BUFFER)
		yy_delete_buffer(YY_CURRENT_BUFFER);

	/*
	 * Make a scan buffer with special termination needed by flex.
	 */
	scanbuf = (char *) palloc(slen + 2);
	memcpy(scanbuf, str, slen);
	scanbuf[slen] = scanbuf[slen + 1] = YY_END_OF_BUFFER_CHAR;
	scanbufhandle = yy_scan_buffer(scanbuf, slen + 2);
}

void
statistic_scanner_finish(void)
{
	yy_delete_buffer(scanbufhandle);
	scanbufhandle = NULL;
}
