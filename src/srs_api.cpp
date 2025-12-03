/* Spatial reference system functions operating on WKT
   Wraps a subset of the GDAL Spatial Reference System C API (ogr_srs_api.h)

   Chris Toney <chris.toney at usda.gov>
   Copyright (c) 2023-2025 gdalraster authors
*/

#include <cpl_port.h>
#include <cpl_conv.h>
#include <ogr_srs_api.h>
#include <ogr_spatialref.h>
#include <ogrsf_frmts.h>

#include <Rcpp.h>

#include <memory>
#include <string>
#include <vector>

#include "srs_api.h"
#include "transform.h"


//' Convert spatial reference definitions to OGC WKT or PROJJSON
//'
//' These functions convert various spatial reference formats to Well Known
//' Text (WKT) or PROJJSON.
//'
//' @name srs_convert
//'
//' @details
//' `epsg_to_wkt()` exports the spatial reference for an EPSG code to
//' WKT format.
//' Wrapper for `OSRImportFromEPSG()` in the GDAL Spatial Reference System API
//' with output to WKT.
//'
//' `srs_to_wkt()` converts a spatial reference system (SRS) definition
//' in various text formats to WKT. The function will examine the input SRS,
//' try to deduce the format, and then export it to WKT.
//' Wrapper for `OSRSetFromUserInput()` in the GDAL Spatial Reference System
//' API with output to WKT.
//'
//' `srs_to_projjson()` accepts a spatial reference system (SRS) definition in
//' any of the formats supported by `srs_to_wkt()`, and converts into PROJJSON
//' format. Wrapper for `OSRExportToPROJJSON()` in the GDAL Spatial Reference
//' System API.
//'
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
//' `srs_to_wkt()` is intended to be flexible, but by its nature it is
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
//' @param epsg Integer EPSG code.
//' @param srs Character string containing an SRS definition in various
//' formats (see Details).
//' @param pretty Logical value. `TRUE` to return a nicely formatted WKT string
//' for display to a person. `FALSE` for a regular WKT string (the default).
//' @return Character string containing OGC WKT.
//' @param gcs_only Logical value. `TRUE` to return only the definition of the
//' GEOGCS node of the input `srs`. Defaults to `FALSE` (see Note).
//' @param multiline Logical value. `TRUE` for PROJJSON multiline output (the
//' default).
//' @param indent_width Integer value. Defaults to `2`. Only used if
//' `multiline = TRUE` for PROJJSON output.
//' @param schema Character string containing URL to PROJJSON schema. Can be
//' set to empty string to disable it.
//'
//' @note
//' Setting `gcs_only = TRUE` in `srs_to_wkt()` is a wrapper of
//' `OSRCloneGeogCS()` in the GDAL API. The returned WKT will be for the GEOGCS
//' node of the input `srs`.
//'
//' @seealso
//' [srs_query]
//'
//' @examples
//' epsg_to_wkt(5070)
//' writeLines(epsg_to_wkt(5070, pretty = TRUE))
//'
//' srs_to_wkt("NAD83")
//' writeLines(srs_to_wkt("NAD83", pretty = TRUE))
//' set_config_option("OSR_WKT_FORMAT", "WKT2")
//' writeLines(srs_to_wkt("NAD83", pretty = TRUE))
//' set_config_option("OSR_WKT_FORMAT", "")
//'
//' srs_to_wkt("EPSG:5070", gcs_only = TRUE)
//'
//' srs_to_projjson("NAD83") |> writeLines()
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

//' @noRd
std::string srs_to_wkt(const std::string &srs, bool pretty) {
    return srs_to_wkt(srs, pretty, false);
}

//' @rdname srs_convert
// [[Rcpp::export]]
std::string srs_to_wkt(const std::string &srs, bool pretty = false,
                       bool gcs_only = false) {
    if (srs == "")
        return "";

    OGRSpatialReferenceH hSRS = OSRNewSpatialReference(nullptr);
    OGRSpatialReferenceH hSRS_out = nullptr;
    char *pszSRS_WKT = nullptr;

    if (OSRSetFromUserInput(hSRS, srs.c_str()) != OGRERR_NONE) {
        if (hSRS != nullptr)
            OSRDestroySpatialReference(hSRS);
        Rcpp::stop("error importing SRS from user input");
    }

    if (gcs_only)
        hSRS_out = OSRCloneGeogCS(hSRS);
    else
        hSRS_out = hSRS;

    if (pretty) {
        if (OSRExportToPrettyWkt(hSRS_out, &pszSRS_WKT, false) != OGRERR_NONE) {
            if (gcs_only && hSRS_out)
                OSRDestroySpatialReference(hSRS_out);

            OSRDestroySpatialReference(hSRS);
            Rcpp::stop("error exporting to pretty WKT");
        }
    }
    else {
        if (OSRExportToWkt(hSRS_out, &pszSRS_WKT) != OGRERR_NONE) {
            if (gcs_only && hSRS_out)
                OSRDestroySpatialReference(hSRS_out);

            OSRDestroySpatialReference(hSRS);
            Rcpp::stop("error exporting to WKT");
        }
    }

    std::string wkt(pszSRS_WKT);
    if (gcs_only)
        OSRDestroySpatialReference(hSRS_out);
    OSRDestroySpatialReference(hSRS);
    CPLFree(pszSRS_WKT);

    return wkt;
}

