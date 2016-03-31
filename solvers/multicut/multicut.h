#ifndef LP_MP_MULTICUT_HXX
#define LP_MP_MULTICUT_HXX

#include "LP_MP.h"
#include "problem_decomposition.hxx"
#include "factors_messages.hxx"
#include "multicut_unary_factor.hxx"
#include "multicut_triplet_factor.hxx"
#include "multicut_global_factor.hxx"
#include "multicut_odd_wheel_factor.hxx"
#include "multicut_message.hxx"
#include "multicut_triplet_odd_wheel_message.hxx"
#include "multicut_constructor.hxx"

#include "parse_rules.h"

#include "hdf5.h"

#include <iostream>
#include <vector>

namespace LP_MP {

// do zrobienia: possibly rename unary to edge factor

template<MessageSending MESSAGE_SENDING>
struct FMC_MULTICUT {
   constexpr static const char* name = "Multicut";

   typedef FactorContainer<MulticutUnaryFactor, FixedSizeExplicitRepamStorage<1>::type, FMC_MULTICUT, 0, true> MulticutUnaryFactorContainer;
   typedef FactorContainer<MulticutTripletFactor, FixedSizeExplicitRepamStorage<3>::type, FMC_MULTICUT, 1> MulticutTripletFactorContainer;
   typedef FactorContainer<MulticutGlobalFactor, MulticutGlobalRepamStorage, FMC_MULTICUT, 2> MulticutGlobalFactorContainer;

   typedef MessageContainer<MulticutUnaryTripletMessage<MESSAGE_SENDING>, FixedMessageStorage<1>, FMC_MULTICUT, 0 > MulticutUnaryTripletMessageContainer;
   typedef MessageContainer<MulticutUnaryGlobalMessage, FixedMessageStorage<0>, FMC_MULTICUT, 1> MulticutUnaryGlobalMessageContainer;

   using FactorList = meta::list< MulticutUnaryFactorContainer, MulticutTripletFactorContainer, MulticutGlobalFactorContainer >;
   using MessageList = meta::list<
      MessageListItem< MulticutUnaryTripletMessageContainer, 0, 1, std::vector, FixedSizeMessageContainer<3>::type >,
      MessageListItem< MulticutUnaryGlobalMessageContainer, 0, 2, FixedSizeMessageContainer<1>::type, std::vector >
      >;

   using multicut = MulticutConstructor<FMC_MULTICUT,0,1,2,0,1>;
   using ProblemDecompositionList = meta::list<multicut>;
};

// maybe this will not work, as factors in base class have wrong FMC
template<MessageSending MESSAGE_SENDING>
struct FMC_ODD_WHEEL_MULTICUT : public FMC_MULTICUT<MESSAGE_SENDING> {
   constexpr static const char* name = "Multicut with odd wheel constraints";

   typedef FactorContainer<MulticutUnaryFactor, FixedSizeExplicitRepamStorage<1>::type, FMC_ODD_WHEEL_MULTICUT, 0, true> MulticutUnaryFactorContainer;
   typedef FactorContainer<MulticutTripletFactor, FixedSizeExplicitRepamStorage<3>::type, FMC_ODD_WHEEL_MULTICUT, 1> MulticutTripletFactorContainer;
   typedef FactorContainer<MulticutGlobalFactor, MulticutGlobalRepamStorage, FMC_ODD_WHEEL_MULTICUT, 2> MulticutGlobalFactorContainer;
   typedef FactorContainer<MulticutOddWheelFactor, ExplicitRepamStorage, FMC_ODD_WHEEL_MULTICUT, 3> MulticutOddWheelFactorContainer;

