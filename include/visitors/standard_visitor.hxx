#ifndef LP_MP_STANDARD_VISITOR_HXX
#define LP_MP_STANDARD_VISITOR_HXX

#include "LP_MP.h"
#include "tolerance.hxx"
#include "mem_use.c"
#include "tclap/CmdLine.h"
#include <chrono>

namespace LP_MP {
   class PositiveRealConstraint : public TCLAP::Constraint<REAL>
   {
      public:
         std::string description() const { return "positive real constraint"; };
         std::string shortID() const { return "positive real number"; };
         bool check(const REAL& value) const { return value >= 0.0; };
   };

   // standard visitor class for LP_MP solver, when no custom visitor is given
   // do zrobienia: add xor arguments primalBoundComputationInterval, dualBoundComputationInterval with boundComputationInterval
   template<class PROBLEM_DECOMPOSITION>
   class StandardVisitor {
      
      public:
      StandardVisitor(TCLAP::CmdLine& cmd, PROBLEM_DECOMPOSITION& pd)
         :
            maxIterArg_("","maxIter","maximum number of iterations of LP_MP, default = 1000",false,1000,"positive integer",cmd),
            maxMemoryArg_("","maxMemory","maximum amount of memory (MB) LP_MP is allowed to use",false,std::numeric_limits<INDEX>::max(),"positive integer",cmd),
            timeoutArg_("","timeout","time after which algorithm is stopped, in seconds, default = never, should this be type double?",false,std::numeric_limits<INDEX>::max(),"positive integer",cmd),
            // xor those //
            //boundComputationIntervalArg_("","boundComputationInterval","lower bound computation performed every x-th iteration, default = 5",false,5,"positive integer",cmd),
            primalComputationIntervalArg_("","primalComputationInterval","primal computation performed every x-th iteration, default = 5",false,5,"positive integer",cmd),
            lowerBoundComputationIntervalArg_("","lowerBoundComputationInterval","lower bound computation performed every x-th iteration, default = 1",false,1,"positive integer",cmd),
            ///////////////
            posConstraint_(),
            minDualImprovementArg_("","minDualImprovement","minimum dual improvement between iterations of LP_MP",false,0.0,&posConstraint_,cmd),
            standardReparametrizationArg_("","standardReparametrization","mode of reparametrization: {anisotropic,uniform}",false,"anisotropic","{anisotropic|uniform}",cmd),
            roundingReparametrizationArg_("","roundingReparametrization","mode of reparametrization for rounding primal solution: {anisotropic|uniform}",false,"uniform","{anisotropic|uniform}",cmd),
            protocolateFileArg_("","protocolate","file into which to protocolate progress of algorithm, obsolete, not implemented yet, as with silent option",false,"","file name",cmd),
            silentArg_("","silent","suppress output on stdout, not implemented yet. Implement via logging mechanism, search for suitable package.",cmd,false),
            pd_(pd)
      {}

      LPVisitorReturnType begin(const LP* lp) // called, after problem is constructed. 
      {
         try {
            maxIter_ = maxIterArg_.getValue();
            maxMemory_ = maxMemoryArg_.getValue();
            remainingIter_ = maxIter_;
            minDualImprovement_ = minDualImprovementArg_.getValue();
            timeout_ = timeoutArg_.getValue();
            //boundComputationInterval_ = boundComputationIntervalArg_.getValue();
            primalComputationInterval_ = primalComputationIntervalArg_.getValue();
            lowerBoundComputationInterval_ = lowerBoundComputationIntervalArg_.getValue();

            standardReparametrization_ = standardReparametrizationArg_.getValue();
            roundingReparametrization_ = roundingReparametrizationArg_.getValue();
            protocolateFile_ = protocolateFileArg_.getValue();
         } catch (TCLAP::ArgException &e) {
            std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl; 
            exit(1);
         }
         curDualBound_ = lp->LowerBound();
         std::cout << "Initial lower bound before optimizing = " << curDualBound_ << std::endl;
         beginTime_ = std::chrono::steady_clock::now();

         if( ! (standardReparametrization_ == "anisotropic" || standardReparametrization_ == "uniform") ) {
            throw std::runtime_error("standard repararametrization mode must be {anisotropic|uniform}, is " + standardReparametrization_);
         }
         if( ! (roundingReparametrization_ == "anisotropic" || roundingReparametrization_ == "uniform") ) {
            throw std::runtime_error("rounding repararametrization mode must be {anisotropic|uniform}, is " + roundingReparametrization_);
         }

         if(roundingReparametrization_ == "anisotropic") {
            return LPVisitorReturnType::ReparametrizeLowerBoundPrimalAnisotropic;
         } else if(roundingReparametrization_ == "uniform") {
            return LPVisitorReturnType::ReparametrizeLowerBoundPrimalUniform;
         }
         // do zrobienia: make output stream such that it protocolates into file and on std::cout
      }


