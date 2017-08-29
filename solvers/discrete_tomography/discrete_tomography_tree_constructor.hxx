#ifndef LP_MP_DT_TREE_CONSTRUCTOR_HXX
#define LP_MP_DT_TREE_CONSTRUCTOR_HXX

namespace LP_MP {

template<typename FMC,
   INDEX MRF_PROBLEM_CONSTRUCTOR_NO,
   typename COUNTING_FACTOR,
   typename COUNTING_MESSAGE_LEFT,
   typename COUNTING_MESSAGE_RIGHT,
   typename PAIRWISE_COUNTING_CENTER,
   typename PAIRWISE_COUNTING_LEFT,
   typename PAIRWISE_COUNTING_RIGHT,
   typename LEFT_PAIRWISE_COUNTING_LEFT,
   typename RIGHT_PAIRWISE_COUNTING_LEFT,
   typename LEFT_PAIRWISE_COUNTING_RIGHT,
   typename RIGHT_PAIRWISE_COUNTING_RIGHT
      > 
class DiscreteTomographyTreeConstructor {
  public:
    using MrfConstructorType =
      typename meta::at_c<typename FMC::ProblemDecompositionList,MRF_PROBLEM_CONSTRUCTOR_NO>;
    using counting_factor_type = COUNTING_FACTOR;
    using PairwiseFactorType = typename MrfConstructorType::PairwiseFactorContainer;

    template<typename SOLVER>
    DiscreteTomographyTreeConstructor(SOLVER& s) :
       mrf_constructor_(s.template GetProblemConstructor<MRF_PROBLEM_CONSTRUCTOR_NO>()),
       lp_(&s.GetLP()) 
   {}
    
    void SetNumberOfLabels(const INDEX noLabels) { noLabels_ = noLabels; }

    void connect_pairwise_counting_center(PairwiseFactorType* p, COUNTING_FACTOR* c, LP_tree* tree) {
       auto* m = new PAIRWISE_COUNTING_CENTER(false, p, c);
       lp_->AddMessage(m);
       lp_->AddFactorRelation(p,c);

       if(tree != nullptr) {
          tree->AddMessage(m, Chirality::right);
       }
    }
    void connect_pairwise_counting_left(PairwiseFactorType* p, COUNTING_FACTOR* c, LP_tree* tree) {
       auto* m = new PAIRWISE_COUNTING_LEFT(false, p, c);
       lp_->AddMessage(m);
       lp_->AddFactorRelation(p,c);

       if(tree != nullptr) {
          tree->AddMessage(m, Chirality::right);
       }
    }
    void connect_pairwise_counting_right(PairwiseFactorType* p, COUNTING_FACTOR* c, LP_tree* tree) {
       auto* m = new PAIRWISE_COUNTING_RIGHT(false, p, c);
       lp_->AddMessage(m);
       lp_->AddFactorRelation(p,c);

       if(tree != nullptr) {
          tree->AddMessage(m, Chirality::right);
       }
    }
    void connect_pairwise_counting_left(PairwiseFactorType* p1, PairwiseFactorType* p2, COUNTING_FACTOR* c, LP_tree* tree) {
       auto* m1 = new LEFT_PAIRWISE_COUNTING_LEFT(false,p1,c);
       lp_->AddMessage(m1);
       auto* m2 = new RIGHT_PAIRWISE_COUNTING_LEFT(false,p2,c);
       lp_->AddMessage(m2);
       lp_->AddFactorRelation(p2,c);

       if(tree != nullptr) {
          tree->AddMessage(m1, Chirality::right);
          tree->AddMessage(m2, Chirality::right);
       }
    }
    void connect_pairwise_counting_right(PairwiseFactorType* p1, PairwiseFactorType* p2, COUNTING_FACTOR* c, LP_tree* tree) {
       auto* m1 = new LEFT_PAIRWISE_COUNTING_RIGHT(false,p1,c);
       lp_->AddMessage(m1);
       auto* m2 = new RIGHT_PAIRWISE_COUNTING_RIGHT(false,p2,c);
       lp_->AddMessage(m2);
       lp_->AddFactorRelation(p2,c);

       if(tree != nullptr) {
          tree->AddMessage(m1, Chirality::right);
          tree->AddMessage(m2, Chirality::right);
       }
    }

