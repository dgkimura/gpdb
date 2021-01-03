%{
#include "postgres.h"

#include "statistics/extended_stats_internal.h"
#include "statistics/statistics.h"

MVNDistinct *mvndistinct_parse_result;

/*
 * Bison doesn't allocate anything that needs to live across parser calls,
 * so we can easily have it use palloc instead of malloc.  This prevents
 * memory leaks if we error out during parsing.  Note this only works with
 * bison >= 2.0.  However, in bison 1.875 the default is to use alloca()
 * if possible, so there's not really much problem anyhow, at least if
 * you're building with gcc.
 */
#define YYMALLOC palloc
#define YYFREE   pfree

%}

%expect 0
%name-prefix="statistic_yy"


%union {
		uint32					uintval;

		MVNDistinct				*ndistinct;
		MVNDistinctItem			*ndistinct_item;
		Bitmapset				*bitmap;
		List					*list;
}

/* Non-keyword tokens */
%token <uintval> UCONST

%type <ndistinct>	ndistinct
%type <ndistinct_item>	ndistinct_item
%type <bitmap>	ndistinct_attrs
%type <list>	ndistinct_item_list

%%

ndistinct:
	'{' ndistinct_item_list '}'
		{
			$$ = palloc0(MAXALIGN(offsetof(MVNDistinct, items)) +
						 list_length($2) * sizeof(MVNDistinctItem));
			mvndistinct_parse_result = $$;
			$$->magic = STATS_NDISTINCT_MAGIC;
			$$->type = STATS_NDISTINCT_TYPE_BASIC;
			$$->nitems = list_length($2);

			ListCell *cell;
			MVNDistinctItem *pointer = $$->items;
			foreach (cell, $2)
			{
				memcpy(pointer, lfirst(cell), sizeof(MVNDistinctItem));
				pointer += 1;
			}
		}
	;

ndistinct_item_list:
	ndistinct_item_list ':' ndistinct_item
		{
			$$ = lappend($1, $3);
		}
	| ndistinct_item { $$ = lappend(NIL, $1);}
	;

ndistinct_item:
	'"' ndistinct_attrs '"' ':' UCONST
		{
			$$ = (MVNDistinctItem *)palloc0(sizeof(MVNDistinctItem));
			$$->attrs = $2;
			$$->ndistinct = $5;
		}
	;

ndistinct_attrs:
	ndistinct_attrs ',' UCONST
		{
			$$ = bms_add_member($1, $3);
		}
	| UCONST ',' UCONST
		{
			$$ = bms_make_singleton($1);
			$$ = bms_add_member($$, $3);
		}
	;


%%

#include "statistics_scanner.c"
