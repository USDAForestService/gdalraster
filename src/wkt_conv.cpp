/* WKT-related convenience functions
   Chris Toney <chris.toney at usda.gov>
   Copyright (c) 2023-2024 gdalraster authors
*/

#include "wkt_conv.h"

#include "cpl_port.h"
#include "cpl_conv.h"
#include "ogr_srs_api.h"
#include "ogr_spatialref.h"
#include "ogrsf_frmts.h"

#include "geos_wkt.h"

//' Convert spatial reference from EPSG code to OGC Well Known Text
//'
//' `epsg_to_wkt()` exports the spatial reference for an EPSG code to
//' WKT format.
//'
//' @details
//' As of GDAL 3.0, the default format for WKT export is OGC WKT 1.
//' The WKT version can be overridden by using the OSR_WKT_FORMAT
//' configuration option (see [set_config_option()]).
//' Valid values are one of: SFSQL, WKT1_SIMPLE, WKT1, WKT1_GDAL,
//' WKT1_ESRI, WKT2_2015, WKT2_2018, WKT2, DEFAULT.
//' If SFSQL, a WKT1 string without AXIS, TOWGS84, AUTHORITY or
//' EXTENSION node is returned. If WKT1_SIMPLE, a WKT1 string without
//' AXIS, AUTHORITY or EXTENSION node is returned. WKT1 is an alias of
//' WKT1_GDAL. WKT2 will default to the latest revision implemented
//' (currently WKT2_2018). WKT2_2019 can be used as an alias of
//' WKT2_2018 since GDAL 3.2
//'
//' @param epsg Integer EPSG code.
//' @param pretty Logical. `TRUE` to return a nicely formatted WKT string
//' for display to a person. `FALSE` for a regular WKT string (the default).
//' @return Character string containing OGC WKT.
//'
//' @seealso
//' [srs_to_wkt()]
//'
//' @examples
//' epsg_to_wkt(5070)
//' writeLines(epsg_to_wkt(5070, pretty=TRUE))
//' set_config_option("OSR_WKT_FORMAT", "WKT2")
//' writeLines(epsg_to_wkt(5070, pretty=TRUE))
//' set_config_option("OSR_WKT_FORMAT", "")
// [[Rcpp::export]]
std::string epsg_to_wkt(int epsg, bool pretty = false) {
    OGRSpatialReferenceH hSRS = OSRNewSpatialReference(nullptr);
    char *pszSRS_WKT = nullptr;

    if (OSRImportFromEPSG(hSRS, epsg) != OGRERR_NONE) {
        if (hSRS != nullptr)
            OSRDestroySpatialReference(hSRS);
        Rcpp::stop("error importing SRS from EPSG code");
    }

    if (pretty) {
        if (OSRExportToPrettyWkt(hSRS, &pszSRS_WKT, false) != OGRERR_NONE) {
            OSRDestroySpatialReference(hSRS);
            Rcpp::stop("error exporting to pretty WKT");
        }
    }
    else {
        if (OSRExportToWkt(hSRS, &pszSRS_WKT) != OGRERR_NONE) {
            OSRDestroySpatialReference(hSRS);
            Rcpp::stop("error exporting to WKT");
        }
    }

    std::string wkt(pszSRS_WKT);
    OSRDestroySpatialReference(hSRS);
    CPLFree(pszSRS_WKT);

    return wkt;
}

