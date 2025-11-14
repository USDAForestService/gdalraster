/* Implementation of class GDALRaster
   Encapsulates a subset of GDALDataset, GDALDriver and GDALRasterBand.

   Chris Toney <chris.toney at usda.gov>
   Copyright (c) 2023-2025 gdalraster authors
*/

#include <gdal.h>
#include <gdal_priv.h>
#include <cpl_port.h>
#include <cpl_conv.h>
#include <cpl_string.h>
#include <cpl_vsi.h>
#include <gdal_alg.h>
#include <gdal_utils.h>

#include <Rcpp.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdint>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "gdalraster.h"
#include "gdal_vsi.h"
#include "rcpp_util.h"
#include "transform.h"

void gdal_error_handler_r(CPLErr err_class, int err_no, const char *msg) {
    switch (err_class) {
        case CE_None:
        break;

        case CE_Debug:
        {
            Rcpp::Rcout << "GDAL DEBUG: " << msg << "\n";
        }
        break;

        case CE_Warning:
        {
            // try to be compatible with sf, and terra default level 2, wrt to
            // whether a warning is emitted in case sharing a GDAL instance
            if (is_namespace_loaded_("sf")) {
                std::stringstream ss_msg;
                ss_msg << "GDAL WARNING " << err_no << ": " << msg;
                Rcpp::warning(ss_msg.str());
            }
            else {
                Rcpp::Rcout << "GDAL WARNING " << err_no << ": " << msg << "\n";
            }
        }
        break;

        case CE_Failure:
        {
            // try to be compatible with sf, and terra default level 2, wrt to
            // whether a warning is emitted in case sharing a GDAL instance
            if (is_namespace_loaded_("sf") || is_namespace_loaded_("terra")) {
                std::stringstream ss_msg;
                ss_msg << "GDAL FAILURE " << err_no << ": " << msg;
                Rcpp::warning(ss_msg.str());
            }
            else {
                Rcpp::Rcout << "GDAL FAILURE " << err_no << ": " << msg << "\n";
            }
        }
        break;

        case CE_Fatal:
        {
            Rcpp::Rcout << "GDAL FATAL ERROR " << err_no << ": " << msg << "\n";
            Rcpp::stop("aborted for GDAL fatal error");
        }

        default:
        {
            Rcpp::Rcout << "UNKNOWN GDAL ERROR CLASS " << err_no << ": " <<
                msg << "\n";
        }
        break;
    }
}

void gdal_silent_errors_r(CPLErr err_class, int err_no, const char *msg) {
    if (err_class == CE_Fatal) {
        Rcpp::Rcout << "GDAL FATAL ERROR " << err_no << ": " << msg << "\n";
        Rcpp::stop("aborted for GDAL fatal error");
    }
}

// [[Rcpp::init]]
void gdal_init(DllInfo *dll) {
    CPLSetErrorHandler((CPLErrorHandler) gdal_silent_errors_r);
    GDALAllRegister();
    CPLSetErrorHandler((CPLErrorHandler) gdal_error_handler_r);
    CPLSetConfigOption("OGR_CT_FORCE_TRADITIONAL_GIS_ORDER", "YES");
}

// Map certain GDAL enums to string names for use in R
// GDALColorInterp (GCI)
static const std::map<std::string, GDALColorInterp> MAP_GCI = {
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
static const std::map<std::string, GDALRATFieldUsage> MAP_GFU = {
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

// Internal lookup of GDALColorInterp by string descriptor
// Returns GCI_Undefined if no match
static GDALColorInterp getGCI_(const std::string &col_interp) {
    if (MAP_GCI.count(col_interp) == 0) {
        return GCI_Undefined;
    }
    else {
        auto gci = MAP_GCI.find(col_interp);
        return gci->second;
    }
}

// Internal lookup of GCI string by GDALColorInterp
// Returns "Undefined" if no match
static std::string getGCI_string_(GDALColorInterp gci) {
    for (auto it = MAP_GCI.begin(); it != MAP_GCI.end(); ++it)
        if (it->second == gci)
            return it->first;

    return "Undefined";
}

// Internal lookup of GDALRATFieldUsage by string descriptor
// Returns GFU_Generic if no match
static GDALRATFieldUsage getGFU_(const std::string &fld_usage) {
    if (MAP_GFU.count(fld_usage) == 0) {
        Rcpp::warning("unrecognized GFU string, using GFU_Generic");
        return GFU_Generic;
    }
    else {
        auto gfu = MAP_GFU.find(fld_usage);
        return gfu->second;
    }
}

// Internal lookup of GFU string by GDALRATFieldUsage
// Returns "Generic" if no match
static std::string getGFU_string_(GDALRATFieldUsage gfu) {
    for (auto it = MAP_GFU.begin(); it != MAP_GFU.end(); ++it)
        if (it->second == gfu)
            return it->first;

    Rcpp::warning("unrecognized GDALRATFieldUsage, using GFU_Generic");
    return "Generic";
}

// ****************************************************************************
//  Implementation of exposed class GDALRaster, which wraps a raster
//  GDALDataset and its GDALRasterBand objects.
//  Documented in R/gdalraster.R.
// ****************************************************************************

GDALRaster::GDALRaster()
        : m_fname(""), m_open_options(Rcpp::CharacterVector::create()),
          m_shared(false), m_allowed_drivers(Rcpp::CharacterVector::create()) {}

GDALRaster::GDALRaster(const Rcpp::CharacterVector &filename)
        : GDALRaster(filename, true, R_NilValue, true,
                     Rcpp::CharacterVector::create()) {}

GDALRaster::GDALRaster(const Rcpp::CharacterVector &filename,
                       bool read_only)
        : GDALRaster(filename, read_only, R_NilValue, true,
                     Rcpp::CharacterVector::create()) {}

GDALRaster::GDALRaster(const Rcpp::CharacterVector &filename, bool read_only,
                       const Rcpp::CharacterVector &open_options)
        : GDALRaster(filename, read_only, open_options, true,
                     Rcpp::CharacterVector::create()) {}

GDALRaster::GDALRaster(const Rcpp::CharacterVector &filename, bool read_only,
                       const Rcpp::Nullable<Rcpp::CharacterVector>
                           &open_options,
                       bool shared)
        : GDALRaster(filename, read_only, open_options, shared,
                     Rcpp::CharacterVector::create()) {}

GDALRaster::GDALRaster(const Rcpp::CharacterVector &filename, bool read_only,
                       const Rcpp::Nullable<Rcpp::CharacterVector>
                           &open_options,
                       bool shared,
                       const Rcpp::CharacterVector &allowed_drivers)
        : m_shared(shared), m_allowed_drivers(allowed_drivers) {

    m_fname = Rcpp::as<std::string>(check_gdal_filename(filename));

    if (open_options.isNotNull())
        m_open_options = open_options;
    else
        m_open_options = Rcpp::CharacterVector::create();

    open(read_only);

    // warn for now if 64-bit integer
    if (hasInt64_())
        warnInt64_();
}

GDALRaster::~GDALRaster() {
    if (m_hDataset) {
        // use GDALClose() on shared, and driver-less datasets such as the one
        // returned by mdim_as_classic()
        if (m_shared || !GDALGetDatasetDriver(m_hDataset))
            GDALClose(m_hDataset);
        else
            GDALReleaseDataset(m_hDataset);
    }
}

std::string GDALRaster::getFilename() const {
    return m_fname;
}

void GDALRaster::setFilename(const std::string &filename) {
    if (m_hDataset != nullptr) {
        if (m_fname == "" && getDescription(0) == "") {
            m_fname = Rcpp::as<std::string>(check_gdal_filename(filename));
            setDescription(0, m_fname);
        }
        else {
            Rcpp::stop("the filename cannot be set on this object");
        }
    }
    else {
        if (m_fname == "")
            m_fname = Rcpp::as<std::string>(check_gdal_filename(filename));
        else
            Rcpp::stop("the filename cannot be set on this object");
    }
}

void GDALRaster::open(bool read_only) {
    if (m_fname == "")
        Rcpp::stop("'filename' is not set");

    if (m_hDataset != nullptr)
        close();

    std::vector<char *> dsoo = {};
    if (m_open_options.size() > 0) {
        for (R_xlen_t i = 0; i < m_open_options.size(); ++i) {
            dsoo.push_back((char *) m_open_options[i]);
        }
        dsoo.push_back(nullptr);
    }

    std::vector<char *> allowed_drivers = {};
    if (m_allowed_drivers.size() > 0) {
        for (R_xlen_t i = 0; i < m_allowed_drivers.size(); ++i) {
            allowed_drivers.push_back((char *) m_allowed_drivers[i]);
        }
        allowed_drivers.push_back(nullptr);
    }

    unsigned int nOpenFlags = GDAL_OF_RASTER;
    if (read_only) {
        m_eAccess = GA_ReadOnly;
        nOpenFlags |= GDAL_OF_READONLY;
    }
    else {
        m_eAccess = GA_Update;
        nOpenFlags |= GDAL_OF_UPDATE;
    }
    if (m_shared)
        nOpenFlags |= GDAL_OF_SHARED;

    nOpenFlags |= GDAL_OF_VERBOSE_ERROR;

    m_hDataset = GDALOpenEx(
        m_fname.c_str(), nOpenFlags,
        allowed_drivers.empty() ? nullptr : allowed_drivers.data(),
        dsoo.empty() ? nullptr : dsoo.data(), nullptr);

    if (m_hDataset == nullptr)
        Rcpp::stop("open raster failed");
}

bool GDALRaster::isOpen() const {
    if (m_hDataset == nullptr)
        return false;
    else
        return true;
}

void GDALRaster::info() const {
    checkAccess_(GA_ReadOnly);

    // see https://github.com/OSGeo/gdal/issues/13106
    // avoid crash with a driver-less dataset and GDAL < 3.12
    GDALDriverH hDriver = GDALGetDatasetDriver(m_hDataset);
    if (!hDriver && GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 12, 0)) {
        Rcpp::Rcout <<
            "info() unavailable for driver-less dataset and GDAL < 3.12\n";

        return;
    }

    const Rcpp::CharacterVector argv = this->infoOptions;
    std::vector<char *> opt(1);
    if (argv.size() == 0 || (argv.size() == 1 && argv[0] == "")) {
        opt[0] = nullptr;
    }
    else {
        opt.resize(argv.size() + 1);
        for (R_xlen_t i = 0; i < argv.size(); ++i) {
            opt[i] = (char *) argv[i];
        }
        opt[argv.size()] = nullptr;
    }

    GDALInfoOptions* psOptions = GDALInfoOptionsNew(opt.data(), nullptr);
    if (psOptions == nullptr)
        Rcpp::stop("creation of GDALInfoOptions failed (check $infoOptions)");
    char *pszGDALInfoOutput = GDALInfo(m_hDataset, psOptions);
    if (pszGDALInfoOutput != nullptr)
        Rcpp::Rcout << pszGDALInfoOutput;
    GDALInfoOptionsFree(psOptions);
    CPLFree(pszGDALInfoOutput);
}

Rcpp::String GDALRaster::infoAsJSON() const {
    checkAccess_(GA_ReadOnly);

    Rcpp::String out = "";

    // see https://github.com/OSGeo/gdal/issues/13106
    // avoid crash with a driver-less dataset and GDAL < 3.12
    GDALDriverH hDriver = GDALGetDatasetDriver(m_hDataset);
    if (!hDriver && GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 12, 0)) {
        Rcpp::Rcout <<
            "info() unavailable for driver-less dataset and GDAL < 3.12\n";

        return out;
    }

    const Rcpp::CharacterVector argv = this->infoOptions;
    std::vector<char *> opt = {nullptr};
    if (argv.size() == 1 && argv[0] == "") {
        opt.resize(2);
        opt[0] = const_cast<char *>("-json");
        opt[1] = nullptr;
    }
    else {
        opt[0] = const_cast<char *>("-json");
        for (R_xlen_t i = 0; i < argv.size(); ++i) {
            if (EQUAL(argv[i], "-json"))
                continue;
            opt.push_back((char *) argv[i]);
        }
        opt.push_back(nullptr);
    }

    GDALInfoOptions* psOptions = GDALInfoOptionsNew(opt.data(), nullptr);
    if (psOptions == nullptr)
        Rcpp::stop("creation of GDALInfoOptions failed (check '$infoOptions')");

    char *pszGDALInfoOutput = GDALInfo(m_hDataset, psOptions);
    if (pszGDALInfoOutput != nullptr)
        out = pszGDALInfoOutput;

    GDALInfoOptionsFree(psOptions);

    // out.replace_all("\n", " ");
    CPLFree(pszGDALInfoOutput);

    return out;
}

