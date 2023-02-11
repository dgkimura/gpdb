//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 EMC Corp.
//
//	@filename:
//		CDXLPhysicalDynamicIndexScan.h
//
//	@doc:
//		Class for representing DXL dynamic index scan operators
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLPhysicalDynamicIndexScan_H
#define GPDXL_CDXLPhysicalDynamicIndexScan_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "naucrates/dxl/operators/CDXLIndexDescr.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/operators/CDXLPhysical.h"
#include "naucrates/dxl/operators/CDXLTableDescr.h"

namespace gpdxl
{
//---------------------------------------------------------------------------
//	@class:
//		CDXLPhysicalDynamicIndexScan
//
//	@doc:
//		Class for representing DXL dynamic index scan operators
//
//---------------------------------------------------------------------------
class CDXLPhysicalDynamicIndexScan : public CDXLPhysical
{
private:
	// table descriptor for the scanned table
	gpos::Ref<CDXLTableDescr> m_dxl_table_descr;

	// index descriptor associated with the scanned table
	gpos::Ref<CDXLIndexDescr> m_dxl_index_descr;

	// scan direction of the index
	EdxlIndexScanDirection m_index_scan_dir;

	gpos::Ref<IMdIdArray> m_part_mdids;

	gpos::Ref<ULongPtrArray> m_selector_ids = nullptr;

public:
	CDXLPhysicalDynamicIndexScan(CDXLPhysicalDynamicIndexScan &) = delete;

	// indices of dynamic index scan elements in the children array
	enum Edxldis
	{
		EdxldisIndexProjList = 0,
		EdxldisIndexFilter,
		EdxldisIndexCondition,
		EdxldisSentinel
	};

	//ctor
	CDXLPhysicalDynamicIndexScan(CMemoryPool *mp,
								 gpos::Ref<CDXLTableDescr> table_descr,
								 gpos::Ref<CDXLIndexDescr> dxl_index_descr,
								 EdxlIndexScanDirection idx_scan_direction,
								 gpos::Ref<IMdIdArray> part_mdids,
								 gpos::Ref<ULongPtrArray> selector_ids);

	//dtor
	~CDXLPhysicalDynamicIndexScan() override;

	// operator type
	Edxlopid GetDXLOperator() const override;

	// operator name
	const CWStringConst *GetOpNameStr() const override;

	// index descriptor
	const gpos::Ref<CDXLIndexDescr> GetDXLIndexDescr() const;

	//table descriptor
	const gpos::Ref<CDXLTableDescr> GetDXLTableDescr() const;

	// scan direction
	EdxlIndexScanDirection GetIndexScanDir() const;

	IMdIdArray *GetParts() const;

	const ULongPtrArray *
	GetSelectorIds() const
	{
		return m_selector_ids.get();
	}

	// serialize operator in DXL format
	void SerializeToDXL(CXMLSerializer *xml_serializer,
						const CDXLNode *node) const override;

	// conversion function
	static CDXLPhysicalDynamicIndexScan *
	Cast(gpos::Ref<CDXLOperator> dxl_op)
	{
		GPOS_ASSERT(nullptr != dxl_op);
		GPOS_ASSERT(EdxlopPhysicalDynamicIndexScan == dxl_op->GetDXLOperator());

		return dynamic_cast<CDXLPhysicalDynamicIndexScan *>(dxl_op);
	}

#ifdef GPOS_DEBUG
	// checks whether the operator has valid structure, i.e. number and
	// types of child nodes
	void AssertValid(const gpos::Ref<CDXLNode>,
					 BOOL validate_children) const override;
#endif	// GPOS_DEBUG
};
}  // namespace gpdxl
#endif	// !GPDXL_CDXLPhysicalDynamicIndexScan_H

// EOF
