/* Misc. utility functions for internal use
   Chris Toney <chris.toney at usda.gov> */

#ifndef SRC_RCPP_UTIL_H_
#define SRC_RCPP_UTIL_H_

#include <algorithm>
#include <cctype>
#include <string>

#include <Rcpp.h>
#include <RcppInt64>

Rcpp::NumericMatrix _df_to_matrix(const Rcpp::DataFrame& df);
Rcpp::IntegerMatrix _df_to_int_matrix(const Rcpp::DataFrame& df);
Rcpp::CharacterVector _path_expand(Rcpp::CharacterVector path);
Rcpp::CharacterVector _normalize_path(Rcpp::CharacterVector path,
                                      int must_work);
Rcpp::CharacterVector _normalize_path(Rcpp::CharacterVector path,
                                      int must_work = NA_LOGICAL);
Rcpp::CharacterVector _enc_to_utf8(Rcpp::CharacterVector x);
std::string _str_toupper(std::string s);

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
