/* Functions for coordinate transformation using PROJ via GDAL headers
   Chris Toney <chris.toney at usda.gov>
   Copyright (c) 2023-2024 gdalraster authors
*/

#include "transform.h"

#include "ogr_core.h"
#include "ogr_srs_api.h"
#include "ogr_spatialref.h"

#include "wkt_conv.h"

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
//' `inv_project()` transforms geospatial x/y coordinates to
//' longitude/latitude in the same geographic coordinate system used by the
//' given projected spatial reference system. The output long/lat can
//' optionally be set to a specific geographic coordinate system by specifying
//' a well known name (see Details).
//'
//' @details
//' By default, the geographic coordinate system of the projection specified
//' by `srs` will be used. If a specific geographic coordinate system is
//' desired, then `well_known_gcs` can be set to one of the values below:
//' \tabular{rl}{
//'  EPSG:n \tab where n is the code of a geographic coordinate system\cr
//'  WGS84  \tab same as EPSG:4326\cr
//'  WGS72  \tab same as EPSG:4322\cr
//'  NAD83  \tab same as EPSG:4269\cr
//'  NAD27  \tab same as EPSG:4267\cr
//'  CRS84  \tab same as WGS84\cr
//'  CRS72  \tab same as WGS72\cr
//'  CRS27  \tab same as NAD27
//' }
//' The returned array will always be in longitude, latitude order
//' (traditional GIS order) regardless of the axis order defined for the
//' names above.
//'
//' @param pts A two-column data frame or numeric matrix containing geospatial
//' x/y coordinates.
//' @param srs Character string specifying the projected spatial reference
//' system for `pts`. May be in WKT format or any of the formats supported by
//' [srs_to_wkt()].
//' @param well_known_gcs Optional character string containing a supported
//' well known name of a geographic coordinate system (see Details for
//' supported values).
//' @returns Numeric array of longitude, latitude. An error is raised if the
//' transformation cannot be performed.
//' @seealso
//' [transform_xy()]
//' @examples
//' pt_file <- system.file("extdata/storml_pts.csv", package="gdalraster")
//' ## id, x, y in NAD83 / UTM zone 12N
//' pts <- read.csv(pt_file)
//' print(pts)
//' inv_project(pts[,-1], "EPSG:26912")
// [[Rcpp::export]]
Rcpp::NumericMatrix inv_project(const Rcpp::RObject &pts,
                                const std::string &srs,
                                std::string well_known_gcs = "") {

    Rcpp::NumericMatrix pts_in;
    if (Rcpp::is<Rcpp::DataFrame>(pts)) {
        pts_in = df_to_matrix_(pts);
    }
    else if (Rcpp::is<Rcpp::NumericVector>(pts)) {
        if (Rf_isMatrix(pts))
            pts_in = Rcpp::as<Rcpp::NumericMatrix>(pts);
    }
    else {
        Rcpp::stop("'pts' must be a data frame or matrix");
    }

    std::string srs_in = srs_to_wkt(srs, false);

    OGRSpatialReference oSourceSRS{};
    OGRSpatialReference *poLongLat = nullptr;
    OGRCoordinateTransformation *poCT = nullptr;
    OGRErr err;

    err = oSourceSRS.importFromWkt(srs_in.c_str());
    if (err != OGRERR_NONE)
        Rcpp::stop("failed to import SRS from WKT string");

    // config option OGR_CT_FORCE_TRADITIONAL_GIS_ORDER=YES is set in gdal_init
    // this should be redundant
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
            delete poLongLat;
            Rcpp::stop("failed to set well known GCS");
        }
    }
    poLongLat->SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);

    poCT = OGRCreateCoordinateTransformation(&oSourceSRS, poLongLat);
    if (poCT == nullptr) {
        if (poLongLat != nullptr)
            poLongLat->Release();
        Rcpp::stop("failed to create coordinate transformer");
    }

    Rcpp::NumericVector x = pts_in(Rcpp::_ , 0);
    Rcpp::NumericVector y = pts_in(Rcpp::_ , 1);
    std::vector<double> xbuf = Rcpp::as<std::vector<double>>(x);
    std::vector<double> ybuf = Rcpp::as<std::vector<double>>(y);
    if (!poCT->Transform(pts_in.nrow(), xbuf.data(), ybuf.data())) {
        OGRCoordinateTransformation::DestroyCT(poCT);
        if (poLongLat != nullptr)
            poLongLat->Release();
        Rcpp::stop("coordinate transformation failed");
    }

    Rcpp::NumericMatrix ret(pts_in.nrow(), 2);
    ret.column(0) = Rcpp::as<Rcpp::NumericVector>(Rcpp::wrap(xbuf));
    ret.column(1) = Rcpp::as<Rcpp::NumericVector>(Rcpp::wrap(ybuf));

    OGRCoordinateTransformation::DestroyCT(poCT);
    if (poLongLat != nullptr)
        poLongLat->Release();

    return ret;
}

