//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLLogicalInsert.h
//
//	@doc:
//		Class for representing logical insert operators
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLLogicalInsert_H
#define GPDXL_CDXLLogicalInsert_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "naucrates/dxl/operators/CDXLLogical.h"
#include "naucrates/dxl/operators/CDXLTableDescr.h"

namespace gpdxl
{
using namespace gpmd;

//---------------------------------------------------------------------------
//	@class:
//		CDXLLogicalInsert
//
//	@doc:
//		Class for representing logical insert operators
//
//---------------------------------------------------------------------------
class CDXLLogicalInsert : public CDXLLogical
{
private:
	// target table descriptor
	gpos::owner<CDXLTableDescr *> m_dxl_table_descr;

	// list of source column ids
	gpos::owner<ULongPtrArray *> m_src_colids_array;

public:
	CDXLLogicalInsert(const CDXLLogicalInsert &) = delete;

	// ctor/dtor
	CDXLLogicalInsert(CMemoryPool *mp,
					  gpos::owner<CDXLTableDescr *> table_descr,
					  gpos::owner<ULongPtrArray *> src_colids_array);

	~CDXLLogicalInsert() override;

	// operator type
	Edxlopid GetDXLOperator() const override;

	// operator name
	const CWStringConst *GetOpNameStr() const override;

	// target table descriptor
	gpos::pointer<CDXLTableDescr *>
	GetDXLTableDescr() const
	{
		return m_dxl_table_descr;
	}

	// source column ids
	gpos::pointer<ULongPtrArray *>
	GetSrcColIdsArray() const
	{
		return m_src_colids_array;
	}

#ifdef GPOS_DEBUG
	// checks whether the operator has valid structure, i.e. number and
	// types of child nodes
	void AssertValid(gpos::pointer<const CDXLNode *> node,
					 BOOL validate_children) const override;
#endif	// GPOS_DEBUG

	// serialize operator in DXL format
	void SerializeToDXL(CXMLSerializer *xml_serializer,
						gpos::pointer<const CDXLNode *> node) const override;

	// conversion function
	static gpos::cast_func<CDXLLogicalInsert *>
	Cast(CDXLOperator *dxl_op)
	{
		GPOS_ASSERT(nullptr != dxl_op);
		GPOS_ASSERT(EdxlopLogicalInsert == dxl_op->GetDXLOperator());

		return dynamic_cast<CDXLLogicalInsert *>(dxl_op);
	}
};
}  // namespace gpdxl

#endif	// !GPDXL_CDXLLogicalInsert_H

// EOF
