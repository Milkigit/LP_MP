#ifndef LP_MP_VECTOR_HXX
#define LP_MP_VECTOR_HXX

#include "memory_allocator.hxx"
#include "serialization.hxx"
#include "config.hxx"
//#include "cereal/archives/binary.hpp"

namespace LP_MP {

// fixed size vector allocated from block allocator
// possibly holding size explicitly is not needed: It is held by allocator as well

// primitive expression templates for vector
template<typename T, typename E>
class vector_expression {
public:
   T operator[](const INDEX i) const { return static_cast<E const&>(*this)[i]; }
   T operator()(const INDEX i1, const INDEX i2) const { return static_cast<E const&>(*this)(i1,i2); }
   T operator()(const INDEX i1, const INDEX i2, const INDEX i3) const { return static_cast<E const&>(*this)(i1,i2,i3); }
   INDEX size() const { return static_cast<E const&>(*this).size(); }
   INDEX dim1() const { return static_cast<E const&>(*this).dim1(); }
   INDEX dim2() const { return static_cast<E const&>(*this).dim2(); }
   INDEX dim3() const { return static_cast<E const&>(*this).dim3(); }

   E& operator()() { return static_cast<E&>(*this); }
   const E& operator()() const { return static_cast<const E&>(*this); }
};

template<typename T, typename E>
class matrix_expression {
public:
   T operator[](const INDEX i) const { return static_cast<E const&>(*this)[i]; }
   INDEX size() const { return static_cast<E const&>(*this).size(); }

   T operator()(const INDEX i1, const INDEX i2) const { return static_cast<E const&>(*this)(i1,i2); }
   INDEX dim1() const { return static_cast<E const&>(*this).dim1(); }
   INDEX dim2() const { return static_cast<E const&>(*this).dim2(); }

   E& operator()() { return static_cast<E&>(*this); }
   const E& operator()() const { return static_cast<const E&>(*this); }
};

template<typename T, typename E>
class tensor3_expression {
public:
   T operator[](const INDEX i) const { return static_cast<E const&>(*this)[i]; }
   INDEX size() const { return static_cast<E const&>(*this).size(); }

   T operator()(const INDEX i1, const INDEX i2) const { return static_cast<E const&>(*this)(i1,i2); }
   T operator()(const INDEX i1, const INDEX i2, const INDEX i3) const { return static_cast<E const&>(*this)(i1,i2,i3); }
   INDEX dim1() const { return static_cast<E const&>(*this).dim1(); }
   INDEX dim2() const { return static_cast<E const&>(*this).dim2(); }
   INDEX dim3() const { return static_cast<E const&>(*this).dim3(); }

   E& operator()() { return static_cast<E&>(*this); }
   const E& operator()() const { return static_cast<const E&>(*this); }
};


// possibly also support different allocators: a pure stack allocator without block might be a good choice as well for short-lived memory
template<typename T=REAL>
class vector : public vector_expression<T,vector<T>> {
public:
  template<typename ITERATOR>
  vector(ITERATOR begin, ITERATOR end)
  {
    const INDEX size = std::distance(begin,end);
    const INDEX padding = (REAL_ALIGNMENT-(size%REAL_ALIGNMENT))%REAL_ALIGNMENT;
    assert(size > 0);
    begin_ = global_real_block_allocator_array[stack_allocator_index].allocate(size+padding,32);
    assert(begin_ != nullptr);
    end_ = begin_ + size;
    for(auto it=this->begin(); begin!=end; ++begin, ++it) {
      (*it) = *begin;
    }
    if(padding != 0) {
      std::fill(end_, end_ + padding, std::numeric_limits<REAL>::infinity());
    }
  }

