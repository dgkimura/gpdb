//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CShapeSpec.cpp
//
//	@doc:
//		Specification of order property
//---------------------------------------------------------------------------

#include "gpopt/base/CShapeSpec.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/operators/CPhysicalSort.h"

#ifdef GPOS_DEBUG
#include "gpos/error/CAutoTrace.h"
#endif	// GPOS_DEBUG

using namespace gpopt;
using namespace gpmd;


//---------------------------------------------------------------------------
//	@function:
//		CShapeSpec::CShapeExpression::CShapeExpression
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CShapeSpec::CShapeExpression::CShapeExpression(ULONG shape) : m_shape(shape)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CShapeSpec::CShapeExpression::~CShapeExpression
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CShapeSpec::CShapeExpression::~CShapeExpression()
{
}

//---------------------------------------------------------------------------
//	@function:
//		CShapeSpec::CShapeExpression::Matches
//
//	@doc:
//		Check if order expression equal to given one;
//
//---------------------------------------------------------------------------
BOOL
CShapeSpec::CShapeExpression::Matches(const CShapeExpression *pse) const
{
	GPOS_ASSERT(nullptr != pse);

	return pse->m_shape == m_shape;
}


FORCE_GENERATE_DBGSTR(gpopt::CShapeSpec::CShapeExpression);

//---------------------------------------------------------------------------
//	@function:
//		CShapeSpec::CShapeExpression::OsPrint
//
//	@doc:
//		Print order expression
//
//---------------------------------------------------------------------------
IOstream &
CShapeSpec::CShapeExpression::OsPrint(IOstream &os) const
{
	os << "( ";
	os << m_shape;
	os << " )";

	return os;
}


//---------------------------------------------------------------------------
//	@function:
//		CShapeSpec::CShapeSpec
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CShapeSpec::CShapeSpec(CMemoryPool *mp) : m_mp(mp), m_pdrgpoe(nullptr)
{
	m_pdrgpoe = GPOS_NEW(mp) CShapeExpressionArray(mp);
}


//---------------------------------------------------------------------------
//	@function:
//		CShapeSpec::~CShapeSpec
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CShapeSpec::~CShapeSpec()
{
	m_pdrgpoe->Release();
}


