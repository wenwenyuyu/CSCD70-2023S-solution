/**
 * @file Loop Invariant Code Motion
 */
#include <llvm/Analysis/LoopPass.h>
#include <llvm/Analysis/ValueTracking.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Dominators.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/ADT/SmallVector.h>

using namespace llvm;

namespace {

class LoopInvariantCodeMotion final : public LoopPass {
private:
  DominatorTree *DT;
  std::list<Instruction *> InvariantList;
public:
  static char ID;

  LoopInvariantCodeMotion() : LoopPass(ID) {}

  virtual void getAnalysisUsage(AnalysisUsage &AU) const override {
    /**
     * @todo(CSCD70) Request the dominator tree and the loop simplify pass.
     */
    AU.addRequired<DominatorTreeWrapperPass>();
    AU.addRequired<LoopInfoWrapperPass>();
    AU.setPreservesCFG();
  }

  /**
   * @todo(CSCD70) Please finish the implementation of this method.
   */
  virtual bool runOnLoop(Loop *L, LPPassManager &LPM) override {
    DT = &(getAnalysis<DominatorTreeWrapperPass>().getDomTree());
    if(!L->getLoopPreheader())
      return false;

    InvariantList = {};
    bool HasChanged;
    bool Move = false;
    do{
      HasChanged = false;
      for(BasicBlock *BB : L->getBlocks()){
        LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
        if(LI.getLoopFor(BB) == L){
          for(Instruction &I : *BB){
            if(isInvariant(&I, L) && (std::find(InvariantList.begin(), InvariantList.end(), &I) == InvariantList.end())){
              HasChanged = true;
              InvariantList.push_back(std::move(&I));
            }
          }
        }
      }
    }while(HasChanged);

    int moveCount = 0;
    for(Instruction *I : InvariantList){
      if(isDomianExitBlock(I, L)){
        moveToPreHead(I, L);
        Move = true;
        moveCount++;
      }
    }

    return Move;
  }

  bool isDomianExitBlock(Instruction *I, Loop *L){
    SmallVector<BasicBlock *, 0> ExitBlocks;
    BasicBlock *from = I->getParent();
    L->getExitBlocks(ExitBlocks);
    for(BasicBlock *to : ExitBlocks){
      if(!DT->dominates(from, to))
        return false;
    }
    return true;
  }
  
  bool moveToPreHead(Instruction *I, Loop *L){
    BasicBlock *prehead = L->getLoopPreheader();
    I->moveBefore(prehead->getTerminator());
    return false;
  }

  bool isInvariant(Instruction *I, Loop *L){
    bool isInvariant = true;
    for(Value *op : I->operands()){
      if(!isa<Constant>(op) && !isa<Argument>(op)){
         if(const Instruction *Inst = dyn_cast<Instruction>(op)){
          //如果该变量在内部定义且不是循环不变量
          if(L->contains(Inst->getParent()) && (std::find(InvariantList.begin(), InvariantList.end(), Inst) == InvariantList.end()))
            isInvariant = false;
        }
      }
    }

    return isSafeToSpeculativelyExecute(I)
    && !I->mayReadFromMemory()
    && !isa<LandingPadInst>(I)
    && isInvariant;
  }

};

char LoopInvariantCodeMotion::ID = 0;
RegisterPass<LoopInvariantCodeMotion> X("loop-invariant-code-motion",
                                        "Loop Invariant Code Motion");

} // anonymous namespace
