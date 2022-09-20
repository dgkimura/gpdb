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
#include "gpos/common/owner.h"

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
	gpos::owner<CColRefSet *> m_pcrsLocalUsed;

	// output column generation given a list of column descriptors
	static gpos::owner<CColRefArray *> PdrgpcrCreateMapping(
		CMemoryPool *mp,
		gpos::pointer<const CColumnDescriptorArray *> pdrgpcoldesc,
		ULONG ulOpSourceId, gpos::pointer<IMDId *> mdid_table = nullptr);

	// initialize the array of partition columns
	static gpos::owner<CColRef2dArray *> PdrgpdrgpcrCreatePartCols(
		CMemoryPool *mp, gpos::pointer<CColRefArray *> colref_array,
		gpos::pointer<const ULongPtrArray *> pdrgpulPart);

	// derive dummy statistics
	gpos::owner<IStatistics *> PstatsDeriveDummy(CMemoryPool *mp,
												 CExpressionHandle &exprhdl,
												 CDouble rows) const;

	// helper for common case of output derivation from outer child
	static gpos::owner<CColRefSet *> PcrsDeriveOutputPassThru(
		CExpressionHandle &exprhdl);

	// helper for common case of not nullable columns derivation from outer child
	static gpos::owner<CColRefSet *> PcrsDeriveNotNullPassThruOuter(
		CExpressionHandle &exprhdl);

	// helper for common case of output derivation from all logical children
	static gpos::owner<CColRefSet *> PcrsDeriveOutputCombineLogical(
		CMemoryPool *mp, CExpressionHandle &exprhdl);

	// helper for common case of combining not nullable columns from all logical children
	static gpos::owner<CColRefSet *> PcrsDeriveNotNullCombineLogical(
		CMemoryPool *mp, CExpressionHandle &exprhdl);

	// helper for common case of stat columns computation
	static gpos::owner<CColRefSet *> PcrsReqdChildStats(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::pointer<CColRefSet *> pcrsInput,
		gpos::pointer<CColRefSet *> pcrsUsed, ULONG child_index);

	// helper for common case of passing through required stat columns
	static gpos::owner<CColRefSet *> PcrsStatsPassThru(
		gpos::pointer<CColRefSet *> pcrsInput);

	// helper for common case of passing through derived stats
	static gpos::owner<IStatistics *> PstatsPassThruOuter(
		CExpressionHandle &exprhdl);

	// shorthand to addref and pass through keys from n-th child
	static CKeyCollection *PkcDeriveKeysPassThru(CExpressionHandle &exprhdl,
												 ULONG ulInput);

	// shorthand to combine keys from first n - 1 children
	static gpos::owner<CKeyCollection *> PkcCombineKeys(
		CMemoryPool *mp, CExpressionHandle &exprhdl);

	// helper function for computing the keys in a base relation
	static gpos::owner<CKeyCollection *> PkcKeysBaseTable(
		CMemoryPool *mp, gpos::pointer<const CBitSetArray *> pdrgpbsKeys,
		gpos::pointer<const CColRefArray *> pdrgpcrOutput);

	// helper for the common case of passing through partition consumer info
	static gpos::owner<CPartInfo *> PpartinfoPassThruOuter(
		CExpressionHandle &exprhdl);

	// helper for common case of combining partition consumer info from logical children
	static gpos::owner<CPartInfo *> PpartinfoDeriveCombine(
		CMemoryPool *mp, CExpressionHandle &exprhdl);

	// derive constraint property from a table/index get
	static gpos::owner<CPropConstraint *> PpcDeriveConstraintFromTable(
		CMemoryPool *mp, gpos::pointer<const CTableDescriptor *> ptabdesc,
		gpos::pointer<const CColRefArray *> pdrgpcrOutput);

	// derive constraint property from a table/index get with predicates
	static gpos::owner<CPropConstraint *>
	PpcDeriveConstraintFromTableWithPredicates(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::pointer<const CTableDescriptor *> ptabdesc,
		gpos::pointer<const CColRefArray *> pdrgpcrOutput);

	// shorthand to addref and pass through constraint from a given child
	static CPropConstraint *PpcDeriveConstraintPassThru(
		CExpressionHandle &exprhdl, ULONG ulChild);

	// derive constraint property only on the given columns
	static gpos::owner<CPropConstraint *> PpcDeriveConstraintRestrict(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::pointer<CColRefSet *> pcrsOutput);

	// default max card for join and apply operators
	static CMaxCard MaxcardDef(CExpressionHandle &exprhdl);

	// compute max card given scalar child and constraint property
	static CMaxCard Maxcard(CExpressionHandle &exprhdl, ULONG ulScalarIndex,
							CMaxCard maxcard);

	// compute order spec based on an index
	static gpos::owner<COrderSpec *> PosFromIndex(
		CMemoryPool *mp, gpos::pointer<const IMDIndex *> pmdindex,
		gpos::pointer<CColRefArray *> colref_array,
		gpos::pointer<const CTableDescriptor *> ptabdesc);

	// derive function properties using data access property of scalar child
	static gpos::owner<CFunctionProp *> PfpDeriveFromScalar(
		CMemoryPool *mp, CExpressionHandle &exprhdl);

	// derive outer references
	static gpos::owner<CColRefSet *> DeriveOuterReferences(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::pointer<CColRefSet *> pcrsUsedAdditional);

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
	gpos::pointer<CColRefSet *>
	PcrsLocalUsed() const
	{
		return m_pcrsLocalUsed;
	}

	//-------------------------------------------------------------------------------------
	// Derived Relational Properties
	//-------------------------------------------------------------------------------------

	// create derived properties container
	gpos::owner<CDrvdProp *> PdpCreate(CMemoryPool *mp) const override;

	// derive output columns
	virtual gpos::owner<CColRefSet *> DeriveOutputColumns(
		CMemoryPool *mp, CExpressionHandle &exprhdl) = 0;

	// derive outer references
	virtual gpos::owner<CColRefSet *>
	DeriveOuterReferences(CMemoryPool *mp, CExpressionHandle &exprhdl)
	{
		return DeriveOuterReferences(mp, exprhdl,
									 nullptr /*pcrsUsedAdditional*/);
	}

	// derive outer references for index get and dynamic index get operators
	virtual gpos::owner<CColRefSet *> PcrsDeriveOuterIndexGet(
		CMemoryPool *mp, CExpressionHandle &exprhdl);

	// derive not nullable output columns
	virtual gpos::owner<CColRefSet *>
	DeriveNotNullColumns(CMemoryPool *mp,
						 CExpressionHandle &  // exprhdl
	) const
	{
		// by default, return an empty set
		return GPOS_NEW(mp) CColRefSet(mp);
	}

	// derive columns from the inner child of a correlated-apply expression that can be used above the apply expression
	virtual gpos::owner<CColRefSet *> DeriveCorrelatedApplyColumns(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const;

	// derive key collections
	virtual gpos::owner<CKeyCollection *> DeriveKeyCollection(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const;

	// derive max card
	virtual CMaxCard DeriveMaxCard(CMemoryPool *mp,
								   CExpressionHandle &exprhdl) const;

	// derive join depth
	virtual ULONG DeriveJoinDepth(CMemoryPool *mp,
								  CExpressionHandle &exprhdl) const;

	// derive partition information
	virtual gpos::owner<CPartInfo *> DerivePartitionInfo(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const = 0;

	// derive constraint property
	virtual gpos::owner<CPropConstraint *> DerivePropertyConstraint(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const = 0;

	// derive function properties
	virtual gpos::owner<CFunctionProp *> DeriveFunctionProperties(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const;

	virtual gpos::pointer<CTableDescriptor *> DeriveTableDescriptor(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const;

	//-------------------------------------------------------------------------------------
	// Derived Stats
	//-------------------------------------------------------------------------------------

	// derive statistics
	virtual gpos::owner<IStatistics *> PstatsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::pointer<IStatisticsArray *> stats_ctxt) const = 0;

	// promise level for stat derivation
	virtual EStatPromise Esp(CExpressionHandle &) const = 0;

	//-------------------------------------------------------------------------------------
	// Required Relational Properties
	//-------------------------------------------------------------------------------------

	// create required properties container
	gpos::owner<CReqdProp *> PrpCreate(CMemoryPool *mp) const override;

	// compute required stat columns of the n-th child
	virtual gpos::owner<CColRefSet *> PcrsStat(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::pointer<CColRefSet *> pcrsInput, ULONG child_index) const = 0;

	// compute partition predicate to pass down to n-th child
	virtual gpos::owner<CExpression *> PexprPartPred(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::pointer<CExpression *> pexprInput, ULONG child_index) const;

	//-------------------------------------------------------------------------------------
	// Transformations
	//-------------------------------------------------------------------------------------

	// candidate set of xforms
	virtual gpos::owner<CXformSet *> PxfsCandidates(CMemoryPool *mp) const = 0;

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
	virtual BOOL
	FCanPullProjectionsUp(ULONG	 //child_index
	) const
	{
		return true;
	}

	// helper for deriving statistics on a base table
	static gpos::owner<IStatistics *> PstatsBaseTable(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::pointer<CTableDescriptor *> ptabdesc,
		gpos::pointer<CColRefSet *> pcrsStatExtra = nullptr);

	// conversion function
	static gpos::cast_func<CLogical *>
	PopConvert(COperator *pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(pop->FLogical());

		return dynamic_cast<CLogical *>(pop);
	}

	// returns the table descriptor for (Dynamic)(BitmapTable)Get operators
	static CTableDescriptor *PtabdescFromTableGet(
		gpos::pointer<COperator *> pop);

	// returns the output columns for selected operator
	static CColRefArray *PoutputColsFromTableGet(COperator *pop);

	// extract the output columns descriptor from a logical get or dynamic get operator
	static CColRefArray *PdrgpcrOutputFromLogicalGet(
		gpos::pointer<CLogical *> pop);

	// extract the table name from a logical get or dynamic get operator
	static const CName &NameFromLogicalGet(gpos::pointer<CLogical *> pop);

	// return the set of distribution columns
	static gpos::owner<CColRefSet *> PcrsDist(
		CMemoryPool *mp, gpos::pointer<const CTableDescriptor *> ptabdesc,
		gpos::pointer<const CColRefArray *> colref_array);

	// derive constraint property when expression has relational children and predicates
	static gpos::owner<CPropConstraint *> PpcDeriveConstraintFromPredicates(
		CMemoryPool *mp, CExpressionHandle &exprhdl);

};	// class CLogical

}  // namespace gpopt


#endif	// !GPOPT_CLogical_H

// EOF