Rcpp::CharacterVector GDALRaster::getFileList() const {
    checkAccess_(GA_ReadOnly);

    char **papszFiles = GDALGetFileList(m_hDataset);
    int nItems = CSLCount(papszFiles);
    if (nItems > 0) {
        Rcpp::CharacterVector files(nItems);
        for (int i = 0; i < nItems; ++i) {
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
    checkAccess_(GA_ReadOnly);

    GDALDriverH hDriver = GDALGetDatasetDriver(m_hDataset);
    if (!hDriver)
        return("");

    return GDALGetDriverShortName(hDriver);
}

std::string GDALRaster::getDriverLongName() const {
    checkAccess_(GA_ReadOnly);

    GDALDriverH hDriver = GDALGetDatasetDriver(m_hDataset);
    if (!hDriver)
        return("");

    return GDALGetDriverLongName(hDriver);
}

double GDALRaster::getRasterXSize() const {
    checkAccess_(GA_ReadOnly);

    // return as R numeric (double) to avoid integer overflow when multiplying
    return static_cast<double>(GDALGetRasterXSize(m_hDataset));
}

double GDALRaster::getRasterYSize() const {
    checkAccess_(GA_ReadOnly);

    // return as R numeric (double) to avoid integer overflow when multiplying
    return static_cast<double>(GDALGetRasterYSize(m_hDataset));
}

Rcpp::NumericVector GDALRaster::getGeoTransform() const {
    checkAccess_(GA_ReadOnly);

    // returned by GDALGetGeoTransform() even when CE_Failure:
    Rcpp::NumericVector gt = {0, 1, 0, 0, 0, 1};

    CPLErr err = CE_None;
    err = GDALGetGeoTransform(m_hDataset, gt.begin());
    if (!quiet && err == CE_Failure)
        Rcpp::warning("failed to get geotransform, default returned");

    return gt;
}

bool GDALRaster::setGeoTransform(const Rcpp::NumericVector &transform) {
    checkAccess_(GA_Update);

    if (transform.size() != 6)
        Rcpp::stop("setGeoTransform() requires a numeric vector of length 6");

    if (GDALSetGeoTransform(m_hDataset, const_cast<double *>(transform.begin()))
        == CE_Failure) {

        if (!quiet)
            Rcpp::Rcout << "set geotransform failed\n";
        return false;
    }
    else {
        return true;
    }
}

int GDALRaster::getRasterCount() const {
    checkAccess_(GA_ReadOnly);

    return GDALGetRasterCount(m_hDataset);
}

bool GDALRaster::addBand(const std::string &dataType,
                         const Rcpp::Nullable<Rcpp::CharacterVector> &options) {

    checkAccess_(GA_Update);

    GDALDataType dt = GDALGetDataTypeByName(dataType.c_str());
    if (dt == GDT_Unknown)
        Rcpp::stop("'dataType' is unknown");

    std::vector<const char *> opt_list = {nullptr};
    if (options.isNotNull()) {
        Rcpp::CharacterVector options_in(options);
        opt_list.resize(options_in.size() + 1);
        for (R_xlen_t i = 0; i < options_in.size(); ++i) {
            opt_list[i] = (const char *) options_in[i];
        }
        opt_list[options_in.size()] = nullptr;
    }

    CPLErr err = CE_None;
    err = GDALAddBand(m_hDataset, dt, opt_list.data());
    if (err != CE_None) {
        return false;
    }
    else {
        return true;
    }
}

std::string GDALRaster::getProjection() const {
    return getProjectionRef();
}

std::string GDALRaster::getProjectionRef() const {
    checkAccess_(GA_ReadOnly);

    std::string srs(GDALGetProjectionRef(m_hDataset));
    if (srs.size() > 0 && srs != "") {
        return srs;
    }
    else {
        if (!quiet)
            Rcpp::Rcout << "failed to get projection ref\n";
        return "";
    }
}

bool GDALRaster::setProjection(const std::string &projection) {
    checkAccess_(GA_Update);

    if (projection.size() == 0 || projection == "") {
        if (!quiet)
            Rcpp::Rcout << "setProjection() requires a WKT string\n";
        return false;
    }

    if (GDALSetProjection(m_hDataset, projection.c_str()) == CE_Failure) {
        if (!quiet) {
            Rcpp::Rcout << "set projection failed\n";
        }
        return false;
    }
    else {
        return true;
    }
}

Rcpp::NumericVector GDALRaster::bbox() const {
    checkAccess_(GA_ReadOnly);

    return bbox_grid_to_geo_(getGeoTransform(),
                             0.0, GDALGetRasterXSize(m_hDataset),
                             0.0, GDALGetRasterYSize(m_hDataset));
}

Rcpp::NumericVector GDALRaster::res() const {
    checkAccess_(GA_ReadOnly);

    Rcpp::NumericVector gt = getGeoTransform();
    Rcpp::NumericVector ret = {NA_REAL, NA_REAL};

    if (gt[2] == 0.0 && gt[4] == 0.0) {
        ret[0] = gt[1];
        ret[1] = std::fabs(gt[5]);
    }
    else {
        if (!quiet)
            Rcpp::warning("rotated raster unsupported by res(), NA returned");
    }

    return ret;
}

Rcpp::NumericVector GDALRaster::dim() const {
    checkAccess_(GA_ReadOnly);

    // return as R numeric (double) to avoid integer overflow when multiplying
    Rcpp::NumericVector ret = {getRasterXSize(), getRasterYSize(),
                               static_cast<double>(getRasterCount())};
    return ret;
}

Rcpp::NumericMatrix GDALRaster::apply_geotransform(
        const Rcpp::RObject& col_row) const {

    checkAccess_(GA_ReadOnly);

    return apply_geotransform_ds(col_row, this);
}

Rcpp::IntegerMatrix GDALRaster::get_pixel_line(const Rcpp::RObject &xy) const {
    checkAccess_(GA_ReadOnly);

    return get_pixel_line_ds(xy, this);
}

Rcpp::NumericMatrix GDALRaster::pixel_extract(const Rcpp::RObject &xy,
                                              const Rcpp::IntegerVector &bands,
                                              const std::string &interp,
                                              int krnl_dim,
                                              const std::string &xy_srs) const {

    /*
       *************************************************************************
       undocumented method with public wrapper in R/gdalraster_proc.R

       extract pixel values at point locations
       xy:          geospatial xy coordinates in the same projection as the
                    raster, a 2-column data frame or matrix
       bands:       band number(s), or 0 to extract from all bands
       interp:      one of "nearest", "bilinear" (2x2 kernel),
                    "cubic" (4x4 kernel) or "cubicspline" (4x4 kernel)
       krnl_dim:    1 for single-pixel extract at xy (with
                    interp = "nearest"), or the size of a square kernel to
                    extract all pixels, e.g., krnl_dim = 3 to return the values
                    of the 9 pixels in a 3x3 kernel centered on the pixel
                    containing xy
                    ignored if interp is not "nearest" (will use the kernel
                    implied by the given interpolation method)
       xy_srs:      character string specifying the spatial reference system
                    for xy. May be in WKT format or any of the formats
                    supported by srs_to_wkt().
       *************************************************************************
    */

    checkAccess_(GA_ReadOnly);

    constexpr int KRNL_DIM_MAX_ = 1000;

    Rcpp::NumericMatrix xy_in;
    if (Rcpp::is<Rcpp::NumericVector>(xy) ||
        Rcpp::is<Rcpp::IntegerVector>(xy)) {

        if (!Rf_isMatrix(xy)) {
            Rcpp::NumericVector v(xy);
            if (v.size() != 2)
                Rcpp::stop("'xy' must be a two-column data frame or matrix");

            xy_in = Rcpp::NumericMatrix(1, 2, v.begin());
        }
        else {
            xy_in = Rcpp::NumericMatrix(xy);
        }
    }
    else if (Rcpp::is<Rcpp::DataFrame>(xy)) {
        xy_in = df_to_matrix_(xy);
    }
    else {
        Rcpp::stop("'xy' must be a two-column data frame or matrix");
    }

    if (xy_srs != "")
        xy_in = transform_xy(xy_in, xy_srs, getProjection());

    if (xy_in.nrow() == 0)
        Rcpp::stop("input matrix is empty");

    const R_xlen_t num_pts = xy_in.nrow();

    if (xy_in.ncol() != 2)
        Rcpp::stop("input matrix must have 2 columns");

    Rcpp::IntegerVector bands_in;
    if (bands[0] == 0)
        bands_in = Rcpp::seq(1, getRasterCount());
    else
        bands_in = bands;

    const R_xlen_t num_bands = bands_in.size();

    Rcpp::CharacterVector band_names = Rcpp::CharacterVector::create();
    for (const auto &b : bands_in) {
        GDALRasterBandH hBand = GDALGetRasterBand(m_hDataset, b);
        if (hBand == nullptr) {
            Rcpp::Rcout << "invalid band number: " << b << "\n";
            Rcpp::stop("failed to access the requested band");
        }
        GDALDataType eDT = GDALGetRasterDataType(hBand);
        if (GDALDataTypeIsComplex(eDT)) {
            Rcpp::stop("complex data types currently unsupported for extract");
        }
        std::string nm = std::string("b") + std::to_string(b);
        band_names.push_back(nm);
    }

    GDALRIOResampleAlg eResampleAlg;
    if (EQUAL(interp.c_str(), "nearest") || EQUAL(interp.c_str(), "near")) {
        eResampleAlg = GRIORA_NearestNeighbour;
    }
    else if (EQUAL(interp.c_str(), "bilinear")) {
        eResampleAlg = GRIORA_Bilinear;
    }
    else if (EQUAL(interp.c_str(), "cubic")) {
        if (GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 10, 0))
            Rcpp::stop("'cubic' interpolation requires GDAL >= 3.10");

        eResampleAlg = GRIORA_Cubic;
    }
    else if (EQUAL(interp.c_str(), "cubicspline")) {
        if (GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 10, 0))
            Rcpp::stop("'cubicspline' interpolation requires GDAL >= 3.10");

        eResampleAlg = GRIORA_CubicSpline;
    }
    else {
        Rcpp::stop("'interp' is invalid");
    }

    if (krnl_dim < 1)
        Rcpp::stop("'krnl_dim' must be a positive number");
    if (krnl_dim > KRNL_DIM_MAX_)
        Rcpp::stop("'krnl_dim' must be <= " + std::to_string(KRNL_DIM_MAX_));

    if (eResampleAlg == GRIORA_NearestNeighbour && krnl_dim > 1 &&
        num_bands > 1) {

        Rcpp::stop(
            "one band must be specified to extract pixel values for kernel");
    }

    const Rcpp::NumericVector inv_gt = inv_geotransform(getGeoTransform());
    if (Rcpp::is_true(Rcpp::any(Rcpp::is_na(inv_gt))))
        Rcpp::stop("failed to get inverse geotransform");

    const int krnl_size = krnl_dim * krnl_dim;
    const int raster_xsize = GDALGetRasterXSize(m_hDataset);
    const int raster_ysize = GDALGetRasterYSize(m_hDataset);

    GDALProgressFunc pfnProgress = GDALTermProgressR;
    uint64_t pts_outside = 0;

    Rcpp::NumericMatrix values;
    if (krnl_dim == 1 || eResampleAlg != GRIORA_NearestNeighbour) {
        values = Rcpp::no_init(num_pts, num_bands);
        Rcpp::colnames(values) = band_names;
    }
    else {
        // for returning individual pixel values within the kernel
        Rcpp::CharacterVector col_names = {};
        values = Rcpp::no_init(num_pts, krnl_size);
        for (int i = 0; i < krnl_size; ++i) {
            col_names.push_back(
                std::string(band_names[0]) + "_p" + std::to_string(i + 1));
        }
        Rcpp::colnames(values) = col_names;
    }

    for (R_xlen_t band_idx = 0; band_idx < num_bands; ++band_idx) {
        if (!quiet) {
            Rcpp::Rcout << "extracting from band " << bands_in[band_idx]
                << "...\n";

            pfnProgress(0, nullptr, nullptr);
        }

        for (R_xlen_t row_idx = 0; row_idx < num_pts; ++row_idx) {
            // row_idx refers to rows of the input and output matrices
            const double geo_x = xy_in(row_idx, 0);
            const double geo_y = xy_in(row_idx, 1);
            if (Rcpp::NumericVector::is_na(geo_x) ||
                Rcpp::NumericVector::is_na(geo_y)) {

                values.row(row_idx) = Rcpp::NumericVector(values.ncol(),
                                                          NA_REAL);
                continue;
            }

            double grid_x = inv_gt[0] + inv_gt[1] * geo_x + inv_gt[2] * geo_y;
            double grid_y = inv_gt[3] + inv_gt[4] * geo_x + inv_gt[5] * geo_y;

            // allow input coordinates exactly on the bottom or right edges
            // match behavior in: https://github.com/OSGeo/gdal/pull/12087
            const bool pt_is_on_right_edge =
                ARE_REAL_EQUAL(grid_x, static_cast<double>(raster_xsize));
            const bool pt_is_on_bottom_edge =
                ARE_REAL_EQUAL(grid_y, static_cast<double>(raster_ysize));

            if ((grid_x < 0 || grid_x > static_cast<double>(raster_xsize) ||
                 grid_y < 0 || grid_y > static_cast<double>(raster_ysize)) &&
                !(pt_is_on_right_edge || pt_is_on_bottom_edge)) {

                if (band_idx == 0)
                    pts_outside += 1;

                values.row(row_idx) = Rcpp::NumericVector(values.ncol(),
                                                          NA_REAL);
                continue;
            }

            if (eResampleAlg == GRIORA_NearestNeighbour && krnl_dim == 1) {
                if (pt_is_on_right_edge)
                    grid_x -= 0.25;
                if (pt_is_on_bottom_edge)
                    grid_y -= 0.25;

                const int x_off = static_cast<int>(std::floor(grid_x));
                const int y_off = static_cast<int>(std::floor(grid_y));

                double v = Rcpp::as<double>(read(bands_in[band_idx],
                                                 x_off, y_off, 1, 1, 1, 1));

                values(row_idx, band_idx) = v;
            }
            else if (eResampleAlg == GRIORA_Bilinear) {
                int x_off = static_cast<int>(std::floor(grid_x - 0.5));
                int y_off = static_cast<int>(std::floor(grid_y - 0.5));

                // allow the 2x2 kernel to be outside the extent by one
                // pixel dimension and handle the border cases
                if (x_off < -1 || x_off + 2 > raster_xsize + 1 ||
                    y_off < -1 || y_off + 2 > raster_ysize + 1) {

                    if (band_idx == 0)
                        pts_outside += 1;

                    values(row_idx, band_idx) = NA_REAL;
                    continue;
                }

                // x_off and y_off might be at most one pixel outside the extent
                int read_xsize = 2;
                if (x_off < 0) {
                    x_off = 0;
                    read_xsize = 1;
                }
                else if (x_off + 2 > raster_xsize) {
                    x_off = raster_xsize - 1;
                    read_xsize = 1;
                }
                int read_ysize = 2;
                if (y_off < 0) {
                    y_off = 0;
                    read_ysize = 1;
                }
                else if (y_off + 2 > raster_ysize) {
                    y_off = raster_ysize - 1;
                    read_ysize = 1;
                }

                Rcpp::NumericVector v(read(bands_in[band_idx], x_off, y_off,
                                           read_xsize, read_ysize,
                                           read_xsize, read_ysize));

                if (Rcpp::is_true(Rcpp::any(Rcpp::is_na(v)))) {
                    values(row_idx, band_idx) = NA_REAL;
                    continue;
                }

                if (v.size() == 4) {
                    // Convert to unit square coordinates for the 2x2 kernel.
                    // The center of the lower left pixel in the kernel is 0,0.
                    const double x = grid_x - (x_off + 0.5);
                    const double y = (y_off + 1.5) - grid_y;

                    // Pixels in v are left to right, top to bottom.
                    // Pixel values in the square:
                    // 0,0: v[2]
                    // 1,0: v[3]
                    // 0,1: v[0]
                    // 1,1: v[1]
                    values(row_idx, band_idx) = (v[2] * (1.0 - x) * (1.0 - y) +
                                                 v[3] * x * (1.0 - y) +
                                                 v[0] * (1.0 - x) * y +
                                                 v[1] * x * y);
                }
                else if (read_xsize == 2 && read_ysize == 1) {
                    // linear interp along x
                    const double t = grid_x - (x_off + 0.5);
                    values(row_idx, band_idx) = v[0] + t * (v[1] - v[0]);
                }
                else if (read_xsize == 1 && read_ysize == 2) {
                    // linear interp along y
                    const double t = (y_off + 1.5) - grid_y;
                    values(row_idx, band_idx) = v[0] + t * (v[1] - v[0]);
                }
                else if (v.size() == 1) {
                    // corner pixel, return its value
                    values(row_idx, band_idx) = v[0];
                }
            }
#if GDAL_VERSION_NUM >= GDAL_COMPUTE_VERSION(3, 10, 0)
            else if (eResampleAlg == GRIORA_Cubic ||
                     eResampleAlg == GRIORA_CubicSpline) {

                double dfRealValue = NA_REAL;
                double dfImagValue = NA_REAL;
                CPLErr err = CE_None;
                GDALRasterBandH hBand = getBand_(bands_in[band_idx]);

                err = GDALRasterInterpolateAtPoint(hBand, grid_x, grid_y,
                                                   eResampleAlg,
                                                   &dfRealValue, &dfImagValue);

                if (err != CE_None)
                    values(row_idx, band_idx) = NA_REAL;
                else
                    values(row_idx, band_idx) = dfRealValue;
            }
#endif
            else {
                // all pixel values in kernel
                const int x_off = static_cast<int>(
                    std::floor(grid_x - ((krnl_dim / 2.0) - 0.5)));

                const int y_off = static_cast<int>(
                    std::floor(grid_y - ((krnl_dim / 2.0) - 0.5)));

                // Is any portion of the kernel outside the raster extent?
                // The wrapper in R/gdalraster_proc.R avoids this as long
                // as the point itself is inside, by reading through a VRT
                // that extends the bounds.
                if (x_off < 0 || x_off + krnl_dim > raster_xsize ||
                    y_off < 0 || y_off + krnl_dim > raster_ysize) {

                    if (band_idx == 0)
                        pts_outside += 1;

                    values.row(row_idx) = Rcpp::NumericVector(krnl_size,
                                                              NA_REAL);
                    continue;
                }

                values.row(row_idx) =
                    Rcpp::NumericVector(read(bands_in[band_idx], x_off, y_off,
                                             krnl_dim, krnl_dim,
                                             krnl_dim, krnl_dim));
            }

            if (!quiet) {
                pfnProgress((row_idx + 1.0) / num_pts, nullptr, nullptr);
            }
            if (row_idx % 10000 == 0) {
                Rcpp::checkUserInterrupt();
            }
        }
    }

    if (!quiet && pts_outside > 0) {
        std::string msg =
            "point(s) were outside the raster extent, NA returned";

        Rcpp::warning(std::to_string(pts_outside) + " " + msg);
    }

    return values;
}

