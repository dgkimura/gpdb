//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLPhysicalSplit.h
//
//	@doc:
//		Class for representing physical split operator
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLPhysicalSplit_H
#define GPDXL_CDXLPhysicalSplit_H

#include "gpos/base.h"
#include "gpos/common/CDynamicPtrArray.h"
#include "gpos/common/owner.h"

#include "naucrates/dxl/operators/CDXLPhysical.h"

namespace gpdxl
{
//---------------------------------------------------------------------------
//	@class:
//		CDXLPhysicalSplit
//
//	@doc:
//		Class for representing physical split operators
//
//---------------------------------------------------------------------------
class CDXLPhysicalSplit : public CDXLPhysical
{
private:
	// list of deletion column ids
	gpos::Ref<ULongPtrArray> m_deletion_colid_array;

	// list of insertion column ids
	gpos::Ref<ULongPtrArray> m_insert_colid_array;

	// action column id
	ULONG m_action_colid;

	// ctid column id
	ULONG m_ctid_colid;

	// segmentid column id
	ULONG m_segid_colid;

public:
	CDXLPhysicalSplit(const CDXLPhysicalSplit &) = delete;

	// ctor
	CDXLPhysicalSplit(CMemoryPool *mp,
					  gpos::Ref<ULongPtrArray> delete_colid_array,
					  gpos::Ref<ULongPtrArray> insert_colid_array,
					  ULONG action_colid, ULONG ctid_colid, ULONG segid_colid);

	// dtor
	~CDXLPhysicalSplit() override;

	// operator type
	Edxlopid GetDXLOperator() const override;

	// operator name
	const CWStringConst *GetOpNameStr() const override;

	// deletion column ids
	gpos::Ref<ULongPtrArray>
	GetDeletionColIdArray() const
	{
		return m_deletion_colid_array.get();
	}

	// insertion column ids
	gpos::Ref<ULongPtrArray>
	GetInsertionColIdArray() const
	{
		return m_insert_colid_array.get();
	}

	// action column id
	ULONG
	ActionColId() const
	{
		return m_action_colid;
	}

	// ctid column id
	ULONG
	GetCtIdColId() const
	{
		return m_ctid_colid;
	}

	// segmentid column id
	ULONG
	GetSegmentIdColId() const
	{
		return m_segid_colid;
	}

#ifdef GPOS_DEBUG
	// checks whether the operator has valid structure, i.e. number and
	// types of child nodes
	void AssertValid(const gpos::Ref<CDXLNode> dxlnode,
					 BOOL validate_children) const override;
#endif	// GPOS_DEBUG

	// serialize operator in DXL format
	void SerializeToDXL(CXMLSerializer *xml_serializer,
						const CDXLNode *dxlnode) const override;

	// conversion function
	static gpos::Ref<CDXLPhysicalSplit>
	Cast(gpos::Ref<CDXLOperator> dxl_op)
	{
		GPOS_ASSERT(nullptr != dxl_op);
		GPOS_ASSERT(EdxlopPhysicalSplit == dxl_op->GetDXLOperator());

		return gpos::dyn_cast<CDXLPhysicalSplit>(dxl_op);
	}
};
}  // namespace gpdxl

#endif	// !GPDXL_CDXLPhysicalSplit_H

// EOF