  vector(const INDEX size) 
  {
    const INDEX padding = (REAL_ALIGNMENT-(size%REAL_ALIGNMENT))%REAL_ALIGNMENT;
    assert((padding + size)%REAL_ALIGNMENT == 0);
    begin_ = global_real_block_allocator_array[stack_allocator_index].allocate(size+padding,32);
    assert(size > 0);
    assert(begin_ != nullptr);
    end_ = begin_ + size;
    // infinities in padding
    if(padding != 0) {
      std::fill(end_, end_ + padding, std::numeric_limits<REAL>::infinity());
    }
  }
  vector(const INDEX size, const T value) 
     : vector(size)
  {
     assert(size > 0);
     std::fill(begin_, end_, value);
  }
  ~vector() {
     if(begin_ != nullptr) {
        global_real_block_allocator_array[stack_allocator_index].deallocate(begin_,1);
     }
  }
   vector(const vector& o)  {
     const INDEX padding = (REAL_ALIGNMENT-(o.size()%REAL_ALIGNMENT))%REAL_ALIGNMENT;
     begin_ = global_real_block_allocator_array[stack_allocator_index].allocate(o.size()+padding,32);
     end_ = begin_ + o.size();
     assert(begin_ != nullptr);
     auto it = begin_;
     for(auto o_it = o.begin(); o_it!=o.end(); ++it, ++o_it) { *it = *o_it; }
     if(padding != 0) {
       std::fill(end_, end_ + padding, std::numeric_limits<REAL>::infinity());
     }
   }
   vector(vector&& o) {
      begin_ = o.begin_;
      end_ = o.end_;
      o.begin_ = nullptr;
      o.end_ = nullptr;
   }
   template<typename E>
   void operator=(const vector_expression<T,E>& o) {
      assert(size() == o.size());
      for(INDEX i=0; i<o.size(); ++i) { 
         (*this)[i] = o[i]; }
   }
   template<typename E>
   void operator-=(const vector_expression<T,E>& o) {
      assert(size() == o.size());
      for(INDEX i=0; i<o.size(); ++i) { 
         (*this)[i] -= o[i]; } 
   }
   template<typename E>
   void operator+=(const vector_expression<T,E>& o) {
      assert(size() == o.size());
      for(INDEX i=0; i<o.size(); ++i) { 
         (*this)[i] += o[i]; } 
   }

   void operator+=(const REAL x) {
      for(INDEX i=0; i<this->size(); ++i) { 
         (*this)[i] += x;
      } 
   }

   void operator-=(const REAL x) {
      for(INDEX i=0; i<this->size(); ++i) { 
         (*this)[i] -= x;
      } 
   }


   // force construction from expression template
   template<typename E>
   vector(vector_expression<T,E>& v) : vector(v.size())
   {
      for(INDEX i=0; v.size(); ++i) {
         (*this)[i] = v[i];
      }
   }

   INDEX size() const { return end_ - begin_; }

   T operator[](const INDEX i) const {
      assert(i<size());
      assert(!std::isnan(begin_[i]));
      return begin_[i];
   }
   T& operator[](const INDEX i) {
      assert(i<size());
      assert(!std::isnan(begin_[i]));
      return begin_[i];
   }
   using iterator = T*;
   T* begin() const { return begin_; }
   T* end() const { return end_; }

   T back() const { return *(end_-1); }
   T& back() { return *(end_-1); }

   template<typename ARCHIVE>
   void serialize(ARCHIVE& ar)
   {
      assert(false);
      //ar( cereal::binary_data( begin_, sizeof(T)*size()) );
      //ar( binary_data<REAL>( begin_, sizeof(T)*size()) );
   }

   void prefetch() const { simdpp::prefetch_read(begin_); }

   // minimum operation with simd instructions (when T is float, double or integer)
   T min() const
   {
     //check for correct alignment
     //std::cout << std::size_t(begin_) << " aligned?" << std::endl;
     //if((std::size_t(begin_) % 32) != 0) {
     //  std::cout << std::size_t(begin_) << "not aligned" << std::endl;
     //  assert(false);
     //}

     if(std::is_same<T,double>::value) {

       simdpp::float64<4> min_val = simdpp::load( begin_ );
       for(auto it=begin_+4; it<end_; it+=4) {
         simdpp::float64<4> tmp = simdpp::load( it );
         min_val = simdpp::min(min_val, tmp); 
       }
       return simdpp::reduce_min(min_val);

     } else if(std::is_same<T,float>::value) {

       simdpp::float32<8> min_val = simdpp::load( begin_ );
       for(auto it=begin_+8; it<end_; it+=8) {
         simdpp::float32<8> tmp = simdpp::load( it );
         min_val = simdpp::min(min_val, tmp); 
       }
       return simdpp::reduce_min(min_val);

     } else {
       return *std::min_element(begin(), end());
     }
   }

