/** @addtogroup math
 *  @{
 */

#include <Core/ModelicaDefine.h>
#include <Core/Modelica.h>
#include <Core/Math/ArrayOperations.h>
#include <Core/Math/ArraySlice.h>
#include <sstream>
#include <stdio.h>

using namespace std;

/**
Concatenates n real arrays along the k:th dimension.
*/
template <typename T>
void cat_array(int k, const vector<const BaseArray<T>*>& x, BaseArray<T>& a)
{
  unsigned int new_k_dim_size = 0;
  unsigned int n = x.size();
  /* check dim sizes of all inputs */
  if (n < 1)
    throw ModelicaSimulationError(MODEL_ARRAY_FUNCTION, "No input arrays");

  if (x[0]->getNumDims() < k)
    throw ModelicaSimulationError(MODEL_ARRAY_FUNCTION, "Wrong dimension for input array");

  new_k_dim_size = x[0]->getDim(k);
  for (int i = 1; i < n; i++)
  {
    //arrays must have same number of dimensions
		if (x[0]->getNumDims() != x[i]->getNumDims())
      throw ModelicaSimulationError(MODEL_ARRAY_FUNCTION, "Wrong dimension for input array");
    //Size matching: Arrays must have identical array sizes with the exception of the size of dimension k
		for (int j = 1; j < k; j++)
      if (x[0]->getDim(j) != x[i]->getDim(j))
        throw ModelicaSimulationError(MODEL_ARRAY_FUNCTION, "Wrong size for input array");
		//calculate new size of dimension k
    new_k_dim_size += x[i]->getDim(k);
    //Size matching: Arrays must have identical array sizes with the exception of the size of dimension k
		for (int j = k + 1; j <= x[0]->getNumDims(); j++)
      if (x[0]->getDim(j) != x[i]->getDim(j))
        throw ModelicaSimulationError(MODEL_ARRAY_FUNCTION, "Wrong size for input array");
  }
  /* calculate size of sub and super structure in 1-dim data representation */
  unsigned int n_sub = 1;
  unsigned int n_super = 1;
  for (int i = 1; i < k; i++)
    n_super *= x[0]->getDim(i);
  for (int i = k + 1; i <= x[0]->getNumDims(); i++)
    n_sub *= x[0]->getDim(i);
  /* allocate output array */
  vector<size_t> ex = x[0]->getDims();
  ex[k-1] = new_k_dim_size;
  if (ex.size() < k)
    throw ModelicaSimulationError(MODEL_ARRAY_FUNCTION, "Error resizing concatenate array");
  a.setDims(ex);

  /* concatenation along k-th dimension */
  T* a_data = a.getData();
  int j = 0;
  for (int i = 0; i < n_sub; i++)
  {
    for (int c = 0; c < n; c++)
    {
      int n_super_k = n_super * x[c]->getDim(k);
      const T* x_data = x[c]->getData() + i * n_super_k;
      std::copy(x_data, x_data + n_super_k, a_data + j);
      j += n_super_k;
    }
  }
}

void identity_alloc(size_t n, DynArrayDim2<int>& I)
{
  I.setDims(n, n);
  fill_array(I, 0);
  for (size_t i = 1; i <= n; i++)
    I(i, i) = 1;
}

template <typename T>
void diagonal_alloc(const BaseArray<T>& v, BaseArray<T>& D)
{
  if (v.getNumDims() != 1)
    throw ModelicaSimulationError(MODEL_ARRAY_FUNCTION, "Error in diagonal, input must be vector");
  if (D.getNumDims() != 2)
    throw ModelicaSimulationError(MODEL_ARRAY_FUNCTION, "Error in diagonal, output must be matrix");
  vector<size_t> dims = v.getDims();
  size_t n = dims[0];
  dims.push_back(n);
  D.setDims(dims);
  const T* v_data = v.getData();
  T* D_data = D.getData();
  std::fill(D_data, D_data + n * n, 0);
  for (size_t i = 0; i < n; i++)
    D_data[i * n + i] = v_data[i];
}

/**
 * appends dimensions of size 1 to the right of s up to dimension n
 */
