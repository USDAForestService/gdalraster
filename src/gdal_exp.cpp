/* Exported stand-alone functions for gdalraster
   Chris Toney <chris.toney at usda.gov> */

#include <cmath>
#include <unordered_map>

#include "gdal.h"
#include "cpl_port.h"
#include "cpl_conv.h"
#include "cpl_string.h"
#include "cpl_vsi.h"
#include "gdal_alg.h"
#include "gdal_utils.h"

#include <errno.h>

#include "gdalraster.h"
#include "cmb_table.h"
#include "ogr_util.h"

//' Get GDAL version
//'
//' `gdal_version()` returns runtime version information.
//'
//' @returns Character vector of length four containing:
//'   * "–version" - one line version message, e.g., “GDAL 3.6.3, released
//'   2023/03/12”
//'   * "GDAL_VERSION_NUM" - formatted as a string, e.g., “3060300” for
//'   GDAL 3.6.3.0
//'   * "GDAL_RELEASE_DATE" - formatted as a string, e.g., “20230312”
//'   * "GDAL_RELEASE_NAME" - e.g., “3.6.3”
//' @examples
//' gdal_version()
// [[Rcpp::export]]
Rcpp::CharacterVector gdal_version() {
    Rcpp::CharacterVector ret(4);
    ret(0) = GDALVersionInfo("-version");
    ret(1) = GDALVersionInfo("VERSION_NUM");
    ret(2) = GDALVersionInfo("RELEASE_DATE");
    ret(3) = GDALVersionInfo("RELEASE_NAME");
    return ret;
}


//' @noRd
// [[Rcpp::export(name = ".gdal_version_num")]]
int _gdal_version_num() {
    std::string version(GDALVersionInfo("VERSION_NUM"));
    return std::stoi(version);
}


//' Retrieve information on GDAL format drivers for raster and vector
//'
//' `gdal_formats()` returns a table of the supported raster and vector
//' formats, with information about the capabilities of each format driver.
//'
//' @param format A character string containing a driver short name. By default,
//' information for all configured raster and vector format drivers will be
//' returned.
//' @returns A data frame containing the format short name, long name, raster
//' (logical), vector (logical), read/write flag (`ro` is read-only,
//' `w` supports CreateCopy, `w+` supports Create), virtual I/O supported
//' (logical), and subdatasets (logical).
//'
//' @note
//' Virtual I/O refers to operations on GDAL Virtual File Systems. See
//' \url{https://gdal.org/user/virtual_file_systems.html#virtual-file-systems}.
//'
//' @examples
//' nrow(gdal_formats())
//' head(gdal_formats())
//'
//' gdal_formats("GPKG")
// [[Rcpp::export]]
Rcpp::DataFrame gdal_formats(std::string format = "") {

    Rcpp::CharacterVector short_name = Rcpp::CharacterVector::create();
    Rcpp::CharacterVector long_name = Rcpp::CharacterVector::create();
    Rcpp::LogicalVector raster_fmt = Rcpp::LogicalVector::create();
    Rcpp::LogicalVector vector_fmt = Rcpp::LogicalVector::create();
    Rcpp::CharacterVector rw_flag = Rcpp::CharacterVector::create();
    Rcpp::LogicalVector virtual_io = Rcpp::LogicalVector::create();
    Rcpp::LogicalVector subdatasets = Rcpp::LogicalVector::create();

    for (int i=0; i < GDALGetDriverCount(); ++i) {
        GDALDriverH hDriver = GDALGetDriver(i);
        char **papszMD = GDALGetMetadata(hDriver, nullptr);
        std::string rw = "";

        if (format != "" &&
                !EQUAL(format.c_str(), GDALGetDriverShortName(hDriver)))
            continue;

        if (CPLFetchBool(papszMD, GDAL_DCAP_RASTER, false) ||
                CPLFetchBool(papszMD, GDAL_DCAP_VECTOR, false)) {

            CPLFetchBool(papszMD, GDAL_DCAP_RASTER, false) ?
                    raster_fmt.push_back(true) : raster_fmt.push_back(false);

            CPLFetchBool(papszMD, GDAL_DCAP_VECTOR, false) ?
                    vector_fmt.push_back(true) : vector_fmt.push_back(false);

        }
        else {
            continue;
        }

        if (CPLFetchBool(papszMD, GDAL_DCAP_OPEN, false))
            rw += "r";
        if (CPLFetchBool(papszMD, GDAL_DCAP_CREATE, false))
            rw += "w+";
        else if (CPLFetchBool(papszMD, GDAL_DCAP_CREATECOPY, false))
            rw += "w";
        else
            rw += "o";
        rw_flag.push_back(rw);

        CPLFetchBool(papszMD, GDAL_DCAP_VIRTUALIO, false) ?
                virtual_io.push_back(true) : virtual_io.push_back(false);

        CPLFetchBool(papszMD, GDAL_DMD_SUBDATASETS, false) ?
                subdatasets.push_back(true) : subdatasets.push_back(false);

        short_name.push_back(GDALGetDriverShortName(hDriver));
        long_name.push_back(GDALGetDriverLongName(hDriver));
    }

    Rcpp::DataFrame df_out = Rcpp::DataFrame::create();
    df_out.push_back(short_name, "short_name");
    df_out.push_back(raster_fmt, "raster");
    df_out.push_back(vector_fmt, "vector");
    df_out.push_back(rw_flag, "rw_flag");
    df_out.push_back(virtual_io, "virtual_io");
    df_out.push_back(subdatasets, "subdatasets");
    df_out.push_back(long_name, "long_name");

    return df_out;
}


//' Get GDAL configuration option
//'
//' `get_config_option()` gets the value of GDAL runtime configuration option.
//' Configuration options are essentially global variables the user can set.
//' They are used to alter the default behavior of certain raster format
//' drivers, and in some cases the GDAL core. For a full description and
//' listing of available options see
//' \url{https://gdal.org/user/configoptions.html}.
//'
//' @param key Character name of a configuration option.
//' @returns Character. The value of a (key, value) option previously set with
//' `set_config_option()`. An empty string (`""`) is returned if `key` is not
//' found.
//'
//' @seealso
//' [set_config_option()]
//'
//' `vignette("gdal-config-quick-ref")`
//'
//' @examples
//' ## this option is set during initialization of the gdalraster package
//' get_config_option("OGR_CT_FORCE_TRADITIONAL_GIS_ORDER")
// [[Rcpp::export]]
std::string get_config_option(std::string key) {
    const char* default_ = "";
    std::string ret(CPLGetConfigOption(key.c_str(), default_));
    return ret;
}


//' Set GDAL configuration option
//'
//' `set_config_option()` sets a GDAL runtime configuration option.
//' Configuration options are essentially global variables the user can set.
//' They are used to alter the default behavior of certain raster format
//' drivers, and in some cases the GDAL core. For a full description and
//' listing of available options see
//' \url{https://gdal.org/user/configoptions.html}.
//'
//' @param key Character name of a configuration option.
//' @param value Character value to set for the option.
//' `value = ""` (empty string) will unset a value previously set by
//' `set_config_option()`.
//' @returns No return value, called for side effects.
//'
//' @seealso
//' [get_config_option()]
//'
//' `vignette("gdal-config-quick-ref")`
//'
//' @examples
//' set_config_option("GDAL_CACHEMAX", "10%")
//' get_config_option("GDAL_CACHEMAX")
//' ## unset:
//' set_config_option("GDAL_CACHEMAX", "")
// [[Rcpp::export]]
void set_config_option(std::string key, std::string value) {
    const char* value_ = nullptr;
    if (value != "")
        value_ = value.c_str();

    CPLSetConfigOption(key.c_str(), value_);
}


//' Get the size of memory in use by the GDAL block cache
//'
//' `get_cache_used()` returns the amount of memory currently in use for
//' GDAL block caching. This a wrapper for `GDALGetCacheUsed64()` with return
//' value as MB.
//'
//' @returns Integer. Amount of cache memory in use in MB.
//'
//' @seealso
//' [GDAL Block Cache](https://usdaforestservice.github.io/gdalraster/articles/gdal-block-cache.html)
//'
//' @examples
//' get_cache_used()
// [[Rcpp::export]]
int get_cache_used() {
    GIntBig nCacheUsed = GDALGetCacheUsed64();
    return static_cast<int>(nCacheUsed / (1000 * 1000));
}


//' Push a new GDAL CPLError handler
//'
//' `push_error_handler()` is a wrapper for
//' `CPLPushErrorHandler()` in the GDAL Common Portability
//' Library.
//' This pushes a new error handler on the thread-local error handler stack.
//' This handler will be used until removed with `pop_error_handler()`.
//' A typical use is to temporarily set `CPLQuietErrorHandler()` which doesn't
//' make any attempt to report passed error or warning messages, but will
//' process debug messages via `CPLDefaultErrorHandler`.
//'
//' @param handler Character name of the error handler to push.
//' One of `quiet`, `logging` or `default`.
//' @returns No return value, called for side effects.
//'
//' @note
//' Setting `handler = "logging"` will use `CPLLoggingErrorHandler()`, error
//' handler that logs into the file defined by the `CPL_LOG` configuration
//' option, or `stderr` otherwise.
//'
//' This only affects error reporting from GDAL.
//'
//' @seealso
//' [pop_error_handler()]
//'
//' @examples
//' push_error_handler("quiet")
//' # ...
//' pop_error_handler()
// [[Rcpp::export]]
void push_error_handler(std::string handler) {
    if (EQUAL(handler.c_str(), "quiet"))
        CPLPushErrorHandler(CPLQuietErrorHandler);
    else if (EQUAL(handler.c_str(), "logging"))
        CPLPushErrorHandler(CPLLoggingErrorHandler);
    else if (EQUAL(handler.c_str(), "default"))
        CPLPushErrorHandler(CPLDefaultErrorHandler);
}


//' Pop error handler off stack
//'
//' `pop_error_handler()` is a wrapper for `CPLPopErrorHandler()` in the GDAL
//' Common Portability Library.
//' Discards the current error handler on the error handler stack, and restores
//' the one in use before the last `push_error_handler()` call. This method has
//' no effect if there are no error handlers on the current thread's error
//' handler stack.
//'
//' @returns No return value, called for side effects.
//'
//' @seealso
//' [push_error_handler()]
//'
//' @examples
//' push_error_handler("quiet")
//' # ...
//' pop_error_handler()
// [[Rcpp::export]]
void pop_error_handler() {
    CPLPopErrorHandler();
}



//' Check a filename before passing to GDAL and potentially fix.
//' filename may be a physical file, URL, connection string, file name with
//' additional parameters, etc. Returned in UTF-8 encoding.
//'
//' @noRd
// [[Rcpp::export(name = ".check_gdal_filename")]]
Rcpp::CharacterVector _check_gdal_filename(Rcpp::CharacterVector filename) {
    /*
    Rcpp::CharacterVector may have marked encoding when needed.
    Rcpp::String may drop encoding or have issues with encoding on Windows:
    https://github.com/RcppCore/Rcpp/issues/988
    */

    if (filename.size() > 1)
        Rcpp::stop("'filename' must be a character vector of length 1");

    std::string std_filename(filename[0]);
    Rcpp::CharacterVector out_filename(1);

    if (std_filename.find("/vsi") == 0) {
        out_filename[0] = filename[0];
    }
    else if (std_filename.find("~") != std_filename.npos) {
    /* 	This does not catch an error from normalizePath() in R. But we're
        not using mustWork=TRUE, so only a warning is emitted if path does
        not exist. Leaving as try/catch for now. */
        try {
            out_filename = _normalize_path(filename);
        }
        catch (...) {
            out_filename[0] = filename[0];
        }
    }
    else {
        out_filename[0] = filename[0];
    }

    return _enc_to_utf8(out_filename);
}


//' Get usable physical RAM in MB
//'
//' @noRd
// [[Rcpp::export(name = ".get_physical_RAM")]]
int _get_physical_RAM() {
    GIntBig nPhysicalRAM = CPLGetUsablePhysicalRAM();
    return static_cast<int>(nPhysicalRAM / (1000 * 1000));
}


