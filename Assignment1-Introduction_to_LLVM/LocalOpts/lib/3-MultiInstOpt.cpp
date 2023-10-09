#include "LocalOpts.h"

using namespace llvm;

PreservedAnalyses MultiInstOptPass::run([[maybe_unused]] Function &F,
                                        FunctionAnalysisManager &) {

  //常量折叠相关blog:
  //https://zhuanlan.zhihu.com/p/644260335
  
  int NumConstFold = 0;

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

        if(isa<ConstantInt>(op1) && isa<ConstantInt>(op2)){
          bool ConstFoldFlag = true;

          switch (I.getOpcode()) {
            case Instruction::Add:
              I.replaceAllUsesWith(ConstantInt::getSigned(I.getType(), 
                                                             ConstVal1 + ConstVal2));
              break;
            case Instruction::Sub:
              I.replaceAllUsesWith(ConstantInt::getSigned(I.getType(), 
                                                             ConstVal1 - ConstVal2));
              break;
            
            case Instruction::Mul:
              I.replaceAllUsesWith(ConstantInt::getSigned(I.getType(), 
                                                             ConstVal1 * ConstVal2));
              break;


            case Instruction::SDiv:
              if(ConstVal2 != 0){
              I.replaceAllUsesWith(ConstantInt::getSigned(I.getType(), 
                                                             ConstVal1 / ConstVal2));
              }else{
                ConstFoldFlag = false;
              }
              break;

            default:
              ConstFoldFlag = false;
              break;
          }

          if(ConstFoldFlag)
            ++NumConstFold;
        }

      }
    }
  }

  outs() << "define dso_local void @MultiInstOpt(i32 noundef %0, i32 noundef %1) {" << "\n";
  outs() << "Muti Inst Opt: " << NumConstFold << "\n";

  return PreservedAnalyses::none();
}