    void connect_counting_factors_left(COUNTING_FACTOR* c_left, COUNTING_FACTOR* c_top, LP_tree* tree) {
       assert(c_left->GetFactor()->up_sum_size() == c_top->GetFactor()->left_sum_size());
       assert(c_left->GetFactor()->left().dim1() == c_top->GetFactor()->up().dim1());
       lp_->AddFactorRelation(c_left,c_top);
       auto* m = new COUNTING_MESSAGE_LEFT(DIRECTION::left, c_left, c_top);
       lp_->AddMessage(m);

       if(tree != nullptr) {
          tree->AddMessage(m, Chirality::right);
       }
    }
    void connect_counting_factors_right(COUNTING_FACTOR* c_right, COUNTING_FACTOR* c_top, LP_tree* tree) {
       assert(c_right->GetFactor()->up_sum_size() == c_top->GetFactor()->right_sum_size());
       const INDEX no_right_labels_1 = c_right->GetFactor()->right().dim2();
       const INDEX no_right_labels_2 = c_top->GetFactor()->up().dim2();
       assert(c_right->GetFactor()->right().dim2() == c_top->GetFactor()->up().dim2());
       //lp_->AddFactorRelation(c_right,c_top);
       lp_->AddFactorRelation(c_top,c_right);
       auto* m = new COUNTING_MESSAGE_LEFT(DIRECTION::right, c_right, c_top);
       lp_->AddMessage(m);

       if(tree != nullptr) {
          tree->AddMessage(m, Chirality::right);
       }
    }


    struct counting_factor_rec {
       COUNTING_FACTOR* f;
       INDEX rightmost_var;
    };

    template<typename ITERATOR>
    COUNTING_FACTOR* join_factors_recursively(std::deque<counting_factor_rec>& queue, ITERATOR projection_var_begin, const INDEX max_sum, LP_tree* tree)
    {
       assert(queue.size() >= 2);
       //auto& mrf = s_.template GetProblemConstructor<MRF_PROBLEM_CONSTRUCTOR_NO>();
       // recursively join factors on lower levels
       while(queue.size() > 1) {
          const INDEX q_size = queue.size();
          counting_factor_rec first_counting = queue.front();
          if(q_size%2 == 1) { 
             queue.pop_front(); 
             assert(queue.size() == q_size-1);
          }
          assert(queue.size()%2 == 0);
          for(INDEX i=q_size%2; i<q_size; i+=2) {
             auto left = queue.front();
             queue.pop_front();
             auto right = queue.front();
             queue.pop_front();
             assert(left.rightmost_var < right.rightmost_var);

             const INDEX left_sum_size = left.f->GetFactor()->up_sum_size();
             const INDEX right_sum_size = right.f->GetFactor()->up_sum_size();
             const INDEX no_left_labels = left.f->GetFactor()->no_left_labels();
             const INDEX no_center_left_labels = left.f->GetFactor()->no_center_left_labels();
             const INDEX no_center_right_labels = right.f->GetFactor()->no_center_right_labels();
             const INDEX no_right_labels = right.f->GetFactor()->no_right_labels();
             auto* f = new COUNTING_FACTOR(no_left_labels,  left_sum_size, no_center_left_labels, no_center_right_labels, right_sum_size, no_right_labels, std::min(left_sum_size + no_center_left_labels + no_center_right_labels + right_sum_size-3, max_sum));
             lp_->AddFactor(f);
             connect_counting_factors_left(left.f, f, tree);
             connect_pairwise_counting_center( mrf_constructor_.GetPairwiseFactor(*(projection_var_begin+left.rightmost_var), *(projection_var_begin+left.rightmost_var+1)), f, tree);
             connect_counting_factors_right(right.f,f, tree);

             queue.push_back({f,right.rightmost_var}); 
          }
          if(q_size%2 == 1) { 
             queue.push_front(first_counting);
          } 
       }

       assert(queue.size() == 1);
       // set projection constraints in top factor
       auto* f = queue.front().f;
       return f;
    }

    COUNTING_FACTOR* add_top_factor(const INDEX left_sum, const INDEX right_sum, const INDEX max_sum, const INDEX var1, const INDEX var2, LP_tree* tree)
    {
       assert(var1<var2);
       auto* f = new COUNTING_FACTOR(1, left_sum, noLabels_, noLabels_, right_sum, 1, std::min(left_sum + right_sum + 2*noLabels_-3, max_sum), tree);
       lp_->AddFactor(f);
       //auto& mrf = s_.template GetProblemConstructor<MRF_PROBLEM_CONSTRUCTOR_NO>();
       connect_pairwise_counting_center(mrf_constructor_.GetPairwiseFactor(var1,var2),f);
       return f;
    }

