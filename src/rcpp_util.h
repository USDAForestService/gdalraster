/* Misc. utility functions for internal use
   Chris Toney <chris.toney at usda.gov> */

#ifndef rcpp_util_H
#define rcpp_util_H

#include <Rcpp.h> 

Rcpp::NumericMatrix _df_to_matrix(Rcpp::DataFrame df);
Rcpp::IntegerMatrix _df_to_int_matrix(Rcpp::DataFrame df);

#endif