      // the default
      // do zrobienia: it makes little sense to have visit templatised. Just pass parameter as an argument
      template<LPVisitorReturnType LP_STATE>
      LPVisitorReturnType visit(LP* lp)
      {

         const INDEX timeElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - beginTime_).count();

         if(LP_STATE == LPVisitorReturnType::ReparametrizeUniform || LP_STATE == LPVisitorReturnType::ReparametrizeAnisotropic) {
            // output nothing
         } else if(LP_STATE == LPVisitorReturnType::ReparametrizeLowerBoundUniform || LP_STATE == LPVisitorReturnType::ReparametrizeLowerBoundAnisotropic) {
            std::cout << "iteration = " << curIter_ << ", lower bound = " << lp->BestLowerBound() << ", time elapsed = " << timeElapsed << " milliseconds\n";
         } else if(LP_STATE == LPVisitorReturnType::ReparametrizePrimalUniform || LP_STATE == LPVisitorReturnType::ReparametrizePrimalAnisotropic) {
            std::cout << "iteration = " << curIter_ << ", upper bound = " << lp->BestPrimalBound() << ", time elapsed = " << timeElapsed << " milliseconds\n";
         } else if(LP_STATE == LPVisitorReturnType::ReparametrizeLowerBoundPrimalUniform || LP_STATE == LPVisitorReturnType::ReparametrizeLowerBoundPrimalAnisotropic) {
            std::cout << "iteration = " << curIter_ << ", lower bound = " << lp->BestLowerBound() << ", upper bound = " << lp->BestPrimalBound() << ", time elapsed = " << timeElapsed << " milliseconds\n";
         } else if(LP_STATE == LPVisitorReturnType::Break) {
            auto endTime = std::chrono::steady_clock::now();
            std::cout << "Optimization took " << std::chrono::duration_cast<std::chrono::milliseconds>(endTime - beginTime_).count() << " milliseconds and " << curIter_ << " iterations\n";
            return LPVisitorReturnType::Break;
         } else if(LP_STATE == LPVisitorReturnType::Error) {
         } else {
            assert(false);
         }

         curIter_++;
         remainingIter_--;

         if(LP_STATE == LPVisitorReturnType::ReparametrizeLowerBoundAnisotropic
               || LP_STATE == LPVisitorReturnType::ReparametrizeLowerBoundPrimalAnisotropic
               || LP_STATE == LPVisitorReturnType::ReparametrizeLowerBoundUniform
               || LP_STATE == LPVisitorReturnType::ReparametrizeLowerBoundPrimalUniform) {
            prevDualBound_ = curDualBound_;
            curDualBound_ = lp->BestLowerBound();
         }
         // check if optimization has to be terminated
         if(remainingIter_ == 0) {
            return LPVisitorReturnType::Break;
         } 
         if(lp->BestPrimalBound() <= lp->BestLowerBound() + eps) {
            std::cout << "Primal cost equals lower bound\n";
            return LPVisitorReturnType::Break;
         }
         if(timeout_ != std::numeric_limits<REAL>::max() && timeElapsed >= timeout_) {
            std::cout << "Timeout reached after " << timeElapsed << " seconds\n";
            remainingIter_ = std::min(INDEX(1),remainingIter_);
         }
         if(maxMemory_ > 0) {
            const INDEX memoryUsed = memory_used()/(1024*1024);
            if(maxMemory_ < memoryUsed) {
               remainingIter_ = std::min(INDEX(1),remainingIter_);
               std::cout << "Solver uses " << memoryUsed << " MB memory, aborting optimization\n";
            }
         }
         if(minDualImprovement_ > 0 && curDualBound_ - prevDualBound_ < minDualImprovement_) {
            std::cout << "Dual improvement smaller than " << minDualImprovement_ << "\n";
            remainingIter_ = std::min(INDEX(1),remainingIter_);
         }

         if(remainingIter_ == 1) {
            if(roundingReparametrization_ == "anisotropic") {
               return LPVisitorReturnType::ReparametrizeLowerBoundPrimalAnisotropic;
            } else if(roundingReparametrization_ == "uniform") {
               return LPVisitorReturnType::ReparametrizeLowerBoundPrimalUniform;
            }
         }


