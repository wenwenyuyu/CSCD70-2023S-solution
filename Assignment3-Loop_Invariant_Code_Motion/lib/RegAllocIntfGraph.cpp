/**
 * @file Interference Graph Register Allocator
 */
#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/CodeGen/CalcSpillWeights.h>
#include <llvm/CodeGen/LiveIntervals.h>
#include <llvm/CodeGen/LiveRangeEdit.h>
#include <llvm/CodeGen/LiveRegMatrix.h>
#include <llvm/CodeGen/LiveStacks.h>
#include <llvm/CodeGen/MachineBlockFrequencyInfo.h>
#include <llvm/CodeGen/MachineDominators.h>
#include <llvm/CodeGen/MachineFunctionPass.h>
#include <llvm/CodeGen/MachineLoopInfo.h>
#include <llvm/CodeGen/RegAllocRegistry.h>
#include <llvm/CodeGen/RegisterClassInfo.h>
#include <llvm/CodeGen/Spiller.h>
#include <llvm/CodeGen/TargetRegisterInfo.h>
#include <llvm/CodeGen/VirtRegMap.h>
#include <llvm/InitializePasses.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>

#include <cmath>
#include <queue>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <stack>
using namespace llvm;

namespace llvm {

void initializeRAIntfGraphPass(PassRegistry &Registry);

} // namespace llvm

namespace std {

template <> //
struct hash<Register> {
  size_t operator()(const Register &Reg) const {
    return DenseMapInfo<Register>::getHashValue(Reg);
  }
};

template <> //
struct greater<LiveInterval *> {
  bool operator()(LiveInterval *const &LHS, LiveInterval *const &RHS) const {
    /**
     * @todo(CSCD70) Please finish the implementation of this function that is
     *               used for determining whether one live interval has spill
     *               cost greater than the other.
     */
    return LHS->weight() > RHS->weight();
  }
};

} // namespace std

