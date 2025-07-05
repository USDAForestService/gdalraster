/* R interface to the GDAL C API for raster
   https://gdal.org/en/stable/api/raster_c_api.html

   Chris Toney <chris.toney at usda.gov>
   Copyright (c) 2023-2025 gdalraster authors
*/

#ifndef SRC_GDALRASTER_H_
#define SRC_GDALRASTER_H_

#include <limits>
#include <map>
#include <string>
#include <vector>

#include "rcpp_util.h"

#ifndef SRC_GDALRASTER_TYPES_H_
#include <cpl_port.h>
int CPL_DLL CPL_STDCALL GDALTermProgressR(double, const char *, void *);
#endif

// Predeclare some GDAL types until the public header is included
#ifndef GDAL_H_INCLUDED
typedef void *GDALDatasetH;
typedef void *GDALRasterBandH;
typedef enum {GA_ReadOnly = 0, GA_Update = 1} GDALAccess;
#endif

// The function ARE_REAL_EQUAL was copied from gcore/gdal_priv.h since we
// do not need that header otherwise
// Copyright (c) 1998, Frank Warmerdam
// Copyright (c) 2007-2014, Even Rouault <even dot rouault at spatialys.com>
// License: MIT
//
// Behavior is undefined if fVal1 or fVal2 are NaN (should be tested before
// calling this function)
template <class T> inline bool ARE_REAL_EQUAL(T fVal1, T fVal2, int ulp = 2)
{
    return fVal1 == fVal2 || /* Should cover infinity */
           std::abs(fVal1 - fVal2) < std::numeric_limits<float>::epsilon() *
                                         std::abs(fVal1 + fVal2) * ulp;
}

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

class GDALRaster {
 public:
    GDALRaster();
    explicit GDALRaster(const Rcpp::CharacterVector &filename);
    GDALRaster(const Rcpp::CharacterVector &filename, bool read_only);
    GDALRaster(const Rcpp::CharacterVector &filename, bool read_only,
               const Rcpp::CharacterVector &open_options);
    GDALRaster(const Rcpp::CharacterVector &filename,
               bool read_only,
               const Rcpp::Nullable<Rcpp::CharacterVector> &open_options,
               bool shared);
    ~GDALRaster();

    // read/write fields exposed to R
    Rcpp::CharacterVector infoOptions = Rcpp::CharacterVector::create();
    bool quiet = false;
    bool readByteAsRaw = false;

    // methods exported to R
    std::string getFilename() const;
    void setFilename(const std::string &filename);
    void open(bool read_only);
    bool isOpen() const;
    Rcpp::CharacterVector getFileList() const;

    void info() const;
    Rcpp::String infoAsJSON() const;

    std::string getDriverShortName() const;
    std::string getDriverLongName() const;

    double getRasterXSize() const;
    double getRasterYSize() const;
    std::vector<double> getGeoTransform() const;
    bool setGeoTransform(std::vector<double> transform);
    int getRasterCount() const;
    bool addBand(const std::string &dataType,
                 const Rcpp::Nullable<Rcpp::CharacterVector> &options);

    std::string getProjection() const;
    std::string getProjectionRef() const;
    bool setProjection(const std::string &projection);

    std::vector<double> bbox() const;
    std::vector<double> res() const;
    std::vector<double> dim() const;
    Rcpp::NumericMatrix apply_geotransform(const Rcpp::RObject &col_row) const;
    Rcpp::IntegerMatrix get_pixel_line(const Rcpp::RObject &xy) const;
    Rcpp::NumericMatrix pixel_extract(const Rcpp::RObject &xy,
                                      const Rcpp::IntegerVector &bands,
                                      const std::string &interp_method,
                                      int krnl_dim,
                                      const std::string &xy_srs) const;