template <typename T>
void promote_array(size_t n, const BaseArray<T>& s, BaseArray<T>& d)
{
  vector<size_t> ex = s.getDims();
  for (size_t i = ex.size(); i < n; i++)
    ex.push_back(1);
  d.setDims(ex);
  d.assign(s.getData());
}

/**
 * permutes the first two dimensions of x into a
 */
template <typename T>
void transpose_array(const BaseArray<T>& x, BaseArray<T>& a)

{
  size_t ndims = x.getNumDims();
  if(ndims < 2 || ndims != a.getNumDims())
    throw ModelicaSimulationError(MODEL_ARRAY_FUNCTION,
                                  "Wrong dimensions in transpose_array");
  vector<size_t> ex = x.getDims();
  std::swap(ex[0], ex[1]);
  a.setDims(ex);
  vector<Slice> sx(ndims);
  vector<Slice> sa(ndims);
  for (int i = 1; i <= x.getDim(1); i++) {
    sa[1] = sx[0] = Slice(i);
    ArraySlice<T>(a, sa).assign(ArraySliceConst<T>(x, sx));
  }
}

template <typename T>
void multiply_array(const BaseArray<T>& inputArray, const T &b, BaseArray<T>& outputArray)
{
  size_t dim = inputArray.getNumElems();
  if(dim > 0)
  {
	outputArray.setDims(inputArray.getDims());
	const T* data = inputArray.getData();
	T* aim = outputArray.getData();
	std::transform (data, data + inputArray.getNumElems(),
                  aim, std::bind2nd(std::multiplies<T>(), b));
  }
};

template <typename T>
void multiply_array(const BaseArray<T> &leftArray, const BaseArray<T> &rightArray, BaseArray<T> &resultArray)
{
  size_t leftNumDims = leftArray.getNumDims();
  size_t rightNumDims = rightArray.getNumDims();
  size_t matchDim = rightArray.getDim(1);
  if (leftArray.getDim(leftNumDims) != matchDim)
    throw ModelicaSimulationError(MODEL_ARRAY_FUNCTION,
                                  "Wrong sizes in multiply_array");
  if (leftNumDims == 1 && rightNumDims == 2) {
    size_t rightDim = rightArray.getDim(2);
    vector<size_t> dims;
    dims.push_back(rightDim);
    resultArray.setDims(dims);
    for (size_t j = 1; j <= rightDim; j++) {
      T val = T();
      for (size_t k = 1; k <= matchDim; k++)
        val += leftArray(k) * rightArray(k, j);
      resultArray(j) = val;
    }
  }
  else if (leftNumDims == 2 && rightNumDims == 1) {
    size_t leftDim = leftArray.getDim(1);
    vector<size_t> dims;
    dims.push_back(leftDim);
    resultArray.setDims(dims);
    for (size_t i = 1; i <= leftDim; i++) {
      T val = T();
      for (size_t k = 1; k <= matchDim; k++)
        val += leftArray(i, k) * rightArray(k);
      resultArray(i) = val;
    }
  }
  else if (leftNumDims == 2 && rightNumDims == 2) {
    size_t leftDim = leftArray.getDim(1);
    size_t rightDim = rightArray.getDim(2);
    vector<size_t> dims;
    dims.push_back(leftDim);
    dims.push_back(rightDim);
    resultArray.setDims(dims);
    for (size_t i = 1; i <= leftDim; i++) {
      for (size_t j = 1; j <= rightDim; j++) {
        T val = T();
        for (size_t k = 1; k <= matchDim; k++)
          val += leftArray(i, k) * rightArray(k, j);
        resultArray(i, j) = val;
      }
    }
  }
  else
    throw ModelicaSimulationError(MODEL_ARRAY_FUNCTION,
                                  "Unsupported dimensions in multiply_array");
}

template <typename T>
void multiply_array_elem_wise(const BaseArray<T> &leftArray, const BaseArray<T> &rightArray, BaseArray<T> &resultArray)
{
  size_t dimLeft = leftArray.getNumElems();
  size_t dimRight = rightArray.getNumElems();

  if(dimLeft != dimRight)
      throw ModelicaSimulationError(MODEL_ARRAY_FUNCTION,
                                      "Right and left array must have the same size for element wise multiplication");

  resultArray.setDims(leftArray.getDims());
  const T* leftData = leftArray.getData();
  const T* rightData = rightArray.getData();
  T* aim = resultArray.getData();

  std::transform (leftData, leftData + leftArray.getNumElems(), rightData, aim, std::multiplies<T>());
}

