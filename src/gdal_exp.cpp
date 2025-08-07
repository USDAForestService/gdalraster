/* Exported stand-alone functions for gdalraster
   Chris Toney <chris.toney at usda.gov>
   Copyright (c) 2023-2025 gdalraster authors
*/

#include <errno.h>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <unordered_map>

#include "gdal.h"
#include "cpl_port.h"
#include "cpl_conv.h"
#include "cpl_http.h"
#include "cpl_multiproc.h"
#include "cpl_string.h"
#include "cpl_vsi.h"
#include "gdal_alg.h"
#include "gdal_utils.h"
#include "gdalwarper.h"

#include "cmb_table.h"
#include "ogr_util.h"
#include "gdalraster.h"

//' Get GDAL version
//'
//' `gdal_version()` returns a character vector of GDAL runtime version
//' information. `gdal_version_num()` returns only the full version number
//' (`gdal_version()[2]`) as an integer value.
//'
//' @name gdal_version
//'
//' @returns
//' `gdal_version()` returns a character vector of length four containing:
//'   * "–version" - one line version message, e.g., “GDAL 3.6.3, released
//'   2023/03/12”
//'   * "GDAL_VERSION_NUM" - formatted as a string, e.g., “3060300” for
//'   GDAL 3.6.3.0
//'   * "GDAL_RELEASE_DATE" - formatted as a string, e.g., “20230312”
//'   * "GDAL_RELEASE_NAME" - e.g., “3.6.3”
//'
//' `gdal_version_num()` returns `as.integer(gdal_version()[2])`
//' @examples
//' gdal_version()
//'
//' gdal_version_num()
// [[Rcpp::export]]
Rcpp::CharacterVector gdal_version() {
    Rcpp::CharacterVector ret(4);
    ret(0) = GDALVersionInfo("-version");
    ret(1) = GDALVersionInfo("VERSION_NUM");
    ret(2) = GDALVersionInfo("RELEASE_DATE");
    ret(3) = GDALVersionInfo("RELEASE_NAME");
    return ret;
}