Rcpp::NumericMatrix GDALRaster::get_block_indexing(int band) const {
    checkAccess_(GA_ReadOnly);

    GDALRasterBandH hBand = getBand_(band);

    const double dfRasterXSize = getRasterXSize();
    const double dfRasterYSize = getRasterYSize();

    int nBlockXSize = 0;
    int nBlockYSize = 0;
    GDALGetBlockSize(hBand, &nBlockXSize, &nBlockYSize);
    if (nBlockXSize < 1 || nBlockYSize < 0) {
        // assume scanlines
        if (!quiet)
            Rcpp::Rcout << "invalid block size reported for this band, "
                           "assuming one whole row\n";
        nBlockXSize = GDALGetRasterXSize(m_hDataset);
        nBlockYSize = 1;
    }

    const int num_blocks_x = static_cast<int>(
        std::ceil(dfRasterXSize / nBlockXSize));

    const int num_blocks_y = static_cast<int>(
        std::ceil(dfRasterYSize / nBlockYSize));

    const R_xlen_t num_blocks =
        static_cast<R_xlen_t>(num_blocks_x) * num_blocks_y;

    Rcpp::NumericMatrix blocks = Rcpp::no_init(num_blocks, 10);
    Rcpp::colnames(blocks) =
        Rcpp::CharacterVector::create("xblockoff", "yblockoff", "xoff", "yoff",
                                      "xsize", "ysize", "xmin", "xmax", "ymin",
                                      "ymax");

    R_xlen_t out_row_i = 0;
    for (int y = 0; y < num_blocks_y; ++y) {
        for (int x = 0; x < num_blocks_x; ++x) {
            const int this_xoff = x * nBlockXSize;
            const int this_yoff = y * nBlockYSize;
            const Rcpp::NumericVector &this_size =
                getActualBlockSize(band, x, y);

            const Rcpp::NumericVector &this_bbox =
                bbox_grid_to_geo_(getGeoTransform(),
                                  this_xoff, this_xoff + this_size[0],
                                  this_yoff, this_yoff + this_size[1]);

            blocks.row(out_row_i) =
                Rcpp::NumericVector::create(x, y, this_xoff, this_yoff,
                                            this_size[0], this_size[1],
                                            this_bbox[0], this_bbox[2],
                                            this_bbox[1], this_bbox[3]);

            ++out_row_i;
        }
    }

    return blocks;
}

Rcpp::NumericVector GDALRaster::getBlockSize(int band) const {
    checkAccess_(GA_ReadOnly);

    GDALRasterBandH hBand = getBand_(band);
    Rcpp::IntegerVector panBlockSize = {NA_INTEGER, NA_INTEGER};
    GDALGetBlockSize(hBand, panBlockSize.begin(), panBlockSize.begin() + 1);
    return Rcpp::NumericVector(panBlockSize);
}

Rcpp::NumericVector GDALRaster::getActualBlockSize(int band, int xblockoff,
                                                   int yblockoff) const {
    checkAccess_(GA_ReadOnly);

    GDALRasterBandH hBand = getBand_(band);
    Rcpp::IntegerVector panBlockSize = {NA_INTEGER, NA_INTEGER};
    GDALGetActualBlockSize(hBand, xblockoff, yblockoff,
                           panBlockSize.begin(), panBlockSize.begin() + 1);

    return Rcpp::NumericVector(panBlockSize);
}

Rcpp::NumericMatrix GDALRaster::make_chunk_index(int band,
    const Rcpp::NumericVector &max_pixels) const {
    //' must be kept in sync with readChunk() / writeChunk()
    //' 'max_pixels' is a scalar value, but NumericVector is used so it can
    //' optionally carry the bit64::integer64 class attribute.

    checkAccess_(GA_ReadOnly);

    GDALRasterBandH hBand = getBand_(band);

    const int nRasterXSize = GDALGetRasterXSize(m_hDataset);
    const int nRasterYSize = GDALGetRasterYSize(m_hDataset);

    int nBlockXSize = 0;
    int nBlockYSize = 0;
    GDALGetBlockSize(hBand, &nBlockXSize, &nBlockYSize);
    if (nBlockXSize < 1 || nBlockYSize < 0) {
        // assume scanlines
        if (!quiet)
            Rcpp::Rcout << "invalid block size reported for this band, "
                           "assuming one whole row\n";
        nBlockXSize = nRasterXSize;
        nBlockYSize = 1;
    }

    return make_chunk_index_(nRasterXSize, nRasterYSize, nBlockXSize,
                             nBlockYSize, getGeoTransform(), max_pixels);
}

int GDALRaster::getOverviewCount(int band) const {
    checkAccess_(GA_ReadOnly);

    GDALRasterBandH hBand = getBand_(band);
    return GDALGetOverviewCount(hBand);
}