    Rcpp::NumericMatrix get_block_indexing(int band) const;
    std::vector<int> getBlockSize(int band) const;
    std::vector<int> getActualBlockSize(int band, int xblockoff,
                                        int yblockoff) const;
    int getOverviewCount(int band) const;
    void buildOverviews(const std::string &resampling, std::vector<int> levels,
                        std::vector<int> bands);
    std::string getDataTypeName(int band) const;
    bool hasNoDataValue(int band) const;
    double getNoDataValue(int band) const;
    bool setNoDataValue(int band, double nodata_value);
    void deleteNoDataValue(int band);
    Rcpp::List getMaskFlags(int band) const;
    Rcpp::List getMaskBand(int band) const;
    std::string getUnitType(int band) const;
    bool setUnitType(int band, const std::string &unit_type);
    bool hasScale(int band) const;
    double getScale(int band) const;
    bool setScale(int band, double scale);
    bool hasOffset(int band) const;
    double getOffset(int band) const;
    bool setOffset(int band, double offset);
    std::string getDescription(int band) const;
    void setDescription(int band, const std::string &desc);
    std::string getRasterColorInterp(int band) const;
    void setRasterColorInterp(int band, const std::string &col_interp);

    std::vector<double> getMinMax(int band, bool approx_ok) const;
    Rcpp::NumericVector getStatistics(int band, bool approx_ok,
                                      bool force) const;
    void clearStatistics();
    std::vector<double> getHistogram(int band, double min, double max,
                                     int num_buckets, bool incl_out_of_range,
                                     bool approx_ok) const;
    Rcpp::List getDefaultHistogram(int band, bool force) const;

    Rcpp::CharacterVector getMetadata(int band,
                                      const std::string &domain) const;
    bool setMetadata(int band, const Rcpp::CharacterVector &metadata,
                     const std::string &domain);
    std::string getMetadataItem(int band, const std::string &mdi_name,
                                const std::string &domain) const;
    bool setMetadataItem(int band, const std::string &mdi_name,
                         const std::string &mdi_value,
                         const std::string &domain);
    Rcpp::CharacterVector getMetadataDomainList(int band) const;

    SEXP read(int band, int xoff, int yoff, int xsize, int ysize,
              int out_xsize, int out_ysize) const;

    void write(int band, int xoff, int yoff, int xsize, int ysize,
               const Rcpp::RObject &rasterData);

    void fillRaster(int band, double value, double ivalue);

    SEXP getColorTable(int band) const;
    std::string getPaletteInterp(int band) const;
    bool setColorTable(int band, const Rcpp::RObject &col_tbl,
                       const std::string &palette_interp);
    bool clearColorTable(int band);

    SEXP getDefaultRAT(int band) const;
    bool setDefaultRAT(int band, const Rcpp::DataFrame &df);

    void flushCache();

    int getChecksum(int band, int xoff, int yoff, int xsize, int ysize) const;

    void close();

    void show() const;

    // methods for internal use not exported to R
    void checkAccess_(GDALAccess access_needed) const;
    GDALRasterBandH getBand_(int band) const;
    bool readableAsInt_(int band) const;
    bool hasInt64_() const;
    void warnInt64_() const;
    GDALDatasetH getGDALDatasetH_() const;
    void setGDALDatasetH_(GDALDatasetH hDs, bool with_update);

 private:
    std::string m_fname {};
    Rcpp::CharacterVector m_open_options{};
    bool m_shared {true};
    GDALDatasetH m_hDataset {nullptr};
    GDALAccess m_eAccess {GA_ReadOnly};
};

// cppcheck-suppress unknownMacro
RCPP_EXPOSED_CLASS(GDALRaster)

