#include "LocalOpts.h"
#include <llvm/IR/InstrTypes.h>


using namespace llvm;


int getShift(int64_t x){
  if(x < 0 || (x & (x - 1)) != 0)
    return -1;

  int i = 0;
  while(x > 1){
    x >>= 1;
    ++i;
  }
  return i;
}


PreservedAnalyses StrengthReductionPass::run([[maybe_unused]] Function &F,
                                             FunctionAnalysisManager &) {

  /// @todo(CSCD70) Please complete this method.
  /// x * 2 ; x * 4
  /// x / 2 ; x / 4
  int StrengthOptNum = 0;
  for(auto &BB : F){
    for(auto &I : BB){
      if(I.getNumOperands() == 2){

        Value *op1 = I.getOperand(0);
        Value *op2 = I.getOperand(1);

        int64_t ConstVal1, ConstVal2;

        if(isa<ConstantInt>(op1))
          ConstVal1 = dyn_cast<ConstantInt>(op1)->getSExtValue();

        if(isa<ConstantInt>(op2))
          ConstVal2 = dyn_cast<ConstantInt>(op2)->getSExtValue();

        bool StrengthFlag = true;

        switch (I.getOpcode()) {
          case Instruction::Mul:
            // x * 2 ^ n ==> x << n 
            // shl x , n 
            if(isa<ConstantInt>(op2) && getShift(ConstVal2) != -1){
              I.replaceAllUsesWith(
                BinaryOperator::Create(
                Instruction::Shl, op1,
                ConstantInt::getSigned(I.getType(), getShift(ConstVal2)), 
                "shl", &I)
              );
            }

            else if(isa<ConstantInt>(op1) && getShift(ConstVal1) != -1){
              I.replaceAllUsesWith(
                BinaryOperator::Create(
                Instruction::Shl, op2,
                ConstantInt::getSigned(I.getType(), getShift(ConstVal1)), 
                "shl", &I)
              );
            }

            else{
              StrengthFlag = false;
            }

            break;

          case Instruction::SDiv:
            // x / 2 ^ n ==> x >> n 
            // lshr x , n 
            if(isa<ConstantInt>(op2) && getShift(ConstVal2) != -1){
              I.replaceAllUsesWith(
                BinaryOperator::Create(
                Instruction::LShr, op1,
                ConstantInt::getSigned(I.getType(), getShift(ConstVal2)), 
                "lshr", &I)
              );
            }
            else{
              StrengthFlag = false;
            }

            break;

          default:
            StrengthFlag = false;
            break;
        }

        if(StrengthFlag)
          ++StrengthOptNum;

      }
    }
  }

  outs() << "Strength Reduction: " << StrengthOptNum << "\n";
  return PreservedAnalyses::none();
}