void GDALRaster::buildOverviews(const std::string &resampling,
                                std::vector<int> levels,
                                std::vector<int> bands) {

    checkAccess_(GA_ReadOnly);

    int nOvr = 0;
    int *panOvrList = nullptr;
    if (!(levels.size() == 1 && levels[0] == 0)) {
        nOvr = static_cast<int>(levels.size());
        panOvrList = levels.data();
    }

    int nBands = 0;
    int *panBandList = nullptr;
    if (!(bands.size() == 1 && bands[0] == 0)) {
        nBands = static_cast<int>(bands.size());
        panBandList = bands.data();
    }

    CPLErr err = GDALBuildOverviews(m_hDataset, resampling.c_str(), nOvr,
                                    panOvrList, nBands, panBandList,
                                    quiet ? nullptr : GDALTermProgressR,
                                    nullptr);

    if (err == CE_Failure) {
        Rcpp::stop("build overviews failed");
    }
}

std::string GDALRaster::getDataTypeName(int band) const {
    checkAccess_(GA_ReadOnly);

    GDALRasterBandH hBand = getBand_(band);
    return GDALGetDataTypeName(GDALGetRasterDataType(hBand));
}

bool GDALRaster::hasNoDataValue(int band) const {
    checkAccess_(GA_ReadOnly);

    GDALRasterBandH hBand = getBand_(band);
    int hasNoData = 0;
    GDALGetRasterNoDataValue(hBand, &hasNoData);
    return CPL_TO_BOOL(hasNoData);
}

double GDALRaster::getNoDataValue(int band) const {
    checkAccess_(GA_ReadOnly);

    if (hasNoDataValue(band)) {
        GDALRasterBandH hBand = getBand_(band);
        return GDALGetRasterNoDataValue(hBand, nullptr);
    }
    else {
        return NA_REAL;
    }
}

bool GDALRaster::setNoDataValue(int band, double nodata_value) {
    checkAccess_(GA_Update);

    if (Rcpp::NumericVector::is_na(nodata_value))
        return false;

    GDALRasterBandH hBand = getBand_(band);
    if (GDALSetRasterNoDataValue(hBand, nodata_value) == CE_Failure) {
        if (!quiet)
            Rcpp::Rcout << "set nodata value failed\n";
        return false;
    }
    else {
        return true;
    }
}

void GDALRaster::deleteNoDataValue(int band) {
    checkAccess_(GA_Update);

    GDALRasterBandH hBand = getBand_(band);
    if (GDALDeleteRasterNoDataValue(hBand) == CE_Failure) {
        Rcpp::stop("delete nodata value failed");
    }
}

Rcpp::List GDALRaster::getMaskFlags(int band) const {
    checkAccess_(GA_ReadOnly);

    GDALRasterBandH hBand = getBand_(band);
    int mask_flags = GDALGetMaskFlags(hBand);

    Rcpp::List list_out = Rcpp::List::create();

    if (mask_flags & GMF_ALL_VALID)
        list_out.push_back(true, "ALL_VALID");
    else
        list_out.push_back(false, "ALL_VALID");

    if (mask_flags & GMF_PER_DATASET)
        list_out.push_back(true, "PER_DATASET");
    else
        list_out.push_back(false, "PER_DATASET");

    if (mask_flags & GMF_ALPHA)
        list_out.push_back(true, "ALPHA");
    else
        list_out.push_back(false, "ALPHA");

    if (mask_flags & GMF_NODATA)
        list_out.push_back(true, "NODATA");
    else
        list_out.push_back(false, "NODATA");

    return list_out;
}

Rcpp::List GDALRaster::getMaskBand(int band) const {
    checkAccess_(GA_ReadOnly);

    GDALRasterBandH hBand = getBand_(band);

    Rcpp::List list_out = Rcpp::List::create();

    GDALRasterBandH hMaskBand = GDALGetMaskBand(hBand);
    int band_number = 0;
    if (hMaskBand != nullptr)
        band_number = GDALGetBandNumber(hMaskBand);

    std::string mask_file = "";
    GDALDatasetH hMaskDS = GDALGetBandDataset(hMaskBand);
    if (hMaskDS != nullptr)
        mask_file = GDALGetDescription(hMaskDS);

    list_out.push_back(mask_file, "MaskFile");
    list_out.push_back(band_number, "BandNumber");

    return list_out;
}

std::string GDALRaster::getUnitType(int band) const {
    checkAccess_(GA_ReadOnly);

    GDALRasterBandH hBand = getBand_(band);
    return GDALGetRasterUnitType(hBand);
}

bool GDALRaster::setUnitType(int band, const std::string &unit_type) {
    checkAccess_(GA_ReadOnly);

    GDALRasterBandH hBand = getBand_(band);
    if (GDALSetRasterUnitType(hBand, unit_type.c_str()) == CE_Failure) {
        if (!quiet)
            Rcpp::Rcout << "set unit type failed\n";
        return false;
    }
    else {
        return true;
    }
}

bool GDALRaster::hasScale(int band) const {
    checkAccess_(GA_ReadOnly);

    GDALRasterBandH hBand = getBand_(band);
    int hasScale = 0;
    GDALGetRasterScale(hBand, &hasScale);
    return CPL_TO_BOOL(hasScale);
}

double GDALRaster::getScale(int band) const {
    checkAccess_(GA_ReadOnly);

    if (hasScale(band)) {
        GDALRasterBandH hBand = getBand_(band);
        return GDALGetRasterScale(hBand, nullptr);
    }
    else {
        return NA_REAL;
    }
}

bool GDALRaster::setScale(int band, double scale) {
    checkAccess_(GA_ReadOnly);

    if (!std::isfinite(scale))
        return false;

    GDALRasterBandH hBand = getBand_(band);
    if (GDALSetRasterScale(hBand, scale) == CE_Failure) {
        if (!quiet)
            Rcpp::Rcout << "set scale failed\n";
        return false;
    }
    else {
        return true;
    }
}

bool GDALRaster::hasOffset(int band) const {
    checkAccess_(GA_ReadOnly);

    GDALRasterBandH hBand = getBand_(band);
    int hasOffset = 0;
    GDALGetRasterOffset(hBand, &hasOffset);
    return CPL_TO_BOOL(hasOffset);
}

double GDALRaster::getOffset(int band) const {
    checkAccess_(GA_ReadOnly);

    if (hasOffset(band)) {
        GDALRasterBandH hBand = getBand_(band);
        return GDALGetRasterOffset(hBand, nullptr);
    }
    else {
        return NA_REAL;
    }
}

bool GDALRaster::setOffset(int band, double offset) {
    checkAccess_(GA_ReadOnly);

    if (!std::isfinite(offset))
        return false;

    GDALRasterBandH hBand = getBand_(band);
    if (GDALSetRasterOffset(hBand, offset) == CE_Failure) {
        if (!quiet)
            Rcpp::Rcout << "set offset failed\n";
        return false;
    }
    else {
        return true;
    }
}

std::string GDALRaster::getDescription(int band) const {
    checkAccess_(GA_ReadOnly);

    std::string desc = "";

    if (band == 0) {
        desc = GDALGetDescription(m_hDataset);
    }
    else {
        GDALRasterBandH hBand = getBand_(band);
        desc = GDALGetDescription(hBand);
    }

    return desc;
}

void GDALRaster::setDescription(int band, const std::string &desc) {
    checkAccess_(GA_ReadOnly);

    if (band == 0) {
        GDALSetDescription(m_hDataset, desc.c_str());
    }
    else {
        GDALRasterBandH hBand = getBand_(band);
        GDALSetDescription(hBand, desc.c_str());
    }
}

std::string GDALRaster::getRasterColorInterp(int band) const {
    checkAccess_(GA_ReadOnly);

    GDALRasterBandH hBand = getBand_(band);
    GDALColorInterp gci = GDALGetRasterColorInterpretation(hBand);

    return getGCI_string_(gci);
}

void GDALRaster::setRasterColorInterp(int band, const std::string &col_interp) {
    checkAccess_(GA_ReadOnly);

    GDALRasterBandH hBand = getBand_(band);
    GDALColorInterp gci;
    if (col_interp == "Undefined") {
        gci = GCI_Undefined;
    }
    else {
        gci = getGCI_(col_interp);
        if (gci == GCI_Undefined)
            Rcpp::stop("invalid 'col_interp'");
    }

    GDALSetRasterColorInterpretation(hBand, gci);
}

Rcpp::NumericVector GDALRaster::getMinMax(int band, bool approx_ok) const {
    checkAccess_(GA_ReadOnly);

    GDALRasterBandH hBand = getBand_(band);
    Rcpp::NumericVector min_max = Rcpp::NumericVector::create(NA_REAL, NA_REAL);
    CPLErr err = CE_None;
#if GDAL_VERSION_NUM >= 3060000
    err = GDALComputeRasterMinMax(hBand, approx_ok, min_max.begin());
#else
    GDALComputeRasterMinMax(hBand, approx_ok, min_max.begin());
#endif
    if (err != CE_None)
        Rcpp::Rcout << "error from GDALComputeRasterMinMax()\n";

    return min_max;
}

Rcpp::NumericVector GDALRaster::getMinMaxLocation(int band) const {
#if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 11, 0)
    Rcpp::stop("GDALComputeRasterMinMaxLocation() requires GDAL >= 3.11");
#else
    checkAccess_(GA_ReadOnly);

    GDALRasterBandH hBand = getBand_(band);
    CPLErr err = CE_None;
    double dfMin = NA_REAL, dfMax = NA_REAL;
    int nMinX = NA_INTEGER, nMinY = NA_INTEGER;
    int nMaxX = NA_INTEGER, nMaxY = NA_INTEGER;

    Rcpp::CharacterVector ret_names =
        Rcpp::CharacterVector::create("min", "min_col", "min_row", "min_geo_x",
                                      "min_geo_y", "min_wgs84_lon",
                                      "min_wgs84_lat", "max", "max_col",
                                      "max_row", "max_geo_x", "max_geo_y",
                                      "max_wgs84_lon", "max_wgs84_lat");

    err = GDALComputeRasterMinMaxLocation(hBand, &dfMin, &dfMax, &nMinX, &nMinY,
                                          &nMaxX, &nMaxY);

    if (err != CE_None) {
        Rcpp::Rcout << "error in GDALComputeRasterMinMaxLocation() or no valid "
                       "values returned\n";
        Rcpp::NumericVector ret(ret_names.size(), NA_REAL);
        ret.names() = ret_names;
        return ret;
    }

    Rcpp::NumericVector min_geo_xy =
        apply_geotransform(Rcpp::NumericVector{nMinX + 0.5, nMinY + 0.5});
    Rcpp::NumericVector max_geo_xy =
        apply_geotransform(Rcpp::NumericVector{nMaxX + 0.5, nMaxY + 0.5});

    Rcpp::NumericVector min_wgs84 = {NA_REAL, NA_REAL};
    Rcpp::NumericVector max_wgs84 = {NA_REAL, NA_REAL};
    if (getProjection() != "") {
        min_wgs84 = transform_xy(min_geo_xy, getProjection(), "WGS84").row(0);
        max_wgs84 = transform_xy(max_geo_xy, getProjection(), "WGS84").row(0);
    }

    Rcpp::NumericVector ret =
        Rcpp::NumericVector::create(dfMin, nMinX, nMinY, min_geo_xy(0),
                                    min_geo_xy(1), min_wgs84(0), min_wgs84(1),
                                    dfMax, nMaxX, nMaxY, max_geo_xy(0),
                                    max_geo_xy(1), max_wgs84(0), max_wgs84(1));
    ret.names() = ret_names;
    return ret;
#endif
}

