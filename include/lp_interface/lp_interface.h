#ifndef LP_MP_LP_INTERFACE_HXX
#define LP_MP_LP_INTERFACE_HXX

#include "config.hxx"
#include "primal_solution_storage.hxx"

#ifdef USE_GUROBI
#include <gurobi_c++.h>
#else
#include "lp_auxiliary.hxx"
#endif

namespace LP_MP {

#ifdef USE_GUROBI
  typedef GRBVar LpVariable;
  typedef GRBLinExpr LinExpr;
#endif

    
  class LpInterfaceAdapter {
  public:

    LpInterfaceAdapter(){ }
    
    template<typename FACTOR_ITERATOR, typename MESSAGE_ITERATOR>
    LpInterfaceAdapter(FACTOR_ITERATOR factorBegin, FACTOR_ITERATOR factorEnd, MESSAGE_ITERATOR messageBegin, MESSAGE_ITERATOR messageEnd)
    { } 

    virtual LpVariable CreateAuxiliaryVariable(REAL lb,REAL ub,bool integer = false) = 0;

    virtual INDEX GetFactorSize() const = 0;
    virtual INDEX GetLeftFactorSize() const = 0;
    virtual INDEX GetRightFactorSize() const = 0;
    
    virtual LpVariable GetVariable(const INDEX i) const = 0;
    virtual LpVariable GetLeftVariable(const INDEX i) const = 0;
    virtual LpVariable GetRightVariable(const INDEX i) const = 0;
    
    virtual void SetVariableBound(LpVariable v,REAL lb,REAL ub,bool integer = false) = 0;
    
    virtual void addLinearEquality(LinExpr lhs,LinExpr rhs) = 0;
    virtual void addLinearInequality(LinExpr lhs,LinExpr rhs) = 0;
    
    virtual void solve() = 0;
    virtual void solve(PrimalSolutionStorage::Element primal) = 0;

    virtual void WriteLpModel(std::string name) = 0;
    
  };
}

#endif // LP_MP_LP_INTERFACE_HXX