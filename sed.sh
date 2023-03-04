git grep 'public CRefCount'

git grep -l 'CConstraintConjunction \*' -- *.h -- *.cpp | xargs sed -ie 's/CConstraintConjunction \*/gpos::Ref<CConstraintConjunction> /g'
git grep -l 'CConstraintDisjunction \*' -- *.h -- *.cpp | xargs sed -ie 's/CConstraintDisjunction \*/gpos::Ref<CConstraintDisjunction> /g'
git grep -l 'CConstraintArray \*' -- *.h -- *.cpp | xargs sed -ie 's/CConstraintArray \*/gpos::Ref<CConstraintArray> /g'
git grep -l 'CExpressionArray \*' -- *.h -- *.cpp | xargs sed -ie 's/CExpressionArray \*/gpos::Ref<CExpressionArray> /g'
git grep -l 'CExpressionArrays \*' -- *.h -- *.cpp | xargs sed -ie 's/CExpressionArrays \*/gpos::Ref<CExpressionArrays> /g'
git grep -l 'UlongToColRefMap \*' -- *.h -- *.cpp | xargs sed -ie 's/UlongToColRefMap \*/gpos::Ref<UlongToColRefMap> /g'
git grep -l 'SPartPropSpecInfo \*' -- *.h -- *.cpp | xargs sed -ie 's/SPartPropSpecInfo \*/gpos::Ref<SPartPropSpecInfo> /g'
git grep -l 'IMDTypeBool \*' -- *.h -- *.cpp | xargs sed -ie 's/IMDTypeBool \*/gpos::Ref<IMDTypeBool> /g'
git grep -l 'IMDType \*' -- *.h -- *.cpp | xargs sed -ie 's/IMDType \*/gpos::Ref<IMDType> /g'
git grep -l 'IMDTypeGeneric \*' -- *.h -- *.cpp | xargs sed -ie 's/IMDTypeGeneric \*/gpos::Ref<IMDTypeGeneric> /g'
git grep -l 'COptimizationContext \*' -- *.h -- *.cpp | xargs sed -ie 's/COptimizationContext \*/gpos::Ref<COptimizationContext> /g'
git grep -l 'CDrvdPropCtxtPlan \*' -- *.h -- *.cpp | xargs sed -ie 's/CDrvdPropCtxtPlan \*/gpos::Ref<CDrvdPropCtxtPlan> /g'
git grep -l 'CCostContext \*' -- *.h -- *.cpp | xargs sed -ie 's/CCostContext \*/gpos::Ref<CCostContext> /g'
git grep -l 'CTreeNodeArray \*' -- *.h -- *.cpp | xargs sed -ie 's/CTreeNodeArray \*/gpos::Ref<CTreeNodeArray> /g'
git grep -l 'CTreeNode \*' -- *.h -- *.cpp | xargs sed -ie 's/CTreeNode \*/gpos::Ref<CTreeNode> /g'
git grep -l 'CDrvdPropArray \*' -- *.h -- *.cpp | xargs sed -ie 's/CDrvdPropArray \*/gpos::Ref<CDrvdPropArray> /g'
git grep -l 'COptimizationContextArray \*' -- *.h -- *.cpp | xargs sed -ie 's/COptimizationContextArray \*/gpos::Ref<COptimizationContextArray> /g'
git grep -l 'EsetStates \*' -- *.h -- *.cpp | xargs sed -ie 's/EsetStates \*/gpos::Ref<EsetStates> /g'
git grep -l 'EsetEvents \*' -- *.h -- *.cpp | xargs sed -ie 's/EsetEvents \*/gpos::Ref<EsetEvents> /g'
git grep -l 'IMDRelation \*' -- *.h -- *.cpp | xargs sed -ie 's/IMDRelation \*/gpos::Ref<IMDRelation> /g'
git grep -l 'IMDScalarOp \*' -- *.h -- *.cpp | xargs sed -ie 's/IMDScalarOp \*/gpos::Ref<IMDScalarOp> /g'
git grep -l 'IMDFunction \*' -- *.h -- *.cpp | xargs sed -ie 's/IMDFunction \*/gpos::Ref<IMDFunction> /g'
git grep -l 'IMDAggregate \*' -- *.h -- *.cpp | xargs sed -ie 's/IMDAggregate \*/gpos::Ref<IMDAggregate> /g'
git grep -l 'IMDTrigger \*' -- *.h -- *.cpp | xargs sed -ie 's/IMDTrigger \*/gpos::Ref<IMDTrigger> /g'
git grep -l 'IMDIndex \*' -- *.h -- *.cpp | xargs sed -ie 's/IMDIndex \*/gpos::Ref<IMDIndex> /g'
git grep -l 'IMDCheckConstraint \*' -- *.h -- *.cpp | xargs sed -ie 's/IMDCheckConstraint \*/gpos::Ref<IMDCheckConstraint> /g'
git grep -l 'IMDColStats \*' -- *.h -- *.cpp | xargs sed -ie 's/IMDColStats \*/gpos::Ref<IMDColStats> /g'
git grep -l 'IMDRelStats \*' -- *.h -- *.cpp | xargs sed -ie 's/IMDRelStats \*/gpos::Ref<IMDRelStats> /g'
git grep -l 'IMDCast \*' -- *.h -- *.cpp | xargs sed -ie 's/IMDCast \*/gpos::Ref<IMDCast> /g'
git grep -l 'IMDScCmp \*' -- *.h -- *.cpp | xargs sed -ie 's/IMDScCmp \*/gpos::Ref<IMDScCmp> /g'
git grep -l 'CMDArrayCoerceCastGPDB \*' -- *.h -- *.cpp | xargs sed -ie 's/CMDArrayCoerceCastGPDB \*/gpos::Ref<CMDArrayCoerceCastGPDB> /g'
git grep -l 'IMDCacheObject \*' -- *.h -- *.cpp | xargs sed -ie 's/IMDCacheObject \*/gpos::Ref<IMDCacheObject> /g'
git grep -l 'CMDTypeInt4GPDB \*' -- *.h -- *.cpp | xargs sed -ie 's/CMDTypeInt4GPDB \*/gpos::Ref<CMDTypeInt4GPDB> /g'
git grep -l 'IStatisticsArray \*' -- *.h -- *.cpp | xargs sed -ie 's/IStatisticsArray \*/gpos::Ref<IStatisticsArray> /g'
git grep -l 'CPatternNode \*' -- *.h -- *.cpp | xargs sed -ie 's/CPatternNode \*/gpos::Ref<CPatternNode> /g'
git grep -l 'CTEPredsMap \*' -- *.h -- *.cpp | xargs sed -ie 's/CTEPredsMap \*/gpos::Ref<CTEPredsMap> /g'
git grep -l 'CDistributionSpecArray \*' -- *.h -- *.cpp | xargs sed -ie 's/CDistributionSpecArray \*/gpos::Ref<CDistributionSpecArray> /g'
git grep -l 'CColRefArray \*' -- *.h -- *.cpp | xargs sed -ie 's/CColRefArray \*/gpos::Ref<CColRefArray> /g'
git grep -l 'CDXLColRefArray \*' -- *.h -- *.cpp | xargs sed -ie 's/CDXLColRefArray \*/gpos::Ref<CDXLColRefArray> /g'
git grep -l 'SAdjBucketBoundary \*' -- *.h -- *.cpp | xargs sed -ie 's/SAdjBucketBoundary \*/gpos::Ref<SAdjBucketBoundary> /g'
git grep -l 'CColRef \*' -- *.h -- *.cpp | xargs sed -ie 's/CColRef \*/gpos::Ref<CColRef> /g'
git grep -l 'CColRefTable \*' -- *.h -- *.cpp | xargs sed -ie 's/CColRefTable \*/gpos::Ref<CColRefTable> /g'
git grep -l 'XXX \*' -- *.h -- *.cpp | xargs sed -ie 's/XXX \*/gpos::Ref<XXX> /g'