//' @rdname srs_convert
// [[Rcpp::export]]
std::string srs_to_projjson(const std::string &srs,
                            bool multiline = true,
                            int indent_width = 2,
                            const Rcpp::String &schema = NA_STRING) {
    if (srs == "")
        return "";

    std::vector<int> proj_ver = getPROJVersion();
    if (!(proj_ver[0] > 6 || proj_ver[1] >= 2))
        Rcpp::stop("srs_to_projjson() requires PROJ >= 6.2");

    OGRSpatialReferenceH hSRS = OSRNewSpatialReference(nullptr);

    if (OSRSetFromUserInput(hSRS, srs.c_str()) != OGRERR_NONE) {
        if (hSRS != nullptr)
            OSRDestroySpatialReference(hSRS);
        Rcpp::stop("error importing SRS from user input");
    }

    std::vector<const char *> opt_list;
    if (!multiline) {
        opt_list.push_back("MULTILINE=NO");
    }
    std::string str_indent = "INDENTATION_WIDTH=2";
    if (indent_width != 2)
        str_indent = "INDENTATION_WIDTH=" + std::to_string(indent_width);
    if (multiline)
        opt_list.push_back(str_indent.c_str());
    std::string str_schema = "SCHEMA=";
    if (schema != NA_STRING) {
        str_schema += schema.get_cstring();
        opt_list.push_back(str_schema.c_str());
    }
    opt_list.push_back(nullptr);

    char *pszSRS_PROJJSON = nullptr;
    if (OSRExportToPROJJSON(hSRS, &pszSRS_PROJJSON, opt_list.data())
        != OGRERR_NONE) {

        OSRDestroySpatialReference(hSRS);
        CPLFree(pszSRS_PROJJSON);
        Rcpp::stop("error exporting to PROJJSON");
    }

    std::string json(pszSRS_PROJJSON);
    OSRDestroySpatialReference(hSRS);
    CPLFree(pszSRS_PROJJSON);

    return json;
}

