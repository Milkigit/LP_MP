#ifndef LP_MP_SIMPLEX_MARGINALIZATION_MESSAGE_HXX
#define LP_MP_SIMPLEX_MARGINALIZATION_MESSAGE_HXX

#include "LP_MP.h"
#include "factors_messages.hxx"
#include "vector.hxx"
#include "memory_allocator.hxx"
#include <cmath>
#include "config.hxx"

namespace LP_MP {

// specialized messages between UnarySimplexFactor and PairwiseSimplexFactor
  template<MessageSendingType TYPE,  bool PROPAGATE_PRIMAL_TO_LEFT = false, bool PROPAGATE_PRIMAL_TO_RIGHT = false, bool SUPPORT_INFINITY = true> 
  class UnaryPairwiseMessageLeft {
    public:
      UnaryPairwiseMessageLeft(const INDEX i1, const INDEX i2) : i1_(i1), i2_(i2) {} // the pairwise factor size
      // standard functions which take all possible arguments, to be replaced with minimal ones
      template<typename RIGHT_FACTOR, typename G2, bool ENABLE = TYPE == MessageSendingType::SRMP>
        typename std::enable_if<ENABLE,void>::type
        ReceiveMessageFromRight(const RIGHT_FACTOR& r, G2& msg) 
        {
          MinimizeRight(r,msg); 
        }
      template<typename LEFT_FACTOR, typename G2, bool ENABLE = TYPE == MessageSendingType::MPLP>
        typename std::enable_if<ENABLE,void>::type 
        ReceiveMessageFromLeft(const LEFT_FACTOR& l, G2& msg) 
        { 
          MaximizeLeft(l,msg); 
        }
      template<typename LEFT_FACTOR, typename G3, bool ENABLE = TYPE == MessageSendingType::SRMP>
        typename std::enable_if<ENABLE,void>::type
        SendMessageToRight(const LEFT_FACTOR& l, G3& msg, const REAL omega)
        { 
          MaximizeLeft(l,msg,omega); 
        }
      template<typename RIGHT_FACTOR, typename G3, bool ENABLE = TYPE == MessageSendingType::MPLP>
        typename std::enable_if<ENABLE,void>::type
        SendMessageToLeft(const RIGHT_FACTOR& r, G3& msg, const REAL omega) 
        { 
          MinimizeRight(r,msg,omega); 
        }

      // for primal computation as in TRW-S, we need to compute restricted messages as well
      template<typename RIGHT_FACTOR, typename G2, bool ENABLE = TYPE == MessageSendingType::SRMP>
        typename std::enable_if<ENABLE,void>::type
        ReceiveRestrictedMessageFromRight(const RIGHT_FACTOR& r, G2& msg) 
        {
           // we assume that only r.right_primal was assigned, r.left_primal not
           //assert(r.primal_[0] == i1_);
           if(r.primal()[1] < i2_ && r.primal()[0] >= i1_) {
              vector<REAL> msgs(i1_);
              for(INDEX x1=0; x1<i1_; ++x1) {
                 msgs[x1] = r(x1,r.primal()[1]);
              }
              msg -= msgs;
           }
        }

    // reparametrize left potential for i-th entry of msg
    // do zrobienia: put strides in here and below
    template<typename G>
    void RepamLeft(G& r, const REAL msg, const INDEX msg_dim)
    {
       assert(!std::isnan(msg));
       if(SUPPORT_INFINITY) {
          r[msg_dim] += normalize( msg );
       } else {
          r[msg_dim] += msg;
       }
       assert(!std::isnan(r[msg_dim]));
    }
    template<typename A1, typename A2>
    void RepamRight(A1& r, const A2& msgs)
    {
       for(INDEX x1=0; x1<r.dim1(); ++x1) {
          assert(!std::isnan(msgs[x1]));
           if(SUPPORT_INFINITY) {
              r.msg1(x1) += normalize( msgs[x1] );
           } else {
              r.msg2(x1) += msgs[x1];
           }
           assert(!std::isnan(r.msg1(x1)));
       }
    }
    template<typename G>
    void RepamRight(G& r, const REAL msg, const INDEX dim)
    {
       assert(!std::isnan(msg));
       if(SUPPORT_INFINITY) {
          r.msg1(dim) += normalize( msg );
       } else {
          r.msg1(dim) += msg;
       }
       assert(!std::isnan(r.msg1(dim)));
    }

    template<bool ENABLE = TYPE == MessageSendingType::SRMP, typename LEFT_FACTOR, typename RIGHT_FACTOR>
    //typename std::enable_if<ENABLE,void>::type
    void
    ComputeRightFromLeftPrimal(const LEFT_FACTOR& l, RIGHT_FACTOR& r)
    {
       assert(l.primal() < i1_);
       //assert(r.primal_[0] == i1_);
       r.primal()[0] = l.primal();
    }


