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
int _gdal_version_num();
Rcpp::DataFrame gdal_formats(std::string fmt);
std::string get_config_option(std::string key);
void set_config_option(std::string key, std::string value);
int get_cache_used();
void push_error_handler(std::string handler);
void pop_error_handler();

Rcpp::CharacterVector _check_gdal_filename(Rcpp::CharacterVector filename);
int _get_physical_RAM();

bool create(std::string format, Rcpp::CharacterVector dst_filename,
            int xsize, int ysize, int nbands, std::string dataType,
            Rcpp::Nullable<Rcpp::CharacterVector> options);
bool createCopy(std::string format, Rcpp::CharacterVector dst_filename,
                Rcpp::CharacterVector src_filename, bool strict,
                Rcpp::Nullable<Rcpp::CharacterVector> options, bool quiet);
std::string _getCreationOptions(std::string format);
bool copyDatasetFiles(Rcpp::CharacterVector new_filename,
                      Rcpp::CharacterVector old_filename,
                      std::string format);
bool deleteDataset(Rcpp::CharacterVector filename, std::string format);
bool renameDataset(Rcpp::CharacterVector new_filename,
                   Rcpp::CharacterVector old_filename,
                   std::string format);
bool bandCopyWholeRaster(Rcpp::CharacterVector src_filename, int src_band,
                         Rcpp::CharacterVector dst_filename, int dst_band,
                         Rcpp::Nullable<Rcpp::CharacterVector> options,
                         bool quiet);
bool _addFileInZip(std::string zip_filename, bool overwrite,
                   std::string archive_filename, std::string in_filename,
                   Rcpp::Nullable<Rcpp::CharacterVector> options,
                   bool quiet);
int vsi_copy_file(Rcpp::CharacterVector src_file,
                  Rcpp::CharacterVector target_file,
                  bool show_progess);
void vsi_curl_clear_cache(bool partial, Rcpp::CharacterVector file_prefix);
Rcpp::CharacterVector vsi_read_dir(Rcpp::CharacterVector path,
                                   int max_files);
bool vsi_sync(Rcpp::CharacterVector src,
              Rcpp::CharacterVector target,
              bool show_progess,
              Rcpp::Nullable<Rcpp::CharacterVector> options);
int vsi_mkdir(Rcpp::CharacterVector path, int mode);
int vsi_rmdir(Rcpp::CharacterVector path);
int vsi_unlink(Rcpp::CharacterVector filename);
SEXP vsi_unlink_batch(Rcpp::CharacterVector filenames);
SEXP vsi_stat(Rcpp::CharacterVector filename, std::string info);
int vsi_rename(Rcpp::CharacterVector oldpath, Rcpp::CharacterVector newpath);
std::string _vsi_get_fs_options(Rcpp::CharacterVector filename);
Rcpp::CharacterVector vsi_get_fs_prefixes();
bool vsi_supports_seq_write(Rcpp::CharacterVector filename,
                            bool allow_local_tmpfile);
bool vsi_supports_rnd_write(Rcpp::CharacterVector filename,
                            bool allow_local_tmpfile);
double vsi_get_disk_free_space(Rcpp::CharacterVector path);

Rcpp::NumericVector _apply_geotransform(const std::vector<double> gt,
                                        double pixel, double line);
Rcpp::NumericVector inv_geotransform(const std::vector<double> gt);
Rcpp::IntegerMatrix get_pixel_line(const Rcpp::NumericMatrix xy,
                                   const std::vector<double> gt);

bool buildVRT(Rcpp::CharacterVector vrt_filename,
              Rcpp::CharacterVector input_rasters,
              Rcpp::Nullable<Rcpp::CharacterVector> cl_arg,
              bool quiet);

Rcpp::DataFrame _combine(Rcpp::CharacterVector src_files,
                         Rcpp::CharacterVector var_names,
                         std::vector<int> bands,
                         std::string dst_filename,
                         std::string fmt,
                         std::string dataType,
                         Rcpp::Nullable<Rcpp::CharacterVector> options,
                         bool quiet);

Rcpp::DataFrame _value_count(std::string src_filename, int band,
                             bool quiet);

bool _dem_proc(std::string mode,
               Rcpp::CharacterVector src_filename,
               Rcpp::CharacterVector dst_filename,
               Rcpp::Nullable<Rcpp::CharacterVector> cl_arg,
               Rcpp::Nullable<Rcpp::String> col_file,
               bool quiet);

bool fillNodata(Rcpp::CharacterVector filename, int band,
                Rcpp::CharacterVector mask_file,
                double max_dist, int smooth_iterations,
                bool quiet);

bool footprint(Rcpp::CharacterVector src_filename,
               Rcpp::CharacterVector dst_filename,
               Rcpp::Nullable<Rcpp::CharacterVector> cl_arg);

bool ogr2ogr(Rcpp::CharacterVector src_dsn,
             Rcpp::CharacterVector dst_dsn,
             Rcpp::Nullable<Rcpp::CharacterVector> src_layers,
             Rcpp::Nullable<Rcpp::CharacterVector> cl_arg);

std::string ogrinfo(Rcpp::CharacterVector dsn,
                    Rcpp::Nullable<Rcpp::CharacterVector> layers,
                    Rcpp::Nullable<Rcpp::CharacterVector> cl_arg,
                    Rcpp::Nullable<Rcpp::CharacterVector> open_options,
                    bool read_only,
                    bool cout);

bool _polygonize(Rcpp::CharacterVector src_filename, int src_band,
                 Rcpp::CharacterVector out_dsn,
                 std::string out_layer, std::string fld_name,
                 Rcpp::CharacterVector mask_file, bool nomask,
                 int connectedness, bool quiet);

bool _rasterize(std::string src_dsn, std::string dst_filename,
                Rcpp::CharacterVector cl_arg, bool quiet);

bool sieveFilter(Rcpp::CharacterVector src_filename, int src_band,
                 Rcpp::CharacterVector dst_filename, int dst_band,
                 int size_threshold, int connectedness,
                 Rcpp::CharacterVector mask_filename , int mask_band,
                 Rcpp::Nullable<Rcpp::CharacterVector> options, bool quiet);

bool translate(Rcpp::CharacterVector src_filename,
               Rcpp::CharacterVector dst_filename,
               Rcpp::Nullable<Rcpp::CharacterVector> cl_arg,
               bool quiet);

bool warp(Rcpp::CharacterVector src_files,
          Rcpp::CharacterVector dst_filename,
          std::string t_srs,
          Rcpp::Nullable<Rcpp::CharacterVector> cl_arg,
          bool quiet);

Rcpp::IntegerMatrix createColorRamp(int start_index,
                                    Rcpp::IntegerVector start_color,
                                    int end_index,
                                    Rcpp::IntegerVector end_color,
                                    std::string palette_interp);

class GDALRaster {

    private:
    std::string fname_in;
    Rcpp::CharacterVector open_options_in;
    GDALDatasetH  hDataset;
    GDALAccess eAccess;

    public:
    GDALRaster();
    GDALRaster(Rcpp::CharacterVector filename);
    GDALRaster(Rcpp::CharacterVector filename, bool read_only);
    GDALRaster(Rcpp::CharacterVector filename, bool read_only,
               Rcpp::CharacterVector open_options);

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
    void clearStatistics();
    std::vector<double> getHistogram(int band, double min, double max,
                                     int num_buckets, bool incl_out_of_range,
                                     bool approx_ok) const;
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
    bool _hasInt64() const;
    void _warnInt64() const;
};

RCPP_EXPOSED_CLASS(GDALRaster)

#endif
