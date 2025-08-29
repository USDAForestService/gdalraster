/* Misc. utility functions for internal use
   Chris Toney <chris.toney at usda.gov>
   Copyright (c) 2023-2025 gdalraster authors
*/

#include "rcpp_util.h"

// convert data frame to numeric matrix in Rcpp
Rcpp::NumericMatrix df_to_matrix_(const Rcpp::DataFrame &df) {
    Rcpp::NumericMatrix m = Rcpp::no_init(df.nrows(), df.size());
    for (R_xlen_t i = 0; i < df.size(); ++i) {
        if (Rcpp::is<Rcpp::NumericVector>(df[i]) ||
            Rcpp::is<Rcpp::IntegerVector>(df[i]) ||
            Rcpp::is<Rcpp::LogicalVector>(df[i])) {

            m.column(i) = Rcpp::NumericVector(df[i]);
        }
        else {
            Rcpp::stop("data frame columns must be numeric");
        }
    }
    return m;
}

// convert data frame to integer matrix in Rcpp
Rcpp::IntegerMatrix df_to_int_matrix_(const Rcpp::DataFrame &df) {
    Rcpp::IntegerMatrix m = Rcpp::no_init(df.nrows(), df.size());
    for (R_xlen_t i = 0; i < df.size(); ++i) {
        if (Rcpp::is<Rcpp::NumericVector>(df[i]) ||
            Rcpp::is<Rcpp::IntegerVector>(df[i]) ||
            Rcpp::is<Rcpp::LogicalVector>(df[i])) {

            m.column(i) = Rcpp::IntegerVector(df[i]);
        }
        else {
            Rcpp::stop("data frame columns must be numeric");
        }
    }
    return m;
}

// convert allowed xy inputs to numeric matrix
Rcpp::NumericMatrix xy_robject_to_matrix_(const Rcpp::RObject &xy) {
    if (xy.isNULL())
        Rcpp::stop("NULL was given for the input coordinates");

    Rcpp::NumericMatrix xy_ret;

    if (Rcpp::is<Rcpp::NumericVector>(xy) ||
        Rcpp::is<Rcpp::IntegerVector>(xy) ||
        Rcpp::is<Rcpp::LogicalVector>(xy)) {

        if (!Rf_isMatrix(xy)) {
            Rcpp::NumericVector v = Rcpp::as<Rcpp::NumericVector>(xy);
            if (v.size() < 2 || v.size() > 4)
                Rcpp::stop("input as vector must have one xy, xyz, or xyzm");

            xy_ret = Rcpp::NumericMatrix(1, v.size(), v.begin());
        }
        else {
            xy_ret = Rcpp::as<Rcpp::NumericMatrix>(xy);
        }
    }
    else if (Rcpp::is<Rcpp::DataFrame>(xy)) {
        xy_ret = df_to_matrix_(xy);
    }
    else {
        Rcpp::stop("coordinates must be in a vector, matrix or data frame");
    }

    return xy_ret;
}

// wrapper for base R path.expand()
Rcpp::CharacterVector path_expand_(const Rcpp::CharacterVector &path) {
    Rcpp::Function f("path.expand");
    return f(path);
}

// wrapper for base R normalizePath()
// int must_work should be NA_LOGICAL (the default), 0 or 1
Rcpp::CharacterVector normalize_path_(const Rcpp::CharacterVector &path,
                                      int must_work) {

    Rcpp::Function f("normalizePath");
    return f(path, Rcpp::Named("mustWork") = must_work);
}

// wrapper for base R enc2utf8()
Rcpp::CharacterVector enc_to_utf8_(const Rcpp::CharacterVector &x) {
    Rcpp::Function f("enc2utf8");
    return f(x);
}

// wrapper for base R strsplit()
Rcpp::CharacterVector strsplit_(const Rcpp::CharacterVector &x,
                                const Rcpp::CharacterVector &split) {
    Rcpp::Function f("strsplit");
    return f(x, split);
}

// wrapper for base R paste() with a value for collapse
// the single input x is expected to be a vector of values to collapse
Rcpp::String paste_collapse_(const SEXP &x, const Rcpp::String &s) {
    Rcpp::Function f("paste");
    Rcpp::CharacterVector tmp = f(x, Rcpp::Named("collapse") = s);
    Rcpp::CharacterVector ret = enc_to_utf8_(tmp);
    return ret[0];
}

// std::string to uppercase
std::string str_toupper_(const std::string &s) {
    std::string s_out = s;
    std::transform(s_out.begin(), s_out.end(), s_out.begin(),
                   [](unsigned char c){ return std::toupper(c); });
    return s_out;
}

// std::string to lowercase
std::string str_tolower_(const std::string &s) {
    std::string s_out = s;
    std::transform(s_out.begin(), s_out.end(), s_out.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return s_out;
}

// does character vector contain string element
bool contains_str_(const Rcpp::CharacterVector &v, const Rcpp::String &s) {
    auto it = std::find(v.cbegin(), v.cend(), s);
    if (it == v.cend())
        return false;
    else
        return true;
}

// wrapper for base R isNamespaceLoaded()
bool is_namespace_loaded_(const Rcpp::String &pkg) {
    Rcpp::Function f("isNamespaceLoaded");
    Rcpp::LogicalVector res = f(pkg);
    return Rcpp::is_true(Rcpp::all(res));
}