    /*
    template<class LEFT_FACTOR_TYPE,class RIGHT_FACTOR_TYPE>
      void CreateConstraints(LpInterfaceAdapter* lp,LEFT_FACTOR_TYPE* LeftFactor,RIGHT_FACTOR_TYPE* RightFactor) const
      { 
        for(auto x1=0; x1<i1_ ; x1++){
          LinExpr lhs = lp->CreateLinExpr() + 0.0;
          LinExpr rhs = lp->CreateLinExpr() + 0.0;
          if(lp->IsLeftObjective(x1)){
            lhs += lp->GetLeftVariable(x1);
          }
          for(auto x2=0; x2<i2_; x2++){
            if(lp->IsRightObjective(x2*i1_ + x1)){
              rhs += lp->GetRightVariable(x2*i1_ + x1);
            }
          }
          lp->addLinearEquality(lhs,rhs);
        }
      }
      */

    template<typename SAT_SOLVER, typename LEFT_FACTOR, typename RIGHT_FACTOR>
    void construct_sat_clauses(SAT_SOLVER& s, const LEFT_FACTOR& l, const RIGHT_FACTOR& r, const sat_var left_begin, const sat_var right_begin) const
    {
       for(INDEX i=0; i<l.size(); ++i) {
          auto left_var = left_begin+i;
          auto right_var = right_begin+i;
          make_sat_var_equal(s, to_literal(left_var), to_literal(right_var)); 
       }
    }
 

  private:
    template<typename LEFT_FACTOR, typename G2>
    void MaximizeLeft(const LEFT_FACTOR& l, G2& msg, const REAL omega = 1.0)
    {
      for(INDEX x1=0; x1<i1_; ++x1) {
        msg[x1] -= omega*l[x1];
      }
    }
    template<typename RIGHT_FACTOR, typename G2>
    void MinimizeRight(const RIGHT_FACTOR& r, G2& msg, const REAL omega = 1.0)
    {
       vector<REAL> msgs(i1_,std::numeric_limits<REAL>::infinity());
       r.min_marginal_1(msgs);
       msg -= omega*msgs;
    }

    const INDEX i1_,i2_;
  };


  template<MessageSendingType TYPE,  bool PROPAGATE_PRIMAL_TO_LEFT = false, bool PROPAGATE_PRIMAL_TO_RIGHT = false, bool SUPPORT_INFINITY = true> 
  class UnaryPairwiseMessageRight {
    public:
      UnaryPairwiseMessageRight(const INDEX i1, const INDEX i2) : i1_(i1), i2_(i2) {} // the pairwise factor size
      // standard functions which take all possible arguments, to be replaced with minimal ones
      template<typename RIGHT_FACTOR, typename G2, bool ENABLE = TYPE == MessageSendingType::SRMP>
        typename std::enable_if<ENABLE,void>::type
        ReceiveMessageFromRight(const RIGHT_FACTOR& r, G2& msg) 
        {
          MinimizeRight(r,msg); 
        }
      template<typename LEFT_FACTOR, typename G2, bool ENABLE = TYPE == MessageSendingType::MPLP>
        typename std::enable_if<ENABLE,void>::type 
        ReceiveMessageFromLeft(const LEFT_FACTOR& l, G2& msg) 
        { 
          MaximizeLeft(l,msg); 
        }
      template<typename LEFT_FACTOR, typename G3, bool ENABLE = TYPE == MessageSendingType::SRMP>
        typename std::enable_if<ENABLE,void>::type
        SendMessageToRight(const LEFT_FACTOR& l, G3& msg, const REAL omega)
        { 
          MaximizeLeft(l,msg,omega); 
        }
      template<typename RIGHT_FACTOR, typename G3, bool ENABLE = TYPE == MessageSendingType::MPLP>
        typename std::enable_if<ENABLE,void>::type
        SendMessageToLeft(const RIGHT_FACTOR& r, G3& msg, const REAL omega) 
        { 
          MinimizeRight(r,msg,omega); 
        }

      // for primal computation as in TRW-S, we need to compute restricted messages as well
      template<typename RIGHT_FACTOR, typename G2, bool ENABLE = TYPE == MessageSendingType::SRMP>
        typename std::enable_if<ENABLE,void>::type
        ReceiveRestrictedMessageFromRight(const RIGHT_FACTOR& r, G2& msg) 
        {
           //assert(r.primal_[1] == i1_);
           if(r.primal()[0] < i1_ && r.primal()[1] >= i2_) {
              vector<REAL> msgs(i2_);
              for(INDEX x2=0; x2<i2_; ++x2) {
                 msgs[x2] = r(r.primal()[0],x2);
              }
              msg -= msgs;
           }
        }

    // reparametrize left potential for i-th entry of msg
    // do zrobienia: put strides in here and below
    template<typename G>
    void RepamLeft(G& l, const REAL msg, const INDEX msg_dim)
    {
       assert(!std::isnan(msg));
       if(SUPPORT_INFINITY) {
          l[msg_dim] += normalize( msg );
       } else {
          l[msg_dim] += msg;
       }
       assert(!std::isnan(l[msg_dim]));
    }
    template<typename A1, typename A2>
    void RepamRight(A1& r, const A2& msgs)
    {
      for(INDEX x2=0; x2<i2_; ++x2) {
         assert(!std::isnan(msgs[x2]));
         if(SUPPORT_INFINITY) {
            r.msg2(x2) += normalize( msgs[x2] );
         } else {
            r.msg2(x2) += msgs[x2];
         }
         assert(!std::isnan(r.msg2(x2)));
      }
    }
    template<typename G>
    void RepamRight(G& r, const REAL msg, const INDEX dim)
    {
       assert(!std::isnan(msg));
       if(SUPPORT_INFINITY) {
          r.msg2(dim) += normalize( msg );
       } else {
          r.msg2(dim) += msg;
       }
       assert(!std::isnan(r.msg2(dim)));
    }