Rcpp::CharacterVector gdal_version();
int gdal_version_num();
Rcpp::DataFrame gdal_formats(const std::string &fmt);
std::string get_config_option(const std::string &key);
void set_config_option(const std::string &key, const std::string &value);
Rcpp::NumericVector get_cache_max(std::string units);
Rcpp::NumericVector get_cache_used(std::string units);
void set_cache_max(Rcpp::NumericVector nbytes);
int dump_open_datasets(const std::string &outfile);
int get_num_cpus();
Rcpp::NumericVector get_usable_physical_ram();
void push_error_handler(const std::string &handler);
void pop_error_handler();
bool has_spatialite();
bool http_enabled();
void cpl_http_cleanup();
std::string cpl_get_filename(const Rcpp::CharacterVector &full_filename);
std::string cpl_get_basename(const Rcpp::CharacterVector &full_filename);
std::string cpl_get_extension(const Rcpp::CharacterVector &full_filename);

Rcpp::CharacterVector check_gdal_filename(
        const Rcpp::CharacterVector &filename);

GDALRaster *create(const std::string &format,
                   const Rcpp::CharacterVector &dst_filename,
                   int xsize, int ysize, int nbands,
                   const std::string &dataType,
                   const Rcpp::Nullable<Rcpp::CharacterVector> &options);

GDALRaster *createCopy(const std::string &format,
                       const Rcpp::CharacterVector &dst_filename,
                       const GDALRaster* const &src_ds, bool strict,
                       const Rcpp::Nullable<Rcpp::CharacterVector> &options,
                       bool quiet);

std::string getCreationOptions(const std::string &format);
bool validateCreationOptions(const std::string &format,
                             const Rcpp::CharacterVector &options);

bool copyDatasetFiles(const Rcpp::CharacterVector &new_filename,
                      const Rcpp::CharacterVector &old_filename,
                      const std::string &format);

bool deleteDataset(const Rcpp::CharacterVector &filename,
                   const std::string &format);

bool renameDataset(const Rcpp::CharacterVector &new_filename,
                   const Rcpp::CharacterVector &old_filename,
                   const std::string &format);

SEXP identifyDriver(const Rcpp::CharacterVector &filename,
                    bool raster, bool vector,
                    const Rcpp::Nullable<Rcpp::CharacterVector>
                        &allowed_drivers,
                    const Rcpp::Nullable<Rcpp::CharacterVector> &file_list);

bool bandCopyWholeRaster(const Rcpp::CharacterVector &src_filename,
                         int src_band,
                         const Rcpp::CharacterVector &dst_filename,
                         int dst_band,
                         const Rcpp::Nullable<Rcpp::CharacterVector> &options,
                         bool quiet);

bool addFileInZip(const std::string &zip_filename, bool overwrite,
                  const std::string &archive_filename,
                  const std::string &in_filename,
                  const Rcpp::Nullable<Rcpp::CharacterVector> &options,
                  bool quiet);

Rcpp::NumericVector apply_geotransform_(const std::vector<double> &gt,
                                        double pixel, double line);

Rcpp::NumericMatrix apply_geotransform_gt(const Rcpp::RObject &col_row,
                                          const std::vector<double> &gt);

Rcpp::NumericMatrix apply_geotransform_ds(const Rcpp::RObject &col_row,
                                          const GDALRaster* const &ds);

Rcpp::NumericVector inv_geotransform(const std::vector<double> &gt);

Rcpp::IntegerMatrix get_pixel_line_gt(const Rcpp::RObject &xy,
                                      const std::vector<double> &gt);

Rcpp::IntegerMatrix get_pixel_line_ds(const Rcpp::RObject &xy,
                                      const GDALRaster* const &ds);

std::vector<double> bbox_grid_to_geo_(const std::vector<double> &gt,
                                      double grid_xmin, double grid_xmax,
                                      double grid_ymin, double grid_ymax);

Rcpp::NumericVector flip_vertical(const Rcpp::NumericVector &v,
                                  int xsize, int ysize, int nbands);

GDALRaster *autoCreateWarpedVRT(const GDALRaster* const &src_ds,
                                const std::string &dst_wkt,
                                const std::string &resample_alg,
                                const std::string &src_wkt,
                                double max_err, bool alpha_band,
                                bool reserved1, bool reserved2);