//' Obtain information about a spatial reference system
//'
//' Bindings to a subset of the GDAL Spatial Reference System API
//' (\url{https://gdal.org/en/stable/api/ogr_srs_api.html}).
//' These functions return various information about a spatial reference
//' system passed as text in any of the formats supported by [srs_to_wkt()].
//'
//' @name srs_query
//'
//' @details
//' `srs_find_epsg()` tries to find a matching EPSG code.
//' Matching may be partial, or may fail. If `all_matches = TRUE`, returns a
//' data frame with entries sorted by decreasing match confidence (first
//' entry has the highest match confidence); the default is `FALSE` which
//' returns a character string in the form "EPSG:####" for the first match
//' (highest confidence). Wrapper of `OSRFindMatches()` in the GDAL SRS API.
//'
//' `srs_get_name()` returns the SRS name.
//' Wrapper of `OSRGetName()` in the GDAL API.
//'
//' `srs_is_geographic()` returns `TRUE`  if the root is a GEOGCS node.
//' Wrapper of `OSRIsGeographic()` in the GDAL API.
//'
//' `srs_is_derived_gcs()` returns `TRUE` if the SRS is a derived geographic
//' coordinate system (for example a rotated long/lat grid).
//' Wrapper of `OSRIsDerivedGeographic()` in the GDAL API.
//'
//' `srs_is_local()` returns `TRUE` if the SRS is a local coordinate system
//' (the root is a LOCAL_CS node).
//' Wrapper of `OSRIsLocal()` in the GDAL API.
//'
//' `srs_is_projected()` returns `TRUE` if the SRS contains a PROJCS node
//' indicating a it is a projected coordinate system.
//' Wrapper of `OSRIsProjected()` in the GDAL API.
//'
//' `srs_is_compound()` returns `TRUE` if the SRS is compound.
//' Wrapper of `OSRIsCompound()` in the GDAL API.
//'
//' `srs_is_geocentric()` returns `TRUE` if the SRS is a geocentric coordinate
//' system.
//' Wrapper of `OSRIsGeocentric()` in the GDAL API.
//'
//' `srs_is_vertical()` returns `TRUE` if the SRS is a vertical coordinate
//' system.
//' Wrapper of `OSRIsVertical()` in the GDAL API.
//'
//' `srs_is_dynamic()` returns `TRUE` if the SRS is is a dynamic coordinate
//' system (relies on a dynamic datum, i.e., a datum that is not plate-fixed).
//' Wrapper of `OSRIsDynamic()` in the GDAL API. Requires GDAL >= 3.4.
//'
//' `srs_is_same()` returns `TRUE` if two spatial references describe
//' the same system.
//' Wrapper of `OSRIsSame()` in the GDAL API.
//'
//' `srs_get_angular_units()` fetches the angular geographic coordinate system
//' units. Returns a list of length two: the first element contains the unit
//' name as a character string, and the second element contains a numeric value
//' to multiply by angular distances to transform them to radians.
//' Wrapper of `OSRGetAngularUnits()` in the GDAL API.
//'
//' `srs_get_linear_units()` fetches the linear projection units.
//' Returns a list of length two: the first element contains the unit
//' name as a character string, and the second element contains a numeric value
//' to multiply by linear distances to transform them to meters.
//' If no units are available, values of "Meters" and 1.0 will be assumed.
//' Wrapper of `OSRGetLinearUnits()` in the GDAL API.
//'
//' `srs_get_coord_epoch()` returns the coordinate epoch, as decimal year
//' (e.g. 2021.3), or `0` if not set or not relevant.
//' Wrapper of `OSRGetCoordinateEpoch()` in the GDAL API. Requires GDAL >= 3.4.
//'
//' `srs_get_utm_zone()` returns the UTM zone number or zero if `srs` isn't a
//' UTM definition. A positive value indicates northern hemisphere; a negative
//' value is in the southern hemisphere.
//' Wrapper of `OSRGetUTMZone()` in the GDAL API.
//'
//' `srs_get_axis_mapping_strategy()` returns the data axis to CRS axis mapping
//' strategy as a character string, one of:
//' * `OAMS_TRADITIONAL_GIS_ORDER`: for geographic CRS with lat/long order, the
//' data will still be long/lat ordered. Similarly for a projected CRS with
//' northing/easting order, the data will still be easting/northing ordered.
//' * `OAMS_AUTHORITY_COMPLIANT`: the data axis will be identical to the CRS
//' axis.
//' * `OAMS_CUSTOM`: custom-defined data axis
//'
//' `srs_get_area_of_use()` is a wrapper of `OSRGetAreaOfUse()` in the GDAL API.
//' Returns a named list containing the following elements (or returns `NULL` if
//' the API call does not succeed):
//' * `AreaName`: the area of use
//' * `WestLongitudeDeg`: the western-most longitude expressed in degree, or
//' `NA` if the bounding box is unknown
//' * `SouthLatitudeDeg`: the southern-most latitude expressed in degree, or
//' `NA` if the bounding box is unknown
//' * `EastLongitudeDeg`: the eastern-most longitude expressed in degree, or
//' `NA` if the bounding box is unknown
//' * `NorthLatitudeDeg`: the northern-most latitude expressed in degree, or
//' `NA` if the bounding box is unknown
//'
//' `srs_get_axes_count()` returns the integer number of axes of the coordinate
//' system of the SRS. Wrapper of `OSRGetAxesCount()` in the GDAL API.
//'
//' `srs_get_axes()` returns a named list of the axis names and their
//' orientations. Wrapper of `OSRGetAxis()` in the GDAL API.
//'
//' `srs_get_celestial_body_name()` returns the name of the celestial body of
//' the SRS, e.g., `"Earth"` for an Earth SRS. Wrapper of
//' `OSRGetCelestialBodyName()` in the GDAL API. Requires GDAL >= 3.12 and
//' PROJ >= 8.1.
//'
//' @param srs Character string containing an SRS definition in various
//' formats (e.g., WKT, PROJ.4 string, well known name such as NAD27, NAD83,
//' WGS84, etc., see [srs_to_wkt()]).
//' @param srs_other Character string containing an SRS definition in various
//' formats(see above).
//' @param criterion Character string. One of `STRICT`, `EQUIVALENT`,
//' `EQUIVALENT_EXCEPT_AXIS_ORDER_GEOGCRS`.
//' Defaults to `EQUIVALENT_EXCEPT_AXIS_ORDER_GEOGCRS`.
//' @param ignore_axis_mapping Logical scalar. If `TRUE`, sets
//' `IGNORE_DATA_AXIS_TO_SRS_AXIS_MAPPING=YES` in the call to `OSRIsSameEx()`
//' in the GDAL Spatial Reference System API. Defaults to `NO`.
//' @param ignore_coord_epoch Logical scalar. If `TRUE`, sets
//' `IGNORE_COORDINATE_EPOCH=YES` in the call to `OSRIsSameEx()`
//' in the GDAL Spatial Reference System API. Defaults to `NO`.
//' @param all_matches Logical scalar. `TRUE` to return all identified matches
//' in a data frame, including a confidence value (0-100) for each match. The
//' default is `FALSE` which returns a character string in the form
//' `"EPSG:<code>"` for the first match (highest confidence).
//' @param target_key Optional character string giving the coordinate system
//' part to query, either `"PROJCS"` or `"GEOGCS"` (case-insensitive)
//'
//' @seealso
//' [srs_convert]
//'
//' @examples
//' wkt <- 'PROJCS["ETRS89 / UTM zone 32N (N-E)",
//'         GEOGCS["ETRS89",
//'             DATUM["European_Terrestrial_Reference_System_1989",
//'                 SPHEROID["GRS 1980",6378137,298.257222101,
//'                     AUTHORITY["EPSG","7019"]],
//'                 TOWGS84[0,0,0,0,0,0,0],
//'                 AUTHORITY["EPSG","6258"]],
//'             PRIMEM["Greenwich",0,
//'                 AUTHORITY["EPSG","8901"]],
//'             UNIT["degree",0.0174532925199433,
//'                 AUTHORITY["EPSG","9122"]],
//'             AUTHORITY["EPSG","4258"]],
//'         PROJECTION["Transverse_Mercator"],
//'         PARAMETER["latitude_of_origin",0],
//'         PARAMETER["central_meridian",9],
//'         PARAMETER["scale_factor",0.9996],
//'         PARAMETER["false_easting",500000],
//'         PARAMETER["false_northing",0],
//'         UNIT["metre",1,
//'             AUTHORITY["EPSG","9001"]],
//'         AXIS["Northing",NORTH],
//'         AXIS["Easting",EAST]]'
//'
//' srs_find_epsg(wkt)
//'
//' srs_find_epsg(wkt, all_matches = TRUE)
//'
//' srs_get_name("EPSG:5070")
//'
//' srs_is_geographic("EPSG:5070")
//' srs_is_geographic("EPSG:4326")
//'
//' srs_is_derived_gcs("WGS84")
//'
//' srs_is_projected("EPSG:5070")
//' srs_is_projected("EPSG:4326")
//'
//' srs_is_compound("EPSG:4326")
//'
//' srs_is_geocentric("EPSG:7789")
//'
//' srs_is_vertical("EPSG:5705")
//'
//' srs_get_area_of_use("EPSG:3976")
//'
//' srs_get_axes_count("EPSG:4326")
//' srs_get_axes_count("EPSG:4979")
//'
//' ## ordered list of axis names and their orientation
//' srs_get_axes("EPSG:4326+5773")
//'
//' ## Requires GDAL >= 3.12 and PROJ >= 8.1
//' # srs_get_celestial_body_name("EPSG:4326")
//' #> [1] "Earth"
//' # srs_get_celestial_body_name("IAU_2015:30100")
//' #> [1] "Moon"
//'
//' f <- system.file("extdata/storml_elev.tif", package="gdalraster")
//' ds <- new(GDALRaster, f)
//'
//' ds$getProjection() |> srs_is_projected()
//' ds$getProjection() |> srs_get_utm_zone()
//' ds$getProjection() |> srs_get_angular_units()
//' ds$getProjection() |> srs_get_linear_units()
//' ds$getProjection() |> srs_get_axis_mapping_strategy()
//'
//' ds$getProjection() |> srs_is_same("EPSG:26912")
//' ds$getProjection() |> srs_is_same("NAD83")
//'
//' ds$close()
//'
//' ## Requires GDAL >= 3.4
//' if (gdal_version_num() >= gdal_compute_version(3, 4, 0)) {
//'   if (srs_is_dynamic("WGS84"))
//'     print("WGS84 is dynamic")
//'
//'   if (!srs_is_dynamic("NAD83"))
//'     print("NAD83 is not dynamic")
//' }
// [[Rcpp::export]]
std::string srs_get_name(const std::string &srs) {
    if (srs == "")
        return "";

    OGRSpatialReferenceH hSRS = OSRNewSpatialReference(nullptr);

    if (OSRSetFromUserInput(hSRS, srs.c_str()) != OGRERR_NONE) {
        if (hSRS != nullptr)
            OSRDestroySpatialReference(hSRS);
        Rcpp::stop("error importing SRS from user input");
    }

    const char *pszName = OSRGetName(hSRS);
    std::string ret = "";
    if (pszName)
        ret = pszName;

    OSRDestroySpatialReference(hSRS);
    return ret;
}