   /*
   typedef MessageContainer<MulticutUnaryOddWheelCycleMessage<MESSAGE_SENDING>, FixedMessageStorage<1>, FMC_ODD_WHEEL_MULTICUT, 2 > MulticutUnaryOddWheelCycleMessageContainer;
   typedef MessageContainer<MulticutUnaryOddWheelCenterMessage<MESSAGE_SENDING>, FixedMessageStorage<1>, FMC_ODD_WHEEL_MULTICUT, 3 > MulticutUnaryOddWheelCenterMessageContainer;
   */
   typedef MessageContainer<MulticutUnaryTripletMessage<MESSAGE_SENDING>, FixedMessageStorage<1>, FMC_ODD_WHEEL_MULTICUT, 0 > MulticutUnaryTripletMessageContainer;
   typedef MessageContainer<MulticutUnaryGlobalMessage, FixedMessageStorage<0>, FMC_ODD_WHEEL_MULTICUT, 1> MulticutUnaryGlobalMessageContainer;
   typedef MessageContainer<MulticutTripletOddWheelMessage<MESSAGE_SENDING>, FixedMessageStorage<3>, FMC_ODD_WHEEL_MULTICUT, 2 > MulticutTripletOddWheelMessageContainer;

   using FactorList = meta::list< MulticutUnaryFactorContainer, MulticutTripletFactorContainer, MulticutGlobalFactorContainer, MulticutOddWheelFactorContainer >;
   using MessageList = meta::list<
      MessageListItem< MulticutUnaryTripletMessageContainer, 0, 1, std::vector, FixedSizeMessageContainer<3>::type >,
      MessageListItem< MulticutUnaryGlobalMessageContainer, 0, 2, FixedSizeMessageContainer<1>::type, std::vector >,
      MessageListItem< MulticutTripletOddWheelMessageContainer, 1, 3, std::vector, std::vector >
      >;

   using multicut = MulticutOddWheelConstructor<FMC_ODD_WHEEL_MULTICUT,0,1,2,0,1,3,2>;
   using ProblemDecompositionList = meta::list<multicut>;
};

namespace MulticutOpenGmInput {

   INDEX GetDimension(const hid_t handle, const std::string datasetName) 
   {
      auto dataset = H5Dopen(handle, datasetName.c_str(), H5P_DEFAULT);
      if(dataset < 0) {
         throw std::runtime_error("cannot open dataset " + datasetName);
      }
      hid_t filespace = H5Dget_space(dataset);
      hsize_t dimension = H5Sget_simple_extent_ndims(filespace);
      assert(dimension == 1);
      hsize_t shape[] = {0};// hsize_t[(size_t)(dimension)];
      auto status = H5Sget_simple_extent_dims(filespace, shape, NULL);
      assert(status >= 0);

      H5Dclose(dataset);
      H5Sclose(filespace);

      return shape[0];
   }
   template<typename T>
   std::vector<T> ReadVector(const hid_t handle, const std::string datasetName)
   {
      herr_t status;
      auto dataset = H5Dopen(handle, datasetName.c_str(), H5P_DEFAULT);
      if(dataset < 0) {
         throw std::runtime_error("cannot open dataset " + datasetName);
      }
      hid_t filespace = H5Dget_space(dataset);
      hid_t type = H5Dget_type(dataset);
      hid_t nativeType = H5Tget_native_type(type, H5T_DIR_DESCEND);
      /*
      if(!H5Tequal(nativeType, hdf5Type<T>())) {
        H5Dclose(dataset);
        H5Tclose(nativeType);
        H5Tclose(type);
        H5Sclose(filespace);
        throw std::runtime_error("Data types not equal error.");
      }
      */

      hsize_t dimension = H5Sget_simple_extent_ndims(filespace);
      assert(dimension == 1);
      hsize_t shape[] = {0};// hsize_t[(size_t)(dimension)];
      status = H5Sget_simple_extent_dims(filespace, shape, NULL);
      assert(status >= 0);
      hid_t memspace = H5Screate_simple(dimension, &shape[0], NULL);

      std::vector<T> out(shape[0]);
      status = H5Dread(dataset, nativeType, memspace, filespace, H5P_DEFAULT, &out[0]);
      H5Dclose(dataset);
      H5Tclose(nativeType);
      H5Tclose(type);
      H5Sclose(memspace);
      H5Sclose(filespace);
      assert(status >= 0);
      return out;
   }

