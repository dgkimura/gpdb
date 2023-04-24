//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLLogicalUpdate.h
//
//	@doc:
//		Class for representing logical update operator
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLLogicalUpdate_H
#define GPDXL_CDXLLogicalUpdate_H

#include "gpos/base.h"
#include "gpos/common/Casting.h"

#include "naucrates/dxl/operators/CDXLLogical.h"
#include "naucrates/dxl/operators/CDXLTableDescr.h"
namespace gpdxl
{
using namespace gpmd;

//---------------------------------------------------------------------------
//	@class:
//		CDXLLogicalUpdate
//
//	@doc:
//		Class for representing logical update operator
//
//---------------------------------------------------------------------------
class CDXLLogicalUpdate : public CDXLLogical
{
private:
	// target table descriptor
	gpos::Ref<CDXLTableDescr> m_dxl_table_descr;

	// ctid column id
	ULONG m_ctid_colid;

	// segmentId column id
	ULONG m_segid_colid;

	// list of deletion column ids
	gpos::Ref<ULongPtrArray> m_deletion_colid_array;

	// list of insertion column ids
	gpos::Ref<ULongPtrArray> m_insert_colid_array;

public:
	CDXLLogicalUpdate(const CDXLLogicalUpdate &) = delete;

	// ctor
	CDXLLogicalUpdate(CMemoryPool *mp, gpos::Ref<CDXLTableDescr> table_descr,
					  ULONG ctid_colid, ULONG segid_colid,
					  gpos::Ref<ULongPtrArray> delete_colid_array,
					  gpos::Ref<ULongPtrArray> insert_colid_array);

	// dtor
	~CDXLLogicalUpdate() override;

	// operator type
	Edxlopid GetDXLOperator() const override;

	// operator name
	const CWStringConst *GetOpNameStr() const override;

	// target table descriptor
	gpos::Ref<CDXLTableDescr>
	GetDXLTableDescr() const
	{
		return m_dxl_table_descr;
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

	// deletion column ids
	gpos::Ref<ULongPtrArray>
	GetDeletionColIdArray() const
	{
		return m_deletion_colid_array;
	}

	// insertion column ids
	gpos::Ref<ULongPtrArray>
	GetInsertionColIdArray() const
	{
		return m_insert_colid_array;
	}

#ifdef GPOS_DEBUG
	// checks whether the operator has valid structure, i.e. number and
	// types of child nodes
	void AssertValid(const gpos::Ref<CDXLNode> node,
					 BOOL validate_children) const override;
#endif	// GPOS_DEBUG

	// serialize operator in DXL format
	void SerializeToDXL(CXMLSerializer *xml_serializer,
						const gpos::Ref<CDXLNode> dxlnode) const override;

	// conversion function
	static gpos::Ref<CDXLLogicalUpdate>
	Cast(gpos::Ref<CDXLOperator> dxl_op)
	{
		GPOS_ASSERT(nullptr != dxl_op);
		GPOS_ASSERT(EdxlopLogicalUpdate == dxl_op->GetDXLOperator());

		return gpos::dyn_cast<CDXLLogicalUpdate>(dxl_op);
	}
};
}  // namespace gpdxl

#endif	// !GPDXL_CDXLLogicalUpdate_H

// EOF
