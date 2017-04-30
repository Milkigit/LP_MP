#ifndef LP_MP_MULTICUT_TRIPLET_FACTOR_HXX
#define LP_MP_MULTICUT_TRIPLET_FACTOR_HXX

#include "LP_MP.h"
#include "lp_interface/lp_interface.h"

namespace LP_MP {

// encoding with 4 entries corresponding to the four possible labelings in this order:
// 011 101 110 111. Labeling 000 is always assigned cost 0. Labelings 100 010 001 are forbidden.
class MulticutTripletFactor : public std::array<REAL,4>
{
public:
   MulticutTripletFactor() 
   {
      std::fill(this->begin(), this->end(), 0.0);
   };

   using TripletEdges = std::array<std::array<INDEX,2>,3>;
   static TripletEdges SortEdges(const INDEX i1, const INDEX i2, const INDEX i3)
   {
      std::array<INDEX,3> ti{i1,i2,i3}; // the node indices of the triplet
      std::sort(ti.begin(), ti.end()); // do zrobienia: use faster sorting
      assert(ti[0] < ti[1] < ti[2]);
      TripletEdges te{{{ti[0],ti[1]},{ti[0],ti[2]},{ti[1],ti[2]}}}; 
      return te;
   }

   constexpr static INDEX size() { return 4; }
   constexpr static INDEX PrimalSize() { return 5; } // primal states: 011 101 110 111 000. Last one receives cost 0 always, however is needed to keep track of primal solution in PropagatePrimal etc.

   REAL LowerBound() const
   {
      return std::min(0.0, *std::min_element(this->begin(), this->end()));
   }

   // if one entry is unknown, set it to true. If one entry is true, set all other to false
   void PropagatePrimal(PrimalSolutionStorage::Element primal) const
   {
      INDEX noTrue = 0;
      INDEX noUnknown = 0;
      INDEX unknownIndex;
      for(INDEX i=0; i<PrimalSize(); ++i) {
         if(primal[i] == true) { 
            ++noTrue;
         }
         if(primal[i] == unknownState) {
            ++noUnknown;
            unknownIndex = i;
         }
      }
      if(noTrue == 0 && noUnknown == 1) {
         primal[unknownIndex] = true;
      } else if(noTrue == 1 && noUnknown > 0) { // should not happen in code currently. Left it for completeness
         for(INDEX i=0; i<PrimalSize(); ++i) {
            if(primal[i] == unknownState) {
               primal[i] = false;
            }
         }
      }
   }

   REAL EvaluatePrimal() const
   {
      const auto sum = std::count(primal_.begin(), primal_.end(), true);
      if(sum == 1) { return std::numeric_limits<REAL>::infinity(); }
      assert(std::count(primal_.begin(), primal_.end(), true) != 1);

      if(!primal_[0] && primal_[1] && primal_[2]) {
         return (*this)[0];
      } else if(primal_[0] && !primal_[1] && primal_[2]) {
         return (*this)[1];
      } else if(primal_[0] && primal_[1] && !primal_[2]) {
         return (*this)[2];
      } else if(primal_[0] && primal_[1] && primal_[2]) {
         return (*this)[3];
      } else if(!primal_[0] && !primal_[1] && !primal_[2]) {
         return 0.0;
      } else {
         assert(false);
         return std::numeric_limits<REAL>::infinity();
      }
   }

   /*
   void CreateConstraints(LpInterfaceAdapter* lp) const 
   {
      LinExpr lhs = lp->CreateLinExpr(); 
      lhs += lp->GetVariable(0);
      lhs += lp->GetVariable(1);
      lhs += lp->GetVariable(2);
      lhs += lp->GetVariable(3);
      LinExpr rhs = lp->CreateLinExpr();
      rhs += 1;
      lp->addLinearInequality(lhs,rhs);
   } 
   */

   void init_primal() {}
   template<typename ARCHIVE> void serialize_dual(ARCHIVE& ar) { ar( *static_cast<std::array<REAL,4>*>(this) ); }
   template<typename ARCHIVE> void serialize_primal(ARCHIVE& ar) { ar( primal_ ); }

   auto& set_primal() { return primal_; }
   auto get_primal() const { return primal_; }
private:
   std::array<bool,3> primal_;
};


// message between unary factor and triplet factor
//enum class MessageSending { SRMP, MPLP }; // do zrobienia: place this possibly more global, also applies to pairwise factors in MRFs
template<MessageSendingType MST = MessageSendingType::SRMP>
class MulticutUnaryTripletMessage
{
public:
   // i is the index in the triplet factor
   MulticutUnaryTripletMessage(const INDEX i) : i_(i) 
   { 
      assert(i < 3); 
   }; 
   ~MulticutUnaryTripletMessage()
   {
      static_assert(MST == MessageSendingType::SRMP,"");
   }

   constexpr static INDEX size() { return 1; }

