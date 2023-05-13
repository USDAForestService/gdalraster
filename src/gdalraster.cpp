/* Implementation of class GDALRaster
   Encapsulates a subset of GDALDataset, GDALDriver and GDALRasterBand.
   Chris Toney <chris.toney at usda.gov> */

#include <complex>
#include <algorithm>
#include <cmath>
#include <limits>

#include "gdal.h"
#include "cpl_conv.h"
#include "cpl_string.h"
#include "gdal_utils.h"
#include "gdal_alg.h"

#include <errno.h>

#include "gdalraster.h"

// [[Rcpp::init]]
void _gdal_init(DllInfo *dll) {
    GDALAllRegister();
    CPLSetConfigOption("OGR_CT_FORCE_TRADITIONAL_GIS_ORDER", "YES");
}

/*	ARE_REAL_EQUAL() from gdal_priv.h
	That header is not needed otherwise so copying here
	Copyright (c) 1998, Frank Warmerdam
	Copyright (c) 2007-2014, Even Rouault <even dot rouault at spatialys.com>
	License: MIT */
template <class T> inline bool ARE_REAL_EQUAL(T fVal1, T fVal2, int ulp = 2)
{
    return fVal1 == fVal2 || /* Should cover infinity */
           std::abs(fVal1 - fVal2) < std::numeric_limits<float>::epsilon() *
                                         std::abs(fVal1 + fVal2) * ulp;
}

GDALRaster::GDALRaster() : 
				fname(""),
				hDataset(NULL),
				eAccess(GA_ReadOnly) {}

GDALRaster::GDALRaster(std::string filename, bool read_only) : 
				fname(filename),
				hDataset(NULL),
				eAccess(GA_ReadOnly) {
						
	if (!read_only)
		eAccess = GA_Update;
	hDataset = GDALOpenShared(fname.c_str(), eAccess);
	if (hDataset == NULL)
		Rcpp::stop("Open raster failed.");
}

std::string GDALRaster::getFilename() const {
	return fname;
}

void GDALRaster::open(bool read_only) {
	if (fname == "")
		Rcpp::stop("Filename is not set.");
	
	GDALClose(hDataset);
	hDataset = NULL;
	if (read_only)
		eAccess = GA_ReadOnly;
	else
		eAccess = GA_Update;
	hDataset = GDALOpenShared(fname.c_str(), eAccess);
	if (hDataset == NULL)
		Rcpp::stop("Open raster failed.");
}

bool GDALRaster::isOpen() const {
	if (hDataset == NULL)
		return false;
	else
		return true;
}

void GDALRaster::info() const {
	if (!this->isOpen())
		Rcpp::stop("Raster dataset is not open.");
	
	Rcpp::CharacterVector argv = {"-nomd", "-norat", "-noct"};
	std::vector<char *> opt(argv.size() + 1);
	for (R_xlen_t i = 0; i < argv.size(); ++i) {
		opt[i] = (char *) (argv[i]);
	}
	opt[argv.size()] = NULL;
	GDALInfoOptions* psOptions = GDALInfoOptionsNew(opt.data(), NULL);
	Rcpp::Rcout << GDALInfo(hDataset, psOptions);
	GDALInfoOptionsFree(psOptions);
}

std::string GDALRaster::getDriverShortName() const {
	if (!this->isOpen())
		Rcpp::stop("Raster dataset is not open.");
		
	GDALDriverH hDriver = GDALGetDatasetDriver(hDataset);
	return GDALGetDriverShortName(hDriver);
}

std::string GDALRaster::getDriverLongName() const {
	if (!this->isOpen())
		Rcpp::stop("Raster dataset is not open.");
		
	GDALDriverH hDriver = GDALGetDatasetDriver(hDataset);
	return GDALGetDriverLongName(hDriver);
}

int GDALRaster::getRasterXSize() const {
	if (!this->isOpen())
		Rcpp::stop("Raster dataset is not open.");
		
	return GDALGetRasterXSize(hDataset);
}

int GDALRaster::getRasterYSize() const {
	if (!this->isOpen())
		Rcpp::stop("Raster dataset is not open.");
		
	return GDALGetRasterYSize(hDataset);
}