   void min(const T val)
   {
     static_assert(std::is_same<T,float>::value || std::is_same<T,double>::value,"");
     if(std::is_same<T,float>::value) {
       simdpp::float32<8> val_vec = simdpp::make_float(val);
       for(auto it=begin_+8; it<end_; it+=8) {
         simdpp::float32<8> tmp = simdpp::load( it );
         tmp = simdpp::min(val_vec, tmp); 
         simdpp::store(it, tmp);
       }
     } else if(std::is_same<T,double>::value) {
       simdpp::float64<4> val_vec = simdpp::make_float(val);
       for(auto it=begin_+4; it<end_; it+=4) {
         simdpp::float64<4> tmp = simdpp::load( it );
         tmp = simdpp::min(val_vec, tmp); 
         simdpp::store(it, tmp);
       }
     } else {
       assert(false);
     } 
   }

   std::array<T,2> two_min() const
   {
     //assert(std::is_same<T,float>::value == true);

     simdpp::float32<8> min_val = simdpp::make_float(std::numeric_limits<REAL>::infinity());
     simdpp::float32<8> second_min_val = simdpp::make_float(std::numeric_limits<REAL>::infinity());
     for(auto it=begin_; it<end_; it+=8) {
         simdpp::float32<8> tmp = simdpp::load( it );
         simdpp::float32<8> tmp_min = simdpp::min(tmp, min_val);
         simdpp::float32<8> tmp_max = simdpp::max(tmp, min_val);
         min_val = tmp_min;
         second_min_val = simdpp::min(tmp_max, second_min_val); 
     }

     // second minimum is the minimum of the second minimum in vector min and the minimum in second_min_val
     simdpp::float32<4> x1;
     simdpp::float32<4> y1;
     simdpp::split(min_val, x1, y1);
     simdpp::float32<4> min2 = simdpp::min(x1, y1);
     simdpp::float32<4> max2 = simdpp::max(x1, y1);

     std::array<T,4> min_array; //
     min_array[0] = min2.vec(0); //min2.vec(1), min2.vec(2), min2.vec(3)});
     const auto min3 = two_smallest_elements<T>(min_array.begin(), min_array.end());
     //const REAL min = simdpp::reduce_min(min2);
     const REAL second_min = std::min({simdpp::reduce_min(second_min_val), simdpp::reduce_min(max2), min3[1]});
     return std::array<T,2>({min3[0], second_min});
     /*
     simdpp::float32<2> x2;
     simdpp::float32<2> y2;
     simdpp::split(min2, x2, y2);
     simdpp::float32<2> min3 = simdpp::min(x2, y2);
     simdpp::float32<2> max3 = simdpp::max(x2, y2);


     const REAL min = simdpp::reduce_min(min3);
     const REAL second_min = std::min({simdpp::reduce_min(second_min_val), simdpp::reduce_min(max2), simdpp::reduce_min(max3), simdpp::reduce_max(min3)});
     assert(min == two_smallest_elements<T>(begin_, end_)[0]);
     assert(second_min == two_smallest_elements<T>(begin_, end_)[1]);
     return std::array<T,2>({min, second_min});
     */
   }

private:
  T* begin_;
  T* end_;
};

template<typename T, INDEX N>
using array_impl = std::array<T,N>;



template<typename T, INDEX N>
class array : public vector_expression<T,array<T,N>> {
public:
   array() {}

   template<typename ITERATOR>
   array(ITERATOR begin, ITERATOR end)
   {
      const INDEX size = std::distance(begin,end);
      assert(size == N);
      for(auto it=array_.begin(); it!=array_.end(); ++it) {
         (*it) = *begin;
      }
   }

   array(const T value) 
   {
      std::fill(array_.begin(), array_.end(), value);
   }
   array(const array<T,N>& o)  {
      assert(size() == o.size());
      auto it = begin();
      for(auto o_it = o.begin(); o_it!=o.end(); ++it, ++o_it) { *it = *o_it; }
   }
   template<typename E>
   void operator=(const vector_expression<T,E>& o) {
      assert(size() == o.size());
      for(INDEX i=0; i<o.size(); ++i) { 
         (*this)[i] = o[i]; }
   }
   template<typename E>
   void operator-=(const vector_expression<T,E>& o) {
      assert(size() == o.size());
      for(INDEX i=0; i<o.size(); ++i) { 
         (*this)[i] -= o[i]; } 
   }
   template<typename E>
   void operator+=(const vector_expression<T,E>& o) {
      assert(size() == o.size());
      for(INDEX i=0; i<o.size(); ++i) { 
         (*this)[i] += o[i]; } 
   }


