#ifndef LP_MP_UNION_FIND_HXX
#define LP_MP_UNION_FIND_HXX

namespace LP_MP {
class union_find {
   std::size_t *id, cnt, *sz, N; // it is not necessary to hold sz!
public:
   // Create an empty union find data structure with N isolated sets.
   union_find(std::size_t _N) : N(_N) {
      id = new std::size_t[2*N];
      sz = id + N;
      reset();
   }
   ~union_find() {
      delete [] id;
   }
   void reset() {
      cnt = N;
      for(std::size_t i=0; i<N; ++i) { id[i] = i; }
      for(std::size_t i=0; i<N; ++i) { sz[i] = 1; }
   }
   // Return the id of component corresponding to object p.
   std::size_t find(std::size_t p) {
      assert(p < N);
      std::size_t root = p;
      while (root != id[root])
         root = id[root];
      while (p != root) {
         std::size_t newp = id[p];
         id[p] = root;
         p = newp;
      }
      return root;
   }
   // Replace sets containing x and y with their union.
   void merge(std::size_t x, std::size_t y) {
      std::size_t i = find(x);
      std::size_t j = find(y);
      if(i == j) return;

      // make smaller root point to larger one
      if(sz[i] < sz[j])	{ 
         id[i] = j; 
         sz[j] += sz[i]; 
      } else	{ 
         id[j] = i; 
         sz[i] += sz[j]; 
      }
      cnt--;
   }
   // Are objects x and y in the same set?
   bool connected(std::size_t x, std::size_t y) {
      return find(x) == find(y);
   }

   std::size_t thread_safe_find(std::size_t p) const {
      std::size_t root = p;
      while (root != id[root])
         root = id[root];
      return root;
   }
   bool thread_safe_connected(std::size_t x, std::size_t y) const {
      return thread_safe_find(x) == thread_safe_find(y);
   }
   // Return the number of disjoint sets.
   std::size_t count() {
      return cnt;
   }

   std::vector<std::size_t> get_contiguous_ids()
   {
      std::vector<std::size_t> contiguous_ids(N);
      std::vector<std::size_t> id_mapping(N, std::numeric_limits<std::size_t>::max());
      //std::size_t* id_mapping = new std::size_t[N];
      //std::fill(contiguous_ids.begin(), contiguous_idx.end(), std::numeric_limits<std::size_t>::max());
      for(std::size_t i=0; i<N; ++i) {
         std::size_t d = find(i);
         id_mapping[d] = 1; 
      }
      std::size_t next_id = 0;
      for(std::size_t d=0; d<N; ++d) {
         if(id_mapping[d] == 1) {
            id_mapping[d] = next_id;
            ++next_id;
         }
      }

      for(std::size_t i=0; i<N; ++i) {
         std::size_t d = find(i);
         assert(id_mapping[d] != std::numeric_limits<std::size_t>::max());
         contiguous_ids[i] = id_mapping[d];
      }
      return std::move(id_mapping);
   }
};

}; // end namespace LP_MP

#endif // LP_MP_UNION_FIND_HXX
