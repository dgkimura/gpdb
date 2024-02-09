//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (c) 2023 VMware, Inc. or its affiliates. All Rights Reserved.
//
//	@filename:
//		CJoinHint.h
//---------------------------------------------------------------------------
#ifndef GPOS_CJoinHint_H
#define GPOS_CJoinHint_H

#include <functional>

#include "gpos/base.h"
#include "gpos/common/CDynamicPtrArray.h"
#include "gpos/common/CRefCount.h"

#include "gpopt/exception.h"
#include "gpopt/hints/IHint.h"
#include "gpopt/operators/CExpression.h"
#include "gpopt/operators/COperator.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

namespace gpopt
{
class CJoinHint : public IHint, public DbgPrintMixin<CJoinHint>
{
public:
	using CWStringConstHashSet =
		CHashSet<const CWStringConst, CWStringConst::HashValue,
				 CWStringConst::Equals, CleanupDelete<const CWStringConst>>;

	using CWStringConstHashSetIter =
		CHashSetIter<const CWStringConst, CWStringConst::HashValue,
					 CWStringConst::Equals, CleanupDelete<const CWStringConst>>;

	class JoinPair : public CRefCount, public DbgPrintMixin<JoinPair>
	{
	private:
		CWStringConst *m_name{nullptr};

		JoinPair *m_outer{nullptr};
		JoinPair *m_inner{nullptr};

		CWStringConstHashSet *m_aliases{nullptr};

	public:
		JoinPair(CWStringConst *name, CMemoryPool *mp)
			: m_name(name),
			  m_aliases(GPOS_NEW(mp) CJoinHint::CWStringConstHashSet(mp))
		{
			m_aliases->Insert(m_name);
		}
		JoinPair(JoinPair *outer, JoinPair *inner,
				 CWStringConstHashSet *aliases)
			: m_outer(outer), m_inner(inner), m_aliases(aliases)
		{
		}

		~JoinPair() override
		{
			GPOS_DELETE(m_name);

			CRefCount::SafeRelease(m_outer);
			CRefCount::SafeRelease(m_inner);
		}

		CWStringConst *
		GetName() const
		{
			return m_name;
		}

		JoinPair *
		GetOuter() const
		{
			return m_outer;
		}

		JoinPair *
		GetInner() const
		{
			return m_inner;
		}

		CWStringConstHashSet *
		GetAliases() const
		{
			return m_aliases;
		}

		IOstream &OsPrint(IOstream &os) const;
	};

private:
	JoinPair *m_join_pair;

	// m_aliases contains sorted alias names.
	StringPtrArray *m_aliases{nullptr};

public:
	CJoinHint(CMemoryPool *mp, JoinPair *join_pair)
		: m_join_pair(join_pair), m_aliases(GPOS_NEW(mp) StringPtrArray(mp))
	{
		std::function<void(JoinPair *, StringPtrArray *)> build_aliases =
			[&build_aliases](JoinPair *pair, StringPtrArray *aliases) {
				if (nullptr != pair->GetName())
				{
					aliases->Append(pair->GetName());
				}
				else
				{
					build_aliases(pair->GetOuter(), aliases);
					build_aliases(pair->GetInner(), aliases);
				}
			};
		build_aliases(join_pair, m_aliases);

		m_aliases->Sort(CWStringBase::Compare);
	}

	~CJoinHint() override
	{
		m_join_pair->Release();
	}

	const StringPtrArray *
	GetAliasNames() const
	{
		return m_aliases;
	}

	const CJoinHint::JoinPair *
	GetJoinPair() const
	{
		return m_join_pair;
	}

	IOstream &OsPrint(IOstream &os) const;

	void Serialize(CXMLSerializer *xml_serializer) const;
};

using JoinHintList = CDynamicPtrArray<CJoinHint, CleanupRelease>;

}  // namespace gpopt

#endif	// !GPOS_CJoinHint_H

// EOF