         // determine next state of solver
         if(curIter_ % primalComputationInterval_ == 0 && curIter_ % lowerBoundComputationInterval_ == 0) {
            if(roundingReparametrization_ == "anisotropic") {
               return LPVisitorReturnType::ReparametrizeLowerBoundPrimalAnisotropic;
            } else if(roundingReparametrization_ == "uniform") {
               return LPVisitorReturnType::ReparametrizeLowerBoundPrimalUniform;
            } else {
               throw std::runtime_error("unknown reparametrization mode");
            }
         } else if(curIter_ % primalComputationInterval_ == 0) {
            if(roundingReparametrization_ == "anisotropic") {
               return LPVisitorReturnType::ReparametrizePrimalAnisotropic;
            } else if(roundingReparametrization_ == "uniform") {
               return LPVisitorReturnType::ReparametrizePrimalUniform;
            } else {
               throw std::runtime_error("unknown reparametrization mode");
            }
         } else if(curIter_ % lowerBoundComputationInterval_ == 0) {
            if(standardReparametrization_ == "anisotropic") {
               return LPVisitorReturnType::ReparametrizeLowerBoundAnisotropic;
            } else if(roundingReparametrization_ == "uniform") {
               return LPVisitorReturnType::ReparametrizeLowerBoundUniform;
            } else {
               throw std::runtime_error("unknown reparametrization mode");
            }
         } else {
            if(standardReparametrization_ == "anisotropic") {
               return LPVisitorReturnType::ReparametrizeAnisotropic;
            } else if(roundingReparametrization_ == "uniform") {
               return LPVisitorReturnType::ReparametrizeUniform;
            } else {
               throw std::runtime_error("unknown reparametrization mode");
            }
         }


         
                  /*
         // do zrobienia: this whole logic is quite brittle. Think about state diagram
         switch(LP_STATE) {
            case LPVisitorReturnType::SetAnisotropicReparametrization: 
               {
               return LPVisitorReturnType::ReparametrizeAndComputePrimal;
               break;
               }
            case LPVisitorReturnType::SetRoundingReparametrization:
               {
               std::cout << "Perform rounding reparametrization step and compute primal\n";
               return LPVisitorReturnType::ReparametrizeAndComputePrimal;
               break;
               }
            case LPVisitorReturnType::ReparametrizeAndComputePrimal:
               {
               prevDualBound_ = curDualBound_;
               curDualBound_ = lp->LowerBound();
               const REAL lowerBoundDiff = curDualBound_ - prevDualBound_;
               const REAL primalCost = lp->BestPrimalBound(); 
               // measure time
               const INDEX timeElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - beginTime_).count();
               std::cout << "Iteration " << curIter_ << ", lower bound = " << curDualBound_ << ", upper bound = " << primalCost << ", time elapsed = " << timeElapsed << " ms\n";
               assert(primalCost >= curDualBound_ - eps);
               if(primalCost <= curDualBound_ + eps) {
                  ++curIter_;
                  std::cout << "Primal cost equals lower bound\n";
                  return LPVisitorReturnType::Break;
               }
               if(minDualImprovement_ > 0 && remainingIter_ > 1 && lowerBoundDiff < minDualImprovement_) {
                  ++curIter_;
                  std::cout << "Dual improvement smaller than " << minDualImprovement_ << "\n";
                  remainingIter_ = 1;
                  return LPVisitorReturnType::SetRoundingReparametrization;
               }
               }
            case LPVisitorReturnType::Reparametrize:
               {
               ++curIter_;
               --remainingIter_;
               if(remainingIter_ == 1) {
                  return LPVisitorReturnType::SetRoundingReparametrization;
               }
               if(curIter_ == maxIter_) {
                  std::cout << "Maximum number of iterations reached\n";
                  return LPVisitorReturnType::Break;
               }
               if(remainingIter_ == 0) {
                  std::cout << "Terminating optimization\n";
                  return LPVisitorReturnType::Break;
               }
               INDEX timeElapsed;
               if(timeout_ != std::numeric_limits<REAL>::max() && (timeElapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - beginTime_).count()) >= timeout_) {
                  std::cout << "Timeout reached after " << timeElapsed << " seconds\n";
                  remainingIter_ = 1;
                  return LPVisitorReturnType::SetRoundingReparametrization;
               }
               if(maxMemory_ > 0) {
                  const INDEX memoryUsed = memory_used()/(1024*1024);
                  if(maxMemory_ < memoryUsed) {
                     remainingIter_ = 1;
                     std::cout << "Solver uses " << memoryUsed << " MB memory, aborting optimization\n";
                     return LPVisitorReturnType::SetRoundingReparametrization;
                  }
               }
               if(curIter_ % boundComputationInterval_ == 0) {
                  return LPVisitorReturnType::ReparametrizeAndComputePrimal;
               }
               return LPVisitorReturnType::Reparametrize;
               break;
               }
            case LPVisitorReturnType::Break:
               {
               auto endTime = std::chrono::steady_clock::now();
               std::cout << "Optimization took " << std::chrono::duration_cast<std::chrono::milliseconds>(endTime - beginTime_).count() << " milliseconds and " << curIter_ << " iterations\n";

               return LPVisitorReturnType::Break;
               break;
               }
         }
      */
         assert(false);
      }

      
      using TimeType = decltype(std::chrono::steady_clock::now());
      TimeType GetBeginTime() const { return beginTime_; }
      REAL GetLowerBound() const { return curDualBound_; }
      INDEX GetIter() const { return curIter_; }

