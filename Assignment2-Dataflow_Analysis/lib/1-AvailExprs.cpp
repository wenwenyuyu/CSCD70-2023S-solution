#include "DFA.h"
#include "../include/DFA/Domain/Expression.h"
#include "../include/DFA/Flow/Framework.h"

using namespace llvm;

AnalysisKey AvailExprs::Key;

bool AvailExprs::transferFunc(const Instruction &Inst, const DomainVal_t &IDV,
                             DomainVal_t &ODV) {

  /// @todo(CSCD70) Please complete this method.
  DomainVal_t tmp = IDV;

  if (isa<BinaryOperator>(Inst)) {
    dfa::Expression expr(*dyn_cast<BinaryOperator>(&Inst));

    auto iter = DomainIdMap.find(expr);
    if (iter != DomainIdMap.end()) {
      tmp[iter->second].Value = true;
    }
  }

  for(std::size_t i = 0; i < tmp.size(); ++i){
    if(tmp[i].Value != ODV[i].Value){
      ODV = tmp;
      return true;
    }
  }

  return false;
}