Rcpp::NumericVector GDALRaster::getStatistics(int band, bool approx_ok,
                                              bool force) const {

    checkAccess_(GA_ReadOnly);

    GDALRasterBandH hBand = getBand_(band);
    double min = NA_REAL, max = NA_REAL, mean = NA_REAL, sd = NA_REAL;
    CPLErr err = CE_None;
    GDALProgressFunc pfnProgress = nullptr;
    void *pProgressData = nullptr;

    if (!force) {
        err = GDALGetRasterStatistics(hBand, approx_ok, force,
                                      &min, &max, &mean, &sd);
    }
    else {
        if (!quiet)
            pfnProgress = GDALTermProgressR;

        err = GDALComputeRasterStatistics(hBand, approx_ok,
                                          &min, &max, &mean, &sd,
                                          pfnProgress, pProgressData);
    }

    if (err != CE_None) {
        if (!quiet)
            Rcpp::Rcout << "failed to get statistics, 'NA' returned\n";
        Rcpp::NumericVector stats(4, NA_REAL);
        return stats;
    }
    else {
        Rcpp::NumericVector stats = {min, max, mean, sd};
        return stats;
    }
}

void GDALRaster::clearStatistics() {
    checkAccess_(GA_ReadOnly);

#if GDAL_VERSION_NUM >= 3020000
    GDALDatasetClearStatistics(m_hDataset);
#else
    Rcpp::Rcout << "clearStatistics() requires GDAL >= 3.2\n";
#endif
}

Rcpp::NumericVector GDALRaster::getHistogram(int band, double min, double max,
                                             int num_buckets,
                                             bool incl_out_of_range,
                                             bool approx_ok) const {

    checkAccess_(GA_ReadOnly);

    GDALRasterBandH hBand = getBand_(band);
    std::vector<GUIntBig> hist(num_buckets);
    CPLErr err = GDALGetRasterHistogramEx(hBand, min, max, num_buckets,
                                          hist.data(), incl_out_of_range,
                                          approx_ok,
                                          quiet ? nullptr : GDALTermProgressR,
                                          nullptr);

    if (err != CE_None)
        Rcpp::stop("failed to get histogram");

    return Rcpp::NumericVector(hist.begin(), hist.end());
}

Rcpp::List GDALRaster::getDefaultHistogram(int band, bool force) const {
    checkAccess_(GA_ReadOnly);

    GDALRasterBandH hBand = getBand_(band);
    double min = NA_REAL;
    double max = NA_REAL;
    int num_buckets = 0;
    GUIntBig *panHistogram = nullptr;

    CPLErr err = GDALGetDefaultHistogramEx(hBand, &min, &max, &num_buckets,
                                           &panHistogram, force,
                                           quiet ? nullptr : GDALTermProgressR,
                                           nullptr);

    if (err == CE_Failure)
        Rcpp::stop("failed to get default histogram");

    if (err == CE_Warning)
        Rcpp::warning("no default histogram is available");

    std::vector<double> hist(num_buckets, NA_REAL);

    if (err == CE_None) {
        for (int i = 0; i < num_buckets; ++i)
            hist[i] = static_cast<double>(panHistogram[i]);
        VSIFree(panHistogram);
    }

    Rcpp::List list_out = Rcpp::List::create(
            Rcpp::Named("min") = min,
            Rcpp::Named("max") = max,
            Rcpp::Named("num_buckets") = num_buckets,
            Rcpp::Named("histogram") = hist);

    return list_out;
}

Rcpp::CharacterVector GDALRaster::getMetadata(int band,
                                              const std::string &domain) const {

    checkAccess_(GA_ReadOnly);

    char **papszMD = nullptr;

    if (band == 0) {
        papszMD = GDALGetMetadata(m_hDataset,
                                  domain == "" ? nullptr : domain.c_str());
    }
    else {
        GDALRasterBandH hBand = getBand_(band);
        papszMD = GDALGetMetadata(hBand,
                                  domain == "" ? nullptr : domain.c_str());
    }

    int nItems = CSLCount(papszMD);
    if (nItems > 0) {
        Rcpp::CharacterVector md(nItems);
        for (int i = 0; i < nItems; ++i) {
            md(i) = papszMD[i];
        }
        return md;
    }
    else {
        return "";
    }
}

bool GDALRaster::setMetadata(int band, const Rcpp::CharacterVector &metadata,
                             const std::string &domain) {

    checkAccess_(GA_ReadOnly);

    const char* domain_in = nullptr;
    if (domain != "")
        domain_in = domain.c_str();

    std::vector<const char *> metadata_in(metadata.size() + 1);
    if (metadata.size() > 0) {
        for (R_xlen_t i = 0; i < metadata.size(); ++i) {
            metadata_in[i] = (const char *) (metadata[i]);
        }
    }
    metadata_in[metadata.size()] = nullptr;

    CPLErr err = CE_None;
    if (band == 0) {
        err = GDALSetMetadata(m_hDataset, metadata_in.data(), domain_in);
    }
    else {
        GDALRasterBandH hBand = getBand_(band);
        err = GDALSetMetadata(hBand, metadata_in.data(), domain_in);
    }

    if (err != CE_None) {
        return false;
    }
    else {
        return true;
    }
}

std::string GDALRaster::getMetadataItem(int band, const std::string &mdi_name,
                                        const std::string &domain) const {

    checkAccess_(GA_ReadOnly);

    const char* domain_in = nullptr;
    if (domain != "")
        domain_in = domain.c_str();

    std::string mdi = "";

    if (band == 0) {
        if (GDALGetMetadataItem(m_hDataset, mdi_name.c_str(), domain_in)
                != nullptr) {
            mdi += std::string(GDALGetMetadataItem(m_hDataset, mdi_name.c_str(),
                                                   domain_in));
        }
    }
    else {
        GDALRasterBandH hBand = getBand_(band);
        if (GDALGetMetadataItem(hBand, mdi_name.c_str(), domain_in) != nullptr)
            mdi += std::string(GDALGetMetadataItem(hBand, mdi_name.c_str(),
                                                   domain_in));
    }

    return mdi;
}

bool GDALRaster::setMetadataItem(int band, const std::string &mdi_name,
                                 const std::string &mdi_value,
                                 const std::string &domain) {

    checkAccess_(GA_ReadOnly);

    const char* domain_in = nullptr;
    if (domain != "")
        domain_in = domain.c_str();

    CPLErr err = CE_None;
    if (band == 0) {
        err = GDALSetMetadataItem(m_hDataset, mdi_name.c_str(),
                                  mdi_value.c_str(), domain_in);
    }
    else {
        GDALRasterBandH hBand = getBand_(band);
        err = GDALSetMetadataItem(hBand, mdi_name.c_str(), mdi_value.c_str(),
                                  domain_in);
    }

    if (err != CE_None) {
        return false;
    }
    else {
        return true;
    }
}

Rcpp::CharacterVector GDALRaster::getMetadataDomainList(int band) const {
    checkAccess_(GA_ReadOnly);

    char **papszMD = nullptr;

    if (band == 0) {
        papszMD = GDALGetMetadataDomainList(m_hDataset);
    }
    else {
        GDALRasterBandH hBand = getBand_(band);
        papszMD = GDALGetMetadataDomainList(hBand);
    }

    int nItems = CSLCount(papszMD);
    if (nItems > 0) {
        Rcpp::CharacterVector md(nItems);
        for (int i = 0; i < nItems; ++i) {
            md(i) = papszMD[i];
        }
        CSLDestroy(papszMD);
        return md;
    }
    else {
        CSLDestroy(papszMD);
        return "";
    }
}

SEXP GDALRaster::read(int band, int xoff, int yoff, int xsize, int ysize,
                      int out_xsize, int out_ysize) const {

    if (!isOpen())
        Rcpp::stop("dataset is not open");

    if (out_xsize < 1 || out_ysize < 1)
        Rcpp::stop("'out_xsize' and 'out_ysize' must be > 0");

    GDALRasterBandH hBand = nullptr;
    hBand = GDALGetRasterBand(m_hDataset, band);
    if (hBand == nullptr)
        Rcpp::stop("failed to access the requested band");

    const GDALDataType eDT = GDALGetRasterDataType(hBand);

    const R_xlen_t buf_size = static_cast<R_xlen_t>(out_xsize) * out_ysize;

    int nHasNoData = 0;
    const double dfNoDataValue = GDALGetRasterNoDataValue(hBand, &nHasNoData);
    const bool has_nodata_value = CPL_TO_BOOL(nHasNoData);

    CPLErr err = CE_None;

    if (!CPL_TO_BOOL(GDALDataTypeIsComplex(eDT))) {
        // real data types read as either GDT_Byte, GDT_Int32 or GDT_Float64
        if (CPL_TO_BOOL(GDALDataTypeIsInteger(eDT)) &&
                (GDALGetDataTypeSizeBits(eDT) <= 16 ||
                 (GDALGetDataTypeSizeBits(eDT) <= 32 &&
                  CPL_TO_BOOL(GDALDataTypeIsSigned(eDT))))) {

            // if signed integer <= 32 bits or any integer <= 16 bits:
            // use int32 buffer, unless we are reading Byte as R raw type
            if (this->readByteAsRaw && eDT == GDT_Byte) {
                Rcpp::RawVector v = Rcpp::no_init(buf_size);

                err = GDALRasterIO(hBand, GF_Read, xoff, yoff, xsize, ysize,
                                   v.begin(), out_xsize, out_ysize, GDT_Byte,
                                   0, 0);

                if (err == CE_Failure)
                    Rcpp::stop("read raster failed");

                return v;
            }
            else {
                Rcpp::IntegerVector v = Rcpp::no_init(buf_size);

                err = GDALRasterIO(hBand, GF_Read, xoff, yoff, xsize, ysize,
                                   v.begin(), out_xsize, out_ysize, GDT_Int32,
                                   0, 0);

                if (err == CE_Failure)
                    Rcpp::stop("read raster failed");

                if (has_nodata_value) {
                    const int nNoDataValue = static_cast<int>(dfNoDataValue);
                    std::replace(v.begin(), v.end(), nNoDataValue, NA_INTEGER);
                }

                return v;
            }
        }
        else {
            // UInt32, Float32, Float64
            // use double buffer
            // (Int64, UInt64 would currently be handled here but would lose
            //  precision when > 9,007,199,254,740,992 (2^53). Support for
            //  Int64/UInt64 raster could potentially be added using {bit64}.)
            Rcpp::NumericVector v = Rcpp::no_init(buf_size);

            err = GDALRasterIO(hBand, GF_Read, xoff, yoff, xsize, ysize,
                               v.begin(), out_xsize, out_ysize, GDT_Float64,
                               0, 0);

            if (err == CE_Failure)
                Rcpp::stop("read raster failed");

            if (has_nodata_value && !std::isnan(dfNoDataValue)) {
                if (CPL_TO_BOOL(GDALDataTypeIsFloating(eDT))) {
                    for (double &val : v) {
                        if (std::isnan(val))
                            val = NA_REAL;
                        else if (ARE_REAL_EQUAL(val, dfNoDataValue))
                            val = NA_REAL;
                    }
                }
                else {
                    std::replace(v.begin(), v.end(), dfNoDataValue, NA_REAL);
                }
            }
            else if (CPL_TO_BOOL(GDALDataTypeIsFloating(eDT))) {
                for (double &val : v) {
                    if (std::isnan(val))
                        val = NA_REAL;
                }
            }

            return v;
        }
    }
    else {
        // complex data types read as GDT_CFloat64
        Rcpp::ComplexVector v = Rcpp::no_init(buf_size);

        err = GDALRasterIO(hBand, GF_Read, xoff, yoff, xsize, ysize, v.begin(),
                           out_xsize, out_ysize, GDT_CFloat64, 0, 0);

        if (err == CE_Failure)
            Rcpp::stop("read raster failed");

        return v;
    }
}

SEXP GDALRaster::readBlock(int band, int xblockoff, int yblockoff) const {

    if (!isOpen())
        Rcpp::stop("dataset is not open");

    if (xblockoff < 0 || yblockoff < 0)
        Rcpp::stop("'xblockoff' and 'yblockoff' must be >= 0");

    GDALRasterBandH hBand = nullptr;
    hBand = GDALGetRasterBand(m_hDataset, band);
    if (hBand == nullptr)
        Rcpp::stop("failed to access the requested band");

    int nBlockXSize = 0;
    int nBlockYSize = 0;
    GDALGetBlockSize(hBand, &nBlockXSize, &nBlockYSize);
    if (nBlockXSize < 1 || nBlockYSize < 0) {
        Rcpp::stop("invalid block size reported for this band");
    }

    const int nXOff = xblockoff * nBlockXSize;
    const int nYOff = yblockoff * nBlockYSize;

    int nOutXSize = 0;
    int nOutYSize = 0;
    if (GDALGetActualBlockSize(
        hBand, xblockoff, yblockoff, &nOutXSize, &nOutYSize) != CE_None) {

        Rcpp::stop("GDALGetActualBlockSize() failed");
    }

    // use GDALRasterIO() instead of GDALReadBlock() since the latter would
    // still require a data type conversion to a type that can be returned to R
    // (i.e., INTSXP or REALSXP, generally incurring a copy)
    return read(band, nXOff, nYOff, nOutXSize, nOutYSize, nOutXSize, nOutYSize);
}