std::vector<double> GDALRaster::getGeoTransform() const {
	if (!this->isOpen())
		Rcpp::stop("Raster dataset is not open.");
		
	std::vector<double> gt(6);
	if (GDALGetGeoTransform(hDataset, gt.data()) == CE_Failure)
		Rcpp::Rcerr << "Failed to get geotransform, default returned.\n";

	return gt;
}

bool GDALRaster::setGeoTransform(std::vector<double> transform) {
	if (!this->isOpen())
		Rcpp::stop("Raster dataset is not open.");
		
	if (GDALGetAccess(hDataset) == GA_ReadOnly)
		Rcpp::stop("Cannot set geotransform (GA_ReadOnly).");

	if (transform.size() != 6)
		Rcpp::stop("Argument must be a numeric vector of length six.");

	if (GDALSetGeoTransform(hDataset, transform.data()) == CE_Failure) {
		Rcpp::Rcerr << "Set geotransform failed.\n";
		return false;
	}
	else {
		return true;
	}
}

std::vector<double> GDALRaster::bbox() const {
	std::vector<double> gt = this->getGeoTransform();
	double xmin = gt[0];
	double xmax = xmin + gt[1] * this->getRasterXSize();
	double ymax = gt[3];
	double ymin = ymax + gt[5] * this->getRasterYSize();
	std::vector<double> ret = {xmin, ymin, xmax, ymax};
	return ret;
}

std::vector<double> GDALRaster::res() const {
	std::vector<double> gt = this->getGeoTransform();
	double pixel_width = gt[1];
	double pixel_height = std::fabs(gt[5]);
	std::vector<double> ret = {pixel_width, pixel_height};
	return ret;
}

int GDALRaster::getRasterCount() const {
	if (!this->isOpen())
		Rcpp::stop("Raster dataset is not open.");
		
	return GDALGetRasterCount(hDataset);
}

std::string GDALRaster::getProjectionRef() const {
	if (!this->isOpen())
		Rcpp::stop("Raster dataset is not open.");

	std::string srs(GDALGetProjectionRef(hDataset));
	if (srs.size() > 0 and srs != "") {
		return srs;
	}
	else {
		Rcpp::Rcerr << "Failed to get projection ref.\n";
		return "";
	}
}

bool GDALRaster::setProjection(std::string projection) {
	if (!this->isOpen())
		Rcpp::stop("Raster dataset is not open.");
		
	if (GDALGetAccess(hDataset) == GA_ReadOnly)
		Rcpp::stop("Cannot set projection (GA_ReadOnly).");
		
	if (projection.size() == 0 || projection == "") {
		Rcpp::Rcerr << "setProjection() requires a WKT string.\n";
		return false;
	}

	if (GDALSetProjection(hDataset, projection.c_str()) == CE_Failure) {
		Rcpp::Rcerr << "Set projection failed.\n";
		return false;
	}
	else {
		return true;
	}
}

std::vector<int> GDALRaster::getBlockSize(int band) const {
	if (!this->isOpen())
		Rcpp::stop("Raster dataset is not open.");
		
	GDALRasterBandH hBand = GDALGetRasterBand(hDataset, band);
	int nBlockXSize, nBlockYSize;
	GDALGetBlockSize(hBand, &nBlockXSize, &nBlockYSize);
	std::vector<int> ret = {nBlockXSize, nBlockYSize};
	return ret;
}

std::string GDALRaster::getDataTypeName(int band) const {
	if (!this->isOpen())
		Rcpp::stop("Raster dataset is not open.");
		
	GDALRasterBandH hBand = GDALGetRasterBand(hDataset, band);
	return GDALGetDataTypeName(GDALGetRasterDataType(hBand));
}

