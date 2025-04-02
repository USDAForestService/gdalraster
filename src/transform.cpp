/* Functions for coordinate transformation using PROJ via GDAL headers
   Chris Toney <chris.toney at usda.gov>
   Copyright (c) 2023-2024 gdalraster authors
*/

#include "transform.h"

#include "ogr_core.h"
#include "ogr_srs_api.h"
#include "ogr_spatialref.h"

#include "gdalraster.h"
#include "srs_api.h"

//' get PROJ version
//' @noRd
// [[Rcpp::export(name = ".getPROJVersion")]]
std::vector<int> getPROJVersion() {
    int major, minor, patch;
    major = minor = patch = NA_INTEGER;
    OSRGetPROJVersion(&major, &minor, &patch);
    std::vector<int> ret = {major, minor, patch};
    return ret;
}

//' get search path(s) for PROJ resource files
//' @noRd
// [[Rcpp::export(name = ".getPROJSearchPaths")]]
Rcpp::CharacterVector getPROJSearchPaths() {
    char **papszPaths;
    papszPaths = OSRGetPROJSearchPaths();

    int items = CSLCount(papszPaths);
    if (items > 0) {
        Rcpp::CharacterVector paths(items);
        for (int i=0; i < items; ++i) {
            paths(i) = papszPaths[i];
        }
        CSLDestroy(papszPaths);
        return paths;
    }
    else {
        CSLDestroy(papszPaths);
        return "";
    }
}

//' set search path(s) for PROJ resource files
//' @noRd
// [[Rcpp::export(name = ".setPROJSearchPaths")]]
void setPROJSearchPaths(Rcpp::CharacterVector paths) {
    std::vector<char *> path_list = {nullptr};
    path_list.resize(paths.size() + 1);
    for (R_xlen_t i = 0; i < paths.size(); ++i) {
        path_list[i] = (char *) (paths[i]);
    }
    path_list[paths.size()] = nullptr;
    OSRSetPROJSearchPaths(path_list.data());
    return;
}

//' get whether PROJ networking capabilities are enabled
//' returns logical NA if GDAL < 3.4
//' @noRd
// [[Rcpp::export(name = ".getPROJEnableNetwork")]]
Rcpp::LogicalVector getPROJEnableNetwork() {
    Rcpp::LogicalVector ret = Rcpp::LogicalVector::create(NA_LOGICAL);
#if GDAL_VERSION_NUM >= 3040000
    if (getPROJVersion()[0] >= 7) {
        ret[0] = OSRGetPROJEnableNetwork();
        return ret;
    }
    else {
        ret[0] = false;
        return ret;
    }
#endif
    return ret;
}

//' enable or disable PROJ networking capabilities
//' @noRd
// [[Rcpp::export(name = ".setPROJEnableNetwork")]]
void setPROJEnableNetwork(int enabled) {
#if GDAL_VERSION_NUM >= 3040000
    if (getPROJVersion()[0] >= 7)
        OSRSetPROJEnableNetwork(enabled);
    else
        Rcpp::Rcerr << "OSRSetPROJEnableNetwork() requires PROJ 7 or later\n";
#else
    Rcpp::Rcerr << "OSRSetPROJEnableNetwork() requires GDAL 3.4 or later\n";
#endif
    return;
}