//' @rdname srs_query
// [[Rcpp::export]]
SEXP srs_find_epsg(const std::string &srs, bool all_matches = false) {
    if (srs == "")
        return R_NilValue;

    OGRSpatialReferenceH hSRS = OSRNewSpatialReference(nullptr);

    if (OSRSetFromUserInput(hSRS, srs.c_str()) != OGRERR_NONE) {
        if (hSRS != nullptr)
            OSRDestroySpatialReference(hSRS);
        Rcpp::stop("error importing SRS from user input");
    }

    std::unique_ptr<OGRSpatialReferenceH> pahSRS;
    int nEntries = 0;
    int *panConfidence = nullptr;
    OGRSpatialReference oSRS;
    std::string identified_code = "";
    Rcpp::CharacterVector authority_name = Rcpp::CharacterVector::create();
    Rcpp::CharacterVector authority_code = Rcpp::CharacterVector::create();
    Rcpp::IntegerVector confidence = Rcpp::IntegerVector::create();

    pahSRS = std::unique_ptr<OGRSpatialReferenceH>(
        OSRFindMatches(hSRS, nullptr, &nEntries, &panConfidence));

    OSRDestroySpatialReference(hSRS);

    for (int i = 0; i < nEntries; i++) {
        oSRS = *reinterpret_cast<OGRSpatialReference *>(pahSRS.get()[i]);
        const char *pszAuthorityName = oSRS.GetAuthorityName(nullptr);
        const char *pszAuthorityCode = oSRS.GetAuthorityCode(nullptr);

        if (!all_matches) {
            if (panConfidence[i] != 100) {
                Rcpp::Rcout << "confidence in this match: " <<
                    panConfidence[i] << "%" << "\n";
            }
            if (pszAuthorityName && pszAuthorityCode) {
                identified_code = pszAuthorityName;
                identified_code += ":";
                identified_code += pszAuthorityCode;
            }
            break;
        }

        authority_name.push_back(pszAuthorityName);
        authority_code.push_back(pszAuthorityCode);
        confidence.push_back(panConfidence[i]);
    }

    OSRFreeSRSArray(pahSRS.release());
    CPLFree(panConfidence);

    if (nEntries == 0) {
        return R_NilValue;
    }
    else if (!all_matches) {
        return Rcpp::wrap(identified_code);
    }
    else {
        Rcpp::DataFrame df_out = Rcpp::DataFrame::create();
        df_out.push_back(authority_name, "authority_name");
        df_out.push_back(authority_code, "authority_code");
        df_out.push_back(confidence, "confidence");
        return df_out;
    }
}

//' @rdname srs_query
// [[Rcpp::export]]
bool srs_is_geographic(const std::string &srs) {
    if (srs == "")
        return false;

    OGRSpatialReferenceH hSRS = OSRNewSpatialReference(nullptr);

    if (OSRSetFromUserInput(hSRS, srs.c_str()) != OGRERR_NONE) {
        if (hSRS != nullptr)
            OSRDestroySpatialReference(hSRS);
        Rcpp::stop("error importing SRS from user input");
    }

    bool ret = OSRIsGeographic(hSRS);
    OSRDestroySpatialReference(hSRS);
    return ret;
}

//' @rdname srs_query
// [[Rcpp::export]]
bool srs_is_derived_gcs(const std::string &srs) {
    if (srs == "")
        return false;

    OGRSpatialReferenceH hSRS = OSRNewSpatialReference(nullptr);

    if (OSRSetFromUserInput(hSRS, srs.c_str()) != OGRERR_NONE) {
        if (hSRS != nullptr)
            OSRDestroySpatialReference(hSRS);
        Rcpp::stop("error importing SRS from user input");
    }

    bool ret = OSRIsDerivedGeographic(hSRS);
    OSRDestroySpatialReference(hSRS);
    return ret;
}

//' @rdname srs_query
// [[Rcpp::export]]
bool srs_is_local(const std::string &srs) {
    if (srs == "")
        return false;

    OGRSpatialReferenceH hSRS = OSRNewSpatialReference(nullptr);

    if (OSRSetFromUserInput(hSRS, srs.c_str()) != OGRERR_NONE) {
        if (hSRS != nullptr)
            OSRDestroySpatialReference(hSRS);
        Rcpp::stop("error importing SRS from user input");
    }

    bool ret = OSRIsLocal(hSRS);
    OSRDestroySpatialReference(hSRS);
    return ret;
}