template <typename T>
void divide_array(const BaseArray<T>& inputArray, const T &b, BaseArray<T>& outputArray)
{
  size_t nelems = inputArray.getNumElems();
  if (outputArray.getNumElems() != nelems)
  {
    outputArray.setDims(inputArray.getDims());
  }
  const T* data = inputArray.getData();
  T* aim = outputArray.getData();
  std::transform(data, data + nelems, aim, std::bind2nd(std::divides<T>(), b));
}

template <typename T>
void divide_array(const T &b, const BaseArray<T>& inputArray, BaseArray<T>& outputArray)
{
  size_t nelems = inputArray.getNumElems();
  if (outputArray.getNumElems() != nelems)
  {
    outputArray.setDims(inputArray.getDims());
  }
  const T* data = inputArray.getData();
  T* aim = outputArray.getData();
  std::transform(data, data + nelems, aim, std::bind1st(std::divides<T>(), b));
}

template <typename T>
void divide_array_elem_wise(const BaseArray<T> &leftArray, const BaseArray<T> &rightArray, BaseArray<T> &resultArray)
{
  size_t dimLeft = leftArray.getNumElems();
  size_t dimRight = rightArray.getNumElems();

  if(dimLeft != dimRight)
    throw ModelicaSimulationError(MODEL_ARRAY_FUNCTION,
      "Right and left array must have the same size for element wise division");

  resultArray.setDims(leftArray.getDims());
  const T* leftData = leftArray.getData();
  const T* rightData = rightArray.getData();
  T* result = resultArray.getData();

  std::transform (leftData, leftData + leftArray.getNumElems(), rightData, result, std::divides<T>());
}

template <typename T>
void fill_array(BaseArray<T>& inputArray, T b)
{
  T* data = inputArray.getData();
  std::fill(data, data + inputArray.getNumElems(), b);
}

template <typename T>
void pow_array_scalar(const BaseArray<double> &inputArray, T exponent,
                      BaseArray<double> &outputArray)
{
  size_t nelems = inputArray.getNumElems();
  if (outputArray.getNumElems() != nelems)
    outputArray.setDims(inputArray.getDims());
  const double *data = inputArray.getData();
  double *dest = outputArray.getData();
  double *end = dest + nelems;
  while (dest != end)
    *dest++ = pow(*data++, exponent);
}

template <typename T>
void subtract_array(const BaseArray<T>& leftArray, const BaseArray<T>& rightArray, BaseArray<T>& resultArray)
{
  size_t dimLeft = leftArray.getNumElems();
  size_t dimRight = rightArray.getNumElems();

  if(dimLeft != dimRight)
    throw ModelicaSimulationError(MODEL_ARRAY_FUNCTION,
                                      "Right and left array must have the same size for element wise substraction");

  resultArray.setDims(leftArray.getDims());
  const T* data1 = leftArray.getData();
  const T* data2 = rightArray.getData();
  T* aim = resultArray.getData();

  std::transform (data1, data1 + dimLeft, data2, aim, std::minus<T>());
}

template <typename T>
void subtract_array_scalar(const BaseArray<T>& inputArray, T b, BaseArray<T>& outputArray)
{
  size_t dim = inputArray.getNumElems();
  if(dim > 0)
  {
    outputArray.setDims(inputArray.getDims());
    const T* data = inputArray.getData();
    T* aim = outputArray.getData();
    std::transform (data, data + inputArray.getNumElems(),
                  aim, std::bind2nd(std::minus<T>(), b));
  }
}

template <typename T>
void add_array(const BaseArray<T>& leftArray, const BaseArray<T>& rightArray, BaseArray<T>& resultArray)
{
  size_t dimLeft = leftArray.getNumElems();
  size_t dimRight = rightArray.getNumElems();

  if(dimLeft != dimRight)
    throw ModelicaSimulationError(MODEL_ARRAY_FUNCTION,
                                      "Right and left array must have the same size for element wise addition");

  resultArray.setDims(leftArray.getDims());
  const T* data1 = leftArray.getData();
  const T* data2 = rightArray.getData();
  T* aim = resultArray.getData();

  std::transform(data1, data1 + leftArray.getNumElems(), data2, aim, std::plus<T>());
}