//' Convert various spatial reference formats to Well Known Text
//'
//' `srs_to_wkt()` converts a spatial reference system (SRS) definition
//' in various text formats to WKT. The function will examine the input SRS,
//' try to deduce the format, and then export it to WKT.
//'
//' @details
//' This is a wrapper for `OSRSetFromUserInput()` in the GDAL Spatial
//' Reference System C API with output to WKT.
//' The input SRS may take the following forms:
//'   * WKT - to convert WKT versions (see below)
//'   * EPSG:n - EPSG code n
//'   * AUTO:proj_id,unit_id,lon0,lat0 - WMS auto projections
//'   * urn:ogc:def:crs:EPSG::n - OGC URNs
//'   * PROJ.4 definitions
//'   * filename - file to read for WKT, XML or PROJ.4 definition
//'   * well known name such as NAD27, NAD83, WGS84 or WGS72
//'   * IGNF:xxxx, ESRI:xxxx - definitions from the PROJ database
//'   * PROJJSON (PROJ >= 6.2)
//'
//' This function is intended to be flexible, but by its nature it is
//' imprecise as it must guess information about the format intended.
//' [epsg_to_wkt()] could be used instead for EPSG codes.
//'
//' As of GDAL 3.0, the default format for WKT export is OGC WKT 1.
//' The WKT version can be overridden by using the OSR_WKT_FORMAT
//' configuration option (see [set_config_option()]).
//' Valid values are one of: SFSQL, WKT1_SIMPLE, WKT1, WKT1_GDAL,
//' WKT1_ESRI, WKT2_2015, WKT2_2018, WKT2, DEFAULT.
//' If SFSQL, a WKT1 string without AXIS, TOWGS84, AUTHORITY or
//' EXTENSION node is returned. If WKT1_SIMPLE, a WKT1 string without
//' AXIS, AUTHORITY or EXTENSION node is returned. WKT1 is an alias of
//' WKT1_GDAL. WKT2 will default to the latest revision implemented
//' (currently WKT2_2018). WKT2_2019 can be used as an alias of
//' WKT2_2018 since GDAL 3.2
//'
//' @param srs Character string containing an SRS definition in various
//' formats (see Details).
//' @param pretty Logical. `TRUE` to return a nicely formatted WKT string
//' for display to a person. `FALSE` for a regular WKT string (the default).
//' @return Character string containing OGC WKT.
//'
//' @seealso
//' [epsg_to_wkt()]
//'
//' @examples
//' srs_to_wkt("NAD83")
//' writeLines(srs_to_wkt("NAD83", pretty=TRUE))
//' set_config_option("OSR_WKT_FORMAT", "WKT2")
//' writeLines(srs_to_wkt("NAD83", pretty=TRUE))
//' set_config_option("OSR_WKT_FORMAT", "")
// [[Rcpp::export]]
std::string srs_to_wkt(const std::string &srs, bool pretty = false) {
    if (srs == "")
        return "";

    OGRSpatialReferenceH hSRS = OSRNewSpatialReference(nullptr);
    char *pszSRS_WKT = nullptr;

    if (OSRSetFromUserInput(hSRS, srs.c_str()) != OGRERR_NONE) {
        if (hSRS != nullptr)
            OSRDestroySpatialReference(hSRS);
        Rcpp::stop("error importing SRS from user input");
    }

    if (pretty) {
        if (OSRExportToPrettyWkt(hSRS, &pszSRS_WKT, false) != OGRERR_NONE) {
            OSRDestroySpatialReference(hSRS);
            Rcpp::stop("error exporting to pretty WKT");
        }
    }
    else {
        if (OSRExportToWkt(hSRS, &pszSRS_WKT) != OGRERR_NONE) {
            OSRDestroySpatialReference(hSRS);
            Rcpp::stop("error exporting to WKT");
        }
    }

    std::string wkt(pszSRS_WKT);
    OSRDestroySpatialReference(hSRS);
    CPLFree(pszSRS_WKT);

    return wkt;
}

//' Check if WKT definition is a geographic coordinate system
//'
//' `srs_is_geographic()` will attempt to import the given WKT string as a
//' spatial reference system, and returns `TRUE`  if the root is a
//' GEOGCS node. This is a wrapper for `OSRIsGeographic()` in the GDAL Spatial
//' Reference System C API.
//'
//' @param srs Character OGC WKT string for a spatial reference system
//' @return Logical. `TRUE` if `srs` is geographic, otherwise `FALSE`
//'
//' @seealso
//' [srs_is_projected()], [srs_is_same()]
//'
//' @examples
//' srs_is_geographic(epsg_to_wkt(5070))
//' srs_is_geographic(srs_to_wkt("WGS84"))
// [[Rcpp::export]]
bool srs_is_geographic(const std::string &srs) {
    OGRSpatialReferenceH hSRS = OSRNewSpatialReference(nullptr);
    char* pszWKT;
    pszWKT = (char*) srs.c_str();

    if (OSRImportFromWkt(hSRS, &pszWKT) != OGRERR_NONE) {
        if (hSRS != nullptr)
            OSRDestroySpatialReference(hSRS);
        Rcpp::stop("error importing SRS from user input");
    }

    bool ret = OSRIsGeographic(hSRS);
    OSRDestroySpatialReference(hSRS);
    return ret;
}

//' Check if WKT definition is a projected coordinate system
//'
//' `srs_is_projected()` will attempt to import the given WKT string as a
//' spatial reference system (SRS), and returns `TRUE` if the SRS contains a
//' PROJCS node indicating a it is a projected coordinate system. This is a
//' wrapper for `OSRIsProjected()` in the GDAL Spatial Reference System C API.
//'
//' @param srs Character OGC WKT string for a spatial reference system
//' @return Logical. `TRUE` if `srs` is projected, otherwise `FALSE`
//'
//' @seealso
//' [srs_is_geographic()], [srs_is_same()]
//'
//' @examples
//' srs_is_projected(epsg_to_wkt(5070))
//' srs_is_projected(srs_to_wkt("WGS84"))
// [[Rcpp::export]]
bool srs_is_projected(const std::string &srs) {
    OGRSpatialReferenceH hSRS = OSRNewSpatialReference(nullptr);
    char* pszWKT;
    pszWKT = (char*) srs.c_str();

    if (OSRImportFromWkt(hSRS, &pszWKT) != OGRERR_NONE) {
        if (hSRS != nullptr)
            OSRDestroySpatialReference(hSRS);
        Rcpp::stop("error importing SRS from user input");
    }

    bool ret = OSRIsProjected(hSRS);
    OSRDestroySpatialReference(hSRS);
    return ret;
}

