/* Implementation of class GDALRaster
   Encapsulates a subset of GDALDataset, GDALDriver and GDALRasterBand.
   Chris Toney <chris.toney at usda.gov> */

#include <complex>
#include <algorithm>
#include <cmath>

#include "gdal_priv.h"
#include "cpl_port.h"
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
	
	// warn for now if 64-bit integer
	bool has_int64 = false;
	for (int b = 1; b <= this->getRasterCount(); ++b) {
		GDALRasterBandH hBand = GDALGetRasterBand(hDataset, b);
		GDALDataType eDT = GDALGetRasterDataType(hBand);
		if (GDALDataTypeIsInteger(eDT) && (GDALGetDataTypeSizeBits(eDT) == 64))
			has_int64 = true;
	}
	if (has_int64) {
		Rcpp::Rcout << "Int64/UInt64 raster data types not fully supported.\n";
		Rcpp::Rcout << "Loss of precision will occur for values > 2^53.\n";
		std::string msg = 
			"Int64/UInt64 raster data are currently handled as double.";
		Rcpp::warning(msg);
	}
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
	this->_checkAccess(GA_ReadOnly);
	
	Rcpp::CharacterVector argv = {"-norat", "-noct"};
	std::vector<char *> opt(argv.size() + 1);
	for (R_xlen_t i = 0; i < argv.size(); ++i) {
		opt[i] = (char *) (argv[i]);
	}
	opt[argv.size()] = NULL;
	GDALInfoOptions* psOptions = GDALInfoOptionsNew(opt.data(), NULL);
	if (psOptions == NULL)
		Rcpp::stop("Creation of GDALInfoOptions failed.");
	Rcpp::Rcout << GDALInfo(hDataset, psOptions);
	GDALInfoOptionsFree(psOptions);
}

std::string GDALRaster::infoAsJSON() const {
	this->_checkAccess(GA_ReadOnly);
	
	Rcpp::CharacterVector argv = {"-json", "-stats", "-hist"};
	std::vector<char *> opt(argv.size() + 1);
	for (R_xlen_t i = 0; i < argv.size(); ++i) {
		opt[i] = (char *) (argv[i]);
	}
	opt[argv.size()] = NULL;
	GDALInfoOptions* psOptions = GDALInfoOptionsNew(opt.data(), NULL);
	if (psOptions == NULL)
		Rcpp::stop("Creation of GDALInfoOptions failed.");
	std::string out = GDALInfo(hDataset, psOptions);
	GDALInfoOptionsFree(psOptions);
	out.erase(std::remove(out.begin(), out.end(), '\n'), out.cend());
	return out;
}

Rcpp::CharacterVector GDALRaster::getFileList() const {
	this->_checkAccess(GA_ReadOnly);
	
	char **papszFiles;
	papszFiles = GDALGetFileList(hDataset);
	
	int items = CSLCount(papszFiles);
	if (items > 0) {
		Rcpp::CharacterVector files(items);
		for (int i=0; i < items; ++i) {
			files(i) = papszFiles[i];
		}
		CSLDestroy(papszFiles);
		return files;
	}
	else {
		CSLDestroy(papszFiles);
		return "";	
	}
}

std::string GDALRaster::getDriverShortName() const {
	this->_checkAccess(GA_ReadOnly);
		
	GDALDriverH hDriver = GDALGetDatasetDriver(hDataset);
	return GDALGetDriverShortName(hDriver);
}

std::string GDALRaster::getDriverLongName() const {
	this->_checkAccess(GA_ReadOnly);
		
	GDALDriverH hDriver = GDALGetDatasetDriver(hDataset);
	return GDALGetDriverLongName(hDriver);
}

int GDALRaster::getRasterXSize() const {
	this->_checkAccess(GA_ReadOnly);
		
	return GDALGetRasterXSize(hDataset);
}

int GDALRaster::getRasterYSize() const {
	this->_checkAccess(GA_ReadOnly);
		
	return GDALGetRasterYSize(hDataset);
}

std::vector<double> GDALRaster::getGeoTransform() const {
	this->_checkAccess(GA_ReadOnly);
		
	std::vector<double> gt(6);
	if (GDALGetGeoTransform(hDataset, gt.data()) == CE_Failure)
		Rcpp::Rcerr << "Failed to get geotransform, default returned.\n";

	return gt;
}

