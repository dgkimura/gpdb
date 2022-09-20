//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 VMware, Inc. or its affiliates.
//
//	@filename:
//		CDXLPhysicalAbstractBitmapScan.h
//
//	@doc:
//		Parent class for representing DXL bitmap table scan operators,
//		both not partitioned and dynamic.
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLPhysicalAbstractBitmapScan_H
#define GPDXL_CDXLPhysicalAbstractBitmapScan_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "naucrates/dxl/operators/CDXLPhysical.h"
#include "naucrates/dxl/operators/CDXLTableDescr.h"

namespace gpdxl
{
using namespace gpos;

enum Edxlbs
{
	EdxlbsIndexProjList = 0,
	EdxlbsIndexFilter,
	EdxlbsIndexRecheckCond,
	EdxlbsIndexBitmap,
	EdxlbsSentinel
};

// fwd declarations
class CXMLSerializer;

//---------------------------------------------------------------------------
//	@class:
//		CDXLPhysicalAbstractBitmapScan
//
//	@doc:
//		Parent class for representing DXL bitmap table scan operators, both not
//		partitioned and dynamic.
//
//---------------------------------------------------------------------------
class CDXLPhysicalAbstractBitmapScan : public CDXLPhysical
{
private:
protected:
	// table descriptor for the scanned table
	gpos::owner<CDXLTableDescr *> m_dxl_table_descr;

public:
	CDXLPhysicalAbstractBitmapScan(const CDXLPhysicalAbstractBitmapScan &) =
		delete;

	// ctor
	CDXLPhysicalAbstractBitmapScan(CMemoryPool *mp,
								   gpos::owner<CDXLTableDescr *> table_descr)
		: CDXLPhysical(mp), m_dxl_table_descr(std::move(table_descr))
	{
		GPOS_ASSERT(nullptr != m_dxl_table_descr);
	}

	// dtor
	~CDXLPhysicalAbstractBitmapScan() override;

	// table descriptor
	gpos::pointer<const CDXLTableDescr *>
	GetDXLTableDescr()
	{
		return m_dxl_table_descr;
	}

#ifdef GPOS_DEBUG
	// checks whether the operator has valid structure, i.e. number and
	// types of child nodes
	void AssertValid(gpos::pointer<const CDXLNode *> node,
					 BOOL validate_children) const override;
#endif	// GPOS_DEBUG
};		// class CDXLPhysicalAbstractBitmapScan
}  // namespace gpdxl

#endif	// !GPDXL_CDXLPhysicalAbstractBitmapScan_H

// EOF