    // recursively add counting structures. on the left, start with initial_left_sum counting states, on the right with initial_right_sum ones.
    // return top, left and rightmost counting factors.
    template<typename ITERATOR>
    std::array<COUNTING_FACTOR*,3> AddProjection(ITERATOR projection_var_begin, ITERATOR projection_var_end, const INDEX max_sum, const INDEX initial_left_sum, const INDEX initial_right_sum, LP_tree* tree = nullptr)
    {
       const INDEX no_nodes = std::distance(projection_var_begin, projection_var_end); 
       assert(no_nodes % 6 == 2 && no_nodes >= 8);
       //auto& mrf = s_.template GetProblemConstructor<MRF_PROBLEM_CONSTRUCTOR_NO>();
       assert(std::is_sorted(projection_var_begin, projection_var_end));

       for(auto it=projection_var_begin; it!=projection_var_end-1; ++it) {
          const INDEX i1 = std::min(*it, *(it+1));
          const INDEX i2 = std::max(*it, *(it+1));

          if(!mrf_constructor_.HasPairwiseFactor(i1,i2)) {
             mrf_constructor_.AddPairwiseFactor(i1,i2,std::vector<REAL>(pow(noLabels_,2),0.0));
          }
       }

       std::deque<counting_factor_rec> queue;

       // create leftmost counting factor
       auto* f_left = new COUNTING_FACTOR(1, initial_left_sum, noLabels_, noLabels_, noLabels_, noLabels_, std::min(initial_left_sum + 2*noLabels_-2, max_sum));
       lp_->AddFactor(f_left);
       connect_pairwise_counting_center(mrf_constructor_.GetPairwiseFactor(*projection_var_begin, *(projection_var_begin+1)),f_left, tree);
       connect_pairwise_counting_right(mrf_constructor_.GetPairwiseFactor(*(projection_var_begin+1),*(projection_var_begin+2)), mrf_constructor_.GetPairwiseFactor(*(projection_var_begin+2), *(projection_var_begin+3)), f_left, tree);
       queue.push_back({f_left,3});

       // create cardinality factors on base level spanning six unaries
       for(INDEX i=4; i+6<no_nodes; i+=6) {
         auto* f = new COUNTING_FACTOR(noLabels_, noLabels_, noLabels_, std::min(4*noLabels_-3, max_sum));
         lp_->AddFactor(f);
         connect_pairwise_counting_left(mrf_constructor_.GetPairwiseFactor(*(projection_var_begin+i),*(projection_var_begin+i+1)), mrf_constructor_.GetPairwiseFactor(*(projection_var_begin+i+1), *(projection_var_begin+i+2)), f, tree);
         connect_pairwise_counting_center(mrf_constructor_.GetPairwiseFactor(*(projection_var_begin+i+2),*(projection_var_begin+i+3)), f, tree);
         connect_pairwise_counting_right(mrf_constructor_.GetPairwiseFactor(*(projection_var_begin+i+3),*(projection_var_begin+i+4)), mrf_constructor_.GetPairwiseFactor(*(projection_var_begin+i+4), *(projection_var_begin+i+5)), f, tree);
         lp_->AddFactorRelation(f, mrf_constructor_.GetUnaryFactor(*(projection_var_begin+i+5)));
         queue.push_back({f,i+5}); 
       } 

       // create rightmost counting factor
       auto* f_right = new COUNTING_FACTOR(noLabels_, noLabels_, noLabels_, noLabels_, initial_right_sum, 1, std::min(initial_right_sum + 2*noLabels_-2, max_sum));
       lp_->AddFactor(f_right);
       auto projection_var_right = projection_var_end - 4;
       connect_pairwise_counting_left(mrf_constructor_.GetPairwiseFactor(*(projection_var_right),*(projection_var_right+1)), mrf_constructor_.GetPairwiseFactor(*(projection_var_right+1), *(projection_var_right+2)), f_right, tree);
       connect_pairwise_counting_center(mrf_constructor_.GetPairwiseFactor(*(projection_var_right+2), *(projection_var_right+3)), f_right, tree);
       queue.push_back({f_right,no_nodes-1});
       assert(projection_var_right - projection_var_begin == no_nodes - 4);

       auto* f_top = join_factors_recursively(queue, projection_var_begin, max_sum, tree);
       return {f_left,f_right,f_top};
    }