//' Create a new uninitialized raster
//'
//' `create()` makes an empty raster in the specified format.
//'
//' @param format Raster format short name (e.g., "GTiff").
//' @param dst_filename Filename to create.
//' @param xsize Integer width of raster in pixels.
//' @param ysize Integer height of raster in pixels.
//' @param nbands Integer number of bands.
//' @param dataType Character data type name.
//' (e.g., common data types include Byte, Int16, UInt16, Int32, Float32).
//' @param options Optional list of format-specific creation options in a
//' vector of `"NAME=VALUE"` pairs
//' (e.g., \code{options = c("COMPRESS=LZW")} to set LZW
//' compression during creation of a GTiff file).
//' The APPEND_SUBDATASET=YES option can be
//' specified to avoid prior destruction of existing dataset.
//' @returns Logical indicating success (invisible \code{TRUE}).
//' An error is raised if the operation fails.
//' @seealso
//' [`GDALRaster-class`][GDALRaster], [createCopy()], [rasterFromRaster()],
//' [getCreationOptions()]
//' @examples
//' new_file <- paste0(tempdir(), "/", "newdata.tif")
//' create(format="GTiff", dst_filename=new_file, xsize=143, ysize=107,
//'        nbands=1, dataType="Int16")
//' ds <- new(GDALRaster, new_file, read_only=FALSE)
//' ## EPSG:26912 - NAD83 / UTM zone 12N
//' ds$setProjection(epsg_to_wkt(26912))
//' gt <- c(323476.1, 30, 0, 5105082.0, 0, -30)
//' ds$setGeoTransform(gt)
//' ds$setNoDataValue(band = 1, -9999)
//' ds$fillRaster(band = 1, -9999, 0)
//' ## ...
//' ## close the dataset when done
//' ds$close()
//'
//' deleteDataset(new_file)
// [[Rcpp::export(invisible = true)]]
bool create(std::string format, Rcpp::CharacterVector dst_filename,
        int xsize, int ysize, int nbands, std::string dataType,
        Rcpp::Nullable<Rcpp::CharacterVector> options = R_NilValue) {

    GDALDriverH hDriver = GDALGetDriverByName( format.c_str() );
    if (hDriver == nullptr)
        Rcpp::stop("failed to get driver for the specified format");

    char **papszMetadata = GDALGetMetadata(hDriver, nullptr);
    if (!CPLFetchBool(papszMetadata, GDAL_DCAP_CREATE, FALSE))
        Rcpp::stop("driver does not support create");

    std::string dst_filename_in;
    dst_filename_in = Rcpp::as<std::string>(_check_gdal_filename(dst_filename));

    GDALDataType dt = GDALGetDataTypeByName(dataType.c_str() );

    std::vector<char *> opt_list = {nullptr};
    if (options.isNotNull()) {
        Rcpp::CharacterVector options_in(options);
        opt_list.resize(options_in.size() + 1);
        for (R_xlen_t i = 0; i < options_in.size(); ++i) {
            opt_list[i] = (char *) options_in[i];
        }
        opt_list[options_in.size()] = nullptr;
    }

    GDALDatasetH hDstDS = nullptr;
    hDstDS = GDALCreate(hDriver, dst_filename_in.c_str(),
                        xsize, ysize, nbands, dt,
                        opt_list.data());

    if (hDstDS == nullptr)
        Rcpp::stop("create() failed");

    GDALClose(hDstDS);
    return true;
}


//' Create a copy of a raster
//'
//' `createCopy()` copies a raster dataset, optionally changing the format.
//' The extent, cell size, number of bands, data type, projection, and
//' geotransform are all copied from the source raster.
//'
//' @param format Format short name for the output raster
//' (e.g., "GTiff" or "HFA").
//' @param dst_filename Filename to create.
//' @param src_filename Filename of source raster.
//' @param strict Logical. TRUE if the copy must be strictly equivalent,
//' or more normally FALSE indicating that the copy may adapt as needed for
//' the output format.
//' @param options Optional list of format-specific creation options in a
//' vector of `"NAME=VALUE"` pairs
//' (e.g., \code{options = c("COMPRESS=LZW")} to set \code{LZW}
//' compression during creation of a GTiff file).
//' The APPEND_SUBDATASET=YES option can be
//' specified to avoid prior destruction of existing dataset.
//' @param quiet Logical scalar. If `TRUE`, a progress bar will be not be
//' displayed. Defaults to `FALSE`.
//' @returns Logical indicating success (invisible \code{TRUE}).
//' An error is raised if the operation fails.
//' @seealso
//' [`GDALRaster-class`][GDALRaster], [create()], [rasterFromRaster()],
//' [getCreationOptions()], [translate()]
//' @examples
//' lcp_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
//' tif_file <- paste0(tempdir(), "/", "storml_lndscp.tif")
//' opt <- c("COMPRESS=LZW")
//' createCopy(format="GTiff", dst_filename=tif_file, src_filename=lcp_file,
//'            options=opt)
//' file.size(lcp_file)
//' file.size(tif_file)
//' ds <- new(GDALRaster, tif_file, read_only=FALSE)
//' ds$getMetadata(band=0, domain="IMAGE_STRUCTURE")
//' for (band in 1:ds$getRasterCount())
//'     ds$setNoDataValue(band, -9999)
//' ds$getStatistics(band=1, approx_ok=FALSE, force=TRUE)
//' ds$close()
//'
//' deleteDataset(tif_file)
// [[Rcpp::export(invisible = true)]]
bool createCopy(std::string format, Rcpp::CharacterVector dst_filename,
        Rcpp::CharacterVector src_filename, bool strict = false,
        Rcpp::Nullable<Rcpp::CharacterVector> options = R_NilValue,
        bool quiet = false) {

    GDALDriverH hDriver = GDALGetDriverByName(format.c_str());
    if (hDriver == nullptr)
        Rcpp::stop("failed to get driver from format name");

    char **papszMetadata = GDALGetMetadata(hDriver, nullptr);
    if (!CPLFetchBool(papszMetadata, GDAL_DCAP_CREATECOPY, FALSE))
        Rcpp::stop("driver does not support createCopy");

    std::string src_filename_in;
    src_filename_in = Rcpp::as<std::string>(_check_gdal_filename(src_filename));
    std::string dst_filename_in;
    dst_filename_in = Rcpp::as<std::string>(_check_gdal_filename(dst_filename));

    GDALDatasetH hSrcDS = GDALOpenShared(src_filename_in.c_str(), GA_ReadOnly);
    if(hSrcDS == nullptr)
        Rcpp::stop("open source raster failed");

    std::vector<char *> opt_list = {nullptr};
    if (options.isNotNull()) {
        Rcpp::CharacterVector options_in(options);
        opt_list.resize(options_in.size() + 1);
        for (R_xlen_t i = 0; i < options_in.size(); ++i) {
            opt_list[i] = (char *) options_in[i];
        }
        opt_list[options_in.size()] = nullptr;
    }

    GDALDatasetH hDstDS = nullptr;
    hDstDS = GDALCreateCopy(hDriver, dst_filename_in.c_str(), hSrcDS, strict,
                            opt_list.data(),
                            quiet ? nullptr : GDALTermProgressR, nullptr);

    GDALClose(hSrcDS);
    if (hDstDS == nullptr)
        Rcpp::stop("createCopy() failed");

    GDALClose(hDstDS);
    return true;
}


//' Apply geotransform
//'
//' `_apply_geotransform()` applies geotransform coefficients to a raster
//' coordinate in pixel/line space (colum/row), converting into a
//' georeferenced (x, y) coordinate.
//'
//' @param gt Numeric vector of length six containing the geotransform to
//' apply.
//' @param pixel Numeric scalar. A raster pixel (column) coordinate.
//' @param line Numeric scalar. A raster line (row) coordinate.
//' @returns Numeric vector of length two containing a geospatial x/y
//' coordinate (spatial reference system of `gt`).
//' @seealso [`GDALRaster$getGeoTransform()`][GDALRaster], [get_pixel_line()],
//' [inv_geotransform()]
//' @noRd
// [[Rcpp::export(name = ".apply_geotransform")]]
Rcpp::NumericVector _apply_geotransform(const std::vector<double> gt,
        double pixel, double line) {

    double geo_x, geo_y;
    GDALApplyGeoTransform((double *) (gt.data()), pixel, line, &geo_x, &geo_y);
    Rcpp::NumericVector geo_xy = {geo_x, geo_y};
    return geo_xy;
}


//' Invert geotransform
//'
//' `inv_geotransform()` inverts a vector of geotransform coefficients. This
//' converts the equation from being:\cr
//' raster pixel/line (column/row) -> geospatial x/y coordinate\cr
//' to:\cr
//' geospatial x/y coordinate -> raster pixel/line (column/row)
//'
//' @param gt Numeric vector of length six containing the geotransform to
//' invert.
//' @returns Numeric vector of length six containing the inverted
//' geotransform. The output vector will contain NAs if the input geotransform
//' is uninvertable.
//' @seealso [`GDALRaster$getGeoTransform()`][GDALRaster], [get_pixel_line()]
//' @examples
//' elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
//' ds <- new(GDALRaster, elev_file)
//' gt <- ds$getGeoTransform()
//' ds$close()
//' invgt <- inv_geotransform(gt)
//'
//' ptX = 324181.7
//' ptY = 5103901.4
//'
//' ## for a point x, y in the spatial reference system of elev_file
//' ## raster pixel (column number):
//' pixel <- floor(invgt[1] +
//'                invgt[2] * ptX +
//'                invgt[3] * ptY)
//'
//' ## raster line (row number):
//' line <- floor(invgt[4] +
//'               invgt[5] * ptX +
//'               invgt[6] * ptY)
//'
//' ## get_pixel_line() applies this conversion
// [[Rcpp::export]]
Rcpp::NumericVector inv_geotransform(const std::vector<double> gt) {
    std::vector<double> gt_inv(6);
    if (GDALInvGeoTransform((double *) (gt.data()), gt_inv.data()))
        return Rcpp::wrap(gt_inv);
    else
        return Rcpp::NumericVector(6, NA_REAL);
}


//' Raster pixel/line from geospatial x,y coordinates
//'
//' `get_pixel_line()` converts geospatial coordinates to pixel/line (raster
//' column, row numbers).
//' The upper left corner pixel is the raster origin (0,0) with column, row
//' increasing left to right, top to bottom.
//'
//' @param xy Numeric array of geospatial x,y coordinates in the same
//' spatial reference system as \code{gt}.
//' @param gt Numeric vector of length six. The affine geotransform for the
//' raster.
//' @returns Integer array of raster pixel/line.
//'
//' @seealso [`GDALRaster$getGeoTransform()`][GDALRaster], [inv_geotransform()]
//'
//' @examples
//' pt_file <- system.file("extdata/storml_pts.csv", package="gdalraster")
//' ## id, x, y in NAD83 / UTM zone 12N
//' pts <- read.csv(pt_file)
//' print(pts)
//' raster_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
//' ds <- new(GDALRaster, raster_file)
//' gt <- ds$getGeoTransform()
//' get_pixel_line(as.matrix(pts[,-1]), gt)
//' ds$close()
// [[Rcpp::export]]
Rcpp::IntegerMatrix get_pixel_line(const Rcpp::NumericMatrix xy,
        const std::vector<double> gt) {

    Rcpp::NumericVector inv_gt = inv_geotransform(gt);
    if (Rcpp::any(Rcpp::is_na(inv_gt)))
        Rcpp::stop("could not get inverse geotransform");
    Rcpp::IntegerMatrix pixel_line(xy.nrow(), 2);
    for (R_xlen_t i = 0; i < xy.nrow(); ++i) {
            double geo_x = xy(i, 0);
            double geo_y = xy(i, 1);
            // column
            pixel_line(i, 0) = static_cast<int>(std::floor(inv_gt[0] +
                                                inv_gt[1] * geo_x +
                                                inv_gt[2] * geo_y));
            // row
            pixel_line(i, 1) = static_cast<int>(std::floor(inv_gt[3] +
                                                inv_gt[4] * geo_x +
                                                inv_gt[5] * geo_y));
    }
    return pixel_line;
}


//' Build a GDAL virtual raster from a list of datasets
//'
//' `buildVRT()` is a wrapper of the \command{gdalbuildvrt} command-line
//' utility for building a VRT (Virtual Dataset) that is a mosaic of the list
//' of input GDAL datasets
//' (see \url{https://gdal.org/programs/gdalbuildvrt.html}).
//'
//' @details
//' Several command-line options are described in the GDAL documentation at the
//' URL above. By default, the input files are considered as tiles of a larger
//' mosaic and the VRT file has as many bands as one of the input files.
//' Alternatively, the `-separate` argument can be used to put each input
//' raster into a separate band in the VRT dataset.
//'
//' Some amount of checks are done to assure that all files that will be put in
//' the resulting VRT have similar characteristics: number of bands,
//' projection, color interpretation.... If not, files that do not match the
//' common characteristics will be skipped. (This is true in the default
//' mode for virtual mosaicing, and not when using the `-separate` option).
//'
//' In a virtual mosaic, if there is spatial overlap between
//' input rasters then the order of files appearing in the list of
//' sources matter: files that are listed at the end are the ones
//' from which the data will be fetched. Note that nodata will be taken into
//' account to potentially fetch data from less priority datasets.
//'
//' @param vrt_filename Character string. Filename of the output VRT.
//' @param input_rasters Character vector of input raster filenames.
//' @param cl_arg Optional character vector of command-line arguments to
//' \code{gdalbuildvrt}.
//' @param quiet Logical scalar. If `TRUE`, a progress bar will not be
//' displayed. Defaults to `FALSE`.
//' @returns Logical indicating success (invisible \code{TRUE}).
//' An error is raised if the operation fails.
//'
//' @seealso
//' [rasterToVRT()]
//'
//' @examples
//' # build a virtual 3-band RGB raster from individual Landsat band files
//' b4_file <- system.file("extdata/sr_b4_20200829.tif", package="gdalraster")
//' b5_file <- system.file("extdata/sr_b5_20200829.tif", package="gdalraster")
//' b6_file <- system.file("extdata/sr_b6_20200829.tif", package="gdalraster")
//' band_files <- c(b6_file, b5_file, b4_file)
//' vrt_file <- paste0(tempdir(), "/", "storml_b6_b5_b4.vrt")
//' buildVRT(vrt_file, band_files, cl_arg = "-separate")
//' ds <- new(GDALRaster, vrt_file)
//' ds$getRasterCount()
//' plot_raster(ds, nbands=3, main="Landsat 6-5-4 (vegetative analysis)")
//' ds$close()
//'
//' vsi_unlink(vrt_file)
// [[Rcpp::export(invisible = true)]]
bool buildVRT(Rcpp::CharacterVector vrt_filename,
        Rcpp::CharacterVector input_rasters,
        Rcpp::Nullable<Rcpp::CharacterVector> cl_arg = R_NilValue,
        bool quiet = false) {

    std::string vrt_filename_in;
    vrt_filename_in = Rcpp::as<std::string>(
            _check_gdal_filename(vrt_filename));

    std::vector<std::string> input_rasters_in(input_rasters.size());
    std::vector<const char *> src_ds_files(input_rasters.size() + 1);
    for (R_xlen_t i = 0; i < input_rasters.size(); ++i) {
        input_rasters_in[i] = Rcpp::as<std::string>(
                _check_gdal_filename(
                        Rcpp::as<Rcpp::CharacterVector>(input_rasters[i])));
        src_ds_files[i] = input_rasters_in[i].c_str();
    }
    src_ds_files[input_rasters.size()] = nullptr;

    std::vector<char *> argv = {nullptr};
    if (cl_arg.isNotNull()) {
        Rcpp::CharacterVector cl_arg_in(cl_arg);
        argv.resize(cl_arg_in.size() + 1);
        for (R_xlen_t i = 0; i < cl_arg_in.size(); ++i) {
            argv[i] = (char *) cl_arg_in[i];
        }
        argv[cl_arg_in.size()] = nullptr;
    }

    GDALBuildVRTOptions* psOptions = GDALBuildVRTOptionsNew(argv.data(),
                                                            nullptr);
    if (psOptions == nullptr)
        Rcpp::stop("buildVRT failed (could not create options struct)");
    if (!quiet)
        GDALBuildVRTOptionsSetProgress(psOptions, GDALTermProgressR, nullptr);

    GDALDatasetH hDstDS = GDALBuildVRT(vrt_filename_in.c_str(),
                                       input_rasters.size(), nullptr,
                                       src_ds_files.data(), psOptions,
                                       nullptr);

    GDALBuildVRTOptionsFree(psOptions);

    if (hDstDS == nullptr)
        Rcpp::stop("buildVRT failed");

    GDALClose(hDstDS);
    return true;
}