    template<bool ENABLE = TYPE == MessageSendingType::SRMP, typename LEFT_FACTOR, typename RIGHT_FACTOR>
    //typename std::enable_if<ENABLE,void>::type
    void
    ComputeRightFromLeftPrimal(const LEFT_FACTOR& l, RIGHT_FACTOR& r)
    {
       assert(l.primal() < i2_);
       //assert(r.primal_[1] == i2_);
       r.primal()[1] = l.primal();
    }


    /*
    template<class LEFT_FACTOR_TYPE,class RIGHT_FACTOR_TYPE>
      void CreateConstraints(LpInterfaceAdapter* lp,LEFT_FACTOR_TYPE* LeftFactor,RIGHT_FACTOR_TYPE* RightFactor) const
      { 
        for(auto x2=0; x2<i2_ ; x2++){
          LinExpr lhs = lp->CreateLinExpr() + 0.0;
          LinExpr rhs = lp->CreateLinExpr() + 0.0;
          if(lp->IsLeftObjective(x2)){
            lhs += lp->GetLeftVariable(x2);
          }
          for(auto x1=0; x1<i1_; x1++){
            if(lp->IsRightObjective(x2*i1_ + x1)){
              rhs += lp->GetRightVariable(x2*i1_ + x1);
            }
          }
          lp->addLinearEquality(lhs,rhs);
        }
      }
      */
    template<typename SAT_SOLVER, typename LEFT_FACTOR, typename RIGHT_FACTOR>
    void construct_sat_clauses(SAT_SOLVER& s, const LEFT_FACTOR& l, const RIGHT_FACTOR& r, const sat_var left_begin, const sat_var right_begin) const
    {
       for(INDEX i=0; i<l.size(); ++i) {
          auto left_var = left_begin+i;
          auto right_var = right_begin + r.dim1() + i;
          make_sat_var_equal(s, to_literal(left_var), to_literal(right_var)); 
       }
    }
 

  private:
    template<typename LEFT_FACTOR, typename G2>
    void MaximizeLeft(const LEFT_FACTOR& l, G2& msg, const REAL omega = 1.0)
    {
      for(INDEX x2=0; x2<i2_; ++x2) {
        msg[x2] -= omega*l[x2];
      }
    }
    template<typename RIGHT_FACTOR, typename G2>
    void MinimizeRight(const RIGHT_FACTOR& r, G2& msg, const REAL omega = 1.0)
    {
       vector<REAL> msgs(i2_, std::numeric_limits<REAL>::infinity());//std::vector<REAL, stack_allocator<REAL>>;
       r.min_marginal_2(msgs);
       msg -= omega*msgs;
    }

    const INDEX i1_,i2_; // do zrobienia: these values are not needed, as they can be obtained from the factors whenever they are used
  };

  template<INDEX I1, INDEX I2, MessageSendingType MESSAGE_SENDING_TYPE>
  class PairwiseTripletMessage {
     public:
      PairwiseTripletMessage() {} 
      PairwiseTripletMessage(const INDEX, const INDEX, const INDEX) {} 
      ~PairwiseTripletMessage() {
         static_assert(I1 < I2 && I2 < 3,""); 
      } 

      // standard functions which take all possible arguments, to be replaced with minimal ones
      template<typename RIGHT_FACTOR, typename G2, bool ENABLE = MESSAGE_SENDING_TYPE == MessageSendingType::SRMP>
        typename std::enable_if<ENABLE,void>::type
        ReceiveMessageFromRight(const RIGHT_FACTOR& r, G2& msg) 
        {
          MinimizeRight(r,msg); 
        }
      template<typename LEFT_FACTOR, typename G2, bool ENABLE = MESSAGE_SENDING_TYPE == MessageSendingType::MPLP>
        typename std::enable_if<ENABLE,void>::type 
        ReceiveMessageFromLeft(const LEFT_FACTOR& l, G2& msg) 
        { 
          MaximizeLeft(l,msg); 
        }
      template<typename LEFT_FACTOR, typename G3, bool ENABLE = MESSAGE_SENDING_TYPE == MessageSendingType::SRMP>
        typename std::enable_if<ENABLE,void>::type
        SendMessageToRight(const LEFT_FACTOR& l, G3& msg, const REAL omega)
        { 
          MaximizeLeft(l,msg,omega); 
        }
      template<typename RIGHT_FACTOR, typename G3, bool ENABLE = MESSAGE_SENDING_TYPE == MessageSendingType::MPLP>
        typename std::enable_if<ENABLE,void>::type
        SendMessageToLeft(const RIGHT_FACTOR& r, G3& msg, const REAL omega) 
        { 
          MinimizeRight(r,msg,omega); 
        }

      // for primal computation as in TRW-S, we need to compute restricted messages as well
      template<typename RIGHT_FACTOR, typename G2, bool ENABLE = MESSAGE_SENDING_TYPE == MessageSendingType::SRMP>
        typename std::enable_if<ENABLE,void>::type
        ReceiveRestrictedMessageFromRight(const RIGHT_FACTOR& r, G2& msg, typename PrimalSolutionStorage::Element rightPrimal) 
        {
           throw std::runtime_error("rounding on pairwise factors is not currently supported");
           assert(false);
        }