Rcpp::NumericVector GDALRaster::getStatistics(int band,	bool approx_ok, 
					bool force) const {
					
	if (!this->isOpen())
		Rcpp::stop("Raster dataset is not open.");
	
	GDALRasterBandH hBand = GDALGetRasterBand(hDataset, band);
	double min, max, mean, sd;
	CPLErr err;
	
	if (!force) {
		err = GDALGetRasterStatistics(hBand, approx_ok, force, 
								&min, &max, &mean, &sd);
	}
	else {
		GDALProgressFunc pfnProgress = GDALTermProgressR;
		void* pProgressData = NULL;
		err = GDALComputeRasterStatistics(hBand, approx_ok, 
									&min, &max, &mean, &sd, 
									pfnProgress, pProgressData);
	}
	
	if (err != CE_None) {
		Rcpp::Rcout << "Failed to get statistics, NA returned.\n";
		Rcpp::NumericVector stats(4, NA_REAL);
		return stats;
	}
	else {
		Rcpp::NumericVector stats = {min, max, mean, sd};
		return stats;
	}
}

bool GDALRaster::hasNoDataValue(int band) const {
	if (!this->isOpen())
		Rcpp::stop("Raster dataset is not open.");
		
	GDALRasterBandH hBand = GDALGetRasterBand(hDataset, band);
	int hasNoData;
	GDALGetRasterNoDataValue(hBand, &hasNoData);
	return hasNoData;
}

double GDALRaster::getNoDataValue(int band) const {
	if (!this->isOpen())
		Rcpp::stop("Raster dataset is not open.");
		
	if (this->hasNoDataValue(band)) {
		GDALRasterBandH hBand = GDALGetRasterBand(hDataset, band);
		return GDALGetRasterNoDataValue(hBand, NULL);
	}
	else {
		return NA_REAL;
	}
}

bool GDALRaster::setNoDataValue(int band, double nodata_value) {
	if (!this->isOpen())
		Rcpp::stop("Raster dataset is not open.");
		
	if (GDALGetAccess(hDataset) == GA_ReadOnly)
		Rcpp::stop("Cannot set nodata value (GA_ReadOnly).");

	GDALRasterBandH hBand = GDALGetRasterBand(hDataset, band);
	if (GDALSetRasterNoDataValue(hBand, nodata_value) == CE_Failure) {
		Rcpp::Rcerr << "Set nodata value failed.\n";
		return false;
	}
	else {
		return true;
	}
}

void GDALRaster::deleteNoDataValue(int band) {
	if (!this->isOpen())
		Rcpp::stop("Raster dataset is not open.");
		
	if (GDALGetAccess(hDataset) == GA_ReadOnly)
		Rcpp::stop("Cannot delete nodata value (GA_ReadOnly).");

	GDALRasterBandH hBand = GDALGetRasterBand(hDataset, band);
	if (GDALDeleteRasterNoDataValue(hBand) == CE_Failure) {
		Rcpp::stop("Delete nodata value failed.");
	}
}

std::string GDALRaster::getUnitType(int band) const {
	if (!this->isOpen())
		Rcpp::stop("Raster dataset is not open.");
		
	GDALRasterBandH hBand = GDALGetRasterBand(hDataset, band);
	return GDALGetRasterUnitType(hBand);
}

bool GDALRaster::hasScale(int band) const {
	if (!this->isOpen())
		Rcpp::stop("Raster dataset is not open.");
		
	GDALRasterBandH hBand = GDALGetRasterBand(hDataset, band);
	int hasScale;
	GDALGetRasterScale(hBand, &hasScale);
	return hasScale;
}

double GDALRaster::getScale(int band) const {
	if (!this->isOpen())
		Rcpp::stop("Raster dataset is not open.");
		
	if (this->hasScale(band)) {
		GDALRasterBandH hBand = GDALGetRasterBand(hDataset, band);
		return GDALGetRasterScale(hBand, NULL);
	}
	else {
		return NA_REAL;
	}
}	

bool GDALRaster::hasOffset(int band) const {
	if (!this->isOpen())
		Rcpp::stop("Raster dataset is not open.");
		
	GDALRasterBandH hBand = GDALGetRasterBand(hDataset, band);
	int hasOffset;
	GDALGetRasterOffset(hBand, &hasOffset);
	return hasOffset;
}

