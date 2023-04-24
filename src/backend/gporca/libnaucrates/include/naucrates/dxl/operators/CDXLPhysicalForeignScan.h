//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 VMware, Inc. or its affiliates.
//
//	@filename:
//		CDXLPhysicalForeignScan.h
//
//	@doc:
//		Class for representing DXL foreign scan operators
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLPhysicalForeignScan_H
#define GPDXL_CDXLPhysicalForeignScan_H

#include "gpos/base.h"
#include "gpos/common/Casting.h"

#include "naucrates/dxl/operators/CDXLPhysicalTableScan.h"
#include "naucrates/dxl/operators/CDXLTableDescr.h"
namespace gpdxl
{
//---------------------------------------------------------------------------
//	@class:
//		CDXLPhysicalForeignScan
//
//	@doc:
//		Class for representing DXL foreign scan operators
//
//---------------------------------------------------------------------------
class CDXLPhysicalForeignScan : public CDXLPhysicalTableScan
{
private:
public:
	CDXLPhysicalForeignScan(CDXLPhysicalForeignScan &) = delete;

	// ctors
	explicit CDXLPhysicalForeignScan(CMemoryPool *mp);

	CDXLPhysicalForeignScan(CMemoryPool *mp,
							gpos::Ref<CDXLTableDescr> table_descr);

	// operator type
	Edxlopid GetDXLOperator() const override;

	// operator name
	const CWStringConst *GetOpNameStr() const override;

	// conversion function
	static gpos::Ref<CDXLPhysicalForeignScan>
	Cast(gpos::Ref<CDXLOperator> dxl_op)
	{
		GPOS_ASSERT(nullptr != dxl_op);
		GPOS_ASSERT(EdxlopPhysicalForeignScan == dxl_op->GetDXLOperator());

		return gpos::dyn_cast<CDXLPhysicalForeignScan>(dxl_op);
	}
};
}  // namespace gpdxl
#endif	// !GPDXL_CDXLPhysicalForeignScan_H

// EOF