      template<typename A1, typename A2>
      void RepamLeft(A1& l, const A2& msgs)
      {
         // do zrobienia: possibly use counter
         for(INDEX x1=0; x1<l.dim1(); ++x1) {
            for(INDEX x2=0; x2<l.dim2(); ++x2) {
               l.cost(x1,x2) += normalize( msgs(x1,x2) );
               assert(!std::isnan(l(x1,x2)));
            }
         }
      }
      template<typename A1, typename A2>
      void RepamRight(A1& r, const A2& msgs)
      {
         // do zrobienia: possibly use counter
         if(I1 == 0 && I2 == 1) {
            for(INDEX x1=0; x1<r.dim1(); ++x1) {
               for(INDEX x2=0; x2<r.dim2(); ++x2) {
                  assert(!std::isnan(msgs(x1,x2)));
                  r.msg12(x1,x2) += normalize( msgs(x1,x2) );
                  assert(!std::isnan(r.msg12(x1,x2)));
               }
            }
         } else
         if(I1 == 0 && I2 == 2) {
            for(INDEX x1=0; x1<r.dim1(); ++x1) {
               for(INDEX x2=0; x2<r.dim3(); ++x2) {
                  assert(!std::isnan(msgs(x1,x2)));
                  r.msg13(x1,x2) += normalize( msgs(x1,x2) );
                  assert(!std::isnan(r.msg13(x1,x2)));
               }
            }
         } else
         if(I1 == 1 && I2 == 2) {
            for(INDEX x1=0; x1<r.dim2(); ++x1) {
               for(INDEX x2=0; x2<r.dim3(); ++x2) {
                  assert(!std::isnan(msgs(x1,x2)));
                  r.msg23(x1,x2) += normalize( msgs(x1,x2) );
                  assert(!std::isnan(r.msg23(x1,x2)));
               }
            }
         } else {
            assert(false);
         }
      }

      template<bool ENABLE = MESSAGE_SENDING_TYPE == MessageSendingType::SRMP, typename LEFT_FACTOR, typename RIGHT_FACTOR>
      //typename std::enable_if<ENABLE,void>::type
      void
      ComputeRightFromLeftPrimal(const LEFT_FACTOR& l, RIGHT_FACTOR& r)
      {
         if(I1 == 0 && I2 == 1) {

            if(l.primal()[0] < l.dim1()) {
               r.primal()[0] = l.primal()[0];
            }
            if(l.primal()[1] < l.dim2()) {
               r.primal()[1] = l.primal()[1];
            }

         } else
         if(I1 == 0 && I2 == 2) {

            if(l.primal()[0] < l.dim1()) {
               r.primal()[0] = l.primal()[0];
            }
            if(l.primal()[1] < l.dim2()) {
               r.primal()[2] = l.primal()[1];
            }

         } else
         if(I1 == 1 && I2 == 2) {

            if(l.primal()[0] < l.dim1()) {
               r.primal()[1] = l.primal()[0];
            }
            if(l.primal()[1] < l.dim2()) {
               r.primal()[2] = l.primal()[1];
            }


         } else {
            assert(false);
         }
      }

      template<typename SAT_SOLVER, typename LEFT_FACTOR, typename RIGHT_FACTOR>
      void construct_sat_clauses(SAT_SOLVER& s, const LEFT_FACTOR& l, const RIGHT_FACTOR& r, const sat_var left_begin, const sat_var right_begin) const
      {
         if(I1 == 0 && I2 == 1) {

            for(INDEX x1=0; x1<l.dim1(); ++x1) {
               for(INDEX x2=0; x2<l.dim2(); ++x2) {
                  const auto left_var = left_begin + l.dim1() + l.dim2() + x1*l.dim2() + x2;
                  const auto right_var = right_begin + x1*l.dim2() + x2;
                  make_sat_var_equal(s, to_literal(left_var), to_literal(right_var));
               }
            }

         } else
            if(I1 == 0 && I2 == 2) {

               for(INDEX x1=0; x1<l.dim1(); ++x1) {
                  for(INDEX x2=0; x2<l.dim2(); ++x2) {
                     const auto left_var = left_begin + l.dim1() + l.dim2() + x1*l.dim2() + x2;
                     const auto right_var = right_begin + r.dim1()*r.dim2() + x1*l.dim2() + x2;
                     make_sat_var_equal(s, to_literal(left_var), to_literal(right_var));
                  }
               }

            } else
               if(I1 == 1 && I2 == 2) {

                  for(INDEX x1=0; x1<l.dim1(); ++x1) {
                     for(INDEX x2=0; x2<l.dim2(); ++x2) {
                        const auto left_var = left_begin + l.dim1() + l.dim2() + x1*l.dim2() + x2;
                        const auto right_var = right_begin + r.dim1()*r.dim2() + r.dim1()*r.dim3() + x1*l.dim2() + x2;
                        make_sat_var_equal(s, to_literal(left_var), to_literal(right_var));
             }
          }

       } else {
          assert(false); // not possible
       }
    }


