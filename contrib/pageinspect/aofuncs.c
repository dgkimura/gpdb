#include "postgres.h"

#include "funcapi.h"
#include "fmgr.h"
#include "cdb/cdbappendonlystorageread.h"
#include "cdb/cdbappendonlystorageformat.h"

static char *
to_string_header_kind(AoHeaderKind kind)
{
	switch (kind)
	{
		case AoHeaderKind_None:
			return "none";
		case AoHeaderKind_SmallContent:
			return "small content";
		case AoHeaderKind_LargeContent:
			return "large content";
		case AoHeaderKind_NonBulkDenseContent:
			return "non-bulk dense content";
		case AoHeaderKind_BulkDenseContent:
			return "bulk dense content";
		case MaxAoHeaderKind:
			return "invalid header";
	}
	return "unknown";
}

PG_FUNCTION_INFO_V1(ao_page_header);

Datum
ao_page_header(PG_FUNCTION_ARGS)
{
	bytea	   *raw_page = PG_GETARG_BYTEA_P(0);

	Datum		result;
	HeapTuple	tuple;
	TupleDesc	tupleDesc;
	char       *values[7];

	AppendOnlyStorageReadCurrent blkHdr;
	bool useChecksum = true; /* TODO: should this be an option? */

	memset(&blkHdr, 0, sizeof(AppendOnlyStorageReadCurrent));
	AppendOnlyStorageFormat_GetHeaderInfo((uint8 *)(raw_page->vl_dat), useChecksum,
			&blkHdr.headerKind, &blkHdr.actualHeaderLen);

	if (get_call_result_type(fcinfo, NULL, &tupleDesc) != TYPEFUNC_COMPOSITE)
		elog(ERROR, "return type must be a row type");

	values[0] = psprintf("%s", to_string_header_kind(blkHdr.headerKind));
	values[1] = psprintf("%d", 0); /* TODO: add checksum */
	values[2] = psprintf("%d", blkHdr.rowCount);
	values[3] = psprintf("%ld", blkHdr.firstRowNum);
	values[4] = psprintf("%d", blkHdr.actualHeaderLen);
	values[5] = psprintf("%d", blkHdr.compressedLen);
	values[6] = psprintf("%d", blkHdr.uncompressedLen);
	tuple = BuildTupleFromCStrings(TupleDescGetAttInMetadata(tupleDesc),
								   values);
	result = HeapTupleGetDatum(tuple);
	PG_RETURN_DATUM(result);
}

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