//' Transform geospatial x/y coordinates
//'
//' `transform_xy()` transforms geospatial x/y coordinates to a new projection.
//'
//' @param pts A two-column data frame or numeric matrix containing geospatial
//' x/y coordinates.
//' @param srs_from Character string specifying the spatial reference system
//' for `pts`. May be in WKT format or any of the formats supported by
//' [srs_to_wkt()].
//' @param srs_to Character string specifying the output spatial reference
//' system. May be in WKT format or any of the formats supported by
//' [srs_to_wkt()].
//' @returns Numeric array of geospatial x/y coordinates in the projection
//' specified by `srs_to`.
//'
//' @seealso
//' [epsg_to_wkt()], [srs_to_wkt()], [inv_project()]
//' @examples
//' pt_file <- system.file("extdata/storml_pts.csv", package="gdalraster")
//' pts <- read.csv(pt_file)
//' print(pts)
//' ## id, x, y in NAD83 / UTM zone 12N
//' ## transform to NAD83 / CONUS Albers
//' transform_xy(pts = pts[, -1], srs_from = "EPSG:26912", srs_to = "EPSG:5070")
// [[Rcpp::export]]
Rcpp::NumericMatrix transform_xy(const Rcpp::RObject &pts,
                                 const std::string &srs_from,
                                 const std::string &srs_to) {

    Rcpp::NumericMatrix pts_in;
    if (Rcpp::is<Rcpp::DataFrame>(pts)) {
        pts_in = df_to_matrix_(pts);
    }
    else if (Rcpp::is<Rcpp::NumericVector>(pts)) {
        if (Rf_isMatrix(pts))
            pts_in = Rcpp::as<Rcpp::NumericMatrix>(pts);
    }
    else {
        Rcpp::stop("'pts' must be a data frame or matrix");
    }

    std::string srs_from_in = srs_to_wkt(srs_from, false);
    std::string srs_to_in = srs_to_wkt(srs_to, false);

    OGRSpatialReference oSourceSRS{}, oDestSRS{};
    OGRCoordinateTransformation *poCT = nullptr;
    OGRErr err;

    err = oSourceSRS.importFromWkt(srs_from_in.c_str());
    if (err != OGRERR_NONE)
        Rcpp::stop("failed to import source SRS from WKT string");

    // config option OGR_CT_FORCE_TRADITIONAL_GIS_ORDER=YES is set in gdal_init
    // this should be redundant
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
    std::vector<double> xbuf = Rcpp::as<std::vector<double>>(x);
    std::vector<double> ybuf = Rcpp::as<std::vector<double>>(y);
    if (!poCT->Transform(pts_in.nrow(), xbuf.data(), ybuf.data())) {
        OGRCoordinateTransformation::DestroyCT(poCT);
        Rcpp::stop("coordinate transformation failed");
    }

    Rcpp::NumericMatrix ret(pts_in.nrow(), 2);
    ret.column(0) = Rcpp::as<Rcpp::NumericVector>(Rcpp::wrap(xbuf));
    ret.column(1) = Rcpp::as<Rcpp::NumericVector>(Rcpp::wrap(ybuf));

    OGRCoordinateTransformation::DestroyCT(poCT);

    return ret;
}