namespace {

class RAIntfGraph;

class AllocationHints {
private:
  SmallVector<MCPhysReg, 16> Hints;

public:
  AllocationHints(RAIntfGraph *const RA, const LiveInterval *const LI);
  SmallVectorImpl<MCPhysReg>::iterator begin() { return Hints.begin(); }
  SmallVectorImpl<MCPhysReg>::iterator end() { return Hints.end(); }
};

class RAIntfGraph final : public MachineFunctionPass,
                          private LiveRangeEdit::Delegate {
private:
  MachineFunction *MF;

  SlotIndexes *SI;
  VirtRegMap *VRM;
  const TargetRegisterInfo *TRI;
  MachineRegisterInfo *MRI;
  RegisterClassInfo RCI;
  LiveRegMatrix *LRM;
  MachineLoopInfo *MLI;
  LiveIntervals *LIS;

  /**
   * @brief Interference Graph
   */
  class IntfGraph {
  private:
    RAIntfGraph *RA;

    /// Interference Relations
    std::multimap<LiveInterval *, std::unordered_set<Register>,
                  std::greater<LiveInterval *>>
        IntfRels;

    /**
     * @brief  Try to materialize all the virtual registers (internal).
     *
     * @return (nullptr, VirtPhysRegMap) in the case when a successful
     *         materialization is made, (LI, *) in the case when unsuccessful
     *         (and LI is the live interval to spill)
     *
     * @sa tryMaterializeAll
     */
    using MaterializeResult_t =
        std::tuple<LiveInterval *,
                   std::unordered_map<LiveInterval *, MCPhysReg>>;
    MaterializeResult_t tryMaterializeAllInternal();
    MCRegister selectOrSplit(LiveInterval *const interval, SmallVectorImpl<Register> *const split_regs);
    bool spillInterferences(LiveInterval *const LI, MCRegister PhysReg, SmallVectorImpl<Register> *.const SplitVirtRegs);
  public:
    explicit IntfGraph(RAIntfGraph *const RA) : RA(RA) {}
    /**
     * @brief Insert a virtual register @c Reg into the interference graph.
     */
    void insert(const Register &Reg);
    /**
     * @brief Erase a virtual register @c Reg from the interference graph.
     *
     * @sa RAIntfGraph::LRE_CanEraseVirtReg
     */
    void erase(const Register &Reg);
    /**
     * @brief Build the whole graph.
     */
    void build();
    /**
     * @brief Try to materialize all the virtual registers.
     */
    void updateWeight();
    void simplify();
    void tryMaterializeAll();
    void clear() { IntfRels.clear(); }
  } G;

  SmallPtrSet<MachineInstr *, 32> DeadRemats;
  std::unique_ptr<Spiller> SpillerInst;
  std::set<Register> SpillerRegSet;
  std::map<Register, bool> isOnStack;
  std::stack<LiveInterval*> RuningStack;

  void postOptimization() {
    SpillerInst->postOptimization();
    for (MachineInstr *const DeadInst : DeadRemats) {
      LIS->RemoveMachineInstrFromMaps(*DeadInst);
      DeadInst->eraseFromParent();
    }
    DeadRemats.clear();
    G.clear();
  }

  friend class AllocationHints;
  friend class IntfGraph;

  /// The following two methods are inherited from @c LiveRangeEdit::Delegate
  /// and implicitly used by the spiller to edit the live ranges.
  bool LRE_CanEraseVirtReg(Register Reg) override {
    /**
     * @todo(cscd70) Please implement this method.
     */
    // If the virtual register has been materialized, undo its physical
    // assignment and erase it from the interference graph.
    if(VRM->hasPhys(Reg)){
        VRM->clearVirt(Reg);
        G.erase(Reg);
        return true;
      }
    return false;
  }
  void LRE_WillShrinkVirtReg(Register Reg) override {
    /**
     * @todo(cscd70) Please implement this method.
     */
    // If the virtual register has been materialized, undo its physical
    // assignment and re-insert it into the interference graph.
    if(VRM->hasPhys(Reg)){
        VRM->clearVirt(Reg);
        G.insert(Reg);
        return;
      }
  }

public:
  static char ID;

  StringRef getPassName() const override {
    return "Interference Graph Register Allocator";
  }

  RAIntfGraph() : MachineFunctionPass(ID), G(this) {}

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    MachineFunctionPass::getAnalysisUsage(AU);
    AU.setPreservesCFG();
#define REQUIRE_AND_PRESERVE_PASS(PassName)                                    \
  AU.addRequired<PassName>();                                                  \
  AU.addPreserved<PassName>()

    REQUIRE_AND_PRESERVE_PASS(SlotIndexes);
    REQUIRE_AND_PRESERVE_PASS(VirtRegMap);
    REQUIRE_AND_PRESERVE_PASS(LiveIntervals);
    REQUIRE_AND_PRESERVE_PASS(LiveRegMatrix);
    REQUIRE_AND_PRESERVE_PASS(LiveStacks);
    REQUIRE_AND_PRESERVE_PASS(AAResultsWrapperPass);
    REQUIRE_AND_PRESERVE_PASS(MachineDominatorTree);
    REQUIRE_AND_PRESERVE_PASS(MachineLoopInfo);
    REQUIRE_AND_PRESERVE_PASS(MachineBlockFrequencyInfo);
  }

  MachineFunctionProperties getRequiredProperties() const override {
    return MachineFunctionProperties().set(
        MachineFunctionProperties::Property::NoPHIs);
  }
  MachineFunctionProperties getClearedProperties() const override {
    return MachineFunctionProperties().set(
        MachineFunctionProperties::Property::IsSSA);
  }

  bool runOnMachineFunction(MachineFunction &MF) override;
}; // class RAIntfGraph

AllocationHints::AllocationHints(RAIntfGraph *const RA,
                                 const LiveInterval *const LI) {
  const TargetRegisterClass *const RC = RA->MRI->getRegClass(LI->reg());

  /**
   * @todo(CSCD70) Please complete this part by constructing the allocation
   *               hints, similar to the tutorial example.
   */
  ArrayRef<MCPhysReg> Order = RA->RCI.getOrder(RC);
  bool IsHardHint = RA->TRI->getRegAllocationHints(LI->reg(), Order, Hints, *RA->MF, RA->VRM, RA->LRM);
  if(!IsHardHint){
    for(const MCPhysReg &PhysReg : Order){
      Hints.push_back(PhysReg);
    }
  }
  outs() << "Hint Registers for Class " << RA->TRI->getRegClassName(RC)
         << ": [";
  for (const MCPhysReg &PhysReg : Hints) {
    outs() << RA->TRI->getRegAsmName(PhysReg) << ", ";
  }
  outs() << "]\n";
}