  private:
    template<typename LEFT_FACTOR, typename G2>
    void MaximizeLeft(const LEFT_FACTOR& l, G2& msg, const REAL omega = 1.0)
    {
       msg -= omega*l;
    }
    template<typename RIGHT_FACTOR, typename G2>
    void MinimizeRight(const RIGHT_FACTOR& r, G2& msg, const REAL omega = 1.0)
    {
       if(I1 == 0 && I2 == 1) {

          matrix<REAL> msgs(r.dim1(), r.dim2(), std::numeric_limits<REAL>::infinity());
          r.min_marginal12(msgs);
          msg -= omega*msgs;

       } else
       if(I1 == 0 && I2 == 2) {

          matrix<REAL> msgs(r.dim1(), r.dim3(), std::numeric_limits<REAL>::infinity());
          r.min_marginal13(msgs);
          msg -= omega*msgs; 

       } else
       if(I1 == 1 && I2 == 2) {

          matrix<REAL> msgs(r.dim2(), r.dim3(), std::numeric_limits<REAL>::infinity());
          r.min_marginal23(msgs);
          msg -= omega*msgs; 

       } else {
          assert(false);
       }
    }

    
  };


// specialized messages for pairwise/triplet marginalization
template<MessageSendingType TYPE,  bool PROPAGATE_PRIMAL_TO_LEFT = false, bool PROPAGATE_PRIMAL_TO_RIGHT = false> 
class PairwiseTripletMessage12 {
   public:
      PairwiseTripletMessage12(const INDEX i1, const INDEX i2, const INDEX i3) : i1_(i1), i2_(i2), i3_(i3) {} // the pairwise factor size
      // standard functions which take all possible arguments, to be replaced with minimal ones
      template<typename RIGHT_FACTOR, typename G2, bool ENABLE = TYPE == MessageSendingType::SRMP>
         typename std::enable_if<ENABLE,void>::type
         ReceiveMessageFromRight(const RIGHT_FACTOR& r, G2& msg) 
         {
            MinimizeRight(r,msg); 
        }
      template<typename LEFT_FACTOR, typename G2, bool ENABLE = TYPE == MessageSendingType::MPLP>
        typename std::enable_if<ENABLE,void>::type 
        ReceiveMessageFromLeft(const LEFT_FACTOR& l, G2& msg) 
        { 
          MaximizeLeft(l,msg); 
        }
      template<typename LEFT_FACTOR, typename G3, bool ENABLE = TYPE == MessageSendingType::SRMP>
        typename std::enable_if<ENABLE,void>::type
        SendMessageToRight(const LEFT_FACTOR& l, G3& msg, const REAL omega)
        { 
          MaximizeLeft(l,msg,omega); 
        }
      template<typename RIGHT_FACTOR, typename G3, bool ENABLE = TYPE == MessageSendingType::MPLP>
        typename std::enable_if<ENABLE,void>::type
        SendMessageToLeft(const RIGHT_FACTOR& r, G3& msg, const REAL omega) 
        { 
          MinimizeRight(r,msg,omega); 
        }

      // for primal computation as in TRW-S, we need to compute restricted messages as well
      template<typename RIGHT_FACTOR, typename G2, bool ENABLE = TYPE == MessageSendingType::SRMP>
        typename std::enable_if<ENABLE,void>::type
        ReceiveRestrictedMessageFromRight(const RIGHT_FACTOR& r, G2& msg, typename PrimalSolutionStorage::Element rightPrimal) 
        {
           throw std::runtime_error("rounding on pairwise factors is not ucrrently supported");
           assert(false);
        }

    // reparametrize left potential for i-th entry of msg
    // do zrobienia: put strides in here and below
      /*
    template<typename G>
    void RepamLeft(G& l, const REAL msg, const INDEX msg_dim)
    {
      const INDEX x1 = msg_dim/i2_;
      const INDEX x2 = msg_dim%i2_;
      //if(SUPPORT_INFINITY) {
         l(x1,x2) += normalize( msg );
      //} else {
      //   l(x1,x2) += msg;
      //}
    }
    */
    template<typename A1, typename A2>
    void RepamLeft(A1& l, const A2& msgs)
    {
      // do zrobienia: possibly use counter
       for(INDEX x1=0; x1<i1_; ++x1) {
          for(INDEX x2=0; x2<i2_; ++x2) {
             l.cost(x1,x2) += normalize( msgs(x1,x2) );
             assert(!std::isnan(l(x1,x2)));
          }
       }
    }
    template<typename A1, typename A2>
    void RepamRight(A1& r, const A2& msgs)
    {
      // do zrobienia: possibly use counter
       for(INDEX x1=0; x1<i1_; ++x1) {
          for(INDEX x2=0; x2<i2_; ++x2) {
             r.msg12(x1,x2) += normalize( msgs(x1,x2) );
          }
       }
    }
    /*
    template<typename G>
    void RepamRight(G& r, const REAL msg, const INDEX dim)
    {
      const INDEX x1 = dim/i2_;
      const INDEX x2 = dim%i2_;
      r.msg12(x1,x2) += normalize( msg );
    }
    */