    void AddProjection(const std::vector<INDEX>& projectionVar, const std::vector<REAL>& summationCost, LP_tree* tree = nullptr)
    {
       const INDEX max_sum = std::max(noLabels_,INDEX(summationCost.size()));
       assert(projectionVar.size() > 3); // otherwise just use a ternary factor to enforce consistency
       assert(std::is_sorted(projectionVar.begin(), projectionVar.end())); // support unsorted projectionVar (transpose in messages) later

      for(INDEX i=0;i<projectionVar.size()-1;++i) {
        const INDEX i1 = std::min(projectionVar[i],projectionVar[i+1]);
        const INDEX i2 = std::max(projectionVar[i],projectionVar[i+1]);

        if(!mrf_constructor_.HasPairwiseFactor(i1,i2)) {
           matrix<REAL> pairwise_cost(noLabels_, noLabels_, 0.0);
           mrf_constructor_.AddPairwiseFactor(i1,i2,pairwise_cost);
        }
      }

      std::deque<counting_factor_rec> queue;

      // partition projectionVar into subsets of size at least 4 and at most 6
      INDEX begin_rest;
      switch(projectionVar.size() % 6) {
         case 0: begin_rest = 0; break;// nothing to do
         case 1: {// use two counting factors. Right counting subfactor of left counting factor is connected to top subcounting factor of right counting factor 
                 auto* f1 = new COUNTING_FACTOR(noLabels_, 1, 1, std::min(2*noLabels_-1,max_sum));
                 lp_->AddFactor(f1);
                 connect_pairwise_counting_left(mrf_constructor_.GetPairwiseFactor(projectionVar[0],projectionVar[1]),f1, tree);
                 connect_pairwise_counting_center(mrf_constructor_.GetPairwiseFactor(projectionVar[1],projectionVar[2]),f1, tree);
                 connect_pairwise_counting_right(mrf_constructor_.GetPairwiseFactor(projectionVar[2],projectionVar[3]),f1, tree);
                 lp_->AddFactorRelation(f1, mrf_constructor_.GetUnaryFactor(projectionVar[3]));

                 auto* f2 = new COUNTING_FACTOR(noLabels_, f1->GetFactor()->up_sum_size(), 1, std::min(f1->GetFactor()->up_sum_size() + 2*noLabels_-2, max_sum));
                 lp_->AddFactor(f2);
                 connect_counting_factors_left(f1,f2, tree);
                 connect_pairwise_counting_center(mrf_constructor_.GetPairwiseFactor(projectionVar[3],projectionVar[4]),f2, tree);
                 connect_pairwise_counting_right(mrf_constructor_.GetPairwiseFactor(projectionVar[4],projectionVar[5]),f2, tree);
                 lp_->AddFactorRelation(f2, mrf_constructor_.GetUnaryFactor(projectionVar[5]));

                 begin_rest = 7;
                 queue.push_back({f2,6});
                 
                 break;
                 }
         case 2: {// for the remaining cases we use two separate counting factors.
                 auto* f1 = new COUNTING_FACTOR(noLabels_, 1, 1, std::min(2*noLabels_-1, max_sum));
                 lp_->AddFactor(f1);
                 connect_pairwise_counting_left(mrf_constructor_.GetPairwiseFactor(projectionVar[0],projectionVar[1]), f1, tree);
                 connect_pairwise_counting_center(mrf_constructor_.GetPairwiseFactor(projectionVar[1],projectionVar[2]),f1, tree);
                 connect_pairwise_counting_right(mrf_constructor_.GetPairwiseFactor(projectionVar[2],projectionVar[3]),f1, tree);
                 lp_->AddFactorRelation(f1, mrf_constructor_.GetUnaryFactor(projectionVar[3]));
                 queue.push_back({f1,3});

                 auto* f2 = new COUNTING_FACTOR(noLabels_, 1, 1, std::min(2*noLabels_-1, max_sum));
                 lp_->AddFactor(f2);
                 connect_pairwise_counting_left(mrf_constructor_.GetPairwiseFactor(projectionVar[4],projectionVar[5]), f2, tree);
                 connect_pairwise_counting_center(mrf_constructor_.GetPairwiseFactor(projectionVar[5],projectionVar[6]),f2, tree);
                 connect_pairwise_counting_right(mrf_constructor_.GetPairwiseFactor(projectionVar[6],projectionVar[7]),f2, tree);
                 lp_->AddFactorRelation(f2, mrf_constructor_.GetUnaryFactor(projectionVar[7]));
                 queue.push_back({f2,7});

                 begin_rest = 8;
                 break;
                 }
         case 3: {
                 auto* f1 = new COUNTING_FACTOR(noLabels_, noLabels_, 1, std::min(3*noLabels_-2, max_sum));
                 lp_->AddFactor(f1);
                 connect_pairwise_counting_left(mrf_constructor_.GetPairwiseFactor(projectionVar[0],projectionVar[1]), mrf_constructor_.GetPairwiseFactor(projectionVar[1], projectionVar[2]), f1, tree);
                 connect_pairwise_counting_center(mrf_constructor_.GetPairwiseFactor(projectionVar[2],projectionVar[3]),f1, tree);
                 connect_pairwise_counting_right(mrf_constructor_.GetPairwiseFactor(projectionVar[3],projectionVar[4]),f1, tree);
                 lp_->AddFactorRelation(f1, mrf_constructor_.GetUnaryFactor(projectionVar[4]));
                 queue.push_back({f1,4});

                 auto* f2 = new COUNTING_FACTOR(noLabels_, 1, 1, std::min(2*noLabels_-1, max_sum));
                 lp_->AddFactor(f2);
                 connect_pairwise_counting_left(mrf_constructor_.GetPairwiseFactor(projectionVar[5],projectionVar[6]), f2, tree);
                 connect_pairwise_counting_center(mrf_constructor_.GetPairwiseFactor(projectionVar[6],projectionVar[7]),f2, tree);
                 connect_pairwise_counting_right(mrf_constructor_.GetPairwiseFactor(projectionVar[7],projectionVar[8]),f2, tree);
                 lp_->AddFactorRelation(f2, mrf_constructor_.GetUnaryFactor(projectionVar[8]));
                 queue.push_back({f2,8});

                 begin_rest = 9;
                 break;
                 }
         case 4: {
                 auto* f = new COUNTING_FACTOR(noLabels_, 1, 1, std::min(2*noLabels_-1, max_sum));
                 lp_->AddFactor(f);
                 connect_pairwise_counting_left(mrf_constructor_.GetPairwiseFactor(projectionVar[0],projectionVar[1]), f, tree);
                 connect_pairwise_counting_center(mrf_constructor_.GetPairwiseFactor(projectionVar[1],projectionVar[2]),f, tree);
                 connect_pairwise_counting_right(mrf_constructor_.GetPairwiseFactor(projectionVar[2],projectionVar[3]), f, tree);
                 lp_->AddFactorRelation(f, mrf_constructor_.GetUnaryFactor(projectionVar[3]));
                 queue.push_back({f,3});

                 begin_rest = 4;
                 break;
                 }
         case 5: {
                 auto* f = new COUNTING_FACTOR(noLabels_, noLabels_, 1, std::min(3*noLabels_-2, max_sum));
                 lp_->AddFactor(f);
                 connect_pairwise_counting_left(mrf_constructor_.GetPairwiseFactor(projectionVar[0],projectionVar[1]), mrf_constructor_.GetPairwiseFactor(projectionVar[1], projectionVar[2]), f, tree);
                 connect_pairwise_counting_center(mrf_constructor_.GetPairwiseFactor(projectionVar[2],projectionVar[3]),f, tree);
                 connect_pairwise_counting_right(mrf_constructor_.GetPairwiseFactor(projectionVar[3],projectionVar[4]), f, tree);
                 lp_->AddFactorRelation(f, mrf_constructor_.GetUnaryFactor(projectionVar[4]));
                 queue.push_back({f,4});

                 begin_rest = 5;
                 break; 
                 }
      }
      assert((projectionVar.size() - begin_rest) % 6 == 0);

      for(INDEX i=begin_rest;i<projectionVar.size(); i+=6){ // a counting factor on base level can take care of six unaries
         auto* f = new COUNTING_FACTOR(noLabels_, noLabels_, noLabels_, std::min(4*noLabels_-3, max_sum));
         lp_->AddFactor(f);
         connect_pairwise_counting_left(mrf_constructor_.GetPairwiseFactor(projectionVar[i],projectionVar[i+1]), mrf_constructor_.GetPairwiseFactor(projectionVar[i+1], projectionVar[i+2]), f, tree);
         connect_pairwise_counting_center(mrf_constructor_.GetPairwiseFactor(projectionVar[i+2],projectionVar[i+3]),f, tree);
         connect_pairwise_counting_right(mrf_constructor_.GetPairwiseFactor(projectionVar[i+3],projectionVar[i+4]), mrf_constructor_.GetPairwiseFactor(projectionVar[i+4], projectionVar[i+5]), f, tree);
         lp_->AddFactorRelation(f, mrf_constructor_.GetUnaryFactor(projectionVar[i+5]));
         queue.push_back({f,i+5});
      }

      // recursively join factors on lower levels
      auto* f = join_factors_recursively(queue, projectionVar.begin(), max_sum, tree);
      // set projection constraints in top factor
      f->GetFactor()->summation_cost(summationCost); 
    }

