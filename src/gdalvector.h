/* R interface to a subset of the GDAL C API for vector
   https://gdal.org/api/vector_c_api.html
   Chris Toney <chris.toney at usda.gov> */

#ifndef gdalvector_H
#define gdalvector_H

#include "rcpp_util.h"

#include <string>
#include <vector>

// Predeclare some GDAL types until the public header is included
#ifndef GDAL_H_INCLUDED
typedef void *GDALDatasetH;
typedef void *OGRLayerH;
typedef enum {GA_ReadOnly = 0, GA_Update = 1} GDALAccess;
#endif


class GDALVector {

	private:
	std::string dsn_in;
	GDALDatasetH  hDataset;
	GDALAccess eAccess;
	OGRLayerH hLayer;
	bool bVirtual;
	
	public:
	GDALVector();
	GDALVector(OGRLayerH lyr_obj);
	GDALVector(Rcpp::CharacterVector dsn, std::string layer);
	GDALVector(Rcpp::CharacterVector dsn, std::string layer, bool read_only);
	GDALVector(Rcpp::CharacterVector dsn, std::string layer, bool read_only,
			Rcpp::CharacterVector open_options);
	
	std::string getDsn() const;
	bool isOpen() const;
	bool isVirtual() const;
	Rcpp::CharacterVector getFileList() const;

	std::string getDriverShortName() const;
	std::string getDriverLongName() const;

	double getFeatureCount(bool force) const;
	SEXP getNextFeature() const;

	void close();
	
	// methods for internal use not exported to R
	void _checkAccess(GDALAccess access_needed) const;
};

RCPP_EXPOSED_CLASS(GDALVector)

#endif
