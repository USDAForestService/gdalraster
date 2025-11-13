/* Implementation of class CmbTable
   Chris Toney <chris.toney at usda.gov>
*/

#include "cmb_table.h"

#include <Rcpp.h>

#include <string>
#include <vector>

#include "rcpp_util.h"

CmbTable::CmbTable()
        : CmbTable(1, Rcpp::CharacterVector::create()) {}

CmbTable::CmbTable(int keyLen)
        : CmbTable(keyLen, Rcpp::CharacterVector::create()) {}

CmbTable::CmbTable(int keyLen, const Rcpp::CharacterVector &varNames)
        : m_key_len(keyLen),
          m_var_names(Rcpp::as<std::vector<std::string>>(varNames))  {

    if (keyLen <= 0)
        Rcpp::stop("'keyLen' must be a positive integer");

    m_key_len = static_cast<R_xlen_t>(keyLen);

    if (varNames.size() == 0) {
        for (R_xlen_t i = 0; i < m_key_len; ++i) {
            Rcpp::String s = "V" + std::to_string(i + 1);
            m_var_names.push_back(s);
        }
    }

    if (m_key_len != static_cast<R_xlen_t>(m_var_names.size()))
        Rcpp::stop("'keyLen' must equal 'length(varNames)'");
}

double CmbTable::update(const Rcpp::IntegerVector &int_cmb, double incr) {
    // Increment count for existing int_cmb
    // or insert new int_cmb with count = incr.
    cmbKey key(int_cmb);
    cmbData &cmbdat = m_cmb_map[key];
    cmbdat.count += incr;
    if (cmbdat.ID == 0.0) {
        m_last_ID += 1.0;
        cmbdat.ID = m_last_ID;
    }
    return cmbdat.ID;
}

Rcpp::NumericVector CmbTable::updateFromMatrix(
    const Rcpp::IntegerMatrix &int_cmbs, double incr) {

    // int combinations (int_cmbs) are columns of a matrix (nrow = m_key_len).
    // Increment count for existing int_cmb,
    // else insert new int_cmb with count = incr.
    // Return a vector of cmb IDs for the columns of the input matrix.
    if (int_cmbs.nrow() != m_key_len) {
        Rcpp::stop("number of matrix rows must equal the key length: " +
                   std::to_string(m_key_len));
    }

    const R_xlen_t ncol = int_cmbs.ncol();
    Rcpp::NumericVector out(ncol);

    for (R_xlen_t k = 0; k < ncol; ++k) {
        out[k] = update(int_cmbs.column(k), incr);
    }
    return out;
}

Rcpp::NumericVector CmbTable::updateFromMatrixByRow(
    const Rcpp::IntegerMatrix &int_cmbs, double incr) {

    // int combinations (int_cmbs) are rows of a matrix (ncol = m_key_len).
    // Same as updateFromMatrix() except by row instead of by column
    // (i.e., variables here are in the columns).
    // Return a vector of cmb IDs for the rows of the input matrix.
    if (int_cmbs.ncol() != m_key_len) {
        Rcpp::stop("number of matrix columns must equal the key length: " +
                   std::to_string(m_key_len));
    }

    const R_xlen_t nrow = int_cmbs.nrow();
    Rcpp::NumericVector out(nrow);

    for (R_xlen_t k = 0; k < nrow; ++k) {
        out[k] = update(int_cmbs.row(k), incr);
    }
    return out;
}

Rcpp::DataFrame CmbTable::asDataFrame() const {
    Rcpp::NumericVector dvCmbID(m_cmb_map.size());
    Rcpp::NumericVector dvCmbCount(m_cmb_map.size());
    std::vector<Rcpp::IntegerVector> aVec(m_key_len);
    cmbKey key;
    cmbData cmbdat;

    for (R_xlen_t i = 0; i < m_key_len; ++i) {
        aVec[i] = Rcpp::IntegerVector(m_cmb_map.size());
    }
    std::size_t this_idx = 0;
    for (auto iter = m_cmb_map.begin(); iter != m_cmb_map.end(); ++iter) {
        key = iter->first;
        cmbdat = iter->second;
        dvCmbID[this_idx] = cmbdat.ID;
        dvCmbCount[this_idx] = cmbdat.count;
        for (R_xlen_t var = 0; var < m_key_len; ++var) {
            aVec[var][this_idx] = key.cmb[var];
        }
        ++this_idx;
    }

    Rcpp::DataFrame dfOut = Rcpp::DataFrame::create();
    dfOut.push_back(dvCmbID, "cmbid");
    dfOut.push_back(dvCmbCount, "count");
    for (R_xlen_t i = 0; i < m_key_len; ++i) {
        dfOut.push_back(aVec[i], m_var_names[i]);
    }
    return dfOut;
}

Rcpp::NumericMatrix CmbTable::asMatrix() const {
    Rcpp::NumericMatrix m_out = df_to_matrix_(asDataFrame());

    Rcpp::CharacterVector cvColNames = Rcpp::clone(Rcpp::wrap(m_var_names));

    cvColNames.push_front("count");
    cvColNames.push_front("cmbid");
    Rcpp::colnames(m_out) = cvColNames;

    return m_out;
}

void CmbTable::show() const {
    std::string out = "cmbid count";
    for (const auto& s : m_var_names) {
        out += (" " + s);
    }
    Rcpp::Rcout << "C++ object of class CmbTable\n";
    Rcpp::Rcout << " Columns: " << out.c_str() << "\n";
}

RCPP_MODULE(mod_cmb_table) {
    Rcpp::class_<CmbTable>("CmbTable")

    .constructor
        ("Default constructor (combination vector of length 1)")
    .constructor<int>
        ("Length of the combination vector")
    .constructor<int, Rcpp::CharacterVector>
        ("Length of the combination vector, vector of variable names")

    .method("update", &CmbTable::update,
        "Increment by incr if int_cmb exists, else insert with count = incr")
    .method("updateFromMatrix", &CmbTable::updateFromMatrix,
        "update() on integer combinations contained in columns of a matrix")
    .method("updateFromMatrixByRow", &CmbTable::updateFromMatrixByRow,
        "update() on integer combinations contained in rows of a matrix")
    .const_method("asDataFrame", &CmbTable::asDataFrame,
        "Returns a dataframe containing the combinations table")
    .const_method("asMatrix", &CmbTable::asMatrix,
        "Returns a matrix containing the combinations table")
    .const_method("show", &CmbTable::show,
        "S4 show()")
    ;
}