//' Do these two spatial references describe the same system?
//'
//' `srs_is_same()` returns `TRUE` if these two spatial references describe
//' the same system. This is a wrapper for `OSRIsSame()` in the GDAL Spatial
//' Reference System C API.
//'
//' @param srs1 Character string. OGC WKT for a spatial reference system.
//' @param srs2 Character string. OGC WKT for a spatial reference system.
//' @param criterion Character string. One of `STRICT`, `EQUIVALENT`,
//' `EQUIVALENT_EXCEPT_AXIS_ORDER_GEOGCRS`.
//' Defaults to `EQUIVALENT_EXCEPT_AXIS_ORDER_GEOGCRS`.
//' @param ignore_axis_mapping Logical scalar. If `TRUE`, sets
//' `IGNORE_DATA_AXIS_TO_SRS_AXIS_MAPPING=YES` in the call to `OSRIsSameEx()`
//' in the GDAL Spatial Reference System API. Defaults to `NO`.
//' @param ignore_coord_epoch Logical scalar. If `TRUE`, sets
//' `IGNORE_COORDINATE_EPOCH=YES` in the call to `OSRIsSameEx()`
//' in the GDAL Spatial Reference System API. Defaults to `NO`.
//' @return Logical. `TRUE` if these two spatial references describe the same
//' system, otherwise `FALSE`.
//'
//' @seealso
//' [srs_is_geographic()], [srs_is_projected()]
//'
//' @examples
//' elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
//' ds <- new(GDALRaster, elev_file, TRUE)
//' srs_is_same(ds$getProjectionRef(), epsg_to_wkt(26912))
//' srs_is_same(ds$getProjectionRef(), epsg_to_wkt(5070))
//' ds$close()
// [[Rcpp::export]]
bool srs_is_same(const std::string &srs1, const std::string &srs2,
        std::string criterion = "",
        bool ignore_axis_mapping = false,
        bool ignore_coord_epoch = false) {

    OGRSpatialReferenceH hSRS1 = OSRNewSpatialReference(nullptr);
    OGRSpatialReferenceH hSRS2 = OSRNewSpatialReference(nullptr);

    char* pszWKT1;
    pszWKT1 = (char*) srs1.c_str();
    if (OSRImportFromWkt(hSRS1, &pszWKT1) != OGRERR_NONE) {
        if (hSRS1 != nullptr)
            OSRDestroySpatialReference(hSRS1);
        if (hSRS2 != nullptr)
            OSRDestroySpatialReference(hSRS2);
        Rcpp::stop("error importing SRS from user input");
    }

    char* pszWKT2;
    pszWKT2 = (char*) srs2.c_str();
    if (OSRImportFromWkt(hSRS2, &pszWKT2) != OGRERR_NONE) {
        if (hSRS1 != nullptr)
            OSRDestroySpatialReference(hSRS1);
        if (hSRS2 != nullptr)
            OSRDestroySpatialReference(hSRS2);
        Rcpp::stop("error importing SRS from user input");
    }

    std::vector<char *> opt_list;
    std::string str_axis;
    std::string str_epoch;

    if (criterion != "") {
        criterion = "CRITERION=" + criterion;
        opt_list.push_back((char *) criterion.c_str());
    }

    if (ignore_axis_mapping) {
        str_axis = "IGNORE_DATA_AXIS_TO_SRS_AXIS_MAPPING=YES";
    }
    else {
        str_axis = "IGNORE_DATA_AXIS_TO_SRS_AXIS_MAPPING=NO";
    }
    opt_list.push_back((char *) str_axis.c_str());

    if (ignore_coord_epoch) {
        str_epoch = "IGNORE_COORDINATE_EPOCH=YES";
    }
    else {
        str_epoch = "IGNORE_COORDINATE_EPOCH=NO";
    }
    opt_list.push_back((char *) str_epoch.c_str());

    opt_list.push_back(nullptr);

    bool ret = OSRIsSameEx(hSRS1, hSRS2, opt_list.data());
    OSRDestroySpatialReference(hSRS1);
    OSRDestroySpatialReference(hSRS2);
    return ret;
}

