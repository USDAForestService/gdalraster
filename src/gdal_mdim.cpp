/* GDAL Multidimensional Raster support
   Chris Toney <jctoney at gmail.com>
   Copyright (c) 2023-2025 gdalraster authors
*/

#include <Rcpp.h>

#include <gdal.h>
#include <cpl_conv.h>
#include <gdal_utils.h>

#include <string>
#include <vector>

#include "gdalraster.h"


// Return a view of an MDArray as a "classic" GDALDataset (i.e., 2D)
//
// GDALMDArrayAsClassicDataset()
//
// Public wrapper in R/gdal_mdim.R
// Implemented as a GDALRaster object factory registered in
// RCPP_MODULE(mod_GDALRaster), see src/gdalraster.cpp.
// Unique function signature based on number of parameters.
// Called in R with `ds <- new(GDALRaster, ...)` giving all 9 parameters
GDALRaster *mdim_as_classic(
    const Rcpp::CharacterVector &filename, const std::string &array_name,
    int idx_xdim, int idx_ydim, bool read_only, const std::string &group_name,
    const Rcpp::Nullable<Rcpp::CharacterVector> &allowed_drivers,
    const Rcpp::Nullable<Rcpp::CharacterVector> &open_options, bool reserved) {

// requires GDAL >= 3.2 for GDALGroupOpenGroupFromFullname()
#if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 2, 0)
    Rcpp::stop("mdim_as_classic() requires GDAL >= 3.2");
#else

    std::string fname_in = Rcpp::as<std::string>(check_gdal_filename(filename));

    if (idx_xdim < 0)
        Rcpp::stop("'idx_xdim' must be >= 0");

    if (idx_ydim < 0)
        Rcpp::stop("'idx_ydim' must be >= 0");

    std::vector<char *> oAllowedDrivers = {};
    if (allowed_drivers.isNotNull()) {
        Rcpp::CharacterVector allowed_drivers_in(allowed_drivers);
        if (allowed_drivers_in.size() > 0) {
            for (R_xlen_t i = 0; i < allowed_drivers_in.size(); ++i) {
                oAllowedDrivers.push_back((char *) allowed_drivers_in[i]);
            }
        }
        oAllowedDrivers.push_back(nullptr);
    }

    std::vector<char *> oOpenOptions = {};
    if (open_options.isNotNull()) {
        Rcpp::CharacterVector open_options_in(open_options);
        if (open_options_in.size() > 0) {
            for (R_xlen_t i = 0; i < open_options_in.size(); ++i) {
                oOpenOptions.push_back((char *) open_options_in[i]);
            }
        }
        oOpenOptions.push_back(nullptr);
    }

    unsigned int nOpenFlags = GDAL_OF_MULTIDIM_RASTER;
    if (read_only)
        nOpenFlags |= GDAL_OF_READONLY;
    else
        nOpenFlags |= GDAL_OF_UPDATE;

    GDALDatasetH hDS = nullptr;
    hDS = GDALOpenEx(fname_in.c_str(), nOpenFlags,
                     oAllowedDrivers.empty() ? nullptr : oAllowedDrivers.data(),
                     oOpenOptions.empty() ? nullptr : oOpenOptions.data(),
                     nullptr);

    if (!hDS)
        Rcpp::stop("failed to open multidim raster dataset");

    GDALGroupH hRootGroup = nullptr;
    hRootGroup = GDALDatasetGetRootGroup(hDS);
    GDALReleaseDataset(hDS);
    if (!hRootGroup)
        Rcpp::stop("failed to get object for the root group");

    GDALMDArrayH hVar = nullptr;
    if (group_name != "") {
        GDALGroupH hSubGroup = nullptr;
        hSubGroup = GDALGroupOpenGroupFromFullname(hRootGroup,
                                                   group_name.c_str(),
                                                   nullptr);
        if (!hSubGroup)
            Rcpp::stop("failed to get object for the sub-group");

        hVar = GDALGroupOpenMDArray(hSubGroup, array_name.c_str(), nullptr);
        GDALGroupRelease(hSubGroup);
        GDALGroupRelease(hRootGroup);
        if (!hVar)
            Rcpp::stop("failed to get object for the MDArray");
    }
    else {
        hVar = GDALGroupOpenMDArray(hRootGroup, array_name.c_str(), nullptr);
        GDALGroupRelease(hRootGroup);
        if (!hVar)
            Rcpp::stop("failed to get object for the MDArray");
    }

    GDALDatasetH hClassicDS = nullptr;
    hClassicDS = GDALMDArrayAsClassicDataset(hVar,
                                             static_cast<size_t>(idx_xdim),
                                             static_cast<size_t>(idx_ydim));

    GDALMDArrayRelease(hVar);
    if (!hClassicDS)
        Rcpp::stop("failed to get MDArray as classic dataset");

    GDALRaster *ds = new GDALRaster();
    if (ds == nullptr) {
        GDALClose(hClassicDS);
        Rcpp::stop("failed to create GDALRaster object");
    }
    ds->setGDALDatasetH_(hClassicDS, !read_only);
    return ds;
#endif
}