//' Raster overlay for unique combinations
//'
//' @description
//' `combine()` overlays multiple rasters so that a unique ID is assigned to
//' each unique combination of input values. The input raster layers
//' typically have integer data types (floating point will be coerced to
//' integer by truncation), and must have the same projection, extent and cell
//' size. Pixel counts for each unique combination are obtained, and
//' combination IDs are optionally written to an output raster.
//'
//' Called from and documented in R/gdalraster_proc.R
//' @noRd
// [[Rcpp::export(name = ".combine")]]
Rcpp::DataFrame _combine(
        Rcpp::CharacterVector src_files,
        Rcpp::CharacterVector var_names,
        std::vector<int> bands,
        std::string dst_filename = "",
        std::string fmt = "",
        std::string dataType = "UInt32",
        Rcpp::Nullable<Rcpp::CharacterVector> options = R_NilValue,
        bool quiet = false) {

    std::size_t nrasters = (std::size_t) src_files.size();

    std::vector<GDALRaster> src_ds;
    src_ds.reserve(nrasters);

    int nrows = 0;
    int ncols = 0;
    std::vector<double> gt;
    std::string srs;
    GDALRaster dst_ds;
    bool out_raster = false;

    if ((nrasters != ((std::size_t) var_names.size())) ||
            (nrasters !=  bands.size()))
        Rcpp::stop("'src_files', 'var_names', 'bands' must have same length");

    if (dst_filename != "") {
        out_raster = true;
        if (fmt == "")
            Rcpp::stop("format of output raster must be specified");
    }

    for (std::size_t i = 0; i < nrasters; ++i) {
        src_ds.push_back(GDALRaster(std::string(src_files[i]), true));
        // use the first raster as reference
        if (i==0) {
            nrows = src_ds[i].getRasterYSize();
            ncols = src_ds[i].getRasterXSize();
            gt = src_ds[i].getGeoTransform();
            srs = src_ds[i].getProjectionRef();
        }
    }

    if (out_raster) {
        if (create(fmt, dst_filename, ncols, nrows, 1, dataType, options)) {
            dst_ds = GDALRaster(dst_filename, false);
            if (!dst_ds.setGeoTransform(gt))
                Rcpp::warning("failed to set output geotransform");
            if (!dst_ds.setProjection(srs))
                Rcpp::warning("failed to set output projection");
        }
        else {
            for (std::size_t i = 0; i < nrasters; ++i)
                src_ds[i].close();
            Rcpp::stop("failed to create output raster");
        }
    }

    CmbTable tbl(nrasters, var_names);
    Rcpp::IntegerMatrix rowdata(nrasters, ncols);
    Rcpp::NumericVector cmbid;
    GDALProgressFunc pfnProgress = GDALTermProgressR;
    void* pProgressData = nullptr;

    if (!quiet) {
        if (nrasters == 1)
            Rcpp::Rcout << "scanning raster...\n";
        else
            Rcpp::Rcout << "combining " << nrasters << " rasters...\n";
    }

    for (int y = 0; y < nrows; ++y) {
        for (std::size_t i = 0; i < nrasters; ++i) {
            rowdata.row(i) = Rcpp::as<Rcpp::IntegerVector>(
                                    src_ds[i].read(bands[i], 0, y,
                                                   ncols, 1,
                                                   ncols, 1));
        }

        cmbid = tbl.updateFromMatrix(rowdata, 1);

        if (out_raster) {
            dst_ds.write(1, 0, y, ncols, 1, cmbid);
        }

        if (!quiet) {
            pfnProgress(y / (nrows-1.0), nullptr, pProgressData);
        }
    }

    if (out_raster)
        dst_ds.close();

    for (std::size_t i = 0; i < nrasters; ++i)
        src_ds[i].close();

    return tbl.asDataFrame();
}


//' Compute for a raster band the set of unique pixel values and their counts
//'
//' @noRd
// [[Rcpp::export(name = ".value_count")]]
Rcpp::DataFrame _value_count(std::string src_filename, int band = 1,
                             bool quiet = false) {

    GDALRaster src_ds = GDALRaster(src_filename, true);
    int nrows = src_ds.getRasterYSize();
    int ncols = src_ds.getRasterXSize();
    GDALProgressFunc pfnProgress = nullptr;
    void* pProgressData = nullptr;
    if (!quiet)
        pfnProgress = GDALTermProgressR;
    Rcpp::DataFrame df_out = Rcpp::DataFrame::create();

    if (!quiet)
        Rcpp::Rcout << "scanning raster...\n";

    if (src_ds._readableAsInt(band)) {
        // read pixel values as int
        Rcpp::IntegerVector rowdata(ncols);
        std::unordered_map<int, double> tbl;
        for (int y = 0; y < nrows; ++y) {
            rowdata = Rcpp::as<Rcpp::IntegerVector>(
                            src_ds.read(band, 0, y, ncols, 1, ncols, 1));
            for (auto const& i : rowdata)
                tbl[i] += 1.0;
            if (!quiet)
                pfnProgress(y / (nrows-1.0), nullptr, pProgressData);
        }

        Rcpp::IntegerVector value(tbl.size());
        Rcpp::NumericVector count(tbl.size());
        std::size_t this_idx = 0;
        for(auto iter = tbl.begin(); iter != tbl.end(); ++iter) {
            value[this_idx] = iter->first;
            count[this_idx] = iter->second;
            ++this_idx;
        }

        df_out.push_back(value, "VALUE");
        df_out.push_back(count, "COUNT");
    }
    else {
        // UInt32, Float32, Float64
        // read pixel values as double
        Rcpp::NumericVector rowdata(ncols);
        std::unordered_map<double, double> tbl;
        for (int y = 0; y < nrows; ++y) {
            rowdata = Rcpp::as<Rcpp::NumericVector>(
                            src_ds.read(band, 0, y, ncols, 1, ncols, 1));
            for (auto const& i : rowdata)
                tbl[i] += 1.0;
            if (!quiet)
                pfnProgress(y / (nrows-1.0), nullptr, pProgressData);
        }

        Rcpp::NumericVector value(tbl.size());
        Rcpp::NumericVector count(tbl.size());
        std::size_t this_idx = 0;
        for(auto iter = tbl.begin(); iter != tbl.end(); ++iter) {
            value[this_idx] = iter->first;
            count[this_idx] = iter->second;
            ++this_idx;
        }

        df_out.push_back(value, "VALUE");
        df_out.push_back(count, "COUNT");
    }

    return df_out;
}


//' Wrapper for GDALDEMProcessing in the GDAL Algorithms C API
//'
//' Called from and documented in R/gdalraster_proc.R
//' @noRd
// [[Rcpp::export(name = ".dem_proc")]]
bool _dem_proc(std::string mode,
            Rcpp::CharacterVector src_filename,
            Rcpp::CharacterVector dst_filename,
            Rcpp::Nullable<Rcpp::CharacterVector> cl_arg = R_NilValue,
            Rcpp::Nullable<Rcpp::String> col_file = R_NilValue,
            bool quiet = false) {

    std::string src_filename_in;
    src_filename_in = Rcpp::as<std::string>(_check_gdal_filename(src_filename));
    std::string dst_filename_in;
    dst_filename_in = Rcpp::as<std::string>(_check_gdal_filename(dst_filename));

    GDALDatasetH src_ds = GDALOpenShared(src_filename_in.c_str(), GA_ReadOnly);
    if (src_ds == nullptr)
        Rcpp::stop("open source raster failed");

    std::vector<char *> argv = {nullptr};
    if (cl_arg.isNotNull()) {
        Rcpp::CharacterVector cl_arg_in(cl_arg);
        argv.resize(cl_arg_in.size() + 1);
        for (R_xlen_t i = 0; i < cl_arg_in.size(); ++i) {
            argv[i] = (char *) cl_arg_in[i];
        }
        argv[cl_arg_in.size()] = nullptr;
    }

    GDALDEMProcessingOptions* psOptions;
    psOptions = GDALDEMProcessingOptionsNew(argv.data(), nullptr);
    if (psOptions == nullptr)
        Rcpp::stop("DEM processing failed (could not create options struct)");
    if (!quiet)
        GDALDEMProcessingOptionsSetProgress(psOptions, GDALTermProgressR,
                                            nullptr);

    GDALDatasetH hDstDS;
    if (col_file.isNotNull()) {
        Rcpp::String col_file_in(col_file);
        hDstDS = GDALDEMProcessing(dst_filename_in.c_str(), src_ds,
                                   mode.c_str(), col_file_in.get_cstring(),
                                   psOptions, nullptr);
    }
    else {
        hDstDS = GDALDEMProcessing(dst_filename_in.c_str(), src_ds,
                                   mode.c_str(), nullptr, psOptions, nullptr);
    }

    GDALDEMProcessingOptionsFree(psOptions);
    GDALClose(src_ds);
    if (hDstDS == nullptr)
        Rcpp::stop("DEM processing failed");

    GDALClose(hDstDS);
    return true;
}


//' Fill selected pixels by interpolation from surrounding areas
//'
//' `fillNodata()` is a wrapper for `GDALFillNodata()` in the GDAL Algorithms
//' API. This algorithm will interpolate values for all designated nodata
//' pixels (pixels having an intrinsic nodata value, or marked by zero-valued
//' pixels in the optional raster specified in `mask_file`). For each nodata
//' pixel, a four direction conic search is done to find values to interpolate
//' from (using inverse distance weighting).
//' Once all values are interpolated, zero or more smoothing iterations
//' (3x3 average filters on interpolated pixels) are applied to smooth out
//' artifacts.
//'
//' @note
//' The input raster will be modified in place. It should not be open in a
//' `GDALRaster` object while processing with `fillNodata()`.
//'
//' @param filename Filename of input raster in which to fill nodata pixels.
//' @param band Integer band number to modify in place.
//' @param mask_file Optional filename of raster to use as a validity mask
//' (band 1 is used, zero marks nodata pixels, non-zero marks valid pixels).
//' @param max_dist Maximum distance (in pixels) that the algorithm
//' will search out for values to interpolate (100 pixels by default).
//' @param smooth_iterations The number of 3x3 average filter smoothing
//' iterations to run after the interpolation to dampen artifacts
//' (0 by default).
//' @param quiet Logical scalar. If `TRUE`, a progress bar will not be
//' displayed. Defaults to `FALSE`.
//' @returns Logical indicating success (invisible \code{TRUE}).
//' An error is raised if the operation fails.
//' @examples
//' ## fill nodata edge pixels in the elevation raster
//' elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
//'
//' ## get count of nodata
//' tbl <- buildRAT(elev_file)
//' head(tbl)
//' tbl[is.na(tbl$VALUE),]
//'
//' ## make a copy that will be modified
//' mod_file <- paste0(tempdir(), "/", "storml_elev_fill.tif")
//' file.copy(elev_file,  mod_file)
//'
//' fillNodata(mod_file, band=1)
//'
//' mod_tbl = buildRAT(mod_file)
//' head(mod_tbl)
//' mod_tbl[is.na(mod_tbl$VALUE),]
//'
//' deleteDataset(mod_file)
// [[Rcpp::export(invisible = true)]]
bool fillNodata(Rcpp::CharacterVector filename, int band,
        Rcpp::CharacterVector mask_file = "",
        double max_dist = 100, int smooth_iterations = 0,
        bool quiet = false) {

    GDALDatasetH hDS = nullptr;
    GDALRasterBandH hBand = nullptr;
    GDALDatasetH hMaskDS = nullptr;
    GDALRasterBandH hMaskBand = nullptr;
    CPLErr err;

    std::string filename_in;
    filename_in = Rcpp::as<std::string>(_check_gdal_filename(filename));
    std::string mask_file_in;
    mask_file_in = Rcpp::as<std::string>(_check_gdal_filename(mask_file));

    hDS = GDALOpenShared(filename_in.c_str(), GA_Update);
    if (hDS == nullptr)
        Rcpp::stop("open raster failed");
    hBand = GDALGetRasterBand(hDS, band);
    if (hBand == nullptr) {
        GDALClose(hDS);
        Rcpp::stop("failed to access the requested band");
    }

    if (mask_file_in != "") {
        hMaskDS = GDALOpenShared(mask_file_in.c_str(), GA_ReadOnly);
        if (hMaskDS == nullptr) {
            GDALClose(hDS);
            Rcpp::stop("open mask raster failed");
        }
        hMaskBand = GDALGetRasterBand(hMaskDS, 1);
        if (hMaskBand == nullptr) {
            GDALClose(hDS);
            GDALClose(hMaskDS);
            Rcpp::stop("failed to access the mask band");
        }
    }

    err = GDALFillNodata(hBand, hMaskBand, max_dist, 0, smooth_iterations,
                         nullptr, quiet ? nullptr : GDALTermProgressR,
                         nullptr);

    GDALClose(hDS);
    if (hMaskDS != nullptr)
        GDALClose(hMaskDS);
    if (err != CE_None)
        Rcpp::stop("error in GDALFillNodata()");

    return true;
}