  private:
    //std::vector<Tree> treeIndices_;
    INDEX noLabels_ = 0;
    LP* lp_;
    MrfConstructorType& mrf_constructor_;
};

// construct one-dimensional discrete tomography problem with sequential factors
template<typename FMC,
     INDEX MRF_PROBLEM_CONSTRUCTOR_NO,
     //typename SUM_FACTOR,
     typename SUM_PAIRWISE_FACTOR,
     typename SUM_PAIRWISE_MESSAGE,
     //typename SUM_PAIRWISE_MESSAGE_LEFT,
     //typename SUM_PAIRWISE_MESSAGE_RIGHT,
     typename SUM_PAIRWISE_PAIRWISE_MESSAGE,
     typename SUM_UNARY_MESSAGE_LEFT,
     typename SUM_UNARY_MESSAGE_RIGHT
        >
class dt_sequential_constructor {
public:
   //using sum_factor_type = SUM_FACTOR;
   using PairwiseSumFactor = SUM_PAIRWISE_FACTOR;
   using PairwiseSumMessageType = SUM_PAIRWISE_MESSAGE;
   using MrfConstructorType =
      typename meta::at_c<typename FMC::ProblemDecompositionList,MRF_PROBLEM_CONSTRUCTOR_NO>;
   using PairwiseFactorType = typename MrfConstructorType::PairwiseFactorContainer;

