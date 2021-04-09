/*! \file utils.hpp
 * \brief Defines some miscellaneous functions.
 */

#ifndef __UTILS_H__ 
#define __UTILS_H__

#include <string>
#include <map>
#include <set>
#include "linalg.hpp"

/*! \namespace utils
 *  \brief A few helpful functions that didn't seem to be readily
 *  classified anywhere else.   
 */ 
namespace utils{
  ///Clears whitespace from front and back of string s.
  std::string trim(const std::string& s);

  ///Does map m contain thing?
  template<typename K, typename V> 
  bool contains(const std::map<K,V>& m, K thing);
  ///Does set s contain thing?
  template<typename E>
  bool contains(const std::set<E>& s, E thing);

  ///Does set s1 include set s2?
  template<typename E>
  bool includes(const std::set<E>& s1, const std::set<E>& s2);

  ///Reads matrices from filenames.
  linalg::matrix read_matrix(std::string filename);
  ///Reads vectors from filenames.
  linalg::vector read_vector(std::string filename);

  /*! \brief Reads map<point,double> from a matrix.  
   *
   *Last column is the value at each point which is represented in
   *turn by the rest of the row.
   */
  std::map<linalg::point, double> read_pd_map(std::string filename);

  ///Outputs some information about generic exceptions.
  void show_exception(error_handling::error exc);

}

#endif