//' Inverse project geospatial x/y coordinates to longitude/latitude
//'
//' public wrapper in R/transform.R
//' @noRd
// [[Rcpp::export(name = ".inv_project")]]
Rcpp::NumericMatrix inv_project(const Rcpp::RObject &pts,
                                const std::string &srs,
                                const std::string &well_known_gcs = "") {

    Rcpp::NumericMatrix pts_in = xy_robject_to_matrix_(pts);

    if (pts_in.nrow() == 0)
        Rcpp::stop("input matrix is empty");

    if (pts_in.ncol() < 2 || pts_in.ncol() > 4)
        Rcpp::stop("input matrix must have 2, 3 or 4 columns");

    bool has_z = false;
    bool has_t = false;
    if (pts_in.ncol() == 3) {
        has_z = true;
    }
    else if (pts_in.ncol() == 4) {
        has_z = true;
        has_t = true;
    }

    std::string srs_in = srs_to_wkt(srs, false);

    OGRSpatialReference oSourceSRS{};
    OGRSpatialReference *poLongLat = nullptr;
    OGRCoordinateTransformation *poCT = nullptr;
    OGRErr err = OGRERR_NONE;

    err = oSourceSRS.importFromWkt(srs_in.c_str());
    if (err != OGRERR_NONE)
        Rcpp::stop("failed to import SRS from WKT string");

    oSourceSRS.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);

    if (well_known_gcs == "") {
        poLongLat = oSourceSRS.CloneGeogCS();
        if (poLongLat == nullptr)
            Rcpp::stop("failed to clone GCS");
    }
    else {
        poLongLat = new OGRSpatialReference();
        err = poLongLat->SetWellKnownGeogCS(well_known_gcs.c_str());
        if (err == OGRERR_FAILURE) {
            poLongLat->Release();
            Rcpp::stop("failed to set well known GCS");
        }
    }
    poLongLat->SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);

    poCT = OGRCreateCoordinateTransformation(&oSourceSRS, poLongLat);
    if (poCT == nullptr) {
        poLongLat->Release();
        Rcpp::stop("failed to create coordinate transformer");
    }

    Rcpp::NumericVector x = pts_in(Rcpp::_ , 0);
    Rcpp::NumericVector y = pts_in(Rcpp::_ , 1);
    Rcpp::NumericVector z{};
    if (has_z)
        z = pts_in(Rcpp::_ , 2);
    Rcpp::NumericVector t{};
    if (has_t)
        t = pts_in(Rcpp::_ , 3);

    Rcpp::LogicalVector na_in = Rcpp::is_na(x) | Rcpp::is_na(y);
    if (has_z)
        na_in = na_in | Rcpp::is_na(z);
    if (has_t)
        na_in = na_in | Rcpp::is_na(t);

    if (Rcpp::is_true(Rcpp::all(na_in))) {
        Rcpp::stop("all input points have one or more missing values");
    }

    std::vector<double> xbuf = Rcpp::as<std::vector<double>>(x);
    std::vector<double> ybuf = Rcpp::as<std::vector<double>>(y);
    std::vector<double> zbuf{};
    if (has_z)
        zbuf = Rcpp::as<std::vector<double>>(z);
    std::vector<double> tbuf{};
    if (has_t)
        tbuf = Rcpp::as<std::vector<double>>(t);

    std::vector<int> success(pts_in.size());

    int res = poCT->Transform(pts_in.nrow(), xbuf.data(), ybuf.data(),
                              has_z ? zbuf.data() : nullptr,
                              has_t ? tbuf.data() : nullptr,
                              success.data());

    OGRCoordinateTransformation::DestroyCT(poCT);
    poLongLat->Release();

    // behavior change at GDAL 3.11 (https://github.com/OSGeo/gdal/pull/11819)
    // if FALSE returned, we know at least one or more points failed so it's
    // probably worth checking them all at this point
    if (GDAL_VERSION_NUM >= GDAL_COMPUTE_VERSION(3, 11, 0)) {
        if (!res &&
            std::find(success.begin(), success.end(), TRUE) == success.end()) {

            Rcpp::stop("transformation failed for all points");
        }
    }

    Rcpp::NumericVector ret_x = Rcpp::wrap(xbuf);
    Rcpp::NumericVector ret_y = Rcpp::wrap(ybuf);
    Rcpp::NumericVector ret_z{};
    if (has_z)
        ret_z = Rcpp::wrap(zbuf);
    Rcpp::NumericVector ret_t{};
    if (has_t)
        ret_t = Rcpp::wrap(tbuf);
    size_t num_err = 0;
    size_t num_na = 0;
    for (R_xlen_t i = 0; i < na_in.size(); ++i) {
        if (na_in[i] == TRUE || !success[i]) {
            ret_x[i] = NA_REAL;
            ret_y[i] = NA_REAL;
            if (has_z)
                ret_z[i] = NA_REAL;
            if (has_t)
                ret_t[i] = NA_REAL;

            if (na_in[i] != TRUE && !success[i])
                num_err += 1;
            else
                num_na += 1;
        }
    }

    Rcpp::NumericMatrix ret = Rcpp::no_init(pts_in.nrow(), pts_in.ncol());
    ret.column(0) = ret_x;
    ret.column(1) = ret_y;
    if (has_z)
        ret.column(2) = ret_z;
    if (has_t)
        ret.column(3) = ret_t;

    if (num_err > 0) {
        Rcpp::warning(std::to_string(num_err) +
            " point(s) failed to transform, NA returned in that case");
    }

    if (num_na > 0) {
        Rcpp::warning(std::to_string(num_na) +
            " point(s) had missing values, NA returned in that case");
    }

    return ret;
}