//' Report structure and content of a multidimensional dataset
//'
//' `mdim_info()` is a wrapper of the \command{gdalmdiminfo} command-line
//' utility (see \url{https://gdal.org/en/stable/programs/gdalmdiminfo.html}).
//' This function lists various information about a GDAL supported
//' multidimensional raster dataset as JSON output. It follows the JSON schema
//' [gdalmdiminfo_output.schema.json](https://github.com/OSGeo/gdal/blob/release/3.11/apps/data/gdalmdiminfo_output.schema.json).
//' Requires GDAL >= 3.2.
//'
//' @param filename Character string giving the data source name of the
//' multidimensional raster (e.g., file, VSI path).
//' @param array_name Character string giving the name of the MDarray in
//' `filename`.
//' @param pretty Logical value, `FALSE` to output a single line without any
//' indentation. Defaults to `TRUE`.
//' @param detailed Logical value, `TRUE` for verbose output. Report attribute
//' data types and array values. Defaults to `FALSE`.
//' @param limit Integer value. Number of values in each dimension that is used
//' to limit the display of array values. By default, unlimited. Only taken into
//' account if used with `detailed = TRUE`. Set to a positive integer to enable.
//' @param stats Logical value, `TRUE` to read and display array statistics.
//' Forces computation if no statistics are stored in an array. Defaults to
//' `FALSE`.
//' @param array_options Optional character vector of `"NAME=VALUE"` pairs to
//' filter reported arrays. Such option is format specific. Consult driver
//' documentation (passed to `GDALGroup::GetMDArrayNames()`).
//' @param allowed_drivers Optional character vector of driver short names that
//' must be considered. By default, all known multidimensional raster drivers are
//' considered.
//' @param open_options Optional character vector of format-specific dataset open
//' options as `"NAME=VALUE"` pairs.
//' @returns A JSON string containing information about the multidimensional
//' raster dataset.
//'
//' @seealso
//' [mdim_as_classic()]
//'
//' @examplesIf gdal_version_num() >= gdal_compute_version(3, 2, 0)
//' f <- system.file("extdata/byte.nc", package="gdalraster")
//' mdim_info(f) |> writeLines()
// [[Rcpp::export()]]
std::string mdim_info(
    const Rcpp::CharacterVector &filename,
    const std::string &array_name = "",
    bool pretty = true,
    bool detailed = false,
    int limit = -1,
    bool stats = false,
    const Rcpp::Nullable<Rcpp::CharacterVector> &array_options = R_NilValue,
    const Rcpp::Nullable<Rcpp::CharacterVector> &allowed_drivers = R_NilValue,
    const Rcpp::Nullable<Rcpp::CharacterVector> &open_options = R_NilValue) {

#if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 2, 0)
    Rcpp::stop("mdim_info() requires GDAL >= 3.2");
#else
    std::string fname_in = Rcpp::as<std::string>(check_gdal_filename(filename));

    std::vector<char *> oAllowedDrivers = {};
    if (allowed_drivers.isNotNull()) {
        Rcpp::CharacterVector allowed_drivers_in(allowed_drivers);
        if (allowed_drivers_in.size() > 0) {
            for (R_xlen_t i = 0; i < allowed_drivers_in.size(); ++i) {
                oAllowedDrivers.push_back((char *) allowed_drivers_in[i]);
            }
        }
        oAllowedDrivers.push_back(nullptr);
    }

    std::vector<char *> oOpenOptions = {};
    if (open_options.isNotNull()) {
        Rcpp::CharacterVector open_options_in(open_options);
        if (open_options_in.size() > 0) {
            for (R_xlen_t i = 0; i < open_options_in.size(); ++i) {
                oOpenOptions.push_back((char *) open_options_in[i]);
            }
        }
        oOpenOptions.push_back(nullptr);
    }

    unsigned int nOpenFlags = GDAL_OF_MULTIDIM_RASTER;

    GDALDatasetH hDS = nullptr;
    hDS = GDALOpenEx(fname_in.c_str(), nOpenFlags,
                     oAllowedDrivers.empty() ? nullptr : oAllowedDrivers.data(),
                     oOpenOptions.empty() ? nullptr : oOpenOptions.data(),
                     nullptr);

    if (!hDS)
        Rcpp::stop("failed to open multidim raster dataset");

    std::vector<char *> argv = {};

    if (array_name != "") {
        argv.push_back(const_cast<char *>("-array"));
        argv.push_back(const_cast<char *>(array_name.c_str()));
    }

    if (!pretty)
        argv.push_back(const_cast<char *>("-nopretty"));

    if (detailed)
        argv.push_back(const_cast<char *>("-detailed"));

    if (limit > 0 && !detailed) {
        Rcpp::Rcout << "'limit' only taken into account if 'detailed = TRUE'\n";
    }
    else if (limit > 0) {
        argv.push_back(const_cast<char *>("-limit"));
        argv.push_back(const_cast<char *>(std::to_string(limit).c_str()));
    }
    
    if (stats)
        argv.push_back(const_cast<char *>("-stats"));
    
    if (array_options.isNotNull()) {
        Rcpp::CharacterVector array_options_in(array_options);
        for (R_xlen_t i = 0; i < array_options_in.size(); ++i) {
            argv.push_back(const_cast<char *>("-arrayoption"));
            argv.push_back((char *) array_options_in[i]);
        }
    }

    if (!argv.empty())
        argv.push_back(nullptr);

    GDALMultiDimInfoOptions *psOptions = nullptr;
    if (!argv.empty()) {
        psOptions = GDALMultiDimInfoOptionsNew(argv.data(), nullptr);
        if (psOptions == nullptr) {
            GDALReleaseDataset(hDS);
            Rcpp::stop("mdim_info() failed (could not create options struct)");
        }
    }

    std::string info_out = "";
    char *pszInfo = nullptr;
    pszInfo = GDALMultiDimInfo(hDS, psOptions);
    if (pszInfo)
        info_out = pszInfo;
    CPLFree(pszInfo);

    if (psOptions)
        GDALMultiDimInfoOptionsFree(psOptions);

    GDALReleaseDataset(hDS);

    return info_out;
#endif
}
