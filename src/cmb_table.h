/* Hash table class for counting unique combinations of integers
Chris Toney <chris.toney at usda.gov> */

#ifndef SRC_CMB_TABLE_H_
#define SRC_CMB_TABLE_H_

#include <Rcpp.h>

#include <string>
#include <vector>
#include <unordered_map>

struct cmbKey {
    Rcpp::IntegerVector cmb;

    bool operator==(const cmbKey &other) const {
        for (R_xlen_t i=0; i < cmb.size(); ++i) {
            if (cmb[i] != other.cmb[i])
                return false;
        }
        return true;
    }
};

struct cmbData {
    double ID = 0;
    double count = 0;
};

struct cmbHasher {
    std::size_t operator()(cmbKey const& key) const {
        // Boost hash_combine method
        // Copyright 2005-2014 Daniel James.
        // Copyright 2021, 2022 Peter Dimov.
        // Distributed under the Boost Software License, Version 1.0.
        // https://www.boost.org/LICENSE_1_0.txt
        std::size_t seed = 0;
        for (R_xlen_t i=0; i < key.cmb.size(); ++i) {
            seed ^= key.cmb[i] + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }
};

class CmbTable {
 public:
    CmbTable();
    explicit CmbTable(unsigned int keyLen);
    CmbTable(unsigned int keyLen, Rcpp::CharacterVector varNames);

    double update(const Rcpp::IntegerVector& int_cmb, double incr);
    Rcpp::NumericVector updateFromMatrix(const Rcpp::IntegerMatrix& int_cmbs,
            double incr);
    Rcpp::NumericVector updateFromMatrixByRow(
            const Rcpp::IntegerMatrix& int_cmbs, double incr);

    Rcpp::DataFrame asDataFrame() const;
    Rcpp::NumericMatrix asMatrix() const;

 private:
    unsigned int m_key_len;
    Rcpp::CharacterVector m_var_names;
    double m_last_ID;
    std::unordered_map<cmbKey, cmbData, cmbHasher> m_cmb_map;
};

RCPP_EXPOSED_CLASS(CmbTable)

#endif  // SRC_CMB_TABLE_H_