//' Transform geospatial x/y coordinates
//'
//' public wrapper in R/transform.R
//' @noRd
// [[Rcpp::export(name = ".transform_xy")]]
Rcpp::NumericMatrix transform_xy(const Rcpp::RObject &pts,
                                 const std::string &srs_from,
                                 const std::string &srs_to) {

    Rcpp::NumericMatrix pts_in = xy_robject_to_matrix_(pts);

    if (pts_in.nrow() == 0)
        Rcpp::stop("input matrix is empty");

    if (pts_in.ncol() < 2 || pts_in.ncol() > 4)
        Rcpp::stop("input matrix must have 2, 3 or 4 columns");

    bool has_z = false;
    bool has_t = false;
    if (pts_in.ncol() == 3) {
        has_z = true;
    }
    else if (pts_in.ncol() == 4) {
        has_z = true;
        has_t = true;
    }

    std::string srs_from_in = srs_to_wkt(srs_from, false);
    std::string srs_to_in = srs_to_wkt(srs_to, false);

    OGRSpatialReference oSourceSRS{}, oDestSRS{};
    OGRCoordinateTransformation *poCT = nullptr;
    OGRErr err = OGRERR_NONE;

    err = oSourceSRS.importFromWkt(srs_from_in.c_str());
    if (err != OGRERR_NONE)
        Rcpp::stop("failed to import source SRS from WKT string");

    oSourceSRS.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);

    err = oDestSRS.importFromWkt(srs_to_in.c_str());
    if (err != OGRERR_NONE)
        Rcpp::stop("failed to import destination SRS from WKT string");

    oDestSRS.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);

    poCT = OGRCreateCoordinateTransformation(&oSourceSRS, &oDestSRS);
    if (poCT == nullptr)
        Rcpp::stop("failed to create coordinate transformer");

    Rcpp::NumericVector x = pts_in(Rcpp::_ , 0);
    Rcpp::NumericVector y = pts_in(Rcpp::_ , 1);
    Rcpp::NumericVector z{};
    if (has_z)
        z = pts_in(Rcpp::_ , 2);
    Rcpp::NumericVector t{};
    if (has_t)
        t = pts_in(Rcpp::_ , 3);

    Rcpp::LogicalVector na_in = Rcpp::is_na(x) | Rcpp::is_na(y);
    if (has_z)
        na_in = na_in | Rcpp::is_na(z);
    if (has_t)
        na_in = na_in | Rcpp::is_na(t);

    if (Rcpp::is_true(Rcpp::all(na_in))) {
        Rcpp::stop("all input points have one or more missing values");
    }

    std::vector<double> xbuf = Rcpp::as<std::vector<double>>(x);
    std::vector<double> ybuf = Rcpp::as<std::vector<double>>(y);
    std::vector<double> zbuf{};
    if (has_z)
        zbuf = Rcpp::as<std::vector<double>>(z);
    std::vector<double> tbuf{};
    if (has_t)
        tbuf = Rcpp::as<std::vector<double>>(t);

    std::vector<int> success(pts_in.size());

    int res = poCT->Transform(pts_in.nrow(), xbuf.data(), ybuf.data(),
                              has_z ? zbuf.data() : nullptr,
                              has_t ? tbuf.data() : nullptr,
                              success.data());

    OGRCoordinateTransformation::DestroyCT(poCT);

    // behavior change at GDAL 3.11 (https://github.com/OSGeo/gdal/pull/11819)
    // if FALSE returned, we know at least one or more points failed so it's
    // probably worth checking them all at this point
    if (GDAL_VERSION_NUM >= GDAL_COMPUTE_VERSION(3, 11, 0)) {
        if (!res &&
            std::find(success.begin(), success.end(), TRUE) == success.end()) {

            Rcpp::stop("transformation failed for all points");
        }
    }

    Rcpp::NumericVector ret_x = Rcpp::wrap(xbuf);
    Rcpp::NumericVector ret_y = Rcpp::wrap(ybuf);
    Rcpp::NumericVector ret_z{};
    if (has_z)
        ret_z = Rcpp::wrap(zbuf);
    Rcpp::NumericVector ret_t{};
    if (has_t)
        ret_t = Rcpp::wrap(tbuf);
    size_t num_err = 0;
    size_t num_na = 0;
    for (R_xlen_t i = 0; i < na_in.size(); ++i) {
        if (na_in[i] == TRUE || !success[i]) {
            ret_x[i] = NA_REAL;
            ret_y[i] = NA_REAL;
            if (has_z)
                ret_z[i] = NA_REAL;
            if (has_t)
                ret_t[i] = NA_REAL;

            if (na_in[i] != TRUE && !success[i])
                num_err += 1;
            else
                num_na += 1;
        }
    }

    Rcpp::NumericMatrix ret = Rcpp::no_init(pts_in.nrow(), pts_in.ncol());
    ret.column(0) = ret_x;
    ret.column(1) = ret_y;
    if (has_z)
        ret.column(2) = ret_z;
    if (has_t)
        ret.column(3) = ret_t;

    if (num_err > 0) {
        Rcpp::warning(std::to_string(num_err) +
            " point(s) failed to transform, NA returned in that case");
    }

    if (num_na > 0) {
        Rcpp::warning(std::to_string(num_na) +
            " point(s) had missing values, NA returned in that case");
    }

    return ret;
}