//' Get the bounding box of a geometry specified in OGC WKT format
//'
//' `bbox_from_wkt()` returns the bounding box of a WKT 2D geometry
//' (e.g., LINE, POLYGON, MULTIPOLYGON).
//'
//' @param wkt Character. OGC WKT string for a simple feature 2D geometry.
//' @param extend_x Numeric scalar. Distance to extend the output bounding box
//' in both directions along the x-axis
//' (results in `xmin = bbox[1] - extend_x`, `xmax = bbox[3] + extend_x`).
//' @param extend_y Numeric scalar. Distance to extend the output bounding box
//' in both directions along the y-axis
//' (results in `ymin = bbox[2] - extend_y`, `ymax = bbox[4] + extend_y`).
//' @return Numeric vector of length four containing the xmin, ymin,
//' xmax, ymax of the geometry specified by `wkt` (possibly extended by values
//' in `extend_x`, `extend_y`).
//'
//' @seealso
//' [bbox_to_wkt()]
//'
//' @examples
//' bnd <- "POLYGON ((324467.3 5104814.2, 323909.4 5104365.4, 323794.2
//' 5103455.8, 324970.7 5102885.8, 326420.0 5103595.3, 326389.6 5104747.5,
//' 325298.1 5104929.4, 325298.1 5104929.4, 324467.3 5104814.2))"
//' bbox_from_wkt(bnd, 100, 100)
// [[Rcpp::export]]
Rcpp::NumericVector bbox_from_wkt(const std::string &wkt,
        double extend_x = 0, double extend_y = 0) {

    OGRGeometryH hGeometry = nullptr;
    char* pszWKT;
    pszWKT = (char*) wkt.c_str();

    if (OGR_G_CreateFromWkt(&pszWKT, nullptr, &hGeometry) != OGRERR_NONE) {
        if (hGeometry != nullptr)
            OGR_G_DestroyGeometry(hGeometry);
        Rcpp::Rcerr << "failed to create geometry object from WKT string\n";
        Rcpp::NumericVector ret(4, NA_REAL);
        return ret;
    }

    OGREnvelope sBbox;
    OGR_G_GetEnvelope(hGeometry, &sBbox);
    Rcpp::NumericVector bbox = {
        sBbox.MinX - extend_x,
        sBbox.MinY - extend_y,
        sBbox.MaxX + extend_x,
        sBbox.MaxY + extend_y
    };

    OGR_G_DestroyGeometry(hGeometry);

    return bbox;
}

//' Convert a bounding box to POLYGON in OGC WKT format
//'
//' `bbox_to_wkt()` returns a WKT POLYGON string for the given bounding box.
//' Requires GDAL built with the GEOS library.
//'
//' @param bbox Numeric vector of length four containing xmin, ymin,
//' xmax, ymax.
//' @param extend_x Numeric scalar. Distance in units of `bbox` to extend the
//' rectangle in both directions along the x-axis
//' (results in `xmin = bbox[1] - extend_x`, `xmax = bbox[3] + extend_x`).
//' @param extend_y Numeric scalar. Distance in units of `bbox` to extend the
//' rectangle in both directions along the y-axis
//' (results in `ymin = bbox[2] - extend_y`, `ymax = bbox[4] + extend_y`).
//' @return Character string for an OGC WKT polygon.
//' `NA` is returned if GDAL was built without the GEOS library.
//'
//' @seealso
//' [bbox_from_wkt()], [g_buffer()]
//'
//' @examples
//' elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
//' ds <- new(GDALRaster, elev_file, read_only=TRUE)
//' bbox_to_wkt(ds$bbox())
//' ds$close()
// [[Rcpp::export]]
Rcpp::String bbox_to_wkt(const Rcpp::NumericVector &bbox,
        double extend_x = 0, double extend_y = 0) {

    if (bbox.size() != 4)
        Rcpp::stop("invalid bounding box");

    Rcpp::NumericVector bbox_in = Rcpp::clone(bbox);
    bbox_in[0] -= extend_x;
    bbox_in[1] -= extend_y;
    bbox_in[2] += extend_x;
    bbox_in[3] += extend_y;

    Rcpp::NumericMatrix poly_xy(5, 2);
    poly_xy.row(0) = Rcpp::NumericVector::create(bbox_in(0), bbox_in(1));
    poly_xy.row(1) = Rcpp::NumericVector::create(bbox_in(2), bbox_in(1));
    poly_xy.row(2) = Rcpp::NumericVector::create(bbox_in(2), bbox_in(3));
    poly_xy.row(3) = Rcpp::NumericVector::create(bbox_in(0), bbox_in(3));
    poly_xy.row(4) = Rcpp::NumericVector::create(bbox_in(0), bbox_in(1));

    return g_create(poly_xy, "POLYGON");
}