   // force construction from expression template
   template<typename E>
   array(vector_expression<T,E>& v) 
   {
      for(INDEX i=0; v.size(); ++i) {
         (*this)[i] = v[i];
      }
   }

   constexpr static INDEX size() { return N; }

   T operator[](const INDEX i) const {
      assert(i<size());
      assert(!std::isnan(array_[i]));
      return array_[i];
   }
   T& operator[](const INDEX i) {
      assert(i<size());
      return array_[i];
   }
   using iterator = T*;
   auto begin() const { return array_.begin(); }
   auto end() const { return array_.end(); }
   auto begin() { return array_.begin(); }
   auto end() { return array_.end(); }

   template<typename ARCHIVE>
   void serialize(ARCHIVE& ar)
   {
      ar( array_ );
   } 

private:
   std::array<T,N> array_;
};


// matrix is based on vector.
// However, the entries are padded so that each coordinate (i,0) is aligned
template<typename T=REAL>
class matrix : public matrix_expression<T,matrix<T>> {
public:
   T& operator[](const INDEX i) { return vec_[i]; }
   const T operator[](const INDEX i) const { return vec_[i]; }
   INDEX size() const { return vec_.size(); }
   template<typename ARCHIVE>
   void serialize(ARCHIVE& ar)
   {
      ar( vec_ );
   }

   T* begin() const { return vec_.begin(); }
   T* end() const { return vec_.end(); }

   static INDEX underlying_vec_size(const INDEX d1, const INDEX d2)
   {
     return d1*(d2 + padding(d2));
   }
   static INDEX padding(const INDEX i) {
     static_assert(std::is_same<T,float>::value || std::is_same<T,double>::value,"");
     return (REAL_ALIGNMENT-(i%REAL_ALIGNMENT))%REAL_ALIGNMENT;
   }

   INDEX padded_dim2() const { return padded_dim2_; }

   void fill_padding()
   {
     for(INDEX x1=0; x1<dim1(); ++x1) {
       for(INDEX x2=dim2(); x2<padded_dim2(); ++x2) {
         vec_[x1*padded_dim2() + x2] = std::numeric_limits<REAL>::infinity();
       }
     }
   }
   matrix(const INDEX d1, const INDEX d2) : vec_(underlying_vec_size(d1,d2)), dim2_(d2), padded_dim2_(d2 + padding(d2)) {
      assert(d1 > 0 && d2 > 0);
      fill_padding();
   }
   matrix(const INDEX d1, const INDEX d2, const T val) : vec_(underlying_vec_size(d1,d2)), dim2_(d2), padded_dim2_(d2 + padding(d2)) {
      assert(d1 > 0 && d2 > 0);
      std::fill(this->begin(), this->end(), val);
      fill_padding();
   }
   matrix(const matrix& o) 
      : vec_(o.vec_),
      dim2_(o.dim2_),
      padded_dim2_(o.padded_dim2_)
   {}
   matrix(matrix&& o) 
      : vec_(std::move(o.vec_)),
      dim2_(o.dim2_),
      padded_dim2_(o.padded_dim2_)
   {}
   void operator=(const matrix<T>& o) {
      assert(this->size() == o.size() && o.dim2_ == dim2_);
      // to do: use SIMD
      for(INDEX i=0; i<o.size(); ++i) { 
         vec_[i] = o[i]; 
      }
   }
   T& operator()(const INDEX x1, const INDEX x2) { assert(x1<dim1() && x2<dim2()); return vec_[x1*padded_dim2() + x2]; }
   T operator()(const INDEX x1, const INDEX x2) const { assert(x1<dim1() && x2<dim2()); return vec_[x1*padded_dim2() + x2]; }
   T& operator()(const INDEX x1, const INDEX x2, const INDEX x3) { assert(x3 == 0); return (*this)(x1,x2); } // sometimes we treat a matrix as a tensor with trivial last dimension
   T operator()(const INDEX x1, const INDEX x2, const INDEX x3) const { assert(x3 == 0); return (*this)(x1,x2); } // sometimes we treat a matrix as a tensor with trivial last dimension

   const INDEX dim1() const { return vec_.size()/padded_dim2(); }
   const INDEX dim2() const { return dim2_; }