bool RAIntfGraph::runOnMachineFunction(MachineFunction &MF) {
  outs() << "************************************************\n"
         << "* Machine Function\n"
         << "************************************************\n";
  SI = &getAnalysis<SlotIndexes>();
  for (const MachineBasicBlock &MBB : MF) {
    MBB.print(outs(), SI);
    outs() << "\n";
  }
  outs() << "\n\n";

  this->MF = &MF;

  VRM = &getAnalysis<VirtRegMap>();
  TRI = &VRM->getTargetRegInfo();
  MRI = &VRM->getRegInfo();
  MRI->freezeReservedRegs(MF);
  LIS = &getAnalysis<LiveIntervals>();
  LRM = &getAnalysis<LiveRegMatrix>();
  RCI.runOnMachineFunction(MF);
  MLI = &getAnalysis<MachineLoopInfo>();

  VirtRegAuxInfo VRAI(MF, *LIS, *VRM, getAnalysis<MachineLoopInfo>(),
                      getAnalysis<MachineBlockFrequencyInfo>());
  VRAI.calculateSpillWeightsAndHints();
  SpillerInst.reset(createInlineSpiller(*this, MF, *VRM, VRAI));

  G.build();
  G.tryMaterializeAll();

  postOptimization();
  return true;
}

void RAIntfGraph::IntfGraph::insert(const Register &Reg) {
  /**
   * @todo(CSCD70) Please implement this method.
   */
  // 1. Collect all VIRTUAL registers that interfere with 'Reg'.
  // 2. Collect all PHYSICAL registers that interfere with 'Reg'.
  // 3. Insert 'Reg' into the graph.
  auto interval = &(RA->LIS->getInterval(Reg));
  IntfRels.insert({interval, {}});
  std::unordered_set<Register> ConfiltReg;
  for(std::size_t i = 0; i < RA->MRI->getNumVirtRegs(); ++i){
    Register reg = Register::index2VirtReg(i);
    if(reg == Reg || RA->MRI->reg_nodbg_empty(reg))
      continue;
    LiveInterval *RegLive = &(RA->LIS->getInterval(reg));
    if(interval->overlaps(RegLive)){
      auto tmp = IntfRels.find(RegLive);
      if(tmp != IntfRels.end()){
        tmp->second.insert(Reg);
        ConfiltReg.insert(reg);
      }
    }
  }

  auto &result = IntfRels.find(Reg);
  result->second = ConfiltReg;
}

void RAIntfGraph::IntfGraph::erase(const Register &Reg) {
  /**
   * @todo(cscd70) Please implement this method.
   */
  // 1. ∀n ∈ neighbors(Reg), erase 'Reg' from n's interfering set and update its
  //    weights accordingly.
  // 2. Erase 'Reg' from the interference graph.
  auto &interval = RA->LIS->getInterval(Reg);
  auto index = IntfRels.find(&interval);
  if(index == IntfRels.end())
    return;
  std::unordered_set<Register> &target = index->second;
  std::vector<Register> toErase;
  for(auto &reg : target){
    toErase.push_back(reg);
  }
  for(auto &reg : toErase){
    IntfRels.find(&(RA->LIS->getInterval(reg)))->second.erase(Reg);
  }
  IntfRels.erase(&interval);
}

void RAIntfGraph::IntfGraph::build() {
  /**
   * @todo(CSCD70) Please implement this method.
   */
  for(std::size_t i = 0 ; i < RA->MRI->getNumVirtRegs(); ++i){
    Register reg = Register::index2VirtReg(i);
    if(RA->SpillerRegSet.count(reg) || RA->MRI->reg_nodbg_empty(reg))
      continue;
    RA->isOnStack[reg] = false;
    insert(reg);
  }
}