//' @rdname srs_query
// [[Rcpp::export]]
bool srs_is_projected(const std::string &srs) {
    if (srs == "")
        return false;

    OGRSpatialReferenceH hSRS = OSRNewSpatialReference(nullptr);

    if (OSRSetFromUserInput(hSRS, srs.c_str()) != OGRERR_NONE) {
        if (hSRS != nullptr)
            OSRDestroySpatialReference(hSRS);
        Rcpp::stop("error importing SRS from user input");
    }

    bool ret = OSRIsProjected(hSRS);
    OSRDestroySpatialReference(hSRS);
    return ret;
}

//' @rdname srs_query
// [[Rcpp::export]]
bool srs_is_compound(const std::string &srs) {
    if (srs == "")
        return false;

    OGRSpatialReferenceH hSRS = OSRNewSpatialReference(nullptr);

    if (OSRSetFromUserInput(hSRS, srs.c_str()) != OGRERR_NONE) {
        if (hSRS != nullptr)
            OSRDestroySpatialReference(hSRS);
        Rcpp::stop("error importing SRS from user input");
    }

    bool ret = OSRIsCompound(hSRS);
    OSRDestroySpatialReference(hSRS);
    return ret;
}

//' @rdname srs_query
// [[Rcpp::export]]
bool srs_is_geocentric(const std::string &srs) {
    if (srs == "")
        return false;

    OGRSpatialReferenceH hSRS = OSRNewSpatialReference(nullptr);

    if (OSRSetFromUserInput(hSRS, srs.c_str()) != OGRERR_NONE) {
        if (hSRS != nullptr)
            OSRDestroySpatialReference(hSRS);
        Rcpp::stop("error importing SRS from user input");
    }

    bool ret = OSRIsGeocentric(hSRS);
    OSRDestroySpatialReference(hSRS);
    return ret;
}

//' @rdname srs_query
// [[Rcpp::export]]
bool srs_is_vertical(const std::string &srs) {
    if (srs == "")
        return false;

    OGRSpatialReferenceH hSRS = OSRNewSpatialReference(nullptr);

    if (OSRSetFromUserInput(hSRS, srs.c_str()) != OGRERR_NONE) {
        if (hSRS != nullptr)
            OSRDestroySpatialReference(hSRS);
        Rcpp::stop("error importing SRS from user input");
    }

    bool ret = OSRIsVertical(hSRS);
    OSRDestroySpatialReference(hSRS);
    return ret;
}

//' @rdname srs_query
// [[Rcpp::export]]
bool srs_is_dynamic(const std::string &srs) {

#if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 4, 0)
    Rcpp::stop("srs_is_dynamic() requires GDAL >= 3.4");

#else
    if (srs == "")
        return false;

    OGRSpatialReferenceH hSRS = OSRNewSpatialReference(nullptr);

    if (OSRSetFromUserInput(hSRS, srs.c_str()) != OGRERR_NONE) {
        if (hSRS != nullptr)
            OSRDestroySpatialReference(hSRS);
        Rcpp::stop("error importing SRS from user input");
    }

    bool ret = OSRIsDynamic(hSRS);
    OSRDestroySpatialReference(hSRS);
    return ret;
#endif
}

//' @rdname srs_query
// [[Rcpp::export]]
bool srs_is_same(const std::string &srs, const std::string &srs_other,
                 std::string criterion = "",
                 bool ignore_axis_mapping = false,
                 bool ignore_coord_epoch = false) {

    if (srs == "" || srs_other == "")
        return false;

    OGRSpatialReferenceH hSRS1 = OSRNewSpatialReference(nullptr);
    if (OSRSetFromUserInput(hSRS1, srs.c_str()) != OGRERR_NONE) {
        if (hSRS1 != nullptr)
            OSRDestroySpatialReference(hSRS1);
        Rcpp::stop("error importing SRS from user input");
    }

    OGRSpatialReferenceH hSRS2 = OSRNewSpatialReference(nullptr);
    if (OSRSetFromUserInput(hSRS2, srs_other.c_str()) != OGRERR_NONE) {
        if (hSRS2 != nullptr) {
            OSRDestroySpatialReference(hSRS1);
            OSRDestroySpatialReference(hSRS2);
        }
        Rcpp::stop("error importing SRS from user input");
    }

    std::vector<char *> opt_list;
    std::string str_axis;
    std::string str_epoch;

    if (criterion != "") {
        criterion = "CRITERION=" + criterion;
        opt_list.push_back(const_cast<char *>(criterion.c_str()));
    }

    if (ignore_axis_mapping) {
        str_axis = "IGNORE_DATA_AXIS_TO_SRS_AXIS_MAPPING=YES";
    }
    else {
        str_axis = "IGNORE_DATA_AXIS_TO_SRS_AXIS_MAPPING=NO";
    }
    opt_list.push_back(const_cast<char *>(str_axis.c_str()));

    if (ignore_coord_epoch) {
        str_epoch = "IGNORE_COORDINATE_EPOCH=YES";
    }
    else {
        str_epoch = "IGNORE_COORDINATE_EPOCH=NO";
    }
    opt_list.push_back(const_cast<char *>(str_epoch.c_str()));

    opt_list.push_back(nullptr);

    bool ret = OSRIsSameEx(hSRS1, hSRS2, opt_list.data());
    OSRDestroySpatialReference(hSRS1);
    OSRDestroySpatialReference(hSRS2);
    return ret;
}

//' @rdname srs_query
// [[Rcpp::export]]
SEXP srs_get_angular_units(const std::string &srs) {
    if (srs == "")
        return R_NilValue;

    OGRSpatialReferenceH hSRS = OSRNewSpatialReference(nullptr);

    if (OSRSetFromUserInput(hSRS, srs.c_str()) != OGRERR_NONE) {
        if (hSRS != nullptr)
            OSRDestroySpatialReference(hSRS);
        Rcpp::stop("error importing SRS from user input");
    }

    char *pszNameTmp = nullptr;
    double to_rad = OSRGetAngularUnits(hSRS, &pszNameTmp);
    std::string name_out(pszNameTmp);

    Rcpp::List list_out = Rcpp::List::create();
    list_out.push_back(name_out, "unit_name");
    list_out.push_back(to_rad, "to_radians");

    OSRDestroySpatialReference(hSRS);
    return list_out;
}