   void transpose() {
      assert(dim1() == dim2());
      for(INDEX x1=0; x1<dim1(); ++x1) {
         for(INDEX x2=0; x2<x1; ++x2) {
            std::swap((*this)(x1,x2), (*this)(x2,x1));
         }
      }
   }

   // should these functions be members?
   // minima along second dimension
   // should be slower than min2
   vector<T> min1() const
   {
     vector<T> min(dim1());
     if(std::is_same<T,float>::value || std::is_same<T,double>::value) {
       for(INDEX x1=0; x1<dim1(); ++x1) {
         REAL_VECTOR cur_min = simdpp::load( vec_.begin() + x1*padded_dim2() );
         for(INDEX x2=8; x2<dim2(); x2+=8) {
           REAL_VECTOR tmp = simdpp::load( vec_.begin() + x1*padded_dim2() + x2 );
           cur_min = simdpp::min(cur_min, tmp); 
         }
         min[x1] = simdpp::reduce_min(cur_min); 
       }
     } else {
       assert(false);
     }

     return std::move(min);
   }

   // minima along first dimension
   vector<T> min2() const
   {
     vector<T> min(dim2());
     // possibly iteration strategy is faster, e.g. doing a non-contiguous access, or explicitly holding a few variables and not storing them back in vector min for a few sizes
     if(std::is_same<T,float>::value || std::is_same<T,double>::value) {
       for(INDEX x2=0; x2<dim2(); x2+=8) {
         REAL_VECTOR tmp = simdpp::load( vec_.begin() + x2 );
         simdpp::store(&min[x2], tmp);
       }

       for(INDEX x1=1; x1<dim1(); ++x1) {
         for(INDEX x2=0; x2<dim2(); x2+=8) {
           REAL_VECTOR cur_min = simdpp::load( vec_.begin() + x1*padded_dim2() + x2 );
           REAL_VECTOR tmp = simdpp::load( vec_.begin() + x1*padded_dim2() + x2 );
           cur_min = simdpp::min(cur_min, tmp);
           simdpp::store(&min[x2], cur_min);
         } 
       }
     } else {
       assert(false);
     }

     return std::move(min); 
   }
protected:
   vector<T> vec_;
   const INDEX dim2_;
   const INDEX padded_dim2_; // possibly do not store but compute when needed?
};

template<typename T=REAL>
class tensor3 : public vector<T> { // do zrobienia: remove
public:
   tensor3(const INDEX d1, const INDEX d2, const INDEX d3) : vector<T>(d1*d2*d3), dim2_(d2), dim3_(d3) {}
   tensor3(const INDEX d1, const INDEX d2, const INDEX d3, const T val) : tensor3<T>(d1,d2,d3) {
      std::fill(this->begin(), this->end(), val);
   }
   tensor3(const tensor3<T>& o) :
      vector<T>(o),
      dim2_(o.dim2_),
      dim3_(o.dim3_)
   {}
   tensor3(tensor3&& o) :
      vector<T>(std::move(o)),
      dim2_(o.dim2_),
      dim3_(o.dim3_)
   {}
   void operator=(const tensor3<T>& o) {
      assert(this->size() == o.size() && o.dim2_ == dim2_ && o.dim3_ == dim3_);
      for(INDEX i=0; i<o.size(); ++i) { 
         (*this)[i] = o[i]; 
      }
   }
   T& operator()(const INDEX x1, const INDEX x2, const INDEX x3) { 
      assert(x1<dim1() && x2<dim2() && x3<dim3());
      return (*this)[x1*dim2_*dim3_ + x2*dim3_ + x3]; 
   }
   T operator()(const INDEX x1, const INDEX x2, const INDEX x3) const { return (*this)[x1*dim2_*dim3_ + x2*dim3_ + x3]; }
   const INDEX dim1() const { return this->size()/(dim2_*dim3_); }
   const INDEX dim2() const { return dim2_; }
   const INDEX dim3() const { return dim3_; }
protected:
   const INDEX dim2_, dim3_;
};

template<INDEX FIXED_DIM, typename T=REAL>
class matrix_view_of_tensor : public vector_expression<T,matrix_view_of_tensor<FIXED_DIM,T>> {
public:
   matrix_view_of_tensor(tensor3<T>& t, const INDEX fixed_index) : fixed_index_(fixed_index), t_(t) {}
   ~matrix_view_of_tensor() {
      static_assert(FIXED_DIM < 3,"");
   }
   const INDEX size() const { 
      if(FIXED_DIM==0) {
         return t_.dim2()*t_.dim3();
      } else if(FIXED_DIM == 1) {
         return t_.dim1()*t_.dim2();
      } else {
         return t_.dim2()*t_.dim3();
      }
   }
   const INDEX dim1() const { 
      if(FIXED_DIM==0) {
         return t_.dim2();
      } else {
         return t_.dim1();
      }
   }
   const INDEX dim2() const { 
      if(FIXED_DIM==2) {
         return t_.dim2();
      } else {
         return t_.dim3();
      }
   }
   T& operator()(const INDEX x1, const INDEX x2) { 
      if(FIXED_DIM==0) {
         return t_(fixed_index_,x1,x2);
      } else if(FIXED_DIM == 1) {
         return t_(x1,fixed_index_,x2);
      } else {
         return t_(x1,x2,fixed_index_);
      }
   }
   T operator()(const INDEX x1, const INDEX x2) const { 
      if(FIXED_DIM==0) {
         return t_(fixed_index_,x1,x2);
      } else if(FIXED_DIM == 1) {
         return t_(x1,fixed_index_,x2);
      } else {
         return t_(x1,x2,fixed_index_);
      }
   }

private:
   const INDEX fixed_index_;
   tensor3<T>& t_;
};

// primitive expression templates for all the above linear algebraic classes
template<typename T, typename E>
struct scaled_vector : public vector_expression<T,scaled_vector<T,E>> {
   scaled_vector(const T& omega, const E& a) : omega_(omega), a_(a) {}
   const T operator[](const INDEX i) const {
      return omega_*a_[i];
   }
   const T operator()(const INDEX i, const INDEX j) const {
      return omega_*a_(i,j);
   }
   const T operator()(const INDEX i, const INDEX j, const INDEX k) const {
      return omega_*a_(i,j,k);
   }
   INDEX size() const { return a_.size(); }
   INDEX dim1() const { return a_.dim1(); }
   INDEX dim2() const { return a_.dim2(); }
   INDEX dim3() const { return a_.dim3(); }
   private:
   const T omega_;
   const E& a_;
};


template<typename T, typename E>
scaled_vector<T,vector_expression<T,E>> 
operator*(const T omega, const vector_expression<T,E> & v) {
   return scaled_vector<T,vector_expression<T,E>>(omega, v);
}

template<typename T, typename E>
scaled_vector<T,matrix_expression<T,E>> 
operator*(const T omega, const matrix_expression<T,E> & v) {
   return scaled_vector<T,matrix_expression<T,E>>(omega, v);
}


template<typename T, typename E>
struct minus_vector : public vector_expression<T,minus_vector<T,E>> {
   minus_vector(const E& a) : a_(a) {}
   const T operator[](const INDEX i) const {
      return -a_[i];
   }
   const T operator()(const INDEX i, const INDEX j) const {
      return -a_(i,j);
   }
   const T operator()(const INDEX i, const INDEX j, const INDEX k) const {
      return -a_(i,j,k);
   }
   INDEX size() const { return a_.size(); }
   INDEX dim1() const { return a_.dim1(); }
   INDEX dim2() const { return a_.dim2(); }
   INDEX dim3() const { return a_.dim3(); }
   private:
   const E& a_;
};

template<typename T, typename E>
struct minus_matrix : public matrix_expression<T,minus_matrix<T,E>> {
   minus_matrix(const E& a) : a_(a) {}
   const T operator[](const INDEX i) const {
      return -a_[i];
   }
   const T operator()(const INDEX i, const INDEX j) const {
      return -a_(i,j);
   }
   INDEX size() const { return a_.size(); }
   INDEX dim1() const { return a_.dim1(); }
   INDEX dim2() const { return a_.dim2(); }
   private:
   const E& a_;
};

template<typename T, typename E>
minus_vector<T,vector_expression<T,E>> 
operator-(vector_expression<T,E> const& v) {
   return minus_vector<T,vector_expression<T,E>>(v);
}

template<typename T, typename E>
minus_matrix<T,matrix_expression<T,E>> 
operator-(matrix_expression<T,E> const& v) {
   return minus_matrix<T,matrix_expression<T,E>>(v);
}


} // end namespace LP_MP

#endif // LP_MP_VECTOR_HXX
