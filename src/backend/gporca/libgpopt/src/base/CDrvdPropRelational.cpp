//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2009 - 2011 EMC CORP.
//
//	@filename:
//		CDrvdPropRelational.cpp
//
//	@doc:
//		Relational derived properties;
//---------------------------------------------------------------------------

#include "gpopt/base/CDrvdPropRelational.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"
#include "gpos/task/CAutoSuspendAbort.h"
#include "gpos/task/CWorker.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CDrvdPropCtxtPlan.h"
#include "gpopt/base/CKeyCollection.h"
#include "gpopt/base/CPartInfo.h"
#include "gpopt/base/CReqdPropPlan.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CLogical.h"
#include "gpopt/operators/CLogicalDynamicGet.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CDrvdPropRelational::CDrvdPropRelational
//
//	@doc:
//		ctor
//
//---------------------------------------------------------------------------
CDrvdPropRelational::CDrvdPropRelational(CMemoryPool *mp)
	: m_mp(mp),
	  m_is_prop_derived(nullptr),
	  m_pcrsOutput(nullptr),
	  m_pcrsOuter(nullptr),
	  m_pcrsNotNull(nullptr),
	  m_pcrsCorrelatedApply(nullptr),
	  m_pkc(nullptr),
	  m_pdrgpfd(nullptr),
	  m_ulJoinDepth(0),
	  m_ppartinfo(nullptr),
	  m_ppc(nullptr),
	  m_pfp(nullptr),
	  m_is_complete(false)
{
	m_is_prop_derived = GPOS_NEW(mp) CBitSet(mp, EdptSentinel);
}