   template<typename RIGHT_FACTOR, typename G2, MessageSendingType MST_TMP = MST>
   //typename std::enable_if<MST_TMP == MessageSending::SRMP,void>::type
   void
   ReceiveMessageFromRight(const RIGHT_FACTOR& r, G2& msg) const
   {
      msg[0] -= std::min({r[(i_+1)%3], r[(i_+2)%3], r[3]}) - std::min(r[i_],0.0);
   }

   template<typename RIGHT_FACTOR, typename G2, MessageSendingType MST_TMP = MST>
   //typename std::enable_if<MST_TMP == MessageSending::SRMP,void>::type
   void
   ReceiveRestrictedMessageFromRight(const RIGHT_FACTOR& r, G2& msg, PrimalSolutionStorage::Element primal) const
   {
      //return;
      if(primal[(i_+1)%3] == true || primal[(i_+2)%3] == true || primal[3] == true) { // force unary to one
         msg[0] += std::numeric_limits<REAL>::infinity();
         //std::cout << msg.GetLeftFactor()->operator[](0) << "\n";
         //assert(msg.GetLeftFactor()->operator[](0) == -std::numeric_limits<REAL>::infinity() );
      } else if(primal[i_] == true || primal[4] == true) { // force unary to zero 
         //assert(msg.GetLeftFactor()->operator[](0) != std::numeric_limits<REAL>::infinity() );
         msg[0] -= std::numeric_limits<REAL>::infinity(); 
         //std::cout << msg.GetLeftFactor()->operator[](0) << "\n";
         //assert(msg.GetLeftFactor()->operator[](0) == std::numeric_limits<REAL>::infinity() );
      } else { // compute message on unknown values only. No entry of primal is true
         assert(4 == r.size());
         assert(RIGHT_FACTOR::PrimalSize() == 5);
         std::array<REAL,RIGHT_FACTOR::PrimalSize()> restrictedPot;
         restrictedPot.fill(std::numeric_limits<REAL>::infinity());
         for(INDEX i=0; i<size(); ++i) {
            assert(primal[i] != true);
            if(primal[i] == unknownState) {
               restrictedPot[i] = r[i];
            }
         }
         if(primal[4] == unknownState) {
            restrictedPot[4] = 0.0;
         }
         msg[0] -= std::min(std::min(restrictedPot[(i_+1)%3], restrictedPot[(i_+2)%3]), restrictedPot[3]) - std::min(restrictedPot[i_],restrictedPot[4]);
      }
   }

   template<typename LEFT_FACTOR, typename G3, MessageSendingType MST_TMP = MST>
   //typename std::enable_if<MST_TMP == MessageSending::SRMP,void>::type
   void
   SendMessageToRight(const LEFT_FACTOR& l, G3& msg, const REAL omega) const
   {
      static_assert(MST_TMP == MST,"");
      msg[0] -= omega*l;
   }

   template<typename G>
   void RepamLeft(G& repamPot, const REAL msg, const INDEX msg_dim) const
   {
      assert(msg_dim == 0);
      repamPot += msg;
   }
   template<typename G>
   void RepamRight(G& repamPot, const REAL msg, const INDEX msg_dim) const
   {
      assert(msg_dim == 0);
      // the labeling with two 1s in them
      repamPot[(i_+1)%3] += msg;
      repamPot[(i_+2)%3] += msg;
      // for the 111 labeling which is always affected
      repamPot[3] += msg; 
   }

   template<typename LEFT_FACTOR, typename RIGHT_FACTOR>
   void ComputeRightFromLeftPrimal(const LEFT_FACTOR& l, RIGHT_FACTOR& r) const
   {
      r.set_primal()[i_] = l.get_primal();
   }

   template<typename LEFT_FACTOR, typename RIGHT_FACTOR>
   bool CheckPrimalConsistency(const LEFT_FACTOR& l, const RIGHT_FACTOR& r) const
   {
      return l.get_primal() == r.get_primal()[i_];
   }

   /*
    void CreateConstraints(LpInterfaceAdapter* lp, MulticutUnaryFactor* u, MulticutTripletFactor* t) const
    {
      LinExpr lhs = lp->CreateLinExpr(); 
      lhs += lp->GetLeftVariable(0);
      LinExpr rhs = lp->CreateLinExpr();
      rhs += lp->GetRightVariable((i_+1)%3);
      rhs += lp->GetRightVariable((i_+2)%3);
      rhs += lp->GetRightVariable(3);
      lp->addLinearEquality(lhs,rhs);
    }
    */

private:
   const SHORT_INDEX i_;
   // do zrobienia: measure whether precomputing (i_+1)%3 and (i_+2)%3 would make algorithm faster.
   // all values can be stored in one byte
};

} // end namespace LP_MP

#endif // LP_MP_MULTICUT_TRIPLET_FACTOR

