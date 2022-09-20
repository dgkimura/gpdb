//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 VMware, Inc. or its affiliates.
//
//	@filename:
//		CDXLPhysicalBitmapTableScan.h
//
//	@doc:
//		Class for representing DXL bitmap table scan operators.
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLPhysicalBitmapTableScan_H
#define GPDXL_CDXLPhysicalBitmapTableScan_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "naucrates/dxl/operators/CDXLPhysicalAbstractBitmapScan.h"
#include "naucrates/dxl/operators/CDXLTableDescr.h"

namespace gpdxl
{
using namespace gpos;

// fwd declarations
class CXMLSerializer;

//---------------------------------------------------------------------------
//	@class:
//		CDXLPhysicalBitmapTableScan
//
//	@doc:
//		Class for representing DXL bitmap table scan operators
//
//---------------------------------------------------------------------------
class CDXLPhysicalBitmapTableScan : public CDXLPhysicalAbstractBitmapScan
{
private:
public:
	CDXLPhysicalBitmapTableScan(const CDXLPhysicalBitmapTableScan &) = delete;

	// ctors
	CDXLPhysicalBitmapTableScan(CMemoryPool *mp,
								gpos::owner<CDXLTableDescr *> table_descr)
		: CDXLPhysicalAbstractBitmapScan(mp, std::move(table_descr))
	{
	}

	// dtor
	~CDXLPhysicalBitmapTableScan() override = default;

	// operator type
	Edxlopid
	GetDXLOperator() const override
	{
		return EdxlopPhysicalBitmapTableScan;
	}

	// operator name
	const CWStringConst *GetOpNameStr() const override;

	// serialize operator in DXL format
	void SerializeToDXL(CXMLSerializer *xml_serializer,
						gpos::pointer<const CDXLNode *> dxlnode) const override;

	// conversion function
	static gpos::cast_func<CDXLPhysicalBitmapTableScan *>
	Cast(CDXLOperator *dxl_op)
	{
		GPOS_ASSERT(nullptr != dxl_op);
		GPOS_ASSERT(EdxlopPhysicalBitmapTableScan == dxl_op->GetDXLOperator());

		return dynamic_cast<CDXLPhysicalBitmapTableScan *>(dxl_op);
	}

};	// class CDXLPhysicalBitmapTableScan
}  // namespace gpdxl

#endif	// !GPDXL_CDXLPhysicalBitmapTableScan_H

// EOF
