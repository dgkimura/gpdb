//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLPhysicalDynamicTableScan.h
//
//	@doc:
//		Class for representing DXL dynamic table scan operators
//---------------------------------------------------------------------------



#ifndef GPDXL_CDXLPhysicalDynamicTableScan_H
#define GPDXL_CDXLPhysicalDynamicTableScan_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "naucrates/dxl/operators/CDXLPhysical.h"
#include "naucrates/dxl/operators/CDXLTableDescr.h"


namespace gpdxl
{
// indices of dynamic table scan elements in the children array
enum Edxldts
{
	EdxldtsIndexProjList = 0,
	EdxldtsIndexFilter,
	EdxldtsSentinel
};

//---------------------------------------------------------------------------
//	@class:
//		CDXLPhysicalDynamicTableScan
//
//	@doc:
//		Class for representing DXL dynamic table scan operators
//
//---------------------------------------------------------------------------
class CDXLPhysicalDynamicTableScan : public CDXLPhysical
{
private:
	// table descriptor for the scanned table
	gpos::owner<CDXLTableDescr *> m_dxl_table_descr;

	gpos::Ref<IMdIdArray> m_part_mdids;

	gpos::owner<ULongPtrArray *> m_selector_ids = nullptr;

public:
	CDXLPhysicalDynamicTableScan(CDXLPhysicalDynamicTableScan &) = delete;

	// ctor
	CDXLPhysicalDynamicTableScan(CMemoryPool *mp,
								 gpos::owner<CDXLTableDescr *> table_descr,
								 gpos::Ref<IMdIdArray> part_mdids,
								 gpos::owner<ULongPtrArray *> selector_ids);

	// dtor
	~CDXLPhysicalDynamicTableScan() override;

	// operator type
	Edxlopid GetDXLOperator() const override;

	// operator name
	const CWStringConst *GetOpNameStr() const override;

	// table descriptor
	gpos::pointer<const CDXLTableDescr *> GetDXLTableDescr() const;

	IMdIdArray *GetParts() const;

	gpos::pointer<const ULongPtrArray *>
	GetSelectorIds() const
	{
		return m_selector_ids;
	}
	// serialize operator in DXL format
	void SerializeToDXL(CXMLSerializer *xml_serializer,
						gpos::pointer<const CDXLNode *> node) const override;

	// conversion function
	static gpos::cast_func<CDXLPhysicalDynamicTableScan *>
	Cast(CDXLOperator *dxl_op)
	{
		GPOS_ASSERT(nullptr != dxl_op);
		GPOS_ASSERT(EdxlopPhysicalDynamicTableScan == dxl_op->GetDXLOperator());

		return dynamic_cast<CDXLPhysicalDynamicTableScan *>(dxl_op);
	}

#ifdef GPOS_DEBUG
	// checks whether the operator has valid structure, i.e. number and
	// types of child nodes
	void AssertValid(gpos::pointer<const CDXLNode *>,
					 BOOL validate_children) const override;
#endif	// GPOS_DEBUG
};
}  // namespace gpdxl
#endif	// !GPDXL_CDXLPhysicalDynamicTableScan_H

// EOF
