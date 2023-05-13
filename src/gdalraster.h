/* R interface to a subset of the GDAL C API for low level raster I/O
   See: https://gdal.org/api/raster_c_api.html)
   Chris Toney <chris.toney at usda.gov> */

#ifndef gdalraster_H
#define gdalraster_H

#include <Rcpp.h> 
// [[Rcpp::plugins(cpp11)]]

#include <string>
#include <vector>

#ifndef gdalraster_types_H
#include "cpl_port.h"
int CPL_DLL CPL_STDCALL GDALTermProgressR(double, const char *, void *);
#endif

// Predeclare some GDAL types until the public header is included
#ifndef GDAL_H_INCLUDED
typedef void *GDALDatasetH;
typedef enum {GA_ReadOnly = 0, GA_Update = 1} GDALAccess;
#endif

Rcpp::CharacterVector gdal_version();
std::string get_config_option(std::string key);
void set_config_option(std::string key, std::string value);

Rcpp::NumericVector _apply_geotransform(const std::vector<double> gt, 
		double pixel, double line);

bool create(std::string format, std::string dst_filename,
		int xsize, int ysize, int nbands, std::string dataType,
		Rcpp::Nullable<Rcpp::CharacterVector> options);

bool createCopy(std::string format, std::string dst_filename,
		std::string src_filename, bool strict,
		Rcpp::Nullable<Rcpp::CharacterVector> options);
			
Rcpp::NumericVector inv_geotransform(const std::vector<double> gt);

Rcpp::IntegerMatrix get_pixel_line(const Rcpp::NumericMatrix xy,
		const std::vector<double> gt);	

bool warp(Rcpp::CharacterVector src_files, std::string dst_filename,
		Rcpp::CharacterVector t_srs, 
		Rcpp::Nullable<Rcpp::CharacterVector> arg_list);

Rcpp::DataFrame _combine(
		Rcpp::CharacterVector src_files,
		Rcpp::CharacterVector var_names,
		std::vector<int> bands, 
		std::string dst_filename,
		std::string fmt, 
		std::string dataType,
		Rcpp::Nullable<Rcpp::CharacterVector> options);

class GDALRaster {

	private:
	std::string fname;
	GDALDatasetH  hDataset;
	GDALAccess eAccess;
	
	public:
	GDALRaster();
	GDALRaster(std::string filename, bool read_only);
	
	std::string getFilename() const;
	void open(bool read_only);
	bool isOpen() const;
	
	void info() const;
	
	std::string getDriverShortName() const;
	std::string getDriverLongName() const;
	
	int getRasterXSize() const;
	int getRasterYSize() const;
	std::vector<double> getGeoTransform() const;
	bool setGeoTransform(std::vector<double> transform);
	std::vector<double> bbox() const;
	std::vector<double> res() const;
	int getRasterCount() const;
	
	std::string getProjectionRef() const;
	bool setProjection(std::string projection);
	
	std::vector<int> getBlockSize(int band) const;
	std::string getDataTypeName(int band) const;
	Rcpp::NumericVector getStatistics(int band,	bool approx_ok, 
			bool force) const;
	bool hasNoDataValue(int band) const;
	double getNoDataValue(int band) const;
	bool setNoDataValue(int band, double nodata_value);
	void deleteNoDataValue(int band);
	std::string getUnitType(int band) const;
	bool hasScale(int band) const;
	double getScale(int band) const;
	bool hasOffset(int band) const;
	double getOffset(int band) const;
	
	Rcpp::CharacterVector getMetadata(int band, std::string domain) const;
	std::string getMetadataItem(int band,
			std::string mdi_name, std::string domain) const;
	
	SEXP read(int band, int xoff, int yoff, int xsize, int ysize,
			int out_xsize, int out_ysize) const;
				
	void write(int band, int xoff, int yoff, int xsize, int ysize,
			Rcpp::RObject rasterData);

	void fillRaster(int band, double value, double ivalue);
	
	int getChecksum(int band, int xoff, int yoff, int xsize, int ysize) const;
	
	void _setMetadataItem(int band, std::string mdi_name, 
			std::string mdi_value, std::string domain);
	
	void close();
};

RCPP_EXPOSED_CLASS(GDALRaster)

#endif