template <typename T>
void add_array_scalar(const BaseArray<T>& inputArray, T b, BaseArray<T>& outputArray)
{
  size_t dim = inputArray.getNumElems();
  if (dim > 0) {
    outputArray.setDims(inputArray.getDims());
    const T* data = inputArray.getData();
    T* result = outputArray.getData();
    std::transform (data, data + inputArray.getNumElems(),
                    result, std::bind2nd(std::plus<T>(), b));
  }
}

template <typename T>
void usub_array(const BaseArray<T>& a, BaseArray<T>& b)
{
  size_t nelems =  a.getNumElems();
  if (nelems > 0) {
    b.setDims(a.getDims());
    const T* data = a.getData();
    T* result = b.getData();
    for (size_t i = 0; i < nelems; i++)
      result[i] = -data[i];
  }
}

template <typename T>
T sum_array (const BaseArray<T>& x)
{
  const T* data = x.getData();
  T val = std::accumulate(data, data + x.getNumElems(), T());
  return val;
}

template <typename T>
T product_array(const BaseArray<T>& x)
{
  const T* data = x.getData();
  T val = std::accumulate(data, data + x.getNumElems(), T(1), std::multiplies<T>());
  return val;
}

/**
scalar product of two arrays (a,b type as template parameter)
*/
template <typename T>
T dot_array(const BaseArray<T>& a, const BaseArray<T>& b)
{
  if (a.getNumDims() != 1  || b.getNumDims() != 1)
    throw ModelicaSimulationError(MODEL_ARRAY_FUNCTION, "error in dot array function. Wrong dimension");
  const T* data1 = a.getData();
  const T* data2 = b.getData();
  T r = std::inner_product(data1, data1 + a.getNumElems(), data2, 0.0);
  return r;
}

/**
cross product of two arrays (a,b type as template parameter)
*/
template <typename T>
void cross_array(const BaseArray<T>& a, const BaseArray<T>& b, BaseArray<T>& res)
{
  res(1) = (a(2) * b(3)) - (a(3) * b(2));
  res(2) = (a(3) * b(1)) - (a(1) * b(3));
  res(3) = (a(1) * b(2)) - (a(2) * b(1));
};

/**
finds min/max elements of an array
*/
template <typename T>
std::pair<T,T> min_max(const BaseArray<T>& x)
{
  if (x.getNumElems() < 1)
    throw ModelicaSimulationError(MODEL_ARRAY_FUNCTION, "min/max requires at least one element");
  const T* data = x.getData();
  std::pair<const T*, const T*>
  ret = minmax_element(data, data + x.getNumElems());
  return std::make_pair(*(ret.first), *(ret.second));
}

template <typename S, typename T>
void cast_array(const BaseArray<S>& a, BaseArray<T>& b)
{
  b.setDims(a.getDims());
  int numElems = a.getNumElems();
  const S* src_data = a.getData();
  T* dst_data = b.getData();
  for (int i = 0; i < numElems; i++)
    *dst_data++ = (T)(*src_data++);
}

/**
 * helper for assignRowMajorData
 * recursive function for muli-dimensional assignment of raw data
 */
template <typename T>
static size_t assignRowMajorDim(size_t dim, const T* data,
                                BaseArray<T> &array, vector<size_t> &idx) {
  size_t processed = 0;
  size_t size = array.getDim(dim);
  for (size_t i = 1; i <= size; i++) {
    idx[dim - 1] = i;
    if (dim < idx.size())
      processed += assignRowMajorDim(dim + 1, data + processed, array, idx);
    else
      array(idx) = data[processed++];
  }
  return processed;
}

template <typename T>
void assignRowMajorData(const T *data, BaseArray<T> &array) {
  vector<size_t> idx(array.getNumDims());
  assignRowMajorDim(1, data, array, idx);
}

/**
 * helper for convertArrayLayout
 * recursive function for changing between row and column major
 */