    template<bool ENABLE = TYPE == MessageSendingType::SRMP, typename LEFT_FACTOR, typename RIGHT_FACTOR>
    //typename std::enable_if<ENABLE,void>::type
    void
    ComputeRightFromLeftPrimal(const LEFT_FACTOR& l, RIGHT_FACTOR& r)
    {
      assert(r.dim1() == i1_ && r.dim2() == i2_ && r.dim3() == i3_);
      assert(l.size() == i1_*i2_);
      if(l.primal()[0] < i1_) {
         r.primal()[0] = l.primal()[0];
      }
      if(l.primal()[1] < i2_) {
         r.primal()[1] = l.primal()[1];
      }
    }


    /*
    template<class LEFT_FACTOR_TYPE,class RIGHT_FACTOR_TYPE>
      void CreateConstraints(LpInterfaceAdapter* lp,LEFT_FACTOR_TYPE* LeftFactor,RIGHT_FACTOR_TYPE* RightFactor) const
      { 
        for(auto x1=0; x1<i1_ ; x1++){
          for(auto x2=0; x2<i2_; x2++){
            LinExpr lhs = lp->CreateLinExpr() + 0.0;
            LinExpr rhs = lp->CreateLinExpr() + 0.0;
            if(lp->IsLeftObjective(x2*i1_ + x1)){
              lhs += lp->GetLeftVariable(x2*i1_ + x1);
            }
          for(auto x3=0; x3<i3_; x3++){
            if(lp->IsRightObjective(x3*i1_*i2_ + x2*i1_ + x1)){
              rhs += lp->GetRightVariable(x3*i1_+i2_ + x2*i1_ + x1);
            }
          }
          lp->addLinearEquality(lhs,rhs);
          }
        }
      }
      */

  private:
    template<typename LEFT_FACTOR, typename G2>
    void MaximizeLeft(const LEFT_FACTOR& l, G2& msg, const REAL omega = 1.0)
    {
       msg -= omega*l;
      //for(INDEX x2=0; x2<i2_; ++x2) {
      //  for(INDEX x1=0; x1<i1_; ++x1) {
      //    msg[x2*i1_ + x1] -= omega*l[x2*i1_ + x1];
      //  }
      //}
    }
    template<typename RIGHT_FACTOR, typename G2>
    void MinimizeRight(const RIGHT_FACTOR& r, G2& msg, const REAL omega = 1.0)
    {
       matrix<REAL> msgs(i1_,i2_, std::numeric_limits<REAL>::infinity());
       r.min_marginal12(msgs);
       msg -= omega*msgs;
    }

    const INDEX i1_,i2_, i3_;
  };

  template<MessageSendingType TYPE,  bool PROPAGATE_PRIMAL_TO_LEFT = false, bool PROPAGATE_PRIMAL_TO_RIGHT = false> 
  class PairwiseTripletMessage13 {
    public:
      PairwiseTripletMessage13(const INDEX i1, const INDEX i2, const INDEX i3) : i1_(i1), i2_(i2), i3_(i3) {} // the pairwise factor size
      // standard functions which take all possible arguments, to be replaced with minimal ones
      template<typename RIGHT_FACTOR, typename G2, bool ENABLE = TYPE == MessageSendingType::SRMP>
        typename std::enable_if<ENABLE,void>::type
        ReceiveMessageFromRight(const RIGHT_FACTOR& r, G2& msg) 
        {
          MinimizeRight(r,msg); 
        }
      template<typename LEFT_FACTOR, typename G2, bool ENABLE = TYPE == MessageSendingType::MPLP>
        typename std::enable_if<ENABLE,void>::type 
        ReceiveMessageFromLeft(const LEFT_FACTOR& l, G2& msg) 
        { 
          MaximizeLeft(l,msg); 
        }
      template<typename LEFT_FACTOR, typename G3, bool ENABLE = TYPE == MessageSendingType::SRMP>
        typename std::enable_if<ENABLE,void>::type
        SendMessageToRight(const LEFT_FACTOR& l, G3& msg, const REAL omega)
        { 
          MaximizeLeft(l,msg,omega); 
        }
      template<typename RIGHT_FACTOR, typename G3, bool ENABLE = TYPE == MessageSendingType::MPLP>
        typename std::enable_if<ENABLE,void>::type
        SendMessageToLeft(const RIGHT_FACTOR& r, G3& msg, const REAL omega) 
        { 
          MinimizeRight(r,msg,omega); 
        }

      // for primal computation as in TRW-S, we need to compute restricted messages as well
      template<typename RIGHT_FACTOR, typename G2, bool ENABLE = TYPE == MessageSendingType::SRMP>
        typename std::enable_if<ENABLE,void>::type
        ReceiveRestrictedMessageFromRight(const RIGHT_FACTOR& r, G2& msg, typename PrimalSolutionStorage::Element rightPrimal) 
        {
           throw std::runtime_error("rounding on pairwise factors is not ucrrently supported");
           assert(false);
        }

