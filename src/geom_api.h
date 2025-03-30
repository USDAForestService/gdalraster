/* GEOS wrapper functions operating on WKT geometries
   Called via GDAL ogr headers, requires GDAL built against GEOS.
   For spatial predicate definitions: https://en.wikipedia.org/wiki/DE-9IM

   Chris Toney <chris.toney at usda.gov>
   Copyright (c) 2023-2025 gdalraster authors
*/

#ifndef SRC_GEOM_API_H_
#define SRC_GEOM_API_H_

#include <string>
#include <vector>

#include <Rcpp.h>

#include "ogr_geometry.h"


std::vector<int> getGEOSVersion();
bool has_geos();  // GDAL built against GEOS is required at gdalraster 1.10

OGRGeometryH createGeomFromWkb(const Rcpp::RawVector &wkb);
bool exportGeomToWkb(OGRGeometryH hGeom, unsigned char *wkb, bool as_iso,
                     const std::string &byte_order);

std::string g_wkb2wkt(const Rcpp::RawVector &geom, bool as_iso);

Rcpp::CharacterVector g_wkb_list2wkt(const Rcpp::List &geom, bool as_iso);

Rcpp::RawVector g_wkt2wkb(const std::string &geom, bool as_iso,
                          const std::string &byte_order);

Rcpp::List g_wkt_vector2wkb(const Rcpp::CharacterVector &geom, bool as_iso,
                            const std::string &byte_order);

Rcpp::RawVector g_create(const std::string &geom_type,
                         const Rcpp::RObject &pts, bool as_iso,
                         const std::string &byte_order);

Rcpp::RawVector g_add_geom(const Rcpp::RawVector &sub_geom,
                           const Rcpp::RawVector &container,
                           bool as_iso, const std::string &byte_order);

Rcpp::LogicalVector g_is_valid(const Rcpp::RawVector &geom, bool quiet);
SEXP g_make_valid(const Rcpp::RawVector &geom, const std::string &method,
                  bool keep_collapsed, bool as_iso,
                  const std::string &byte_order, bool quiet);

SEXP g_swap_xy(const Rcpp::RawVector &geom, bool as_iso,
               const std::string &byte_order, bool quiet);

Rcpp::LogicalVector g_is_empty(const Rcpp::RawVector &geom, bool quiet);
Rcpp::LogicalVector g_is_3D(const Rcpp::RawVector &geom, bool quiet);
Rcpp::LogicalVector g_is_measured(const Rcpp::RawVector &geom, bool quiet);
SEXP g_name(const Rcpp::RawVector &geom, bool quiet);
SEXP g_summary(const Rcpp::RawVector &geom, bool quiet);
Rcpp::NumericVector g_envelope(const Rcpp::RawVector &geom,
                               bool quiet);

Rcpp::LogicalVector g_intersects(const Rcpp::RawVector &this_geom,
                                 const Rcpp::RawVector &other_geom,
                                 bool quiet);

Rcpp::LogicalVector g_equals(const Rcpp::RawVector &this_geom,
                             const Rcpp::RawVector &other_geom,
                             bool quiet);

Rcpp::LogicalVector g_disjoint(const Rcpp::RawVector &this_geom,
                               const Rcpp::RawVector &other_geom,
                               bool quiet);

Rcpp::LogicalVector g_touches(const Rcpp::RawVector &this_geom,
                              const Rcpp::RawVector &other_geom,
                              bool quiet);

Rcpp::LogicalVector g_contains(const Rcpp::RawVector &this_geom,
                               const Rcpp::RawVector &other_geom,
                               bool quiet);

Rcpp::LogicalVector g_within(const Rcpp::RawVector &this_geom,
                             const Rcpp::RawVector &other_geom,
                             bool quiet);

Rcpp::LogicalVector g_crosses(const Rcpp::RawVector &this_geom,
                              const Rcpp::RawVector &other_geom,
                              bool quiet);

Rcpp::LogicalVector g_overlaps(const Rcpp::RawVector &this_geom,
                               const Rcpp::RawVector &other_geom,
                               bool quiet);

SEXP g_buffer(const Rcpp::RawVector &geom, double dist, int quad_segs,
              bool as_iso, const std::string &byte_order, bool quiet);

SEXP g_simplify(const Rcpp::RawVector &geom, double tolerance,
                bool preserve_topology, bool as_iso,
                const std::string &byte_order, bool quiet);

SEXP g_intersection(const Rcpp::RawVector &this_geom,
                    const Rcpp::RawVector &other_geom,
                    bool as_iso, const std::string &byte_order,
                    bool quiet);

SEXP g_union(const Rcpp::RawVector &this_geom,
             const Rcpp::RawVector &other_geom,
             bool as_iso, const std::string &byte_order,
             bool quiet);

SEXP g_difference(const Rcpp::RawVector &this_geom,
                  const Rcpp::RawVector &other_geom,
                  bool as_iso, const std::string &byte_order,
                  bool quiet);

SEXP g_sym_difference(const Rcpp::RawVector &this_geom,
                      const Rcpp::RawVector &other_geom,
                      bool as_iso, const std::string &byte_order,
                      bool quiet);

double g_distance(const Rcpp::RawVector &this_geom,
                  const Rcpp::RawVector &other_geom,
                  bool quiet);

double g_length(const Rcpp::RawVector &geom, bool quiet);
double g_area(const Rcpp::RawVector &geom, bool quiet);
double g_geodesic_area(const Rcpp::RawVector &geom, const std::string &srs,
                       bool traditional_gis_order, bool quiet);
double g_geodesic_length(const Rcpp::RawVector &geom, const std::string &srs,
                         bool traditional_gis_order, bool quiet);
Rcpp::NumericVector g_centroid(const Rcpp::RawVector &geom, bool quiet);

SEXP g_transform(const Rcpp::RawVector &geom, const std::string &srs_from,
                 const std::string &srs_to, bool wrap_date_line,
                 int date_line_offset, bool traditional_gis_order, bool as_iso,
                 const std::string &byte_order, bool quiet);

Rcpp::NumericVector bbox_from_wkt(const std::string &wkt,
                                  double extend_x, double extend_y);

Rcpp::String bbox_to_wkt(const Rcpp::NumericVector &bbox,
                         double extend_x, double extend_y);

OGRwkbGeometryType getTargetGeomType(OGRwkbGeometryType geom_type,
                                     bool convert_to_linear,
                                     bool promote_to_multi);

#endif  // SRC_GEOM_API_H_
