#include "graph_matching.h"
#include "visitors/standard_visitor.hxx"
using FMC_INST = FMC_MP_T<PairwiseConstruction::Left>;
LP_MP_CONSTRUCT_SOLVER_WITH_INPUT_AND_VISITOR_MP_ROUNDING(FMC_INST, UaiGraphMatchingInput::ParseProblemMP<FMC_INST>, StandardTighteningVisitor);