    // reparametrize left potential for i-th entry of msg
    // do zrobienia: put strides in here and below
    /*
    template<typename G>
    void RepamLeft(G& repamPot, const REAL msg, const INDEX msg_dim)
    {
      REAL msgn;
      if( std::isfinite(msg) ){ msgn = msg; }
      else{ msgn = std::numeric_limits<REAL>::infinity(); }
      const INDEX x1 = msg_dim/i3_;
      const INDEX x3 = msg_dim%i3_;
      repamPot(x1,x3) += msgn;
    }
    */
    template<typename A1, typename A2>
    void RepamLeft(A1& repamPot, const A2& msgs)
    {
       // do zrobienia: possibly use counter
       for(INDEX x1=0; x1<i1_; ++x1) {
          for(INDEX x3=0; x3<i3_; ++x3) {
             repamPot.cost(x1,x3) += normalize( msgs(x1,x3) );
          }
       }
    }
    template<typename A1, typename A2>
    void RepamRight(A1& repamPot, const A2& msgs)
    {
       // do zrobienia: possibly use counter
       for(INDEX x1=0; x1<i1_; ++x1) {
          for(INDEX x3=0; x3<i3_; ++x3) {
             repamPot.msg13(x1,x3) += normalize( msgs(x1,x3) );
          }
       }
    }
    /*
    template<typename G>
    void RepamRight(G& repamPot, const REAL msg, const INDEX dim)
    {
      REAL msgn;
      const INDEX x1 = dim % i1_;
      const INDEX x3 = dim / i1_;
      if( std::isfinite(msg) ){ msgn = msg; }
      else{ msgn = std::numeric_limits<REAL>::infinity(); }
      assert(false);
      //for(INDEX x2 = 0; x2<i2_; ++x2) {
      //  repamPot[x3*i2_*i1_ + x2*i1_ + x1] += msgn;
      //}
    }
    */

    template<bool ENABLE = TYPE == MessageSendingType::SRMP, typename LEFT_FACTOR, typename RIGHT_FACTOR>
    //typename std::enable_if<ENABLE,void>::type
    void
    ComputeRightFromLeftPrimal(const LEFT_FACTOR& l, RIGHT_FACTOR& r)
    {
      assert(r.dim1() == i1_ && r.dim2() == i2_ && r.dim3() == i3_);
      assert(l.size() == i1_*i3_);
      if(l.primal()[0] < i1_) {
         r.primal()[0] = l.primal()[0];
      }
      if(l.primal()[1] < i3_) {
         r.primal()[2] = l.primal()[1];
      }
    }


    /*
    template<class LEFT_FACTOR_TYPE,class RIGHT_FACTOR_TYPE>
      void CreateConstraints(LpInterfaceAdapter* lp,LEFT_FACTOR_TYPE* LeftFactor,RIGHT_FACTOR_TYPE* RightFactor) const
      { 
        for(auto x1=0; x1<i1_ ; x1++){
          for(auto x3=0; x3<i3_; x3++){
            LinExpr lhs = lp->CreateLinExpr() + 0.0;
            LinExpr rhs = lp->CreateLinExpr() + 0.0;
            if(lp->IsLeftObjective(x3*i1_ + x1)){
              lhs += lp->GetLeftVariable(x3*i1_ + x1);
            }
          for(auto x2=0; x2<i2_; x2++){
            if(lp->IsRightObjective(x3*i1_*i2_ + x2*i1_ + x1)){
              rhs += lp->GetRightVariable(x3*i1_+i2_ + x2*i1_ + x1);
            }
          }
          lp->addLinearEquality(lhs,rhs);
          }
        }
      }
      */

  private:
    template<typename LEFT_FACTOR, typename G2>
    void MaximizeLeft(const LEFT_FACTOR& l, G2& msg, const REAL omega = 1.0)
    {
       msg -= omega*l;
    }
    template<typename RIGHT_FACTOR, typename G2>
    void MinimizeRight(const RIGHT_FACTOR& r, G2& msg, const REAL omega = 1.0)
    {
       matrix<REAL> msgs(i1_,i3_, std::numeric_limits<REAL>::infinity());
       r.min_marginal13(msgs);
       msg -= omega*msgs;
    }

    const INDEX i1_,i2_, i3_;
  };

  template<MessageSendingType TYPE,  bool PROPAGATE_PRIMAL_TO_LEFT = false, bool PROPAGATE_PRIMAL_TO_RIGHT = false> 
  class PairwiseTripletMessage23 {
    public:
      PairwiseTripletMessage23(const INDEX i1, const INDEX i2, const INDEX i3) : i1_(i1), i2_(i2), i3_(i3) {} // the pairwise factor size
      // standard functions which take all possible arguments, to be replaced with minimal ones
      template<typename RIGHT_FACTOR, typename G2, bool ENABLE = TYPE == MessageSendingType::SRMP>
        typename std::enable_if<ENABLE,void>::type
        ReceiveMessageFromRight(const RIGHT_FACTOR& r, G2& msg) 
        {
          MinimizeRight(r,msg); 
        }
      template<typename LEFT_FACTOR, typename G2, bool ENABLE = TYPE == MessageSendingType::MPLP>
        typename std::enable_if<ENABLE,void>::type 
        ReceiveMessageFromLeft(const LEFT_FACTOR& l, G2& msg) 
        { 
          MaximizeLeft(l,msg); 
        }
      template<typename LEFT_FACTOR, typename G3, bool ENABLE = TYPE == MessageSendingType::SRMP>
        typename std::enable_if<ENABLE,void>::type
        SendMessageToRight(const LEFT_FACTOR& l, G3& msg, const REAL omega)
        { 
          MaximizeLeft(l,msg,omega); 
        }
      template<typename RIGHT_FACTOR, typename G3, bool ENABLE = TYPE == MessageSendingType::MPLP>
        typename std::enable_if<ENABLE,void>::type
        SendMessageToLeft(const RIGHT_FACTOR& r, G3& msg, const REAL omega) 
        { 
          MinimizeRight(r,msg,omega); 
        }