//' Transform boundary
//'
//' `transform_bounds()` transforms a bounding box, densifying the edges to
//' account for nonlinear transformations along these edges and extracting
//' the outermost bounds. Wrapper of `OCTTransformBounds()` in the GDAL Spatial
//' Reference System API. Requires GDAL >= 3.4.
//'
//' @details
//' The following refer to the *output* values `xmin`, `ymin`, `xmax`, `ymax`:
//'
//' If the destination CRS is geographic, the first axis is longitude, and
//' `xmax < xmin` then the bounds crossed the antimeridian. In this scenario
//' there are two polygons, one on each side of the antimeridian. The first
//' polygon should be constructed with `(xmin, ymin, 180, ymax)` and the second
//' with `(-180, ymin, xmax, ymax)`.
//'
//' If the destination CRS is geographic, the first axis is latitude, and
//' `ymax < ymin` then the bounds crossed the antimeridian. In this scenario
//' there are two polygons, one on each side of the antimeridian. The first
//' polygon should be constructed with `(ymin, xmin, ymax, 180)` and the second
//' with `(ymin, -180, ymax, xmax)`.
//'
//' @param bbox Numeric vector of length four containing the input bounding
//' box (xmin, ymin, xmax, ymax).
//' @param srs_from Character string specifying the spatial reference system
//' for `pts`. May be in WKT format or any of the formats supported by
//' [srs_to_wkt()].
//' @param srs_to Character string specifying the output spatial reference
//' system. May be in WKT format or any of the formats supported by
//' [srs_to_wkt()].
//' @param densify_pts Integer value giving the number of points to use to
//' densify the bounding polygon in the transformation. Recommended to use `21`
//' (the default).
//' @param traditional_gis_order Logical value, `TRUE` to use traditional GIS
//' order of axis mapping (the default) or `FALSE` to use authority compliant
//' axis order (see Note).
//' @returns Numeric vector of length four containing the bounding box in the
//' output spatial reference system (xmin, ymin, xmax, ymax).
//'
//' @seealso
//' [srs_to_wkt()]
//'
//' @note
//' `traditional_gis_order = TRUE` (the default) means that for geographic CRS
//' with lat/long order, the data will still be long/lat ordered. Similarly for
//' a projected CRS with northing/easting order, the data will still be
//' easting/northing ordered (GDAL's OAMS_TRADITIONAL_GIS_ORDER).
//'
//' `traditional_gis_order = FALSE` means that the data axis will be identical
//'  to the CRS axis (GDAL's OAMS_AUTHORITY_COMPLIANT).
//'
//' See
//' \url{https://gdal.org/en/stable/tutorials/osr_api_tut.html#crs-and-axis-order}.
//'
//' @examples
//' bb <- c(-1405880.71737, -1371213.76254, 5405880.71737, 5371213.76254)
//'
//' # traditional GIS axis ordering by  default (lon, lat)
//' transform_bounds(bb, "EPSG:32761", "EPSG:4326")
//'
//' # authority compliant axis ordering
//' transform_bounds(bb, "EPSG:32761", "EPSG:4326",
//'                  traditional_gis_order = FALSE)
// [[Rcpp::export]]
Rcpp::NumericVector transform_bounds(const Rcpp::NumericVector &bbox,
                                     const std::string &srs_from,
                                     const std::string &srs_to,
                                     int densify_pts = 21,
                                     bool traditional_gis_order = true) {

    if (GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 4, 0))
        Rcpp::stop("transform_bounds() requires GDAL >= 3.4");

    if (bbox.size() != 4)
        Rcpp::stop("'bbox' must be a numeric vector of length 4");

    if (Rcpp::any(Rcpp::is_na(bbox))) {
        Rcpp::warning("'bbox' has one or more 'NA' values");
        Rcpp::NumericVector::create(NA_REAL, NA_REAL, NA_REAL, NA_REAL);
    }

    std::string srs_from_in = srs_to_wkt(srs_from, false);
    std::string srs_to_in = srs_to_wkt(srs_to, false);

    OGRSpatialReferenceH hSRS_from = OSRNewSpatialReference(nullptr);
    OGRSpatialReferenceH hSRS_to = OSRNewSpatialReference(nullptr);

    char *pszWKT1 = (char*) srs_from_in.c_str();
    if (OSRImportFromWkt(hSRS_from, &pszWKT1) != OGRERR_NONE) {
        if (hSRS_from != nullptr)
            OSRDestroySpatialReference(hSRS_from);
        if (hSRS_to != nullptr)
            OSRDestroySpatialReference(hSRS_to);
        Rcpp::stop("error importing SRS from user input");
    }

    char *pszWKT2 = (char*) srs_to_in.c_str();
    if (OSRImportFromWkt(hSRS_to, &pszWKT2) != OGRERR_NONE) {
        if (hSRS_from != nullptr)
            OSRDestroySpatialReference(hSRS_from);
        if (hSRS_to != nullptr)
            OSRDestroySpatialReference(hSRS_to);
        Rcpp::stop("error importing SRS from user input");
    }

    std::string save_opt =
            get_config_option("OGR_CT_FORCE_TRADITIONAL_GIS_ORDER");

    if (traditional_gis_order) {
        OSRSetAxisMappingStrategy(hSRS_from, OAMS_TRADITIONAL_GIS_ORDER);
        OSRSetAxisMappingStrategy(hSRS_to, OAMS_TRADITIONAL_GIS_ORDER);
    }
    else {
        set_config_option("OGR_CT_FORCE_TRADITIONAL_GIS_ORDER", "NO");
        OSRSetAxisMappingStrategy(hSRS_from, OAMS_AUTHORITY_COMPLIANT);
        OSRSetAxisMappingStrategy(hSRS_to, OAMS_AUTHORITY_COMPLIANT);
    }

    OGRCoordinateTransformationH hCT = nullptr;
    hCT = OCTNewCoordinateTransformation(hSRS_from, hSRS_to);
    if (hCT == nullptr) {
        if (hSRS_from != nullptr)
            OSRDestroySpatialReference(hSRS_from);
        if (hSRS_to != nullptr)
            OSRDestroySpatialReference(hSRS_to);
        Rcpp::stop("failed to create coordinate transformer");
    }

    double out_xmin, out_ymin, out_xmax, out_ymax;
    out_xmin = out_ymin = out_xmax = out_ymax = NA_REAL;

    int res = OCTTransformBounds(hCT, bbox[0], bbox[1], bbox[2], bbox[3],
                                 &out_xmin, &out_ymin, &out_xmax, &out_ymax,
                                 densify_pts);

    OCTDestroyCoordinateTransformation(hCT);
    OSRDestroySpatialReference(hSRS_from);
    OSRDestroySpatialReference(hSRS_to);

    set_config_option("OGR_CT_FORCE_TRADITIONAL_GIS_ORDER", save_opt);

    if (!res)
        Rcpp::stop("error returned by OCTTransformBounds()");

    return Rcpp::NumericVector::create(out_xmin, out_ymin, out_xmax, out_ymax);
}