double GDALRaster::getOffset(int band) const {
	if (!this->isOpen())
		Rcpp::stop("Raster dataset is not open.");
		
	if (this->hasOffset(band)) {
		GDALRasterBandH hBand = GDALGetRasterBand(hDataset, band);
		return GDALGetRasterOffset(hBand, NULL);
	}
	else {
		return NA_REAL;
	}
}

Rcpp::CharacterVector GDALRaster::getMetadata(int band, 
						std::string domain) const {
						
	if (!this->isOpen())
		Rcpp::stop("Raster dataset is not open.");
	
	char **papszMetadata;

	if (band == 0) {
		if (domain == "")
			papszMetadata = GDALGetMetadata(hDataset, NULL);
		else
			papszMetadata = GDALGetMetadata(hDataset, domain.c_str());
	}
	else {
		GDALRasterBandH hBand = GDALGetRasterBand(hDataset, band);
		if (domain == "")
			papszMetadata = GDALGetMetadata(hBand, NULL);
		else
			papszMetadata = GDALGetMetadata(hDataset, domain.c_str());
	}
	
	int items = CSLCount(papszMetadata);
	if (items > 0) {
		Rcpp::CharacterVector md(items);
		for (int i=0; i < items; ++i) {
			md(i) = papszMetadata[i];
		}
		return md;
	}
	else {
		return "";	
	}
}

std::string GDALRaster::getMetadataItem(int band, std::string mdi_name, 
			std::string domain) const {
			
	if (!this->isOpen())
		Rcpp::stop("Raster dataset is not open.");

	const char* domain_ = NULL;
	if (domain != "")
		domain_ = domain.c_str();
		
	std::string mdi = "";

	if (band == 0) {	
		if (GDALGetMetadataItem(hDataset, mdi_name.c_str(), domain_) != NULL)
			mdi += std::string(
					GDALGetMetadataItem(hDataset, mdi_name.c_str(), NULL) );
	}
	else {
		GDALRasterBandH hBand = GDALGetRasterBand(hDataset, band);
		if (GDALGetMetadataItem(hBand, mdi_name.c_str(), domain_) != NULL) 
			mdi += std::string(
					GDALGetMetadataItem(hBand, mdi_name.c_str(), domain_) );
	}
	
	return mdi;
}

SEXP GDALRaster::read(int band, int xoff, int yoff, int xsize, int ysize,
		int out_xsize, int out_ysize) const {
	
	if (!this->isOpen())
		Rcpp::stop("Raster dataset is not open.");
	
	GDALRasterBandH hBand = GDALGetRasterBand(hDataset, band);
	GDALDataType eDT = GDALGetRasterDataType(hBand);
	
	if (GDALDataTypeIsComplex(eDT)) {
	// complex data types
		std::vector<std::complex<double>> buf(out_xsize * out_ysize);
		
		CPLErr err = GDALRasterIO(hBand, GF_Read, xoff, yoff, xsize, ysize,
						buf.data(), out_xsize, out_ysize, GDT_CFloat64, 0, 0);
					
		if (err == CE_Failure)
			Rcpp::stop("Read raster failed.");

		Rcpp::ComplexVector v = Rcpp::wrap(buf);
		v.attr("dim") = Rcpp::Dimension(out_ysize, out_xsize);
		return v;

	}
	else {
	// real data types
		std::vector<double> buf(out_xsize * out_ysize);
	
		CPLErr err = GDALRasterIO(hBand, GF_Read, xoff, yoff, xsize, ysize,
						buf.data(), out_xsize, out_ysize, GDT_Float64, 0, 0);
						
		if (err == CE_Failure)
			Rcpp::stop("Read raster failed.");


		if (this->hasNoDataValue(band)) {
		// with a nodata value
			double nodata_value = this->getNoDataValue(band);
			if (GDALDataTypeIsFloating(GDALGetRasterDataType(hBand))) {
			// floating point
				for (double& val : buf) {
					if (CPLIsNan(val))
						val = NA_REAL;
					else if (ARE_REAL_EQUAL(val, nodata_value))
						val = NA_REAL;
				}
			}
			else {
			// integer
				std::replace(buf.begin(), buf.end(), nodata_value, NA_REAL);
			}
		}
		// without a nodata value
		else if (GDALDataTypeIsFloating(GDALGetRasterDataType(hBand))) {
			for (double& val : buf) {
				if (CPLIsNan(val))
					val = NA_REAL;
			}
		}
		
		Rcpp::NumericVector v = Rcpp::wrap(buf);
		v.attr("dim") = Rcpp::Dimension(out_ysize, out_xsize);
		return v;
	}
}

