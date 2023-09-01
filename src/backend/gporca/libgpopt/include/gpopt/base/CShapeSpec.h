//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CShapeSpec.h
//
//	@doc:
//		Description of sort order;
//		Can be used as required or derived property;
//---------------------------------------------------------------------------
#ifndef GPOPT_CShapeSpec_H
#define GPOPT_CShapeSpec_H

#include "gpos/base.h"
#include "gpos/common/DbgPrintMixin.h"

#include "gpopt/base/CColRef.h"
#include "gpopt/base/CPropSpec.h"
#include "naucrates/md/IMDId.h"

namespace gpopt
{
// type definition of corresponding dynamic pointer array
class CShapeSpec;
using CShapeSpecArray = CDynamicPtrArray<CShapeSpec, CleanupRelease>;

using namespace gpos;

// fwd declaration
class CColRefSet;

//---------------------------------------------------------------------------
//	@class:
//		CShapeSpec
//
//	@doc:
//		Array of Shape Expressions
//
//---------------------------------------------------------------------------
class CShapeSpec : public CPropSpec
{
private:
	//---------------------------------------------------------------------------
	//	@class:
	//		CShapeExpression
	//
	//	@doc:
	//		Spec of shape component
	//
	//---------------------------------------------------------------------------
	class CShapeExpression : public gpos::DbgPrintMixin<CShapeExpression>
	{
	private:
		COperator::EOperatorId m_id;
		CWStringConst *m_alias;

	public:
		CShapeExpression(const CShapeExpression &) = delete;

		// ctor
		CShapeExpression(COperator::EOperatorId id, CWStringConst *alias);

		// dtor
		virtual ~CShapeExpression();

		// accessor of shape
		COperator::EOperatorId
		GetOperator() const
		{
			return m_id;
		}

		CWStringConst *
		GetAlias() const
		{
			return m_alias;
		}

		// check if order specs match
		BOOL Matches(const CShapeExpression *pse) const;

		// print
		IOstream &OsPrint(IOstream &os) const;

	};	// class CShapeExpression

	// array of order expressions
	using CShapeExpressionArray =
		CDynamicPtrArray<CShapeExpression, CleanupDelete>;


	// memory pool
	CMemoryPool *m_mp;

protected:
	// components of order spec
	CShapeExpressionArray *m_pdrgpoe;

public:
	CShapeSpec(const CShapeSpec &) = delete;

	// ctor
	explicit CShapeSpec(CMemoryPool *mp);

	// dtor
	~CShapeSpec() override;

	// number of sort expressions
	ULONG
	UlSortColumns() const
	{
		return m_pdrgpoe->Size();
	}

	// check if order spec has no columns
	BOOL
	IsEmpty() const
	{
		return UlSortColumns() == 0;
	}

	// append new component
	void Append(COperator::EOperatorId id, CWStringConst *alias);

	// append shape spec
	void Append(CShapeSpec *pss);

	// extract colref set of order columns
	CColRefSet *PcrsUsed(CMemoryPool *mp) const override;

	// property type
	EPropSpecType
	Epst() const override
	{
		return EpstShape;
	}

	// check if order specs match
	BOOL Matches(const CShapeSpec *pos) const;

	// check if order specs satisfies req'd spec
	BOOL FSatisfies(const CShapeSpec *pos) const;

	// append enforcers to dynamic array for the given plan properties
	void AppendEnforcers(CMemoryPool *mp, CExpressionHandle &exprhdl,
						 CReqdPropPlan *prpp, CExpressionArray *pdrgpexpr,
						 CExpression *pexpr) override;

	// hash function
	ULONG HashValue() const override;

	// print
	IOstream &OsPrint(IOstream &os) const override;

	// matching function over order spec arrays
	static BOOL Equals(const CShapeSpecArray *pdrgposFirst,
					   const CShapeSpecArray *pdrgposSecond);

	// combine hash values of a maximum number of entries
	static ULONG HashValue(const CShapeSpecArray *pdrgpos, ULONG ulMaxSize);

	// print array of order spec objects
	static IOstream &OsPrint(IOstream &os, const CShapeSpecArray *pdrgpos);


};	// class CShapeSpec

}  // namespace gpopt

#endif	// !GPOPT_CShapeSpec_H

// EOF