   template<typename SOLVER>
   dt_sequential_constructor(SOLVER& s) :
      lp_(&s.GetLP()),
      mrf_constructor_(s.template GetProblemConstructor<MRF_PROBLEM_CONSTRUCTOR_NO>())
   {
      assert(&mrf_constructor_ != nullptr);
   }

   void SetNumberOfLabels(const INDEX noLabels) { noLabels_ = noLabels; }

   //void connect_unary_and_sum_factor(typename MrfConstructorType::UnaryFactorContainer* u, SUM_FACTOR* s)
   //{
   //   auto* m = new UNARY_SUM_MESSAGE(u,s);
   //   lp_->AddMessage(m);
   //}

   SUM_PAIRWISE_FACTOR* AddProjection(const std::vector<INDEX>& projectionVar, const std::vector<REAL>& summationCost, LP_tree* tree = nullptr)
   {
      assert(summationCost.size() > 0);
      const INDEX max_sum = std::max(noLabels_,INDEX(summationCost.size()));

      auto* f = AddProjection(projectionVar.begin(), projectionVar.end(), max_sum, tree);
      f->GetFactor()->summation_cost(summationCost.begin(), summationCost.end());

      return f;
   }

   template<typename ITERATOR>
   SUM_PAIRWISE_FACTOR* AddProjection(ITERATOR projection_var_begin, ITERATOR projection_var_end, const INDEX max_sum, LP_tree* tree)
   { 
      assert(noLabels_ > 0);
      assert(std::distance(projection_var_begin, projection_var_end) > 1);

      for(auto it=projection_var_begin; it!=projection_var_end-1; ++it) {
         const INDEX i1 = std::min(*it, *(it+1));
         const INDEX i2 = std::max(*it, *(it+1));

         if(!mrf_constructor_.HasPairwiseFactor(i1,i2)) {
            matrix<REAL> pairwise_cost(noLabels_,noLabels_,0.0); // to do: this is bad, as it will leave a hole in the allocator
            mrf_constructor_.AddPairwiseFactor(i1,i2,pairwise_cost);
         }
      }

      SUM_PAIRWISE_FACTOR* f_prev = nullptr;
      typename MrfConstructorType::UnaryFactorContainer* last_added_1 = nullptr;
      typename MrfConstructorType::UnaryFactorContainer* last_added_2 = nullptr;
      INDEX prev_sum_size = 1;//std::min(noLabels_, max_sum);
      //connect_unary_and_sum_factor(mrf_constructor_.GetUnaryFactor(*projection_var_begin), f_prev);
      INDEX i=1;
      for(auto it=projection_var_begin+1; it!=projection_var_end; ++it, ++i) {
         const INDEX prev_var = *(it-1);
         const INDEX next_var = *(it);
         const INDEX sum_size = std::min(i*(noLabels_-1)+1, max_sum); 

         auto* f = new SUM_PAIRWISE_FACTOR(noLabels_, prev_sum_size, sum_size, prev_var, next_var);
         lp_->AddFactor(f);

         auto* f_l = mrf_constructor_.GetUnaryFactor(*(it-1));
         auto* m_l = new SUM_UNARY_MESSAGE_LEFT(f_l, f);
         lp_->AddMessage(m_l);

         auto* f_r = mrf_constructor_.GetUnaryFactor(*(it));
         auto* m_r = new SUM_UNARY_MESSAGE_RIGHT(f_r, f);
         lp_->AddMessage(m_r);

         const bool transpose = *(it-1) > *it;

         // interleave dt factors reparametrization with unary/pairwise factor reparametrization
         if(!transpose) {
            lp_->AddFactorRelation(f_l, f);
            lp_->AddFactorRelation(f, f_r);
         } else {
            lp_->AddFactorRelation(f, f_l);
            lp_->AddFactorRelation(f_r, f);
         }

         if(f_prev != nullptr) {
            auto* m = new SUM_PAIRWISE_MESSAGE(f_prev,f);
            lp_->AddMessage(m);
            if(tree != nullptr) {
               tree->AddMessage(m, Chirality::right);
            }

            // reparametrize dt factors only after unary/pairwise ones
            //lp_->AddFactorRelation(f_prev, f);
         } else {
            // reparametrize dt factors only after unary/pairwise ones
            for(auto it_tmp=projection_var_begin; it_tmp!=projection_var_end; ++it_tmp) {
               auto* u = mrf_constructor_.GetUnaryFactor(*it_tmp);
               //lp_->AddFactorRelation(u, f);
            }
         }

         auto* p = mrf_constructor_.GetPairwiseFactor(std::min(*(it-1), *it), std::max(*(it-1), *it));
         auto* m_c = new SUM_PAIRWISE_PAIRWISE_MESSAGE(transpose, p, f);
         lp_->AddMessage(m_c);

         if(tree != nullptr) {
            // also add messages to unaries considering that unaries can be covered twice (for inner unaries). This allows to have significantly smaller trees for the MRF part, since many pairwise factors and associated unaries are covered by the projection trees.

            auto* left_msg = mrf_constructor_.get_left_message(std::min(*(it-1), *it), std::max(*(it-1), *it));
            auto* right_msg = mrf_constructor_.get_right_message(std::min(*(it-1), *it), std::max(*(it-1), *it));

            auto* left_factor = mrf_constructor_.GetUnaryFactor(std::min(*(it-1), *it));
            auto* right_factor = mrf_constructor_.GetUnaryFactor(std::max(*(it-1), *it));

            if(last_added_1 != left_factor && last_added_2 != left_factor) {
               tree->AddMessage(left_msg, Chirality::right); 
            }
            if(last_added_1 != right_factor && last_added_2 != right_factor) {
               tree->AddMessage(right_msg, Chirality::right); 
            }

            last_added_1 = left_factor;
            last_added_2 = right_factor;

            tree->AddMessage(m_c, Chirality::right);
         }

         f_prev = f;
         prev_sum_size = sum_size;
      }

      if(tree != nullptr) {
         tree->init();
      }
      return f_prev;
   }

private:
   INDEX noLabels_ = 0;
   LP* lp_;
   MrfConstructorType& mrf_constructor_;
};

// note: when mixing sequential and recursive mode, messages of type PAIRWISE_COUNTING_LEFT and PAIRWISE_COUNTING_RIGHT used in recursive constructor are not needed
template<typename FMC,
     INDEX MRF_PROBLEM_CONSTRUCTOR_NO, // not needed, sequential and recursive constructors have it also.
     typename SEQUENTIAL_CONSTRUCTOR,
     typename RECURSIVE_CONSTRUCTOR,
     // combine
     typename SEQUENTIAL_RECURSIVE_MESSAGE_LEFT,
     typename SEQUENTIAL_RECURSIVE_MESSAGE_RIGHT> 
class dt_combined_constructor : public SEQUENTIAL_CONSTRUCTOR, public RECURSIVE_CONSTRUCTOR {
public:
   constexpr static INDEX recursive_threshold = 20; // when sum can be more than 200, switch to recursive factors, otherwise stick with sequential
   using MrfConstructorType =
      typename meta::at_c<typename FMC::ProblemDecompositionList,MRF_PROBLEM_CONSTRUCTOR_NO>;
   using PairwiseFactorType = typename MrfConstructorType::PairwiseFactorContainer;

