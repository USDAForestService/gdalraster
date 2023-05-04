/* Hash table class for counting unique integer combinations
Chris Toney <chris.toney at usda.gov> */

#ifndef cmb_table_H
#define cmb_table_H

#include <Rcpp.h> 
// [[Rcpp::plugins(cpp11)]]

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
	double ID;
	double count = 0;
};

struct cmbHasher {
	// Boost hash_combine method
	//Copyright 2005-2014 Daniel James.
	//Copyright 2021, 2022 Peter Dimov.
	//Distributed under the Boost Software License, Version 1.0.
	//https://www.boost.org/LICENSE_1_0.txt
	
	std::size_t operator()(cmbKey const& vec) const {
		std::size_t seed = 0;
		for (R_xlen_t i=0; i < vec.cmb.size(); ++i) {
			seed ^= vec.cmb[i] + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		}
		return seed;
	}
};

class CmbTable {
	private:
	unsigned int key_len;
	Rcpp::CharacterVector cvVarNames;
	double last_ID;

	std::unordered_map<cmbKey, cmbData, cmbHasher> cmb_map;

	public:
	CmbTable();
	CmbTable(unsigned int keyLen, Rcpp::CharacterVector varNames);
	
	double update(Rcpp::IntegerVector int_cmb, double incr);
	
	Rcpp::NumericVector updateFromMatrix(const Rcpp::IntegerMatrix& int_cmbs,
		double incr);
	
	Rcpp::DataFrame asDataFrame() const;
};

RCPP_EXPOSED_CLASS(CmbTable)

#endif
