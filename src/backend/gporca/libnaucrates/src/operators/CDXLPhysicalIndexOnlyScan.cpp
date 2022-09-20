//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLPhysicalIndexOnlyScan.cpp
//
//	@doc:
//		Implementation of DXL physical index only scan operators
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLPhysicalIndexOnlyScan.h"

#include "gpos/common/owner.h"

#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalIndexOnlyScan::CDXLPhysicalIndexOnlyScan
//
//	@doc:
//		Construct an index only scan node given its table descriptor,
//		index descriptor and filter conditions on the index
//
//---------------------------------------------------------------------------
CDXLPhysicalIndexOnlyScan::CDXLPhysicalIndexOnlyScan(
	CMemoryPool *mp, gpos::owner<CDXLTableDescr *> table_descr,
	gpos::owner<CDXLIndexDescr *> dxl_index_descr,
	EdxlIndexScanDirection idx_scan_direction)
	: CDXLPhysicalIndexScan(mp, std::move(table_descr),
							std::move(dxl_index_descr), idx_scan_direction)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalIndexOnlyScan::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLPhysicalIndexOnlyScan::GetDXLOperator() const
{
	return EdxlopPhysicalIndexOnlyScan;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalIndexOnlyScan::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLPhysicalIndexOnlyScan::GetOpNameStr() const
{
	return CDXLTokens::GetDXLTokenStr(EdxltokenPhysicalIndexOnlyScan);
}

// EOF