   template<typename FMC>
   bool ParseProblem(const std::string filename, ProblemDecomposition<FMC>& pd)
   {
      //read with hdf5
      auto fileHandle = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
      if(fileHandle < 0) {
         throw std::runtime_error("cannot open file " + filename);
      }
      // first look into dataset "gm".
      auto gmHandle = H5Gopen(fileHandle, "gm", H5P_DEFAULT);
      if(gmHandle < 0) {
         throw std::runtime_error("Could not open HDF5 group gm in " + filename);
      }

      // Read 'numbers-of-states' to get the variable number. Note: this is an array of length equal to number of variables, with each entry having the length as its number.
      // -> numberOfVariables
      INDEX numberOfVariables = GetDimension(gmHandle, "numbers-of-states"); 
      std::cout << "Has " << numberOfVariables << " variables\n";

      // second, read edges in graph in 'factors' in chunks of 5. (${factorNo},1,2,${i1},${i2}), where (i1,i2) is the edge
      // note: we must double check the datatype here
      static_assert(sizeof(size_t) == 8,"HDF5 file format has 64 bits for integers");
      auto factor = ReadVector<size_t>(gmHandle, "factors");
      std::vector<std::tuple<INDEX,INDEX,REAL>> edges;
      for(INDEX i=0; i<factor.size()/5; ++i) {
         const INDEX factorNo = factor[5*i];
         const INDEX one = factor[5*i+1];
         const INDEX two = factor[5*i+2];
         const INDEX i1 = factor[5*i+3];
         const INDEX i2 = factor[5*i+4];
         edges.push_back(std::make_tuple(i1,i2,0.0));
      }

      // third, read edge costs in 'function-id-...16006' in chunks of two. (${x1},${x2}) -> the edge cost is x1-x2 plus some global offset, if x1 != 0
      auto functionHandle = H5Gopen(gmHandle, "function-id-16006", H5P_DEFAULT);
      assert(functionHandle >= 0);
      static_assert(sizeof(REAL) == 8, "HDF5 file format has 64 bits for floats");
      auto edgeCosts = ReadVector<REAL>(functionHandle,"values");
      assert(edgeCosts.size()/2 == edges.size());
      for(INDEX i=0; i<edges.size(); ++i) {
         std::get<2>(edges[i]) = -edgeCosts[2*i] + edgeCosts[2*i+1]; // do zrobienia: or the other way around
      }
      // note: theoretically, this could be much more complicated, if some factors are shared etc. Then this simple approach above will not work.

      auto& mc = pd.template GetProblemConstructor<0>();
      for(INDEX i=0; i<edges.size(); ++i) {
         INDEX i1 = std::get<0>(edges[i]); 
         INDEX i2 = std::get<1>(edges[i]); 
         if(i1 > i2) {
            std::swap(i1,i2);
         }
         const REAL cost = std::get<2>(edges[i]);
         //std::cout << "add edge (" << i1 << "," << i2 << ") with cost " << cost << "\n";
         mc.AddUnaryFactor(i1, i2, cost);
      }
      //mc.Tighten(*numberOfVariables);

      return true;
   }

}

namespace MulticutTextInput {
   struct init_line : pegtl::seq< opt_whitespace, pegtl::string<'M','U','L','T','I','C','U','T'>, opt_whitespace > {};
   struct numberOfVariables_line : pegtl::seq< opt_whitespace, positive_integer, opt_whitespace > {};
   struct edge_line : pegtl::seq< opt_whitespace, positive_integer, opt_whitespace, positive_integer, opt_whitespace, real_number, opt_whitespace > {};

   struct grammar : pegtl::must<
                    init_line, pegtl::eol,
                    numberOfVariables_line, pegtl::eol,
                    pegtl::star<edge_line, pegtl::eol>,
                    pegtl::opt<edge_line>,
                    pegtl::star<pegtl::sor<mand_whitespace, pegtl::eol>>,
                    pegtl::eof> {};

   struct MulticutInput {
      INDEX numberOfVariables_;
      std::vector<std::tuple<INDEX,INDEX,REAL>> edges_;
   };