bool buildVRT(const Rcpp::CharacterVector &vrt_filename,
              const Rcpp::CharacterVector &input_rasters,
              const Rcpp::Nullable<Rcpp::CharacterVector> &cl_arg,
              bool quiet);

Rcpp::DataFrame combine(const Rcpp::CharacterVector &src_files,
                        const Rcpp::CharacterVector &var_names,
                        const std::vector<int> &bands,
                        const std::string &dst_filename,
                        const std::string &fmt,
                        const std::string &dataType,
                        const Rcpp::Nullable<Rcpp::CharacterVector> &options,
                        bool quiet);

Rcpp::DataFrame value_count(const GDALRaster* const &src_ds, int band,
                            bool quiet);

bool dem_proc(const std::string &mode,
              const Rcpp::CharacterVector &src_filename,
              const Rcpp::CharacterVector &dst_filename,
              const Rcpp::Nullable<Rcpp::CharacterVector> &cl_arg,
              const Rcpp::Nullable<Rcpp::String> &col_file,
              bool quiet);

bool fillNodata(const Rcpp::CharacterVector &filename, int band,
                const Rcpp::CharacterVector &mask_file,
                double max_dist, int smooth_iterations,
                bool quiet);

bool footprint(const Rcpp::CharacterVector &src_filename,
               const Rcpp::CharacterVector &dst_filename,
               const Rcpp::Nullable<Rcpp::CharacterVector> &cl_arg);

bool ogr2ogr(const Rcpp::CharacterVector &src_dsn,
             const Rcpp::CharacterVector &dst_dsn,
             const Rcpp::Nullable<Rcpp::CharacterVector> &src_layers,
             const Rcpp::Nullable<Rcpp::CharacterVector> &cl_arg,
             const Rcpp::Nullable<Rcpp::CharacterVector> &open_options);

Rcpp::String ogrinfo(const Rcpp::CharacterVector &dsn,
                     const Rcpp::Nullable<Rcpp::CharacterVector> &layers,
                     const Rcpp::Nullable<Rcpp::CharacterVector> &cl_arg,
                     const Rcpp::Nullable<Rcpp::CharacterVector> &open_options,
                     bool read_only,
                     bool cout);

bool polygonize(const Rcpp::CharacterVector &src_filename, int src_band,
                const Rcpp::CharacterVector &out_dsn,
                const std::string &out_layer, const std::string &fld_name,
                const Rcpp::CharacterVector &mask_file, bool nomask,
                int connectedness, bool quiet);

bool rasterize(const std::string &src_dsn, const std::string &dst_filename,
               Rcpp::List dst_dataset, const Rcpp::CharacterVector &cl_arg,
               bool quiet);

bool sieveFilter(const Rcpp::CharacterVector &src_filename, int src_band,
                 const Rcpp::CharacterVector &dst_filename, int dst_band,
                 int size_threshold, int connectedness,
                 const Rcpp::CharacterVector &mask_filename , int mask_band,
                 const Rcpp::Nullable<Rcpp::CharacterVector> &options,
                 bool quiet);

bool translate(const GDALRaster* const &ds,
               const Rcpp::CharacterVector &dst_filename,
               const Rcpp::Nullable<Rcpp::CharacterVector> &cl_arg,
               bool quiet);

bool warp(const Rcpp::List& src_datasets,
          const Rcpp::CharacterVector &dst_filename,
          Rcpp::List dst_dataset,
          const std::string &t_srs,
          const Rcpp::Nullable<Rcpp::CharacterVector> &cl_arg,
          bool quiet);

Rcpp::IntegerMatrix createColorRamp(int start_index,
                                    const Rcpp::IntegerVector &start_color,
                                    int end_index,
                                    const Rcpp::IntegerVector &end_color,
                                    const std::string &palette_interp);

#endif  // SRC_GDALRASTER_H_