//---------------------------------------------------------------------------
//	@function:
//		CShapeSpec::Append
//
//	@doc:
//		Append order expression;
//
//---------------------------------------------------------------------------
void
CShapeSpec::Append(ULONG shape)
{
	CShapeExpression *poe = GPOS_NEW(m_mp) CShapeExpression(shape);
	m_pdrgpoe->Append(poe);
}
void
CShapeSpec::Append(CShapeSpec *pss)
{
	for (ULONG ul = 0; ul < pss->m_pdrgpoe->Size(); ul++)
	{
		CShapeExpression *pse = (*pss->m_pdrgpoe)[ul];
		Append(pse->GetShape());
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CShapeSpec::Matches
//
//	@doc:
//		Check for equality between order specs
//
//---------------------------------------------------------------------------
BOOL
CShapeSpec::Matches(const CShapeSpec *pos) const
{
	BOOL fMatch =
		m_pdrgpoe->Size() == pos->m_pdrgpoe->Size() && FSatisfies(pos);

	GPOS_ASSERT_IMP(fMatch, pos->FSatisfies(this));

	return fMatch;
}


//---------------------------------------------------------------------------
//	@function:
//		CShapeSpec::FSatisfies
//
//	@doc:
//		Check if this order spec satisfies the given one
//
//---------------------------------------------------------------------------
BOOL
CShapeSpec::FSatisfies(const CShapeSpec *pos) const
{
	const ULONG arity = pos->m_pdrgpoe->Size();
	BOOL fSatisfies = (m_pdrgpoe->Size() >= arity);

	for (ULONG ul = 0; fSatisfies && ul < arity; ul++)
	{
		fSatisfies = (*m_pdrgpoe)[ul]->Matches((*(pos->m_pdrgpoe))[ul]);
	}

	return fSatisfies;
}


//---------------------------------------------------------------------------
//	@function:
//		CShapeSpec::AppendEnforcers
//
//	@doc:
//		Add required enforcers enforcers to dynamic array
//
//---------------------------------------------------------------------------
void
CShapeSpec::AppendEnforcers(CMemoryPool *mp GPOS_UNUSED,
							CExpressionHandle &,  // exprhdl
							CReqdPropPlan *
#ifdef GPOS_DEBUG
								prpp GPOS_UNUSED
#endif	// GPOS_DEBUG
							,
							CExpressionArray *pdrgpexpr GPOS_UNUSED,
							CExpression *pexpr GPOS_UNUSED)
{
	// XXX: Nothing to do. Plan shape doesn't match...

	//GPOS_ASSERT(nullptr != prpp);
	//GPOS_ASSERT(nullptr != mp);
	//GPOS_ASSERT(nullptr != pdrgpexpr);
	//GPOS_ASSERT(nullptr != pexpr);
	//GPOS_ASSERT(this == prpp->Peo()->PssRequired() &&
	//			"required plan properties don't match enforced order spec");

	//AddRef();
	//pexpr->AddRef();
	//CExpression *pexprSort = GPOS_NEW(mp)
	//	CExpression(mp, GPOS_NEW(mp) CPhysicalSort(mp, this), pexpr);
	//pdrgpexpr->Append(pexprSort);
}


//---------------------------------------------------------------------------
//	@function:
//		CShapeSpec::HashValue
//
//	@doc:
//		Hash of components
//
//---------------------------------------------------------------------------
ULONG
CShapeSpec::HashValue() const
{
	ULONG ulHash = 0;
	ULONG arity = m_pdrgpoe->Size();

	for (ULONG ul = 0; ul < arity; ul++)
	{
		CShapeExpression *pse = (*m_pdrgpoe)[ul];
		ulHash = gpos::CombineHashes(ulHash, pse->GetShape());
	}

	return ulHash;
}


//---------------------------------------------------------------------------
//	@function:
//		CShapeSpec::PcrsUsed
//
//	@doc:
//		Extract colref set from order components
//
//---------------------------------------------------------------------------
CColRefSet *
CShapeSpec::PcrsUsed(CMemoryPool *mp) const
{
	CColRefSet *pcrs = GPOS_NEW(mp) CColRefSet(mp);

	return pcrs;
}


//---------------------------------------------------------------------------
//	@function:
//		CShapeSpec::OsPrint
//
//	@doc:
//		Print order spec
//
//---------------------------------------------------------------------------
IOstream &
CShapeSpec::OsPrint(IOstream &os) const
{
	const ULONG arity = m_pdrgpoe->Size();
	if (0 == arity)
	{
		os << "<empty>";
	}
	else
	{
		for (ULONG ul = 0; ul < arity; ul++)
		{
			(*m_pdrgpoe)[ul]->OsPrint(os) << " ";
		}
	}

	return os;
}


//---------------------------------------------------------------------------
//	@function:
//		CShapeSpec::Equals
//
//	@doc:
//		 Matching function over order spec arrays
//
//---------------------------------------------------------------------------
BOOL
CShapeSpec::Equals(const CShapeSpecArray *pdrgposFirst,
				   const CShapeSpecArray *pdrgposSecond)
{
	if (nullptr == pdrgposFirst || nullptr == pdrgposSecond)
	{
		return (nullptr == pdrgposFirst && nullptr == pdrgposSecond);
	}

	if (pdrgposFirst->Size() != pdrgposSecond->Size())
	{
		return false;
	}

	const ULONG size = pdrgposFirst->Size();
	BOOL fMatch = true;
	for (ULONG ul = 0; fMatch && ul < size; ul++)
	{
		fMatch = (*pdrgposFirst)[ul]->Matches((*pdrgposSecond)[ul]);
	}

	return fMatch;
}


//---------------------------------------------------------------------------
//	@function:
//		CShapeSpec::HashValue
//
//	@doc:
//		 Combine hash values of a maximum number of entries
//
//---------------------------------------------------------------------------
ULONG
CShapeSpec::HashValue(const CShapeSpecArray *pdrgpos, ULONG ulMaxSize)
{
	GPOS_ASSERT(nullptr != pdrgpos);
	ULONG size = std::min(ulMaxSize, pdrgpos->Size());

	ULONG ulHash = 0;
	for (ULONG ul = 0; ul < size; ul++)
	{
		ulHash = gpos::CombineHashes(ulHash, (*pdrgpos)[ul]->HashValue());
	}

	return ulHash;
}


//---------------------------------------------------------------------------
//	@function:
//		CShapeSpec::OsPrint
//
//	@doc:
//		 Print array of order spec objects
//
//---------------------------------------------------------------------------
IOstream &
CShapeSpec::OsPrint(IOstream &os, const CShapeSpecArray *pdrgpos)
{
	const ULONG size = pdrgpos->Size();
	os << "[";
	if (0 < size)
	{
		for (ULONG ul = 0; ul < size - 1; ul++)
		{
			(void) (*pdrgpos)[ul]->OsPrint(os);
			os << ", ";
		}

		(void) (*pdrgpos)[size - 1]->OsPrint(os);
	}

	return os << "]";
}


// EOF