bool RAIntfGraph::IntfGraph::spillInterferences(LiveInterval *const LI, MCRegister PhysReg, 
                                                SmallVectorImpl<Register> *const SplitVirtRegs){
  SmallVector<const LiveInterval *, 8> IntLIs;

  for(MCRegUnitIterator Units(PhysReg, RA->TRI); Units.isValid(); ++Units){
    LiveIntervalUnion::Query &Q = RA->LRM->query(*LI, *Units);
    for(const LiveInterval *const IntfLI : reverse(Q.interferingVRegs())){
      if(!IntfLI->isSpillable() || IntfLI->weight() > LI->weight()){
        return false;
      }
      IntLIs.push_back(IntfLI);
    }
  }

  for(unsigned IntfIdx = 0; IntfIdx < IntLIs.size(); ++IntfIdx){
    const LiveInterval *const LTToSpill = IntLIs[IntfIdx];
    if(!RA->VRM->hasPhys(LTToSpill->reg())){
      continue;
    }

    RA->LRM->unassign(*LTToSpill);
    LiveRangeEdit LRE(LTToSpill, *SplitVirtRegs, *RA->MF, *RA->LIS, RA->VRM, this, &DeadRemats);
    RA->SpillerInst->spill(RA->LRE);

  }
  return true;
}

MCRegister RAIntfGraph::IntfGraph::selectOrSplit(LiveInterval *const interval,
                                                 SmallVectorImpl<Register> *const split_regs){
  ArrayRef<MCRegister> Order = RA->RCI.getOrder(RA->MF->getRegInfo.getRegClass(interval->reg()));
  SmallVector<MCRegister, 16> Hints;
  bool IsHardHint = RA->TRI->getRegAllocationHints(interval->reg(), Order, Hints, *RA->MF,RA->VRM, RA->LRM);

  if(!IsHardHint){
    for(auto mcreg : Order){
      Hints.push_back(mcreg);
    }
  }
  
  SmallVector<MCRegister, 8> PhysRegSpillCandidates;
  for(auto PhyReg : Hints){
    switch(RA->LRM->checkInterference(*interval, PhyReg)){
      case llvm::LiveRegMatrix::IK_Free:
        return PhyReg;
      case llvm::LiveRegMatrix::IK_VirtReg:
        PhysRegSpillCandidates.push_back(PhyReg);
        continue;
      default:
        continue;
    }
  }
   for(MCRegister PhysReg : PhysRegSpillCandidates){
    if(!spillInterferences(LI, PhysReg, split_regs)){
      continue;
    }
    return PhysReg;
  }
  LiveRangeEdit LRE(interval, *split_regs, *RA->MF, *RA->LIS, RA->VRM, RA, &RA->DeadRemats);
  RA->SpillerInst->spill(LRE);

  return 0;

}

void RAIntfGraph::IntfGraph::updateWeight() {       // 这一部分算是直接对ppt上公式的实现
    // 更新Weight map, 这个map指的是Spill对应的Weight
    for (auto &inter_map: IntfRels) {
        LiveInterval *interval = inter_map.first;
        Register reg = interval->reg();
        double new_spill_weight = 0;
        // 通过虚拟寄存器的def-use来计算出来Weight
        for (auto reg_inst_it = RA->MRI->reg_instr_begin(reg);
                    reg_inst_it != RA->MRI->reg_instr_end(); ++reg_inst_it) {
            MachineInstr *McInst = &(*reg_inst_it);
            unsigned loop_depth = RA->MLI->getLoopDepth(McInst->getParent());
            auto read_write = McInst->readsWritesVirtualRegister(reg);  // 得出当前该指令对外层循环中的reg的读写情况
            new_spill_weight += (read_write.first + read_write.second) * pow(10, loop_depth);
        }
        auto degree_cnt = static_cast<double>(IntfRels.find(interval)->second.size());
        RegsWeightMap[reg] = new_spill_weight / degree_cnt;
    }
}

