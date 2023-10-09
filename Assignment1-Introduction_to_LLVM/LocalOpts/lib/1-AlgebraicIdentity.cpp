#include "LocalOpts.h"

using namespace llvm;

PreservedAnalyses AlgebraicIdentityPass::run([[maybe_unused]] Function &F,
                                             FunctionAnalysisManager &) {

  //Algebraic Identity
  // x + 0 = 0 + x = x ; x - 0 = x , x - x = 0 ; 
  // x * 0 = 0 * x = 0 , x * 1 = 1 * x = x ;
  // x / x = 1
  int AlgebraicOptNum = 0;
  
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

        bool TargetAlgeFlag = true;
        switch (I.getOpcode()) {
          case Instruction::Add:
            if(isa<ConstantInt>(op2) && ConstVal2 == 0){
              I.replaceAllUsesWith(op1);
            }else if(isa<ConstantInt>(op1) && ConstVal1 == 0){
              I.replaceAllUsesWith(op2);
            }else{
              TargetAlgeFlag = false;
            }

            break;

          case Instruction::Sub:
            if(isa<ConstantInt>(op2) && ConstVal2 == 0){
              I.replaceAllUsesWith(op1);
            }else if(op1 == op2){
              I.replaceAllUsesWith(ConstantInt::getSigned(I.getType(), 0));
            }else{
              TargetAlgeFlag = false;
            }

            break;

          case Instruction::Mul:
            if(isa<ConstantInt>(op2) && ConstVal2 == 1){
              I.replaceAllUsesWith(op1);
            }else if(isa<ConstantInt>(op1) && ConstVal1 == 1){
              I.replaceAllUsesWith(op2);
            }else{
              TargetAlgeFlag = false;
            }

            break;

          case Instruction::SDiv:
            if(isa<ConstantInt>(op2) && ConstVal2 == 1){
              I.replaceAllUsesWith(op1);
            }else if(op1 == op2){
              I.replaceAllUsesWith(ConstantInt::getSigned(I.getType(), 1));
            }else{
              TargetAlgeFlag = false;
            }
            
            break;


          default:
            TargetAlgeFlag = false;
            break;
        }

        if(TargetAlgeFlag)
          ++AlgebraicOptNum;

      }

    }
  }
  
  outs() << "define dso_local void @AlgebraicIdentity(i32 noundef %0) {" << "\n";
  outs() << "Algebraic Identity: " << AlgebraicOptNum << "\n";

  return PreservedAnalyses::none();
}
