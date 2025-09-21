/* GDAL Multidimensional Raster support
   Chris Toney <jctoney at gmail.com>
   Copyright (c) 2023-2025 gdalraster authors
*/

#include <Rcpp.h>

#include <gdal.h>

#include <string>
#include <vector>

#include "gdalraster.h"


// Return a view of an MDArray as a "classic" GDALDataset (i.e., 2D)
//
// GDALMDArrayAsClassicDataset()
// Returns a 1-element list carrying a GDALRaster object, or R_NilValue on error
// Public wrapper in R/gdal_mdim.R
//' @noRd
// [[Rcpp::export(name = ".mdim_as_classic")]]
SEXP mdim_as_classic(
    const Rcpp::CharacterVector &filename, const std::string &array_name,
    int idx_xdim, int idx_ydim, bool read_only = true,
    const std::string &group_name = "",
    const Rcpp::Nullable<Rcpp::CharacterVector> &allowed_drivers = R_NilValue,
    const Rcpp::Nullable<Rcpp::CharacterVector> &open_options = R_NilValue) {

// requires GDAL >= 3.2 for GDALGroupOpenGroupFromFullname()
#if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 2, 0)
    Rcpp::stop("mdim_as_classic() requires GDAL >= 3.2");
#else

    std::string fname_in = Rcpp::as<std::string>(check_gdal_filename(filename));

    if (idx_xdim < 0) {
        Rcpp::Rcout << "'idx_xdim' must be a positive integer\n";
        return R_NilValue;
    }
    if (idx_ydim < 0) {
        Rcpp::Rcout << "'idx_ydim' must be a positive integer\n";
        return R_NilValue;
    }

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

    if (!hDS) {
        Rcpp::Rcout << "failed to open multidim raster dataset\n";
        return R_NilValue;
    }

    GDALGroupH hRootGroup = nullptr;
    hRootGroup = GDALDatasetGetRootGroup(hDS);
    GDALReleaseDataset(hDS);
    if (!hRootGroup) {
        Rcpp::Rcout << "failed to get object for the root group\n";
        return R_NilValue;
    }

    GDALMDArrayH hVar = nullptr;
    if (group_name != "") {
        GDALGroupH hSubGroup = nullptr;
        hSubGroup = GDALGroupOpenGroupFromFullname(hRootGroup,
                                                   group_name.c_str(),
                                                   nullptr);
        if (!hSubGroup) {
            Rcpp::Rcout << "failed to get object for the sub-group\n";
            return R_NilValue;
        }

        hVar = GDALGroupOpenMDArray(hSubGroup, array_name.c_str(), nullptr);
        GDALGroupRelease(hSubGroup);
        GDALGroupRelease(hRootGroup);
        if (!hVar) {
            Rcpp::Rcout << "failed to get object for the MDArray\n";
            return R_NilValue;
        }
    }
    else {
        hVar = GDALGroupOpenMDArray(hRootGroup, array_name.c_str(), nullptr);
        GDALGroupRelease(hRootGroup);
        if (!hVar) {
            Rcpp::Rcout << "failed to get object for the MDArray\n";
            return R_NilValue;
        }
    }

    GDALDatasetH hClassicDS = nullptr;
    hClassicDS = GDALMDArrayAsClassicDataset(hVar,
                                             static_cast<size_t>(idx_xdim),
                                             static_cast<size_t>(idx_ydim));

    GDALMDArrayRelease(hVar);
    if (!hClassicDS) {
        Rcpp::Rcout << "failed to get MDArray as classic dataset\n";
        return R_NilValue;
    }

    GDALRaster *ds = new GDALRaster();
    ds->setGDALDatasetH_(hClassicDS, !read_only);
    const GDALRaster &ds_ref = *ds;
    Rcpp::List out = Rcpp::List::create();
    out.push_back(Rcpp::wrap(ds_ref));
    return out;
#endif
}