   template<typename FMC, typename Rule >
      struct action
      : pegtl::nothing< Rule > {};


   template<typename FMC> struct action<FMC, positive_integer > {
      static void apply(const pegtl::input & in, ProblemDecomposition<FMC>&, std::stack<SIGNED_INDEX>& integer_stack, std::stack<REAL>& real_stack, MulticutInput &)
      {
         integer_stack.push(std::stoul(in.string())); 
      }
   };
   template<typename FMC> struct action<FMC, real_number > {
      static void apply(const pegtl::input & in, ProblemDecomposition<FMC>&, std::stack<SIGNED_INDEX>& integer_stack, std::stack<REAL>& real_stack, MulticutInput &)
      {
         real_stack.push(std::stod(in.string())); 
      }
   };
   template<typename FMC> struct action<FMC, numberOfVariables_line > {
      static void apply(const pegtl::input & in, ProblemDecomposition<FMC>&, std::stack<SIGNED_INDEX>& integer_stack, std::stack<REAL>& real_stack, MulticutInput & mcInput)
      {
         assert(integer_stack.size() == 1);
         mcInput.numberOfVariables_ = integer_stack.top();
         integer_stack.pop();
      }
   };
   template<typename FMC> struct action<FMC, edge_line > {
      static void apply(const pegtl::input & in, ProblemDecomposition<FMC>&, std::stack<SIGNED_INDEX>& integer_stack, std::stack<REAL>& real_stack, MulticutInput & mcInput)
      {
         assert(integer_stack.size() == 2);
         assert(real_stack.size() == 1);
         INDEX i2 = integer_stack.top();
         integer_stack.pop();
         INDEX i1 = integer_stack.top();
         integer_stack.pop();
         const REAL cost = real_stack.top();
         real_stack.pop();
         if(i1 > i2) {
            std::swap(i1,i2);
         }
         assert(i1 < i2);
         assert(i2 < mcInput.numberOfVariables_);
         mcInput.edges_.push_back(std::make_tuple(i1,i2,cost));
      }
   };
   template<typename FMC> struct action<FMC, pegtl::eof> {
      static void apply(const pegtl::input & in, ProblemDecomposition<FMC>& pd, std::stack<SIGNED_INDEX>& integer_stack, std::stack<REAL>& real_stack, MulticutInput& mcInput)
      {
         auto& mc = pd.template GetProblemConstructor<0>();
         for(const auto& it : mcInput.edges_) {
            mc.AddUnaryFactor( std::get<0>(it), std::get<1>(it), std::get<2>(it) );
         }
         //mc.Tighten(0.0,mcInput.numberOfVariables_); // initial tightening
         // only for odd wheel example
         mc.AddTripletFactor(0,1,2);
         mc.AddTripletFactor(0,1,3);
         mc.AddTripletFactor(0,2,3);
         mc.AddTripletFactor(1,2,3);
         //mc.AddOddWheelFactor(0, std::vector<INDEX>{1,2,3});
         //mc.AddOddWheelFactor(1, std::vector<INDEX>{0,2,3});
         //mc.AddOddWheelFactor(2, std::vector<INDEX>{0,1,3});
         //mc.AddOddWheelFactor(3, std::vector<INDEX>{0,1,2});
      }
   };

   template<typename FMC>
      struct actionSpecialization {
         template<typename RULE> struct type : public action<FMC,RULE> {};
      };
      
   template<typename FMC>
   bool ParseProblem(const std::string filename, ProblemDecomposition<FMC>& pd)
   {
      std::stack<SIGNED_INDEX> integer_stack;
      std::stack<REAL> real_stack;
      MulticutInput mcInput;
      std::cout << "parsing " << filename << "\n";

      pegtl::file_parser problem(filename);

      return problem.parse< grammar, actionSpecialization<FMC>::template type >(pd, integer_stack, real_stack, mcInput);
   }

} // end namespace MulticutTextInput

} // end namespace LP_MP

#endif // LP_MP_MULTICUT_HXX