SEXP GDALRaster::readChunk(int band,
                           const Rcpp::IntegerVector &chunk_def) const {
    // helper method intended to be used with the output of make_chunk_index()
    // 'chunk_def' is expected to be a numeric vector containing:
    //   xchunkoff, ychunkoff, xoff, yoff, xsize, ysize, xmin, xmax, ymin, ymax
    // This is a row of the matrix returned by make_chunk_index(). The vector
    // is implicitly cast to integer by Rcpp, and only the first <= 6 columns
    // are referenced. Only xoff, yoff, xsize and ysize are needed here, but the
    // input vector is expected to be as above for the first six values.
    // Also accepts length-4 vector of: xoff, yoff, xsize, ysize.
    if (!isOpen())
        Rcpp::stop("dataset is not open");

    if (chunk_def.size() == 0)
        Rcpp::stop("'chunk_def' is empty");

    R_xlen_t adj_for_chunk_x_y = 0;
    if (chunk_def.size() == 4) {
        adj_for_chunk_x_y = 2;
    }
    else if (chunk_def.size() < 6) {
        Rcpp::stop("'chunk_def' must have length >= 6 (or 4 "
                   "with xoff, yoff, xsize, ysize)");
    }

    return read(
        band,
        chunk_def[2 - adj_for_chunk_x_y],
        chunk_def[3 - adj_for_chunk_x_y],
        chunk_def[4 - adj_for_chunk_x_y],
        chunk_def[5 - adj_for_chunk_x_y],
        chunk_def[4 - adj_for_chunk_x_y],
        chunk_def[5 - adj_for_chunk_x_y]);
}

void GDALRaster::write(int band, int xoff, int yoff, int xsize, int ysize,
                       const Rcpp::RObject &rasterData) {

    checkAccess_(GA_Update);

    if (xsize < 1 || ysize < 1)
        Rcpp::stop("'xsize' and 'ysize' must be > 0");

    if (rasterData.isNULL())
        Rcpp::stop("'rasterData' is NULL");

    CPLErr err = CE_None;

    GDALRasterBandH hBand = nullptr;
    hBand = GDALGetRasterBand(m_hDataset, band);
    if (hBand == nullptr)
        Rcpp::stop("failed to access the requested band");

    if (Rcpp::is<Rcpp::NumericVector>(rasterData)) {
        GDALDataType eBufType = GDT_Float64;
        Rcpp::NumericVector buf(rasterData);
        if (buf.size() != static_cast<R_xlen_t>(xsize) * ysize)
            Rcpp::stop("size of input data is not the same as region size");

        err = GDALRasterIO(hBand, GF_Write, xoff, yoff, xsize, ysize,
                           buf.begin(), xsize, ysize, eBufType, 0, 0);
    }
    else if (Rcpp::is<Rcpp::IntegerVector>(rasterData) ||
             Rcpp::is<Rcpp::LogicalVector>(rasterData)) {

        GDALDataType eBufType = GDT_Int32;
        Rcpp::IntegerVector buf(rasterData);
        if (buf.size() != static_cast<R_xlen_t>(xsize) * ysize)
            Rcpp::stop("size of input data is not the same as region size");

        err = GDALRasterIO(hBand, GF_Write, xoff, yoff, xsize, ysize,
                           buf.begin(), xsize, ysize, eBufType, 0, 0);
    }
    else if (Rcpp::is<Rcpp::RawVector>(rasterData)) {
        GDALDataType eBufType = GDT_Byte;
        Rcpp::RawVector buf(rasterData);
        if (buf.size() != static_cast<R_xlen_t>(xsize) * ysize)
            Rcpp::stop("size of input data is not the same as region size");

        err = GDALRasterIO(hBand, GF_Write, xoff, yoff, xsize, ysize,
                           buf.begin(), xsize, ysize, eBufType, 0, 0);
    }
    else if (Rcpp::is<Rcpp::ComplexVector>(rasterData)) {
        GDALDataType eBufType = GDT_CFloat64;
        Rcpp::ComplexVector buf(rasterData);
        if (buf.size() != static_cast<R_xlen_t>(xsize) * ysize)
            Rcpp::stop("size of input data is not the same as region size");

        err = GDALRasterIO(hBand, GF_Write, xoff, yoff, xsize, ysize,
                           buf.begin(), xsize, ysize, eBufType, 0, 0);
    }
    else {
        Rcpp::stop("'rasterData' must be a vector of type numeric, integer, "
                   "raw or complex");
    }

    if (err == CE_Failure) {
        Rcpp::stop("write to raster failed");
    }
}

void GDALRaster::writeBlock(int band, int xblockoff, int yblockoff,
                            const Rcpp::RObject &rasterData) {

    if (!isOpen())
        Rcpp::stop("dataset is not open");

    if (xblockoff < 0 || yblockoff < 0)
        Rcpp::stop("'xblockoff' and 'yblockoff' must be >= 0");

    GDALRasterBandH hBand = nullptr;
    hBand = GDALGetRasterBand(m_hDataset, band);
    if (hBand == nullptr)
        Rcpp::stop("failed to access the requested band");

    int nBlockXSize = 0;
    int nBlockYSize = 0;
    GDALGetBlockSize(hBand, &nBlockXSize, &nBlockYSize);
    if (nBlockXSize < 1 || nBlockYSize < 0) {
        Rcpp::stop("invalid block size reported for this band");
    }

    const int nXOff = xblockoff * nBlockXSize;
    const int nYOff = yblockoff * nBlockYSize;

    int nOutXSize = 0;
    int nOutYSize = 0;
    if (GDALGetActualBlockSize(
        hBand, xblockoff, yblockoff, &nOutXSize, &nOutYSize) != CE_None) {

        Rcpp::stop("GDALGetActualBlockSize() failed");
    }

    return write(band, nXOff, nYOff, nOutXSize, nOutYSize, rasterData);
}

void GDALRaster::writeChunk(int band, const Rcpp::IntegerVector &chunk_def,
                            const Rcpp::RObject &rasterData) {
    // see comments above for readChunk()

    if (!isOpen())
        Rcpp::stop("dataset is not open");

    if (chunk_def.size() == 0)
        Rcpp::stop("'chunk_def' is empty");

    R_xlen_t adj_for_chunk_x_y = 0;
    if (chunk_def.size() == 4) {
        adj_for_chunk_x_y = 2;
    }
    else if (chunk_def.size() < 6) {
        Rcpp::stop("'chunk_def' must have length >= 6 (or 4 "
                   "with xoff, yoff, xsize, ysize)");
    }

    return write(
        band,
        chunk_def[2 - adj_for_chunk_x_y],
        chunk_def[3 - adj_for_chunk_x_y],
        chunk_def[4 - adj_for_chunk_x_y],
        chunk_def[5 - adj_for_chunk_x_y],
        rasterData);
}

void GDALRaster::fillRaster(int band, double value, double ivalue) {
    checkAccess_(GA_Update);

    GDALRasterBandH hBand = getBand_(band);
    if (GDALFillRaster(hBand, value, ivalue) == CE_Failure) {
        Rcpp::stop("fill raster failed");
    }
}

SEXP GDALRaster::getColorTable(int band) const {
    checkAccess_(GA_ReadOnly);

    GDALRasterBandH hBand = getBand_(band);
    GDALColorTableH hColTbl = GDALGetRasterColorTable(hBand);
    if (hColTbl == nullptr)
        return R_NilValue;

    const int nEntries = GDALGetColorEntryCount(hColTbl);
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

    for (int i = 0; i < nEntries; ++i) {
        const GDALColorEntry *colEntry = GDALGetColorEntry(hColTbl, i);
        col_tbl(i, 0) = i;
        col_tbl(i, 1) = colEntry->c1;
        col_tbl(i, 2) = colEntry->c2;
        col_tbl(i, 3) = colEntry->c3;
        col_tbl(i, 4) = colEntry->c4;
    }

    return col_tbl;
}

