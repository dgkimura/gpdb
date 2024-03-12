//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (c) 2023 VMware, Inc. or its affiliates. All Rights Reserved.
//
//	@filename:
//		CPlanHint.cpp
//
//	@doc:
//		Container of plan hint objects
//---------------------------------------------------------------------------

#include "gpopt/hints/CPlanHint.h"

#include "gpos/base.h"

#include "gpopt/hints/CHintUtils.h"

using namespace gpopt;

FORCE_GENERATE_DBGSTR(CPlanHint);

//---------------------------------------------------------------------------
//	@function:
//		CPlanHint::CPlanHint
//---------------------------------------------------------------------------
CPlanHint::CPlanHint(CMemoryPool *mp)
	: m_mp(mp),
	  m_scan_hints(GPOS_NEW(mp) ScanHintList(mp)),
	  m_row_hints(GPOS_NEW(mp) RowHintList(mp)),
	  m_join_hints(GPOS_NEW(mp) JoinHintList(mp))
{
}

CPlanHint::~CPlanHint()
{
	m_scan_hints->Release();
	m_row_hints->Release();
	m_join_hints->Release();
}

void
CPlanHint::AddHint(CScanHint *hint)
{
	m_scan_hints->Append(hint);
}

void
CPlanHint::AddHint(CRowHint *hint)
{
	m_row_hints->Append(hint);
}

void
CPlanHint::AddHint(CJoinHint *hint)
{
	m_join_hints->Append(hint);
}


CScanHint *
CPlanHint::GetScanHint(const char *relname)
{
	CWStringConst *name = GPOS_NEW(m_mp) CWStringConst(m_mp, relname);
	CScanHint *hint = GetScanHint(name);
	GPOS_DELETE(name);
	return hint;
}

CScanHint *
CPlanHint::GetScanHint(const CWStringBase *name)
{
	for (ULONG ul = 0; ul < m_scan_hints->Size(); ul++)
	{
		CScanHint *hint = (*m_scan_hints)[ul];
		if (name->Equals(hint->GetName()))
		{
			return hint;
		}
	}
	return nullptr;
}

//---------------------------------------------------------------------------
//	@function:
//		CPlanHint::GetRowHint
//
//	@doc:
//		Given a set of table descriptors, find a matching CRowHint.  A match
//		means that the table alias names used to describe the CRowHint equals
//		the set of table alias names provided by the given set of table
//		descriptors.
//
//---------------------------------------------------------------------------
CRowHint *
CPlanHint::GetRowHint(CTableDescriptorHashSet *ptabdescs)
{
	GPOS_ASSERT(ptabdescs->Size() > 0);

	StringPtrArray *aliases = GPOS_NEW(m_mp) StringPtrArray(m_mp);
	CTableDescriptorHashSetIter hsiter(ptabdescs);

	while (hsiter.Advance())
	{
		const CTableDescriptor *ptabdesc = hsiter.Get();
		aliases->Append(GPOS_NEW(m_mp) CWStringConst(
			m_mp, ptabdesc->Name().Pstr()->GetBuffer()));
	}
	// Row hint aliases are sorted because the hint is order agnostic.
	aliases->Sort(CWStringBase::Compare);

	CRowHint *matching_hint = nullptr;
	for (ULONG ul = 0; ul < m_row_hints->Size(); ul++)
	{
		CRowHint *hint = (*m_row_hints)[ul];
		if (aliases->Equals(hint->GetAliasNames()))
		{
			matching_hint = hint;
			break;
		}
	}
	aliases->Release();
	return matching_hint;
}