      protected:
      // command line arguments TCLAP
      TCLAP::ValueArg<INDEX> maxIterArg_;
      TCLAP::ValueArg<INDEX> maxMemoryArg_;
      TCLAP::ValueArg<INDEX> timeoutArg_;
      //TCLAP::ValueArg<INDEX> boundComputationIntervalArg_;
      TCLAP::ValueArg<INDEX> primalComputationIntervalArg_;
      TCLAP::ValueArg<INDEX> lowerBoundComputationIntervalArg_;
      PositiveRealConstraint posConstraint_;
      TCLAP::ValueArg<REAL> minDualImprovementArg_;
      TCLAP::ValueArg<std::string> standardReparametrizationArg_;
      TCLAP::ValueArg<std::string> roundingReparametrizationArg_;
      TCLAP::ValueArg<std::string> protocolateFileArg_;
      TCLAP::SwitchArg silentArg_;

      // command line arguments read out
      INDEX maxIter_;
      INDEX maxMemory_;
      INDEX timeout_;
      //INDEX boundComputationInterval_;
      INDEX primalComputationInterval_;
      INDEX lowerBoundComputationInterval_;
      REAL minDualImprovement_;
      std::string protocolateFile_;
      // do zrobienia: make enum for reparametrization mode
      std::string standardReparametrization_;
      std::string roundingReparametrization_;
      bool silent_;

      // internal state of visitor
      INDEX remainingIter_;
      INDEX curIter_ = 0;
      REAL prevDualBound_ = -std::numeric_limits<REAL>::max();
      REAL curDualBound_ = -std::numeric_limits<REAL>::max();
      TimeType beginTime_;

