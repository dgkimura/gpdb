//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CPartitionPropagationSpec.h
//
//	@doc:
//		Partition Propagation spec in required properties
//---------------------------------------------------------------------------
#ifndef GPOPT_CPartitionPropagationSpec_H
#define GPOPT_CPartitionPropagationSpec_H

#include "gpos/base.h"
#include "gpos/common/CRefCount.h"

#include "gpopt/base/COptCtxt.h"
#include "gpopt/base/CPropSpec.h"


namespace gpopt
{
using namespace gpos;

//---------------------------------------------------------------------------
//	@class:
//		CPartitionPropagationSpec
//
//	@doc:
//		Partition Propagation specification
//
//---------------------------------------------------------------------------
class CPartitionPropagationSpec : public CPropSpec
{
public:
	enum EPartPropSpecInfoType
	{
		EpptPropagator,
		EpptConsumer,
		EpptSentinel
	};

private:
	struct SPartPropSpecInfo : public CRefCount<SPartPropSpecInfo>
	{
		// scan id of the DynamicScan
		ULONG m_scan_id;

		// info type: consumer or propagator
		EPartPropSpecInfoType m_type;

		// relation id of the DynamicScan
		gpos::Ref<IMDId> m_root_rel_mdid;

		//  partition selector ids to use (reqd only)
		gpos::Ref<CBitSet> m_selector_ids = nullptr;

		// filter expressions to generate partition pruning data in the translator (reqd only)
		gpos::Ref<CExpression> m_filter_expr = nullptr;

		SPartPropSpecInfo(ULONG scan_id, EPartPropSpecInfoType type,
						  gpos::Ref<IMDId> rool_rel_mdid)
			: m_scan_id(scan_id), m_type(type), m_root_rel_mdid(rool_rel_mdid)
		{
			GPOS_ASSERT(m_root_rel_mdid != nullptr);

			CMemoryPool *mp = COptCtxt::PoctxtFromTLS()->Pmp();
			m_selector_ids = GPOS_NEW(mp) CBitSet(mp);
		}

		~SPartPropSpecInfo() override
		{
		}

		// hash function
		ULONG
		HashValue() const
		{
			ULONG ulHash = m_root_rel_mdid->HashValue();

			ulHash =
				gpos::CombineHashes(ulHash, gpos::HashValue<ULONG>(&m_scan_id));
			if (m_selector_ids)
			{
				ulHash = gpos::CombineHashes(
					ulHash, gpos::HashPtr<CBitSet>(m_selector_ids.get()));
			}
			if (m_filter_expr)
			{
				ulHash = gpos::CombineHashes(
					ulHash, CExpression::HashValue(m_filter_expr));
			}

			return ulHash;
		}

		IOstream &OsPrint(IOstream &os) const;

		// used for determining equality in memo (e.g in optimization contexts)
		BOOL Equals(const gpos::Ref<SPartPropSpecInfo>) const;

		BOOL FSatisfies(const gpos::Ref<SPartPropSpecInfo>) const;

		// used for sorting SPartPropSpecInfo in an array
		static INT CmpFunc(const void *val1, const void *val2);
	};

	// partition required/derived info, sorted by scanid
	using UlongToSPartPropSpecInfoMap =
		gpos::UnorderedMap<ULONG *, gpos::Ref<SPartPropSpecInfo>>;

	using UlongToSPartPropSpecInfoMapIter =
		gpos::UnorderedMap<ULONG *,
						   gpos::Ref<SPartPropSpecInfo>>::LegacyIterator;

	gpos::Ref<UlongToSPartPropSpecInfoMap> m_part_prop_spec_infos = nullptr;

	// Present scanids (for easy lookup)
	gpos::Ref<CBitSet> m_scan_ids = nullptr;

public:
	CPartitionPropagationSpec(const CPartitionPropagationSpec &) = delete;

	// ctor
	CPartitionPropagationSpec(CMemoryPool *mp);

	// dtor
	~CPartitionPropagationSpec() override;

	// append enforcers to dynamic array for the given plan properties
	void AppendEnforcers(CMemoryPool *mp, CExpressionHandle &exprhdl,
						 gpos::Ref<CReqdPropPlan> prpp,
						 gpos::Ref<CExpressionArray> pdrgpexpr,
						 gpos::Ref<CExpression> pexpr) override;

	// hash function
	ULONG
	HashValue() const override
	{
		ULONG ulHash = 0;

		UlongToSPartPropSpecInfoMapIter hmulpi(m_part_prop_spec_infos);
		while (hmulpi.Advance())
		{
			const gpos::Ref<SPartPropSpecInfo> info = hmulpi.Value();
			ulHash = gpos::CombineHashes(ulHash, info->HashValue());
		}

		return ulHash;
	}

	// extract columns used by the partition propagation spec
	gpos::Ref<CColRefSet>
	PcrsUsed(CMemoryPool *mp) const override
	{
		// return an empty set
		return GPOS_NEW(mp) CColRefSet(mp);
	}

	// property type
	EPropSpecType
	Epst() const override
	{
		return EpstPartPropagation;
	}

	BOOL
	Contains(ULONG scan_id) const
	{
		return m_scan_ids->Get(scan_id);
	}

	BOOL ContainsAnyConsumers() const;

	// equality check to determine compatibility of derived & required properties
	BOOL Equals(const gpos::Ref<CPartitionPropagationSpec> ppps) const;

	// satisfies function
	BOOL FSatisfies(const gpos::Ref<CPartitionPropagationSpec> pps_reqd) const;

	// Check if there is an unsupported part prop spec between two properties
	BOOL IsUnsupportedPartSelector(
		const gpos::Ref<CPartitionPropagationSpec> pps_reqd) const;



	gpos::Ref<SPartPropSpecInfo> FindPartPropSpecInfo(ULONG scan_id) const;

	void Insert(ULONG scan_id, EPartPropSpecInfoType type,
				gpos::Ref<IMDId> rool_rel_mdid, gpos::Ref<CBitSet> selector_ids,
				gpos::Ref<CExpression> expr);

	void Insert(gpos::Ref<SPartPropSpecInfo> other);

	void InsertAll(gpos::Ref<CPartitionPropagationSpec> pps);

	void InsertAllowedConsumers(gpos::Ref<CPartitionPropagationSpec> pps,
								gpos::Ref<CBitSet> allowed_scan_ids);

	void InsertAllExcept(gpos::Ref<CPartitionPropagationSpec> pps,
						 ULONG scan_id);

	const gpos::Ref<CBitSet> SelectorIds(ULONG scan_id) const;

	// is partition propagation required
	BOOL
	FPartPropagationReqd() const
	{
		return true;
	}

	// print
	IOstream &OsPrint(IOstream &os) const override;

	void InsertAllResolve(gpos::Ref<CPartitionPropagationSpec> pSpec);
};	// class CPartitionPropagationSpec

}  // namespace gpopt

#endif	// !GPOPT_CPartitionPropagationSpec_H

// EOF
