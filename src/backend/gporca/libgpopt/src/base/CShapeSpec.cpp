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

using namespace gpopt;
using namespace gpmd;


//---------------------------------------------------------------------------
//	@function:
//		CShapeSpec::CShapeSpec
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CShapeSpec::CShapeSpec(CMemoryPool *mp)
	: m_mp(mp), m_pdrgpScanHint(nullptr), m_pdrgpJoinHint(nullptr)
{
	m_pdrgpScanHint = GPOS_NEW(mp) CPlanHintArray(mp);
	m_pdrgpJoinHint = GPOS_NEW(mp) CPlanHintArray(mp);
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
	m_pdrgpScanHint->Release();
	m_pdrgpJoinHint->Release();
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
CShapeSpec::Append(CPlanHint *_pph)
{
	CScanHint *pph = dynamic_cast<CScanHint *>(_pph);
	if (nullptr != pph)
	{
		_pph->AddRef();
		m_pdrgpScanHint->Append(_pph);
	}
	else
	{
		// XXX: FIXME: TODO guard against bad intput types?
		_pph->AddRef();
		m_pdrgpJoinHint->Append(_pph);
	}
}
void
CShapeSpec::Append(CShapeSpec *pss)
{
	for (ULONG ul = 0; ul < pss->m_pdrgpScanHint->Size(); ul++)
	{
		CPlanHint *pph = (*pss->m_pdrgpScanHint)[ul];
		Append(pph);
	}
	for (ULONG ul = 0; ul < pss->m_pdrgpJoinHint->Size(); ul++)
	{
		CPlanHint *pph = (*pss->m_pdrgpJoinHint)[ul];
		Append(pph);
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
	BOOL fMatch = m_pdrgpScanHint->Size() == pos->m_pdrgpScanHint->Size() &&
				  m_pdrgpJoinHint->Size() == pos->m_pdrgpJoinHint->Size() &&
				  FSatisfies(pos);

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
	ULONG arity = pos->m_pdrgpScanHint->Size();
	BOOL fSatisfies = (m_pdrgpScanHint->Size() >= arity);

	for (ULONG ul = 0; fSatisfies && ul < arity; ul++)
	{
		fSatisfies =
			(*m_pdrgpScanHint)[ul]->Matches((*(pos->m_pdrgpScanHint))[ul]);
	}

	fSatisfies =
		fSatisfies && (m_pdrgpJoinHint->Size() >= pos->m_pdrgpJoinHint->Size());
	for (ULONG ul = 0; fSatisfies && ul < pos->m_pdrgpJoinHint->Size(); ul++)
	{
		fSatisfies =
			(*m_pdrgpJoinHint)[ul]->Matches((*(pos->m_pdrgpJoinHint))[ul]);
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

	for (ULONG ul = 0; ul < m_pdrgpScanHint->Size(); ul++)
	{
		CPlanHint *pse = (*m_pdrgpScanHint)[ul];
		ulHash = gpos::CombineHashes(ulHash, pse->HashValue());
	}
	for (ULONG ul = 0; ul < m_pdrgpJoinHint->Size(); ul++)
	{
		CPlanHint *pse = (*m_pdrgpScanHint)[ul];
		ulHash = gpos::CombineHashes(ulHash, pse->HashValue());
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
	const ULONG arity1 = m_pdrgpScanHint->Size();
	const ULONG arity2 = m_pdrgpJoinHint->Size();
	if (0 == arity1 && 0 == arity2)
	{
		os << "<empty>";
	}
	else
	{
		os << "\nScanHints: ";
		for (ULONG ul = 0; ul < arity1; ul++)
		{
			(*m_pdrgpScanHint)[ul]->OsPrint(os) << " ";
		}

		os << "\nJoinHints: ";
		for (ULONG ul = 0; ul < arity2; ul++)
		{
			(*m_pdrgpJoinHint)[ul]->OsPrint(os) << " ";
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
