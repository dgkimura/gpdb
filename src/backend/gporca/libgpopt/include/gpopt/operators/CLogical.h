//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2009 Greenplum, Inc.
//
//	@filename:
//		CLogical.h
//
//	@doc:
//		Base class for all logical operators
//---------------------------------------------------------------------------
#ifndef GPOPT_CLogical_H
#define GPOPT_CLogical_H

#include "gpos/base.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CDrvdProp.h"
#include "gpopt/base/CKeyCollection.h"
#include "gpopt/base/CMaxCard.h"
#include "gpopt/base/CPartInfo.h"
#include "gpopt/base/CPropConstraint.h"
#include "gpopt/base/CReqdProp.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/operators/COperator.h"
#include "gpopt/xforms/CXform.h"

// fwd declarataion
namespace gpnaucrates
{
class IStatistics;
}

namespace gpopt
{
using namespace gpos;


//---------------------------------------------------------------------------
//	@class:
//		CLogical
//
//	@doc:
//		base class for all logical operators
//
//---------------------------------------------------------------------------
class CLogical : public COperator
{
public:
	// statistics derivation promise levels;
	// used for prioritizing operators within the same Memo group for statistics derivation
	enum EStatPromise
	{
		EspNone,	// operator must not be used for stat derivation
		EspLow,		// operator has low priority for stat derivation
		EspMedium,	// operator has medium priority for stat derivation
		EspHigh		// operator has high priority for stat derivation
	};

private:
	// private copy ctor
	CLogical(const CLogical &);


protected:
	// set of locally used columns
	gpos::Ref<CColRefSet> m_pcrsLocalUsed;

	// output column generation given a list of column descriptors
	static gpos::Ref<CColRefArray> PdrgpcrCreateMapping(
		CMemoryPool *mp, const gpos::Ref<CColumnDescriptorArray> pdrgpcoldesc,
		ULONG ulOpSourceId, gpos::Ref<IMDId> mdid_table = nullptr);

	// initialize the array of partition columns
	static gpos::Ref<CColRef2dArray> PdrgpdrgpcrCreatePartCols(
		CMemoryPool *mp, gpos::Ref<CColRefArray> colref_array,
		const gpos::Ref<ULongPtrArray> pdrgpulPart);

	// derive dummy statistics
	gpos::Ref<IStatistics> PstatsDeriveDummy(CMemoryPool *mp,
											 CExpressionHandle &exprhdl,
											 CDouble rows) const;

	// helper for common case of output derivation from outer child
	static gpos::Ref<CColRefSet> PcrsDeriveOutputPassThru(
		CExpressionHandle &exprhdl);

	// helper for common case of not nullable columns derivation from outer child
	static gpos::Ref<CColRefSet> PcrsDeriveNotNullPassThruOuter(
		CExpressionHandle &exprhdl);

	// helper for common case of output derivation from all logical children
	static gpos::Ref<CColRefSet> PcrsDeriveOutputCombineLogical(
		CMemoryPool *mp, CExpressionHandle &exprhdl);

	// helper for common case of combining not nullable columns from all logical children
	static gpos::Ref<CColRefSet> PcrsDeriveNotNullCombineLogical(
		CMemoryPool *mp, CExpressionHandle &exprhdl);

	// helper for common case of stat columns computation
	static gpos::Ref<CColRefSet> PcrsReqdChildStats(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::Ref<CColRefSet> pcrsInput, gpos::Ref<CColRefSet> pcrsUsed,
		ULONG child_index);

	// helper for common case of passing through required stat columns
	static gpos::Ref<CColRefSet> PcrsStatsPassThru(
		gpos::Ref<CColRefSet> pcrsInput);

	// helper for common case of passing through derived stats
	static gpos::Ref<IStatistics> PstatsPassThruOuter(
		CExpressionHandle &exprhdl);

	// shorthand to addref and pass through keys from n-th child
	static gpos::Ref<CKeyCollection> PkcDeriveKeysPassThru(
		CExpressionHandle &exprhdl, ULONG ulInput);

	// shorthand to combine keys from first n - 1 children
	static gpos::Ref<CKeyCollection> PkcCombineKeys(CMemoryPool *mp,
													CExpressionHandle &exprhdl);

