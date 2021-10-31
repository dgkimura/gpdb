//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CDXLSortGroupClause.h
//
//	@doc:
//		Class for representing sort group clause.
//---------------------------------------------------------------------------



#ifndef GPDXL_CDXLSortGroupClause_H
#define GPDXL_CDXLSortGroupClause_H

#include "gpos/base.h"
#include "gpos/common/CDynamicPtrArray.h"
#include "gpos/common/CRefCount.h"

#include "naucrates/dxl/xml/CXMLSerializer.h"

namespace gpdxl
{
using namespace gpmd;
using namespace gpos;

// fwd decl
class CXMLSerializer;
class CDXLSortGroupClause;

// arrays of column references
typedef CDynamicPtrArray<CDXLSortGroupClause, CleanupRelease>
	CDXLSortGroupClauseArray;

//---------------------------------------------------------------------------
//	@class:
//		CDXLSortGroupClause
//
//	@doc:
//		Class for representing references to columns in DXL trees
//
//---------------------------------------------------------------------------
class CDXLSortGroupClause : public CDXLScalar
{
private:
	INT m_tle_sort_group_ref;
	INT m_eqop;
	INT m_sortop;
	BOOL m_nulls_first;
	BOOL m_hashable;

	// private copy ctor
	CDXLSortGroupClause(const CDXLSortGroupClause &);

public:
	// ctor/dtor
	CDXLSortGroupClause(CMemoryPool *mp, INT tle_sort_group_ref, INT eqop,
						INT sortop, BOOL nulls_first, BOOL hashable)
		: CDXLScalar(mp),
		  m_tle_sort_group_ref(tle_sort_group_ref),
		  m_eqop(eqop),
		  m_sortop(sortop),
		  m_nulls_first(nulls_first),
		  m_hashable(hashable)
	{
	}

	~CDXLSortGroupClause()
	{
	}

	// accessors
	INT
	Index() const
	{
		return m_tle_sort_group_ref;
	}
	INT
	EqOp() const
	{
		return m_eqop;
	}
	INT
	SortOp() const
	{
		return m_sortop;
	}
	BOOL
	NullsFirst() const
	{
		return m_nulls_first;
	}
	BOOL
	IsHashable() const
	{
		return m_hashable;
	}

	static CDXLSortGroupClause *
	Cast(CDXLOperator *dxl_op)
	{
		GPOS_ASSERT(NULL != dxl_op);
		GPOS_ASSERT(EdxlopScalarSortGroupClause == dxl_op->GetDXLOperator());

		return dynamic_cast<CDXLSortGroupClause *>(dxl_op);
	}

	// does the operator return a boolean result
	virtual BOOL
	HasBoolResult(CMDAccessor *) const
	{
		return false;
	}

#ifdef GPOS_DEBUG
	virtual void
	AssertValid(const CDXLNode *, BOOL) const
	{
	}
#endif	// GPOS_DEBUG

	virtual Edxlopid
	GetDXLOperator() const
	{
		return EdxlopScalarSortGroupClause;
	}

	virtual const CWStringConst *
	GetOpNameStr() const
	{
		return CDXLTokens::GetDXLTokenStr(EdxltokenScalarSortGroupClause);
	}
	virtual void
	SerializeToDXL(CXMLSerializer *xml_serializer, const CDXLNode *) const
	{
		const CWStringConst *element_name = GetOpNameStr();

		xml_serializer->OpenElement(
			CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), element_name);

		xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenIndex),
									 Index());
		xml_serializer->AddAttribute(
			CDXLTokens::GetDXLTokenStr(EdxltokenMDTypeEqOp), EqOp());
		xml_serializer->AddAttribute(
			CDXLTokens::GetDXLTokenStr(EdxltokenSortOpId), SortOp());
		xml_serializer->AddAttribute(
			CDXLTokens::GetDXLTokenStr(EdxltokenSortNullsFirst), NullsFirst());
		xml_serializer->AddAttribute(
			CDXLTokens::GetDXLTokenStr(EdxltokenMDTypeHashable), IsHashable());

		xml_serializer->CloseElement(
			CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), element_name);
	}
};
}  // namespace gpdxl



#endif	// !GPDXL_CDXLSortGroupClause_H

// EOF
