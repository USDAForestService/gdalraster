/* Implementation of class CmbTable
Chris Toney <chris.toney at usda.gov> */

#include "cmb_table.h"

CmbTable::CmbTable(): 
	key_len(1), cvVarNames({"V1"}), last_ID(0)  {}

CmbTable::CmbTable(int keyLen, Rcpp::CharacterVector varNames): 
	key_len(keyLen), cvVarNames(varNames), last_ID(0)  {
	
	if (key_len != cvVarNames.size())
		Rcpp::stop("keyLen must equal length(varNames).");	
	}

double CmbTable::update(const Rcpp::IntegerVector& int_cmb, double incr) {
	// Increment count for existing int_cmb
	// or insert new int_cmb with count = incr.

	cmbKey key;
	key.cmb = int_cmb;
	cmbData& cmbdat = cmb_map[key];
	cmbdat.count += incr;
	if (cmbdat.ID == 0.0) {
		this->last_ID += 1.0;
		cmbdat.ID = this->last_ID;
	}
	return cmbdat.ID;
}

Rcpp::NumericVector CmbTable::updateFromMatrix(
		const Rcpp::IntegerMatrix& int_cmbs,
		double incr) {
		
	// int combinations (int_cmbs) are columns of a matrix (nrow = key_len).
	// Increment count for existing int_cmb,
	// else insert new int_cmb with count = incr.
	// Return a vector of cmb IDs for the columns of the input matrix.

	R_xlen_t ncol = int_cmbs.ncol();
	Rcpp::NumericVector out(ncol);

	for (R_xlen_t k=0; k<ncol; ++k) {
		out[k] = update(int_cmbs.column(k), incr);
	}
	return out;
}

Rcpp::NumericVector CmbTable::updateFromMatrixByRow(
		const Rcpp::IntegerMatrix& int_cmbs,
		double incr) {
		
	// int combinations (int_cmbs) are rows of a matrix (ncol = key_len).
	// Same as updateFromMatrix() except by row instead of by column
	// (i.e., variables here are in the columns).
	// Return a vector of cmb IDs for the rows of the input matrix.

	R_xlen_t nrow = int_cmbs.nrow();
	Rcpp::NumericVector out(nrow);

	for (R_xlen_t k=0; k<nrow; ++k) {
		out[k] = update(int_cmbs.row(k), incr);
	}
	return out;
}

Rcpp::DataFrame CmbTable::asDataFrame() const {

	Rcpp::NumericVector dvCmbID(cmb_map.size());
	Rcpp::NumericVector dvCmbCount(cmb_map.size());
	std::vector<Rcpp::IntegerVector> aVec(this->key_len);
	cmbKey key;
	cmbData cmbdat;

	for(int n=0; n < this->key_len; ++n) {
		aVec[n] = Rcpp::IntegerVector(cmb_map.size());
	}
	std::size_t this_idx = 0;
	for(auto iter = cmb_map.begin(); iter != cmb_map.end(); ++iter) {
		key = iter->first;
		cmbdat = iter->second;
		dvCmbID[this_idx] = cmbdat.ID;
		dvCmbCount[this_idx] = cmbdat.count;
		for(int var=0; var < this->key_len; ++var) {
			aVec[var][this_idx] = key.cmb[var];
		}
		++this_idx;
	}
	
	Rcpp::DataFrame dfOut = Rcpp::DataFrame::create();
	dfOut.push_back(dvCmbID, "cmbid");
	dfOut.push_back(dvCmbCount, "count");
	for(int n=0; n < this->key_len; ++n) {
		dfOut.push_back(aVec[n], Rcpp::String(this->cvVarNames[n]));
	}
	
	return dfOut;
}

Rcpp::NumericMatrix CmbTable::asMatrix() const {

	Rcpp::NumericMatrix m_out(cmb_map.size(), this->key_len + 2);
	cmbKey key;
	cmbData cmbdat;

	std::size_t this_idx = 0;
	for(auto iter = cmb_map.begin(); iter != cmb_map.end(); ++iter) {
		key = iter->first;
		cmbdat = iter->second;
		m_out(this_idx, 0) = cmbdat.ID;
		m_out(this_idx, 1) = cmbdat.count;
		for(int var=0; var < this->key_len; ++var) {
			m_out(this_idx, var+2) = key.cmb[var];
		}
		++this_idx;
	}
	
	Rcpp::CharacterVector cvColNames = Rcpp::clone(this->cvVarNames);
	cvColNames.push_front("count");
	cvColNames.push_front("cmbid");
	Rcpp::colnames(m_out) = cvColNames;
	
	return m_out;
}

RCPP_MODULE(mod_cmb_table) {

    Rcpp::class_<CmbTable>("CmbTable")

    .constructor
    	("Default constructor (combination vector of length 1)")
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
    ;
}

