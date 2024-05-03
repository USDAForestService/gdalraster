/* Misc. utility functions for internal use
   Chris Toney <chris.toney at usda.gov> */

#ifndef SRC_RCPP_UTIL_H_
#define SRC_RCPP_UTIL_H_

#include <Rcpp.h>

Rcpp::NumericMatrix _df_to_matrix(Rcpp::DataFrame df);
Rcpp::IntegerMatrix _df_to_int_matrix(Rcpp::DataFrame df);
Rcpp::CharacterVector _normalize_path(Rcpp::CharacterVector path,
        int must_work);
Rcpp::CharacterVector _normalize_path(Rcpp::CharacterVector path,
        int must_work = NA_LOGICAL);
Rcpp::CharacterVector _enc_to_utf8(Rcpp::CharacterVector x);
std::string str_toupper(std::string s);

#endif  // SRC_RCPP_UTIL_H_
