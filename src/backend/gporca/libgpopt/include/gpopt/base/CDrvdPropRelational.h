//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2009 - 2011 EMC CORP.
//
//	@filename:
//		CDrvdPropRelational.h
//
//	@doc:
//		Derived logical properties
//---------------------------------------------------------------------------
#ifndef GPOPT_CDrvdPropRelational_H
#define GPOPT_CDrvdPropRelational_H

#include "gpos/base.h"
#include "gpos/common/CRefCount.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CColRef.h"
#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CDrvdProp.h"
#include "gpopt/base/CFunctionProp.h"
#include "gpopt/base/CFunctionalDependency.h"
#include "gpopt/base/CMaxCard.h"
#include "gpopt/base/CPropConstraint.h"
#include "gpopt/metadata/CTableDescriptor.h"

namespace gpopt
{
using namespace gpos;

// fwd declaration
class CExpressionHandle;
class CReqdPropPlan;
class CKeyCollection;
class CPropConstraint;
class CPartInfo;

//---------------------------------------------------------------------------
//	@class:
//		CDrvdPropRelational
//
//	@doc:
//		Derived logical properties container.
//
//		These are properties than can be inferred from logical expressions or
//		Memo groups. This includes output columns, outer references, primary
//		keys. These properties hold regardless of the physical implementation
//		of an expression.
//
//---------------------------------------------------------------------------
class CDrvdPropRelational : public CDrvdProp
{
	friend class CExpression;

	// See member variables (below) with the same name for description on what
	// the property types respresent
	enum EDrvdPropType
	{
		EdptPcrsOutput = 0,
		EdptPcrsOuter,
		EdptPcrsNotNull,
		EdptPcrsCorrelatedApply,
		EdptPkc,
		EdptPdrgpfd,
		EdptMaxCard,
		EdptPpartinfo,
		EdptPpc,
		EdptPfp,
		EdptJoinDepth,
		EdptTableDescriptor,
		EdptSentinel
	};

private:
	CMemoryPool *m_mp;

	// bitset representing whether property has been derived
	gpos::owner<CBitSet *> m_is_prop_derived;

	// output columns
	gpos::owner<CColRefSet *> m_pcrsOutput;

	// columns not defined in the underlying operator tree
	gpos::owner<CColRefSet *> m_pcrsOuter;

	// output columns that do not allow null values
	gpos::owner<CColRefSet *> m_pcrsNotNull;

	// columns from the inner child of a correlated-apply expression that can be used above the apply expression
	gpos::owner<CColRefSet *> m_pcrsCorrelatedApply;

	// key collection
	gpos::owner<CKeyCollection *> m_pkc;

	// functional dependencies
	gpos::owner<CFunctionalDependencyArray *> m_pdrgpfd;

	// max card
	CMaxCard m_maxcard;

	// join depth (number of relations in underlying tree)
	ULONG m_ulJoinDepth;

	// partition table consumers
	gpos::owner<CPartInfo *> m_ppartinfo;

	// constraint property
	gpos::owner<CPropConstraint *> m_ppc;

	// function properties
	gpos::owner<CFunctionProp *> m_pfp;

	gpos::pointer<CTableDescriptor *> m_table_descriptor;

	// helper for getting applicable FDs from child
	static CFunctionalDependencyArray *DeriveChildFunctionalDependencies(
		CMemoryPool *mp, ULONG child_index, CExpressionHandle &exprhdl);

	// helper for creating local FDs
	static CFunctionalDependencyArray *DeriveLocalFunctionalDependencies(
		CMemoryPool *mp, CExpressionHandle &exprhdl);

	// Have all the properties been derived?
	//
	// NOTE1: This is set ONLY when Derive() is called. If all the properties
	// are independently derived, m_is_complete will remain false. In that
	// case, even though Derive() would attempt to derive all the properties
	// once again, it should be quick, since each individual member has been
	// cached.
	// NOTE2: Once these properties are detached from the
	// corresponding expression used to derive it, this MUST be set to true,
	// since after the detachment, there will be no way to derive the
	// properties once again.
	BOOL m_is_complete;

protected:
	// output columns
	CColRefSet *DeriveOutputColumns(CExpressionHandle &);

	// outer references
	CColRefSet *DeriveOuterReferences(CExpressionHandle &);

	// nullable columns
	CColRefSet *DeriveNotNullColumns(CExpressionHandle &);

	// columns from the inner child of a correlated-apply expression that can be used above the apply expression
	CColRefSet *DeriveCorrelatedApplyColumns(CExpressionHandle &);

	// key collection
	CKeyCollection *DeriveKeyCollection(CExpressionHandle &);

	// functional dependencies
	CFunctionalDependencyArray *DeriveFunctionalDependencies(
		CExpressionHandle &);

	// max cardinality
	CMaxCard DeriveMaxCard(CExpressionHandle &);

	// join depth
	ULONG DeriveJoinDepth(CExpressionHandle &);

	// partition consumers
	CPartInfo *DerivePartitionInfo(CExpressionHandle &);

	// constraint property
	CPropConstraint *DerivePropertyConstraint(CExpressionHandle &);

	// function properties
	CFunctionProp *DeriveFunctionProperties(CExpressionHandle &);

	CTableDescriptor *DeriveTableDescriptor(CExpressionHandle &);

public:
	CDrvdPropRelational(const CDrvdPropRelational &) = delete;

	// ctor
	CDrvdPropRelational(CMemoryPool *mp);

	// dtor
	~CDrvdPropRelational() override;

	// type of properties
	EPropType
	Ept() override
	{
		return EptRelational;
	}

	BOOL
	IsComplete() const override
	{
		return m_is_complete;
	}

	// derivation function
	void Derive(CMemoryPool *mp, CExpressionHandle &exprhdl,
				CDrvdPropCtxt *pdpctxt) override;

	// output columns
	gpos::pointer<CColRefSet *> GetOutputColumns() const;

	// outer references
	gpos::pointer<CColRefSet *> GetOuterReferences() const;

	// nullable columns
	gpos::pointer<CColRefSet *> GetNotNullColumns() const;

	// columns from the inner child of a correlated-apply expression that can be used above the apply expression
	gpos::pointer<CColRefSet *> GetCorrelatedApplyColumns() const;

	// key collection
	gpos::pointer<CKeyCollection *> GetKeyCollection() const;

	// functional dependencies
	gpos::pointer<CFunctionalDependencyArray *> GetFunctionalDependencies()
		const;

	// max cardinality
	CMaxCard GetMaxCard() const;

	// join depth
	ULONG GetJoinDepth() const;

	// partition consumers
	gpos::pointer<CPartInfo *> GetPartitionInfo() const;

	// constraint property
	gpos::pointer<CPropConstraint *> GetPropertyConstraint() const;

	// function properties
	gpos::pointer<CFunctionProp *> GetFunctionProperties() const;

	gpos::pointer<CTableDescriptor *> GetTableDescriptor() const;

	// shorthand for conversion
	static gpos::cast_func<CDrvdPropRelational *> GetRelationalProperties(
		CDrvdProp *pdp);

	// check for satisfying required plan properties
	BOOL FSatisfies(gpos::pointer<const CReqdPropPlan *> prpp) const override;

	// print function
	IOstream &OsPrint(IOstream &os) const override;

};	// class CDrvdPropRelational

}  // namespace gpopt


#endif	// !GPOPT_CDrvdPropRelational_H

// EOF