//' Compute footprint of a raster
//'
//' `footprint()` is a wrapper of the \command{gdal_footprint} command-line
//' utility (see \url{https://gdal.org/programs/gdal_footprint.html}).
//' The function can be used to compute the footprint of a raster file, taking
//' into account nodata values (or more generally the mask band attached to
//' the raster bands), and generating polygons/multipolygons corresponding to
//' areas where pixels are valid, and write to an output vector file.
//' Refer to the GDAL documentation at the URL above for a list of command-line
//' arguments that can be passed in `cl_arg`. Requires GDAL >= 3.8.
//'
//' @details
//' Post-vectorization geometric operations are applied in the following order:
//' * optional splitting (`-split_polys`)
//' * optional densification (`-densify`)
//' * optional reprojection (`-t_srs`)
//' * optional filtering by minimum ring area (`-min_ring_area`)
//' * optional application of convex hull (`-convex_hull`)
//' * optional simplification (`-simplify`)
//' * limitation of number of points (`-max_points`)
//'
//' @param src_filename Character string. Filename of the source raster.
//' @param dst_filename Character string. Filename of the destination vector.
//' If the file and the output layer exist, the new footprint is appended to
//' them, unless the `-overwrite` command-line argument is used.
//' @param cl_arg Optional character vector of command-line arguments for
//' \code{gdal_footprint}.
//' @returns Logical indicating success (invisible \code{TRUE}).
//' An error is raised if the operation fails.
//'
//' @seealso
//' [polygonize()]
//'
//' @examples
//' evt_file <- system.file("extdata/storml_evt.tif", package="gdalraster")
//' out_file <- paste0(tempdir(), "/", "storml.geojson")
//'
//' # Requires GDAL >= 3.8
//' if (as.integer(gdal_version()[2]) >= 3080000) {
//'   # command-line arguments for gdal_footprint
//'   args <- c("-t_srs", "EPSG:4326")
//'   footprint(evt_file, out_file, args)
//'
//'   deleteDataset(out_file)
//' }
// [[Rcpp::export(invisible = true)]]
bool footprint(Rcpp::CharacterVector src_filename,
        Rcpp::CharacterVector dst_filename,
        Rcpp::Nullable<Rcpp::CharacterVector> cl_arg = R_NilValue) {

#if GDAL_VERSION_NUM < 3080000
    Rcpp::stop("footprint() requires GDAL >= 3.8");

#else
    std::string src_filename_in;
    src_filename_in = Rcpp::as<std::string>(_check_gdal_filename(src_filename));
    std::string dst_filename_in;
    dst_filename_in = Rcpp::as<std::string>(_check_gdal_filename(dst_filename));

    bool ret = false;
    GDALDatasetH src_ds = GDALOpenShared(src_filename_in.c_str(), GA_ReadOnly);
    if (src_ds == nullptr)
        Rcpp::stop("open source raster failed");

    std::vector<char *> argv = {nullptr};
    if (cl_arg.isNotNull()) {
        Rcpp::CharacterVector cl_arg_in(cl_arg);
        argv.resize(cl_arg_in.size() + 1);
        for (R_xlen_t i = 0; i < cl_arg_in.size(); ++i) {
            argv[i] = (char *) cl_arg_in[i];
        }
        argv[cl_arg_in.size()] = nullptr;
    }

    GDALFootprintOptions* psOptions = GDALFootprintOptionsNew(argv.data(),
                                                              nullptr);
    if (psOptions == nullptr)
        Rcpp::stop("footprint() failed (could not create options struct)");
    GDALFootprintOptionsSetProgress(psOptions, GDALTermProgressR, nullptr);

    GDALDatasetH hDstDS = GDALFootprint(dst_filename_in.c_str(), nullptr,
                                        src_ds, psOptions, nullptr);

    GDALFootprintOptionsFree(psOptions);

    if (hDstDS != nullptr) {
        GDALReleaseDataset(hDstDS);
        ret = true;
    }
    GDALClose(src_ds);

    if (!ret)
        Rcpp::stop("footprint() failed");

    return ret;

#endif
}


//' Convert vector data between different formats
//'
//' `ogr2ogr()` is a wrapper of the \command{ogr2ogr} command-line
//' utility (see \url{https://gdal.org/programs/ogr2ogr.html}).
//' This function can be used to convert simple features data between file
//' formats. It can also perform various operations during the process, such
//' as spatial or attribute selection, reducing the set of attributes, setting
//' the output coordinate system or even reprojecting the features during
//' translation.
//' Refer to the GDAL documentation at the URL above for a description of
//' command-line arguments that can be passed in `cl_arg`.
//'
//' @param src_dsn Character string. Data source name of the source vector
//' dataset.
//' @param dst_dsn Character string. Data source name of the destination vector
//' dataset.
//' @param src_layers Optional character vector of layer names in the source
//' dataset. Defaults to all layers.
//' @param cl_arg Optional character vector of command-line arguments for
//' the GDAL \code{ogr2ogr} command-line utility (see URL above).
//' @returns Logical indicating success (invisible \code{TRUE}).
//' An error is raised if the operation fails.
//'
//' @note
//' For progress reporting, see command-line argument `-progress`: Display
//' progress on terminal. Only works if input layers have the "fast feature
//' count" capability.
//'
//' @seealso
//' [ogrinfo()]
//'
//' [translate()] for raster data
//'
//' @examples
//' src <- system.file("extdata/ynp_fires_1984_2022.gpkg", package="gdalraster")
//'
//' # Convert GeoPackage to Shapefile
//' shp_file <- file.path(tempdir(), "ynp_fires.shp")
//' ogr2ogr(src, shp_file, src_layers = "mtbs_perims")
//'
//' # Reproject to WGS84
//' ynp_wgs84 <- file.path(tempdir(), "ynp_fires_wgs84.gpkg")
//' args <- c("-t_srs", "EPSG:4326")
//' ogr2ogr(src, ynp_wgs84, cl_arg = args)
//'
//' # Clip to a bounding box (xmin, ymin, xmax, ymax in the source SRS)
//' # This will select features whose geometry intersects the bounding box.
//' # The geometries themselves will not be clipped unless "-clipsrc" is
//' # specified.
//' # The source SRS can be overridden with "-spat_srs" "<srs_def>"
//' ynp_clip <- file.path(tempdir(), "ynp_fires_aoi_clip.gpkg")
//' bb <- c(469685.97, 11442.45, 544069.63, 85508.15)
//' args <- c("-spat", bb)
//' ogr2ogr(src, ynp_clip, cl_arg = args)
//'
//' # Filter features by a -where clause
//' ynp_filtered <- file.path(tempdir(), "ynp_fires_2000_2022.gpkg")
//' sql <- "ig_year >= 2000 ORDER BY ig_year"
//' args <- c("-where", sql)
//' ogr2ogr(src, ynp_filtered, src_layers = "mtbs_perims", cl_arg = args)
//'
//' deleteDataset(shp_file)
//' deleteDataset(ynp_wgs84)
//' deleteDataset(ynp_clip)
//' deleteDataset(ynp_filtered)
// [[Rcpp::export(invisible = true)]]
bool ogr2ogr(Rcpp::CharacterVector src_dsn,
        Rcpp::CharacterVector dst_dsn,
        Rcpp::Nullable<Rcpp::CharacterVector> src_layers = R_NilValue,
        Rcpp::Nullable<Rcpp::CharacterVector> cl_arg = R_NilValue) {

    std::string src_dsn_in;
    src_dsn_in = Rcpp::as<std::string>(_check_gdal_filename(src_dsn));
    std::string dst_dsn_in;
    dst_dsn_in = Rcpp::as<std::string>(_check_gdal_filename(dst_dsn));

    // only 1 source dataset is supported currently by GDALVectorTranslate(),
    // but takes a list of datasets as input
    std::vector<GDALDatasetH> src_ds(1);
    bool ret = false;

    src_ds[0] = GDALOpenEx(src_dsn_in.c_str(), GDAL_OF_VECTOR,
                           nullptr, nullptr, nullptr);

    if (src_ds[0] == nullptr)
        Rcpp::stop("failed to open the source dataset");

    std::vector<char *> argv;
    if (cl_arg.isNotNull()) {
        Rcpp::CharacterVector cl_arg_in(cl_arg);
        for (R_xlen_t i = 0; i < cl_arg_in.size(); ++i) {
            argv.push_back((char *) cl_arg_in[i]);
        }
    }
    if (src_layers.isNotNull()) {
        Rcpp::CharacterVector src_layers_in(src_layers);
        for (R_xlen_t i = 0; i < src_layers_in.size(); ++i) {
            argv.push_back((char *) src_layers_in[i]);
        }
    }
    argv.push_back(nullptr);

    GDALVectorTranslateOptions* psOptions =
            GDALVectorTranslateOptionsNew(argv.data(), nullptr);
    if (psOptions == nullptr) {
        Rcpp::stop("ogr2ogr() failed (could not create options struct)");
    }

    GDALDatasetH hDstDS = GDALVectorTranslate(dst_dsn_in.c_str(), nullptr,
                                              1, src_ds.data(), psOptions,
                                              nullptr);

    GDALVectorTranslateOptionsFree(psOptions);

    if (hDstDS != nullptr) {
        GDALReleaseDataset(hDstDS);
        ret = true;
    }

    GDALReleaseDataset(src_ds[0]);

    if (!ret)
        Rcpp::stop("vector translate failed");

    return ret;
}