//' @rdname srs_query
// [[Rcpp::export]]
SEXP srs_get_linear_units(const std::string &srs) {
    if (srs == "")
        return R_NilValue;

    OGRSpatialReferenceH hSRS = OSRNewSpatialReference(nullptr);

    if (OSRSetFromUserInput(hSRS, srs.c_str()) != OGRERR_NONE) {
        if (hSRS != nullptr)
            OSRDestroySpatialReference(hSRS);
        Rcpp::stop("error importing SRS from user input");
    }

    char *pszNameTmp = nullptr;
    double to_m = OSRGetLinearUnits(hSRS, &pszNameTmp);
    std::string name_out(pszNameTmp);

    Rcpp::List list_out = Rcpp::List::create();
    list_out.push_back(name_out, "unit_name");
    list_out.push_back(to_m, "to_meters");

    OSRDestroySpatialReference(hSRS);
    return list_out;
}

//' @rdname srs_query
// [[Rcpp::export]]
double srs_get_coord_epoch(const std::string &srs) {

#if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 4, 0)
    Rcpp::stop("srs_get_coord_epoch() requires GDAL >= 3.4");

#else
    if (srs == "")
        return 0.0;

    OGRSpatialReferenceH hSRS = OSRNewSpatialReference(nullptr);

    if (OSRSetFromUserInput(hSRS, srs.c_str()) != OGRERR_NONE) {
        if (hSRS != nullptr)
            OSRDestroySpatialReference(hSRS);
        Rcpp::stop("error importing SRS from user input");
    }

    double ret = OSRGetCoordinateEpoch(hSRS);
    OSRDestroySpatialReference(hSRS);
    return ret;
#endif
}

//' @rdname srs_query
// [[Rcpp::export]]
int srs_get_utm_zone(const std::string &srs) {
    if (srs == "")
        return 0;

    OGRSpatialReferenceH hSRS = OSRNewSpatialReference(nullptr);

    if (OSRSetFromUserInput(hSRS, srs.c_str()) != OGRERR_NONE) {
        if (hSRS != nullptr)
            OSRDestroySpatialReference(hSRS);
        Rcpp::stop("error importing SRS from user input");
    }

    int bNorth = 0;
    int utm_zone = OSRGetUTMZone(hSRS, &bNorth);
    OSRDestroySpatialReference(hSRS);
    if (bNorth)
        return utm_zone;
    else
        return utm_zone * -1;
}

//' @rdname srs_query
// [[Rcpp::export]]
std::string srs_get_axis_mapping_strategy(const std::string &srs) {
    if (srs == "")
        return "";

    OGRSpatialReferenceH hSRS = OSRNewSpatialReference(nullptr);

    if (OSRSetFromUserInput(hSRS, srs.c_str()) != OGRERR_NONE) {
        if (hSRS != nullptr)
            OSRDestroySpatialReference(hSRS);
        Rcpp::stop("error importing SRS from user input");
    }

    OSRAxisMappingStrategy eOAMS = OSRGetAxisMappingStrategy(hSRS);
    OSRDestroySpatialReference(hSRS);
    if (eOAMS == OAMS_TRADITIONAL_GIS_ORDER)
        return "OAMS_TRADITIONAL_GIS_ORDER";
    else if (eOAMS == OAMS_AUTHORITY_COMPLIANT)
        return "OAMS_AUTHORITY_COMPLIANT";
    else if (eOAMS == OAMS_CUSTOM)
        return "OAMS_CUSTOM";
    else
        return "";
}

//' @rdname srs_query
// [[Rcpp::export]]
SEXP srs_get_area_of_use(const std::string &srs) {
    if (srs == "")
        return R_NilValue;

    OGRSpatialReferenceH hSRS = OSRNewSpatialReference(nullptr);

    if (OSRSetFromUserInput(hSRS, srs.c_str()) != OGRERR_NONE) {
        if (hSRS != nullptr)
            OSRDestroySpatialReference(hSRS);
        Rcpp::stop("error importing SRS from user input");
    }

    double dfWestLongitudeDeg = NA_REAL;
    double dfSouthLatitudeDeg = NA_REAL;
    double dfEastLongitudeDeg = NA_REAL;
    double dfNorthLatitudeDeg = NA_REAL;
    const char *pszAreaName = nullptr;

    if (!OSRGetAreaOfUse(hSRS, &dfWestLongitudeDeg, &dfSouthLatitudeDeg,
                         &dfEastLongitudeDeg, &dfNorthLatitudeDeg,
                         &pszAreaName)) {

        OSRDestroySpatialReference(hSRS);
        Rcpp::Rcout << "OSRGetAreaOfUse() API call did not succeed\n";
        return R_NilValue;
    }

    Rcpp::List list_out = Rcpp::List::create();
    if (pszAreaName)
        list_out.push_back(pszAreaName, "AreaName");
    else
        list_out.push_back("", "AreaName");
    if (dfWestLongitudeDeg != -1000)
        list_out.push_back(dfWestLongitudeDeg, "WestLongitudeDeg");
    else
        list_out.push_back(NA_REAL, "WestLongitudeDeg");
    if (dfSouthLatitudeDeg != -1000)
        list_out.push_back(dfSouthLatitudeDeg, "SouthLatitudeDeg");
    else
        list_out.push_back(NA_REAL, "SouthLatitudeDeg");
    if (dfEastLongitudeDeg != -1000)
        list_out.push_back(dfEastLongitudeDeg, "EastLongitudeDeg");
    else
        list_out.push_back(NA_REAL, "EastLongitudeDeg");
    if (dfNorthLatitudeDeg != -1000)
        list_out.push_back(dfNorthLatitudeDeg, "NorthLatitudeDeg");
    else
        list_out.push_back(NA_REAL, "NorthLatitudeDeg");

    OSRDestroySpatialReference(hSRS);
    return list_out;
}

