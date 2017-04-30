#include "discrete_tomography.h"
#include "visitors/standard_visitor.hxx"
using namespace LP_MP;
LP_MP_CONSTRUCT_SOLVER_WITH_INPUT_AND_VISITOR_SAT(FMC_DT, DiscreteTomographyTextInput::ParseProblem, StandardTighteningVisitor);

