#include "DFA.h"
#include "../include/DFA/Domain/Variable.h"
#include "../include/DFA/Flow/Framework.h"

using namespace llvm;

AnalysisKey Liveness::Key;

void Liveness::initializeDomainFromInst(const llvm::Instruction &Inst) {
  for (const auto &Op : Inst.operands()) {
    /* Only care the instruction-define value and Argument */
    if (isa<Instruction>(Op) || isa<Argument>(Op)) {
      dfa::Variable VAR(Op);
      if (DomainIdMap.count(VAR) == 0) {
        DomainVector.push_back(VAR);
        DomainIdMap.insert(std::make_pair(VAR, DomainVector.size() - 1));
      }
    }
  }
}

bool Liveness::transferFunc(const Instruction &Inst, const DomainVal_t &IDV,
                             DomainVal_t &ODV) {

  /// @todo(CSCD70) Please complete this method.
  DomainVal_t Tmp = IDV;

  //如果该BasicBlock中含有br，我们要额外处理(会合并到phi指令中)
  //如果phi指令中的value并不是来自本BasicBlock中，我们可以认为在本BasicBlock中该Value不活跃
  //想想把，如果该value不在本BasicBlock中，自然可以设置该Value为不活跃
  //如果该Value是本次处理的Instruction，通过transfer可以设置为活跃，处理结果没有问题
  const llvm::BasicBlock *parent = Inst.getParent();
  const Instruction *BR = parent->getTerminator();

  if(BR){
    unsigned NumberOfSuccessor = BR->getNumSuccessors();
    for(unsigned idx = 0; idx < NumberOfSuccessor; ++idx){
      const llvm::BasicBlock *Successor = BR->getSuccessor(idx);

      for(const Instruction &I : *Successor){
        if(I.getOpcode() == Instruction::PHI){ //phi指令只在开头

          const llvm::PHINode *node = dyn_cast<llvm::PHINode>(&I);
          unsigned Number = node->getNumIncomingValues();
          for(unsigned i = 0; i < Number; ++i){
            const llvm::BasicBlock *prev = node->getIncomingBlock(i);
            if(prev != parent){
              auto value = node->getIncomingValue(i);
              auto iter = DomainIdMap.find(value);
              if(iter != DomainIdMap.end()){
                Tmp[iter->second].Value = false;
              }
            }
          }
        }else{
          break;
        }
      }
    }
  }
  // gen U (IN - def)

  const Value *ValueInst = dyn_cast<llvm::Value>(&Inst);

  for(auto &Var : DomainVector){
    if(Var.Var == ValueInst){
      Tmp[DomainIdMap.find(Var)->second].Value = false;
    }  
  }

  for(auto &Op : Inst.operands()){
    if(isa<Instruction>(Op) || isa<Argument>(Op)){
      Tmp[DomainIdMap.find(dfa::Variable(Op))->second].Value = true;
    }
  }

  for(std::size_t i = 0; i < Tmp.size(); ++i){
    if(Tmp[i].Value != ODV[i].Value){
      ODV = Tmp;
      return true;
    }
  }

  return false;
}