RAIntfGraph::IntfGraph::MaterializeResult_t
RAIntfGraph::IntfGraph::tryMaterializeAllInternal() {
  std::unordered_map<LiveInterval *, MCPhysReg> PhysRegAssignment;

  /**
   * @todo(CSCD70) Please implement this method.
   */
  // ∀r ∈ IntfRels.keys, try to materialize it. If successful, cache it in
  // PhysRegAssignment, else mark it as to be spilled.
  LiveInterval *rt_interval = nullptr;
  while(!RA->RuningStack.empty()){
    LiveInterval *interval = RA->RuningStack.top();
    RA->RuningStack.pop();
    RA->LRM->invalidateVirtRegs();
    SmallVector<Register, 4> split_vir_regs;

    MCRegister PhysReg = selectOrSplit(interval, &split_vir_regs);
    if(PhysReg){
      RA->LRM->assign(*interval, PhysReg);
    }else{
      rt_interval = interval;
    }

    for(Register Reg : split_vir_regs){
      LiveInterval *LI = &RA->LIS->getInterval(Reg);
      if(RA->MRI->reg_nodbg_empty(LI->reg())){
        RA->LIS->removeInterval(LI->reg());
        continue;
      }
      insert(Reg);
    }
  }
  return std::make_tuple(rt_interval, PhysRegAssignment);
}

void RAIntfGraph::IntfGraph::tryMaterializeAll() {
  std::unordered_map<LiveInterval *, MCPhysReg> PhysRegAssignment;

  /**
   * @todo(CSCD70) Please implement this method.
   */
  // Keep looping until a valid assignment is made. In the case of spilling,
  // modify the interference graph accordingly.
  using Vir2PhysAssignmentMap = std::unordered_set<LiveInterval *, MCRegister>;
  bool spill_finish = false;
  size_t rount_cnt = 0;
  while(!spill_finish && rount_cnt < 0){
    build();
    updateWeight();
    simplify();
    auto select_tuple = tryMaterializeAllInternal();
    auto liveinterval = std::get<LiveInterval *>(select_tuple);
    if(liveinterval == nullptr){
      spill_finish = true;
      PhysRegAssignment = std::get<Vir2PhysAssignmentMap>(select_tuple);
    }
    rount_cnt++;
  }
  for (auto &PhysRegAssignPair : PhysRegAssignment) {
    RA->LRM->assign(*PhysRegAssignPair.first, PhysRegAssignPair.second);
  }

}

void RAIntfGraph::IntfGraph::simplify() {
    // 当整个graph都为空时, 直接返回
    if (IntfRels.empty()) {
        return;
    }
    LiveInterval *select_reg_inter = nullptr; // 被选中的寄存器
    double min_weight = -1;
    // 首先需要遍历一遍graph,从中选出来一个, 优先找出来Weight小的
    for (auto &inter_maps : IntfRels) {
        LiveInterval *inter = inter_maps.first;
        Register reg = inter->reg();
        // 首先判断此结点, 是否已经处于栈中
        if (RA->IsOnStack[reg]) {
            continue;
        }
        if (min_weight == -1 || RegsWeightMap[reg] < min_weight) {
            select_reg_inter = inter;
            min_weight = RegsWeightMap[reg];
        }
    }
    if (min_weight == -1) {     // 表示的没有选出来
        return;
    }
    // 然后压入栈中
    Register select_reg = select_reg_inter->reg();
    RA->IsOnStack[select_reg] = true;
    RA->RuningStack.push(select_reg_inter);
    // 之后调节graph, 从graph中移除该结点
    IntfRels.erase(select_reg_inter);
    simplify(); // 递归地调用
}

char RAIntfGraph::ID = 0;

static RegisterRegAlloc X("intfgraph", "Interference Graph Register Allocator",
                          []() -> FunctionPass * { return new RAIntfGraph(); });

} // anonymous namespace

INITIALIZE_PASS_BEGIN(RAIntfGraph, "regallointfgraph",
                      "Interference Graph Register Allocator", false, false)
INITIALIZE_PASS_DEPENDENCY(SlotIndexes)
INITIALIZE_PASS_DEPENDENCY(VirtRegMap)
INITIALIZE_PASS_DEPENDENCY(LiveIntervals)
INITIALIZE_PASS_DEPENDENCY(LiveRegMatrix)
INITIALIZE_PASS_DEPENDENCY(LiveStacks);
INITIALIZE_PASS_DEPENDENCY(AAResultsWrapperPass);
INITIALIZE_PASS_DEPENDENCY(MachineDominatorTree);
INITIALIZE_PASS_DEPENDENCY(MachineLoopInfo);
INITIALIZE_PASS_DEPENDENCY(MachineBlockFrequencyInfo);
INITIALIZE_PASS_END(RAIntfGraph, "regallointfgraph",
                    "Interference Graph Register Allocator", false, false)