      // for primal computation as in TRW-S, we need to compute restricted messages as well
      template<typename RIGHT_FACTOR, typename G1, typename G2, bool ENABLE = TYPE == MessageSendingType::SRMP>
        typename std::enable_if<ENABLE,void>::type
        ReceiveRestrictedMessageFromRight(RIGHT_FACTOR* const r, const G1& rightPot, G2& msg, typename PrimalSolutionStorage::Element rightPrimal) 
        {
           throw std::runtime_error("rounding on pairwise factors is not ucrrently supported");
           assert(false);
        }

    // reparametrize left potential for i-th entry of msg
    // do zrobienia: put strides in here and below
      /*
    template<typename G>
    void RepamLeft(G& repamPot, const REAL msg, const INDEX msg_dim)
    {
      REAL msgn;
      if( std::isfinite(msg) ){ msgn = msg; }
      else{ msgn = std::numeric_limits<REAL>::infinity(); }
      const INDEX x2 = msg_dim/i3_;
      const INDEX x3 = msg_dim%i3_;
      repamPot(x2,x3) += msgn;
    }
    */
    template<typename A1, typename A2>
    void RepamLeft(A1& repamPot, const A2& msgs)
    {
      // do zrobienia: possibly use counter
       for(INDEX x2=0; x2<i2_; ++x2) {
          for(INDEX x3 = 0; x3<i3_; ++x3) {
             repamPot.cost(x2,x3) += normalize( msgs(x2,x3) );
          }
       }
    }
    template<typename A1, typename A2>
    void RepamRight(A1& repamPot, const A2& msgs)
    {
      // do zrobienia: possibly use counter
       for(INDEX x2=0; x2<i2_; ++x2) {
          for(INDEX x3=0; x3<i3_; ++x3) {
             repamPot.msg23(x2,x3) += normalize( msgs(x2,x3) );
          }
       }
    }
    /*
    template<typename G>
    void RepamRight(G& repamPot, const REAL msg, const INDEX dim)
    {
      REAL msgn;
      const INDEX x2 = dim % i2_;
      const INDEX x3 = dim / i2_;
      if( std::isfinite(msg) ){ msgn = msg; }
      else{ msgn = std::numeric_limits<REAL>::infinity(); }
      assert(false);
      //for(INDEX x1 = 0; x1<i1_; ++x1) {
      //  repamPot[x3*i2_*i1_ + x2*i1_ + x1] += msgn;
      //}
    }
    */

    template<bool ENABLE = TYPE == MessageSendingType::SRMP, typename LEFT_FACTOR, typename RIGHT_FACTOR>
    //typename std::enable_if<ENABLE,void>::type
    void
    ComputeRightFromLeftPrimal(const LEFT_FACTOR& l, RIGHT_FACTOR& r)
    {
      assert(r.dim1() == i1_ && r.dim2() == i2_ && r.dim3() == i3_);
      assert(l.size() == i2_*i3_);
      if(l.primal()[0] < i2_) {
         r.primal()[1] = l.primal()[0];
      }
      if(l.primal()[1] < i3_) {
         r.primal()[2] = l.primal()[1];
      }
    }


    /*
    template<class LEFT_FACTOR_TYPE,class RIGHT_FACTOR_TYPE>
      void CreateConstraints(LpInterfaceAdapter* lp,LEFT_FACTOR_TYPE* LeftFactor,RIGHT_FACTOR_TYPE* RightFactor) const
      { 
        for(auto x2=0; x2<i2_ ; x2++){
          for(auto x3=0; x3<i3_; x3++){
            LinExpr lhs = lp->CreateLinExpr() + 0.0;
            LinExpr rhs = lp->CreateLinExpr() + 0.0;
            if(lp->IsLeftObjective(x3*i2_ + x2)){
              lhs += lp->GetLeftVariable(x3*i2_ + x2);
            }
          for(auto x1=0; x1<i1_; x1++){
            if(lp->IsRightObjective(x3*i1_*i2_ + x2*i1_ + x1)){
              rhs += lp->GetRightVariable(x3*i1_+i2_ + x2*i1_ + x1);
            }
          }
          lp->addLinearEquality(lhs,rhs);
          }
        }
      }
      */

  private:
    template<typename LEFT_FACTOR, typename G2>
    void MaximizeLeft(const LEFT_FACTOR& l, G2& msg, const REAL omega = 1.0)
    {
       msg -= omega*l;
    }
    template<typename RIGHT_FACTOR, typename G2>
    void MinimizeRight(const RIGHT_FACTOR& r, G2& msg, const REAL omega = 1.0)
    {
       matrix<REAL> msgs(i2_,i3_, std::numeric_limits<REAL>::infinity());
       r.min_marginal23(msgs);
       msg -= omega*msgs;
    }

    const INDEX i1_,i2_, i3_;
  };

} // end namespace LP_MP

#endif // LP_MP_SIMPLEX_MARGINALIZATION_MESSAGE_HXX