//' @rdname gdal_version
// [[Rcpp::export]]
int gdal_version_num() {
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
//' \url{https://gdal.org/en/stable/user/virtual_file_systems.html#virtual-file-systems}.
//'
//' @examples
//' nrow(gdal_formats())
//' head(gdal_formats())
//'
//' gdal_formats("GPKG")
// [[Rcpp::export]]
Rcpp::DataFrame gdal_formats(const std::string &format = "") {

    Rcpp::CharacterVector short_name = Rcpp::CharacterVector::create();
    Rcpp::CharacterVector long_name = Rcpp::CharacterVector::create();
    Rcpp::LogicalVector raster_fmt = Rcpp::LogicalVector::create();
    Rcpp::LogicalVector vector_fmt = Rcpp::LogicalVector::create();
    Rcpp::CharacterVector rw_flag = Rcpp::CharacterVector::create();
    Rcpp::LogicalVector virtual_io = Rcpp::LogicalVector::create();
    Rcpp::LogicalVector subdatasets = Rcpp::LogicalVector::create();

    for (int i = 0; i < GDALGetDriverCount(); ++i) {
        GDALDriverH hDriver = GDALGetDriver(i);
        char **papszMD = GDALGetMetadata(hDriver, nullptr);
        std::string rw = "";

        if (format != "" && !EQUAL(format.c_str(),
                                   GDALGetDriverShortName(hDriver))) {

            continue;
        }

        if (CPLFetchBool(papszMD, GDAL_DCAP_RASTER, false) ||
            CPLFetchBool(papszMD, GDAL_DCAP_VECTOR, false)) {

            CPLFetchBool(papszMD, GDAL_DCAP_RASTER, false) ?
                    raster_fmt.push_back(true) : raster_fmt.push_back(false);

            CPLFetchBool(papszMD, GDAL_DCAP_VECTOR, false) ?
                    vector_fmt.push_back(true) : vector_fmt.push_back(false);

        } else {
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
//' \url{https://gdal.org/en/stable/user/configoptions.html}.
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
std::string get_config_option(const std::string &key) {
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
//' \url{https://gdal.org/en/stable/user/configoptions.html}.
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
void set_config_option(const std::string &key, const std::string &value) {
    const char* value_ = nullptr;
    if (value != "")
        value_ = value.c_str();

    CPLSetConfigOption(key.c_str(), value_);
}


//' Get the maximum memory size available for the GDAL block cache
//'
//' `get_cache_max()` returns the maximum amount of memory available to the
//' GDALRasterBlock caching system for caching raster read/write data. Wrapper
//' of `GDALGetCacheMax64()` with return value in MB by default.
//'
//' @details
//' The first time this function is called, it will read the `GDAL_CACHEMAX`
//' configuration option to initialize the maximum cache memory. The value of
//' the configuration option can be expressed as x% of the usable physical RAM
//' (which may potentially be used by other processes). Otherwise it is
//' expected to be a value in MB.
//' As of GDAL 3.10, the default value, if `GDAL_CACHEMAX` has not been set
//' explicitly, is 5% of usable physical RAM.
//'
//' @param units Character string specifying units for the return value. One of
//' `"MB"` (the default), `"GB"`, `"KB"` or `"bytes"` (values of `"byte"`,
//' `"B"` and empty string `""` are also recognized to mean bytes).
//' @returns A numeric value carrying the `integer64` class attribute. Maximum
//' cache memory available in the requested units.
//'
//' @note
//' The value of the `GDAL_CACHEMAX` configuration option is only consulted the
//' first time the cache size is requested (i.e., it must be set as a
//' configuration option prior to any raster I/O during the current session).
//' To change this value programmatically during operation of the program it is
//' better to use [set_cache_max()] (in which case, always given in bytes).
//'
//' @seealso
//' [GDAL_CACHEMAX configuration option](https://gdal.org/en/stable/user/configoptions.html#performance-and-caching)
//'
//' [get_config_option()], [set_config_option()], [get_usable_physical_ram()],
//' [get_cache_used()], [set_cache_max()]
//'
//' @examples
//' get_cache_max()
// [[Rcpp::export]]
Rcpp::NumericVector get_cache_max(std::string units = "MB") {
    int64_t nCacheMax = static_cast<int64_t>(GDALGetCacheMax64());
    std::vector<int64_t> ret = {-1};

    if (EQUAL(units.c_str(), "MB")) {
        ret[0] = nCacheMax / (1000 * 1000);
    }
    else if (EQUAL(units.c_str(), "GB")) {
        ret[0] = nCacheMax / (1000 * 1000 * 1000);
    }
    else if (EQUAL(units.c_str(), "KB")) {
        ret[0] = nCacheMax / (1000);
    }
    else if (EQUAL(units.c_str(), "") || EQUAL(units.c_str(), "B") ||
             EQUAL(units.c_str(), "bytes") || EQUAL(units.c_str(), "byte")) {

        ret[0] = nCacheMax;
    }
    else {
        Rcpp::stop("invalid value for 'units'");
    }

    return Rcpp::wrap(ret);
}


//' Get the size of memory in use by the GDAL block cache
//'
//' `get_cache_used()` returns the amount of memory currently in use for
//' GDAL block caching. Wrapper of `GDALGetCacheUsed64()` with return
//' value in MB by default.
//'
//' @param units Character string specifying units for the return value. One of
//' `"MB"` (the default), `"GB"`, `"KB"` or `"bytes"` (values of `"byte"`,
//' `"B"` and empty string `""` are also recognized to mean bytes).
//' @returns A numeric value carrying the `integer64` class attribute. Amount
//' of the available cache memory currently in use in the requested units.
//'
//' @seealso
//' [GDAL Block Cache](https://usdaforestservice.github.io/gdalraster/articles/gdal-block-cache.html)
//'
//' [get_cache_max()], [set_cache_max()]
//'
//' @examples
//' get_cache_used()
// [[Rcpp::export]]
Rcpp::NumericVector get_cache_used(std::string units = "MB") {
    int64_t nCacheUsed = static_cast<int64_t>(GDALGetCacheUsed64());
    std::vector<int64_t> ret = {-1};

    if (EQUAL(units.c_str(), "MB")) {
        ret[0] = nCacheUsed / (1000 * 1000);
    }
    else if (EQUAL(units.c_str(), "GB")) {
        ret[0] = nCacheUsed / (1000 * 1000 * 1000);
    }
    else if (EQUAL(units.c_str(), "KB")) {
        ret[0] = nCacheUsed / (1000);
    }
    else if (EQUAL(units.c_str(), "") || EQUAL(units.c_str(), "B") ||
             EQUAL(units.c_str(), "bytes") || EQUAL(units.c_str(), "byte")) {

        ret[0] = nCacheUsed;
    }
    else {
        Rcpp::stop("invalid value for 'units'");
    }

    return Rcpp::wrap(ret);
}


//' Set the maximum memory size for the GDAL block cache
//'
//' `set_cache_max()` sets the maximum amount of memory that GDAL is permitted
//' to use for GDALRasterBlock caching.
//' *The unit of the value to set is bytes.* Wrapper of `GDALSetCacheMax64()`.
//'
//' @param nbytes A numeric value optionally carrying the `integer64` class
//' attribute (assumed to be a whole number, will be coerced to integer by
//' truncation). Specifies the new cache size in bytes (maximum number of bytes
//' for caching).
//' @returns No return value, called for side effects.
//'
//' @note
//' **This function will not make any attempt to check the consistency of the
//' passed value with the effective capabilities of the OS.**
//'
//' It is recommended to consult the documentation for `get_cache_max()` and
//' `get_cache_used()` before using this function.
//'
//' [get_cache_max()], [get_cache_used()]
//'
//' @examples
//' (cachemax <- get_cache_max("bytes"))
//'
//' set_cache_max(1e8)
//' get_cache_max()  # returns in MB by default
//'
//' # reset to original
//' set_cache_max(cachemax)
//' get_cache_max()
// [[Rcpp::export]]
void set_cache_max(Rcpp::NumericVector nbytes) {
    if (nbytes.size() != 1)
        Rcpp::stop("'nbytes' must be a length-1 numeric vector");

    int64_t nbytes_in = -1;

    if (Rcpp::isInteger64(nbytes)) {
        nbytes_in = Rcpp::fromInteger64(nbytes[0]);
    }
    else {
        nbytes_in = static_cast<int64_t>(nbytes[0]);
    }

    if (nbytes_in < 0)
        Rcpp::stop("'nbytes' cannot be a negative number");

    GDALSetCacheMax64(nbytes_in);
}


//' @noRd
// [[Rcpp::export(name = ".dump_open_datasets")]]
int dump_open_datasets(const std::string &outfile) {
    FILE* fp = std::fopen(outfile.c_str(), "w");
    if (!fp)
        return -1;

    int ret = GDALDumpOpenDatasets(fp);
    std::fclose(fp);
    return ret;
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
void push_error_handler(const std::string &handler) {
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
//' 'filename' may be a physical file, URL, connection string, filename with
//' additional parameters, etc.
//' Currently, only checks for leading tilde and does path expasion in that
//' case. Returns the filename in UTF-8 encoding if possible using R enc2utf8.
//'
//' @noRd
// [[Rcpp::export(name = ".check_gdal_filename")]]
Rcpp::CharacterVector check_gdal_filename(
        const Rcpp::CharacterVector &filename) {

    /*
    We use Rcpp::CharacterVector since it may have marked encoding when needed.
    Rcpp::String may drop encoding or have issues with encoding on Windows:
    https://github.com/RcppCore/Rcpp/issues/988
    */

    if (filename.size() > 1)
        Rcpp::stop("'filename' must be a character vector of length 1");

    std::string std_filename(filename[0]);
    Rcpp::CharacterVector out_filename(1);

    if (STARTS_WITH(std_filename.c_str(), "~"))
        out_filename = path_expand_(filename);
    else
        out_filename = filename;

    return enc_to_utf8_(out_filename);
}


//' Get the number of processors detected by GDAL
//'
//' `get_num_cpus()` returns the number of processors detected by GDAL.
//' Wrapper of `CPLGetNumCPUs()` in the GDAL Common Portability Library.
//'
//' @return Integer scalar, number of CPUs.
//'
//' @examples
//' get_num_cpus()
// [[Rcpp::export(name = "get_num_cpus")]]
int get_num_cpus() {
    return CPLGetNumCPUs();
}


//' Get usable physical RAM reported by GDAL
//'
//' `get_usable_physical_ram()` returns the total physical RAM, usable by a
//' process, in bytes. It will limit to 2 GB for 32 bit processes. Starting
//' with GDAL 2.4.0, it will also take into account resource limits (virtual
//' memory) on Posix systems. Starting with GDAL 3.6.1, it will also take into
//' account RLIMIT_RSS on Linux. Wrapper of `CPLGetUsablePhysicalRAM()` in the
//' GDAL Common Portability Library.
//'
//' @return Numeric scalar, number of bytes as `bit64::integer64` type (or 0 in
//' case of failure).
//'
//' @note
//' This memory may already be partly used by other processes.
//'
//' @examples
//' get_usable_physical_ram()
// [[Rcpp::export(name = "get_usable_physical_ram")]]
Rcpp::NumericVector get_usable_physical_ram() {
    std::vector<int64_t> ret(1);
    ret[0] = CPLGetUsablePhysicalRAM();
    return Rcpp::wrap(ret);
}


//' Is SpatiaLite available?
//'
//' `has_spatialite()` returns a logical value indicating whether GDAL was
//' built with support for the SpatiaLite library. SpatiaLite extends the
//' SQLite core to support full Spatial SQL capabilities.
//'
//' @details
//' GDAL supports executing SQL statements against a datasource. For most file
//' formats (e.g. Shapefiles, GeoJSON, FlatGeobuf files), the built-in OGR SQL
//' dialect will be used by default. It is also possible to request the
//' alternate `"SQLite"`  dialect, which will use the SQLite engine to evaluate
//' commands on GDAL datasets. This assumes that GDAL is built with support for
//' SQLite, and preferably with Spatialite support too to benefit from spatial
//' functions.
//'
//' @return Logical scalar. `TRUE` if SpatiaLite is available to GDAL.
//'
//' @note
//' All GDAL/OGR drivers for database systems, e.g., PostgreSQL / PostGIS,
//' Oracle Spatial, SQLite / Spatialite RDBMS, GeoPackage, etc., override the
//' `GDALDataset::ExecuteSQL()` function with a dedicated implementation and, by
//' default, pass the SQL statements directly to the underlying RDBMS. In these
//' cases the SQL syntax varies in some particulars from OGR SQL. Also, anything
//' possible in SQL can then be accomplished for these particular databases. For
//' those drivers, it is also possible to explicitly request the `OGRSQL` or
//' `SQLite` dialects, although performance will generally be much less than the
//' native SQL engine of those database systems.
//'
//' @seealso
//' [ogrinfo()], [ogr_execute_sql()]
//'
//' OGR SQL dialect and SQLITE SQL dialect:\cr
//' \url{https://gdal.org/en/stable/user/ogr_sql_sqlite_dialect.html}
//'
//' @examples
//' has_spatialite()
// [[Rcpp::export]]
bool has_spatialite() {
    GDALDriverH hDriver = nullptr;
    hDriver = GDALGetDriverByName("SQLite");
    if (hDriver == nullptr)
        return false;

    const char *pszCO = nullptr;
    pszCO = GDALGetMetadataItem(hDriver, GDAL_DMD_CREATIONOPTIONLIST,
                                nullptr);

    if (pszCO == nullptr || std::strstr(pszCO, "SPATIALITE") == nullptr)
        return false;
    else
        return true;
}


//' Check if GDAL CPLHTTP services can be useful (libcurl)
//'
//' `http_enabled()` returns `TRUE` if `libcurl` support is enabled.
//' Wrapper of `CPLHTTPEnabled()` in the GDAL Common Portability Library.
//'
//' @return Logical scalar, `TRUE` if GDAL was built with `libcurl` support.
//'
//' @examples
//' http_enabled()
// [[Rcpp::export]]
bool http_enabled() {
    return static_cast<bool>(CPLHTTPEnabled());
}


//' @noRd
// [[Rcpp::export(name = ".cpl_get_filename")]]
std::string cpl_get_filename(const Rcpp::CharacterVector &full_filename) {
    std::string filename_in = Rcpp::as<std::string>(
            check_gdal_filename(full_filename));

    return std::string(CPLGetFilename(filename_in.c_str()));
}


//' @noRd
// [[Rcpp::export(name = ".cpl_get_basename")]]
std::string cpl_get_basename(const Rcpp::CharacterVector &full_filename) {
    std::string filename_in = Rcpp::as<std::string>(
            check_gdal_filename(full_filename));

    return std::string(CPLGetBasename(filename_in.c_str()));
}


//' @noRd
// [[Rcpp::export(name = ".cpl_get_extension")]]
std::string cpl_get_extension(const Rcpp::CharacterVector &full_filename) {
    std::string filename_in = Rcpp::as<std::string>(
            check_gdal_filename(full_filename));

    return std::string(CPLGetExtension(filename_in.c_str()));
}


//' @noRd
// [[Rcpp::export(name = ".cpl_http_cleanup")]]
void cpl_http_cleanup() {
    CPLHTTPCleanup();
}


// Create a new uninitialized raster
//
// `create()` makes an empty raster in the specified format.
//
// Implemented as a GDALRaster object factory registered in
// RCPP_MODULE(mod_GDALRaster), see src/gdalraster.cpp.
// Unique function signature based on number of parameters.
// Called in R with `ds <- new(GDALRaster, ...)` giving all 7 parameters,
// as in R/gdal_create.R.
GDALRaster *create(const std::string &format,
                   const Rcpp::CharacterVector &dst_filename,
                   int xsize, int ysize, int nbands,
                   const std::string &dataType,
                   const Rcpp::Nullable<Rcpp::CharacterVector> &options) {

    GDALDriverH hDriver = nullptr;
    hDriver = GDALGetDriverByName(format.c_str());
    if (hDriver == nullptr)
        Rcpp::stop("failed to get driver for the specified format");

    char **papszMetadata = GDALGetMetadata(hDriver, nullptr);
    if (!CPLFetchBool(papszMetadata, GDAL_DCAP_CREATE, FALSE))
        Rcpp::stop("driver does not support create");

    std::string dst_filename_in =
        Rcpp::as<std::string>(check_gdal_filename(dst_filename));

    GDALDataType dt = GDT_Unknown;
    dt = GDALGetDataTypeByName(dataType.c_str());
    if (dt == GDT_Unknown)
        Rcpp::stop("'dataType' is unknown");

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

    GDALRaster *ds = nullptr;
    ds = new GDALRaster();
    if (ds == nullptr) {
        GDALClose(hDstDS);
        Rcpp::stop("create() failed");
    }
    ds->setFilename(dst_filename_in);
    ds->setGDALDatasetH_(hDstDS, true);
    return ds;
}


// Create a copy of a raster
//
// `createCopy()` copies a raster dataset, optionally changing the format.
// The extent, cell size, number of bands, data type, projection, and
// geotransform are all copied from the source raster.
//
// Implemented as a GDALRaster object factory registered in
// RCPP_MODULE(mod_GDALRaster), see src/gdalraster.cpp.
// Unique function signature based on number of parameters.
// Called in R with `ds <- new(GDALRaster, ...)` giving all 6 parameters,
// as in R/gdal_create.R.
GDALRaster *createCopy(const std::string &format,
                       const Rcpp::CharacterVector &dst_filename,
                       const GDALRaster* const &src_ds, bool strict,
                       const Rcpp::Nullable<Rcpp::CharacterVector> &options,
                       bool quiet) {

    GDALDriverH hDriver = nullptr;
    hDriver = GDALGetDriverByName(format.c_str());
    if (hDriver == nullptr)
        Rcpp::stop("failed to get driver from format name");

    char **papszMetadata = GDALGetMetadata(hDriver, nullptr);
    if (!CPLFetchBool(papszMetadata, GDAL_DCAP_CREATECOPY, FALSE) &&
        !CPLFetchBool(papszMetadata, GDAL_DCAP_CREATE, FALSE)) {

        Rcpp::stop("driver does not support createCopy");
    }

    std::string dst_filename_in =
        Rcpp::as<std::string>(check_gdal_filename(dst_filename));

    if (!src_ds)
        Rcpp::stop("open source raster failed");

    GDALDatasetH hSrcDS = src_ds->getGDALDatasetH_();
    if (hSrcDS == nullptr)
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
                            quiet ? nullptr : GDALTermProgressR,
                            nullptr);

    if (hDstDS == nullptr)
        Rcpp::stop("createCopy() failed");

    GDALRaster *ds = nullptr;
    ds = new GDALRaster();
    if (ds == nullptr) {
        GDALClose(hDstDS);
        Rcpp::stop("create() failed");
    }
    ds->setFilename(dst_filename_in);
    ds->setGDALDatasetH_(hDstDS, true);
    return ds;
}


//' Apply geotransform - internal wrapper of GDALApplyGeoTransform()
//'
//' `apply_geotransform_()` applies geotransform coefficients to a raster
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
Rcpp::NumericVector apply_geotransform_(const std::vector<double> &gt,
                                        double pixel, double line) {

    double geo_x = 0;
    double geo_y = 0;
    GDALApplyGeoTransform((double *) (gt.data()), pixel, line, &geo_x, &geo_y);
    Rcpp::NumericVector geo_xy = {geo_x, geo_y};
    return geo_xy;
}


//' Apply geotransform (raster column/row to geospatial x/y)
//' input as geotransform vector, no bounds checking on col/row
//' @noRd
// [[Rcpp::export(name = ".apply_geotransform_gt")]]
Rcpp::NumericMatrix apply_geotransform_gt(const Rcpp::RObject &col_row,
                                          const std::vector<double> &gt) {

    Rcpp::NumericMatrix col_row_in = xy_robject_to_matrix_(col_row);

    if (col_row_in.nrow() == 0)
        Rcpp::stop("input matrix is empty");

    Rcpp::LogicalVector na_in = Rcpp::is_na(col_row_in.column(0)) |
                                Rcpp::is_na(col_row_in.column(1));

    Rcpp::NumericMatrix xy = Rcpp::no_init(col_row_in.nrow(), 2);
    for (R_xlen_t i = 0; i < col_row_in.nrow(); ++i) {
        if (na_in[i]) {
            xy(i, 0) = NA_REAL;
            xy(i, 1) = NA_REAL;
        }
        else {
            GDALApplyGeoTransform((double *) (gt.data()),
                                  col_row_in(i, 0), col_row_in(i, 1),
                                  &xy(i, 0), &xy(i, 1));
        }
    }

    return xy;
}


//' Apply geotransform (raster column/row to geospatial x/y)
//' alternate version for GDALRaster input, with bounds checking
//' @noRd
// [[Rcpp::export(name = ".apply_geotransform_ds")]]
Rcpp::NumericMatrix apply_geotransform_ds(const Rcpp::RObject &col_row,
                                          const GDALRaster* const &ds) {

    Rcpp::NumericMatrix col_row_in = xy_robject_to_matrix_(col_row);

    if (col_row_in.nrow() == 0)
        Rcpp::stop("input matrix is empty");

    Rcpp::LogicalVector na_in = Rcpp::is_na(col_row_in.column(0)) |
                                Rcpp::is_na(col_row_in.column(1));

    std::vector<double> gt = ds->getGeoTransform();
    Rcpp::NumericMatrix xy = Rcpp::no_init(col_row_in.nrow(), 2);
    uint64_t num_outside = 0;
    for (R_xlen_t i = 0; i < col_row_in.nrow(); ++i) {
        if (na_in[i]) {
            xy(i, 0) = NA_REAL;
            xy(i, 1) = NA_REAL;
        }
        else if (col_row_in(i, 0) < 0 || col_row_in(i, 1) < 0 ||
                 col_row_in(i, 0) > ds->getRasterXSize() ||
                 col_row_in(i, 1) > ds->getRasterYSize()) {

            num_outside += 1;
            xy(i, 0) = NA_REAL;
            xy(i, 1) = NA_REAL;
        }
        else {
            GDALApplyGeoTransform((double *) (gt.data()),
                                  col_row_in(i, 0), col_row_in(i, 1),
                                  &xy(i, 0), &xy(i, 1));
        }
    }

    if (num_outside > 0)
        Rcpp::warning(std::to_string(num_outside) +
                " coordinates(s) were outside the raster extent, NA returned");

    return xy;
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
//' invgt <- ds$getGeoTransform() |> inv_geotransform()
//' ds$close()
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
Rcpp::NumericVector inv_geotransform(const std::vector<double> &gt) {
    std::vector<double> gt_inv(6);
    if (GDALInvGeoTransform((double *) (gt.data()), gt_inv.data()))
        return Rcpp::wrap(gt_inv);
    else
        return Rcpp::NumericVector(6, NA_REAL);
}


//' Raster pixel/line from geospatial x,y coordinates
//' input is gt vector, no bounds checking done on output
//' @noRd
// [[Rcpp::export(name = ".get_pixel_line_gt")]]
Rcpp::IntegerMatrix get_pixel_line_gt(const Rcpp::RObject &xy,
        const std::vector<double> &gt) {

    Rcpp::NumericMatrix xy_in = xy_robject_to_matrix_(xy);

    if (xy_in.nrow() == 0)
        Rcpp::stop("input matrix is empty");

    Rcpp::LogicalVector na_in = Rcpp::is_na(xy_in.column(0)) |
                                Rcpp::is_na(xy_in.column(1));

    Rcpp::NumericVector inv_gt = inv_geotransform(gt);
    if (Rcpp::any(Rcpp::is_na(inv_gt)))
        Rcpp::stop("could not get inverse geotransform");
    Rcpp::IntegerMatrix pixel_line = Rcpp::no_init(xy_in.nrow(), 2);
    for (R_xlen_t i = 0; i < xy_in.nrow(); ++i) {
        if (na_in[i]) {
            pixel_line(i, 0) = NA_INTEGER;
            pixel_line(i, 1) = NA_INTEGER;
        }
        else {
            double geo_x = xy_in(i, 0);
            double geo_y = xy_in(i, 1);
            // column
            pixel_line(i, 0) = static_cast<int>(std::floor(inv_gt[0] +
                                                inv_gt[1] * geo_x +
                                                inv_gt[2] * geo_y));
            // row
            pixel_line(i, 1) = static_cast<int>(std::floor(inv_gt[3] +
                                                inv_gt[4] * geo_x +
                                                inv_gt[5] * geo_y));
        }
    }
    return pixel_line;
}


//' Raster pixel/line from geospatial x,y coordinates
//' alternate version for GDALRaster input, with bounds checking
//' @noRd
// [[Rcpp::export(name = ".get_pixel_line_ds")]]
Rcpp::IntegerMatrix get_pixel_line_ds(const Rcpp::RObject& xy,
                                      const GDALRaster* const &ds) {

    Rcpp::NumericMatrix xy_in = xy_robject_to_matrix_(xy);

    if (xy_in.nrow() == 0)
        Rcpp::stop("input matrix is empty");

    Rcpp::LogicalVector na_in = Rcpp::is_na(xy_in.column(0)) |
                                Rcpp::is_na(xy_in.column(1));

    std::vector<double> gt = ds->getGeoTransform();
    Rcpp::NumericVector inv_gt = inv_geotransform(gt);
    if (Rcpp::any(Rcpp::is_na(inv_gt)))
        Rcpp::stop("could not get inverse geotransform");
    Rcpp::IntegerMatrix pixel_line = Rcpp::no_init(xy_in.nrow(), 2);
    uint64_t num_outside = 0;
    for (R_xlen_t i = 0; i < xy_in.nrow(); ++i) {
        if (na_in[i]) {
            pixel_line(i, 0) = NA_INTEGER;
            pixel_line(i, 1) = NA_INTEGER;
        }
        else {
            double geo_x = xy_in(i, 0);
            double geo_y = xy_in(i, 1);
            // column
            pixel_line(i, 0) = static_cast<int>(std::floor(inv_gt[0] +
                                                inv_gt[1] * geo_x +
                                                inv_gt[2] * geo_y));
            // row
            pixel_line(i, 1) = static_cast<int>(std::floor(inv_gt[3] +
                                                inv_gt[4] * geo_x +
                                                inv_gt[5] * geo_y));

            if (pixel_line(i, 0) < 0 || pixel_line(i, 1) < 0 ||
                    pixel_line(i, 0) >= ds->getRasterXSize() ||
                    pixel_line(i, 1) >= ds->getRasterYSize()) {

                num_outside += 1;
                pixel_line(i, 0) = NA_INTEGER;
                pixel_line(i, 1) = NA_INTEGER;
            }
        }
    }

    if (num_outside > 0)
        Rcpp::warning(std::to_string(num_outside) +
                " point(s) were outside the raster extent, NA returned");

    return pixel_line;
}


//' Returns bbox geospatial x,y coordinates (xmin, ymin, xmax, ymax) from
//' inputs of geotransform vector and the grid pixel/line extent
//' @noRd
// [[Rcpp::export(name = ".bbox_grid_to_geo")]]
std::vector<double> bbox_grid_to_geo_(const std::vector<double> &gt,
                                      double grid_xmin, double grid_xmax,
                                      double grid_ymin, double grid_ymax) {

    // {ul, ll, ur, lr}
    Rcpp::NumericVector corners_x = {NA_REAL, NA_REAL, NA_REAL, NA_REAL};
    Rcpp::NumericVector corners_y = {NA_REAL, NA_REAL, NA_REAL, NA_REAL};

    // ul
    corners_x[0] = gt[0] + gt[1] * grid_xmin + gt[2] * grid_ymax;
    corners_y[0] = gt[3] + gt[4] * grid_xmin + gt[5] * grid_ymax;

    // ll
    corners_x[1] = gt[0] + gt[1] * grid_xmin + gt[2] * grid_ymin;
    corners_y[1] = gt[3] + gt[4] * grid_xmin + gt[5] * grid_ymin;

    // ur
    corners_x[2] = gt[0] + gt[1] * grid_xmax + gt[2] * grid_ymax;
    corners_y[2] = gt[3] + gt[4] * grid_xmax + gt[5] * grid_ymax;

    // lr
    corners_x[3] = gt[0] + gt[1] * grid_xmax + gt[2] * grid_ymin;
    corners_y[3] = gt[3] + gt[4] * grid_xmax + gt[5] * grid_ymin;

    std::vector<double> ret = {Rcpp::min(corners_x), Rcpp::min(corners_y),
                               Rcpp::max(corners_x), Rcpp::max(corners_y)};

    return ret;
}


//' Flip raster data vertically
//' @noRd
// [[Rcpp::export(name = ".flip_vertical")]]
Rcpp::NumericVector flip_vertical(const Rcpp::NumericVector &v,
                                  int xsize, int ysize, int nbands) {

    // input pixels are interleaved by band
    // each band contains a vector of xsize * ysize pixel values
    // reverses the order of the rows in each band

    if (v.size() == 0)
        Rcpp::stop("the input vector is empty");

    if (xsize < 1 || ysize < 1 || nbands < 1)
        Rcpp::stop("invalid raster dimensions");

    if (v.size() != xsize * ysize * nbands)
        Rcpp::stop("invalid raster dimensions");

    Rcpp::NumericVector out(v.size());

    const size_t num_pixels = static_cast<size_t>(xsize) * ysize;
    for (int b = 0; b < nbands; ++b) {
        const size_t band_offset = b * num_pixels;
        for (int line = 0; line < ysize; ++line) {
            const size_t line_offset = band_offset +
                                       line * static_cast<size_t>(xsize);

            const size_t dst_offset = band_offset + num_pixels -
                                      ((line + 1) * static_cast<size_t>(xsize));

            std::copy_n(v.cbegin() + line_offset, xsize,
                        out.begin() + dst_offset);
        }
    }

    return out;
}


// Create a virtual warped dataset automatically
//
// `autoCreateWarpedVRT()` creates a warped virtual dataset representing the
// input raster warped into a target coordinate system. The output virtual
// dataset will be "north-up" in the target coordinate system. GDAL
// automatically determines the bounds and resolution of the output virtual
// raster which should be large enough to include all the input raster.
// Wrapper of `GDALAutoCreateWarpedVRT()` in the GDAL Warper API.
//
// Implemented as a GDALRaster object factory registered in
// RCPP_MODULE(mod_GDALRaster), see src/gdalraster.cpp.
// Unique function signature based on number of parameters.
// Called in R with `ds <- new(GDALRaster, ...)` giving all 8 parameters,
// as in R/gdal_create.R.
GDALRaster *autoCreateWarpedVRT(const GDALRaster* const &src_ds,
                                const std::string &dst_wkt,
                                const std::string &resample_alg,
                                const std::string &src_wkt,
                                double max_err, bool alpha_band,
                                bool reserved1, bool reserved2) {

    GDALDatasetH hSrcDS = src_ds->getGDALDatasetH_();
    if (hSrcDS == nullptr)
        Rcpp::stop("source dataset is not open");

    GDALResampleAlg eResampleAlg;
    if (EQUAL(resample_alg.c_str(), "NearestNeighbour"))
        eResampleAlg = GRA_NearestNeighbour;
    else if (EQUAL(resample_alg.c_str(), "Bilinear"))
        eResampleAlg = GRA_Bilinear;
    else if (EQUAL(resample_alg.c_str(), "Cubic"))
        eResampleAlg = GRA_Cubic;
    else if (EQUAL(resample_alg.c_str(), "CubicSpline"))
        eResampleAlg = GRA_CubicSpline;
    else if (EQUAL(resample_alg.c_str(), "Lanczos"))
        eResampleAlg = GRA_Lanczos;
    else if (EQUAL(resample_alg.c_str(), "Average"))
        eResampleAlg = GRA_Average;
    else if (EQUAL(resample_alg.c_str(), "RMS"))
        eResampleAlg = GRA_RMS;
    else if (EQUAL(resample_alg.c_str(), "Mode"))
        eResampleAlg = GRA_Mode;
    else
        Rcpp::stop("'resample_alg' is not valid");

    const char *pszDstWKT = nullptr;
    if (dst_wkt != "")
        pszDstWKT = dst_wkt.c_str();

    const char *pszSrcWKT = nullptr;
    if (src_wkt != "")
        pszSrcWKT = src_wkt.c_str();

    GDALWarpOptions *psOptions = nullptr;
    if (alpha_band) {
        psOptions = GDALCreateWarpOptions();
        psOptions->nDstAlphaBand = src_ds->getRasterCount() + 1;
    }

    GDALDatasetH hWarpedDS = nullptr;
    hWarpedDS = GDALAutoCreateWarpedVRT(hSrcDS, pszSrcWKT, pszDstWKT,
                                        eResampleAlg, max_err, psOptions);

    if (psOptions != nullptr)
        GDALDestroyWarpOptions(psOptions);

    if (hWarpedDS == nullptr)
        Rcpp::stop("GDALAutoCreateWarpedVRT() returned NULL on error");

    GDALRaster *ds = new GDALRaster;
    ds->setFilename("");
    ds->setGDALDatasetH_(hWarpedDS, true);
    return ds;
}


//' Build a GDAL virtual raster from a list of datasets
//'
//' `buildVRT()` is a wrapper of the \command{gdalbuildvrt} command-line
//' utility for building a VRT (Virtual Dataset) that is a mosaic of the list
//' of input GDAL datasets
//' (see \url{https://gdal.org/en/stable/programs/gdalbuildvrt.html}).
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
//' vrt_file <- file.path(tempdir(), "storml_b6_b5_b4.vrt")
//' buildVRT(vrt_file, band_files, cl_arg = "-separate")
//' ds <- new(GDALRaster, vrt_file)
//' ds$getRasterCount()
//' plot_raster(ds, nbands = 3, main = "Landsat 6-5-4 (vegetative analysis)")
//' ds$close()
//' \dontshow{vsi_unlink(vrt_file)}
// [[Rcpp::export(invisible = true)]]
bool buildVRT(const Rcpp::CharacterVector &vrt_filename,
              const Rcpp::CharacterVector &input_rasters,
              const Rcpp::Nullable<Rcpp::CharacterVector> &cl_arg = R_NilValue,
              bool quiet = false) {

    std::string vrt_filename_in =
        Rcpp::as<std::string>(check_gdal_filename(vrt_filename));

    std::vector<std::string> input_rasters_in(input_rasters.size());
    std::vector<const char *> src_ds_files(input_rasters.size() + 1);
    for (R_xlen_t i = 0; i < input_rasters.size(); ++i) {
        input_rasters_in[i] = Rcpp::as<std::string>(
                check_gdal_filename(
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
Rcpp::DataFrame combine(const Rcpp::CharacterVector &src_files,
                        const Rcpp::CharacterVector &var_names,
                        const std::vector<int> &bands,
                        const std::string &dst_filename,
                        const std::string &fmt, const std::string &dataType,
                        const Rcpp::Nullable<Rcpp::CharacterVector> &options,
                        bool quiet) {

    R_xlen_t nrasters = src_files.size();
    std::vector<std::unique_ptr<GDALRaster>> src_ds(nrasters);
    int nrows = 0;
    int ncols = 0;
    std::vector<double> gt = {0, 1, 0, 0, 0, 1};
    std::string srs = "";
    std::unique_ptr<GDALRaster> dst_ds{};
    bool out_raster = false;

    if (nrasters != var_names.size() || nrasters !=  (R_xlen_t) bands.size()) {
        Rcpp::stop("'src_files', 'var_names', 'bands' must have same length");
    }

    if (dst_filename != "") {
        out_raster = true;
        if (fmt == "")
            Rcpp::stop("format of output raster must be specified");
    }

    for (R_xlen_t i = 0; i < nrasters; ++i) {
        src_ds[i] = std::make_unique<GDALRaster>(std::string(src_files[i]));
        // use the first raster as reference
        if (i == 0) {
            nrows = static_cast<int>(src_ds[i]->getRasterYSize());
            ncols = static_cast<int>(src_ds[i]->getRasterXSize());
            gt = src_ds[i]->getGeoTransform();
            srs = src_ds[i]->getProjectionRef();
        }
    }

    if (out_raster) {
        dst_ds = std::unique_ptr<GDALRaster>(
            create(fmt, dst_filename, ncols, nrows, 1, dataType, options));

        if (!dst_ds->setGeoTransform(gt))
            Rcpp::warning("failed to set output geotransform");
        if (!dst_ds->setProjection(srs))
            Rcpp::warning("failed to set output projection");
        // } else {
        //     for (std::size_t i = 0; i < nrasters; ++i)
        //         src_ds[i].close();
        //     Rcpp::stop("failed to create output raster");
        // }
    }

    auto tbl = std::make_unique<CmbTable>(nrasters, var_names);
    Rcpp::IntegerMatrix rowdata(nrasters, ncols);
    Rcpp::NumericVector cmbid;
    GDALProgressFunc pfnProgress = GDALTermProgressR;
    void *pProgressData = nullptr;

    if (!quiet) {
        if (nrasters == 1)
            Rcpp::Rcout << "scanning raster...\n";
        else
            Rcpp::Rcout << "combining " << nrasters << " rasters...\n";
    }

    for (int y = 0; y < nrows; ++y) {
        for (R_xlen_t i = 0; i < nrasters; ++i) {
            rowdata.row(i) = Rcpp::as<Rcpp::IntegerVector>(
                src_ds[i]->read(bands[i], 0, y, ncols, 1, ncols, 1));
        }

        cmbid = tbl->updateFromMatrix(rowdata, 1);

        if (out_raster) {
            dst_ds->write(1, 0, y, ncols, 1, cmbid);
        }

        if (!quiet) {
            pfnProgress(y / (nrows-1.0), nullptr, pProgressData);
        }
    }

    if (out_raster)
        dst_ds->close();

    for (R_xlen_t i = 0; i < nrasters; ++i)
        src_ds[i]->close();

    return tbl->asDataFrame();
}


//' Compute for a raster band the set of unique pixel values and their counts
//'
//' @noRd
// [[Rcpp::export(name = ".value_count")]]
Rcpp::DataFrame value_count(const GDALRaster* const &src_ds, int band = 1,
                            bool quiet = false) {

    int nrows = static_cast<int>(src_ds->getRasterYSize());
    int ncols = static_cast<int>(src_ds->getRasterXSize());
    GDALProgressFunc pfnProgress = nullptr;
    void *pProgressData = nullptr;
    if (!quiet)
        pfnProgress = GDALTermProgressR;

    Rcpp::DataFrame df_out = Rcpp::DataFrame::create();

    if (!quiet)
        Rcpp::Rcout << "scanning raster...\n";

    if (src_ds->readableAsInt_(band)) {
        // read pixel values as int
        Rcpp::IntegerVector rowdata(ncols);
        std::unordered_map<int, double> tbl;
        for (int y = 0; y < nrows; ++y) {
            rowdata = Rcpp::as<Rcpp::IntegerVector>(
                            src_ds->read(band, 0, y, ncols, 1, ncols, 1));
            for (auto const& i : rowdata)
                tbl[i] += 1.0;
            if (!quiet)
                pfnProgress(y / (nrows-1.0), nullptr, pProgressData);
        }
        Rcpp::IntegerVector value(tbl.size());
        Rcpp::NumericVector count(tbl.size());
        std::size_t this_idx = 0;
        for (auto iter = tbl.begin(); iter != tbl.end(); ++iter) {
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
                            src_ds->read(band, 0, y, ncols, 1, ncols, 1));
            for (auto const& i : rowdata)
                tbl[i] += 1.0;
            if (!quiet)
                pfnProgress(y / (nrows-1.0), nullptr, pProgressData);
        }
        Rcpp::NumericVector value(tbl.size());
        Rcpp::NumericVector count(tbl.size());
        std::size_t this_idx = 0;
        for (auto iter = tbl.begin(); iter != tbl.end(); ++iter) {
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
bool dem_proc(const std::string &mode,
              const Rcpp::CharacterVector &src_filename,
              const Rcpp::CharacterVector &dst_filename,
              const Rcpp::Nullable<Rcpp::CharacterVector> &cl_arg = R_NilValue,
              const Rcpp::Nullable<Rcpp::String> &col_file = R_NilValue,
              bool quiet = false) {

    std::string src_filename_in;
    src_filename_in = Rcpp::as<std::string>(check_gdal_filename(src_filename));
    std::string dst_filename_in;
    dst_filename_in = Rcpp::as<std::string>(check_gdal_filename(dst_filename));

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

    GDALDEMProcessingOptions* psOptions = nullptr;
    psOptions = GDALDEMProcessingOptionsNew(argv.data(), nullptr);
    if (psOptions == nullptr)
        Rcpp::stop("DEM processing failed (could not create options struct)");
    if (!quiet)
        GDALDEMProcessingOptionsSetProgress(psOptions, GDALTermProgressR,
                                            nullptr);

    GDALDatasetH hDstDS = nullptr;
    if (col_file.isNotNull()) {
        Rcpp::String col_file_in(col_file);
        hDstDS = GDALDEMProcessing(dst_filename_in.c_str(), src_ds,
                                   mode.c_str(), col_file_in.get_cstring(),
                                   psOptions, nullptr);
    } else {
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
//' ## fill nodata edge pixels
//' f <- system.file("extdata/storml_elev_orig.tif", package="gdalraster")
//'
//' ## get count of nodata
//' tbl <- buildRAT(f)
//' head(tbl)
//' tbl[is.na(tbl$VALUE),]
//'
//' ds <- new(GDALRaster, f)
//' plot_raster(ds, legend = TRUE)
//' ds$close()
//'
//' ## make a copy that will be modified
//' mod_file <- file.path(tempdir(), "storml_elev_fill.tif")
//' file.copy(f,  mod_file)
//'
//' fillNodata(mod_file, band = 1)
//'
//' mod_tbl = buildRAT(mod_file)
//' head(mod_tbl)
//' mod_tbl[is.na(mod_tbl$VALUE),]
//'
//' ds <- new(GDALRaster, mod_file)
//' plot_raster(ds, legend = TRUE)
//' ds$close()
//' \dontshow{deleteDataset(mod_file)}
// [[Rcpp::export(invisible = true)]]
bool fillNodata(const Rcpp::CharacterVector &filename, int band,
                const Rcpp::CharacterVector &mask_file = "",
                double max_dist = 100, int smooth_iterations = 0,
                bool quiet = false) {

    GDALDatasetH hDS = nullptr;
    GDALRasterBandH hBand = nullptr;
    GDALDatasetH hMaskDS = nullptr;
    GDALRasterBandH hMaskBand = nullptr;
    CPLErr err = CE_None;

    std::string filename_in;
    filename_in = Rcpp::as<std::string>(check_gdal_filename(filename));
    std::string mask_file_in;
    mask_file_in = Rcpp::as<std::string>(check_gdal_filename(mask_file));

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
//' utility (see \url{https://gdal.org/en/stable/programs/gdal_footprint.html}).
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
//' out_file <- file.path(tempdir(), "storml.geojson")
//'
//' # Requires GDAL >= 3.8
//' if (gdal_version_num() >= gdal_compute_version(3, 8, 0)) {
//'   # command-line arguments for gdal_footprint
//'   args <- c("-t_srs", "EPSG:4326")
//'   footprint(evt_file, out_file, args)
//'   \dontshow{deleteDataset(out_file)}
//' }
// [[Rcpp::export(invisible = true)]]
bool footprint(const Rcpp::CharacterVector &src_filename,
               const Rcpp::CharacterVector &dst_filename,
               const Rcpp::Nullable<Rcpp::CharacterVector>
                        &cl_arg = R_NilValue) {

#if GDAL_VERSION_NUM < 3080000
    Rcpp::stop("footprint() requires GDAL >= 3.8");

#else
    std::string src_filename_in;
    src_filename_in = Rcpp::as<std::string>(check_gdal_filename(src_filename));
    std::string dst_filename_in;
    dst_filename_in = Rcpp::as<std::string>(check_gdal_filename(dst_filename));

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
//' utility (see \url{https://gdal.org/en/stable/programs/ogr2ogr.html}).
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
//' @param open_options Optional character vector of dataset open options.
//' @returns Logical indicating success (invisible \code{TRUE}).
//' An error is raised if the operation fails.
//'
//' @note
//' For progress reporting, see command-line argument `-progress`: Display
//' progress on terminal. Only works if input layers have the "fast feature
//' count" capability.
//'
//' @seealso
//' [ogrinfo()], the [ogr_manage] utilities
//'
//' [translate()] for raster data
//'
//' @examples
//' src <- system.file("extdata/ynp_fires_1984_2022.gpkg", package="gdalraster")
//'
//' # Convert GeoPackage to Shapefile
//' ynp_shp <- file.path(tempdir(), "ynp_fires.shp")
//' ogr2ogr(src, ynp_shp, src_layers = "mtbs_perims")
//'
//' # Reproject to WGS84
//' ynp_gpkg <- file.path(tempdir(), "ynp_fires.gpkg")
//' args <- c("-t_srs", "EPSG:4326", "-nln", "fires_wgs84")
//' ogr2ogr(src, ynp_gpkg, cl_arg = args)
//'
//' # Clip to a bounding box (xmin, ymin, xmax, ymax in the source SRS)
//' # This will select features whose geometry intersects the bounding box.
//' # The geometries themselves will not be clipped unless "-clipsrc" is
//' # specified.
//' # The source SRS can be overridden with "-spat_srs" "<srs_def>"
//' # Using -update mode to write a new layer in the existing DSN
//' bb <- c(469685.97, 11442.45, 544069.63, 85508.15)
//' args <- c("-update", "-nln", "fires_clip", "-spat", bb)
//' ogr2ogr(src, ynp_gpkg, cl_arg = args)
//'
//' # Filter features by a -where clause
//' sql <- "ig_year >= 2000 ORDER BY ig_year"
//' args <- c("-update", "-nln", "fires_2000-2020", "-where", sql)
//' ogr2ogr(src, ynp_gpkg, src_layers = "mtbs_perims", cl_arg = args)
//'
//' # Dissolve features based on a shared attribute value
//' if (has_spatialite()) {
//'     sql <- "SELECT ig_year, ST_Union(geom) AS geom FROM mtbs_perims GROUP BY ig_year"
//'     args <- c("-update", "-sql", sql, "-dialect", "SQLITE")
//'     args <- c(args, "-nlt", "MULTIPOLYGON", "-nln", "dissolved_on_year")
//'     ogr2ogr(src, ynp_gpkg, cl_arg = args)
//' }
//' \dontshow{deleteDataset(ynp_shp)}
//' \dontshow{deleteDataset(ynp_gpkg)}
// [[Rcpp::export(invisible = true)]]
bool ogr2ogr(const Rcpp::CharacterVector &src_dsn,
             const Rcpp::CharacterVector &dst_dsn,
             const Rcpp::Nullable<Rcpp::CharacterVector> &src_layers =
                    R_NilValue,
             const Rcpp::Nullable<Rcpp::CharacterVector> &cl_arg = R_NilValue,
             const Rcpp::Nullable<Rcpp::CharacterVector> &open_options =
                    R_NilValue) {

    std::string src_dsn_in;
    src_dsn_in = Rcpp::as<std::string>(check_gdal_filename(src_dsn));
    std::string dst_dsn_in;
    dst_dsn_in = Rcpp::as<std::string>(check_gdal_filename(dst_dsn));

    // only 1 source dataset is supported currently by GDALVectorTranslate(),
    // but takes a list of datasets as input
    std::vector<GDALDatasetH> src_ds(1);
    bool ret = false;

    std::vector<char *> dsoo;
    if (open_options.isNotNull()) {
        Rcpp::CharacterVector open_options_in(open_options);
        for (R_xlen_t i = 0; i < open_options_in.size(); ++i) {
            dsoo.push_back((char *) open_options_in[i]);
        }
    }
    dsoo.push_back(nullptr);

    src_ds[0] = GDALOpenEx(src_dsn_in.c_str(), GDAL_OF_VECTOR,
                           nullptr, dsoo.data(), nullptr);

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
    else {
        Rcpp::Rcerr << CPLGetLastErrorMsg() << std::endl;
    }

    GDALReleaseDataset(src_ds[0]);

    if (!ret)
        Rcpp::stop("vector translate failed");

    return ret;
}


//' Retrieve information about a vector data source
//'
//' `ogrinfo()` is a wrapper of the \command{ogrinfo} command-line
//' utility (see \url{https://gdal.org/en/stable/programs/ogrinfo.html}).
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
//' [ogr2ogr()], [ogr_manage]
//'
//' @examplesIf gdal_version_num() >= gdal_compute_version(3, 7, 0)
//' src <- system.file("extdata/ynp_fires_1984_2022.gpkg", package="gdalraster")
//'
//' # Get the names of the layers in a GeoPackage file
//' ogrinfo(src)
//'
//' # Summary of a layer
//' ogrinfo(src, "mtbs_perims")
//'
//' # Query an attribute to restrict the output of the features in a layer
//' args <- c("-ro", "-nomd", "-where", "ig_year = 2020")
//' ogrinfo(src, "mtbs_perims", args)
//'
//' # Copy to a temporary in-memory file that is writeable
//' src_mem <- paste0("/vsimem/", basename(src))
//' vsi_copy_file(src, src_mem)
//'
//' # Add a column to a layer
//' args <- c("-sql", "ALTER TABLE mtbs_perims ADD burn_bnd_ha float")
//' ogrinfo(src_mem, cl_arg = args, read_only = FALSE)
//'
//' # Update values of the column with SQL and specify a dialect
//' sql <- "UPDATE mtbs_perims SET burn_bnd_ha = (burn_bnd_ac / 2.471)"
//' args <- c("-dialect", "sqlite", "-sql", sql)
//' ogrinfo(src_mem, cl_arg = args, read_only = FALSE)
// [[Rcpp::export(invisible = true)]]
Rcpp::String ogrinfo(const Rcpp::CharacterVector &dsn,
                     const Rcpp::Nullable<Rcpp::CharacterVector> &layers =
                            R_NilValue,
                     const Rcpp::Nullable<Rcpp::CharacterVector> &cl_arg =
                            Rcpp::CharacterVector::create("-so", "-nomd"),
                     const Rcpp::Nullable<Rcpp::CharacterVector> &open_options =
                            R_NilValue,
                     bool read_only = true,
                     bool cout = true) {

#if GDAL_VERSION_NUM < 3070000
    Rcpp::stop("ogrinfo() requires GDAL >= 3.7");
#else
    std::string dsn_in;
    dsn_in = Rcpp::as<std::string>(check_gdal_filename(dsn));

    GDALDatasetH src_ds = nullptr;

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

    bool as_json = false;
    std::vector<char *> argv;
    if (cl_arg.isNotNull()) {
        Rcpp::CharacterVector cl_arg_in(cl_arg);
        for (R_xlen_t i = 0; i < cl_arg_in.size(); ++i) {
            argv.push_back((char *) cl_arg_in[i]);
            if (EQUAL(cl_arg_in[i], "-json")) {
                as_json = true;
            }
        }
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

    Rcpp::String info_out = "";
    char *pszInfo = GDALVectorInfo(src_ds, psOptions);
    if (pszInfo != nullptr)
        info_out = pszInfo;

    GDALVectorInfoOptionsFree(psOptions);
    GDALReleaseDataset(src_ds);

    if (cout)
        Rcpp::Rcout << info_out.get_cstring();

    if (as_json)
        info_out.replace_all("\n", " ");

    CPLFree(pszInfo);

    return info_out;
#endif
}


//' Wrapper for GDALPolygonize in the GDAL Algorithms C API
//'
//' Called from and documented in R/gdalraster_proc.R
//' @noRd
// [[Rcpp::export(name = ".polygonize")]]
bool polygonize(const Rcpp::CharacterVector &src_filename, int src_band,
                const Rcpp::CharacterVector &out_dsn,
                const std::string &out_layer, const std::string &fld_name,
                const Rcpp::CharacterVector &mask_file = "",
                bool nomask = false, int connectedness = 4,
                bool quiet = false) {

    GDALDatasetH hSrcDS = nullptr;
    GDALRasterBandH hSrcBand = nullptr;
    GDALDatasetH hMaskDS = nullptr;
    GDALRasterBandH hMaskBand = nullptr;
    GDALDatasetH hOutDS = nullptr;
    OGRLayerH hOutLayer = nullptr;
    int iPixValField = -1;

    std::string src_filename_in;
    src_filename_in = Rcpp::as<std::string>(check_gdal_filename(src_filename));
    std::string out_dsn_in;
    out_dsn_in = Rcpp::as<std::string>(check_gdal_filename(out_dsn));
    std::string mask_file_in;
    mask_file_in = Rcpp::as<std::string>(check_gdal_filename(mask_file));

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
    } else if (mask_file_in == "" && nomask == true) {
        // do not use default validity mask for source band (such as nodata)
        hMaskBand = nullptr;
    } else {
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

    iPixValField = ogr_field_index(out_dsn_in, out_layer, fld_name);
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
bool rasterize(const std::string &src_dsn, const std::string &dst_filename,
               Rcpp::List dst_dataset, const Rcpp::CharacterVector &cl_arg,
               bool quiet = false) {

    GDALDatasetH hSrcDS = nullptr;
    hSrcDS = GDALOpenEx(src_dsn.c_str(), GDAL_OF_VECTOR,
                        nullptr, nullptr, nullptr);

    if (hSrcDS == nullptr)
        Rcpp::stop("failed to open vector data source");

    GDALRaster *dst_dataset_in = nullptr;
    if (dst_filename == "" && dst_dataset.size() == 1)
        dst_dataset_in = dst_dataset[0];
    else if (dst_filename == "" && dst_dataset.size() != 1)
        Rcpp::stop("invalid specification of destination raster");

    if (dst_filename == "" && dst_dataset_in->getGDALDatasetH_() == nullptr)
        Rcpp::stop("destination raster is 'nullptr'");

    std::vector<char *> argv(cl_arg.size() + 1);
    for (R_xlen_t i = 0; i < cl_arg.size(); ++i) {
        argv[i] = (char *) cl_arg[i];
    }
    argv[cl_arg.size()] = nullptr;

    GDALRasterizeOptions* psOptions = nullptr;
    psOptions = GDALRasterizeOptionsNew(argv.data(), nullptr);
    if (psOptions == nullptr)
        Rcpp::stop("rasterize failed (could not create options struct)");
    if (!quiet)
        GDALRasterizeOptionsSetProgress(psOptions, GDALTermProgressR, nullptr);

    GDALDatasetH hDstDS = nullptr;
    if (dst_dataset_in) {
        hDstDS = GDALRasterize(nullptr, dst_dataset_in->getGDALDatasetH_(),
                               hSrcDS, psOptions, nullptr);
    }
    else {
        hDstDS = GDALRasterize(dst_filename.c_str(), nullptr, hSrcDS,
                               psOptions, nullptr);
    }

    GDALRasterizeOptionsFree(psOptions);
    GDALReleaseDataset(hSrcDS);
    if (hDstDS == nullptr)
        Rcpp::stop("rasterize failed");

    if (!dst_dataset_in)
        GDALClose(hDstDS);
    else
        dst_dataset_in->flushCache();

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
//' evt_mmu_file <- file.path(tempdir(), "storml_evt_mmu2.tif")
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
//' \dontshow{deleteDataset(mask_file)}
//' \dontshow{deleteDataset(evt_mmu_file)}
// [[Rcpp::export(invisible = true)]]
bool sieveFilter(const Rcpp::CharacterVector &src_filename, int src_band,
                 const Rcpp::CharacterVector &dst_filename, int dst_band,
                 int size_threshold, int connectedness,
                 const Rcpp::CharacterVector &mask_filename = "",
                 int mask_band = 0,
                 const Rcpp::Nullable<Rcpp::CharacterVector> &options =
                        R_NilValue,
                 bool quiet = false) {

    GDALDatasetH hSrcDS = nullptr;
    GDALRasterBandH hSrcBand = nullptr;
    GDALDatasetH hMaskDS = nullptr;
    GDALRasterBandH hMaskBand = nullptr;
    GDALDatasetH hDstDS = nullptr;
    GDALRasterBandH hDstBand = nullptr;
    bool in_place = false;
    CPLErr err = CE_None;

    std::string src_filename_in;
    src_filename_in = Rcpp::as<std::string>(check_gdal_filename(src_filename));
    std::string dst_filename_in;
    dst_filename_in = Rcpp::as<std::string>(check_gdal_filename(dst_filename));
    std::string mask_file_in;
    mask_file_in = Rcpp::as<std::string>(check_gdal_filename(mask_filename));

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
//' utility (see \url{https://gdal.org/en/stable/programs/gdal_translate.html}).
//'
//' Called from and documented in R/gdal_util.R
//'
//' @noRd
// [[Rcpp::export(name = ".translate")]]
bool translate(const GDALRaster* const &src_ds,
               const Rcpp::CharacterVector &dst_filename,
               const Rcpp::Nullable<Rcpp::CharacterVector> &cl_arg = R_NilValue,
               bool quiet = false) {

    bool ret = false;

    std::string dst_filename_in;
    dst_filename_in = Rcpp::as<std::string>(check_gdal_filename(dst_filename));

    if (!src_ds)
        Rcpp::stop("open source raster failed");

    GDALDatasetH hSrcDS = src_ds->getGDALDatasetH_();
    if (hSrcDS == nullptr)
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

    GDALTranslateOptions *psOptions = nullptr;
    psOptions = GDALTranslateOptionsNew(argv.data(), nullptr);

    if (psOptions == nullptr)
        Rcpp::stop("translate failed (could not create options struct)");

    if (!quiet)
        GDALTranslateOptionsSetProgress(psOptions, GDALTermProgressR, nullptr);

    GDALDatasetH hDstDS = GDALTranslate(dst_filename_in.c_str(), hSrcDS,
                                        psOptions, nullptr);

    GDALTranslateOptionsFree(psOptions);

    if (hDstDS != nullptr) {
        GDALClose(hDstDS);
        ret = true;
    }

    return ret;
}


//' Raster reprojection and mosaicing
//'
//' `warp()` is a wrapper of the \command{gdalwarp} command-line utility for
//' raster mosaicing, reprojection and warping
//' (see \url{https://gdal.org/en/stable/programs/gdalwarp.html}).
//'
//' Called from and documented in R/gdal_util.R
//'
//' Destination raster is specified here as either:
//' dst_filename - the destination dataset path or ""
//' dst_dataset - list of length 1 containg a GDALRaster object or empty list
//'               (workaround for a nullable dataset argument)
//'
//' @noRd
// [[Rcpp::export(name = ".warp")]]
bool warp(const Rcpp::List &src_datasets,
          const Rcpp::CharacterVector &dst_filename,
          Rcpp::List dst_dataset, const std::string &t_srs,
          const Rcpp::Nullable<Rcpp::CharacterVector> &cl_arg, bool quiet) {

    bool ret = false;
    std::string dst_filename_in = Rcpp::as<std::string>(
        check_gdal_filename(dst_filename));

    GDALRaster *dst_dataset_in = nullptr;
    if (dst_filename_in == "" && dst_dataset.size() == 1)
        dst_dataset_in = dst_dataset[0];
    else if (dst_filename_in == "" && dst_dataset.size() != 1)
        Rcpp::stop("invalid specification of destination raster");

    if (dst_filename_in == "" && dst_dataset_in->getGDALDatasetH_() == nullptr)
        Rcpp::stop("destination raster is 'nullptr'");

    std::vector<GDALDatasetH> src_hDS(src_datasets.size());

    for (R_xlen_t i = 0; i < src_datasets.size(); ++i) {
        GDALRaster *ds = src_datasets[i];
        if (!ds)
            Rcpp::stop("open source raster failed");
    }

    for (R_xlen_t i = 0; i < src_datasets.size(); ++i) {
        GDALRaster *ds = src_datasets[i];
        GDALDatasetH hDS = ds->getGDALDatasetH_();
        if (hDS == nullptr) {
            Rcpp::Rcerr << "error on source " << (i + 1) << "\n";
            for (R_xlen_t j = 0; j < i; ++j)
                GDALClose(src_hDS[j]);
            Rcpp::stop("open source raster failed");
        } else {
            src_hDS[i] = hDS;
        }
    }

    std::string t_srs_in = "";
    if (t_srs != "")
        t_srs_in = t_srs;
    else
        t_srs_in = GDALGetProjectionRef(src_hDS[0]);

    std::vector<char *> argv;
    argv.push_back((char *) "-t_srs");
    argv.push_back((char *) t_srs_in.c_str());
    if (cl_arg.isNotNull()) {
        Rcpp::CharacterVector cl_arg_in(cl_arg);
        for (R_xlen_t i = 0; i < cl_arg_in.size(); ++i) {
            argv.push_back((char *) cl_arg_in[i]);
        }
    }
    argv.push_back(nullptr);

    GDALWarpAppOptions* psOptions = nullptr;
    psOptions = GDALWarpAppOptionsNew(argv.data(), nullptr);
    if (psOptions == nullptr)
        Rcpp::stop("warp raster failed (could not create options struct)");

    if (!quiet)
        GDALWarpAppOptionsSetProgress(psOptions, GDALTermProgressR, nullptr);

    GDALDatasetH hDstDS = nullptr;
    if (dst_dataset_in) {
        hDstDS = GDALWarp(nullptr, dst_dataset_in->getGDALDatasetH_(),
                          src_datasets.size(), src_hDS.data(),
                          psOptions, nullptr);
    }
    else {
        hDstDS = GDALWarp(dst_filename_in.c_str(), nullptr,
                          src_datasets.size(), src_hDS.data(),
                          psOptions, nullptr);
    }

    GDALWarpAppOptionsFree(psOptions);

    if (hDstDS != nullptr) {
        if (dst_filename_in != "") {
            GDALClose(hDstDS);
        }
        ret = true;
    }

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
//' (see \href{https://gdal.org/en/stable/user/raster_data_model.html#color-table}{GDAL
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
//' ds_tcc$setColorTable(band=1, col_tbl=colors, palette_interp = "RGB")
//' ds_tcc$setRasterColorInterp(band = 1, col_interp = "Palette")
//'
//' # close and re-open the dataset in read_only mode
//' ds_tcc$open(read_only=TRUE)
//'
//' plot_raster(ds_tcc, interpolate = FALSE, legend = TRUE,
//'             main = "Storm Lake Tree Canopy Cover (%)")
//' ds_tcc$close()
//' \dontshow{deleteDataset(tcc_file)}
// [[Rcpp::export]]
Rcpp::IntegerMatrix createColorRamp(int start_index,
                                    const Rcpp::IntegerVector &start_color,
                                    int end_index,
                                    const Rcpp::IntegerVector &end_color,
                                    const std::string &palette_interp = "RGB") {

    Rcpp::IntegerVector start_color_in = Rcpp::clone(start_color);
    Rcpp::IntegerVector end_color_in = Rcpp::clone(end_color);

    if (end_index <= start_index)
        Rcpp::stop("'end_index' must be greater than 'start_index'");
    if (start_color.size() < 3 || start_color.size() > 4)
        Rcpp::stop("length of 'start_color' must be 3 or 4");
    if (end_color.size() < 3 || end_color.size() > 4)
        Rcpp::stop("length of 'end_color' must be 3 or 4");

    if (start_color.size() == 3)
        start_color_in.push_back(255);
    if (end_color.size() == 3)
        end_color_in.push_back(255);

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
            static_cast<short>(start_color_in(0)),
            static_cast<short>(start_color_in(1)),
            static_cast<short>(start_color_in(2)),
            static_cast<short>(start_color_in(3)) };
    const GDALColorEntry colEnd = {
            static_cast<short>(end_color_in(0)),
            static_cast<short>(end_color_in(1)),
            static_cast<short>(end_color_in(2)),
            static_cast<short>(end_color_in(3)) };

    GDALCreateColorRamp(hColTbl, start_index, &colStart, end_index, &colEnd);

    // int nEntries = GDALGetColorEntryCount(hColTbl);
    int nEntries = (end_index - start_index) + 1;
    Rcpp::IntegerMatrix col_tbl(nEntries, 5);
    Rcpp::CharacterVector col_tbl_names;

    if (gpi == GPI_Gray) {
        col_tbl_names = {"value", "gray", "c2", "c3", "c4"};
        Rcpp::colnames(col_tbl) = col_tbl_names;
    } else if (gpi == GPI_RGB) {
        col_tbl_names = {"value", "red", "green", "blue", "alpha"};
        Rcpp::colnames(col_tbl) = col_tbl_names;
    } else if (gpi == GPI_CMYK) {
        col_tbl_names = {"value", "cyan", "magenta", "yellow", "black"};
        Rcpp::colnames(col_tbl) = col_tbl_names;
    } else if (gpi == GPI_HLS) {
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
//' dst_file <- file.path(tempdir(), "sr_multi.tif")
//' rasterFromRaster(b5_file, dst_file, nbands = 7, init = 0)
//' opt <- c("COMPRESSED=YES", "SKIP_HOLES=YES")
//' bandCopyWholeRaster(b5_file, 1, dst_file, 5, options = opt)
//' ds <- new(GDALRaster, dst_file)
//' ds$getStatistics(band = 5, approx_ok = FALSE, force = TRUE)
//' ds$close()
//' \dontshow{deleteDataset(dst_file)}
// [[Rcpp::export(invisible = true)]]
bool bandCopyWholeRaster(const Rcpp::CharacterVector &src_filename,
                         int src_band,
                         const Rcpp::CharacterVector &dst_filename,
                         int dst_band,
                         const Rcpp::Nullable<Rcpp::CharacterVector>
                                &options = R_NilValue,
                         bool quiet = false) {

    GDALDatasetH hSrcDS = nullptr;
    GDALRasterBandH hSrcBand = nullptr;
    GDALDatasetH hDstDS = nullptr;
    GDALRasterBandH hDstBand = nullptr;
    CPLErr err = CE_None;

    std::string src_filename_in;
    src_filename_in = Rcpp::as<std::string>(check_gdal_filename(src_filename));
    std::string dst_filename_in;
    dst_filename_in = Rcpp::as<std::string>(check_gdal_filename(dst_filename));

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
//' b5_tmp <- file.path(tempdir(), "b5_tmp.tif")
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
bool deleteDataset(const Rcpp::CharacterVector &filename,
                   const std::string &format = "") {

    std::string filename_in;
    filename_in = Rcpp::as<std::string>(check_gdal_filename(filename));

    GDALDriverH hDriver = nullptr;
    if (format == "") {
        hDriver = GDALIdentifyDriver(filename_in.c_str(), nullptr);
        if (hDriver == nullptr)
            return false;
    } else {
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
//' b5_tmp <- file.path(tempdir(), "b5_tmp.tif")
//' file.copy(b5_file,  b5_tmp)
//'
//' ds <- new(GDALRaster, b5_tmp)
//' ds$buildOverviews("BILINEAR", levels = c(2, 4, 8), bands = c(1))
//' ds$getFileList()
//' ds$close()
//' b5_tmp2 <- file.path(tempdir(), "b5_tmp_renamed.tif")
//' renameDataset(b5_tmp2, b5_tmp)
//' ds <- new(GDALRaster, b5_tmp2)
//' ds$getFileList()
//' ds$close()
//'
//' deleteDataset(b5_tmp2)
// [[Rcpp::export]]
bool renameDataset(const Rcpp::CharacterVector &new_filename,
                   const Rcpp::CharacterVector &old_filename,
                   const std::string &format = "") {

    std::string new_filename_in;
    new_filename_in = Rcpp::as<std::string>(check_gdal_filename(new_filename));
    std::string old_filename_in;
    old_filename_in = Rcpp::as<std::string>(check_gdal_filename(old_filename));

    GDALDriverH hDriver = nullptr;
    if (format == "") {
        hDriver = GDALIdentifyDriver(old_filename_in.c_str(), nullptr);
        if (hDriver == nullptr)
            return false;
    } else {
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
//' lcp_tmp <- file.path(tempdir(), "storm_lake_copy.lcp")
//' copyDatasetFiles(lcp_tmp, lcp_file)
//' ds_copy <- new(GDALRaster, lcp_tmp)
//' ds_copy$getFileList()
//' ds_copy$close()
//'
//' deleteDataset(lcp_tmp)
// [[Rcpp::export]]
bool copyDatasetFiles(const Rcpp::CharacterVector &new_filename,
                      const Rcpp::CharacterVector &old_filename,
                      const std::string &format = "") {

    std::string new_filename_in;
    new_filename_in = Rcpp::as<std::string>(check_gdal_filename(new_filename));
    std::string old_filename_in;
    old_filename_in = Rcpp::as<std::string>(check_gdal_filename(old_filename));

    GDALDriverH hDriver = nullptr;
    if (format == "") {
        hDriver = GDALIdentifyDriver(old_filename_in.c_str(), nullptr);
        if (hDriver == nullptr)
            return false;
    } else {
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


//' Identify the GDAL driver that can open a dataset
//'
//' `identifyDriver()` will try to identify the driver that can open the passed
//' file name by invoking the Identify method of each registered GDALDriver in
//' turn. The short name of the first driver that successfully identifies the
//' file name will be returned as a character string. If all drivers fail then
//' `NULL` is returned.
//' Wrapper of `GDALIdentifyDriverEx()` in the GDAL C API.
//'
//' @note
//' In order to reduce the need for such searches to touch the file system
//' machinery of the operating system, it is possible to give an optional list
//' of files. This is the list of all files at the same level in the file
//' system as the target file, including the target file. The filenames should
//' not include any path components. If the target object does not have
//' filesystem semantics then the file list should be `NULL`.
//'
//' At least one of the `raster` or `vector` arguments must be `TRUE`.
//'
//' @param filename Character string containing the name of the file to access.
//' This may not refer to a physical file, but instead contain information for
//' the driver on how to access a dataset (e.g., connection string, URL, etc.)
//' @param raster Logical value indicating whether to include raster format
//' drivers in the search, `TRUE` by default. May be set to `FALSE` to include
//' only vector drivers.
//' @param vector Logical value indicating whether to include vector format
//' drivers in the search, `TRUE` by default. May be set to `FALSE` to include
//' only raster drivers.
//' @param allowed_drivers Optional character vector of driver short names
//' that must be considered. Set to `NULL` to consider all candidate drivers
//' (the default).
//' @param file_list Optional character vector of filenames, including those
//' that are auxiliary to the main filename (see Note). May contain the input
//' `filename` but this is not required. Defaults to `NULL`.
//' @returns A character string with the short name of the first driver that
//' successfully identifies the input file name, or `NULL` on failure.
//'
//' @seealso
//' [gdal_formats()]
//'
//' @examples
//' src <- system.file("extdata/ynp_fires_1984_2022.gpkg", package="gdalraster")
//'
//' identifyDriver(src) |> gdal_formats()
// [[Rcpp::export(name = "identifyDriver")]]
SEXP identifyDriver(const Rcpp::CharacterVector &filename,
                    bool raster = true, bool vector = true,
                    const Rcpp::Nullable<Rcpp::CharacterVector>
                            &allowed_drivers = R_NilValue,
                    const Rcpp::Nullable<Rcpp::CharacterVector>
                            &file_list = R_NilValue) {

    std::string filename_in;
    filename_in = Rcpp::as<std::string>(check_gdal_filename(filename));

    unsigned int nIdentifyFlags = GDAL_OF_RASTER | GDAL_OF_VECTOR;
    if (!raster && !vector)
        return R_NilValue;
    else if (!raster)
        nIdentifyFlags = GDAL_OF_VECTOR;
    else if (!vector)
        nIdentifyFlags = GDAL_OF_RASTER;

    std::vector<const char *> papszAllowedDrivers;
    if (allowed_drivers.isNotNull()) {
        Rcpp::CharacterVector allowed_drivers_in(allowed_drivers);
        for (R_xlen_t i = 0; i < allowed_drivers_in.size(); ++i) {
            papszAllowedDrivers.push_back((const char *) allowed_drivers_in[i]);
        }
    }
    papszAllowedDrivers.push_back(nullptr);

    std::vector<const char *> papszFileList;
    if (file_list.isNotNull()) {
        Rcpp::CharacterVector file_list_in(file_list);
        for (R_xlen_t i = 0; i < file_list_in.size(); ++i) {
            papszFileList.push_back((const char *) file_list_in[i]);
        }
    }
    papszFileList.push_back(nullptr);

    GDALDriverH hDriver = GDALIdentifyDriverEx(
            filename_in.c_str(),
            nIdentifyFlags,
            allowed_drivers.isNull() ? nullptr : papszAllowedDrivers.data(),
            file_list.isNull() ? nullptr : papszFileList.data());

    if (hDriver == nullptr)
        return R_NilValue;
    else
        return Rcpp::wrap(GDALGetDriverShortName(hDriver));
}


//' Return the list of creation options of a GDAL driver as XML string
//'
//' Called from and documented in R/gdal_helpers.R
//' @noRd
// [[Rcpp::export(name = ".getCreationOptions")]]
std::string getCreationOptions(const std::string &format) {

    GDALDriverH hDriver = nullptr;
    hDriver = GDALGetDriverByName(format.c_str());
    if (hDriver == nullptr)
        Rcpp::stop("failed to get driver from format name");

    return GDALGetDriverCreationOptionList(hDriver);
}


//' Validate the list of creation options that are handled by a driver
//'
//' `validateCreationOptions()` is a helper function primarily used by GDAL's
//' Create() and CreateCopy() to validate that the passed-in list of creation
//' options is compatible with the GDAL_DMD_CREATIONOPTIONLIST metadata item
//' defined by some drivers. If the GDAL_DMD_CREATIONOPTIONLIST metadata item
//' is not defined, this function will return `TRUE`. Otherwise it will check
//' that the keys and values in the list of creation options are compatible
//' with the capabilities declared by the GDAL_DMD_CREATIONOPTIONLIST metadata
//' item. In case of incompatibility a message will be emitted and `FALSE` will
//' be returned. Wrapper of `GDALValidateCreationOptions()` in the GDAL API.
//'
//' @param format Character string giving a format driver short name
//' (e.g., `"GTiff"`).
//' @param options A character vector of format-specific creation options as
//' `"NAME=VALUE"` pairs.
//' @returns A logical value, `TRUE` if the given creation options are
//' compatible with the capabilities declared by the GDAL_DMD_CREATIONOPTIONLIST
//' metadata item for the specified format driver (or if the
//' GDAL_DMD_CREATIONOPTIONLIST metadata item is not defined for this driver),
//' otherwise `FALSE`.
//'
//' @seealso
//' [getCreationOptions()], [create()], [createCopy()]
//'
//' @examples
//' validateCreationOptions("GTiff", c("COMPRESS=LZW", "TILED=YES"))
// [[Rcpp::export]]
bool validateCreationOptions(const std::string &format,
                             const Rcpp::CharacterVector &options) {

    GDALDriverH hDriver = nullptr;
    hDriver = GDALGetDriverByName(format.c_str());
    if (hDriver == nullptr)
        Rcpp::stop("failed to get driver for the specified format");

    std::vector<const char *> opt_list(options.size() + 1);
    for (R_xlen_t i = 0; i < options.size(); ++i) {
        opt_list[i] = (const char *) options[i];
    }
    opt_list[options.size()] = nullptr;

    if (GDALValidateCreationOptions(hDriver, opt_list.data()))
        return true;
    else
        return false;
}


//' Add a file inside a new or existing ZIP file
//' Mainly for create/append to Seek-Optimized ZIP
//'
//' @noRd
// [[Rcpp::export(name = ".addFileInZip")]]
bool addFileInZip(const std::string &zip_filename, bool overwrite,
                  const std::string &archive_filename,
                  const std::string &in_filename,
                  const Rcpp::Nullable<Rcpp::CharacterVector> &options,
                  bool quiet) {

#if GDAL_VERSION_NUM < 3070000
    Rcpp::stop("addFileInZip() requires GDAL >= 3.7");

#else
    bool ret = false;
    VSIStatBufL buf;

    std::vector<char *> opt_zip_create;
    if (overwrite) {
        VSIUnlink(zip_filename.c_str());
    } else {
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