void GDALRaster::write(int band, int xoff, int yoff, int xsize, int ysize,
		Rcpp::RObject rasterData) {
							
	if (!this->isOpen())
		Rcpp::stop("Raster dataset is not open.");
		
	if (GDALGetAccess(hDataset) == GA_ReadOnly)
		Rcpp::stop("Dataset is read-only.");
	
	if (!Rf_isMatrix(rasterData))
		Rcpp::stop("Data must be an array of numeric or complex.");
	
	GDALDataType eBufType;
	CPLErr err;
	if (Rcpp::is<Rcpp::NumericVector>(rasterData)) {
	// real data types
		Rcpp::NumericMatrix buf = Rcpp::as<Rcpp::NumericMatrix>(rasterData);
		eBufType = GDT_Float64;
		GDALRasterBandH hBand = GDALGetRasterBand(hDataset, band);
		int buf_xsize = buf.ncol();
		int buf_ysize = buf.nrow();
		// get Rcpp matrix as std::vector to access the underlying array
		std::vector<double> buf_ = Rcpp::as<std::vector<double>>(buf);
		
		err = GDALRasterIO(hBand, GF_Write, xoff, yoff, xsize, ysize,
					buf_.data(), buf_xsize, buf_ysize, eBufType, 0, 0);
	}
	else if (Rcpp::is<Rcpp::ComplexVector>(rasterData)) {
	// complex data types
		Rcpp::ComplexMatrix buf = Rcpp::as<Rcpp::ComplexMatrix>(rasterData);
		eBufType = GDT_CFloat64;
		GDALRasterBandH hBand = GDALGetRasterBand(hDataset, band);
		int buf_xsize = buf.ncol();
		int buf_ysize = buf.nrow();
		// get Rcpp matrix as std::vector to access the underlying array
		std::vector<std::complex<double>> buf_ = 
			Rcpp::as<std::vector<std::complex<double>>>(buf);
		
		err = GDALRasterIO(hBand, GF_Write, xoff, yoff, xsize, ysize,
					buf_.data(), buf_xsize, buf_ysize, eBufType, 0, 0);
	}
	else {
		Rcpp::stop("Data must be an array of numeric or complex.");
	}
		
	if (err == CE_Failure)
		Rcpp::stop("Write to raster failed.");
}

void GDALRaster::fillRaster(int band, double value, double ivalue) {
	if (!this->isOpen())
		Rcpp::stop("Raster dataset is not open.");
		
	if (GDALGetAccess(hDataset) == GA_ReadOnly)
		Rcpp::stop("Dataset is read-only.");
	
	GDALRasterBandH hBand = GDALGetRasterBand(hDataset, band);
	if (GDALFillRaster(hBand, value, ivalue) == CE_Failure) {
		Rcpp::stop("Fill raster failed.");
	}
}

int GDALRaster::getChecksum(int band, int xoff, int yoff, int xsize, int ysize) const {
	if (!this->isOpen())
		Rcpp::stop("Raster dataset is not open.");
	
	GDALRasterBandH hBand = GDALGetRasterBand(hDataset, band);
	return GDALChecksumImage(hBand, xoff, yoff, xsize, ysize);
}

void GDALRaster::_setMetadataItem(int band, std::string mdi_name, 
		std::string mdi_value, std::string domain) {
		
	// currently only for GDALRasterBand metadata item
	
	if (!this->isOpen())
		Rcpp::stop("Raster dataset is not open.");
		
	if (GDALGetAccess(hDataset) == GA_ReadOnly)
		Rcpp::stop("Cannot set metadata item (GA_ReadOnly).");
		
	GDALRasterBandH hBand = GDALGetRasterBand(hDataset, band);
	if (GDALSetMetadataItem(hBand, mdi_name.c_str(), mdi_value.c_str(),
							domain.c_str()) != CE_None)
		Rcpp::stop("Set metadata item failed.");
}