template <typename S, typename T>
static void convertArrayDim(size_t dim,
                            const BaseArray<S> &s, vector<size_t> &sidx,
                            BaseArray<T> &d, vector<size_t> &didx) {
  size_t ndims = s.getNumDims();
  size_t size = s.getDim(dim);
  for (size_t i = 1; i <= size; i++) {
    didx[ndims - dim] = sidx[dim - 1] = i;
    if (dim < sidx.size())
      convertArrayDim(dim + 1, s, sidx, d, didx);
    else
      d(didx) = s(sidx);
  }
}


/**
 * permutes dims between row and column major storage layout,
 * including optional type conversion if supported in assignment from S to T
 */
template <typename S, typename T>
void convertArrayLayout(const BaseArray<S> &s, BaseArray<T> &d) {
  size_t ndims = s.getNumDims();
  if (ndims != d.getNumDims())
    throw ModelicaSimulationError(MODEL_ARRAY_FUNCTION,
                                  "Wrong dimensions in convertArrayLayout");
  vector<size_t> sdims = s.getDims();
  vector<size_t> ddims(ndims);
  for (size_t dim = 1; dim <= ndims; dim++)
    ddims[ndims - dim] = sdims[dim - 1];
  d.resize(ddims);
  convertArrayDim(1, s, sdims, d, ddims);
}

/*
Explicit template instantiation for double, int, bool
*/
template  void BOOST_EXTENSION_EXPORT_DECL
cat_array<double>(int k, const vector<const BaseArray<double>*>& x, BaseArray<double>& a);
template  void BOOST_EXTENSION_EXPORT_DECL
cat_array<int>(int k, const vector<const BaseArray<int>*>& x, BaseArray<int>& a);
template  void BOOST_EXTENSION_EXPORT_DECL
cat_array<bool>(int k, const vector<const BaseArray<bool>*>& x, BaseArray<bool>& a);

template void BOOST_EXTENSION_EXPORT_DECL
transpose_array(const BaseArray<double>& x, BaseArray<double>& a);
template void BOOST_EXTENSION_EXPORT_DECL
transpose_array(const BaseArray<int>& x, BaseArray<int>& a);
template void BOOST_EXTENSION_EXPORT_DECL
transpose_array(const BaseArray<bool>& x, BaseArray<bool>& a);

template void BOOST_EXTENSION_EXPORT_DECL
diagonal_alloc(const BaseArray<double>& v, BaseArray<double>& D);
template void BOOST_EXTENSION_EXPORT_DECL
diagonal_alloc(const BaseArray<int>& v, BaseArray<int>& D);

template void BOOST_EXTENSION_EXPORT_DECL
promote_array(size_t n, const BaseArray<double>& s, BaseArray<double>& d);
template void BOOST_EXTENSION_EXPORT_DECL
promote_array(size_t n, const BaseArray<int>& s, BaseArray<int>& d);
template void BOOST_EXTENSION_EXPORT_DECL
promote_array(size_t n, const BaseArray<bool>& s, BaseArray<bool>& d);

template void BOOST_EXTENSION_EXPORT_DECL
multiply_array(const BaseArray<double>& inputArray, const double &b, BaseArray<double>& outputArray);
template void BOOST_EXTENSION_EXPORT_DECL
multiply_array(const BaseArray<int>& inputArray, const int &b, BaseArray<int>& outputArray);
template void BOOST_EXTENSION_EXPORT_DECL
multiply_array(const BaseArray<bool>& inputArray, const bool &b, BaseArray<bool>& outputArray);

template void BOOST_EXTENSION_EXPORT_DECL
multiply_array(const BaseArray<double> &leftArray, const BaseArray<double> &rightArray, BaseArray<double> &resultArray);
template void BOOST_EXTENSION_EXPORT_DECL
multiply_array(const BaseArray<int> &leftArray, const BaseArray<int> &rightArray, BaseArray<int> &resultArray);
template void BOOST_EXTENSION_EXPORT_DECL
multiply_array(const BaseArray<bool> &leftArray, const BaseArray<bool> &rightArray, BaseArray<bool> &resultArray);

template void BOOST_EXTENSION_EXPORT_DECL
multiply_array_elem_wise(const BaseArray<double> &leftArray, const BaseArray<double> &rightArray, BaseArray<double> &resultArray);
template void BOOST_EXTENSION_EXPORT_DECL
multiply_array_elem_wise(const BaseArray<int> &leftArray, const BaseArray<int> &rightArray, BaseArray<int> &resultArray);
template void BOOST_EXTENSION_EXPORT_DECL
multiply_array_elem_wise(const BaseArray<bool> &leftArray, const BaseArray<bool> &rightArray, BaseArray<bool> &resultArray);