//' @rdname srs_query
// [[Rcpp::export]]
int srs_get_axes_count(const std::string &srs) {
    if (srs == "")
        return NA_INTEGER;

    OGRSpatialReferenceH hSRS = OSRNewSpatialReference(nullptr);

    if (OSRSetFromUserInput(hSRS, srs.c_str()) != OGRERR_NONE) {
        if (hSRS != nullptr)
            OSRDestroySpatialReference(hSRS);
        Rcpp::stop("error importing SRS from user input");
    }

    int axes_count = OSRGetAxesCount(hSRS);
    OSRDestroySpatialReference(hSRS);
    return axes_count;
}

//' @rdname srs_query
// [[Rcpp::export]]
SEXP srs_get_axes(const std::string &srs,
    const Rcpp::Nullable<Rcpp::CharacterVector> &target_key = R_NilValue) {

    if (srs == "")
        return R_NilValue;

    OGRSpatialReferenceH hSRS = OSRNewSpatialReference(nullptr);

    if (OSRSetFromUserInput(hSRS, srs.c_str()) != OGRERR_NONE) {
        if (hSRS != nullptr)
            OSRDestroySpatialReference(hSRS);
        Rcpp::stop("error importing SRS from user input");
    }

    const char *pszTargetKey = nullptr;
    if (target_key.isNotNull()) {
        Rcpp::CharacterVector target_key_in(target_key);
        if (target_key_in.size() > 0) {
            if (!EQUAL(target_key_in[0], "GEOGCS") &&
                !EQUAL(target_key_in[0], "PROJCS")) {

                OSRDestroySpatialReference(hSRS);
                Rcpp::stop(
                    "'target_key' must be one of \"GEOGCS\" or \"PROJCS\"");
            }
            Rcpp::String s(target_key_in[0]);
            pszTargetKey = s.get_cstring();
        }
    }

    int axes_count = OSRGetAxesCount(hSRS);
    if (axes_count <= 0) {
        OSRDestroySpatialReference(hSRS);
        return R_NilValue;
    }

    Rcpp::List axes_out = Rcpp::List::create();
    bool is_axis_found = false;
    for (int i = 0; i < axes_count; ++i) {
        const char *axis_name = nullptr;
        OGRAxisOrientation orientation = OAO_Other;
        axis_name = OSRGetAxis(hSRS, pszTargetKey, i, &orientation);

        if (!axis_name)
            axis_name = std::to_string(i).c_str();
        else
            is_axis_found = true;

        Rcpp::String axis_name_r(axis_name);
        axis_name_r.replace_all(" ", "_");

        if (orientation == OAO_Other)
            axes_out.push_back("other", axis_name_r);
        else if (orientation == OAO_North)
            axes_out.push_back("north", axis_name_r);
        else if (orientation == OAO_South)
            axes_out.push_back("south", axis_name_r);
        else if (orientation == OAO_East)
            axes_out.push_back("east", axis_name_r);
        else if (orientation == OAO_West)
            axes_out.push_back("west", axis_name_r);
        else if (orientation == OAO_Up)
            axes_out.push_back("up", axis_name_r);
        else if (orientation == OAO_Down)
            axes_out.push_back("down", axis_name_r);
        else
            axes_out.push_back(NA_STRING, axis_name_r);
    }

    OSRDestroySpatialReference(hSRS);

    if (is_axis_found)
        return axes_out;
    else
        return R_NilValue;
}

//' @rdname srs_query
// [[Rcpp::export]]
std::string srs_get_celestial_body_name(const std::string &srs) {
#if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 12, 0)
    Rcpp::stop("srs_get_celestial_body_name() requires GDAL >= 3.12");
#else
    if (srs == "")
        return "";

    std::vector<int> proj_ver = getPROJVersion();
    if (!(proj_ver[0] > 8 || (proj_ver[0] == 8 && proj_ver[1] >= 1)))
        Rcpp::stop("srs_get_celestial_body_name() requires PROJ >= 8.1");

    OGRSpatialReferenceH hSRS = OSRNewSpatialReference(nullptr);

    if (OSRSetFromUserInput(hSRS, srs.c_str()) != OGRERR_NONE) {
        if (hSRS != nullptr)
            OSRDestroySpatialReference(hSRS);
        Rcpp::stop("error importing SRS from user input");
    }

    const char *pszName = OSRGetCelestialBodyName(hSRS);
    std::string ret = "";
    if (pszName)
        ret = pszName;

    OSRDestroySpatialReference(hSRS);
    return ret;
#endif  // GDAL 3.12
}