   template<typename SOLVER>
   dt_combined_constructor(SOLVER& s) : 
      SEQUENTIAL_CONSTRUCTOR(s), 
      RECURSIVE_CONSTRUCTOR(s)
      {}

   void SetNumberOfLabels(const INDEX noLabels) 
   { 
      noLabels_ = noLabels;
      SEQUENTIAL_CONSTRUCTOR::SetNumberOfLabels(noLabels); 
      RECURSIVE_CONSTRUCTOR::SetNumberOfLabels(noLabels); 
   } 

   void connect_chain_recursive_left(typename SEQUENTIAL_CONSTRUCTOR::sum_factor_type* f_seq, typename RECURSIVE_CONSTRUCTOR::counting_factor_type* f_rec, LP_tree* tree)
   {
      assert(f_rec->GetFactor()->no_left_labels() == 1);
      auto* m = new SEQUENTIAL_RECURSIVE_MESSAGE_LEFT(f_seq, f_rec, tree);

      if(tree != nullptr) {
         tree->AddMessage(m, Chirality::right);
      }
   }

   void connect_chain_recursive_right(typename SEQUENTIAL_CONSTRUCTOR::sum_factor_type* f_seq, typename RECURSIVE_CONSTRUCTOR::counting_factor_type* f_rec, LP_tree* tree)
   {
      assert(f_rec->GetFactor()->no_right_labels() == 1);
      auto* m = new SEQUENTIAL_RECURSIVE_MESSAGE_RIGHT(f_seq, f_rec, tree);

      if(tree != nullptr) {
         tree->AddMessage(m, Chirality::right);
      }
   }