	// helper function for computing the keys in a base relation
	static gpos::Ref<CKeyCollection> PkcKeysBaseTable(
		CMemoryPool *mp, const gpos::Ref<CBitSetArray> pdrgpbsKeys,
		const gpos::Ref<CColRefArray> pdrgpcrOutput);

	// helper for the common case of passing through partition consumer info
	static gpos::Ref<CPartInfo> PpartinfoPassThruOuter(
		CExpressionHandle &exprhdl);

	// helper for common case of combining partition consumer info from logical children
	static gpos::Ref<CPartInfo> PpartinfoDeriveCombine(
		CMemoryPool *mp, CExpressionHandle &exprhdl);

	// derive constraint property from a table/index get
	static gpos::Ref<CPropConstraint> PpcDeriveConstraintFromTable(
		CMemoryPool *mp, const gpos::Ref<CTableDescriptor> ptabdesc,
		const gpos::Ref<CColRefArray> pdrgpcrOutput);

	// derive constraint property from a table/index get with predicates
	static gpos::Ref<CPropConstraint>
	PpcDeriveConstraintFromTableWithPredicates(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		const gpos::Ref<CTableDescriptor> ptabdesc,
		const gpos::Ref<CColRefArray> pdrgpcrOutput);

	// shorthand to addref and pass through constraint from a given child
	static gpos::Ref<CPropConstraint> PpcDeriveConstraintPassThru(
		CExpressionHandle &exprhdl, ULONG ulChild);

	// derive constraint property only on the given columns
	static gpos::Ref<CPropConstraint> PpcDeriveConstraintRestrict(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::Ref<CColRefSet> pcrsOutput);

	// default max card for join and apply operators
	static CMaxCard MaxcardDef(CExpressionHandle &exprhdl);

	// compute max card given scalar child and constraint property
	static CMaxCard Maxcard(CExpressionHandle &exprhdl, ULONG ulScalarIndex,
							CMaxCard maxcard);

	// compute order spec based on an index
	static gpos::Ref<COrderSpec> PosFromIndex(
		CMemoryPool *mp, const gpos::Ref<IMDIndex> pmdindex,
		gpos::Ref<CColRefArray> colref_array,
		const gpos::Ref<CTableDescriptor> ptabdesc);

	// derive function properties using data access property of scalar child
	static gpos::Ref<CFunctionProp> PfpDeriveFromScalar(
		CMemoryPool *mp, CExpressionHandle &exprhdl);

	// derive outer references
	static gpos::Ref<CColRefSet> DeriveOuterReferences(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::Ref<CColRefSet> pcrsUsedAdditional);

public:
	// ctor
	explicit CLogical(CMemoryPool *mp);

	// dtor
	~CLogical() override;

	// type of operator
	BOOL
	FLogical() const override
	{
		GPOS_ASSERT(!FPhysical() && !FScalar() && !FPattern());
		return true;
	}

	// return the locally used columns
	gpos::Ref<CColRefSet>
	PcrsLocalUsed() const
	{
		return m_pcrsLocalUsed;
	}

	//-------------------------------------------------------------------------------------
	// Derived Relational Properties
	//-------------------------------------------------------------------------------------

	// create derived properties container
	gpos::Ref<CDrvdProp> PdpCreate(CMemoryPool *mp) const override;

	// derive output columns
	virtual gpos::Ref<CColRefSet> DeriveOutputColumns(
		CMemoryPool *mp, CExpressionHandle &exprhdl) = 0;

	// derive outer references
	virtual gpos::Ref<CColRefSet>
	DeriveOuterReferences(CMemoryPool *mp, CExpressionHandle &exprhdl)
	{
		return DeriveOuterReferences(mp, exprhdl,
									 nullptr /*pcrsUsedAdditional*/);
	}

	// derive outer references for index get and dynamic index get operators
	virtual gpos::Ref<CColRefSet> PcrsDeriveOuterIndexGet(
		CMemoryPool *mp, CExpressionHandle &exprhdl);

	// derive not nullable output columns
	virtual gpos::Ref<CColRefSet>
	DeriveNotNullColumns(CMemoryPool *mp,
						 CExpressionHandle &  // exprhdl
	) const
	{
		// by default, return an empty set
		return GPOS_NEW(mp) CColRefSet(mp);
	}