//' Retrieve information about a vector data source
//'
//' `ogrinfo()` is a wrapper of the \command{ogrinfo} command-line
//' utility (see \url{https://gdal.org/programs/ogrinfo.html}).
//' This function lists information about an OGR-supported data source.
//' It is also possible to edit data with SQL statements.
//' Refer to the GDAL documentation at the URL above for a description of
//' command-line arguments that can be passed in `cl_arg`.
//' Requires GDAL >= 3.7.
//'
//' @param dsn Character string. Data source name (e.g., filename, database
//' connection string, etc.)
//' @param layers Optional character vector of layer names in the source
//' dataset.
//' @param cl_arg Optional character vector of command-line arguments for
//' the \code{ogrinfo} command-line utility in GDAL (see URL above for
//' reference). The default is `c("-so", "-nomd")` (see Note).
//' @param open_options Optional character vector of dataset open options.
//' @param read_only Logical scalar. `TRUE` to open the data source read-only
//' (the default), or `FALSE` to open with write access.
//' @param cout Logical scalar. `TRUE` to write info to the standard C output
//' stream (the default). `FALSE` to suppress console output.
//' @returns Invisibly, a character string containing information about the
//' vector dataset, or empty string (`""`) in case of error.
//'
//' @note
//' The command-line argument `-so` provides a summary only, i.e., does not
//' include details about every single feature of a layer.
//' `-nomd` suppresses metadata printing. Some datasets may contain a lot of
//' metadata strings.
//'
//' @seealso
//' [ogr2ogr()]
//'
//' @examples
//' src <- system.file("extdata/ynp_fires_1984_2022.gpkg", package="gdalraster")
//'
//' # Requires GDAL >= 3.7
//' if (as.integer(gdal_version()[2]) >= 3070000) {
//'   # Get the names of the layers in a GeoPackage file.
//'   ogrinfo(src)
//'
//'   # Summary of a layer
//'   ogrinfo(src, "mtbs_perims")
//'
//'   # JSON format
//'   args <- c("-json", "-nomd")
//'   json <- ogrinfo(src, "mtbs_perims", args, cout = FALSE)
//'   #info <- jsonlite::fromJSON(json)
//'
//'   # Query an attribute to restrict the output of the features in a layer
//'   args <- c("-ro", "-nomd", "-where", "ig_year = 2020")
//'   ogrinfo(src, "mtbs_perims", args)
//'
//'   # Copy to a temporary in-memory file that is writeable
//'   src_mem <- paste0("/vsimem/", basename(src))
//'   vsi_copy_file(src, src_mem)
//'   print(src_mem)
//'
//'   # Add a column to a layer
//'   args <- c("-sql", "ALTER TABLE mtbs_perims ADD burn_bnd_ha float")
//'   ogrinfo(src_mem, cl_arg = args, read_only = FALSE)
//'
//'   # Update values of the column with SQL and specify a dialect
//'   sql <- "UPDATE mtbs_perims SET burn_bnd_ha = (burn_bnd_ac / 2.471)"
//'   args <- c("-dialect", "sqlite", "-sql", sql)
//'   ogrinfo(src_mem, cl_arg = args, read_only = FALSE)
//'
//'   vsi_unlink(src_mem)
//' }
// [[Rcpp::export(invisible = true)]]
std::string ogrinfo(Rcpp::CharacterVector dsn,
        Rcpp::Nullable<Rcpp::CharacterVector> layers = R_NilValue,
        Rcpp::Nullable<Rcpp::CharacterVector> cl_arg =
                Rcpp::CharacterVector::create("-so", "-nomd"),
        Rcpp::Nullable<Rcpp::CharacterVector> open_options = R_NilValue,
        bool read_only = true,
        bool cout = true) {

#if GDAL_VERSION_NUM < 3070000
    Rcpp::stop("ogrinfo() requires GDAL >= 3.7");

#else
    std::string dsn_in;
    dsn_in = Rcpp::as<std::string>(_check_gdal_filename(dsn));

    GDALDatasetH src_ds;

    std::vector<char *> dsoo;
    if (open_options.isNotNull()) {
        Rcpp::CharacterVector open_options_in(open_options);
        for (R_xlen_t i = 0; i < open_options_in.size(); ++i) {
            dsoo.push_back((char *) open_options_in[i]);
        }
    }
    dsoo.push_back(nullptr);

    unsigned int nOpenFlags = GDAL_OF_VECTOR;
    if (read_only)
        nOpenFlags |= GDAL_OF_READONLY;
    else
        nOpenFlags |= GDAL_OF_UPDATE;

    src_ds = GDALOpenEx(dsn_in.c_str(), nOpenFlags, nullptr, dsoo.data(),
                        nullptr);

    if (src_ds == nullptr)
        Rcpp::stop("failed to open the source dataset");

    bool have_args_in = false;
    Rcpp::CharacterVector cl_arg_in;
    std::vector<char *> argv;
    if (cl_arg.isNotNull()) {
        cl_arg_in = cl_arg;
        for (R_xlen_t i = 0; i < cl_arg_in.size(); ++i) {
            argv.push_back((char *) cl_arg_in[i]);
        }
        have_args_in = true;
    }
    argv.push_back((char *) dsn_in.c_str());
    if (layers.isNotNull()) {
        Rcpp::CharacterVector layers_in(layers);
        for (R_xlen_t i = 0; i < layers_in.size(); ++i) {
            argv.push_back((char *) layers_in[i]);
        }
    }
    argv.push_back(nullptr);

    GDALVectorInfoOptions *psOptions =
            GDALVectorInfoOptionsNew(argv.data(), nullptr);

    if (psOptions == nullptr) {
        Rcpp::stop("ogrinfo() failed (could not create options struct)");
    }

    std::string info_out = "";
    char *pszInfo = GDALVectorInfo(src_ds, psOptions);
    if (pszInfo != nullptr)
        info_out = pszInfo;

    CPLFree(pszInfo);
    GDALVectorInfoOptionsFree(psOptions);
    GDALReleaseDataset(src_ds);

    if (cout)
        Rcpp::Rcout << info_out;

    if (have_args_in) {
        Rcpp::CharacterVector::iterator i;
        for (i = cl_arg_in.begin(); i != cl_arg_in.end(); ++i) {
            if (EQUAL(*i, "-json")) {
                info_out.erase(std::remove(info_out.begin(),
                                           info_out.end(),
                                           '\n'),
                               info_out.cend());
                break;
            }
        }
    }

    return info_out;

#endif
}


//' Wrapper for GDALPolygonize in the GDAL Algorithms C API
//'
//' Called from and documented in R/gdalraster_proc.R
//' @noRd
// [[Rcpp::export(name = ".polygonize")]]
bool _polygonize(Rcpp::CharacterVector src_filename, int src_band,
        Rcpp::CharacterVector out_dsn,
        std::string out_layer, std::string fld_name,
        Rcpp::CharacterVector mask_file = "", bool nomask = false,
        int connectedness = 4, bool quiet = false) {

    GDALDatasetH hSrcDS = nullptr;
    GDALRasterBandH hSrcBand = nullptr;
    GDALDatasetH hMaskDS = nullptr;
    GDALRasterBandH hMaskBand = nullptr;
    GDALDatasetH hOutDS = nullptr;
    OGRLayerH hOutLayer = nullptr;
    int iPixValField;

    std::string src_filename_in;
    src_filename_in = Rcpp::as<std::string>(_check_gdal_filename(src_filename));
    std::string out_dsn_in;
    out_dsn_in = Rcpp::as<std::string>(_check_gdal_filename(out_dsn));
    std::string mask_file_in;
    mask_file_in = Rcpp::as<std::string>(_check_gdal_filename(mask_file));

    if (connectedness != 4 && connectedness != 8)
        Rcpp::stop("'connectedness' must be 4 or 8");

    hSrcDS = GDALOpenShared(src_filename_in.c_str(), GA_ReadOnly);
    if (hSrcDS == nullptr)
        Rcpp::stop("open source raster failed");

    hSrcBand = GDALGetRasterBand(hSrcDS, src_band);
    if (hSrcBand == nullptr) {
        GDALClose(hSrcDS);
        Rcpp::stop("failed to access the source band");
    }

    if (mask_file_in == "" && nomask == false) {
        // default validity mask
        hMaskBand = GDALGetMaskBand(hSrcBand);
    }
    else if (mask_file_in == "" && nomask == true) {
        // do not use default validity mask for source band (such as nodata)
        hMaskBand = nullptr;
    }
    else {
        // mask_file specified
        hMaskDS = GDALOpenShared(mask_file_in.c_str(), GA_ReadOnly);
        if (hMaskDS == nullptr) {
            GDALClose(hSrcDS);
            Rcpp::stop("open mask raster failed");
        }
        hMaskBand = GDALGetRasterBand(hMaskDS, 1);
        if (hMaskBand == nullptr) {
            GDALClose(hSrcDS);
            GDALClose(hMaskDS);
            Rcpp::stop("failed to access the mask band");
        }
    }

    hOutDS = GDALOpenEx(out_dsn_in.c_str(), GDAL_OF_VECTOR | GDAL_OF_UPDATE,
                        nullptr, nullptr, nullptr);
    if (hOutDS == nullptr) {
        GDALClose(hSrcDS);
        if (hMaskDS != nullptr)
            GDALClose(hMaskDS);
        Rcpp::stop("failed to open the output vector data source");
    }

    hOutLayer = GDALDatasetGetLayerByName(hOutDS, out_layer.c_str());
    if (hOutLayer == nullptr) {
        GDALClose(hSrcDS);
        if (hMaskDS != nullptr)
            GDALClose(hMaskDS);
        GDALClose(hOutDS);
        Rcpp::stop("failed to open the output layer");
    }

    iPixValField = _ogr_field_index(out_dsn_in, out_layer, fld_name);
    if (iPixValField == -1)
        Rcpp::warning("field not found, pixel values will not be written");

    std::vector<char *> opt_list = {nullptr};
    if (connectedness == 8) {
        auto it = opt_list.begin();
        it = opt_list.insert(it, (char *) "8CONNECTED=8");
    }

    CPLErr err = GDALPolygonize(hSrcBand, hMaskBand, hOutLayer, iPixValField,
                                opt_list.data(),
                                quiet ? nullptr : GDALTermProgressR, nullptr);

    GDALClose(hSrcDS);
    GDALReleaseDataset(hOutDS);
    if (hMaskDS != nullptr)
        GDALClose(hMaskDS);
    if (err != CE_None)
        Rcpp::stop("error in GDALPolygonize()");

    return true;
}


//' Wrapper for GDALRasterize in the GDAL Algorithms C API
//'
//' Called from and documented in R/gdalraster_proc.R
//' @noRd
// [[Rcpp::export(name = ".rasterize")]]
bool _rasterize(std::string src_dsn, std::string dst_filename,
        Rcpp::CharacterVector cl_arg, bool quiet = false) {

    GDALDatasetH hSrcDS;
    hSrcDS = GDALOpenEx(src_dsn.c_str(), GDAL_OF_VECTOR,
                        nullptr, nullptr, nullptr);

    if (hSrcDS == nullptr)
        Rcpp::stop("failed to open vector data source");

    std::vector<char *> argv(cl_arg.size() + 1);
    for (R_xlen_t i = 0; i < cl_arg.size(); ++i) {
        argv[i] = (char *) cl_arg[i];
    }
    argv[cl_arg.size()] = nullptr;

    GDALRasterizeOptions* psOptions;
    psOptions = GDALRasterizeOptionsNew(argv.data(), nullptr);
    if (psOptions == nullptr)
        Rcpp::stop("rasterize failed (could not create options struct)");
    if (!quiet)
        GDALRasterizeOptionsSetProgress(psOptions, GDALTermProgressR, nullptr);

    GDALDatasetH hDstDS;
    hDstDS = GDALRasterize(dst_filename.c_str(), nullptr, hSrcDS,
                           psOptions, nullptr);

    GDALRasterizeOptionsFree(psOptions);
    GDALClose(hSrcDS);
    if (hDstDS == nullptr)
        Rcpp::stop("rasterize failed");

    GDALClose(hDstDS);
    return true;
}


