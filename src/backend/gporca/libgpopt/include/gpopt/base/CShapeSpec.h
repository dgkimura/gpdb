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

using CWStringConstArray =
	CDynamicPtrArray<CWStringConst, CleanupDelete<CWStringConst>>;

using namespace gpos;

// fwd declaration
class CColRefSet;

class CPlanHint : public CRefCount
{
public:
	// check if order specs match
	virtual BOOL Matches(CPlanHint *pph) const = 0;

	// print
	virtual IOstream &OsPrint(IOstream &os) const = 0;

	virtual ULONG HashValue() const = 0;
};

class CScanHint : public CPlanHint
{
private:
	const CWStringConst *m_name;

	const CWStringConst *m_indexname{nullptr};

	COperator::EOperatorId m_id;

public:
	CScanHint(const CWStringConst *name, const CWStringConst *indexname,
			  COperator::EOperatorId id)
		: m_name(name), m_indexname(indexname), m_id(id)
	{
	}

	CScanHint(const CWStringConst *name, COperator::EOperatorId id)
		: m_name(name), m_id(id)
	{
	}

	BOOL
	Matches(CPlanHint *_pph) const override
	{
		CScanHint *pph = dynamic_cast<CScanHint *>(_pph);
		if (nullptr == pph)
		{
			return false;
		}

		if ((pph->m_indexname == nullptr && m_indexname != nullptr) ||
			(pph->m_indexname != nullptr && m_indexname == nullptr))
		{
			return false;
		}
		if (pph->m_indexname != nullptr && m_indexname != nullptr &&
			!CWStringConst::Equals(m_indexname, pph->m_indexname))
		{
			return false;
		}
		return CWStringConst::Equals(m_name, pph->m_name) && m_id == pph->m_id;
	}

	ULONG
	HashValue() const override
	{
		return gpos::CombineHashes(
			gpos::CombineHashes(
				CWStringConst::HashValue(m_name),
				m_indexname ? CWStringConst::HashValue(m_indexname) : 0),
			m_id);
	}

	IOstream &
	OsPrint(IOstream &os) const
	{
		os << "( ";
		os << "id:" << m_id << " name:" << m_name->GetBuffer();
		if (m_indexname)
		{
			os << " indexname:" << m_indexname->GetBuffer();
		}
		os << " )";

		return os;
	}
};

//FORCE_GENERATE_DBGSTR(gpopt::CScanHint);

class CJoinTypeHint : public CPlanHint
{
private:
	// order is irrelevant
	CWStringConstArray *m_names;

	COperator::EOperatorId m_id;

public:
	CJoinTypeHint(CWStringConstArray *names, COperator::EOperatorId id)
		: m_names(names), m_id(id)
	{
	}

	ULONG
	HashValue() const override
	{
		// TODO:FIXME: Array needs to be sorted or hash will be inconsistent...

		ULONG hashValue = m_id;
		for (ULONG ul = 0; ul < m_names->Size(); ul++)
		{
			hashValue = gpos::CombineHashes(
				hashValue, CWStringConst::HashValue((*m_names)[ul]));
		}

		return hashValue;
	}

	BOOL
	Matches(CPlanHint *_pph) const override
	{
		CJoinTypeHint *pph = dynamic_cast<CJoinTypeHint *>(_pph);
		if (nullptr == pph)
		{
			return false;
		}

		// TODO:FIXME: Array needs to be sorted or hash will be inconsistent...

		if (pph->m_names->Size() != m_names->Size())
		{
			return false;
		}
		for (ULONG ul = 0; ul < m_names->Size(); ul++)
		{
			if (!CWStringConst::Equals((*m_names)[ul], (*pph->m_names)[ul]))
			{
				return false;
			}
		}

		return m_id == pph->m_id;
	}

	IOstream &
	OsPrint(IOstream &os) const
	{
		os << "CJoinTypeHint";

		return os;
	}
};

class CJoinOrderHint : public CPlanHint
{
private:
	// order is relevant
	CWStringConstArray *m_names;

public:
	CJoinOrderHint(CWStringConstArray *names) : m_names(names)
	{
	}

	ULONG
	HashValue() const override
	{
		ULONG hashValue = 0;
		for (ULONG ul = 0; ul < m_names->Size(); ul++)
		{
			hashValue = gpos::CombineHashes(
				hashValue, CWStringConst::HashValue((*m_names)[ul]));
		}

		return hashValue;
	}

	BOOL
	Matches(CPlanHint *_pph) const override
	{
		CJoinOrderHint *pph = dynamic_cast<CJoinOrderHint *>(_pph);
		if (nullptr == pph)
		{
			return false;
		}

		if (pph->m_names->Size() != m_names->Size())
		{
			return false;
		}
		for (ULONG ul = 0; ul < m_names->Size(); ul++)
		{
			if (!CWStringConst::Equals((*m_names)[ul], (*pph->m_names)[ul]))
			{
				return false;
			}
		}

		return true;
	}

	IOstream &
	OsPrint(IOstream &os) const
	{
		os << "CJoinOrderHint";

		return os;
	}
};

class CJoinMotionHint : public CPlanHint
{
public:
	enum class MotionType
	{
		Redistribute,
		Broadcast,
		Gather,
		Any,
		None
	};

	CJoinMotionHint(CWStringConst *name_left, CWStringConst *name_right,
					MotionType motion_left, MotionType motion_right)
		: m_name_left(name_left),
		  m_name_right(name_right),
		  m_motion_left(motion_left),
		  m_motion_right(motion_right)
	{
	}

	ULONG
	HashValue() const override
	{
		return gpos::CombineHashes(
			gpos::CombineHashes(
				gpos::CombineHashes(CWStringConst::HashValue(m_name_left),
									CWStringConst::HashValue(m_name_right)),
				static_cast<ULONG>(m_motion_left)),
			static_cast<ULONG>(m_motion_right));
	}

	BOOL
	Matches(CPlanHint *_pph) const override
	{
		CJoinMotionHint *pph = dynamic_cast<CJoinMotionHint *>(_pph);
		if (nullptr == pph)
		{
			return false;
		}
		return CWStringConst::Equals(m_name_left, pph->m_name_left) &&
			   CWStringConst::Equals(m_name_right, pph->m_name_right) &&
			   m_motion_left == pph->m_motion_left &&
			   m_motion_right == pph->m_motion_right;
	}

	IOstream &
	OsPrint(IOstream &os) const
	{
		os << "CJoinMotionHint";

		return os;
	}

private:
	CWStringConst *m_name_left;
	CWStringConst *m_name_right;
	MotionType m_motion_left;
	MotionType m_motion_right;
};

using CPlanHintArray = CDynamicPtrArray<CPlanHint, CleanupRelease<CPlanHint>>;

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
	// memory pool
	CMemoryPool *m_mp;

protected:
	CPlanHintArray *m_pdrgpScanHint;
	CPlanHintArray *m_pdrgpJoinHint;

public:
	CShapeSpec(const CShapeSpec &) = delete;

	// ctor
	explicit CShapeSpec(CMemoryPool *mp);

	// dtor
	~CShapeSpec() override;

	// check if shape spec has hints
	BOOL
	IsEmpty() const
	{
		return m_pdrgpScanHint->Size() + m_pdrgpJoinHint->Size() == 0;
	}

	// append new component
	void Append(CPlanHint *pph);

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
