/* R interface to a subset of the GDAL C API for raster
   https://gdal.org/api/raster_c_api.html
   Chris Toney <chris.toney at usda.gov> */

#ifndef gdalraster_H
#define gdalraster_H

#include "rcpp_util.h"

#include <string>
#include <vector>

#ifndef gdalraster_types_H
#include "cpl_port.h"
int CPL_DLL CPL_STDCALL GDALTermProgressR(double, const char *, void *);
#endif

// Predeclare some GDAL types until the public header is included
#ifndef GDAL_H_INCLUDED
typedef void *GDALDatasetH;
typedef void *GDALRasterBandH;
typedef enum {GA_ReadOnly = 0, GA_Update = 1} GDALAccess;
#endif

Rcpp::CharacterVector gdal_version();
std::string get_config_option(std::string key);
void set_config_option(std::string key, std::string value);
int get_cache_used();

Rcpp::NumericVector _apply_geotransform(const std::vector<double> gt, 
		double pixel, double line);

bool create(std::string format, std::string dst_filename,
		int xsize, int ysize, int nbands, std::string dataType,
		Rcpp::Nullable<Rcpp::CharacterVector> options);

bool createCopy(std::string format, std::string dst_filename,
		std::string src_filename, bool strict,
		Rcpp::Nullable<Rcpp::CharacterVector> options);

bool bandCopyWholeRaster(std::string src_filename, int src_band,
		std::string dst_filename, int dst_band,
		Rcpp::Nullable<Rcpp::CharacterVector> options);
			
Rcpp::NumericVector inv_geotransform(const std::vector<double> gt);

Rcpp::IntegerMatrix get_pixel_line(const Rcpp::NumericMatrix xy,
		const std::vector<double> gt);

Rcpp::DataFrame _combine(Rcpp::CharacterVector src_files,
		Rcpp::CharacterVector var_names,
		std::vector<int> bands, 
		std::string dst_filename,
		std::string fmt, 
		std::string dataType,
		Rcpp::Nullable<Rcpp::CharacterVector> options);

bool _dem_proc(std::string mode,
		std::string src_filename, 
		std::string dst_filename,
		Rcpp::Nullable<Rcpp::CharacterVector> cl_arg,
		Rcpp::Nullable<Rcpp::String> col_file);

bool fillNodata(std::string filename, int band, std::string mask_file,
		double max_dist, int smooth_iterations);
		
bool sieveFilter(std::string src_filename, int src_band,
		std::string dst_filename, int dst_band,
		int size_threshold, int connectedness,
		std::string mask_filename , int mask_band,
		Rcpp::Nullable<Rcpp::CharacterVector> options);
		
bool warp(Rcpp::CharacterVector src_files, std::string dst_filename,
		Rcpp::CharacterVector t_srs, 
		Rcpp::Nullable<Rcpp::CharacterVector> arg_list);
		
Rcpp::IntegerMatrix createColorRamp(int start_index,
		Rcpp::IntegerVector start_color,
		int end_index,
		Rcpp::IntegerVector end_color,
		std::string palette_interp);

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
	Rcpp::CharacterVector getFileList() const;
	
	void info() const;
	std::string infoAsJSON() const;
	
	std::string getDriverShortName() const;
	std::string getDriverLongName() const;
	
	int getRasterXSize() const;
	int getRasterYSize() const;
	std::vector<double> getGeoTransform() const;
	bool setGeoTransform(std::vector<double> transform);
	int getRasterCount() const;
	
	std::string getProjectionRef() const;
	bool setProjection(std::string projection);
	
	std::vector<double> bbox() const;
	std::vector<double> res() const;
	std::vector<int> dim() const;
	
	std::vector<int> getBlockSize(int band) const;
	int getOverviewCount(int band) const;
	void buildOverviews(std::string resampling, std::vector<int> levels,
			std::vector<int> bands);
	std::string getDataTypeName(int band) const;
	Rcpp::NumericVector getStatistics(int band,	bool approx_ok, 
			bool force) const;
	bool hasNoDataValue(int band) const;
	double getNoDataValue(int band) const;
	bool setNoDataValue(int band, double nodata_value);
	void deleteNoDataValue(int band);
	std::string getUnitType(int band) const;
	bool setUnitType(int band, std::string unit_type);
	bool hasScale(int band) const;
	double getScale(int band) const;
	bool setScale(int band, double scale);
	bool hasOffset(int band) const;
	double getOffset(int band) const;
	bool setOffset(int band, double offset);
	std::string getDescription(int band) const;
	void setDescription(int band, std::string desc);
	std::string getRasterColorInterp(int band) const;
	void setRasterColorInterp(int band, std::string col_interp);
	
	Rcpp::CharacterVector getMetadata(int band, std::string domain) const;
	std::string getMetadataItem(int band, std::string mdi_name, 
			std::string domain) const;
	void setMetadataItem(int band, std::string mdi_name, std::string mdi_value,
			std::string domain);
	
	SEXP read(int band, int xoff, int yoff, int xsize, int ysize,
			int out_xsize, int out_ysize) const;
				
	void write(int band, int xoff, int yoff, int xsize, int ysize,
			Rcpp::RObject rasterData);

	void fillRaster(int band, double value, double ivalue);
	
	SEXP getColorTable(int band) const;
	std::string getPaletteInterp(int band) const;
	bool setColorTable(int band, Rcpp::RObject &col_tbl,
			std::string palette_interp);
	
	void flushCache();
	
	int getChecksum(int band, int xoff, int yoff, int xsize, int ysize) const;
	
	void close();
	
	// methods for internal use not exposed in R
	void _checkAccess(GDALAccess access_needed) const;
	GDALRasterBandH _getBand(int band) const;
};

RCPP_EXPOSED_CLASS(GDALRaster)

#endif