//' Remove small raster polygons
//'
//' `sieveFilter()` is a wrapper for `GDALSieveFilter()` in the GDAL Algorithms
//' API. It removes raster polygons smaller than a provided threshold size
//' (in pixels) and replaces them with the pixel value of the largest neighbour
//' polygon.
//'
//' @details
//' Polygons are determined as regions of the raster where the pixels all have
//' the same value, and that are contiguous (connected).
//' Pixels determined to be "nodata" per the mask band will not be
//' treated as part of a polygon regardless of their pixel values. Nodata areas
//' will never be changed nor affect polygon sizes. Polygons smaller than the
//' threshold with no neighbours that are as large as the threshold will not be
//' altered. Polygons surrounded by nodata areas will therefore not be altered.
//'
//' The algorithm makes three passes over the input file to enumerate the
//' polygons and collect limited information about them. Memory use is
//' proportional to the number of polygons (roughly 24 bytes per polygon), but
//' is not directly related to the size of the raster. So very large raster
//' files can be processed effectively if there aren't too many polygons. But
//' extremely noisy rasters with many one pixel polygons will end up being
//' expensive (in memory) to process.
//'
//' The input dataset is read as integer data which means that floating point
//' values are rounded to integers.
//'
//' @param src_filename Filename of the source raster to be processed.
//' @param src_band Band number in the source raster to be processed.
//' @param dst_filename Filename of the output raster. It may be the same as
//' `src_filename` to update the source file in place.
//' @param dst_band Band number in `dst_filename` to write output. It may be
//' the same as `src_band` to update the source raster in place.
//' @param size_threshold Integer. Raster polygons with sizes (in pixels)
//' smaller than this value will be merged into their largest neighbour.
//' @param connectedness Integer. Either `4` indicating that diagonal pixels
//' are not considered directly adjacent for polygon membership purposes, or
//' `8` indicating they are.
//' @param mask_filename Optional filename of raster to use as a mask.
//' @param mask_band Band number in `mask_filename` to use as a mask. All
//' pixels in the mask band with a value other than zero will be considered
//' suitable for inclusion in polygons.
//' @param options Algorithm options as a character vector of name=value pairs.
//' None currently supported.
//' @param quiet Logical scalar. If `TRUE`, a progress bar will not be
//' displayed. Defaults to `FALSE`.
//' @returns Logical indicating success (invisible \code{TRUE}).
//' An error is raised if the operation fails.
//'
//' @examples
//' ## remove single-pixel polygons from the vegetation type layer (EVT)
//' evt_file <- system.file("extdata/storml_evt.tif", package="gdalraster")
//'
//' # create a blank raster to hold the output
//' evt_mmu_file <- paste0(tempdir(), "/", "storml_evt_mmu2.tif")
//' rasterFromRaster(srcfile = evt_file,
//'                  dstfile = evt_mmu_file,
//'                  init = 32767)
//'
//' # create a mask to exclude water pixels from the algorithm
//' # recode water (7292) to 0
//' expr <- "ifelse(EVT == 7292, 0, EVT)"
//' mask_file <- calc(expr = expr,
//'                   rasterfiles = evt_file,
//'                   var.names = "EVT")
//'
//' # create a version of EVT with two-pixel minimum mapping unit
//' sieveFilter(src_filename = evt_file,
//'             src_band = 1,
//'             dst_filename = evt_mmu_file,
//'             dst_band = 1,
//'             size_threshold = 2,
//'             connectedness = 8,
//'             mask_filename = mask_file,
//'             mask_band = 1)
//'
//' deleteDataset(mask_file)
//' deleteDataset(evt_mmu_file)
// [[Rcpp::export(invisible = true)]]
bool sieveFilter(Rcpp::CharacterVector src_filename, int src_band,
        Rcpp::CharacterVector dst_filename, int dst_band,
        int size_threshold, int connectedness,
        Rcpp::CharacterVector mask_filename = "", int mask_band = 0,
        Rcpp::Nullable<Rcpp::CharacterVector> options = R_NilValue,
        bool quiet = false) {

    GDALDatasetH hSrcDS = nullptr;
    GDALRasterBandH hSrcBand = nullptr;
    GDALDatasetH hMaskDS = nullptr;
    GDALRasterBandH hMaskBand = nullptr;
    GDALDatasetH hDstDS = nullptr;
    GDALRasterBandH hDstBand = nullptr;
    bool in_place = false;
    CPLErr err;

    std::string src_filename_in;
    src_filename_in = Rcpp::as<std::string>(_check_gdal_filename(src_filename));
    std::string dst_filename_in;
    dst_filename_in = Rcpp::as<std::string>(_check_gdal_filename(dst_filename));
    std::string mask_file_in;
    mask_file_in = Rcpp::as<std::string>(_check_gdal_filename(mask_filename));

    if (size_threshold < 1)
        Rcpp::stop("'size_threshold' must be 1 or larger.");

    if (connectedness != 4 && connectedness != 8)
        Rcpp::stop("'connectedness' must be 4 or 8");

    if (src_filename_in == dst_filename_in && src_band == dst_band)
        in_place = true;

    if (in_place)
        hSrcDS = GDALOpenShared(src_filename_in.c_str(), GA_Update);
    else
        hSrcDS = GDALOpenShared(src_filename_in.c_str(), GA_ReadOnly);
    if (hSrcDS == nullptr)
        Rcpp::stop("open source raster failed");
    hSrcBand = GDALGetRasterBand(hSrcDS, src_band);
    if (hSrcBand == nullptr) {
        GDALClose(hSrcDS);
        Rcpp::stop("failed to access the source band");
    }

    if (mask_file_in != "") {
        hMaskDS = GDALOpenShared(mask_file_in.c_str(), GA_ReadOnly);
        if (hMaskDS == nullptr) {
            GDALClose(hSrcDS);
            Rcpp::stop("open mask raster failed");
        }
        hMaskBand = GDALGetRasterBand(hMaskDS, mask_band);
        if (hMaskBand == nullptr) {
            GDALClose(hSrcDS);
            GDALClose(hMaskDS);
            Rcpp::stop("failed to access the mask band");
        }
    }

    if (!in_place) {
        hDstDS = GDALOpenShared(dst_filename_in.c_str(), GA_Update);
        if (hDstDS == nullptr) {
            GDALClose(hSrcDS);
            if (hMaskDS != nullptr)
                GDALClose(hMaskDS);
            Rcpp::stop("open destination raster failed");
        }
        hDstBand = GDALGetRasterBand(hDstDS, dst_band);
        if (hDstBand == nullptr) {
            GDALClose(hSrcDS);
            if (hMaskDS != nullptr)
                GDALClose(hMaskDS);
            GDALClose(hDstDS);
            Rcpp::stop("failed to access the destination band");
        }
    }

    if (in_place)
        err = GDALSieveFilter(hSrcBand, hMaskBand, hSrcBand, size_threshold,
                              connectedness, nullptr,
                              quiet ? nullptr : GDALTermProgressR, nullptr);
    else
        err = GDALSieveFilter(hSrcBand, hMaskBand, hDstBand, size_threshold,
                              connectedness, nullptr,
                              quiet ? nullptr : GDALTermProgressR, nullptr);

    GDALClose(hSrcDS);
    if (hMaskDS != nullptr)
        GDALClose(hMaskDS);
    if (hDstDS != nullptr)
        GDALClose(hDstDS);
    if (err != CE_None)
        Rcpp::stop("error in GDALSieveFilter()");

    return true;
}


//' Convert raster data between different formats
//'
//' `translate()` is a wrapper of the \command{gdal_translate} command-line
//' utility (see \url{https://gdal.org/programs/gdal_translate.html}).
//' The function can be used to convert raster data between different
//' formats, potentially performing some operations like subsetting,
//' resampling, and rescaling pixels in the process. Refer to the GDAL
//' documentation at the URL above for a list of command-line arguments that
//' can be passed in `cl_arg`.
//'
//' @param src_filename Character string. Filename of the source raster.
//' @param dst_filename Character string. Filename of the output raster.
//' @param cl_arg Optional character vector of command-line arguments for
//' \code{gdal_translate} (see URL above).
//' @param quiet Logical scalar. If `TRUE`, a progress bar will not be
//' displayed. Defaults to `FALSE`.
//' @returns Logical indicating success (invisible \code{TRUE}).
//' An error is raised if the operation fails.
//'
//' @seealso
//' [`GDALRaster-class`][GDALRaster], [rasterFromRaster()], [warp()]
//'
//' [ogr2ogr()] for vector data
//'
//' @examples
//' # convert the elevation raster to Erdas Imagine format and resample to 90m
//' elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
//'
//' # command-line arguments for gdal_translate
//' args <- c("-tr", "90", "90", "-r", "average")
//' args <- c(args, "-of", "HFA", "-co", "COMPRESSED=YES")
//'
//' img_file <- paste0(tempdir(), "/", "storml_elev_90m.img")
//' translate(elev_file, img_file, args)
//'
//' ds <- new(GDALRaster, img_file)
//' ds$getDriverLongName()
//' ds$bbox()
//' ds$res()
//' ds$getStatistics(band=1, approx_ok=FALSE, force=TRUE)
//' ds$close()
//'
//' deleteDataset(img_file)
// [[Rcpp::export(invisible = true)]]
bool translate(Rcpp::CharacterVector src_filename,
        Rcpp::CharacterVector dst_filename,
        Rcpp::Nullable<Rcpp::CharacterVector> cl_arg = R_NilValue,
        bool quiet = false) {

    std::string src_filename_in;
    src_filename_in = Rcpp::as<std::string>(_check_gdal_filename(src_filename));
    std::string dst_filename_in;
    dst_filename_in = Rcpp::as<std::string>(_check_gdal_filename(dst_filename));

    bool ret = false;
    GDALDatasetH src_ds = GDALOpenShared(src_filename_in.c_str(), GA_ReadOnly);
    if (src_ds == nullptr)
        Rcpp::stop("open source raster failed");

    std::vector<char *> argv = {nullptr};
    if (cl_arg.isNotNull()) {
        Rcpp::CharacterVector cl_arg_in(cl_arg);
        argv.resize(cl_arg_in.size() + 1);
        for (R_xlen_t i = 0; i < cl_arg_in.size(); ++i) {
            argv[i] = (char *) cl_arg_in[i];
        }
        argv[cl_arg_in.size()] = nullptr;
    }

    GDALTranslateOptions* psOptions = GDALTranslateOptionsNew(argv.data(),
                                                              nullptr);
    if (psOptions == nullptr)
        Rcpp::stop("translate failed (could not create options struct)");
    if (!quiet)
        GDALTranslateOptionsSetProgress(psOptions, GDALTermProgressR, nullptr);

    GDALDatasetH hDstDS = GDALTranslate(dst_filename_in.c_str(), src_ds,
                                        psOptions, nullptr);

    GDALTranslateOptionsFree(psOptions);

    if (hDstDS != nullptr) {
        GDALClose(hDstDS);
        ret = true;
    }
    GDALClose(src_ds);

    if (!ret)
        Rcpp::stop("translate raster failed");

    return ret;
}


