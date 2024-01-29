/* Misc. utility functions for internal use
   Chris Toney <chris.toney at usda.gov> */

#ifndef rcpp_util_H
#define rcpp_util_H

#include <Rcpp.h> 

Rcpp::NumericMatrix _df_to_matrix(Rcpp::DataFrame df);
Rcpp::IntegerMatrix _df_to_int_matrix(Rcpp::DataFrame df);
Rcpp::CharacterVector _normalize_path(Rcpp::CharacterVector path,
		int must_work);
Rcpp::CharacterVector _normalize_path(Rcpp::CharacterVector path,
		int must_work = NA_LOGICAL);
Rcpp::CharacterVector _enc_to_utf8(Rcpp::CharacterVector x);

#endif
