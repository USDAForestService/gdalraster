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

Rcpp::String g_wkb2wkt(const Rcpp::RObject &geom, bool as_iso);

Rcpp::CharacterVector g_wkb_list2wkt(const Rcpp::List &geom, bool as_iso);

SEXP g_wkt2wkb(const std::string &geom, bool as_iso,
               const std::string &byte_order);

Rcpp::List g_wkt_vector2wkb(const Rcpp::CharacterVector &geom, bool as_iso,
                            const std::string &byte_order);

Rcpp::RawVector g_create(const std::string &geom_type,
                         const Rcpp::RObject &pts, bool as_iso,
                         const std::string &byte_order);

Rcpp::RawVector g_add_geom(const Rcpp::RawVector &sub_geom,
                           const Rcpp::RawVector &container,
                           bool as_iso, const std::string &byte_order);

Rcpp::LogicalVector g_is_valid(const Rcpp::RObject &geom, bool quiet);
SEXP g_make_valid(const Rcpp::RObject &geom, const std::string &method,
                  bool keep_collapsed, bool as_iso,
                  const std::string &byte_order, bool quiet);

SEXP g_set_3D(const Rcpp::RObject &geom, bool is_3d, bool as_iso,
              const std::string &byte_order, bool quiet);

SEXP g_set_measured(const Rcpp::RObject &geom, bool is_measured, bool as_iso,
                    const std::string &byte_order, bool quiet);

SEXP g_swap_xy(const Rcpp::RObject &geom, bool as_iso,
               const std::string &byte_order, bool quiet);

Rcpp::LogicalVector g_is_empty(const Rcpp::RObject &geom, bool quiet);
Rcpp::LogicalVector g_is_3D(const Rcpp::RObject &geom, bool quiet);
Rcpp::LogicalVector g_is_measured(const Rcpp::RObject &geom, bool quiet);
Rcpp::LogicalVector g_is_ring(const Rcpp::RObject &geom, bool quiet);
Rcpp::String g_name(const Rcpp::RObject &geom, bool quiet);
Rcpp::String g_summary(const Rcpp::RObject &geom, bool quiet);
Rcpp::NumericVector g_envelope(const Rcpp::RObject &geom, bool as_3d,
                               bool quiet);

Rcpp::LogicalVector g_intersects(const Rcpp::RObject &this_geom,
                                 const Rcpp::RObject &other_geom,
                                 bool quiet);

Rcpp::LogicalVector g_equals(const Rcpp::RObject &this_geom,
                             const Rcpp::RObject &other_geom,
                             bool quiet);

Rcpp::LogicalVector g_disjoint(const Rcpp::RObject &this_geom,
                               const Rcpp::RObject &other_geom,
                               bool quiet);

Rcpp::LogicalVector g_touches(const Rcpp::RObject &this_geom,
                              const Rcpp::RObject &other_geom,
                              bool quiet);

Rcpp::LogicalVector g_contains(const Rcpp::RObject &this_geom,
                               const Rcpp::RObject &other_geom,
                               bool quiet);

Rcpp::LogicalVector g_within(const Rcpp::RObject &this_geom,
                             const Rcpp::RObject &other_geom,
                             bool quiet);

Rcpp::LogicalVector g_crosses(const Rcpp::RObject &this_geom,
                              const Rcpp::RObject &other_geom,
                              bool quiet);

Rcpp::LogicalVector g_overlaps(const Rcpp::RObject &this_geom,
                               const Rcpp::RObject &other_geom,
                               bool quiet);

SEXP g_boundary(const Rcpp::RObject &geom, bool as_iso,
                const std::string &byte_order, bool quiet);

SEXP g_buffer(const Rcpp::RObject &geom, double dist, int quad_segs,
              bool as_iso, const std::string &byte_order, bool quiet);

SEXP g_convex_hull(const Rcpp::RObject &geom, bool as_iso,
                   const std::string &byte_order, bool quiet);

SEXP g_delaunay_triangulation(const Rcpp::RObject &geom, double tolerance,
                              bool only_edges, bool as_iso,
                              const std::string &byte_order, bool quiet);

SEXP g_simplify(const Rcpp::RObject &geom, double tolerance,
                bool preserve_topology, bool as_iso,
                const std::string &byte_order, bool quiet);

SEXP g_intersection(const Rcpp::RObject &this_geom,
                    const Rcpp::RObject &other_geom,
                    bool as_iso, const std::string &byte_order,
                    bool quiet);

SEXP g_union(const Rcpp::RObject &this_geom,
             const Rcpp::RObject &other_geom,
             bool as_iso, const std::string &byte_order,
             bool quiet);

SEXP g_difference(const Rcpp::RObject &this_geom,
                  const Rcpp::RObject &other_geom,
                  bool as_iso, const std::string &byte_order,
                  bool quiet);

SEXP g_sym_difference(const Rcpp::RObject &this_geom,
                      const Rcpp::RObject &other_geom,
                      bool as_iso, const std::string &byte_order,
                      bool quiet);

double g_distance(const Rcpp::RObject &this_geom,
                  const Rcpp::RObject &other_geom,
                  bool quiet);

double g_length(const Rcpp::RObject &geom, bool quiet);
double g_area(const Rcpp::RObject &geom, bool quiet);
double g_geodesic_area(const Rcpp::RObject &geom, const std::string &srs,
                       bool traditional_gis_order, bool quiet);
double g_geodesic_length(const Rcpp::RObject &geom, const std::string &srs,
                         bool traditional_gis_order, bool quiet);
Rcpp::NumericVector g_centroid(const Rcpp::RObject &geom, bool quiet);

SEXP g_transform(const Rcpp::RObject &geom, const std::string &srs_from,
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