//' Raster reprojection and mosaicing
//'
//' `warp()` is a wrapper of the \command{gdalwarp} command-line utility for
//' raster mosaicing, reprojection and warping
//' (see \url{https://gdal.org/programs/gdalwarp.html}).
//' The function can reproject to any supported spatial reference system (SRS).
//' It can also be used to crop, resample, and optionally write output to a
//' different raster format. See Details for a list of commonly used
//' processing options that can be passed as arguments to `warp()`.
//'
//' @details
//' Several processing options can be performed in one call to `warp()` by
//' passing the necessary command-line arguments. The following list describes
//' several commonly used arguments. Note that `gdalwarp` supports a large
//' number of arguments that enable a variety of different processing options.
//' Users are encouraged to review the original source documentation provided
//' by the GDAL project at the URL above for the full list.
//'
//'   * `-te <xmin> <ymin> <xmax> <ymax>`\cr
//'   Georeferenced extents of output file to be created (in target SRS by
//'   default).
//'   * `-te_srs <srs_def>`\cr
//'   SRS in which to interpret the coordinates given with `-te`
//'   (if different than `t_srs`).
//'   * `-tr <xres> <yres>`\cr
//'   Output pixel resolution (in target georeferenced units).
//'   * `-tap`\cr
//'   (target aligned pixels) align the coordinates of the extent of the output
//'   file to the values of the `-tr`, such that the aligned extent includes
//'   the minimum extent. Alignment means that xmin / resx, ymin / resy,
//'   xmax / resx and ymax / resy are integer values.
//'   * `-ovr <level>|AUTO|AUTO-<n>|NONE`\cr
//'   Specify which overview level of source files must be used. The default
//'   choice, `AUTO`, will select the overview level whose resolution is the
//'   closest to the target resolution. Specify an integer value (0-based,
//'   i.e., 0=1st overview level) to select a particular level. Specify
//'   `AUTO-n` where `n` is an integer greater or equal to `1`, to select an
//'   overview level below the `AUTO` one. Or specify `NONE` to force the base
//'   resolution to be used (can be useful if overviews have been generated
//'   with a low quality resampling method, and the warping is done using a
//'   higher quality resampling method).
//'   * `-wo <NAME>=<VALUE>`\cr
//'   Set a warp option as described in the GDAL documentation for
//'   [`GDALWarpOptions`](https://gdal.org/api/gdalwarp_cpp.html#_CPPv415GDALWarpOptions)
//'   Multiple `-wo` may be given. See also `-multi` below.
//'   * `-ot <type>`\cr
//'   Force the output raster bands to have a specific data type supported by
//'   the format, which may be one of the following: `Byte`, `Int8`, `UInt16`,
//'   `Int16`, `UInt32`, `Int32`, `UInt64`, `Int64`, `Float32`, `Float64`,
//'   `CInt16`, `CInt32`, `CFloat32` or `CFloat64`.
//'   * `-r <resampling_method>`\cr
//'   Resampling method to use. Available methods are: `near` (nearest
//'   neighbour, the default), `bilinear`, `cubic`, `cubicspline`, `lanczos`,
//'   `average`, `rms` (root mean square, GDAL >= 3.3), `mode`, `max`, `min`,
//'   `med`, `q1` (first quartile), `q3` (third quartile), `sum` (GDAL >= 3.1).
//'   * `-srcnodata "<value>[ <value>]..."`\cr
//'   Set nodata masking values for input bands (different values can be
//'   supplied for each band). If more than one value is supplied all values
//'   should be quoted to keep them together as a single operating system
//'   argument. Masked values will not be used in interpolation. Use a value of
//'   `None` to ignore intrinsic nodata settings on the source dataset.
//'   If `-srcnodata` is not explicitly set, but the source dataset has nodata
//'   values, they will be taken into account by default.
//'   * `-dstnodata "<value>[ <value>]..."`\cr
//'   Set nodata values for output bands (different values can be supplied for
//'   each band). If more than one value is supplied all values should be
//'   quoted to keep them together as a single operating system argument. New
//'   files will be initialized to this value and if possible the nodata value
//'   will be recorded in the output file. Use a value of `"None"` to ensure
//'   that nodata is not defined. If this argument is not used then nodata
//'   values will be copied from the source dataset.
//'   * `-wm <memory_in_mb>`\cr
//'   Set the amount of memory that the warp API is allowed to use for caching.
//'   The value is interpreted as being in megabytes if the value is <10000.
//'   For values >=10000, this is interpreted as bytes. The warper will
//'   total up the memory required to hold the input and output image arrays
//'   and any auxiliary masking arrays and if they are larger than the
//'   "warp memory" allowed it will subdivide the chunk into smaller chunks and
//'   try again. If the `-wm` value is very small there is some extra overhead
//'   in doing many small chunks so setting it larger is better but it is a
//'   matter of diminishing returns.
//'   * `-multi`\cr
//'   Use multithreaded warping implementation. Two threads will be used to
//'   process chunks of image and perform input/output operation
//'   simultaneously. Note that computation is not multithreaded itself. To do
//'   that, you can use the `-wo NUM_THREADS=val/ALL_CPUS` option, which can be
//'   combined with `-multi`.
//'   * `-of <format>`
//'   Set the output raster format. Will be guessed from the extension if not
//'   specified. Use the short format name (e.g., `"GTiff"`).
//'   * `-co <NAME>=<VALUE>`\cr
//'   Set one or more format specific creation options for the output dataset.
//'   For example, the GeoTIFF driver supports creation options to control
//'   compression, and whether the file should be tiled.
//'   [getCreationOptions()] can be used to look up available creation options,
//'   but the GDAL [Raster drivers](https://gdal.org/drivers/raster/index.html)
//'   documentation is the definitive reference for format specific options.
//'   Multiple `-co` may be given, e.g.,
//'   \preformatted{ c("-co", "COMPRESS=LZW", "-co", "BIGTIFF=YES") }
//'   * `-overwrite`\cr
//'   Overwrite the target dataset if it already exists. Overwriting means
//'   deleting and recreating the file from scratch. Note that if this option
//'   is not specified and the output file already exists, it will be updated
//'   in place.
//'
//' The documentation for [`gdalwarp`](https://gdal.org/programs/gdalwarp.html)
//' describes additional command-line options related to spatial reference
//' systems, source nodata values, alpha bands, polygon cutlines as mask
//' including blending, and more.
//'
//' Mosaicing into an existing output file is supported if the output file
//' already exists. The spatial extent of the existing file will not be
//' modified to accommodate new data, so you may have to remove it in that
//' case, or use the `-overwrite` option.
//'
//' Command-line options are passed to `warp()` as a character vector. The
//' elements of the vector are the individual options followed by their
//' individual values, e.g.,
//' \preformatted{
//' cl_arg = c("-tr", "30", "30", "-r", "bilinear"))
//' }
//' to set the target pixel resolution to 30 x 30 in target georeferenced
//' units and use bilinear resampling.
//'
//' @param src_files Character vector of source file(s) to be reprojected.
//' @param dst_filename Character string. Filename of the output raster.
//' @param t_srs Character string. Target spatial reference system. Usually an
//' EPSG code ("EPSG:#####") or a well known text (WKT) SRS definition.
//' If empty string `""`, the spatial reference of `src_files[1]` will be
//' used (see Note).
//' @param cl_arg Optional character vector of command-line arguments to
//' \code{gdalwarp} in addition to `-t_srs` (see Details).
//' @param quiet Logical scalar. If `TRUE`, a progress bar will not be
//' displayed. Defaults to `FALSE`.
//' @returns Logical indicating success (invisible \code{TRUE}).
//' An error is raised if the operation fails.
//'
//' @note
//' `warp()` can be used to reproject and also perform other processing such
//' as crop, resample, and mosaic.
//' This processing is generally done with a single function call by passing
//' arguments for the target (output) pixel resolution, extent, resampling
//' method, nodata value, format, and so forth.
//' If `warp()` is called with `t_srs` set to `""` (empty string),
//' the target spatial reference will be set to that of `src_files[1]`,
//' so that the processing options given in `cl_arg` will be performed without
//' reprojecting (in the case of one input raster or multiple inputs that
//' all use the same spatial reference system, otherwise would reproject
//' inputs to the SRS of `src_files[1]` when they are different).
//'
//' @seealso
//' [`GDALRaster-class`][GDALRaster], [srs_to_wkt()], [translate()]
//'
//' @examples
//' # reproject the elevation raster to NAD83 / CONUS Albers (EPSG:5070)
//' elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
//'
//' # command-line arguments for gdalwarp
//' # resample to 90-m resolution and keep pixels aligned:
//' args <- c("-tr", "90", "90", "-r", "cubic", "-tap")
//' # write to Erdas Imagine format (HFA) with compression:
//' args <- c(args, "-of", "HFA", "-co", "COMPRESSED=YES")
//'
//' alb83_file <- paste0(tempdir(), "/", "storml_elev_alb83.img")
//' warp(elev_file, alb83_file, t_srs="EPSG:5070", cl_arg = args)
//'
//' ds <- new(GDALRaster, alb83_file)
//' ds$getDriverLongName()
//' ds$getProjectionRef()
//' ds$res()
//' ds$getStatistics(band=1, approx_ok=FALSE, force=TRUE)
//' ds$close()
//'
//' deleteDataset(alb83_file)
// [[Rcpp::export(invisible = true)]]
bool warp(Rcpp::CharacterVector src_files,
        Rcpp::CharacterVector dst_filename,
        std::string t_srs,
        Rcpp::Nullable<Rcpp::CharacterVector> cl_arg = R_NilValue,
        bool quiet = false) {

    std::string dst_filename_in;
    dst_filename_in = Rcpp::as<std::string>(_check_gdal_filename(dst_filename));

    bool ret = false;
    std::vector<GDALDatasetH> src_ds(src_files.size());

    for (R_xlen_t i = 0; i < src_files.size(); ++i) {
        std::string src_file_in;
        src_file_in = Rcpp::as<std::string>(_check_gdal_filename(
                Rcpp::as<Rcpp::CharacterVector>(src_files[i])));
        GDALDatasetH hDS = GDALOpenShared(src_file_in.c_str(), GA_ReadOnly);
        if (hDS == nullptr) {
            Rcpp::Rcerr << "error on source: " << src_file_in.c_str() << "\n";
            for (R_xlen_t j = 0; j < i; ++j)
                GDALClose(src_ds[j]);
            Rcpp::stop("open source raster failed");
        }
        else {
            src_ds[i] = hDS;
        }
    }

    std::string t_srs_in;
    if (t_srs != "")
        t_srs_in = t_srs;
    else
        t_srs_in = GDALGetProjectionRef(src_ds[0]);

    std::vector<char *> argv =
            {(char *) "-t_srs", (char *) t_srs_in.c_str(), nullptr};

    if (cl_arg.isNotNull()) {
        Rcpp::CharacterVector cl_arg_in(cl_arg);
        argv.resize(cl_arg_in.size() + 3);
        for (R_xlen_t i = 0; i < cl_arg_in.size(); ++i) {
            argv[i+2] = (char *) cl_arg_in[i];
        }
        argv[cl_arg_in.size() + 2] = nullptr;
    }

    GDALWarpAppOptions* psOptions = GDALWarpAppOptionsNew(argv.data(), nullptr);
    if (psOptions == nullptr)
        Rcpp::stop("warp raster failed (could not create options struct)");
    if (!quiet)
        GDALWarpAppOptionsSetProgress(psOptions, GDALTermProgressR, nullptr);

    GDALDatasetH hDstDS = GDALWarp(dst_filename_in.c_str(), nullptr,
                                   src_files.size(), src_ds.data(),
                                   psOptions, nullptr);

    GDALWarpAppOptionsFree(psOptions);

    if (hDstDS != nullptr) {
        GDALClose(hDstDS);
        ret = true;
    }
    for (R_xlen_t i = 0; i < src_files.size(); ++i)
        GDALClose(src_ds[i]);

    if (!ret)
        Rcpp::stop("warp raster failed");

    return ret;
}


//' Create a color ramp
//'
//' `createColorRamp()` is a wrapper for `GDALCreateColorRamp()` in the GDAL
//' API. It automatically creates a color ramp from one color entry to another.
//' Output is an integer matrix in color table format for use with
//' [`GDALRaster$setColorTable()`][GDALRaster].
//'
//' @note
//' `createColorRamp()` could be called several times, using `rbind()` to
//' combine multiple ramps into the same color table. Possible duplicate rows
//' in the resulting table are not a problem when used in
//' `GDALRaster$setColorTable()` (i.e., when `end_color` of one ramp is the
//' same as `start_color` of the next ramp).
//'
//' @param start_index Integer start index (raster value).
//' @param start_color Integer vector of length three or four.
//' A color entry value to start the ramp (e.g., RGB values).
//' @param end_index Integer end index (raster value).
//' @param end_color Integer vector of length three or four.
//' A color entry value to end the ramp (e.g., RGB values).
//' @param palette_interp One of "Gray", "RGB" (the default), "CMYK" or "HLS"
//' describing interpretation of `start_color` and `end_color` values
//' (see \href{https://gdal.org/user/raster_data_model.html#color-table}{GDAL
//' Color Table}).
//' @returns Integer matrix with five columns containing the color ramp from
//' `start_index` to `end_index`, with raster index values in column 1 and
//' color entries in columns 2:5).
//'
//' @seealso
//' [`GDALRaster$getColorTable()`][GDALRaster],
//' [`GDALRaster$getPaletteInterp()`][GDALRaster]
//'
//' @examples
//' # create a color ramp for tree canopy cover percent
//' # band 5 of an LCP file contains canopy cover
//' lcp_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
//' ds <- new(GDALRaster, lcp_file)
//' ds$getDescription(band=5)
//' ds$getMetadata(band=5, domain="")
//' ds$close()
//'
//' # create a GTiff file with Byte data type for the canopy cover band
//' # recode nodata -9999 to 255
//' tcc_file <- calc(expr = "ifelse(CANCOV == -9999, 255, CANCOV)",
//'                  rasterfiles = lcp_file,
//'                  bands = 5,
//'                  var.names = "CANCOV",
//'                  fmt = "GTiff",
//'                  dtName = "Byte",
//'                  nodata_value = 255,
//'                  setRasterNodataValue = TRUE)
//'
//' ds_tcc <- new(GDALRaster, tcc_file, read_only=FALSE)
//'
//' # create a color ramp from 0 to 100 and set as the color table
//' colors <- createColorRamp(start_index = 0,
//'                           start_color = c(211, 211, 211),
//'                           end_index = 100,
//'                           end_color = c(0, 100, 0))
//'
//' print(colors)
//' ds_tcc$setColorTable(band=1, col_tbl=colors, palette_interp="RGB")
//' ds_tcc$setRasterColorInterp(band=1, col_interp="Palette")
//'
//' # close and re-open the dataset in read_only mode
//' ds_tcc$open(read_only=TRUE)
//'
//' plot_raster(ds_tcc, interpolate=FALSE, legend=TRUE,
//'             main="Storm Lake Tree Canopy Cover (%)")
//' ds_tcc$close()
//'
//' deleteDataset(tcc_file)
// [[Rcpp::export]]
Rcpp::IntegerMatrix createColorRamp(int start_index,
        Rcpp::IntegerVector start_color,
        int end_index,
        Rcpp::IntegerVector end_color,
        std::string palette_interp = "RGB") {

    if (end_index <= start_index)
        Rcpp::stop("'end_index' must be greater than 'start_index'");
    if (start_color.size() < 3 || start_color.size() > 4)
        Rcpp::stop("length of 'start_color' must be 3 or 4");
    if (end_color.size() < 3 || end_color.size() > 4)
        Rcpp::stop("length of 'end_color' must be 3 or 4");

    if (start_color.size() == 3)
        start_color.push_back(255);
    if (end_color.size() == 3)
        end_color.push_back(255);

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
        Rcpp::stop("invalid 'palette_interp'");

    GDALColorTableH hColTbl = GDALCreateColorTable(gpi);
    if (hColTbl == nullptr)
        Rcpp::stop("could not create GDAL color table");

    const GDALColorEntry colStart = {
            static_cast<short>(start_color(0)),
            static_cast<short>(start_color(1)),
            static_cast<short>(start_color(2)),
            static_cast<short>(start_color(3)) };
    const GDALColorEntry colEnd = {
            static_cast<short>(end_color(0)),
            static_cast<short>(end_color(1)),
            static_cast<short>(end_color(2)),
            static_cast<short>(end_color(3)) };

    GDALCreateColorRamp(hColTbl, start_index, &colStart, end_index, &colEnd);

    //int nEntries = GDALGetColorEntryCount(hColTbl);
    int nEntries = (end_index - start_index) + 1;
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

    int idx = start_index;
    for (int i=0; i < nEntries; ++i) {
        const GDALColorEntry* colEntry = GDALGetColorEntry(hColTbl, idx);
        col_tbl(i, 0) = idx;
        col_tbl(i, 1) = colEntry->c1;
        col_tbl(i, 2) = colEntry->c2;
        col_tbl(i, 3) = colEntry->c3;
        col_tbl(i, 4) = colEntry->c4;
        ++idx;
    }

    GDALDestroyColorTable(hColTbl);

    return col_tbl;
}