header hell
git grep -l 'CGroupExpression.h"' -- *.cpp | xargs sed -ie 's/CGroupExpression.h"/CGroupExpression.h"\n#include "gpopt\/base\/CDrvdPropCtxtPlan.h"\n/g'
git grep -l 'CDrvdPropCtxtPlan.h"' -- *.cpp | xargs sed -ie 's/CDrvdPropCtxtPlan.h"/CDrvdPropCtxtPlan.h"\n#include "gpopt/search/CGroupExpression.h"\n/g'
git grep -l 'owner.h"' -- *.cpp | xargs sed -ie 's/owner.h"/owner.h"\n#include "gpopt\/base\/CDrvdPropCtxtPlan.h"\n/g'
git grep -l 'owner.h"' -- *.cpp | xargs sed -ie 's/owner.h"/owner.h"\n#include "gpopt\/base\/CDrvdPropCtxtPlan.h"\n#include "gpopt\/search\/CGroupExpression.h"\n/g'

magic:

git grep 'public CRefCount' | awk -F ':' '{print $2}' | awk '{print $2}' > magic.sh
[edit forumla]


# git grep 'public CRefCount' | awk -F ':' '{print $2}' | awk  '{print $2}' | xargs printf 'git grep "public %s"\n' >> magic2.sh


