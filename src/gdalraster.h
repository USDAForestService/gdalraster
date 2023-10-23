/* R interface to a subset of the GDAL C API for raster
   https://gdal.org/api/raster_c_api.html
   Chris Toney <chris.toney at usda.gov> */

#ifndef gdalraster_H
#define gdalraster_H

#include "rcpp_util.h"

#include <map>
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

#ifdef GDAL_H_INCLUDED
// Map certain GDAL enums to string names for use in R
// GDALColorInterp (GCI)
const std::map<std::string, GDALColorInterp> MAP_GCI{
	{"Undefined", GCI_Undefined},
	{"Gray", GCI_GrayIndex},
	{"Palette", GCI_PaletteIndex},
	{"Red", GCI_RedBand},
	{"Green", GCI_GreenBand},
	{"Blue", GCI_BlueBand},
	{"Alpha", GCI_AlphaBand},
	{"Hue", GCI_HueBand},
	{"Saturation", GCI_SaturationBand},
	{"Lightness", GCI_LightnessBand},
	{"Cyan", GCI_CyanBand},
	{"Magenta", GCI_MagentaBand},
	{"Yellow", GCI_YellowBand},
	{"Black", GCI_BlackBand},
	{"YCbCr_Y", GCI_YCbCr_YBand},
	{"YCbCr_Cb", GCI_YCbCr_CbBand},
	{"YCbCr_Cr", GCI_YCbCr_CrBand}
};
// GDALRATFieldUsage (GFU)
const std::map<std::string, GDALRATFieldUsage> MAP_GFU{
	{"Generic", GFU_Generic},
	{"PixelCount", GFU_PixelCount},
	{"Name", GFU_Name},
	{"Min", GFU_Min},
	{"Max", GFU_Max},
	{"MinMax", GFU_MinMax},
	{"Red", GFU_Red},
	{"Green", GFU_Green},
	{"Blue", GFU_Blue},
	{"Alpha", GFU_Alpha},
	{"RedMin", GFU_RedMin},
	{"GreenMin", GFU_GreenMin},
	{"BlueMin", GFU_BlueMin},
	{"AlphaMin", GFU_AlphaMin},
	{"RedMax", GFU_RedMax},
	{"GreenMax", GFU_GreenMax},
	{"BlueMax", GFU_BlueMax},
	{"AlphaMax", GFU_AlphaMax}
};
#endif

Rcpp::CharacterVector gdal_version();
void gdal_formats();
std::string get_config_option(std::string key);
void set_config_option(std::string key, std::string value);
int get_cache_used();

bool create(std::string format, std::string dst_filename,
		int xsize, int ysize, int nbands, std::string dataType,
		Rcpp::Nullable<Rcpp::CharacterVector> options);
bool createCopy(std::string format, std::string dst_filename,
		std::string src_filename, bool strict,
		Rcpp::Nullable<Rcpp::CharacterVector> options);
std::string _getCreationOptions(std::string format);
bool copyDatasetFiles(std::string new_filename, std::string old_filename,
		std::string format);
bool deleteDataset(std::string filename, std::string format);
bool renameDataset(std::string new_filename, std::string old_filename,
		std::string format);
bool bandCopyWholeRaster(std::string src_filename, int src_band,
		std::string dst_filename, int dst_band,
		Rcpp::Nullable<Rcpp::CharacterVector> options);

Rcpp::NumericVector _apply_geotransform(const std::vector<double> gt, 
		double pixel, double line);
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
		
Rcpp::DataFrame _value_count(std::string src_filename, int band);

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
	
	std::vector<double> getMinMax(int band, bool approx_ok) const;
	Rcpp::NumericVector getStatistics(int band,	bool approx_ok, 
			bool force) const;
	std::vector<double> getHistogram(int band, double min, double max,
			int num_buckets, bool incl_out_of_range, bool approx_ok) const;
	Rcpp::List getDefaultHistogram(int band, bool force) const;
	
	Rcpp::CharacterVector getMetadata(int band, std::string domain) const;
	std::string getMetadataItem(int band, std::string mdi_name, 
			std::string domain) const;
	void setMetadataItem(int band, std::string mdi_name, std::string mdi_value,
			std::string domain);
	Rcpp::CharacterVector getMetadataDomainList(int band) const;
	
	SEXP read(int band, int xoff, int yoff, int xsize, int ysize,
			int out_xsize, int out_ysize) const;
				
	void write(int band, int xoff, int yoff, int xsize, int ysize,
			Rcpp::RObject rasterData);

	void fillRaster(int band, double value, double ivalue);
	
	SEXP getColorTable(int band) const;
	std::string getPaletteInterp(int band) const;
	bool setColorTable(int band, Rcpp::RObject& col_tbl,
			std::string palette_interp);
	
	SEXP getDefaultRAT(int band) const;
	bool setDefaultRAT(int band, Rcpp::DataFrame& df);
	
	void flushCache();
	
	int getChecksum(int band, int xoff, int yoff, int xsize, int ysize) const;
	
	void close();
	
	// methods for internal use not exported to R
	void _checkAccess(GDALAccess access_needed) const;
	GDALRasterBandH _getBand(int band) const;
	bool _readableAsInt(int band) const;
};

RCPP_EXPOSED_CLASS(GDALRaster)

#endif