//' Copy a whole raster band efficiently
//'
//' `bandCopyWholeRaster()` copies the complete raster contents of one band to
//' another similarly configured band. The source and destination bands must
//' have the same xsize and ysize. The bands do not have to have the same data
//' type. It implements efficient copying, in particular "chunking" the copy in
//' substantial blocks. This is a wrapper for `GDALRasterBandCopyWholeRaster()`
//' in the GDAL API.
//'
//' @param src_filename Filename of the source raster.
//' @param src_band Band number in the source raster to be copied.
//' @param dst_filename Filename of the destination raster.
//' @param dst_band Band number in the destination raster to copy into.
//' @param options Optional list of transfer hints in a vector of `"NAME=VALUE"`
//' pairs. The currently supported `options` are:
//'   * `"COMPRESSED=YES"` to force alignment on target dataset block sizes to
//'   achieve best compression.
//'    * `"SKIP_HOLES=YES"` to skip chunks that contain only empty blocks.
//'    Empty blocks are blocks that are generally not physically present in the
//'    file, and when read through GDAL, contain only pixels whose value is the
//'    nodata value when it is set, or whose value is 0 when the nodata value is
//'    not set. The query is done in an efficient way without reading the actual
//'    pixel values (if implemented by the raster format driver, otherwise will
//'    not be skipped).
//' @param quiet Logical scalar. If `TRUE`, a progress bar will not be
//' displayed. Defaults to `FALSE`.
//' @returns Logical indicating success (invisible \code{TRUE}).
//' An error is raised if the operation fails.
//'
//' @seealso
//' [`GDALRaster-class`][GDALRaster], [create()], [createCopy()],
//' [rasterFromRaster()]
//'
//' @examples
//' ## copy Landsat data from a single-band file to a new multi-band image
//' b5_file <- system.file("extdata/sr_b5_20200829.tif", package="gdalraster")
//' dst_file <- paste0(tempdir(), "/", "sr_multi.tif")
//' rasterFromRaster(b5_file, dst_file, nbands=7, init=0)
//' opt <- c("COMPRESSED=YES", "SKIP_HOLES=YES")
//' bandCopyWholeRaster(b5_file, 1, dst_file, 5, options=opt)
//' ds <- new(GDALRaster, dst_file)
//' ds$getStatistics(band=5, approx_ok=FALSE, force=TRUE)
//' ds$close()
//'
//' deleteDataset(dst_file)
// [[Rcpp::export(invisible = true)]]
bool bandCopyWholeRaster(Rcpp::CharacterVector src_filename, int src_band,
        Rcpp::CharacterVector dst_filename, int dst_band,
        Rcpp::Nullable<Rcpp::CharacterVector> options = R_NilValue,
        bool quiet = false) {

    GDALDatasetH hSrcDS = nullptr;
    GDALRasterBandH hSrcBand = nullptr;
    GDALDatasetH hDstDS = nullptr;
    GDALRasterBandH hDstBand = nullptr;
    CPLErr err;

    std::string src_filename_in;
    src_filename_in = Rcpp::as<std::string>(_check_gdal_filename(src_filename));
    std::string dst_filename_in;
    dst_filename_in = Rcpp::as<std::string>(_check_gdal_filename(dst_filename));

    hSrcDS = GDALOpenShared(src_filename_in.c_str(), GA_ReadOnly);
    if (hSrcDS == nullptr)
        return false;
    hSrcBand = GDALGetRasterBand(hSrcDS, src_band);
    if (hSrcBand == nullptr) {
        GDALClose(hSrcDS);
        return false;
    }

    hDstDS = GDALOpenShared(dst_filename_in.c_str(), GA_Update);
    if (hDstDS == nullptr) {
        GDALClose(hSrcDS);
        return false;
    }
    hDstBand = GDALGetRasterBand(hDstDS, dst_band);
    if (hDstBand == nullptr) {
        GDALClose(hSrcDS);
        GDALClose(hDstDS);
        return false;
    }

    std::vector<char *> opt_list = {nullptr};
    if (options.isNotNull()) {
        Rcpp::CharacterVector options_in(options);
        opt_list.resize(options_in.size() + 1);
        for (R_xlen_t i = 0; i < options_in.size(); ++i) {
            opt_list[i] = (char *) options_in[i];
        }
        opt_list[options_in.size()] = nullptr;
    }

    err = GDALRasterBandCopyWholeRaster(hSrcBand, hDstBand, opt_list.data(),
                                        quiet ? nullptr : GDALTermProgressR,
                                        nullptr);

    GDALClose(hSrcDS);
    GDALClose(hDstDS);
    if (err != CE_None)
        return false;
    else
        return true;
}


//' Delete named dataset
//'
//' `deleteDataset()` will attempt to delete the named dataset in a format
//' specific fashion. Full featured drivers will delete all associated files,
//' database objects, or whatever is appropriate. The default behavior when no
//' format specific behavior is provided is to attempt to delete all the files
//' that would be returned by `GDALRaster$getFileList()` on the dataset.
//' The named dataset should not be open in any existing `GDALRaster` objects
//' when `deleteDataset()` is called. Wrapper for `GDALDeleteDataset()` in the
//' GDAL API.
//'
//' @note
//' If `format` is set to an empty string `""` (the default) then the function
//' will try to identify the driver from `filename`. This is done internally in
//' GDAL by invoking the `Identify` method of each registered `GDALDriver` in
//' turn. The first driver that successful identifies the file name will be
//' returned. An error is raised if a format cannot be determined from the
//' passed file name.
//'
//' @param filename Filename to delete (should not be open in a `GDALRaster`
//' object).
//' @param format Raster format short name (e.g., "GTiff"). If set to empty
//' string `""` (the default), will attempt to guess the raster format from
//' `filename`.
//' @returns Logical `TRUE` if no error or `FALSE` on failure.
//'
//' @seealso
//' [`GDALRaster-class`][GDALRaster], [create()], [createCopy()],
//' [copyDatasetFiles()], [renameDataset()]
//'
//' @examples
//' b5_file <- system.file("extdata/sr_b5_20200829.tif", package="gdalraster")
//' b5_tmp <- paste0(tempdir(), "/", "b5_tmp.tif")
//' file.copy(b5_file,  b5_tmp)
//'
//' ds <- new(GDALRaster, b5_tmp)
//' ds$buildOverviews("BILINEAR", levels = c(2, 4, 8), bands = c(1))
//' files <- ds$getFileList()
//' print(files)
//' ds$close()
//' file.exists(files)
//' deleteDataset(b5_tmp)
//' file.exists(files)
// [[Rcpp::export]]
bool deleteDataset(Rcpp::CharacterVector filename, std::string format = "") {

    std::string filename_in;
    filename_in = Rcpp::as<std::string>(_check_gdal_filename(filename));

    GDALDriverH hDriver;
    if (format == "") {
        hDriver = GDALIdentifyDriver(filename_in.c_str(), nullptr);
        if (hDriver == nullptr)
            return false;
    }
    else {
        hDriver = GDALGetDriverByName(format.c_str());
        if (hDriver == nullptr)
            return false;
    }

    CPLErr err = GDALDeleteDataset(hDriver, filename_in.c_str());
    if (err != CE_None)
        return false;
    else
        return true;
}


//' Rename a dataset
//'
//' `renameDataset()` renames a dataset in a format-specific way (e.g.,
//' rename associated files as appropriate). This could include moving the
//' dataset to a new directory or even a new filesystem.
//' The dataset should not be open in any existing `GDALRaster` objects
//' when `renameDataset()` is called. Wrapper for `GDALRenameDataset()` in the
//' GDAL API.
//'
//' @note
//' If `format` is set to an empty string `""` (the default) then the function
//' will try to identify the driver from `old_filename`. This is done
//' internally in GDAL by invoking the `Identify` method of each registered
//' `GDALDriver` in turn. The first driver that successful identifies the file
//' name will be returned. An error is raised if a format cannot be determined
//' from the passed file name.
//'
//' @param new_filename New name for the dataset.
//' @param old_filename Old name for the dataset (should not be open in a
//' `GDALRaster` object).
//' @param format Raster format short name (e.g., "GTiff"). If set to empty
//' string `""` (the default), will attempt to guess the raster format from
//' `old_filename`.
//' @returns Logical `TRUE` if no error or `FALSE` on failure.
//'
//' @seealso
//' [`GDALRaster-class`][GDALRaster], [create()], [createCopy()],
//' [deleteDataset()], [copyDatasetFiles()]
//'
//' @examples
//' b5_file <- system.file("extdata/sr_b5_20200829.tif", package="gdalraster")
//' b5_tmp <- paste0(tempdir(), "/", "b5_tmp.tif")
//' file.copy(b5_file,  b5_tmp)
//'
//' ds <- new(GDALRaster, b5_tmp)
//' ds$buildOverviews("BILINEAR", levels = c(2, 4, 8), bands = c(1))
//' ds$getFileList()
//' ds$close()
//' b5_tmp2 <- paste0(tempdir(), "/", "b5_tmp_renamed.tif")
//' renameDataset(b5_tmp2, b5_tmp)
//' ds <- new(GDALRaster, b5_tmp2)
//' ds$getFileList()
//' ds$close()
//'
//' deleteDataset(b5_tmp2)
// [[Rcpp::export]]
bool renameDataset(Rcpp::CharacterVector new_filename,
        Rcpp::CharacterVector old_filename,
        std::string format = "") {

    std::string new_filename_in;
    new_filename_in = Rcpp::as<std::string>(_check_gdal_filename(new_filename));
    std::string old_filename_in;
    old_filename_in = Rcpp::as<std::string>(_check_gdal_filename(old_filename));

    GDALDriverH hDriver;
    if (format == "") {
        hDriver = GDALIdentifyDriver(old_filename_in.c_str(), nullptr);
        if (hDriver == nullptr)
            return false;
    }
    else {
        hDriver = GDALGetDriverByName(format.c_str());
        if (hDriver == nullptr)
            return false;
    }

    CPLErr err = GDALRenameDataset(hDriver, new_filename_in.c_str(),
                                   old_filename_in.c_str());
    if (err != CE_None)
        return false;
    else
        return true;
}


//' Copy the files of a dataset
//'
//' `copyDatasetFiles()` copies all the files associated with a dataset.
//' Wrapper for `GDALCopyDatasetFiles()` in the GDAL API.
//'
//' @note
//' If `format` is set to an empty string `""` (the default) then the function
//' will try to identify the driver from `old_filename`. This is done
//' internally in GDAL by invoking the `Identify` method of each registered
//' `GDALDriver` in turn. The first driver that successful identifies the file
//' name will be returned. An error is raised if a format cannot be determined
//' from the passed file name.
//'
//' @param new_filename New name for the dataset (copied to).
//' @param old_filename Old name for the dataset (copied from).
//' @param format Raster format short name (e.g., "GTiff"). If set to empty
//' string `""` (the default), will attempt to guess the raster format from
//' `old_filename`.
//' @returns Logical `TRUE` if no error or `FALSE` on failure.
//'
//' @seealso
//' [`GDALRaster-class`][GDALRaster], [create()], [createCopy()],
//' [deleteDataset()], [renameDataset()], [vsi_copy_file()]
//'
//' @examples
//' lcp_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
//' ds <- new(GDALRaster, lcp_file)
//' ds$getFileList()
//' ds$close()
//'
//' lcp_tmp <- paste0(tempdir(), "/", "storm_lake_copy.lcp")
//' copyDatasetFiles(lcp_tmp, lcp_file)
//' ds_copy <- new(GDALRaster, lcp_tmp)
//' ds_copy$getFileList()
//' ds_copy$close()
//'
//' deleteDataset(lcp_tmp)
// [[Rcpp::export]]
bool copyDatasetFiles(Rcpp::CharacterVector new_filename,
        Rcpp::CharacterVector old_filename,
        std::string format = "") {

    std::string new_filename_in;
    new_filename_in = Rcpp::as<std::string>(_check_gdal_filename(new_filename));
    std::string old_filename_in;
    old_filename_in = Rcpp::as<std::string>(_check_gdal_filename(old_filename));

    GDALDriverH hDriver;
    if (format == "") {
        hDriver = GDALIdentifyDriver(old_filename_in.c_str(), nullptr);
        if (hDriver == nullptr)
            return false;
    }
    else {
        hDriver = GDALGetDriverByName(format.c_str());
        if (hDriver == nullptr)
            return false;
    }

    CPLErr err = GDALCopyDatasetFiles(hDriver, new_filename_in.c_str(),
                                      old_filename_in.c_str());
    if (err != CE_None)
        return false;
    else
        return true;
}


//' Return the list of creation options of a GDAL driver as XML string
//'
//' Called from and documented in R/gdal_helpers.R
//' @noRd
// [[Rcpp::export(name = ".getCreationOptions")]]
std::string _getCreationOptions(std::string format) {

    GDALDriverH hDriver = GDALGetDriverByName(format.c_str());
    if (hDriver == nullptr)
        Rcpp::stop("failed to get driver from format name");

    return GDALGetDriverCreationOptionList(hDriver);
}


//' Add a file inside a new or existing ZIP file
//' Mainly for create/append to Seek-Optimized ZIP
//'
//' @noRd
// [[Rcpp::export(name = ".addFileInZip")]]
bool _addFileInZip(std::string zip_filename, bool overwrite,
        std::string archive_filename, std::string in_filename,
        Rcpp::Nullable<Rcpp::CharacterVector> options,
        bool quiet) {

#if GDAL_VERSION_NUM < 3070000
    Rcpp::stop("_addFileInZip() requires GDAL >= 3.7");

#else
    bool ret;
    std::vector<char *> opt_zip_create;
    VSIStatBufL buf;

    if (overwrite) {
        VSIUnlink(zip_filename.c_str());
    }
    else {
        if (VSIStatExL(zip_filename.c_str(), &buf, VSI_STAT_EXISTS_FLAG) == 0)
            opt_zip_create.push_back((char *) "APPEND=TRUE");
    }
    opt_zip_create.push_back(nullptr);

    void *hZIP = CPLCreateZip(zip_filename.c_str(), opt_zip_create.data());
    if (hZIP == nullptr)
        Rcpp::stop("failed to obtain file handle for zip file");

    std::vector<char *> opt_list = {nullptr};
    if (options.isNotNull()) {
        Rcpp::CharacterVector options_in(options);
        opt_list.resize(options_in.size() + 1);
        for (R_xlen_t i = 0; i < options_in.size(); ++i) {
            opt_list[i] = (char *) options_in[i];
        }
        opt_list[options_in.size()] = nullptr;
    }

    if (!quiet) {
        Rcpp::Rcout << "adding " << in_filename.c_str() << " ...\n";
        GDALTermProgressR(0, nullptr, nullptr);
    }

    CPLErr err = CPLAddFileInZip(hZIP, archive_filename.c_str(),
                                 in_filename.c_str(),
                                 nullptr, opt_list.data(),
                                 quiet ? nullptr : GDALTermProgressR,
                                 nullptr);

    if (err == CE_None)
        ret = true;
    else
        ret = false;

    CPLCloseZip(hZIP);
    return ret;

#endif
}