	// derive columns from the inner child of a correlated-apply expression that can be used above the apply expression
	virtual gpos::Ref<CColRefSet> DeriveCorrelatedApplyColumns(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const;

	// derive key collections
	virtual gpos::Ref<CKeyCollection> DeriveKeyCollection(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const;

	// derive max card
	virtual CMaxCard DeriveMaxCard(CMemoryPool *mp,
								   CExpressionHandle &exprhdl) const;

	// derive join depth
	virtual ULONG DeriveJoinDepth(CMemoryPool *mp,
								  CExpressionHandle &exprhdl) const;

	// derive partition information
	virtual gpos::Ref<CPartInfo> DerivePartitionInfo(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const = 0;

	// derive constraint property
	virtual gpos::Ref<CPropConstraint> DerivePropertyConstraint(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const = 0;

	// derive function properties
	virtual gpos::Ref<CFunctionProp> DeriveFunctionProperties(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const;

	virtual gpos::Ref<CTableDescriptor> DeriveTableDescriptor(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const;

	//-------------------------------------------------------------------------------------
	// Derived Stats
	//-------------------------------------------------------------------------------------

	// derive statistics
	virtual gpos::Ref<IStatistics> PstatsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::Ref<IStatisticsArray> stats_ctxt) const = 0;

	// promise level for stat derivation
	virtual EStatPromise Esp(CExpressionHandle &) const = 0;

	//-------------------------------------------------------------------------------------
	// Required Relational Properties
	//-------------------------------------------------------------------------------------

	// create required properties container
	gpos::Ref<CReqdProp> PrpCreate(CMemoryPool *mp) const override;

	// compute required stat columns of the n-th child
	virtual gpos::Ref<CColRefSet> PcrsStat(CMemoryPool *mp,
										   CExpressionHandle &exprhdl,
										   gpos::Ref<CColRefSet> pcrsInput,
										   ULONG child_index) const = 0;

	//-------------------------------------------------------------------------------------
	// Transformations
	//-------------------------------------------------------------------------------------

	// candidate set of xforms
	virtual gpos::Ref<CXformSet> PxfsCandidates(CMemoryPool *mp) const = 0;

	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------

	// return true if operator can select a subset of input tuples based on some predicate,
	// for example, a Join is a SelectionOp, but a Project is not
	virtual BOOL
	FSelectionOp() const
	{
		return false;
	}

	// return true if we can pull projections up past this operator from its given child
	virtual BOOL FCanPullProjectionsUp(ULONG  //child_index
	) const
	{
		return true;
	}

	// helper for deriving statistics on a base table
	static gpos::Ref<IStatistics> PstatsBaseTable(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::Ref<CTableDescriptor> ptabdesc,
		gpos::Ref<CColRefSet> pcrsStatExtra = nullptr);

	// conversion function
	static gpos::Ref<CLogical>
	PopConvert(gpos::Ref<COperator> pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(pop->FLogical());

		return gpos::dyn_cast<CLogical>(pop);
	}

	// returns the table descriptor for (Dynamic)(BitmapTable)Get operators
	static gpos::Ref<CTableDescriptor> PtabdescFromTableGet(
		gpos::Ref<COperator> pop);

	// returns the output columns for selected operator
	static gpos::Ref<CColRefArray> PoutputColsFromTableGet(
		gpos::Ref<COperator> pop);

	// extract the output columns descriptor from a logical get or dynamic get operator
	static gpos::Ref<CColRefArray> PdrgpcrOutputFromLogicalGet(
		gpos::Ref<CLogical> pop);

	// extract the table name from a logical get or dynamic get operator
	static const CName &NameFromLogicalGet(gpos::Ref<CLogical> pop);

	// return the set of distribution columns
	static gpos::Ref<CColRefSet> PcrsDist(
		CMemoryPool *mp, const gpos::Ref<CTableDescriptor> ptabdesc,
		const gpos::Ref<CColRefArray> colref_array);

	// derive constraint property when expression has relational children and predicates
	static gpos::Ref<CPropConstraint> PpcDeriveConstraintFromPredicates(
		CMemoryPool *mp, CExpressionHandle &exprhdl);

};	// class CLogical

}  // namespace gpopt


#endif	// !GPOPT_CLogical_H

// EOF
