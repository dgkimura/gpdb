//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp
//
//	@filename:
//		CLogicalConstTableGet.cpp
//
//	@doc:
//		Implementation of const table access
//---------------------------------------------------------------------------

#include "gpopt/operators/CLogicalConstTableGet.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CColRefTable.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/metadata/CName.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "naucrates/statistics/CStatistics.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CLogicalConstTableGet::CLogicalConstTableGet
//
//	@doc:
//		ctor - for pattern
//
//---------------------------------------------------------------------------
CLogicalConstTableGet::CLogicalConstTableGet(CMemoryPool *mp)
	: CLogical(mp),
	  m_pdrgpcoldesc(nullptr),
	  m_pdrgpdrgpdatum(nullptr),
	  m_pdrgpcrOutput(nullptr)
{
	m_fPattern = true;
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalConstTableGet::CLogicalConstTableGet
//
//	@doc:
//		ctor
//
//---------------------------------------------------------------------------
CLogicalConstTableGet::CLogicalConstTableGet(
	CMemoryPool *mp, gpos::Ref<CColumnDescriptorArray> pdrgpcoldesc,
	gpos::Ref<IDatum2dArray> pdrgpdrgpdatum)
	: CLogical(mp),
	  m_pdrgpcoldesc(std::move(pdrgpcoldesc)),
	  m_pdrgpdrgpdatum(std::move(pdrgpdrgpdatum)),
	  m_pdrgpcrOutput(nullptr)
{
	GPOS_ASSERT(nullptr != m_pdrgpcoldesc);
	GPOS_ASSERT(nullptr != m_pdrgpdrgpdatum);

	// generate a default column set for the list of column descriptors
	m_pdrgpcrOutput = PdrgpcrCreateMapping(mp, m_pdrgpcoldesc.get(), UlOpId());

#ifdef GPOS_DEBUG
	for (ULONG ul = 0; ul < m_pdrgpdrgpdatum->Size(); ul++)
	{
		IDatumArray *pdrgpdatum = (*m_pdrgpdrgpdatum)[ul].get();
		GPOS_ASSERT(pdrgpdatum->Size() == m_pdrgpcoldesc->Size());
	}
#endif
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalConstTableGet::CLogicalConstTableGet
//
//	@doc:
//		ctor
//
//---------------------------------------------------------------------------
CLogicalConstTableGet::CLogicalConstTableGet(
	CMemoryPool *mp, gpos::Ref<CColRefArray> pdrgpcrOutput,
	gpos::Ref<IDatum2dArray> pdrgpdrgpdatum)
	: CLogical(mp),
	  m_pdrgpcoldesc(nullptr),
	  m_pdrgpdrgpdatum(std::move(pdrgpdrgpdatum)),
	  m_pdrgpcrOutput(std::move(pdrgpcrOutput))
{
	GPOS_ASSERT(nullptr != m_pdrgpcrOutput);
	GPOS_ASSERT(nullptr != m_pdrgpdrgpdatum);

	// generate column descriptors for the given output columns
	m_pdrgpcoldesc = PdrgpcoldescMapping(mp, m_pdrgpcrOutput.get());

#ifdef GPOS_DEBUG
	for (ULONG ul = 0; ul < m_pdrgpdrgpdatum->Size(); ul++)
	{
		IDatumArray *pdrgpdatum = (*m_pdrgpdrgpdatum)[ul].get();
		GPOS_ASSERT(pdrgpdatum->Size() == m_pdrgpcoldesc->Size());
	}
#endif
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalConstTableGet::~CLogicalConstTableGet
//
//	@doc:
//		dtor
//
//---------------------------------------------------------------------------
CLogicalConstTableGet::~CLogicalConstTableGet()
{
	;
	;
	;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalConstTableGet::HashValue
//
//	@doc:
//		Operator specific hash function
//
//---------------------------------------------------------------------------
ULONG
CLogicalConstTableGet::HashValue() const
{
	ULONG ulHash = gpos::CombineHashes(
		COperator::HashValue(),
		gpos::CombineHashes(
			gpos::HashPtr<CColumnDescriptorArray>(m_pdrgpcoldesc.get()),
			gpos::HashPtr<IDatum2dArray>(m_pdrgpdrgpdatum.get())));
	ulHash = gpos::CombineHashes(ulHash,
								 CUtils::UlHashColArray(m_pdrgpcrOutput.get()));

	return ulHash;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalConstTableGet::Matches
//
//	@doc:
//		Match function on operator level
//
//---------------------------------------------------------------------------
BOOL
CLogicalConstTableGet::Matches(COperator *pop) const
{
	if (pop->Eopid() != Eopid())
	{
		return false;
	}

	CLogicalConstTableGet *popCTG = gpos::dyn_cast<CLogicalConstTableGet>(pop);

	// match if column descriptors, const values and output columns are identical
	return m_pdrgpcoldesc->Equals(popCTG->Pdrgpcoldesc()) &&
		   m_pdrgpdrgpdatum->Equals(popCTG->Pdrgpdrgpdatum()) &&
		   m_pdrgpcrOutput->Equals(popCTG->PdrgpcrOutput());
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalConstTableGet::PopCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the operator with remapped columns
//
//---------------------------------------------------------------------------
gpos::Ref<COperator>
CLogicalConstTableGet::PopCopyWithRemappedColumns(
	CMemoryPool *mp, UlongToColRefMap *colref_mapping, BOOL must_exist)
{
	gpos::Ref<CColRefArray> colref_array = nullptr;
	if (must_exist)
	{
		colref_array = CUtils::PdrgpcrRemapAndCreate(mp, m_pdrgpcrOutput.get(),
													 colref_mapping);
	}
	else
	{
		colref_array = CUtils::PdrgpcrRemap(mp, m_pdrgpcrOutput.get(),
											colref_mapping, must_exist);
	};

	return GPOS_NEW(mp)
		CLogicalConstTableGet(mp, std::move(colref_array), m_pdrgpdrgpdatum);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalConstTableGet::DeriveOutputColumns
//
//	@doc:
//		Derive output columns
//
//---------------------------------------------------------------------------
gpos::Ref<CColRefSet>
CLogicalConstTableGet::DeriveOutputColumns(CMemoryPool *mp,
										   CExpressionHandle &	// exprhdl
)
{
	gpos::Ref<CColRefSet> pcrs = GPOS_NEW(mp) CColRefSet(mp);
	pcrs->Include(m_pdrgpcrOutput.get());

	return pcrs;
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalConstTableGet::DeriveMaxCard
//
//	@doc:
//		Derive max card
//
//---------------------------------------------------------------------------
CMaxCard
CLogicalConstTableGet::DeriveMaxCard(CMemoryPool *,		  // mp
									 CExpressionHandle &  // exprhdl
) const
{
	return CMaxCard(m_pdrgpdrgpdatum->Size());
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalConstTableGet::FInputOrderSensitive
//
//	@doc:
//		Not called for leaf operators
//
//---------------------------------------------------------------------------
BOOL
CLogicalConstTableGet::FInputOrderSensitive() const
{
	GPOS_ASSERT(!"Unexpected function call of FInputOrderSensitive");
	return false;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalConstTableGet::PxfsCandidates
//
//	@doc:
//		Get candidate xforms
//
//---------------------------------------------------------------------------
gpos::Ref<CXformSet>
CLogicalConstTableGet::PxfsCandidates(CMemoryPool *mp) const
{
	gpos::Ref<CXformSet> xform_set = GPOS_NEW(mp) CXformSet(mp);
	(void) xform_set->ExchangeSet(CXform::ExfImplementConstTableGet);
	return xform_set;
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalConstTableGet::PdrgpcoldescMapping
//
//	@doc:
//		Construct column descriptors from column references
//
//---------------------------------------------------------------------------
gpos::Ref<CColumnDescriptorArray>
CLogicalConstTableGet::PdrgpcoldescMapping(CMemoryPool *mp,
										   CColRefArray *colref_array)
{
	GPOS_ASSERT(nullptr != colref_array);
	gpos::Ref<CColumnDescriptorArray> pdrgpcoldesc =
		GPOS_NEW(mp) CColumnDescriptorArray(mp);

	const ULONG length = colref_array->Size();
	for (ULONG ul = 0; ul < length; ul++)
	{
		CColRef *colref = (*colref_array)[ul];

		ULONG length = gpos::ulong_max;
		if (CColRef::EcrtTable == colref->Ecrt())
		{
			CColRefTable *pcrTable = CColRefTable::PcrConvert(colref);
			length = pcrTable->Width();
		}

		gpos::Ref<CColumnDescriptor> pcoldesc = GPOS_NEW(mp) CColumnDescriptor(
			mp, colref->RetrieveType(), colref->TypeModifier(), colref->Name(),
			ul + 1,	 //attno
			true,	 // IsNullable
			length);
		pdrgpcoldesc->Append(pcoldesc);
	}

	return pdrgpcoldesc;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalConstTableGet::PstatsDerive
//
//	@doc:
//		Derive statistics
//
//---------------------------------------------------------------------------
gpos::Ref<IStatistics>
CLogicalConstTableGet::PstatsDerive(CMemoryPool *mp, CExpressionHandle &exprhdl,
									IStatisticsArray *	// not used
) const
{
	GPOS_ASSERT(Esp(exprhdl) > EspNone);
	CReqdPropRelational *prprel =
		gpos::dyn_cast<CReqdPropRelational>(exprhdl.Prp());
	CColRefSet *pcrs = prprel->PcrsStat();
	gpos::Ref<ULongPtrArray> colids = GPOS_NEW(mp) ULongPtrArray(mp);
	pcrs->ExtractColIds(mp, colids.get());
	gpos::Ref<ULongPtrArray> pdrgpulColWidth =
		CUtils::Pdrgpul(mp, m_pdrgpcrOutput.get());

	gpos::Ref<IStatistics> stats = CStatistics::MakeDummyStats(
		mp, colids.get(), pdrgpulColWidth.get(), m_pdrgpdrgpdatum->Size());

	// clean up
	;
	;

	return stats;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalConstTableGet::OsPrint
//
//	@doc:
//		debug print
//
//---------------------------------------------------------------------------
IOstream &
CLogicalConstTableGet::OsPrint(IOstream &os) const
{
	if (m_fPattern)
	{
		return COperator::OsPrint(os);
	}
	else
	{
		os << SzId() << " ";
		os << "Columns: [";
		CUtils::OsPrintDrgPcr(os, m_pdrgpcrOutput.get());
		os << "] ";
		os << "Values: [";
		for (ULONG ulA = 0; ulA < m_pdrgpdrgpdatum->Size(); ulA++)
		{
			if (0 < ulA)
			{
				os << "; ";
			}
			os << "(";
			IDatumArray *pdrgpdatum = (*m_pdrgpdrgpdatum)[ulA].get();

			const ULONG length = pdrgpdatum->Size();
			for (ULONG ulB = 0; ulB < length; ulB++)
			{
				IDatum *datum = (*pdrgpdatum)[ulB].get();
				datum->OsPrint(os);

				if (ulB < length - 1)
				{
					os << ", ";
				}
			}
			os << ")";
		}
		os << "]";
	}

	return os;
}



// EOF
