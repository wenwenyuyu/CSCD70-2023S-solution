#pragma once // NOLINT(llvm-header-guard)

#include "4-LCM/LCM.h"

#include <DFA/Domain/Expression.h>
#include <DFA/Domain/Variable.h>
#include <DFA/Flow/ForwardAnalysis.h>
#include <DFA/Flow/BackwardAnalysis.h>
#include <DFA/MeetOp.h>

#include <llvm/IR/PassManager.h>

class AvailExprs final : public dfa::ForwardAnalysis<dfa::Expression, dfa::Bool,
                                                     dfa::Intersect<dfa::Bool>>,
                         public llvm::AnalysisInfoMixin<AvailExprs> {
private:
  using ForwardAnalysis_t = dfa::ForwardAnalysis<dfa::Expression, dfa::Bool,
                                                 dfa::Intersect<dfa::Bool>>;

  friend llvm::AnalysisInfoMixin<AvailExprs>;
  static llvm::AnalysisKey Key;

  std::string getName() const final { return "AvailExprs"; }
  bool transferFunc(const llvm::Instruction &, const DomainVal_t &,
                    DomainVal_t &) final;
  void initializeDomainFromInst(const llvm::Instruction &Inst) final;

public:
  using Result = typename ForwardAnalysis_t::AnalysisResult_t;
  using ForwardAnalysis_t::run;
};

class AvailExprsWrapperPass
    : public llvm::PassInfoMixin<AvailExprsWrapperPass> {
public:
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &FAM) {
    FAM.getResult<AvailExprs>(F);
    return llvm::PreservedAnalyses::all();
  }
};

/// @todo(CSCD70) Please complete the main body of the following passes, similar
///               to the Available Expressions pass above.
class Liveness final : public dfa::BackwardAnalysis<dfa::Variable, dfa::Bool, 
                                                    dfa::Union<dfa::Bool>>,
                       public llvm::AnalysisInfoMixin<Liveness>{
  private:
    using BackwardAnalysis_t = dfa::BackwardAnalysis<dfa::Variable, dfa::Bool, dfa::Union<dfa::Bool>>;

    friend llvm::AnalysisInfoMixin<Liveness>;
    static llvm::AnalysisKey Key;

    std::string getName() const final {return "Liveness";}
    bool transferFunc(const llvm::Instruction &, const DomainVal_t &, DomainVal_t &) final;
    void initializeDomainFromInst(const llvm::Instruction &Inst) final;

  public:
    using Result = typename BackwardAnalysis_t::AnalysisResult_t;
    using BackwardAnalysis_t::run;
};

class LivenessWrapperPass : public llvm::PassInfoMixin<LivenessWrapperPass> {
public:
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &FAM) {

    /// @todo(CSCD70) Get the result from the main body.
    FAM.getResult<Liveness>(F);
    return llvm::PreservedAnalyses::all();
  }
};


class SCCP final : public dfa::ForwardAnalysis<dfa::Variable, dfa::ConstValue,
                                                     dfa::ConstIntersect<dfa::ConstValue>>,
                         public llvm::AnalysisInfoMixin<SCCP> {
private:
  using ForwardAnalysis_t = dfa::ForwardAnalysis<dfa::Variable, dfa::ConstValue,
                                                 dfa::ConstIntersect<dfa::ConstValue>>;

  friend llvm::AnalysisInfoMixin<SCCP>;
  static llvm::AnalysisKey Key;

  std::string getName() const final { return "SCCP"; }
  bool transferFunc(const llvm::Instruction &, const DomainVal_t &,
                    DomainVal_t &) final;
  void initializeDomainFromInst(const llvm::Instruction &Inst) final;
  void handleBO(const llvm::Instruction &, const DomainVal_t &, dfa::ConstValue &);

  void handleCMP(const llvm::Instruction &, const DomainVal_t &, dfa::ConstValue &);
  void handlePHI(const llvm::Instruction &, const DomainVal_t &, dfa::ConstValue &);
public:
  using Result = typename ForwardAnalysis_t::AnalysisResult_t;
  using ForwardAnalysis_t::run;
};

class SCCPWrapperPass
    : public llvm::PassInfoMixin<SCCPWrapperPass> {
public:
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &FAM) {
    FAM.getResult<SCCP>(F);
    return llvm::PreservedAnalyses::all();
  }
};