//---------------------------------------------------------------------------
//	@function:
//		CPlanHint::GetJoinHint
//
//	@doc:
//		Given a join expression, find a matching hint. A match means that every
//		alias in the hint is covered in the expression and at least one alias
//		is a leaf child of the join. A leaf child means the child has only one
//		relation.
//
//		For example, let's say the expression tree is:
//
//		  +--CLogicalNAryJoin
//		     |--CLogicalGet "t1" ("t1"), Columns: ["a" (0), "b" (1),...
//		     |--CLogicalLimit <empty> global
//		     |  |--CLogicalNAryJoin
//		     |  |  |--CLogicalGet "t2" ("t2"), Columns: ["a" (9), "b" (10),...
//		     |  |  |--CLogicalGet "t3" ("t3"), Columns: ["a" (18), "b" (19),...
//		     |  |  ...
//		     |  ...
//		     ...
//
//		If the hint is Leading((t2 t3)), then the hint does not apply to the
//		top level Nary join because neither "t2" nor "t3" is a leaf child of
//		the Nary join. Instead, the hint should be applied to the lower level
//		Nary join that contains both "t2" and "t3" relations as leaf children.
//
//		However, if the hint is Leading((t1 (t2 t3))), then the hint does apply
//		to the top level Nary join because "t1" is a leaf child.
//---------------------------------------------------------------------------
CJoinHint *
CPlanHint::GetJoinHint(CExpression *pexpr)
{
	if (COperator::EopLogicalNAryJoin != pexpr->Pop()->Eopid() &&
		COperator::EopLogicalInnerJoin != pexpr->Pop()->Eopid())
	{
		return nullptr;
	}

	CTableDescriptorHashSet *ptabdesc = pexpr->DeriveTableDescriptor();
	StringPtrArray *pexprAliases =
		CHintUtils::GetAliasesFromTableDescriptors(m_mp, ptabdesc);

	// If every hint alias is contained in the expression's table descriptor
	// set, and at least one hint alias is a leaf in the expression then the
	// hint is returned.
	for (ULONG ul = 0; ul < m_join_hints->Size(); ul++)
	{
		CJoinHint *hint = (*m_join_hints)[ul];

		StringPtrArray *hintAliases =
			CHintUtils::GetAliasesFromHint(m_mp, hint->GetJoinNode());

		bool is_contained = true;
		for (ULONG j = 0; j < hintAliases->Size(); j++)
		{
			if (nullptr == pexprAliases->Find((*hintAliases)[j]))
			{
				is_contained = false;
				break;
			}
		}
		if (is_contained)
		{
			for (ULONG ul = 0; ul < pexpr->Arity(); ul++)
			{
				CTableDescriptorHashSet *childtabs =
					(*pexpr)[ul]->DeriveTableDescriptor();

				// is a leaf and a hint
				if (childtabs->Size() == 1 &&
					nullptr !=
						hintAliases->Find(childtabs->First()->Name().Pstr()))
				{
					pexprAliases->Release();
					hintAliases->Release();
					return hint;
				}
			}
		}
		hintAliases->Release();
	}
	pexprAliases->Release();
	return nullptr;
}

//---------------------------------------------------------------------------
//	@function:
//		CPlanHint::HasJoinHintWithDirection
//
//	@doc:
//		Given an expression, check if there exists a hint that covers the
//		expression.
//
//---------------------------------------------------------------------------
bool
CPlanHint::HasJoinHintWithDirection(CExpression *pexpr)
{
	CTableDescriptorHashSet *ptabdesc = pexpr->DeriveTableDescriptor();
	StringPtrArray *pexprAliases =
		CHintUtils::GetAliasesFromTableDescriptors(m_mp, ptabdesc);

	bool has_join_with_direction = false;

	// If every alias is contained in the overal table descriptor and at least
	// one alias is a leaf.
	for (ULONG ul = 0; ul < m_join_hints->Size(); ul++)
	{
		CJoinHint *hint = (*m_join_hints)[ul];

		// skip directed-less hints
		if (!hint->GetJoinNode()->IsDirected())
		{
			continue;
		}

		StringPtrArray *hintAliases =
			CHintUtils::GetAliasesFromHint(m_mp, hint->GetJoinNode());

		bool is_contained = true;
		for (ULONG j = 0; j < pexprAliases->Size(); j++)
		{
			if (nullptr == hintAliases->Find((*pexprAliases)[j]))
			{
				is_contained = false;
				break;
			}
		}
		hintAliases->Release();

		if (is_contained)
		{
			has_join_with_direction = true;
			break;
		}
	}

	pexprAliases->Release();
	return has_join_with_direction;
}

IOstream &
CPlanHint::OsPrint(IOstream &os) const
{
	os << "PlanHint: [";
	if (nullptr == m_scan_hints)
	{
		os << "]";
		return os;
	}

	os << "\n";
	for (ULONG ul = 0; ul < m_scan_hints->Size(); ul++)
	{
		os << "  ";
		(*m_scan_hints)[ul]->OsPrint(os) << "\n";
	}

	for (ULONG ul = 0; ul < m_row_hints->Size(); ul++)
	{
		os << "  ";
		(*m_row_hints)[ul]->OsPrint(os) << "\n";
	}

	for (ULONG ul = 0; ul < m_join_hints->Size(); ul++)
	{
		os << "  ";
		(*m_join_hints)[ul]->OsPrint(os) << "\n";
	}
	os << "]";
	return os;
}

void
CPlanHint::Serialize(CXMLSerializer *xml_serializer) const
{
	for (ULONG ul = 0; ul < m_scan_hints->Size(); ul++)
	{
		(*m_scan_hints)[ul]->Serialize(xml_serializer);
	}

	for (ULONG ul = 0; ul < m_row_hints->Size(); ul++)
	{
		(*m_row_hints)[ul]->Serialize(xml_serializer);
	}

	for (ULONG ul = 0; ul < m_join_hints->Size(); ul++)
	{
		(*m_join_hints)[ul]->Serialize(xml_serializer);
	}
}


// EOF