//' Obtain information about coordinate reference systems in the PROJ DB
//'
//' `srs_info_from_db()` returns a data frame containing descriptive information
//' about spatial coordinate reference systems in the PROJ database. Wrapper of
//' `OSRGetCRSInfoListFromDatabase()` in the GDAL SRS API.
//'
//' @details
//' The returned information includes the authority name, object code, object
//' name, object type (e.g., Geographic 2D CRS, Geographic 3D CRS, Projected
//' CRS, etc.), whether the object is deprecated, whether the bounding box
//' values for the area of use are valid, the bounding box values in longitude
//' and latitude degrees, the name of the area of use, the name of the
//' projection method for a projected CRS, and the name of the celestial body of
//' the CRS (e.g., "Earth", populated only if GDAL >= 3.12 and PROJ >= 8.1).
//'
//' @param auth_name Character string containing an authority name used to
//' restrict the search, or empty string (`""`) for all authorities (the
//' default).
//' @returns
//' A data frame (will contain `0` rows if no objects are found that match the
//' search criterion).
//'
//' @seealso
//' [srs_query]
//'
//' @examples
//' epsg <- srs_info_from_db("EPSG")
//' str(epsg)
//'
//' iau <- srs_info_from_db("IAU_2015")
//' str(iau)
// [[Rcpp::export]]
Rcpp::DataFrame srs_info_from_db(std::string auth_name = "") {
    const char *pszAuthName = nullptr;
    if (auth_name != "")
        pszAuthName = auth_name.c_str();

    int nObjects = 0;
    std::unique_ptr<OSRCRSInfo *, decltype(&OSRDestroyCRSInfoList)>
        pCRSList(OSRGetCRSInfoListFromDatabase(pszAuthName, nullptr, &nObjects),
                 OSRDestroyCRSInfoList);

    Rcpp::CharacterVector auth_name_vec(nObjects, NA_STRING);
    Rcpp::CharacterVector obj_code_vec(nObjects, NA_STRING);
    Rcpp::CharacterVector obj_name_vec(nObjects, NA_STRING);
    Rcpp::CharacterVector obj_type_vec(nObjects, NA_STRING);
    Rcpp::LogicalVector is_deprecated_vec(nObjects, NA_LOGICAL);
    Rcpp::LogicalVector is_bbox_valid_vec(nObjects, NA_LOGICAL);
    Rcpp::NumericVector west_longitude_vec(nObjects, NA_REAL);
    Rcpp::NumericVector south_latitude_vec(nObjects, NA_REAL);
    Rcpp::NumericVector east_longitude_vec(nObjects, NA_REAL);
    Rcpp::NumericVector north_latitude_vec(nObjects, NA_REAL);
    Rcpp::CharacterVector area_name_vec(nObjects, NA_STRING);
    Rcpp::CharacterVector proj_method_vec(nObjects, NA_STRING);
    Rcpp::CharacterVector celestial_body_name_vec(nObjects, NA_STRING);

#if GDAL_VERSION_NUM >= GDAL_COMPUTE_VERSION(3, 12, 0)
    bool is_proj_8_1_min = false;
    std::vector<int> proj_ver = getPROJVersion();
    if (proj_ver[0] > 8 || (proj_ver[0] == 8 && proj_ver[1] >= 1))
        is_proj_8_1_min = true;
#endif  // GDAL >= 3.12

    for (int i = 0; i < nObjects; ++i) {
        const auto *entry = (pCRSList.get())[i];
        if (entry->pszAuthName)
            auth_name_vec[i] = entry->pszAuthName;
        if (entry->pszCode)
            obj_code_vec[i] = entry->pszCode;
        if (entry->pszName)
            obj_name_vec[i] = entry->pszName;

        switch (entry->eType) {
            case OSR_CRS_TYPE_GEOGRAPHIC_2D:
                obj_type_vec[i] = "Geographic 2D";
            break;
            case OSR_CRS_TYPE_GEOGRAPHIC_3D:
                obj_type_vec[i] = "Geographic 3D";
            break;
            case OSR_CRS_TYPE_GEOCENTRIC:
                obj_type_vec[i] = "Geocentric";
            break;
            case OSR_CRS_TYPE_PROJECTED:
                obj_type_vec[i] = "Projected";
            break;
            case OSR_CRS_TYPE_VERTICAL:
                obj_type_vec[i] = "Vertical";
            break;
            case OSR_CRS_TYPE_COMPOUND:
                obj_type_vec[i] = "Compound";
            break;
            case OSR_CRS_TYPE_OTHER:
                obj_type_vec[i] = "Other";
            break;
            default:
                obj_type_vec[i] = "unhandled CRS type";
            break;
        }

        if (entry->bDeprecated)
            is_deprecated_vec[i] = TRUE;
        else
            is_deprecated_vec[i] = FALSE;

        if (entry->bBboxValid)
            is_bbox_valid_vec[i] = TRUE;
        else
            is_bbox_valid_vec[i] = FALSE;

        if (entry->dfWestLongitudeDeg != -1000)
            west_longitude_vec[i] = entry->dfWestLongitudeDeg;
        if (entry->dfSouthLatitudeDeg != -1000)
            south_latitude_vec[i] = entry->dfSouthLatitudeDeg;
        if (entry->dfEastLongitudeDeg != -1000)
            east_longitude_vec[i] = entry->dfEastLongitudeDeg;
        if (entry->dfNorthLatitudeDeg != -1000)
            north_latitude_vec[i] = entry->dfNorthLatitudeDeg;

        if (entry->pszAreaName)
            area_name_vec[i] = entry->pszAreaName;
        if (entry->pszProjectionMethod)
            proj_method_vec[i] = entry->pszProjectionMethod;

#if GDAL_VERSION_NUM >= GDAL_COMPUTE_VERSION(3, 12, 0)
        if (is_proj_8_1_min) {
            if (entry->pszCelestialBodyName)
                celestial_body_name_vec[i] = entry->pszCelestialBodyName;
        }
#endif  // GDAL >= 3.12
    }

    Rcpp::List df(13);
    Rcpp::CharacterVector col_names =
        {"auth_name", "obj_code", "obj_name", "obj_type", "is_deprecated",
         "is_bbox_valid", "west_long_deg", "south_lat_deg", "east_long_deg",
         "north_lat_deg", "area_of_use", "projection_method",
         "celestial_body_name"};

    df[0] = auth_name_vec;
    df[1] = obj_code_vec;
    df[2] = obj_name_vec;
    df[3] = obj_type_vec;
    df[4] = is_deprecated_vec;
    df[5] = is_bbox_valid_vec;
    df[6] = west_longitude_vec;
    df[7] = south_latitude_vec;
    df[8] = east_longitude_vec;
    df[9] = north_latitude_vec;
    df[10] = area_name_vec;
    df[11] = proj_method_vec;
    df[12] = celestial_body_name_vec;

    df.attr("class") = Rcpp::CharacterVector{"data.frame"};
    df.names() = col_names;
    df.attr("row.names") = Rcpp::seq_len(nObjects);
    return df;
}