template void BOOST_EXTENSION_EXPORT_DECL
divide_array(const BaseArray<double>& inputArray, const double &b, BaseArray<double>& outputArray);
template void BOOST_EXTENSION_EXPORT_DECL
divide_array(const BaseArray<int>& inputArray, const int &b, BaseArray<int>& outputArray);
template void BOOST_EXTENSION_EXPORT_DECL
divide_array(const BaseArray<bool>& inputArray, const bool &b, BaseArray<bool>& outputArray);

template void BOOST_EXTENSION_EXPORT_DECL
divide_array(const double &b, const BaseArray<double>& inputArray, BaseArray<double>& outputArray);
template void BOOST_EXTENSION_EXPORT_DECL
divide_array(const int &b, const BaseArray<int>& inputArray, BaseArray<int>& outputArray);

template void BOOST_EXTENSION_EXPORT_DECL
divide_array_elem_wise(const BaseArray<double> &leftArray, const BaseArray<double> &rightArray, BaseArray<double> &resultArray);
template void BOOST_EXTENSION_EXPORT_DECL
divide_array_elem_wise(const BaseArray<int> &leftArray, const BaseArray<int> &rightArray, BaseArray<int> &resultArray);
template void BOOST_EXTENSION_EXPORT_DECL
divide_array_elem_wise(const BaseArray<bool> &leftArray, const BaseArray<bool> &rightArray, BaseArray<bool> &resultArray);

template void BOOST_EXTENSION_EXPORT_DECL
fill_array(BaseArray<double>& inputArray, double b);
template void BOOST_EXTENSION_EXPORT_DECL
fill_array(BaseArray<int>& inputArray, int b);
template void BOOST_EXTENSION_EXPORT_DECL
fill_array(BaseArray<bool>& inputArray, bool b);

template void BOOST_EXTENSION_EXPORT_DECL
pow_array_scalar(const BaseArray<double>& inputArray, double exponent, BaseArray<double>& outputArray);
template void BOOST_EXTENSION_EXPORT_DECL
pow_array_scalar(const BaseArray<double>& inputArray, int exponent, BaseArray<double>& outputArray);

template void BOOST_EXTENSION_EXPORT_DECL
subtract_array(const BaseArray<double>& leftArray, const BaseArray<double>& rightArray, BaseArray<double>& resultArray);
template void BOOST_EXTENSION_EXPORT_DECL
subtract_array(const BaseArray<int>& leftArray, const BaseArray<int>& rightArray, BaseArray<int>& resultArray);
template void BOOST_EXTENSION_EXPORT_DECL
subtract_array(const BaseArray<bool>& leftArray, const BaseArray<bool>& rightArray, BaseArray<bool>& resultArray);

template void BOOST_EXTENSION_EXPORT_DECL
subtract_array_scalar(const BaseArray<double>& inputArray, double b, BaseArray<double>& outputArray);
template void BOOST_EXTENSION_EXPORT_DECL
subtract_array_scalar(const BaseArray<int>& inputArray, int b, BaseArray<int>& outputArray);
template void BOOST_EXTENSION_EXPORT_DECL
subtract_array_scalar(const BaseArray<bool>& inputArray, bool b, BaseArray<bool>& outputArray);

template void BOOST_EXTENSION_EXPORT_DECL
add_array(const BaseArray<double>& leftArray, const BaseArray<double>& rightArray, BaseArray<double>& resultArray);
template void BOOST_EXTENSION_EXPORT_DECL
add_array(const BaseArray<int>& leftArray, const BaseArray<int>& rightArray, BaseArray<int>& resultArray);
template void BOOST_EXTENSION_EXPORT_DECL
add_array(const BaseArray<bool>& leftArray, const BaseArray<bool>& rightArray, BaseArray<bool>& resultArray);