bool GDALRaster::setGeoTransform(std::vector<double> transform) {
	this->_checkAccess(GA_Update);

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

int GDALRaster::getRasterCount() const {
	this->_checkAccess(GA_ReadOnly);
		
	return GDALGetRasterCount(hDataset);
}

std::string GDALRaster::getProjectionRef() const {
	this->_checkAccess(GA_ReadOnly);

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
	this->_checkAccess(GA_Update);
		
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

std::vector<double> GDALRaster::bbox() const {
	this->_checkAccess(GA_ReadOnly);
		
	std::vector<double> gt = this->getGeoTransform();
	double xmin = gt[0];
	double xmax = xmin + gt[1] * this->getRasterXSize();
	double ymax = gt[3];
	double ymin = ymax + gt[5] * this->getRasterYSize();
	std::vector<double> ret = {xmin, ymin, xmax, ymax};
	return ret;
}

std::vector<double> GDALRaster::res() const {
	this->_checkAccess(GA_ReadOnly);
		
	std::vector<double> gt = this->getGeoTransform();
	double pixel_width = gt[1];
	double pixel_height = std::fabs(gt[5]);
	std::vector<double> ret = {pixel_width, pixel_height};
	return ret;
}

std::vector<int> GDALRaster::dim() const {
	this->_checkAccess(GA_ReadOnly);
		
	std::vector<int> ret = {this->getRasterXSize(),
								this->getRasterYSize(),
								this->getRasterCount()};
	return ret;
}

std::vector<int> GDALRaster::getBlockSize(int band) const {
	this->_checkAccess(GA_ReadOnly);
		
	GDALRasterBandH hBand = this->_getBand(band);
	int nBlockXSize, nBlockYSize;
	GDALGetBlockSize(hBand, &nBlockXSize, &nBlockYSize);
	std::vector<int> ret = {nBlockXSize, nBlockYSize};
	return ret;
}

int GDALRaster::getOverviewCount(int band) const {
	this->_checkAccess(GA_ReadOnly);
		
	GDALRasterBandH hBand = this->_getBand(band);
	return GDALGetOverviewCount(hBand);
}

void GDALRaster::buildOverviews(std::string resampling,
		std::vector<int> levels, std::vector<int> bands) {

	this->_checkAccess(GA_ReadOnly);
		
	int nOvr;
	int* panOvrList = NULL;
	if (levels.size() == 1 && levels[0] == 0) {
		nOvr = 0;
	}
	else {
		nOvr = levels.size();
		panOvrList = levels.data();
	}
	
	int nBands;
	int* panBandList = NULL;
	if (bands.size() == 1 && bands[0] == 0) {
		nBands = 0;
	}
	else {
		nBands = bands.size();
		panBandList = bands.data();
	}
	
	CPLErr err = GDALBuildOverviews(hDataset, resampling.c_str(), nOvr,
			panOvrList, nBands, panBandList, GDALTermProgressR, NULL);

	if (err == CE_Failure)
		Rcpp::stop("Build overviews failed.");
}

std::string GDALRaster::getDataTypeName(int band) const {
	this->_checkAccess(GA_ReadOnly);
		
	GDALRasterBandH hBand = this->_getBand(band);
	return GDALGetDataTypeName(GDALGetRasterDataType(hBand));
}

Rcpp::NumericVector GDALRaster::getStatistics(int band,	bool approx_ok, 
					bool force) const {
					
	this->_checkAccess(GA_ReadOnly);
	
	GDALRasterBandH hBand = this->_getBand(band);
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
	this->_checkAccess(GA_ReadOnly);
		
	GDALRasterBandH hBand = this->_getBand(band);
	int hasNoData;
	GDALGetRasterNoDataValue(hBand, &hasNoData);
	return hasNoData;
}

double GDALRaster::getNoDataValue(int band) const {
	this->_checkAccess(GA_ReadOnly);
		
	if (this->hasNoDataValue(band)) {
		GDALRasterBandH hBand = this->_getBand(band);
		return GDALGetRasterNoDataValue(hBand, NULL);
	}
	else {
		return NA_REAL;
	}
}

bool GDALRaster::setNoDataValue(int band, double nodata_value) {
	this->_checkAccess(GA_Update);

	GDALRasterBandH hBand = this->_getBand(band);
	if (GDALSetRasterNoDataValue(hBand, nodata_value) == CE_Failure) {
		Rcpp::Rcerr << "Set nodata value failed.\n";
		return false;
	}
	else {
		return true;
	}
}

void GDALRaster::deleteNoDataValue(int band) {
	this->_checkAccess(GA_Update);

	GDALRasterBandH hBand = this->_getBand(band);
	if (GDALDeleteRasterNoDataValue(hBand) == CE_Failure) {
		Rcpp::stop("Delete nodata value failed.");
	}
}

std::string GDALRaster::getUnitType(int band) const {
	this->_checkAccess(GA_ReadOnly);
		
	GDALRasterBandH hBand = this->_getBand(band);
	return GDALGetRasterUnitType(hBand);
}

bool GDALRaster::setUnitType(int band, std::string unit_type) {
	this->_checkAccess(GA_Update);

	GDALRasterBandH hBand = this->_getBand(band);
	if (GDALSetRasterUnitType(hBand, unit_type.c_str()) == CE_Failure) {
		Rcpp::Rcerr << "Set unit type failed.\n";
		return false;
	}
	else {
		return true;
	}
}

bool GDALRaster::hasScale(int band) const {
	this->_checkAccess(GA_ReadOnly);
		
	GDALRasterBandH hBand = this->_getBand(band);
	int hasScale;
	GDALGetRasterScale(hBand, &hasScale);
	return hasScale;
}

double GDALRaster::getScale(int band) const {
	this->_checkAccess(GA_ReadOnly);
		
	if (this->hasScale(band)) {
		GDALRasterBandH hBand = this->_getBand(band);
		return GDALGetRasterScale(hBand, NULL);
	}
	else {
		return NA_REAL;
	}
}

bool GDALRaster::setScale(int band, double scale) {
	this->_checkAccess(GA_Update);

	GDALRasterBandH hBand = this->_getBand(band);
	if (GDALSetRasterScale(hBand, scale) == CE_Failure) {
		Rcpp::Rcerr << "Set scale failed.\n";
		return false;
	}
	else {
		return true;
	}
}

bool GDALRaster::hasOffset(int band) const {
	this->_checkAccess(GA_ReadOnly);
		
	GDALRasterBandH hBand = this->_getBand(band);
	int hasOffset;
	GDALGetRasterOffset(hBand, &hasOffset);
	return hasOffset;
}

double GDALRaster::getOffset(int band) const {
	this->_checkAccess(GA_ReadOnly);
		
	if (this->hasOffset(band)) {
		GDALRasterBandH hBand = this->_getBand(band);
		return GDALGetRasterOffset(hBand, NULL);
	}
	else {
		return NA_REAL;
	}
}

bool GDALRaster::setOffset(int band, double offset) {
	this->_checkAccess(GA_Update);

	GDALRasterBandH hBand = this->_getBand(band);
	if (GDALSetRasterOffset(hBand, offset) == CE_Failure) {
		Rcpp::Rcerr << "Set offset failed.\n";
		return false;
	}
	else {
		return true;
	}
}

std::string GDALRaster::getDescription(int band) const {
	this->_checkAccess(GA_ReadOnly);

	std::string desc;

	if (band == 0) {	
		desc = GDALGetDescription(hDataset);
	}
	else {
		GDALRasterBandH hBand = this->_getBand(band);
		desc = GDALGetDescription(hBand);
	}
	
	return desc;
}

void GDALRaster::setDescription(int band, std::string desc) {
	this->_checkAccess(GA_Update);

	GDALRasterBandH hBand = this->_getBand(band);
	GDALSetDescription(hBand, desc.c_str());
}

std::string GDALRaster::getRasterColorInterp(int band) const {
	this->_checkAccess(GA_ReadOnly);

	GDALRasterBandH hBand = this->_getBand(band);
	GDALColorInterp gci = GDALGetRasterColorInterpretation(hBand);
	
	std::string col_interp = "";
	switch (gci) {
		case GCI_Undefined:
			col_interp = "Undefined";
			break;
		case GCI_GrayIndex:
			col_interp = "Gray";
			break;
		case GCI_PaletteIndex:
			col_interp = "Palette";
			break;
		case GCI_RedBand:
			col_interp = "Red";
			break;
		case GCI_GreenBand:
			col_interp = "Green";
			break;
		case GCI_BlueBand:
			col_interp = "Blue";
			break;
		case GCI_AlphaBand:
			col_interp = "Alpha";
			break;
		case GCI_HueBand:
			col_interp = "Hue";
			break;
		case GCI_SaturationBand:
			col_interp = "Saturation";
			break;
		case GCI_LightnessBand:
			col_interp = "Lightness";
			break;
		case GCI_CyanBand:
			col_interp = "Cyan";
			break;
		case GCI_MagentaBand:
			col_interp = "Magenta";
			break;
		case GCI_YellowBand:
			col_interp = "Yellow";
			break;
		case GCI_BlackBand:
			col_interp = "Black";
			break;
		case GCI_YCbCr_YBand:
			col_interp = "YCbCr_Y";
			break;
		case GCI_YCbCr_CbBand:
			col_interp = "YCbCr_Cb";
			break;
		case GCI_YCbCr_CrBand:
			col_interp = "YCbCr_Cr";
			break;
	}

	return col_interp;
}

void GDALRaster::setRasterColorInterp(int band, std::string col_interp) {
	this->_checkAccess(GA_Update);

	GDALRasterBandH hBand = this->_getBand(band);
	GDALColorInterp gci;
	
	if (col_interp == "Undefined")
		gci = GCI_Undefined;
	else if (col_interp == "Gray")
		gci = GCI_GrayIndex;
	else if (col_interp == "Palette")
		gci = GCI_PaletteIndex;
	else if (col_interp == "Red")
		gci = GCI_RedBand;
	else if (col_interp == "Green")
		gci = GCI_GreenBand;
	else if (col_interp == "Blue")
		gci = GCI_BlueBand;
	else if (col_interp == "Alpha")
		gci = GCI_AlphaBand;
	else if (col_interp == "Hue")
		gci = GCI_HueBand;
	else if (col_interp == "Saturation")
		gci = GCI_SaturationBand;
	else if (col_interp == "Lightness")
		gci = GCI_LightnessBand;
	else if (col_interp == "Cyan")
		gci = GCI_CyanBand;
	else if (col_interp == "Magenta")
		gci = GCI_MagentaBand;
	else if (col_interp == "Yellow")
		gci = GCI_YellowBand;
	else if (col_interp == "Black")
		gci = GCI_BlackBand;
	else if (col_interp == "YCbCr_Y")
		gci = GCI_YCbCr_YBand;
	else if (col_interp == "YCbCr_Cb")
		gci = GCI_YCbCr_CbBand;
	else if (col_interp == "YCbCr_Cr")
		gci = GCI_YCbCr_CrBand;
	else
		Rcpp::stop("Invalid col_interp.");

	GDALSetRasterColorInterpretation(hBand, gci);
}

Rcpp::CharacterVector GDALRaster::getMetadata(int band, 
						std::string domain) const {
						
	this->_checkAccess(GA_ReadOnly);
	
	char **papszMetadata;

	if (band == 0) {
		if (domain == "")
			papszMetadata = GDALGetMetadata(hDataset, NULL);
		else
			papszMetadata = GDALGetMetadata(hDataset, domain.c_str());
	}
	else {
		GDALRasterBandH hBand = this->_getBand(band);
		if (domain == "")
			papszMetadata = GDALGetMetadata(hBand, NULL);
		else
			papszMetadata = GDALGetMetadata(hBand, domain.c_str());
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
			
	this->_checkAccess(GA_ReadOnly);

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
		GDALRasterBandH hBand = this->_getBand(band);
		if (GDALGetMetadataItem(hBand, mdi_name.c_str(), domain_) != NULL) 
			mdi += std::string(
					GDALGetMetadataItem(hBand, mdi_name.c_str(), domain_) );
	}
	
	return mdi;
}

void GDALRaster::setMetadataItem(int band, std::string mdi_name, 
		std::string mdi_value, std::string domain) {
	
	this->_checkAccess(GA_Update);

	const char* domain_ = NULL;
	if (domain != "")
		domain_ = domain.c_str();

	if (band == 0) {
		if (GDALSetMetadataItem(hDataset, mdi_name.c_str(), mdi_value.c_str(),
								domain_) != CE_None)
			Rcpp::stop("Set metadata item failed.");
	}
	else {
		GDALRasterBandH hBand = this->_getBand(band);
		if (GDALSetMetadataItem(hBand, mdi_name.c_str(), mdi_value.c_str(),
								domain_) != CE_None)
			Rcpp::stop("Set metadata item failed.");
	}
}

SEXP GDALRaster::read(int band, int xoff, int yoff, int xsize, int ysize,
		int out_xsize, int out_ysize) const {
	
	this->_checkAccess(GA_ReadOnly);
	
	GDALRasterBandH hBand = GDALGetRasterBand(hDataset, band);
	if (hBand == NULL)
		Rcpp::stop("Failed to access the requested band.");
	GDALDataType eDT = GDALGetRasterDataType(hBand);
	
	CPLErr err;
	
	if (GDALDataTypeIsComplex(eDT)) {
	// complex data types
		std::vector<std::complex<double>> buf(out_xsize * out_ysize);
		
		err = GDALRasterIO(hBand, GF_Read, xoff, yoff, xsize, ysize,
				buf.data(), out_xsize, out_ysize, GDT_CFloat64, 0, 0);
					
		if (err == CE_Failure)
			Rcpp::stop("Read raster failed.");

		Rcpp::ComplexVector v = Rcpp::wrap(buf);
		return v;

	}
	else {
	// real data types
		if ( ( GDALDataTypeIsInteger(eDT) && 
				(GDALGetDataTypeSizeBits(eDT) <= 32) && 
				GDALDataTypeIsSigned(eDT) ) ||
				( GDALDataTypeIsInteger(eDT) && 
				(GDALGetDataTypeSizeBits(eDT) <= 16) && 
				!GDALDataTypeIsSigned(eDT) ) ) {
			
			// signed integer <= 32 bits and unsigned integer <= 16 bits
			// use int32 buffer
		
			std::vector<GInt32> buf(out_xsize * out_ysize);
		
			err = GDALRasterIO(hBand, GF_Read, xoff, yoff, xsize, ysize,
					buf.data(), out_xsize, out_ysize, GDT_Int32, 0, 0);
							
			if (err == CE_Failure)
				Rcpp::stop("Read raster failed.");
			
			if (this->hasNoDataValue(band)) {
				GInt32 nodata_value = (GInt32) this->getNoDataValue(band);
				std::replace(buf.begin(), buf.end(), nodata_value, NA_INTEGER);
			}
			
			Rcpp::IntegerVector v = Rcpp::wrap(buf);
			return v;
		
		}
		else {
		
			// UInt32, Float32, Float64
			// use double buffer
			// (Int64, UInt64 would currently be handled here but would lose
			//  precision when > 9,007,199,254,740,992 (2^53). Support for
			//  Int64/UInt64 raster could potentially be added using {bit64}.)

			std::vector<double> buf(out_xsize * out_ysize);

			err = GDALRasterIO(hBand, GF_Read, xoff, yoff, xsize, ysize,
					buf.data(), out_xsize, out_ysize, GDT_Float64, 0, 0);
							
			if (err == CE_Failure)
				Rcpp::stop("Read raster failed.");

			if (this->hasNoDataValue(band)) {
			// with a nodata value
				double nodata_value = this->getNoDataValue(band);
				if (GDALDataTypeIsFloating(eDT)) {
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
			else if (GDALDataTypeIsFloating(eDT)) {
				for (double& val : buf) {
					if (CPLIsNan(val))
						val = NA_REAL;
				}
			}
			
			Rcpp::NumericVector v = Rcpp::wrap(buf);
			return v;
		}
	}
}

void GDALRaster::write(int band, int xoff, int yoff, int xsize, int ysize,
		Rcpp::RObject rasterData) {

	this->_checkAccess(GA_Update);
	
	GDALDataType eBufType;
	CPLErr err;
	
	GDALRasterBandH hBand = GDALGetRasterBand(hDataset, band);
	if (hBand == NULL)
		Rcpp::stop("Failed to access the requested band.");
	
	if (Rcpp::is<Rcpp::IntegerVector>(rasterData) || 
			Rcpp::is<Rcpp::NumericVector>(rasterData)) {
		
		// real data types
	
		eBufType = GDT_Float64;
		std::vector<double> buf_ = Rcpp::as<std::vector<double>>(rasterData);
		if (buf_.size() != ((std::size_t) (xsize * ysize)))
			Rcpp::stop("Size of input data is not the same as region size.");
		err = GDALRasterIO(hBand, GF_Write, xoff, yoff, xsize, ysize,
					buf_.data(), xsize, ysize, eBufType, 0, 0);
	}
	else if (Rcpp::is<Rcpp::ComplexVector>(rasterData)) {
	
		// complex data types
	
		eBufType = GDT_CFloat64;
		std::vector<std::complex<double>> buf_ = 
			Rcpp::as<std::vector<std::complex<double>>>(rasterData);
		if (buf_.size() != ((std::size_t) (xsize * ysize)))
			Rcpp::stop("Size of input data is not the same as region size.");
		err = GDALRasterIO(hBand, GF_Write, xoff, yoff, xsize, ysize,
					buf_.data(), xsize, ysize, eBufType, 0, 0);
	}
	else {
		Rcpp::stop("Data must be numeric or complex vector.");
	}

	if (err == CE_Failure)
		Rcpp::stop("Write to raster failed.");
}

void GDALRaster::fillRaster(int band, double value, double ivalue) {
	this->_checkAccess(GA_Update);
	
	GDALRasterBandH hBand = this->_getBand(band);
	if (GDALFillRaster(hBand, value, ivalue) == CE_Failure) {
		Rcpp::stop("Fill raster failed.");
	}
}

SEXP GDALRaster::getColorTable(int band) const {
	this->_checkAccess(GA_ReadOnly);
	
	GDALRasterBandH hBand = this->_getBand(band);
	GDALColorTableH hColTbl = GDALGetRasterColorTable(hBand);
	if (hColTbl == NULL)
		return R_NilValue;

	int nEntries = GDALGetColorEntryCount(hColTbl);
	GDALPaletteInterp gpi = GDALGetPaletteInterpretation(hColTbl);
	Rcpp::IntegerMatrix col_tbl(nEntries, 5);
	Rcpp::CharacterVector col_tbl_names;

	if (gpi == GPI_Gray) {
		col_tbl_names = {"value", "gray", "c2", "c3", "c4"};
		Rcpp::colnames(col_tbl) = col_tbl_names;
	}
	else if (gpi == GPI_RGB) {
		col_tbl_names = {"value", "red", "green", "blue", "alpha"};
		Rcpp::colnames(col_tbl) = col_tbl_names;
	}
	else if (gpi == GPI_CMYK) {
		col_tbl_names = {"value", "cyan", "magenta", "yellow", "black"};
		Rcpp::colnames(col_tbl) = col_tbl_names;
	}
	else if (gpi == GPI_HLS) {
		col_tbl_names = {"value", "hue", "lightness", "saturation", "c4"};
		Rcpp::colnames(col_tbl) = col_tbl_names;
	}
	else {
		col_tbl_names = {"value", "c1", "c2", "c3", "c4"};
		Rcpp::colnames(col_tbl) = col_tbl_names;
	}

	for (int i=0; i < nEntries; ++i) {
		const GDALColorEntry* colEntry = GDALGetColorEntry(hColTbl, i);
		col_tbl(i, 0) = i;
		col_tbl(i, 1) = colEntry->c1;
		col_tbl(i, 2) = colEntry->c2;
		col_tbl(i, 3) = colEntry->c3;
		col_tbl(i, 4) = colEntry->c4;
	}
	
	return col_tbl;
}

std::string GDALRaster::getPaletteInterp(int band) const {
	this->_checkAccess(GA_ReadOnly);
	
	GDALRasterBandH hBand = this->_getBand(band);
	GDALColorTableH hColTbl = GDALGetRasterColorTable(hBand);
	if (hColTbl == NULL)
		return "";

	GDALPaletteInterp gpi = GDALGetPaletteInterpretation(hColTbl);

	if (gpi == GPI_Gray) {
		return "Gray";
	}
	else if (gpi == GPI_RGB) {
		return "RGB";
	}
	else if (gpi == GPI_CMYK) {
		return "CMYK";
	}
	else if (gpi == GPI_HLS) {
		return "HLS";
	}
	else {
		return "unknown";
	}
}

bool GDALRaster::setColorTable(int band, Rcpp::RObject &col_tbl, 
		std::string palette_interp) {
		
	this->_checkAccess(GA_Update);
	
	GDALRasterBandH hBand = this->_getBand(band);
	
	Rcpp::IntegerMatrix m_col_tbl;
	if (Rcpp::is<Rcpp::DataFrame>(col_tbl)) {
		m_col_tbl = _df_to_int_matrix(col_tbl);
	}
	else if (Rcpp::is<Rcpp::IntegerVector>(col_tbl)) {
		if (Rf_isMatrix(col_tbl))
			m_col_tbl = Rcpp::as<Rcpp::IntegerMatrix>(col_tbl);
	}
	else {
		Rcpp::stop("col_tbl must be a data frame or matrix.");
	}
		
	if (m_col_tbl.ncol() < 4 || m_col_tbl.ncol() > 5)
		Rcpp::stop("col_tbl must have four or five columns.");
	if (m_col_tbl.ncol() == 4) {
		Rcpp::IntegerVector c4(m_col_tbl.nrow(), 255);
		m_col_tbl = Rcpp::cbind(m_col_tbl, c4);
	}
	
	GDALPaletteInterp gpi;
	if (palette_interp ==  "Gray" || palette_interp == "gray")
		gpi = GPI_Gray;
	else if (palette_interp ==  "RGB")
		gpi = GPI_RGB;
	else if (palette_interp ==  "CMYK")
		gpi = GPI_CMYK;
	else if (palette_interp ==  "HLS")
		gpi = GPI_HLS;
	else
		Rcpp::stop("Invalid palette_interp.");
	
	int max_value = Rcpp::max(m_col_tbl.column(0));
	GDALColorTableH hColTbl = GDALCreateColorTable(gpi);
	// initialize all entries
	for (int i=0; i <= max_value; ++i) {
		const GDALColorEntry col = {0, 0, 0, 0};
		GDALSetColorEntry(hColTbl, i, &col);
	}
	
	// set entries from input table
	for (int i=0; i < m_col_tbl.nrow(); ++i) {
		if (m_col_tbl(i,0) >= 0) {
			const GDALColorEntry col = {
					static_cast<short>(m_col_tbl(i,1)),
					static_cast<short>(m_col_tbl(i,2)),
					static_cast<short>(m_col_tbl(i,3)),
					static_cast<short>(m_col_tbl(i,4)) };
			GDALSetColorEntry(hColTbl, m_col_tbl(i,0), &col);
		}
		else {
			Rcpp::warning("Warning: skipped entry with negative value.");
		}
	}
	
	CPLErr err = GDALSetRasterColorTable(hBand, hColTbl);
	GDALDestroyColorTable(hColTbl);
	if (err == CE_None)
		return true;
	else
		return false;
}

void GDALRaster::flushCache() {
	this->_checkAccess(GA_Update);
	
	GDALFlushCache(hDataset);
}

int GDALRaster::getChecksum(int band, int xoff, int yoff,
		int xsize, int ysize) const {

	this->_checkAccess(GA_ReadOnly);
	
	GDALRasterBandH hBand = this->_getBand(band);
	return GDALChecksumImage(hBand, xoff, yoff, xsize, ysize);
}

void GDALRaster::close() {
	GDALClose(hDataset);
	hDataset = NULL;
}

// ****************************************************************************
// class methods for internal use not exposed in R
// ****************************************************************************

void GDALRaster::_checkAccess(GDALAccess access_needed) const {
	if (!this->isOpen())
		Rcpp::stop("Raster dataset is not open.");
	
	if (access_needed == GA_Update && this->eAccess == GA_ReadOnly)
		Rcpp::stop("Dataset is read-only.");
}

GDALRasterBandH GDALRaster::_getBand(int band) const {
	if (band < 1 || band > this->getRasterCount())
		Rcpp::stop("Illegal band number.");
	GDALRasterBandH hBand = GDALGetRasterBand(hDataset, band);
	if (hBand == NULL)
		Rcpp::stop("Failed to access the requested band.");
	return hBand;
}

// ****************************************************************************

RCPP_MODULE(mod_GDALRaster) {

    Rcpp::class_<GDALRaster>("GDALRaster")

    .constructor
    	("Default constructor, no dataset opened.")
    .constructor<std::string, bool>
    	("Usage: new(GDALRaster, filename, read_only=TRUE")
    
    // exposed member functions
    .const_method("getFilename", &GDALRaster::getFilename, 
    	"Return the raster filename.")
    .method("open", &GDALRaster::open, 
    	"(Re-)open the raster dataset on the existing filename.")
    .const_method("isOpen", &GDALRaster::isOpen, 
    	"Is the raster dataset open?")
    .const_method("getFileList", &GDALRaster::getFileList, 
    	"Fetch files forming dataset.")
    .const_method("info", &GDALRaster::info,
    	"Print various information about the raster dataset.")
    .const_method("infoAsJSON", &GDALRaster::infoAsJSON,
    	"Returns full output of gdalinfo as a JSON-formatted string.")
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
    .const_method("getRasterCount", &GDALRaster::getRasterCount, 
    	"Return the number of raster bands on this dataset.")
    .const_method("getProjectionRef", &GDALRaster::getProjectionRef, 
    	"Return the projection definition for this dataset.")
    .method("setProjection", &GDALRaster::setProjection, 
    	"Set the projection reference string for this dataset.")
    .const_method("bbox", &GDALRaster::bbox, 
    	"Return the bounding box (xmin, ymin, xmax, ymax).")
    .const_method("res", &GDALRaster::res, 
    	"Return the resolution (pixel width, pixel height).")
    .const_method("dim", &GDALRaster::dim, 
    	"Return raster dimensions (xsize, ysize, number of bands).")
    .const_method("getBlockSize", &GDALRaster::getBlockSize, 
    	"Get the natural block size of this band.")
    .const_method("getOverviewCount", &GDALRaster::getOverviewCount, 
    	"Return the number of overview layers available.")
    .method("buildOverviews", &GDALRaster::buildOverviews, 
    	"Build raster overview(s).")
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
    .method("setUnitType", &GDALRaster::setUnitType, 
    	"Set name of the raster value units (e.g., m or ft).")
    .const_method("getScale", &GDALRaster::getScale, 
    	"Return the raster value scaling ratio.")
    .method("setScale", &GDALRaster::setScale, 
    	"Set the raster value scaling ratio.")
    .const_method("getOffset", &GDALRaster::getOffset, 
    	"Return the raster value offset.")
    .method("setOffset", &GDALRaster::setOffset, 
    	"Set the raster value offset.")
    .const_method("getDescription", &GDALRaster::getDescription, 
    	"Return object description for a raster band.")
    .method("setDescription", &GDALRaster::setDescription, 
    	"Set object description for a raster band.")
    .const_method("getRasterColorInterp", &GDALRaster::getRasterColorInterp, 
    	"How should this band be interpreted as color?")
    .method("setRasterColorInterp", &GDALRaster::setRasterColorInterp, 
    	"Set color interpretation of a band.")
    .const_method("getMetadata", &GDALRaster::getMetadata, 
    	"Return a list of metadata item=value for a domain.")
    .const_method("getMetadataItem", &GDALRaster::getMetadataItem, 
    	"Return the value of a metadata item.")
    .method("setMetadataItem", &GDALRaster::setMetadataItem, 
    	"Set metadata item name=value in domain.")
    .const_method("read", &GDALRaster::read, 
    	"Read a region of raster data for a band.")
    .method("write", &GDALRaster::write, 
    	"Write a region of raster data for a band.")
    .method("fillRaster", &GDALRaster::fillRaster, 
    	"Fill this band with a constant value.")
    .const_method("getColorTable", &GDALRaster::getColorTable, 
    	"Return the color table associated with this band.")
    .const_method("getPaletteInterp", &GDALRaster::getPaletteInterp, 
    	"Get the palette interpretation.")
    .method("setColorTable", &GDALRaster::setColorTable, 
    	"Set a color table for this band.")
    .method("flushCache", &GDALRaster::flushCache, 
    	"Flush all write cached data to disk.")
    .const_method("getChecksum", &GDALRaster::getChecksum, 
    	"Compute checksum for raster region.")
    .method("close", &GDALRaster::close, 
    	"Close the GDAL dataset for proper cleanup.")
    
    ;
}