   void AddProjection(const std::vector<INDEX>& projectionVar, const std::vector<REAL>& summationCost, LP_tree* tree = nullptr)
   { 
      assert(summationCost.size() > 0);
      const INDEX max_sum = std::max(noLabels_,INDEX(summationCost.size()));
      //auto& mrf = s_.template GetProblemConstructor<MRF_PROBLEM_CONSTRUCTOR_NO>();
      assert(std::is_sorted(projectionVar.begin(), projectionVar.end())); // support unsorted projectionVar (transpose in messages) later

      assert(projectionVar.size() > 2); // otherwise pairwise factor can take care

      if(projectionVar.size()*noLabels_ < recursive_threshold || max_sum < recursive_threshold) { 
         // if maximum sum is small enough, use chain construction solely.
         // better: use two-sided approach, except for very easy cases (max_sum or projectionVar.size() very small)
         SEQUENTIAL_CONSTRUCTOR::AddProjection(projectionVar, summationCost, tree);
      } else { 
         // otherwise construct chain from left and right and join in the middle with a recursive construction
         // the remaining nodes must obey the constraints 8 <= no_remaining_nodes % 6 = 2
         assert(projectionVar.size() >= 8);
         INDEX no_sequential_factors_left = projectionVar.size()/noLabels_;
         INDEX no_sequential_factors_right = projectionVar.size()/noLabels_;
         assert(no_sequential_factors_left + no_sequential_factors_right <= projectionVar.size());
         // adjust no of sequential factors, such that remaining factors satisfy above relations
         const INDEX no_remaining_nodes = projectionVar.size() - no_sequential_factors_left - no_sequential_factors_right;
         if(no_remaining_nodes < 8) {
            assert(false); // not tested yet!
            no_sequential_factors_left += no_remaining_nodes/2;
            no_sequential_factors_right += no_remaining_nodes - no_remaining_nodes/2;
            assert(no_sequential_factors_left + no_sequential_factors_right == projectionVar.size());
            // build left and right subchain and join them directly via single cardinality factor
            auto* last_sequential_left = SEQUENTIAL_CONSTRUCTOR::AddProjection(projectionVar.begin(), projectionVar.begin()+no_sequential_factors_left+1, max_sum, tree);
            auto* last_sequential_right = SEQUENTIAL_CONSTRUCTOR::AddProjection(projectionVar.rbegin(), projectionVar.rbegin()+no_sequential_factors_right+1, max_sum, tree);
            auto* f_top = RECURSIVE_CONSTRUCTOR::add_top_factor(last_sequential_left->GetFactor()->sum_size(), last_sequential_right->GetFactor()->sum_size(), max_sum, projectionVar[no_sequential_factors_left-1], projectionVar[no_sequential_factors_left], tree);
            assert(f_top->LowerBound() < 10000000);
            connect_chain_recursive_left(last_sequential_left, f_top);
            connect_chain_recursive_right(last_sequential_right, f_top);
            f_top->GetFactor()->summation_cost(summationCost);
         } else {
            INDEX no_nodes_to_distribute = 0;
            switch(no_remaining_nodes%6) {
               case 0: no_nodes_to_distribute = 4; break;
               case 1: no_nodes_to_distribute = 5; break;
               case 2: no_nodes_to_distribute = 0; break;
               case 3: no_nodes_to_distribute = 1; break;
               case 4: no_nodes_to_distribute = 2; break;
               case 5: no_nodes_to_distribute = 3; break;
            }
            no_sequential_factors_left += no_nodes_to_distribute/2;
            no_sequential_factors_right += no_nodes_to_distribute - no_nodes_to_distribute/2;
            auto* last_sequential_left = SEQUENTIAL_CONSTRUCTOR::AddProjection(projectionVar.begin(), projectionVar.begin()+no_sequential_factors_left+1, max_sum, tree);
            auto* last_sequential_right = SEQUENTIAL_CONSTRUCTOR::AddProjection(projectionVar.rbegin(), projectionVar.rbegin()+no_sequential_factors_right+1, max_sum, tree);
            auto recursive_factors = RECURSIVE_CONSTRUCTOR::AddProjection(projectionVar.begin() + no_sequential_factors_left, projectionVar.end() - no_sequential_factors_right, max_sum, last_sequential_left->GetFactor()->sum_size(), last_sequential_right->GetFactor()->sum_size(), tree); 
            // connect leftmost and rightmost recursive factors with sequential factors
            connect_chain_recursive_left(last_sequential_left, recursive_factors[0], tree);
            connect_chain_recursive_right(last_sequential_right, recursive_factors[1], tree);
            recursive_factors[2]->GetFactor()->summation_cost(summationCost);
         }
      }
   }

private:
   INDEX noLabels_ = 0;
   LP* lp_;
};

}

#endif // LP_MP_xxx
