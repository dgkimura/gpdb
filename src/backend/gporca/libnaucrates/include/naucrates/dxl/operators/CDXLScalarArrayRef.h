//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 VMware, Inc. or its affiliates.
//
//	@filename:
//		CDXLScalarArrayRef.h
//
//	@doc:
//		Class for representing DXL scalar arrayrefs
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLScalarArrayRef_H
#define GPDXL_CDXLScalarArrayRef_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "naucrates/dxl/operators/CDXLScalar.h"
#include "naucrates/md/IMDId.h"

namespace gpdxl
{
using namespace gpmd;

//---------------------------------------------------------------------------
//	@class:
//		CDXLScalarArrayRef
//
//	@doc:
//		Class for representing DXL scalar arrayrefs
//
//---------------------------------------------------------------------------
class CDXLScalarArrayRef : public CDXLScalar
{
private:
	// base element type id
	gpos::owner<IMDId *> m_elem_type_mdid;

	// element type modifier
	INT m_type_modifier;

	// array type id
	gpos::owner<IMDId *> m_array_type_mdid;

	// return type id
	gpos::owner<IMDId *> m_return_type_mdid;

public:
	CDXLScalarArrayRef(const CDXLScalarArrayRef &) = delete;

	// ctor
	CDXLScalarArrayRef(CMemoryPool *mp, gpos::owner<IMDId *> elem_type_mdid,
					   INT type_modifier, gpos::owner<IMDId *> array_type_mdid,
					   gpos::owner<IMDId *> return_type_mdid);

	// dtor
	~CDXLScalarArrayRef() override;

	// ident accessors
	Edxlopid GetDXLOperator() const override;

	// operator name
	const CWStringConst *GetOpNameStr() const override;

	// element type id
	gpos::pointer<IMDId *>
	ElementTypeMDid() const
	{
		return m_elem_type_mdid;
	}

	// element type modifier
	INT TypeModifier() const;

	// array type id
	gpos::pointer<IMDId *>
	ArrayTypeMDid() const
	{
		return m_array_type_mdid;
	}

	// return type id
	gpos::pointer<IMDId *>
	ReturnTypeMDid() const
	{
		return m_return_type_mdid;
	}

	// serialize operator in DXL format
	void SerializeToDXL(CXMLSerializer *xml_serializer,
						gpos::pointer<const CDXLNode *> dxlnode) const override;

	// does the operator return a boolean result
	BOOL HasBoolResult(CMDAccessor *md_accessor) const override;

#ifdef GPOS_DEBUG
	// checks whether the operator has valid structure, i.e. number and
	// types of child nodes
	void AssertValid(gpos::pointer<const CDXLNode *> dxlnode,
					 BOOL validate_children) const override;
#endif	// GPOS_DEBUG

	// conversion function
	static gpos::cast_func<CDXLScalarArrayRef *>
	Cast(CDXLOperator *dxl_op)
	{
		GPOS_ASSERT(nullptr != dxl_op);
		GPOS_ASSERT(EdxlopScalarArrayRef == dxl_op->GetDXLOperator());

		return dynamic_cast<CDXLScalarArrayRef *>(dxl_op);
	}
};
}  // namespace gpdxl

#endif	// !GPDXL_CDXLScalarArrayRef_H

// EOF