std::string GDALRaster::getPaletteInterp(int band) const {
    checkAccess_(GA_ReadOnly);

    GDALRasterBandH hBand = getBand_(band);
    GDALColorTableH hColTbl = GDALGetRasterColorTable(hBand);
    if (hColTbl == nullptr)
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

bool GDALRaster::setColorTable(int band, const Rcpp::RObject &col_tbl,
                               const std::string &palette_interp) {

    checkAccess_(GA_ReadOnly);

    GDALRasterBandH hBand = getBand_(band);

    Rcpp::IntegerMatrix m_col_tbl = {};
    if (Rcpp::is<Rcpp::DataFrame>(col_tbl)) {
        m_col_tbl = df_to_int_matrix_(col_tbl);
    }
    else if (Rcpp::is<Rcpp::NumericVector>(col_tbl) ||
             Rcpp::is<Rcpp::IntegerVector>(col_tbl)) {

        if (Rf_isMatrix(col_tbl))
            m_col_tbl = Rcpp::as<Rcpp::IntegerMatrix>(col_tbl);
    }
    else {
        Rcpp::stop("'col_tbl' must be a data frame or matrix");
    }

    if (m_col_tbl.ncol() < 4 || m_col_tbl.ncol() > 5)
        Rcpp::stop("'col_tbl' must have four or five columns");
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
        Rcpp::stop("invalid 'palette_interp'");

    int max_value = Rcpp::max(m_col_tbl.column(0));
    GDALColorTableH hColTbl = nullptr;
    hColTbl = GDALCreateColorTable(gpi);
    if (!hColTbl)
        Rcpp::stop("failed to create color table object");

    // initialize all entries
    const GDALColorEntry col = {0, 0, 0, 0};
    for (int i = 0; i <= max_value; ++i) {
        GDALSetColorEntry(hColTbl, i, &col);
    }

    // set entries from input table
    for (int i = 0; i < m_col_tbl.nrow(); ++i) {
        if (m_col_tbl(i, 0) >= 0) {
            const GDALColorEntry col = {
                static_cast<short>(m_col_tbl(i, 1)),
                static_cast<short>(m_col_tbl(i, 2)),
                static_cast<short>(m_col_tbl(i, 3)),
                static_cast<short>(m_col_tbl(i, 4)) };
            GDALSetColorEntry(hColTbl, m_col_tbl(i, 0), &col);
        }
        else {
            Rcpp::warning("skipped entry with negative value");
        }
    }

    CPLErr err = GDALSetRasterColorTable(hBand, hColTbl);
    GDALDestroyColorTable(hColTbl);
    if (err == CE_Failure) {
        if (!quiet)
            Rcpp::Rcout << "failed to set color table\n";
        return false;
    }
    else {
        return true;
    }
}

bool GDALRaster::clearColorTable(int band) {

    checkAccess_(GA_Update);

    GDALRasterBandH hBand = getBand_(band);
    CPLErr err = GDALSetRasterColorTable(hBand, nullptr);
    if (err == CE_Failure) {
        return false;
    }
    else {
        return true;
    }
}

SEXP GDALRaster::getDefaultRAT(int band) const {
    checkAccess_(GA_ReadOnly);

    GDALRasterBandH hBand = getBand_(band);
    GDALRasterAttributeTableH hRAT = nullptr;
    hRAT = GDALGetDefaultRAT(hBand);
    if (!hRAT)
        return R_NilValue;

    CPLErr err = CE_None;
    const int nCol = GDALRATGetColumnCount(hRAT);
    const int nRow = GDALRATGetRowCount(hRAT);
    Rcpp::List df(nCol);
    Rcpp::CharacterVector col_names(nCol);

    GDALProgressFunc pfnProgress = nullptr;
    if (!quiet)
        pfnProgress = GDALTermProgressR;
    void *pProgressData = nullptr;

    for (int i = 0; i < nCol; ++i) {
        std::string colName(GDALRATGetNameOfCol(hRAT, i));
        GDALRATFieldType gft = GDALRATGetTypeOfCol(hRAT, i);
        GDALRATFieldUsage gfu = GDALRATGetUsageOfCol(hRAT, i);
        if (gft == GFT_Integer) {
            std::vector<int> int_values(nRow);
            err = GDALRATValuesIOAsInteger(hRAT, GF_Read, i, 0, nRow,
                                           int_values.data());
            if (err == CE_Failure)
                Rcpp::stop("read column failed");
            Rcpp::IntegerVector v = Rcpp::wrap(int_values);
            v.attr("GFU") = getGFU_string_(gfu);
            df[i] = v;
            col_names[i] = colName;
        }
        else if (gft == GFT_Real) {
            std::vector<double> dbl_values(nRow);
            err = GDALRATValuesIOAsDouble(hRAT, GF_Read, i, 0, nRow,
                                          dbl_values.data());
            if (err == CE_Failure)
                Rcpp::stop("read column failed");
            Rcpp::NumericVector v = Rcpp::wrap(dbl_values);
            v.attr("GFU") = getGFU_string_(gfu);
            df[i] = v;
            col_names[i] = colName;
        }
        else if (gft == GFT_String) {
            char **papszStringList = reinterpret_cast<char**>(
                CPLCalloc(sizeof(char*), nRow));

            err = GDALRATValuesIOAsString(hRAT, GF_Read, i, 0, nRow,
                                          papszStringList);

            if (err == CE_Failure) {
                CPLFree(papszStringList);
                Rcpp::stop("read column failed");
            }
            std::vector<std::string> str_values(nRow);
            for (int n = 0; n < nRow; ++n) {
                str_values[n] = papszStringList[n];
            }
            Rcpp::CharacterVector v = Rcpp::wrap(str_values);
            v.attr("GFU") = getGFU_string_(gfu);
            df[i] = v;
            col_names[i] = colName;
            // free the list of strings
            for (int n = 0; n < nRow; ++n) {
                CPLFree(papszStringList[n]);
            }
            CPLFree(papszStringList);
        }
        else {
            Rcpp::warning("unhandled GDAL field type");
        }

        if (!quiet) {
            pfnProgress(i / (nCol-1.0), nullptr, pProgressData);
        }
    }

    df.names() = col_names;
    df.attr("class") = "data.frame";
    df.attr("row.names") = Rcpp::seq_len(nRow);

    GDALRATTableType grtt = GDALRATGetTableType(hRAT);
    if (grtt == GRTT_ATHEMATIC)
        df.attr("GDALRATTableType") = "athematic";
    else if (grtt == GRTT_THEMATIC)
        df.attr("GDALRATTableType") = "thematic";

    // check for linear binning information
    // lower bound (pixel value) of the first category:
    double dfRow0Min = NA_REAL;
    // width of each category (in pixel value units):
    double dfBinSize = NA_REAL;
    if (GDALRATGetLinearBinning(hRAT, &dfRow0Min, &dfBinSize)) {
        df.attr("Row0Min") = dfRow0Min;
        df.attr("BinSize") = dfBinSize;
    }

    return df;
}

bool GDALRaster::setDefaultRAT(int band, const Rcpp::DataFrame &df) {
    checkAccess_(GA_ReadOnly);

    GDALRasterBandH hBand = getBand_(band);

    const int nRow = static_cast<int>(df.nrows());
    const int nCol = static_cast<int>(df.size());
    int nCol_added = 0;
    Rcpp::CharacterVector colNames = df.names();
    CPLErr err = CE_None;

    GDALRasterAttributeTableH hRAT = nullptr;
    hRAT = GDALCreateRasterAttributeTable();
    if (!hRAT)
        Rcpp::stop("GDALCreateRasterAttributeTable() returned null pointer");

    GDALRATSetRowCount(hRAT, nRow);
    if (df.hasAttribute("GDALRATTableType")) {
        std::string s = Rcpp::as<std::string>(df.attr("GDALRATTableType"));
        if (s == "thematic") {
            err = GDALRATSetTableType(hRAT, GRTT_THEMATIC);
        }
        else if (s == "athematic") {
            err = GDALRATSetTableType(hRAT, GRTT_ATHEMATIC);
        }
        else {
            err = CE_Failure;
            Rcpp::warning("unrecognized table type");
        }
        if (err == CE_Failure)
            Rcpp::warning("failed to set table type");
    }
    if (df.hasAttribute("Row0Min") && df.hasAttribute("BinSize")) {
        double dfRow0Min = Rcpp::as<double>(df.attr("Row0Min"));
        double dfBinSize = Rcpp::as<double>(df.attr("BinSize"));
        err = GDALRATSetLinearBinning(hRAT, dfRow0Min, dfBinSize);
        if (err == CE_Failure)
            Rcpp::warning("failed to set linear binning information");
    }

    for (int col = 0; col < nCol; ++col) {
        if (Rf_isMatrix(df[col])) {
            Rcpp::warning("matrix column is not supported (skipping)");
            continue;
        }
        if (Rf_isFactor(df[col])) {
            Rcpp::warning("factor column is not supported (skipping)");
            continue;
        }
        if (Rcpp::is<Rcpp::IntegerVector>(df[col]) ||
            Rcpp::is<Rcpp::LogicalVector>(df[col])) {

            // add GFT_Integer column
            Rcpp::IntegerVector v = df[col];
            GDALRATFieldUsage gfu = GFU_Generic;
            if (v.hasAttribute("GFU"))
                gfu = getGFU_(v.attr("GFU"));
            Rcpp::String colName(colNames[col]);
            err = GDALRATCreateColumn(hRAT, colName.get_cstring(),
                                      GFT_Integer, gfu);
            if (err == CE_Failure) {
                Rcpp::warning("create 'integer' column failed (skipping)");
                continue;
            }
            for (int row = 0; row < nRow; ++row) {
                GDALRATSetValueAsInt(hRAT, row, col, v(row));
            }
            nCol_added += 1;
        }
        else if (Rcpp::is<Rcpp::NumericVector>(df[col])) {
            // add GFT_Real column
            Rcpp::NumericVector v = df[col];
            GDALRATFieldUsage gfu = GFU_Generic;
            if (v.hasAttribute("GFU"))
                gfu = getGFU_(v.attr("GFU"));
            Rcpp::String colName(colNames[col]);
            err = GDALRATCreateColumn(hRAT, colName.get_cstring(),
                                      GFT_Real, gfu);
            if (err == CE_Failure) {
                Rcpp::warning("create 'real' column failed (skipping)");
                continue;
            }
            for (int row = 0; row < nRow; ++row) {
                GDALRATSetValueAsDouble(hRAT, row, col, v(row));
            }
            nCol_added += 1;
        }
        else if (Rcpp::is<Rcpp::CharacterVector>(df[col])) {
            // add GFT_String column
            Rcpp::CharacterVector v = df[col];
            GDALRATFieldUsage gfu = GFU_Generic;
            if (v.hasAttribute("GFU"))
                gfu = getGFU_(v.attr("GFU"));
            Rcpp::String colName(colNames[col]);
            err = GDALRATCreateColumn(hRAT, colName.get_cstring(),
                                      GFT_String, gfu);
            if (err == CE_Failure) {
                Rcpp::warning("create 'string' column failed (skipping)");
                continue;
            }
            for (int row = 0; row < nRow; ++row) {
                Rcpp::String s(v(row));
                GDALRATSetValueAsString(hRAT, row, col, s.get_cstring());
            }
            nCol_added += 1;
        }
        else {
            Rcpp::warning("unsupported column type (skipping)");
        }
    }

    if (nCol_added > 0)
        err = GDALSetDefaultRAT(hBand, hRAT);

    GDALDestroyRasterAttributeTable(hRAT);

    if (nCol_added == 0 || err == CE_Failure) {
        if (!quiet)
            Rcpp::Rcout << "could not set raster attribute table\n";
        return false;
    }
    else {
        return true;
    }
}

void GDALRaster::flushCache() {
    if (m_hDataset != nullptr)
#if GDAL_VERSION_NUM >= 3070000
        if (GDALFlushCache(m_hDataset) != CE_None)
            Rcpp::warning("error occurred during GDALFlushCache()!");
#else
        GDALFlushCache(m_hDataset);
#endif
}

int GDALRaster::getChecksum(int band, int xoff, int yoff, int xsize,
                            int ysize) const {

    checkAccess_(GA_ReadOnly);

    GDALRasterBandH hBand = getBand_(band);
    return GDALChecksumImage(hBand, xoff, yoff, xsize, ysize);
}

void GDALRaster::close() {
    if (m_hDataset == nullptr)
        return;

#if GDAL_VERSION_NUM >= GDAL_COMPUTE_VERSION(3, 7, 0)
    // use GDALClose() on shared, and driver-less datasets such as the one
    // returned by mdim_as_classic()
    if (m_shared || !GDALGetDatasetDriver(m_hDataset)) {
        if (GDALClose(m_hDataset) != CE_None)
            Rcpp::warning("error occurred during GDALClose()!");
    }
    else {
        GDALReleaseDataset(m_hDataset);
    }
#else
    if (m_shared || !GDALGetDatasetDriver(m_hDataset))
        GDALClose(m_hDataset);
    else
        GDALReleaseDataset(m_hDataset);
#endif

    m_hDataset = nullptr;
}

void GDALRaster::show() const {
    const int xsize = static_cast<int>(getRasterXSize());
    const int ysize = static_cast<int>(getRasterYSize());

    Rcpp::Environment pkg = Rcpp::Environment::namespace_env("gdalraster");
    Rcpp::Function fn = pkg[".get_crs_name"];
    std::string crs_name = Rcpp::as<std::string>(fn(getProjection()));

    Rcpp::Rcout << "C++ object of class GDALRaster\n";
    GDALDriverH hDriver = GDALGetDatasetDriver(m_hDataset);
    if (hDriver) {
        Rcpp::Rcout << " Driver : " << getDriverLongName() << " (" <<
                                       getDriverShortName() << ")\n";
    }
    else {
        Rcpp::Rcout << " Driver :\n";
    }
    Rcpp::Rcout << " DSN    : " << getDescription(0) << "\n";
    Rcpp::Rcout << " Dim    : " << std::to_string(xsize) << ", " <<
                                   std::to_string(ysize) << ", " <<
                                   std::to_string(getRasterCount()) <<
                                   "\n";
    Rcpp::Rcout << " CRS    : " << crs_name << "\n";
    Rcpp::Rcout << " Res    : " << std::to_string(res()[0]) << ", " <<
                                   std::to_string(res()[1]) << "\n";
    Rcpp::Rcout << " Bbox   : " << std::to_string(bbox()[0]) << ", " <<
                                   std::to_string(bbox()[1]) << ", " <<
                                   std::to_string(bbox()[2]) << ", " <<
                                   std::to_string(bbox()[3]) << "\n";
}

// ****************************************************************************
// class methods for internal use not exposed in R
// ****************************************************************************

void GDALRaster::checkAccess_(GDALAccess access_needed) const {
    if (!isOpen())
        Rcpp::stop("dataset is not open");

    if (access_needed == GA_Update && m_eAccess == GA_ReadOnly)
        Rcpp::stop("dataset is read-only");
}

GDALRasterBandH GDALRaster::getBand_(int band) const {
    if (band < 1 || band > getRasterCount())
        Rcpp::stop("illegal band number");
    GDALRasterBandH hBand = nullptr;
    hBand = GDALGetRasterBand(m_hDataset, band);
    if (hBand == nullptr)
        Rcpp::stop("failed to access the requested band");
    return hBand;
}

bool GDALRaster::readableAsInt_(int band) const {
    GDALRasterBandH hBand = getBand_(band);
    GDALDataType eDT = GDALGetRasterDataType(hBand);

    // readable as int32 / R integer type
    // signed integer <= 32 bits or any integer <= 16 bits
    if (CPL_TO_BOOL(GDALDataTypeIsInteger(eDT)) &&
            (GDALGetDataTypeSizeBits(eDT) <= 16 ||
             (GDALGetDataTypeSizeBits(eDT) <= 32 &&
              CPL_TO_BOOL(GDALDataTypeIsSigned(eDT))))) {

        return true;
    }
    else {
        return false;
    }
}

bool GDALRaster::hasInt64_() const {
    bool has_int64 = false;
    for (int b = 1; b <= getRasterCount(); ++b) {
        GDALRasterBandH hBand = GDALGetRasterBand(m_hDataset, b);
        GDALDataType eDT = GDALGetRasterDataType(hBand);
        if (CPL_TO_BOOL(GDALDataTypeIsInteger(eDT)) &&
                (GDALGetDataTypeSizeBits(eDT) == 64)) {
            has_int64 = true;
            break;
        }
    }
    return has_int64;
}

void GDALRaster::warnInt64_() const {
    Rcpp::Rcout << "Int64/UInt64 raster data types are not fully supported.\n";
    Rcpp::Rcout << "Loss of precision will occur for values > 2^53.\n";
    std::string msg =
        "Int64/UInt64 raster data are currently handled as 'double'";
    Rcpp::warning(msg);
}

GDALDatasetH GDALRaster::getGDALDatasetH_() const {
    return m_hDataset;
}

void GDALRaster::setGDALDatasetH_(GDALDatasetH hDs) {
    m_hDataset = hDs;
    if (m_hDataset) {
        if (GDALGetAccess(m_hDataset) == GA_ReadOnly)
            m_eAccess = GA_ReadOnly;
        else
            m_eAccess = GA_Update;

        if (GDALDataset::FromHandle(m_hDataset)->GetShared() == TRUE)
            m_shared = true;
        else
            m_shared = false;
    }
}

// ****************************************************************************

RCPP_MODULE(mod_GDALRaster) {
    Rcpp::class_<GDALRaster>("GDALRaster")

    .constructor
        ("Default constructor, no dataset opened")
    .constructor<Rcpp::CharacterVector>
        ("Usage: new(GDALRaster, filename)")
    .constructor<Rcpp::CharacterVector, bool>
        ("Usage: new(GDALRaster, filename, read_only=[TRUE|FALSE])")
    .constructor<Rcpp::CharacterVector, bool, Rcpp::CharacterVector>
        ("Usage: new(GDALRaster, filename, read_only, open_options)")
    .constructor<Rcpp::CharacterVector, bool,
        Rcpp::Nullable<Rcpp::CharacterVector>, bool>
        ("Usage: new(GDALRaster, filename, read_only, open_options, shared)")
    .constructor<Rcpp::CharacterVector, bool,
        Rcpp::Nullable<Rcpp::CharacterVector>, bool, Rcpp::CharacterVector>
        ("Usage: new(GDALRaster, filename, read_only, open_options, shared, "
         "allowed_drivers)")

    // createCopy() object factory with 6 parameters
    .factory<const std::string&, const Rcpp::CharacterVector&,
             const GDALRaster* const&, bool,
             const Rcpp::Nullable<Rcpp::CharacterVector>&, bool>
             (createCopy)
    // create() object factory with 7 parameters
    .factory<const std::string&, const Rcpp::CharacterVector&,
             int, int, int, const std::string&,
             const Rcpp::Nullable<Rcpp::CharacterVector>&>
             (create)
    // autoCreateWarpedVRT() object factory with 8 parameters
    .factory<const GDALRaster* const&, const std::string&, const std::string&,
             const std::string&, double, bool, bool, bool>
             (autoCreateWarpedVRT)
    // mdim_as_classic() object factory with 9 parameters
    .factory<const Rcpp::CharacterVector&, const std::string&, int, int, bool,
             const std::string&, const std::string&,
             const Rcpp::Nullable<Rcpp::CharacterVector>&,
             const Rcpp::Nullable<Rcpp::CharacterVector>&>
             (mdim_as_classic)

    // exposed read/write fields
    .field("infoOptions", &GDALRaster::infoOptions)
    .field("quiet", &GDALRaster::quiet)
    .field("readByteAsRaw", &GDALRaster::readByteAsRaw)

    // exposed member functions
    .const_method("getFilename", &GDALRaster::getFilename,
        "Return the raster filename")
    .method("setFilename", &GDALRaster::setFilename,
        "Set the raster filename")
    .method("open", &GDALRaster::open,
        "(Re-)open the raster dataset on the existing filename")
    .const_method("isOpen", &GDALRaster::isOpen,
        "Is the raster dataset open")
    .const_method("getFileList", &GDALRaster::getFileList,
        "Fetch files forming dataset")
    .const_method("info", &GDALRaster::info,
        "Print various information about the raster dataset")
    .const_method("infoAsJSON", &GDALRaster::infoAsJSON,
        "Returns full output of gdalinfo as a JSON-formatted string")
    .const_method("getDriverShortName", &GDALRaster::getDriverShortName,
         "Return the short name of the format driver")
    .const_method("getDriverLongName", &GDALRaster::getDriverLongName,
        "Return the long name of the format driver")
    .const_method("getRasterXSize", &GDALRaster::getRasterXSize,
        "Return raster width in pixels")
    .const_method("getRasterYSize", &GDALRaster::getRasterYSize,
        "Return raster height in pixels")
    .const_method("getGeoTransform", &GDALRaster::getGeoTransform,
        "Return the affine transformation coefficients")
    .method("setGeoTransform", &GDALRaster::setGeoTransform,
        "Set the affine transformation coefficients for this dataset")
    .const_method("getRasterCount", &GDALRaster::getRasterCount,
        "Return the number of raster bands on this dataset")
    .method("addBand", &GDALRaster::addBand,
        "Add a new band if the underlying format supports this action")
    .const_method("getProjection", &GDALRaster::getProjection,
        "Return the projection (equivalent to getProjectionRef)")
    .const_method("getProjectionRef", &GDALRaster::getProjectionRef,
        "Return the projection definition for this dataset")
    .method("setProjection", &GDALRaster::setProjection,
        "Set the projection reference string for this dataset")
    .const_method("bbox", &GDALRaster::bbox,
        "Return the bounding box (xmin, ymin, xmax, ymax)")
    .const_method("res", &GDALRaster::res,
        "Return the resolution (pixel width, pixel height)")
    .const_method("dim", &GDALRaster::dim,
        "Return raster dimensions (xsize, ysize, number of bands)")
    .const_method("apply_geotransform", &GDALRaster::apply_geotransform,
        "Apply geotransform (raster column/row to geospatial x/y)")
    .const_method("get_pixel_line", &GDALRaster::get_pixel_line,
        "Convert geospatial coordinates to pixel/line")
    .const_method("pixel_extract", &GDALRaster::pixel_extract,
        "Extract pixel values at geospatial xy locations")
    .const_method("get_block_indexing", &GDALRaster::get_block_indexing,
        "Return a matrix of block x/y, raster x/y offset, block x/y size")
    .const_method("getBlockSize", &GDALRaster::getBlockSize,
        "Retrieve the actual block size for a given block offset")
    .const_method("getActualBlockSize", &GDALRaster::getActualBlockSize,
        "Get the natural block size of this band")
    .const_method("make_chunk_index", &GDALRaster::make_chunk_index,
        "Return a matrix of x/y offsets and sizes for potentially multi-block "
        "chunks")
    .const_method("getOverviewCount", &GDALRaster::getOverviewCount,
        "Return the number of overview layers available")
    .method("buildOverviews", &GDALRaster::buildOverviews,
        "Build raster overview(s)")
    .const_method("getDataTypeName", &GDALRaster::getDataTypeName,
        "Get name of the data type for this band")
    .const_method("getNoDataValue", &GDALRaster::getNoDataValue,
        "Return the nodata value for this band")
    .method("setNoDataValue", &GDALRaster::setNoDataValue,
        "Set the nodata value for this band")
    .method("deleteNoDataValue", &GDALRaster::deleteNoDataValue,
        "Delete the nodata value for this band")
    .const_method("getMaskFlags", &GDALRaster::getMaskFlags,
        "Return the status flags of the mask band associated with this band")
    .const_method("getMaskBand", &GDALRaster::getMaskBand,
        "Return the mask filename and band number associated with this band")
    .const_method("getUnitType", &GDALRaster::getUnitType,
        "Get name of the raster value units (e.g., m or ft)")
    .method("setUnitType", &GDALRaster::setUnitType,
        "Set name of the raster value units (e.g., m or ft)")
    .const_method("getScale", &GDALRaster::getScale,
        "Return the raster value scaling ratio")
    .method("setScale", &GDALRaster::setScale,
        "Set the raster value scaling ratio")
    .const_method("getOffset", &GDALRaster::getOffset,
        "Return the raster value offset")
    .method("setOffset", &GDALRaster::setOffset,
        "Set the raster value offset")
    .const_method("getDescription", &GDALRaster::getDescription,
        "Return object description for a raster band")
    .method("setDescription", &GDALRaster::setDescription,
        "Set object description for a raster band")
    .const_method("getRasterColorInterp", &GDALRaster::getRasterColorInterp,
        "How should this band be interpreted as color?")
    .method("setRasterColorInterp", &GDALRaster::setRasterColorInterp,
        "Set color interpretation of a band")
    .const_method("getMinMax", &GDALRaster::getMinMax,
        "Compute the min/max values for this band")
    .const_method("getMinMaxLocation", &GDALRaster::getMinMaxLocation,
        "Compute the min/max values for this band, and their location")
    .const_method("getStatistics", &GDALRaster::getStatistics,
        "Get min, max, mean and stdev for this band")
    .method("clearStatistics", &GDALRaster::clearStatistics,
        "Clear statistics")
    .const_method("getHistogram", &GDALRaster::getHistogram,
        "Compute raster histogram for this band")
    .const_method("getDefaultHistogram", &GDALRaster::getDefaultHistogram,
        "Fetch default raster histogram for this band")
    .const_method("getMetadata", &GDALRaster::getMetadata,
        "Return a list of metadata name=value for a domain")
    .method("setMetadata", &GDALRaster::setMetadata,
        "Set metadata from a list of name=value")
    .const_method("getMetadataItem", &GDALRaster::getMetadataItem,
        "Return the value of a metadata item")
    .method("setMetadataItem", &GDALRaster::setMetadataItem,
        "Set metadata item name=value in domain")
    .const_method("getMetadataDomainList", &GDALRaster::getMetadataDomainList,
        "Return list of metadata domains")
    .const_method("read", &GDALRaster::read,
        "Read a region of raster data for a band")
    .const_method("readBlock", &GDALRaster::readBlock,
        "Read a block of raster data")
    .const_method("readChunk", &GDALRaster::readChunk,
        "Read a multi-block user-defined chunk of raster data")
    .method("write", &GDALRaster::write,
        "Write a region of raster data for a band")
    .method("writeBlock", &GDALRaster::writeBlock,
        "Write a block of raster data")
    .method("writeChunk", &GDALRaster::writeChunk,
        "Write a multi-block user-defined chunk of raster data")
    .method("fillRaster", &GDALRaster::fillRaster,
        "Fill this band with a constant value")
    .const_method("getColorTable", &GDALRaster::getColorTable,
        "Return the color table associated with this band")
    .const_method("getPaletteInterp", &GDALRaster::getPaletteInterp,
        "Get the palette interpretation")
    .method("setColorTable", &GDALRaster::setColorTable,
        "Set a color table for this band")
    .method("clearColorTable", &GDALRaster::clearColorTable,
        "Clear the color table for this band")
    .const_method("getDefaultRAT", &GDALRaster::getDefaultRAT,
        "Return default Raster Attribute Table as data frame")
    .method("setDefaultRAT", &GDALRaster::setDefaultRAT,
        "Set Raster Attribute Table from data frame")
    .method("flushCache", &GDALRaster::flushCache,
        "Flush all write cached data to disk")
    .const_method("getChecksum", &GDALRaster::getChecksum,
        "Compute checksum for raster region")
    .method("close", &GDALRaster::close,
        "Close the GDAL dataset for proper cleanup")
    .const_method("show", &GDALRaster::show,
        "S4 show()")

    ;
}
