/* Implementation of CmbTable class
Chris Toney <chris.toney at usda.gov> */

#include "cmb_table.h"

CmbTable::CmbTable(): 
	key_len(1), cvVarNames({"V1"}), last_ID(0)  {}

CmbTable::CmbTable(unsigned int keyLen, Rcpp::CharacterVector varNames): 
	key_len(keyLen), cvVarNames(varNames), last_ID(0)  {
	
	if (key_len != cvVarNames.size())
		Rcpp::stop("keyLen must equal length(varNames).");	
	}

double CmbTable::update(Rcpp::IntegerVector int_cmb, double incr) {
	// Increment count for existing int_cmb
	// or insert new int_cmb with count = incr.

	if (incr == 0)
		return 0;
	cmbKey key;
	key.cmb = int_cmb;
	cmbData& cmbdat = cmb_map[key];
	if (cmbdat.count != 0) {
		cmbdat.count += incr;
	}
	else {
		cmbdat.count = incr;
		cmbdat.ID = last_ID + 1;
		last_ID += 1;
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
	// (variables here are in the columns).
	// Return a vector of cmb IDs for the rows of the input matrix.

	R_xlen_t nrow = int_cmbs.nrow();
	Rcpp::NumericVector out(nrow);

	for (R_xlen_t k=0; k<nrow; ++k) {
		out[k] = update(int_cmbs.row(k), incr);
	}
	return out;
}

Rcpp::DataFrame CmbTable::asDataFrame() const {
	// Return the table as a data frame.
	// Set up the data in a named list, then create the dataframe from
	// the list.

	Rcpp::NumericVector dvCmbID(cmb_map.size());
	Rcpp::NumericVector dvCmbCount(cmb_map.size());
	std::vector<Rcpp::IntegerVector> aVec(key_len);
	cmbKey key;
	cmbData cmbdat;

	for(std::size_t n=0; n < key_len; ++n) {
		aVec[n] = Rcpp::IntegerVector(cmb_map.size());
	}
	unsigned long this_idx = 0;
	for(auto iter = cmb_map.begin(); iter != cmb_map.end(); ++iter) {
		key = iter->first;
		cmbdat = iter->second;
		dvCmbID[this_idx] = cmbdat.ID;
		dvCmbCount[this_idx] = cmbdat.count;
		for(std::size_t var=0; var < key_len; ++var) {
			aVec[var][this_idx] = key.cmb[var];
		}
		++this_idx;
	}
	Rcpp::List lOut = Rcpp::List::create(Rcpp::Named("cmbid") = dvCmbID,
			Rcpp::Named("count") = dvCmbCount);
	for(std::size_t n=0; n < key_len; ++n) {
		lOut.push_back(aVec[n], Rcpp::String(cvVarNames[n]));
	}
	
	Rcpp::DataFrame dfOut(lOut);
	return dfOut;
}

Rcpp::NumericMatrix CmbTable::asMatrix() const {
	// Return the table as a numeric matrix.

	Rcpp::NumericMatrix m_out(cmb_map.size(), key_len + 2);
	cmbKey key;
	cmbData cmbdat;

	unsigned long this_idx = 0;
	for(auto iter = cmb_map.begin(); iter != cmb_map.end(); ++iter) {
		key = iter->first;
		cmbdat = iter->second;
		m_out(this_idx, 0) = cmbdat.ID;
		m_out(this_idx, 1) = cmbdat.count;
		for(std::size_t var=0; var < key_len; ++var) {
			m_out(this_idx, var+2) = key.cmb[var];
		}
		++this_idx;
	}
	
	Rcpp::CharacterVector cvColNames = Rcpp::clone(cvVarNames);
	cvColNames.push_front("count");
	cvColNames.push_front("cmbid");
	
	Rcpp::colnames(m_out) = cvColNames;
	return m_out;
}

RCPP_MODULE(mod_cmb_table) {

    Rcpp::class_<CmbTable>("CmbTable")

    .constructor
    	("Default constructor, combination vector of length 1.")
    .constructor<unsigned int, Rcpp::CharacterVector>
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