void GDALRaster::close() {
	GDALClose(hDataset);
	hDataset = NULL;
}

RCPP_MODULE(mod_GDALRaster) {

    Rcpp::class_<GDALRaster>("GDALRaster")

    .constructor<std::string, bool>
    	("Usage: new(GDALRaster, filename, read_only=TRUE")
    
    // exposed member functions
    .const_method("getFilename", &GDALRaster::getFilename, 
    	"Return the raster filename.")
    .method("open", &GDALRaster::open, 
    	"(Re-)open the raster dataset on the existing filename.")
    .const_method("isOpen", &GDALRaster::isOpen, 
    	"Is the raster dataset open?")
    .const_method("info", &GDALRaster::info,
    	"Print various information about the raster dataset.")
    .const_method("getDriverShortName", &GDALRaster::getDriverShortName,
    	 "Return the short name of the format driver.")
    .const_method("getDriverLongName", &GDALRaster::getDriverLongName,
    	"Return the long name of the format driver.")
    .const_method("getRasterXSize", &GDALRaster::getRasterXSize, 
    	"Return raster width in pixels.")
    .const_method("getRasterYSize", &GDALRaster::getRasterYSize, 
    	"Return raster height in pixels.")
    .const_method("getGeoTransform", &GDALRaster::getGeoTransform, 
    	"Return the affine transformation coefficients.")
    .method("setGeoTransform", &GDALRaster::setGeoTransform, 
    	"Set the affine transformation coefficients for this dataset.")
    .const_method("bbox", &GDALRaster::bbox, 
    	"Return the bounding box (xmin, ymin, xmax, ymax).")
    .const_method("res", &GDALRaster::res, 
    	"Return the resolution (pixel width, pixel height).")
    .const_method("getRasterCount", &GDALRaster::getRasterCount, 
    	"Return the number of raster bands on this dataset.")
    .const_method("getProjectionRef", &GDALRaster::getProjectionRef, 
    	"Return the projection definition for this dataset.")
    .method("setProjection", &GDALRaster::setProjection, 
    	"Set the projection reference string for this dataset.")
    .const_method("getBlockSize", &GDALRaster::getBlockSize, 
    	"Get the natural block size of this band.")
    .const_method("getDataTypeName", &GDALRaster::getDataTypeName, 
    	"Get name of the data type for this band.")
    .const_method("getStatistics", &GDALRaster::getStatistics, 
    	"Get min, max, mean and stdev for this band.")
    .const_method("getNoDataValue", &GDALRaster::getNoDataValue, 
    	"Return the nodata value for this band.")
    .method("setNoDataValue", &GDALRaster::setNoDataValue, 
    	"Set the nodata value for this band.")
    .method("deleteNoDataValue", &GDALRaster::deleteNoDataValue, 
    	"Delete the nodata value for this band.")
    .const_method("getUnitType", &GDALRaster::getUnitType, 
    	"Get name of the raster value units (e.g., m or ft).")
    .const_method("getScale", &GDALRaster::getScale, 
    	"Return the raster value scale.")
    .const_method("getOffset", &GDALRaster::getOffset, 
    	"Return the raster value offset.")
    .const_method("getMetadata", &GDALRaster::getMetadata, 
    	"Return a list of metadata item=value for a domain.")
    .const_method("getMetadataItem", &GDALRaster::getMetadataItem, 
    	"Return the value of a metadata item.")
    .const_method("read", &GDALRaster::read, 
    	"Read a region of raster data for a band.")
    .method("write", &GDALRaster::write, 
    	"Write a region of raster data for a band.")
    .method("fillRaster", &GDALRaster::fillRaster, 
    	"Fill this band with a constant value.")
    .const_method("getChecksum", &GDALRaster::getChecksum, 
    	"Compute checksum for raster region.")
    .method(".setMetadataItem", &GDALRaster::_setMetadataItem, 
    	"Set metadata item name=value in domain.")
    .method("close", &GDALRaster::close, 
    	"Close the GDAL dataset for proper cleanup.")
    
    ;
}
