/* Misc. utility functions for internal use
   Chris Toney <chris.toney at usda.gov> */

#include "rcpp_util.h"

//' convert data frame to numeric matrix in Rcpp
//' @noRd
Rcpp::NumericMatrix _df_to_matrix(Rcpp::DataFrame df) {
	Rcpp::NumericMatrix m(df.nrows(), df.size());
	for (R_xlen_t i=0; i<df.size(); ++i) {
		m.column(i) = Rcpp::NumericVector(df[i]);
	}
	return m;
}

//' convert data frame to integer matrix in Rcpp
//' @noRd
Rcpp::IntegerMatrix _df_to_int_matrix(Rcpp::DataFrame df) {
	Rcpp::IntegerMatrix m(df.nrows(), df.size());
	for (R_xlen_t i=0; i<df.size(); ++i) {
		m.column(i) = Rcpp::IntegerVector(df[i]);
	}
	return m;
}

//' wrapper for base R normalizePath()
//' int must_work should be NA_LOGICAL (the default), 0 or 1
//' @noRd
Rcpp::CharacterVector _normalize_path(Rcpp::CharacterVector path,
		int must_work) {

	Rcpp::Function f("normalizePath");
	return f(path, Rcpp::Named("mustWork") = must_work);
}

//' wrapper for base R enc2utf8()
//' @noRd
Rcpp::CharacterVector _enc_to_utf8(Rcpp::CharacterVector x) {
	Rcpp::Function f("enc2utf8");
	return f(x);
}

