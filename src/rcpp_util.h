/* Misc. utility functions for internal use
   Chris Toney <chris.toney at usda.gov>
   Copyright (c) 2023-2025 gdalraster authors
*/

#ifndef SRC_RCPP_UTIL_H_
#define SRC_RCPP_UTIL_H_

#include <limits.h>

#include <Rcpp.h>
#include <RcppInt64>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>

constexpr int64_t MAX_INT_AS_R_NUMERIC = 9007199254740991;

// as defined in the bit64 package src/integer64.h
#define NA_INTEGER64 LLONG_MIN
#define ISNA_INTEGER64(X)((X) == NA_INTEGER64)
#define MIN_INTEGER64 LLONG_MIN+1
#define MAX_INTEGER64 LLONG_MAX

Rcpp::NumericMatrix df_to_matrix_(const Rcpp::DataFrame &df);
Rcpp::IntegerMatrix df_to_int_matrix_(const Rcpp::DataFrame &df);

Rcpp::NumericMatrix xy_robject_to_matrix_(const Rcpp::RObject &xy);

Rcpp::CharacterVector path_expand_(const Rcpp::CharacterVector &path);

Rcpp::CharacterVector normalize_path_(const Rcpp::CharacterVector &path,
                                      int must_work);

Rcpp::CharacterVector normalize_path_(const Rcpp::CharacterVector &path,
                                      int must_work = NA_LOGICAL);

Rcpp::CharacterVector enc_to_utf8_(const Rcpp::CharacterVector &x);

Rcpp::CharacterVector strsplit_(const Rcpp::CharacterVector &x,
                                const Rcpp::CharacterVector &split);

Rcpp::String paste_collapse_(const SEXP &x, const Rcpp::String &s);

std::string str_toupper_(const std::string &s);
std::string str_tolower_(const std::string &s);

bool contains_str_(const Rcpp::CharacterVector &v, const Rcpp::String &s);

bool is_namespace_loaded_(const Rcpp::String &pkg);

// case-insensitive comparator for std::map
// https://stackoverflow.com/questions/1801892/how-can-i-make-the-mapfind-operation-case-insensitive
struct _ci_less {
    struct nocase_compare {
        bool operator() (const unsigned char& c1,
                         const unsigned char& c2) const {
            return std::tolower(c1) < std::tolower(c2);
        }
    };
    bool operator() (const std::string & s1, const std::string & s2) const {
        return std::lexicographical_compare(s1.begin(), s1.end(),
                                            s2.begin(), s2.end(),
                                            nocase_compare());
    }
};

#endif  // SRC_RCPP_UTIL_H_
