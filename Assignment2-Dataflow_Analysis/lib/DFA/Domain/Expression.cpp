#include "../../../include/DFA/Domain/Expression.h"

using namespace llvm;
using dfa::Expression;

raw_ostream &operator<<(raw_ostream &Outs, const Expression &Expr) {
  Outs << "[" << Instruction::getOpcodeName(Expr.Opcode) << " ";
  Expr.LHS->printAsOperand(Outs, false);
  Outs << ", ";
  Expr.RHS->printAsOperand(Outs, false);
  Outs << "]";
  return Outs;
}

void Expression::Initializer::visitBinaryOperator(BinaryOperator &BO) {

  /// @todo(CSCD70) Please complete this method.
   if (isa<BinaryOperator>(BO)) {
    Expression expr(*dyn_cast<BinaryOperator>(&BO));
    if (DomainIdMap.count(expr) == 0) {
      DomainVector.push_back(expr);
      DomainIdMap.insert(std::make_pair(expr, DomainVector.size() - 1));
    }
  }
}
