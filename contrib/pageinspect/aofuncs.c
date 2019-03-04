#include "postgres.h"

#include "funcapi.h"
#include "fmgr.h"

PG_FUNCTION_INFO_V1(ao_page_items);

Datum
ao_page_items(PG_FUNCTION_ARGS)
{
	Datum		result;
	HeapTuple	tuple;
	TupleDesc	tupleDesc;
	char       *values[2];

	if (get_call_result_type(fcinfo, NULL, &tupleDesc) != TYPEFUNC_COMPOSITE)
		elog(ERROR, "return type must be a row type");
	values[0] = psprintf("%s", "small");
	values[1] = psprintf("%d", 42);
	tuple = BuildTupleFromCStrings(TupleDescGetAttInMetadata(tupleDesc),
								   values);
	result = HeapTupleGetDatum(tuple);
	PG_RETURN_DATUM(result);
}