//---------------------------------------------------------------------------
//	@function:
//		CDrvdPropRelational::~CDrvdPropRelational
//
//	@doc:
//		ctor
//
//---------------------------------------------------------------------------
CDrvdPropRelational::~CDrvdPropRelational()
{
	{
		CAutoSuspendAbort asa;

		;
		;
		;
		;
		;
		;
		;
		;
		;
		;
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CDrvdPropRelational::Derive
//
//	@doc:
//		Derive relational props. This derives ALL properties
//
//---------------------------------------------------------------------------
void
CDrvdPropRelational::Derive(CMemoryPool *,	//mp,
							CExpressionHandle &exprhdl,
							gpos::Ref<CDrvdPropCtxt>  // pdpctxt
)
{
	GPOS_CHECK_ABORT;

	// call output derivation function on the operator
	DeriveOutputColumns(exprhdl);

	// derive outer-references
	DeriveOuterReferences(exprhdl);

	// derive not null columns
	DeriveNotNullColumns(exprhdl);

	// derive correlated apply columns
	DeriveCorrelatedApplyColumns(exprhdl);

	// derive constraint
	DerivePropertyConstraint(exprhdl);

	// compute max card
	DeriveMaxCard(exprhdl);

	// derive keys
	DeriveKeyCollection(exprhdl);

	// derive join depth
	DeriveJoinDepth(exprhdl);

	// derive function properties
	DeriveFunctionProperties(exprhdl);

	// derive functional dependencies
	DeriveFunctionalDependencies(exprhdl);

	// derive partition consumers
	DerivePartitionInfo(exprhdl);
	GPOS_ASSERT(nullptr != m_ppartinfo);

	DeriveTableDescriptor(exprhdl);

	m_is_complete = true;
}

//---------------------------------------------------------------------------
//	@function:
//		CDrvdPropRelational::FSatisfies
//
//	@doc:
//		Check for satisfying required properties
//
//---------------------------------------------------------------------------
BOOL
CDrvdPropRelational::FSatisfies(const gpos::Ref<CReqdPropPlan> prpp) const
{
	GPOS_ASSERT(nullptr != prpp);
	GPOS_ASSERT(nullptr != prpp->PcrsRequired());

	BOOL fSatisfies = GetOutputColumns()->ContainsAll(prpp->PcrsRequired());

	return fSatisfies;
}


//---------------------------------------------------------------------------
//	@function:
//		CDrvdPropRelational::GetRelationalProperties
//
//	@doc:
//		Short hand for conversion
//
//---------------------------------------------------------------------------
gpos::Ref<CDrvdPropRelational>
CDrvdPropRelational::GetRelationalProperties(gpos::Ref<CDrvdProp> pdp)
{
	GPOS_ASSERT(nullptr != pdp);
	GPOS_ASSERT(EptRelational == pdp->Ept() &&
				"This is not a relational properties container");
	return gpos::dyn_cast<CDrvdPropRelational>(pdp);
}


//---------------------------------------------------------------------------
//	@function:
//		CDrvdPropRelational::PdrgpfdChild
//
//	@doc:
//		Helper for getting applicable FDs from child
//
//---------------------------------------------------------------------------
gpos::Ref<CFunctionalDependencyArray>
CDrvdPropRelational::DeriveChildFunctionalDependencies(
	CMemoryPool *mp, ULONG child_index, CExpressionHandle &exprhdl)
{
	GPOS_ASSERT(child_index < exprhdl.Arity());
	GPOS_ASSERT(!exprhdl.FScalarChild(child_index));

	// get FD's of the child
	CFunctionalDependencyArray *pdrgpfdChild = exprhdl.Pdrgpfd(child_index);

	// get output columns of the parent
	gpos::Ref<CColRefSet> pcrsOutput = exprhdl.DeriveOutputColumns();

	// collect child FD's that are applicable to the parent
	gpos::Ref<CFunctionalDependencyArray> pdrgpfd =
		GPOS_NEW(mp) CFunctionalDependencyArray(mp);
	const ULONG size = pdrgpfdChild->Size();
	for (ULONG ul = 0; ul < size; ul++)
	{
		gpos::Ref<CFunctionalDependency> pfd = (*pdrgpfdChild)[ul].get();

		// check applicability of FD's LHS
		if (pcrsOutput->ContainsAll(pfd->PcrsKey()))
		{
			// decompose FD's RHS to extract the applicable part
			gpos::Ref<CColRefSet> pcrsDetermined = GPOS_NEW(mp) CColRefSet(mp);
			pcrsDetermined->Include(pfd->PcrsDetermined());
			pcrsDetermined->Intersection(pcrsOutput);
			if (0 < pcrsDetermined->Size())
			{
				// create a new FD and add it to the output array
				;
				;
				gpos::Ref<CFunctionalDependency> pfdNew = GPOS_NEW(mp)
					CFunctionalDependency(pfd->PcrsKey(), pcrsDetermined);
				pdrgpfd->Append(pfdNew);
			};
		}
	}

	return pdrgpfd;
}


//---------------------------------------------------------------------------
//	@function:
//		CDrvdPropRelational::PdrgpfdLocal
//
//	@doc:
//		Helper for deriving local FDs
//
//---------------------------------------------------------------------------
gpos::Ref<CFunctionalDependencyArray>
CDrvdPropRelational::DeriveLocalFunctionalDependencies(
	CMemoryPool *mp, CExpressionHandle &exprhdl)
{
	gpos::Ref<CFunctionalDependencyArray> pdrgpfd =
		GPOS_NEW(mp) CFunctionalDependencyArray(mp);

	// get local key
	gpos::Ref<CKeyCollection> pkc = exprhdl.DeriveKeyCollection();

	if (nullptr == pkc)
	{
		return pdrgpfd;
	}

	ULONG ulKeys = pkc->Keys();
	for (ULONG ul = 0; ul < ulKeys; ul++)
	{
		gpos::Ref<CColRefArray> pdrgpcrKey = pkc->PdrgpcrKey(mp, ul);
		gpos::Ref<CColRefSet> pcrsKey = GPOS_NEW(mp) CColRefSet(mp);
		pcrsKey->Include(pdrgpcrKey.get());

		// get output columns
		gpos::Ref<CColRefSet> pcrsOutput = exprhdl.DeriveOutputColumns();
		gpos::Ref<CColRefSet> pcrsDetermined = GPOS_NEW(mp) CColRefSet(mp);
		pcrsDetermined->Include(pcrsOutput);
		pcrsDetermined->Exclude(pcrsKey.get());

		if (0 < pcrsDetermined->Size())
		{
			// add FD between key and the rest of output columns
			;
			;
			gpos::Ref<CFunctionalDependency> pfdLocal =
				GPOS_NEW(mp) CFunctionalDependency(pcrsKey, pcrsDetermined);
			pdrgpfd->Append(pfdLocal);
		}

		;
		;
		;
	}

	return pdrgpfd;
}


//---------------------------------------------------------------------------
//	@function:
//		CDrvdPropRelational::OsPrint
//
//	@doc:
//		Debug print
//
//---------------------------------------------------------------------------
IOstream &
CDrvdPropRelational::OsPrint(IOstream &os) const
{
	os << "Output Cols: [" << *GetOutputColumns() << "]"
	   << ", Outer Refs: [" << *GetOuterReferences() << "]"
	   << ", Not Null Cols: [" << *GetNotNullColumns() << "]"
	   << ", Corr. Apply Cols: [" << *GetCorrelatedApplyColumns() << "]";

	if (nullptr == GetKeyCollection())
	{
		os << ", Keys: []";
	}
	else
	{
		os << ", " << *GetKeyCollection();
	}

	os << ", Max Card: " << GetMaxCard();

	os << ", Join Depth: " << GetJoinDepth();

	os << ", Constraint Property: [" << *GetPropertyConstraint() << "]";

	const ULONG ulFDs = GetFunctionalDependencies()->Size();

	os << ", FDs: [";
	for (ULONG ul = 0; ul < ulFDs; ul++)
	{
		gpos::Ref<CFunctionalDependency> pfd =
			(*GetFunctionalDependencies())[ul].get();
		os << *pfd;
	}
	os << "]";

	os << ", Function Properties: [" << *GetFunctionProperties() << "]";

	os << ", Part Info: [" << *GetPartitionInfo() << "]";

	return os;
}

// output columns
gpos::Ref<CColRefSet>
CDrvdPropRelational::GetOutputColumns() const
{
	GPOS_RTL_ASSERT(IsComplete());
	return m_pcrsOutput.get();
}

// output columns
gpos::Ref<CColRefSet>
CDrvdPropRelational::DeriveOutputColumns(CExpressionHandle &exprhdl)
{
	if (!m_is_prop_derived->ExchangeSet(EdptPcrsOutput))
	{
		gpos::Ref<CLogical> popLogical =
			gpos::dyn_cast<CLogical>(exprhdl.Pop());
		m_pcrsOutput = popLogical->DeriveOutputColumns(m_mp, exprhdl);
	}

	return m_pcrsOutput.get();
}

// outer references
gpos::Ref<CColRefSet>
CDrvdPropRelational::GetOuterReferences() const
{
	GPOS_RTL_ASSERT(IsComplete());
	return m_pcrsOuter.get();
}

// outer references
gpos::Ref<CColRefSet>
CDrvdPropRelational::DeriveOuterReferences(CExpressionHandle &exprhdl)
{
	if (!m_is_prop_derived->ExchangeSet(EdptPcrsOuter))
	{
		gpos::Ref<CLogical> popLogical =
			gpos::dyn_cast<CLogical>(exprhdl.Pop());
		m_pcrsOuter = popLogical->DeriveOuterReferences(m_mp, exprhdl);
	}

	return m_pcrsOuter.get();
}

// nullable columns
gpos::Ref<CColRefSet>
CDrvdPropRelational::GetNotNullColumns() const
{
	GPOS_RTL_ASSERT(IsComplete());
	return m_pcrsNotNull.get();
}

gpos::Ref<CColRefSet>
CDrvdPropRelational::DeriveNotNullColumns(CExpressionHandle &exprhdl)
{
	if (!m_is_prop_derived->ExchangeSet(EdptPcrsNotNull))
	{
		gpos::Ref<CLogical> popLogical =
			gpos::dyn_cast<CLogical>(exprhdl.Pop());
		m_pcrsNotNull = popLogical->DeriveNotNullColumns(m_mp, exprhdl);
	}

	return m_pcrsNotNull.get();
}

// columns from the inner child of a correlated-apply expression that can be used above the apply expression
gpos::Ref<CColRefSet>
CDrvdPropRelational::GetCorrelatedApplyColumns() const
{
	GPOS_RTL_ASSERT(IsComplete());
	return m_pcrsCorrelatedApply.get();
}

gpos::Ref<CColRefSet>
CDrvdPropRelational::DeriveCorrelatedApplyColumns(CExpressionHandle &exprhdl)
{
	if (!m_is_prop_derived->ExchangeSet(EdptPcrsCorrelatedApply))
	{
		gpos::Ref<CLogical> popLogical =
			gpos::dyn_cast<CLogical>(exprhdl.Pop());
		m_pcrsCorrelatedApply =
			popLogical->DeriveCorrelatedApplyColumns(m_mp, exprhdl);
	}

	return m_pcrsCorrelatedApply.get();
}

// key collection
gpos::Ref<CKeyCollection>
CDrvdPropRelational::GetKeyCollection() const
{
	GPOS_RTL_ASSERT(IsComplete());
	return m_pkc.get();
}

gpos::Ref<CKeyCollection>
CDrvdPropRelational::DeriveKeyCollection(CExpressionHandle &exprhdl)
{
	if (!m_is_prop_derived->ExchangeSet(EdptPkc))
	{
		gpos::Ref<CLogical> popLogical =
			gpos::dyn_cast<CLogical>(exprhdl.Pop());
		m_pkc = popLogical->DeriveKeyCollection(m_mp, exprhdl);

		if (nullptr == m_pkc && 1 == DeriveMaxCard(exprhdl))
		{
			m_pcrsOutput = DeriveOutputColumns(exprhdl);

			if (0 < m_pcrsOutput->Size())
			{
				;
				m_pkc = GPOS_NEW(m_mp) CKeyCollection(m_mp, m_pcrsOutput);
			}
		}
	}

	return m_pkc.get();
}

// functional dependencies
CFunctionalDependencyArray *
CDrvdPropRelational::GetFunctionalDependencies() const
{
	GPOS_RTL_ASSERT(IsComplete());
	return m_pdrgpfd.get();
}

CFunctionalDependencyArray *
CDrvdPropRelational::DeriveFunctionalDependencies(CExpressionHandle &exprhdl)
{
	if (!m_is_prop_derived->ExchangeSet(EdptPdrgpfd))
	{
		gpos::Ref<CFunctionalDependencyArray> pdrgpfd =
			GPOS_NEW(m_mp) CFunctionalDependencyArray(m_mp);
		const ULONG arity = exprhdl.Arity();

		// collect applicable FD's from logical children
		for (ULONG ul = 0; ul < arity; ul++)
		{
			if (!exprhdl.FScalarChild(ul))
			{
				gpos::Ref<CFunctionalDependencyArray> pdrgpfdChild =
					DeriveChildFunctionalDependencies(m_mp, ul, exprhdl);
				CUtils::AddRefAppend(pdrgpfd.get(), pdrgpfdChild.get());
				;
			}
		}
		// add local FD's
		gpos::Ref<CFunctionalDependencyArray> pdrgpfdLocal =
			DeriveLocalFunctionalDependencies(m_mp, exprhdl);
		CUtils::AddRefAppend(pdrgpfd.get(), pdrgpfdLocal.get());
		;

		m_pdrgpfd = pdrgpfd;
	}

	return m_pdrgpfd.get();
}

// max cardinality
CMaxCard
CDrvdPropRelational::GetMaxCard() const
{
	GPOS_RTL_ASSERT(IsComplete());
	return m_maxcard;
}

CMaxCard
CDrvdPropRelational::DeriveMaxCard(CExpressionHandle &exprhdl)
{
	if (!m_is_prop_derived->ExchangeSet(EdptMaxCard))
	{
		gpos::Ref<CLogical> popLogical =
			gpos::dyn_cast<CLogical>(exprhdl.Pop());
		m_maxcard = popLogical->DeriveMaxCard(m_mp, exprhdl);
	}

	return m_maxcard;
}

// join depth
ULONG
CDrvdPropRelational::GetJoinDepth() const
{
	GPOS_RTL_ASSERT(IsComplete());
	return m_ulJoinDepth;
}

ULONG
CDrvdPropRelational::DeriveJoinDepth(CExpressionHandle &exprhdl)
{
	if (!m_is_prop_derived->ExchangeSet(EdptJoinDepth))
	{
		gpos::Ref<CLogical> popLogical =
			gpos::dyn_cast<CLogical>(exprhdl.Pop());
		m_ulJoinDepth = popLogical->DeriveJoinDepth(m_mp, exprhdl);
	}

	return m_ulJoinDepth;
}

// partition consumers
gpos::Ref<CPartInfo>
CDrvdPropRelational::GetPartitionInfo() const
{
	GPOS_RTL_ASSERT(IsComplete());
	return m_ppartinfo.get();
}

gpos::Ref<CPartInfo>
CDrvdPropRelational::DerivePartitionInfo(CExpressionHandle &exprhdl)
{
	if (!m_is_prop_derived->ExchangeSet(EdptPpartinfo))
	{
		gpos::Ref<CLogical> popLogical =
			gpos::dyn_cast<CLogical>(exprhdl.Pop());
		m_ppartinfo = popLogical->DerivePartitionInfo(m_mp, exprhdl);

		GPOS_ASSERT(nullptr != m_ppartinfo);
	}

	return m_ppartinfo.get();
}

// constraint property
gpos::Ref<CPropConstraint>
CDrvdPropRelational::GetPropertyConstraint() const
{
	GPOS_RTL_ASSERT(IsComplete());
	return m_ppc.get();
}

gpos::Ref<CPropConstraint>
CDrvdPropRelational::DerivePropertyConstraint(CExpressionHandle &exprhdl)
{
	if (!m_is_prop_derived->ExchangeSet(EdptPpc))
	{
		gpos::Ref<CLogical> popLogical =
			gpos::dyn_cast<CLogical>(exprhdl.Pop());
		m_ppc = popLogical->DerivePropertyConstraint(m_mp, exprhdl);
	}

	return m_ppc.get();
}

// function properties
gpos::Ref<CFunctionProp>
CDrvdPropRelational::GetFunctionProperties() const
{
	GPOS_RTL_ASSERT(IsComplete());
	return m_pfp.get();
}

gpos::Ref<CFunctionProp>
CDrvdPropRelational::DeriveFunctionProperties(CExpressionHandle &exprhdl)
{
	if (!m_is_prop_derived->ExchangeSet(EdptPfp))
	{
		gpos::Ref<CLogical> popLogical =
			gpos::dyn_cast<CLogical>(exprhdl.Pop());
		m_pfp = popLogical->DeriveFunctionProperties(m_mp, exprhdl);
	}

	return m_pfp.get();
}

// table descriptor
gpos::Ref<CTableDescriptor>
CDrvdPropRelational::GetTableDescriptor() const
{
	GPOS_RTL_ASSERT(IsComplete());
	return m_table_descriptor;
}

gpos::Ref<CTableDescriptor>
CDrvdPropRelational::DeriveTableDescriptor(CExpressionHandle &exprhdl)
{
	if (!m_is_prop_derived->ExchangeSet(EdptTableDescriptor))
	{
		gpos::Ref<CLogical> popLogical =
			gpos::dyn_cast<CLogical>(exprhdl.Pop());
		m_table_descriptor = popLogical->DeriveTableDescriptor(m_mp, exprhdl);
	}

	return m_table_descriptor;
}
// EOF