template void BOOST_EXTENSION_EXPORT_DECL
add_array_scalar(const BaseArray<double>& inputArray, double b, BaseArray<double>& outputArray);
template void BOOST_EXTENSION_EXPORT_DECL
add_array_scalar(const BaseArray<int>& inputArray, int b, BaseArray<int>& outputArray);
template void BOOST_EXTENSION_EXPORT_DECL
add_array_scalar(const BaseArray<bool>& inputArray, bool b, BaseArray<bool>& outputArray);

template void BOOST_EXTENSION_EXPORT_DECL
usub_array(const BaseArray<double>& a, BaseArray<double>& b);
template void BOOST_EXTENSION_EXPORT_DECL
usub_array(const BaseArray<int>& a, BaseArray<int>& b);
template void BOOST_EXTENSION_EXPORT_DECL
usub_array(const BaseArray<bool>& a, BaseArray<bool>& b);

template double BOOST_EXTENSION_EXPORT_DECL
sum_array(const BaseArray<double>& x);
template int BOOST_EXTENSION_EXPORT_DECL
sum_array(const BaseArray<int>& x);
template bool BOOST_EXTENSION_EXPORT_DECL
sum_array(const BaseArray<bool>& x);

template double BOOST_EXTENSION_EXPORT_DECL
product_array(const BaseArray<double>& x);
template int BOOST_EXTENSION_EXPORT_DECL
product_array(const BaseArray<int>& x);

template void BOOST_EXTENSION_EXPORT_DECL
cross_array(const BaseArray<double>& a, const BaseArray<double>& b, BaseArray<double>& res);
template void BOOST_EXTENSION_EXPORT_DECL
cross_array(const BaseArray<int>& a, const BaseArray<int>& b, BaseArray<int>& res);
template void BOOST_EXTENSION_EXPORT_DECL
cross_array(const BaseArray<bool>& a, const BaseArray<bool>& b, BaseArray<bool>& res);

template double BOOST_EXTENSION_EXPORT_DECL
dot_array(const BaseArray<double>&a, const BaseArray<double>& b);
template int BOOST_EXTENSION_EXPORT_DECL
dot_array(const BaseArray<int>&a, const BaseArray<int>& b);
template bool BOOST_EXTENSION_EXPORT_DECL
dot_array(const BaseArray<bool>&a, const BaseArray<bool>& b);

template std::pair<double,double> BOOST_EXTENSION_EXPORT_DECL
min_max(const BaseArray<double>& x);
template std::pair<int,int> BOOST_EXTENSION_EXPORT_DECL
min_max(const BaseArray<int>& x);
template std::pair<bool,bool> BOOST_EXTENSION_EXPORT_DECL
min_max(const BaseArray<bool>& x);

template void BOOST_EXTENSION_EXPORT_DECL
cast_array(const BaseArray<int> &a, BaseArray<double> &b);
template void BOOST_EXTENSION_EXPORT_DECL
cast_array(const BaseArray<int> &a, BaseArray<bool> &b);
template void BOOST_EXTENSION_EXPORT_DECL
cast_array(const BaseArray<bool> &a, BaseArray<int> &b);

template void BOOST_EXTENSION_EXPORT_DECL
convertArrayLayout(const BaseArray<double> &s, BaseArray<double> &d);
template void BOOST_EXTENSION_EXPORT_DECL
convertArrayLayout(const BaseArray<int> &s, BaseArray<int> &d);
template void BOOST_EXTENSION_EXPORT_DECL
convertArrayLayout(const BaseArray<bool> &s, BaseArray<int> &d);
template void BOOST_EXTENSION_EXPORT_DECL
convertArrayLayout(const BaseArray<int> &s, BaseArray<bool> &d);
template void BOOST_EXTENSION_EXPORT_DECL
convertArrayLayout(const BaseArray<string> &s, BaseArray<string> &d);


template void BOOST_EXTENSION_EXPORT_DECL
assignRowMajorData(const double *data, BaseArray<double> &d);
template void BOOST_EXTENSION_EXPORT_DECL
assignRowMajorData(const int *data, BaseArray<int> &d);
template void BOOST_EXTENSION_EXPORT_DECL
assignRowMajorData(const bool *data, BaseArray<bool> &d);
template void BOOST_EXTENSION_EXPORT_DECL
assignRowMajorData(const string *data, BaseArray<string> &d);
/** @} */ // end of math