      PROBLEM_DECOMPOSITION& pd_;
   };

   template<class PROBLEM_DECOMPOSITION>
   class StandardTighteningVisitor : public StandardVisitor<PROBLEM_DECOMPOSITION>
   {
      using BaseVisitorType = StandardVisitor<PROBLEM_DECOMPOSITION>;
      public:
      StandardTighteningVisitor(TCLAP::CmdLine& cmd, PROBLEM_DECOMPOSITION& pd)
         :
            BaseVisitorType(cmd,pd),
            tightenArg_("","tighten","enable tightening",cmd,false),
            tightenIterationArg_("","tightenIteration","number of iterations after which tightening is performed for the first time, default = never",false,std::numeric_limits<INDEX>::max(),"positive integer", cmd),
            tightenIntervalArg_("","tightenInterval","number of iterations between tightenings",false,std::numeric_limits<INDEX>::max(),"positive integer", cmd),
            tightenConstraintsMaxArg_("","tightenConstraintsMax","maximal number of constraints to be added during tightening",false,20,"positive integer", cmd),
            posConstraint_(),
            tightenMinDualIncreaseArg_("","tightenMinDualIncrease","minimum increase which additional constraint must guarantee",false,0.0,&posConstraint_, cmd)
      {}

      LPVisitorReturnType begin(const LP* lp) // called, after problem is constructed. 
      {
         try {
            tighten_ = tightenArg_.getValue();
            tightenIteration_ = tightenIterationArg_.getValue();
            tightenInterval_ = tightenIntervalArg_.getValue();
            tightenConstraintsMax_ = tightenConstraintsMaxArg_.getValue();
            tightenMinDualIncrease_ = tightenMinDualIncreaseArg_.getValue();
         } catch (TCLAP::ArgException &e) {
            std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl; 
            exit(1);
         }

         std::cout << "Initial number of factors = " << lp->GetNumberOfFactors() << "\n";
         return BaseVisitorType::begin(lp);
      }

      // the default
      template<LPVisitorReturnType LP_STATE>
      LPVisitorReturnType visit(LP* lp)
      {
         const auto ret = BaseVisitorType::template visit<LP_STATE>(lp);
         // do zrobienia: introduce tighten reparametrization
         if(tighten_) {
            if(LP_STATE != LPVisitorReturnType::Break) {
               if(this->GetIter() == tightenIteration_ || this->GetIter() >= lastTightenIteration_ + tightenInterval_ || tightenIteration_ <= 0) {
               //if(tightenInNextIteration_) {
               //   tightenInNextIteration_ = false;
               //   resumeInNextIteration_ = true;
                  Tighten();
                  lastTightenIteration_ = this->GetIter();
                  lp->Init(); // reinitialize factors, as they might have changed
                  std::cout << "New number of factors = " << lp->GetNumberOfFactors() << ", tightening took " << tightenTime_ << "ms\n";
                  //if(repamModeBeforeTightening_ == LPReparametrizationMode::Anisotropic) {
                  //   return LPVisitorReturnType::SetAnisotropicReparametrization;
                  //} else if(repamModeBeforeTightening_ == LPReparametrizationMode::Rounding) {
                  //   return LPVisitorReturnType::SetRoundingReparametrization;
                  //} else {
                  //   assert(false); // has there been introduced a new reparametrization?
                  //}
               //} else if(resumeInNextIteration_) {
               //   resumeInNextIteration_ = false;
               //   return retBeforeTightening_;
               //} else if(this->GetIter() == tightenIteration_ || this->GetIter() >= lastTightenIteration_ + tightenInterval_) {
               //   tightenInNextIteration_ = true;
               //   retBeforeTightening_ = BaseVisitorType::template visit<LP_STATE>(lp);
               //   repamModeBeforeTightening_ = lp->GetRepamMode();
               //   return LPVisitorReturnType::SetRoundingReparametrization;
               }
               // if no sufficient dual increase was achieved
            } else if(LP_STATE == LPVisitorReturnType::Break) {
               std::cout << "Tightening took " << tightenTime_ << " milliseconds\n";
            }
         }
         // if dual improvement too small
         //{
         //   if(tighten_) {
         //      tightenInNextIteration_ = true;
         //      return LPVisitorReturnType::SetAnisotropicReparametrization; 
         //      // perform one forward and backward pass with rounding reparametrization and then tighten
         //      // record guaranteed dual improvement
         //   }
         //}

         return ret;
      }
      INDEX Tighten()
      {
         auto tightenBeginTime = std::chrono::steady_clock::now();
         INDEX constraintsAdded = BaseVisitorType::pd_.Tighten(tightenMinDualIncrease_, tightenConstraintsMax_);
         auto tightenEndTime = std::chrono::steady_clock::now();
         tightenTime_ += std::chrono::duration_cast<std::chrono::milliseconds>(tightenEndTime - tightenBeginTime).count();
         return constraintsAdded;
      }

      protected:
      TCLAP::SwitchArg tightenArg_;
      TCLAP::ValueArg<INDEX> tightenIterationArg_; // after how many iterations shall tightening be performed
      TCLAP::ValueArg<INDEX> tightenIntervalArg_; // interval between tightening operations.
      TCLAP::ValueArg<INDEX> tightenConstraintsMaxArg_; // How many constraints to add in tightening maximally
      PositiveRealConstraint posConstraint_;
      TCLAP::ValueArg<REAL> tightenMinDualIncreaseArg_; // only include constraints which guarantee increase larger than specified value

      bool tighten_;
      bool tightenInNextIteration_ = false;
      bool resumeInNextIteration_ = false;
      LPVisitorReturnType retBeforeTightening_;
      LPReparametrizationMode repamModeBeforeTightening_;
      
      INDEX lastTightenIteration_ = std::numeric_limits<INDEX>::max()/2; // otherwise overflow occurs and tightening start immediately
      INDEX tightenIteration_;
      INDEX tightenInterval_;
      INDEX tightenConstraintsMax_;
      REAL tightenMinDualIncrease_;
      
      INDEX tightenTime_ = 0;

   };
} // end namespace LP_MP

#endif // LP_MP_STANDARD_VISITOR_HXX