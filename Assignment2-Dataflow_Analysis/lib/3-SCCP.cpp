#include "DFA.h"
#include "../include/DFA/Domain/Variable.h"
#include "../include/DFA/Flow/Framework.h"
#include "../include/DFA/MeetOp.h"
#include "llvm/IR/InstrTypes.h"

using namespace llvm;

AnalysisKey SCCP::Key;

void SCCP::initializeDomainFromInst(const llvm::Instruction &Inst) {
  if(!(&Inst)->getType()->isVoidTy()){
    dfa::Variable VAR(&Inst);
    if(DomainIdMap.count(VAR) == 0){
      DomainVector.push_back(VAR);
      DomainIdMap.insert(std::make_pair(VAR, DomainVector.size() - 1));
    }
  }

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

void SCCP::handleBO(const Instruction &Inst, const DomainVal_t &IDV, dfa::ConstValue &res){
  Value *op1 = Inst.getOperand(0);
  Value *op2 = Inst.getOperand(1);

  int64_t ConstVal1, ConstVal2;
  if(isa<llvm::ConstantInt>(op2)){
    ConstVal2 = dyn_cast<llvm::ConstantInt>(op2)->getSExtValue();
  }else{
    dfa::Variable var2 = dfa::Variable(op2);
    if(!IDV[DomainIdMap.find(var2)->second].isConst()){
      res = IDV[DomainIdMap.find(var2)->second];
      return;
    }
    ConstVal2 = IDV[DomainIdMap.find(var2)->second].getConst();
  }

  if(isa<llvm::ConstantInt>(op1)){
    ConstVal1 = dyn_cast<llvm::ConstantInt>(op1)->getSExtValue();
  }else{
    dfa::Variable var1 = dfa::Variable(op1);
    if(!IDV[DomainIdMap.find(var1)->second].isConst() && ConstVal2 == 0){
      switch(Inst.getOpcode()){
        case Instruction::SDiv:
          res = dfa::ConstValue::getUndef();
          return;
        default:
          res = dfa::ConstValue::getNac();
         return;
      }
    }else if(!IDV[DomainIdMap.find(var1)->second].isConst()){
      res = IDV[DomainIdMap.find(var1)->second];
      return;
    }
    ConstVal1 = IDV[DomainIdMap.find(var1)->second].getConst();
  }

  switch(Inst.getOpcode()){
    case Instruction::Add:
      res = dfa::ConstValue::getConst(ConstVal1 + ConstVal2);
      break;
    case Instruction::Sub:
      res = dfa::ConstValue::getConst(ConstVal1 - ConstVal2);
      break;
    case Instruction::Mul:
      res = dfa::ConstValue::getConst(ConstVal1 * ConstVal2);
      break;
    case Instruction::SDiv:
      res = dfa::ConstValue::getConst(ConstVal1 / ConstVal2);
      break;
    default: 
      // @Todo
      break;
  }

  return;
}


void SCCP::handleCMP(const Instruction &Inst, const DomainVal_t &IDV, dfa::ConstValue &res){
  Value *op1 = Inst.getOperand(0);
  Value *op2 = Inst.getOperand(1);

  int64_t ConstVal1, ConstVal2;
  if(isa<llvm::ConstantInt>(op2)){
    ConstVal2 = dyn_cast<llvm::ConstantInt>(op2)->getSExtValue();
  }else{
    dfa::Variable var2 = dfa::Variable(op2);
    if(!IDV[DomainIdMap.find(var2)->second].isConst()){
      res = IDV[DomainIdMap.find(var2)->second];
      return;
    }
    ConstVal2 = IDV[DomainIdMap.find(var2)->second].getConst();
  }

  if(isa<llvm::ConstantInt>(op1)){
    ConstVal1 = dyn_cast<llvm::ConstantInt>(op1)->getSExtValue();
  }else{
    dfa::Variable var1 = dfa::Variable(op1);
    if(!IDV[DomainIdMap.find(var1)->second].isConst()){
      res = IDV[DomainIdMap.find(var1)->second];
      return;
    }
    ConstVal1 = IDV[DomainIdMap.find(var1)->second].getConst();
  }

  switch(Inst.getOpcode()){
    case CmpInst::Predicate::ICMP_SGT:
      res = dfa::ConstValue::getConst(ConstVal1 > ConstVal2);
      break;
    case CmpInst::Predicate::ICMP_SLT:
      res = dfa::ConstValue::getConst(ConstVal1 < ConstVal2);
      break;
    default: 
      // @Todo
      break;
  }
  return;
}

void SCCP::handlePHI(const Instruction &Inst, const DomainVal_t &IDV, dfa::ConstValue &res){
  Value *op1 = Inst.getOperand(0);
  Value *op2 = Inst.getOperand(1);

  int64_t ConstVal1, ConstVal2;
  if(isa<llvm::ConstantInt>(op2)){
    ConstVal2 = dyn_cast<llvm::ConstantInt>(op2)->getSExtValue();
  }else{
    dfa::Variable var2 = dfa::Variable(op2);
    if(!IDV[DomainIdMap.find(var2)->second].isConst()){
      res = IDV[DomainIdMap.find(var2)->second];
      return;
    }
    ConstVal2 = IDV[DomainIdMap.find(var2)->second].getConst();
  }

  if(isa<llvm::ConstantInt>(op1)){
    ConstVal1 = dyn_cast<llvm::ConstantInt>(op1)->getSExtValue();
  }else{
    dfa::Variable var1 = dfa::Variable(op1);
    if(!IDV[DomainIdMap.find(var1)->second].isConst()){
      res = IDV[DomainIdMap.find(var1)->second];
      return;
    }
    ConstVal1 = IDV[DomainIdMap.find(var1)->second].getConst();
  }

  switch(Inst.getOpcode()){
    case Instruction::Add:
      res = dfa::ConstValue::getConst(ConstVal1 + ConstVal2);
      break;
    case Instruction::Sub:
      res = dfa::ConstValue::getConst(ConstVal1 - ConstVal2);
      break;
    case Instruction::Mul:
      res = dfa::ConstValue::getConst(ConstVal1 * ConstVal2);
      break;
    case Instruction::SDiv:
      res = dfa::ConstValue::getConst(ConstVal1 / ConstVal2);
      break;
    default: 
      // @Todo
      break;
  }
}

bool SCCP::transferFunc(const Instruction &Inst, const DomainVal_t &IDV,
                             DomainVal_t &ODV) {

  /// @todo(CSCD70) Please complete this method.
  DomainVal_t Tmp = IDV;

  // x = 3  ==> {true, 3}
  // x = y  ==> {IDV[DomainIdMap.find(x)->second] = IDV[DomainIdMap.find(y)->second]}
  // x = y op z  ==> decided by f(y, z)
  //           y.isconst && z.isconst    {true, y op z}
  //f(y , z)   y.isnac || z.isnac        {false, Nac}
  //           op = /%    z == 0         {false, Undef}
  
  // gen U (IN - def)
 if((&Inst)->getType()->isVoidTy())
    return false;

  const Value *ValueInst = dyn_cast<llvm::Value>(&Inst);

  for(auto &Var : DomainVector){
    if(Var.Var == ValueInst){
      Tmp[DomainIdMap.find(Var)->second] = dfa::ConstValue::getUndef();
    }  
  }

  dfa::Variable var(ValueInst);

  if(isa<BinaryOperator>(&Inst)){
    handleBO(Inst, Tmp, Tmp[DomainIdMap.find(var)->second]);
  }

  if(isa<ICmpInst>(&Inst)){
    handleCMP(Inst, Tmp, Tmp[DomainIdMap.find(var)->second]);
  }

  if(isa<PHINode>(&Inst)){
    //handlePHI(Inst, Tmp, Tmp[DomainIdMap.find(var)->second]); 
    Tmp[DomainIdMap.find(var)->second] = dfa::ConstValue::getNac();
  }

  if(isa<CallInst>(&Inst)){
    Tmp[DomainIdMap.find(var)->second] = dfa::ConstValue::getNac();
  }

  for(std::size_t i = 0; i < Tmp.size(); ++i){
    if(Tmp[i] != ODV[i]){
      ODV = Tmp;
      return true;
    }
  }

  return false;
}