magic2.sh
git grep 'dynamic_cast<' src/backend/ | awk -F ':' '{print $2}' | awk -F '<' '{print $2}' | awk '{print $1}' | sort | uniq
git grep -l 'CDistributionSpec \*' -- *.h -- *.cpp | xargs sed -ie 's/CDistributionSpec \*/gpos::Ref<CDistributionSpec> /g'
git grep -l 'UlongToDoubleMap \*' -- *.h -- *.cpp | xargs sed -ie 's/UlongToDoubleMap \*/gpos::Ref<UlongToDoubleMap> /g'
git grep -l 'IMdIdArray \*' -- *.h -- *.cpp | xargs sed -ie 's/IMdIdArray \*/gpos::Ref<IMdIdArray> /g'
git grep -l 'ULongPtrArray \*' -- *.h -- *.cpp | xargs sed -ie 's/ULongPtrArray \*/gpos::Ref<ULongPtrArray> /g'
git grep -l 'CBitSet \*' -- *.h -- *.cpp | xargs sed -ie 's/CBitSet \*/gpos::Ref<CBitSet> /g'
git grep -l 'IMDTypeInt2 \*' -- *.h -- *.cpp | xargs sed -ie 's/IMDTypeInt2 \*/gpos::Ref<IMDTypeInt2> /g'
git grep -l 'IMDTypeInt4 \*' -- *.h -- *.cpp | xargs sed -ie 's/IMDTypeInt4 \*/gpos::Ref<IMDTypeInt4> /g'
git grep -l 'IMDTypeInt8 \*' -- *.h -- *.cpp | xargs sed -ie 's/IMDTypeInt8 \*/gpos::Ref<IMDTypeInt8> /g'
git grep -l 'IMDTypeOid \*' -- *.h -- *.cpp | xargs sed -ie 's/IMDTypeOid \*/gpos::Ref<IMDTypeOid> /g'
git grep -l 'CStatsPredPtrArry \*' -- *.h -- *.cpp | xargs sed -ie 's/CStatsPredPtrArry \*/gpos::Ref<CStatsPredPtrArry> /g'
git grep -l 'dynamic_cast<gpos::Ref<' -- *.h -- *.cpp | xargs sed -ie 's/dynamic_cast<gpos::Ref<\(.*\)> >/gpos::dyn_cast<\1>/g'
