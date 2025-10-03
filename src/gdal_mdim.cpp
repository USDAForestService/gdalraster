/* GDAL Multidimensional Raster support
   Chris Toney <jctoney at gmail.com>
   Copyright (c) 2023-2025 gdalraster authors
*/

#include <Rcpp.h>

#include <gdal.h>
#include <cpl_conv.h>
#include <gdal_utils.h>

#include <memory>
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
    const Rcpp::CharacterVector &dsn, const std::string &array_name,
    int idx_xdim, int idx_ydim, bool read_only, const std::string &group_name,
    const std::string &view_expr,
    const Rcpp::Nullable<Rcpp::CharacterVector> &allowed_drivers,
    const Rcpp::Nullable<Rcpp::CharacterVector> &open_options) {

// requires GDAL >= 3.2 for GDALGroupOpenGroupFromFullname()
#if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 2, 0)
    Rcpp::stop("mdim_as_classic() requires GDAL >= 3.2");
#else

    const std::string dsn_in = Rcpp::as<std::string>(check_gdal_filename(dsn));

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

    unsigned int nOpenFlags = GDAL_OF_MULTIDIM_RASTER | GDAL_OF_VERBOSE_ERROR;
    if (read_only)
        nOpenFlags |= GDAL_OF_READONLY;
    else
        nOpenFlags |= GDAL_OF_UPDATE;

    GDALDatasetH hDS = nullptr;
    hDS = GDALOpenEx(dsn_in.c_str(), nOpenFlags,
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
    if (group_name == "") {
        hVar = GDALGroupOpenMDArray(hRootGroup, array_name.c_str(), nullptr);
    }
    else {
        GDALGroupH hSubGroup = nullptr;
        hSubGroup = GDALGroupOpenGroupFromFullname(hRootGroup,
                                                   group_name.c_str(),
                                                   nullptr);
        if (!hSubGroup)
            Rcpp::stop("failed to get object for the sub-group");

        hVar = GDALGroupOpenMDArray(hSubGroup, array_name.c_str(), nullptr);
        GDALGroupRelease(hSubGroup);
    }

    if (!hVar) {
        GDALGroupRelease(hRootGroup);
        Rcpp::stop("failed to get object for the MDArray");
    }

    GDALDatasetH hClassicDS = nullptr;
    GDALMDArrayH hVarView = nullptr;
    if (view_expr != "") {
        hVarView = GDALMDArrayGetView(hVar, view_expr.c_str());
        GDALMDArrayRelease(hVar);
        hVar = nullptr;
        if (!hVarView) {
            GDALGroupRelease(hRootGroup);
            Rcpp::stop("failed to get object for the MDArray view expression");
        }
    }

    #if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 8, 0)
    hClassicDS = GDALMDArrayAsClassicDataset(
        hVarView ? hVarView : hVar,
        static_cast<size_t>(idx_xdim),
        static_cast<size_t>(idx_ydim));
    #else
    hClassicDS = GDALMDArrayAsClassicDatasetEx(
        hVarView ? hVarView : hVar,
        static_cast<size_t>(idx_xdim),
        static_cast<size_t>(idx_ydim),
        hRootGroup,
        oOpenOptions.empty() ? nullptr : oOpenOptions.data());
    #endif

    if (hVarView)
        GDALMDArrayRelease(hVarView);

    if (hVar)
        GDALMDArrayRelease(hVar);

    GDALGroupRelease(hRootGroup);
    if (!hClassicDS)
        Rcpp::stop("failed to get MDArray as classic dataset");

    auto ds = std::make_unique<GDALRaster>();
    ds->setGDALDatasetH_(hClassicDS);
    return ds.release();
#endif
}


//' Report structure and content of a multidimensional dataset
//'
//' `mdim_info()` is an interface to the \command{gdalmdiminfo} command-line
//' utility (see \url{https://gdal.org/en/stable/programs/gdalmdiminfo.html}).
//' This function lists various information about a GDAL supported
//' multidimensional raster dataset as JSON output. It follows the JSON schema
//' [gdalmdiminfo_output.schema.json](https://github.com/OSGeo/gdal/blob/release/3.11/apps/data/gdalmdiminfo_output.schema.json).
//' Requires GDAL >= 3.2.
//'
//' @param dsn Character string giving the data source name of the
//' multidimensional raster (e.g., file, VSI path).
//' @param array_name Character string giving the name of the MDarray in
//' `dsn`.
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
//' must be considered when opening `dsn`. It is generally not necessary to
//' specify it, but it can be used to skip automatic driver detection, when it
//' fails to select the appropriate driver.
//' @param open_options Optional character vector of format-specific dataset
//' openoptions as `"NAME=VALUE"` pairs.
//' @param cout Logical value, `TRUE` to print info to the console (the
//' default), or `FALSE` to suppress console output.
//' @returns Invisibly, a JSON string containing information about the
//' multidimensional raster dataset. By default, the info string is also printed
//' to the console unless `cout` is set to `FALSE`.
//'
//' @seealso
//' [mdim_as_classic()], [mdim_translate()]
//'
//' @examplesIf gdal_version_num() >= gdal_compute_version(3, 2, 0) && isTRUE(gdal_formats("netCDF")$multidim_raster)
//' f <- system.file("extdata/byte.nc", package="gdalraster")
//' mdim_info(f)
// [[Rcpp::export(invisible = true)]]
std::string mdim_info(
    const Rcpp::CharacterVector &dsn,
    const std::string &array_name = "",
    bool pretty = true,
    bool detailed = false,
    int limit = -1,
    bool stats = false,
    const Rcpp::Nullable<Rcpp::CharacterVector> &array_options = R_NilValue,
    const Rcpp::Nullable<Rcpp::CharacterVector> &allowed_drivers = R_NilValue,
    const Rcpp::Nullable<Rcpp::CharacterVector> &open_options = R_NilValue,
    bool cout = true) {

#if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 2, 0)
    Rcpp::stop("mdim_info() requires GDAL >= 3.2");
#else
    const std::string dsn_in = Rcpp::as<std::string>(check_gdal_filename(dsn));

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

    unsigned int nOpenFlags = GDAL_OF_MULTIDIM_RASTER | GDAL_OF_VERBOSE_ERROR;

    GDALDatasetH hDS = nullptr;
    hDS = GDALOpenEx(dsn_in.c_str(), nOpenFlags,
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

    std::string limit_str;
    if (limit > 0 && !detailed) {
        Rcpp::Rcout << "'limit' only taken into account if 'detailed = TRUE'\n";
    }
    else if (limit > 0) {
        limit_str = std::to_string(limit);
        argv.push_back(const_cast<char *>("-limit"));
        argv.push_back(const_cast<char *>(limit_str.c_str()));
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
    psOptions = GDALMultiDimInfoOptionsNew(argv.empty() ? nullptr : argv.data(),
                                           nullptr);

    if (!psOptions) {
        GDALReleaseDataset(hDS);
        Rcpp::stop("mdim_info() failed (could not create options struct)");
    }

    std::string info_out = "";
    char *pszInfo = nullptr;
    pszInfo = GDALMultiDimInfo(hDS, psOptions);
    if (pszInfo)
        info_out = pszInfo;
    CPLFree(pszInfo);

    GDALMultiDimInfoOptionsFree(psOptions);

    GDALReleaseDataset(hDS);

    if (cout)
        Rcpp::Rcout << info_out.c_str() << "\n";

    return info_out;
#endif
}


//' Convert multidimensional data between different formats, and subset
//'
//' `mdim_translate()` is an interface to the \command{gdalmdimtranslate}
//' command-line utility (see
//' \url{https://gdal.org/en/stable/programs/gdalmdimtranslate.html}).
//' This function converts multidimensional data between different formats and
//' performs subsetting. Requires GDAL >= 3.2.
//'
//' @details
//' \subsection{Array creation options}{
//' Array creation options must be prefixed with `ARRAY:`. The scope may be
//' further restricted to arrays of a certain dimension by adding
//' `IF(DIM={ndims}):` after `ARRAY:`. For example,
//' `"ARRAY:IF(DIM=2):BLOCKSIZE=256,256"` will restrict `BLOCKSIZE=256,256` to
//' arrays of dimension 2. Restriction to arrays of a given name is done with
//' adding `IF(NAME={name}):` after `ARRAY:`. `{name}` can also be a fully
//' qualified name. A non-driver specific array option, `"AUTOSCALE=YES"` can
//' be used to ask (non indexing) variables of type `Float32` or `Float64` to be
//' scaled to `UInt16` with scale and offset values being computed from the
//' minimum and maximum of the source array. The integer data type used can be
//' set with `"AUTOSCALE_DATA_TYPE=Byte|UInt16|Int16|UInt32|Int32"`.
//' }
//'
//' \subsection{`array_specs`}{
//' Instead of converting the whole dataset, select one or more arrays, and
//' possibly perform operations on them. One or more array specifications can
//' be given as elements of a character vector.
//'
//' An array specification may be just an array name, potentially using a fully
//' qualified syntax (`"/group/subgroup/array_name"`). Or it can be a
//' combination of options with the syntax:
//' ```
//' "name={src_array_name}[,dstname={dst_array_name}][,resample=yes][,transpose=[{axis1},{axis2},...][,view={view_expr}]"
//' ```
//' The following options are processed in that order:
//'
//' * `resample=yes` asks for the array to run through
//'   `GDALMDArray::GetResampled()`.
//' * \code{[{axis1},{axis2},...]} is the argument of
//'   `GDALMDArray::Transpose()`. For example, `transpose=[1,0]` switches the
//'   axis order of a 2D array.
//' * `{view_expr}` is the value of the `viewExpr` argument of
//'   `GDALMDArray::GetView()`. When specifying a `view_expr` that performs a
//'   slicing or subsetting on a dimension, the equivalent operation will be
//'   applied to the corresponding indexing variable. See `?mdim_as_classic` for
//'   details on view expressions.
//' }
//'
//' \subsection{`group_specs`}{
//' Instead of converting the whole dataset, select one or more groups, and
//' possibly perform operations on them. One or more group specifications can
//' be given in a character vector, to operate on different groups. If only one
//' group is specified, its content will be copied directly to the target root
//' group. If several are specified, they are copied under the target root
//' group.
//'
//' A group specification may be just a group name, potentially using a fully
//' qualified syntax (`"/group/subgroup/subsubgroup_name"`). Or it can be a
//' combination of options with the syntax:
//' ```
//' "name={src_group_name}[,dstname={dst_group_name}][,recursive=no]"
//' ```
//' }
//'
//' \subsection{`subset_specs`}{
//' Perform subsetting (trimming or slicing) operations along dimensions,
//' provided that the dimension is indexed by a 1D variable of numeric or string
//' data type, and whose values are monotonically sorted. One or more subset
//' specifications can be given in a character vector. A subset specification
//' string follows exactly the OGC WCS 2.0 KVP encoding for subsetting.
//'
//' Syntax is `"dim_name(min_val,max_val)"` or `"dim_name(sliced_val)"`. The
//' first syntax will subset the dimension dim_name to values in the
//' \code{[min_val,max_val]} range. The second syntax will slice the dimension
//' `dim_name` to value `sliced_val` (and this dimension will be removed from
//' the arrays that reference to it)
//'
//' Using a subset specification is incompatible with specifying a view option
//' in `array_specs`.
//' }
//'
//' \subsection{`scaleaxes_specs`}{
//' Applies an integral scale factor to one or several dimensions, i.e., extract
//' 1 value every N values (without resampling). A scale-axes specification
//' string follows exactly the syntax of the KVP encoding of the SCALEAXES
//' parameter of OGC WCS 2.0 Scaling Extension, but limited to integer scale
//' factors.
//'
//' Syntax is a character string of the form:
//' ```
//' "<dim1_name>(<scale_factor>)[,<dim2_name>(<scale_factor>)]..."
//' ```
//'
//' Using a scale-axes specification is incompatible with specifying a view
//' option in `array_specs`.
//' }
//'
//' @param src_dsn Character string giving the name of the source
//' multidimensional raster dataset (e.g., file, VSI path).
//' @param dst_dsn Character string giving the name of the destination
//' multidimensional raster dataset (e.g., file, VSI path).
//' @param output_format Character string giving the output format (driver short
//' name). This can be a format that supports multidimensional output (such as
//' NetCDF: Network Common Data Form, Multidimensional VRT), or a "classic" 2D
//' format, if only one single 2D array results from the other specified
//' conversion operations. When this option is not specified (i.e., empty string
//' `""`), the format is guessed when possible from the extension of `dst_dsn`.
//' @param creation_options Optional character vector of format-specific
//' creation options as `"NAME=VALUE"` pairs. A list of options supported for a
//' format can be obtained with `getCreationOptions()`, but the documentation
//' for the format is the definitive source of information on driver creation
//' options (see \url{https://gdal.org/en/stable/drivers/raster/index.html}).
//' Array-level creation options may be passed by prefixing them with `ARRAY:`
//' (see Details).
//' @param array_specs Optional character vector of one or more array
//' specifications, instead of converting the whole dataset (see Details).
//' @param group_specs Optional character vector of one or more array
//' specifications, instead of converting the whole dataset (see Details).
//' @param subset_specs Optional character vector of one or more subset
//' specifications, that perform trimming or slicing along a dimension, provided
//' that it is indexed by a 1D variable of numeric or string data type, and
//' whose values are monotonically sorted (see Details).
//' @param scaleaxes_specs Optional character string for a scale-axes
//' specification, that apply an integral scale factor to one or several
//' dimensions, i.e., extract 1 value every N values (without resampling) (see
//' Details).
//' @param allowed_drivers Optional character vector of driver short names that
//' must be considered when opening `src_dsn`. It is generally not necessary to
//' specify it, but it can be used to skip automatic driver detection, when it
//' fails to select the appropriate driver.
//' @param open_options Optional character vector of format-specific dataset
//' open options for `src_dsn` as `"NAME=VALUE"` pairs.
//' @param strict Logical value, `FALSE` (the default) some failures during the
//' translation are tolerated, such as not being able to write group attributes.
//' If set to `TRUE`, such failures will cause the process to fail.
//' @param quiet Logical value, set to `TRUE` to disable progress reporting.
//' Defaults to `FALSE`.
//' @returns Logical value indicating success (invisible `TRUE`, output written
//' to `dst_dsn`). An error is raised if the operation fails.
//'
//' @seealso
//' [mdim_as_classic()], [mdim_info()]
//'
//' @examplesIf gdal_version_num() >= gdal_compute_version(3, 2, 0) && isTRUE(gdal_formats("netCDF")$multidim_raster)
//' f_src <- system.file("extdata/byte.nc", package="gdalraster")
//'
//' ## COMPRESS option for MDArray creation
//' opt <- NULL
//' if (isTRUE(gdal_get_driver_md("netCDF")$NETCDF_HAS_NC4 == "YES")) {
//'   # 4 x 4 is for illustration only, the sample dataset is only 20 x 20
//'   opt <- c("ARRAY:IF(DIM=2):BLOCKSIZE=4,4",
//'            "ARRAY:IF(NAME=Band1):COMPRESS=DEFLATE",
//'            "ARRAY:ZLEVEL=6")
//' }
//'
//' f_dst <- tempfile(fileext = ".nc")
//' mdim_translate(f_src, f_dst, creation_options = opt)
//' info <- mdim_info(f_dst, cout = FALSE) |> yyjsonr::read_json_str()
//' # str(info)
//' info$structural_info
//' info$arrays$Band1$block_size
//' info$arrays$Band1$structural_info
//'
//' (ds <- mdim_as_classic(f_dst, "Band1", 1, 0))
//'
//' plot_raster(ds, interpolate = FALSE, legend = TRUE, main = "Band1")
//'
//' ds$close()
//'
//' ## slice along the Y axis with array view
//' f_dst2 <- tempfile(fileext = ".nc")
//' mdim_translate(f_src, f_dst2, array_specs = "name=Band1,view=[10:20,...]")
//' (ds <- mdim_as_classic(f_dst2, "Band1", 1, 0))
//'
//' plot_raster(ds, interpolate = FALSE, legend = TRUE,
//'             main = "Band1[10:20,...]")
//'
//' ds$close()
//'
//' ## trim X and Y by subsetting
//' f_dst3 <- tempfile(fileext = ".nc")
//' subsets <- c("x(441000,441800)", "y(3750400,3751000)")
//' mdim_translate(f_src, f_dst3, subset_specs = subsets)
//' (ds <- mdim_as_classic(f_dst3, "Band1", 1, 0))
//'
//' plot_raster(ds, interpolate = FALSE, legend = TRUE,
//'             main = "Band1 trimmed")
//'
//' ds$close()
//'
//' ## subsample along X and Y
//' f_dst4 <- tempfile(fileext = ".nc")
//' mdim_translate(f_src, f_dst4, scaleaxes_specs = "x(2),y(2)")
//' (ds <- mdim_as_classic(f_dst4, "Band1", 1, 0))
//'
//' plot_raster(ds, interpolate = FALSE, legend = TRUE,
//'             main = "Band1 subsampled")
//'
//' ds$close()
//' \dontshow{deleteDataset(f_dst)}
//' \dontshow{deleteDataset(f_dst2)}
//' \dontshow{deleteDataset(f_dst3)}
//' \dontshow{deleteDataset(f_dst4)}
// [[Rcpp::export(invisible = true)]]
bool mdim_translate(
    const Rcpp::CharacterVector &src_dsn, const Rcpp::CharacterVector &dst_dsn,
    const std::string &output_format = "",
    const Rcpp::Nullable<Rcpp::CharacterVector> &creation_options = R_NilValue,
    const Rcpp::Nullable<Rcpp::CharacterVector> &array_specs = R_NilValue,
    const Rcpp::Nullable<Rcpp::CharacterVector> &group_specs = R_NilValue,
    const Rcpp::Nullable<Rcpp::CharacterVector> &subset_specs = R_NilValue,
    const Rcpp::Nullable<Rcpp::String> &scaleaxes_specs = R_NilValue,
    const Rcpp::Nullable<Rcpp::CharacterVector> &allowed_drivers = R_NilValue,
    const Rcpp::Nullable<Rcpp::CharacterVector> &open_options = R_NilValue,
    bool strict = false, bool quiet = false) {

#if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 2, 0)
    Rcpp::stop("mdim_translate() requires GDAL >= 3.2");
#else
    const std::string src_dsn_in =
        Rcpp::as<std::string>(check_gdal_filename(src_dsn));

    const std::string dst_dsn_in =
        Rcpp::as<std::string>(check_gdal_filename(dst_dsn));

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

    unsigned int nOpenFlags = GDAL_OF_RASTER | GDAL_OF_MULTIDIM_RASTER |
                              GDAL_OF_VERBOSE_ERROR;

    // only 1 source dataset is supported currently but takes a list of
    // datasets as input
    std::vector<GDALDatasetH> src_ds = {};
    GDALDatasetH hSrcDS = nullptr;
    hSrcDS = GDALOpenEx(
        src_dsn_in.c_str(), nOpenFlags,
        oAllowedDrivers.empty() ? nullptr : oAllowedDrivers.data(),
        oOpenOptions.empty() ? nullptr : oOpenOptions.data(), nullptr);

    if (!hSrcDS)
        Rcpp::stop("failed to open source multidim raster dataset");
    else
        src_ds.push_back(hSrcDS);

    std::vector<char *> argv = {};

    if (output_format != "") {
        argv.push_back(const_cast<char *>("-of"));
        argv.push_back(const_cast<char *>(output_format.c_str()));
    }

    if (creation_options.isNotNull()) {
        Rcpp::CharacterVector creation_options_in(creation_options);
        for (R_xlen_t i = 0; i < creation_options_in.size(); ++i) {
            argv.push_back(const_cast<char *>("-co"));
            argv.push_back((char *) creation_options_in[i]);
        }
    }

    if (array_specs.isNotNull()) {
        Rcpp::CharacterVector array_specs_in(array_specs);
        for (R_xlen_t i = 0; i < array_specs_in.size(); ++i) {
            argv.push_back(const_cast<char *>("-array"));
            argv.push_back((char *) array_specs_in[i]);
        }
    }

    if (group_specs.isNotNull()) {
        Rcpp::CharacterVector group_specs_in(group_specs);
        for (R_xlen_t i = 0; i < group_specs_in.size(); ++i) {
            argv.push_back(const_cast<char *>("-group"));
            argv.push_back((char *) group_specs_in[i]);
        }
    }

    if (subset_specs.isNotNull()) {
        Rcpp::CharacterVector subset_specs_in(subset_specs);
        for (R_xlen_t i = 0; i < subset_specs_in.size(); ++i) {
            argv.push_back(const_cast<char *>("-subset"));
            argv.push_back((char *) subset_specs_in[i]);
        }
    }

    Rcpp::String scaleaxes_specs_in;
    if (scaleaxes_specs.isNotNull()) {
        scaleaxes_specs_in = Rcpp::as<Rcpp::String>(scaleaxes_specs);
        argv.push_back(const_cast<char *>("-scaleaxes"));
        argv.push_back(const_cast<char *>(scaleaxes_specs_in.get_cstring()));
    }

    if (strict)
        argv.push_back(const_cast<char *>("-strict"));

    if (!argv.empty())
        argv.push_back(nullptr);

    GDALMultiDimTranslateOptions *psOptions = nullptr;
    psOptions = GDALMultiDimTranslateOptionsNew(
        argv.empty() ? nullptr : argv.data(), nullptr);

    if (!psOptions) {
        GDALReleaseDataset(hSrcDS);
        Rcpp::stop("mdim_translate() failed (could not create options struct)");
    }

    if (!quiet) {
        GDALMultiDimTranslateOptionsSetProgress(psOptions, GDALTermProgressR,
                                                nullptr);
    }

    GDALDatasetH hDstDS = GDALMultiDimTranslate(dst_dsn_in.c_str(), nullptr, 1,
                                                src_ds.data(), psOptions,
                                                nullptr);

    GDALMultiDimTranslateOptionsFree(psOptions);

    bool ret = false;
    if (hDstDS) {
        GDALClose(hDstDS);
        ret = true;
    }

    GDALReleaseDataset(src_ds[0]);

    if (!ret)
        Rcpp::stop("mdim_translate() failed");

    return ret;
#endif
}
