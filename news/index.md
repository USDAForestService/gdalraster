# Changelog

## gdalraster 2.3.0.9001 (dev)

- add to the GDAL Spatial Reference System API bindings:
  [`srs_get_area_of_use()`](https://usdaforestservice.github.io/gdalraster/reference/srs_query.md),
  [`srs_get_axes_count()`](https://usdaforestservice.github.io/gdalraster/reference/srs_query.md)
  and
  [`srs_get_celestial_body_name()`](https://usdaforestservice.github.io/gdalraster/reference/srs_query.md)
  ([\#831](https://github.com/USDAForestService/gdalraster/issues/831))
  (2025-11-28)

- [`plot_raster()`](https://usdaforestservice.github.io/gdalraster/reference/plot_raster.md):
  accept a color palette for the argument `col_map_fn`, to support an
  implied ramp function
  ([\#830](https://github.com/USDAForestService/gdalraster/issues/830))
  (2025-11-20)

## gdalraster 2.3.0

CRAN release: 2025-11-19

### Dependencies

- package **yyjsonr** has been added to Imports

### Features / enhancements

#### GDAL CLI bindings

- [`gdal_usage()`](https://usdaforestservice.github.io/gdalraster/reference/gdal_cli.md):
  include the list of potential step algorithms for `"raster pipeline"`
  and `"vector pipeline"`
  ([\#789](https://github.com/USDAForestService/gdalraster/issues/789))
- [`gdal_run()`](https://usdaforestservice.github.io/gdalraster/reference/gdal_cli.md):
  add argument `setVectorArgsFromObject`
  ([\#790](https://github.com/USDAForestService/gdalraster/issues/790))
- add `GDALAlg::getExplicitlySetArgs()`: exposed class method to return
  a named list of arguments that have been set explicitly along with
  their values
  ([\#807](https://github.com/USDAForestService/gdalraster/issues/807))
- add `GDALAlg::setArg()`: exposed class method to set algorithm
  argument values individually by API
  ([\#822](https://github.com/USDAForestService/gdalraster/issues/822))

#### Raster

- class `GDALRaster`: add helper methods for iterating I/O on raster
  blocks, or on multi-block chunks of user-defined size, `readBlock()` /
  `writeBlock()`,
  [`make_chunk_index()`](https://usdaforestservice.github.io/gdalraster/reference/make_chunk_index.md)
  and `readChunk()` / `writeChunk()`
  ([\#819](https://github.com/USDAForestService/gdalraster/issues/819))
- add
  [`make_chunk_index()`](https://usdaforestservice.github.io/gdalraster/reference/make_chunk_index.md)
  stand-alone function for use without a dataset object
- add `GDALRaster::getMinMaxLocation()`: compute the min/max values for
  a raster band and their locations, using
  `GDALComputeRasterMinMaxLocation()` in GDAL \>= 3.11
  ([\#820](https://github.com/USDAForestService/gdalraster/issues/820))
- [`rasterToVRT()`](https://usdaforestservice.github.io/gdalraster/reference/rasterToVRT.md):
  add argument `krnl_fn` supporting a Function element in
  KernelFilteredSource with GDAL \>= 3.12, one of `"min"`, `"max"`,
  `"stddev"`, `"median"` or `"mode"`
  ([\#821](https://github.com/USDAForestService/gdalraster/issues/821))
- class `GDALRaster`: add a constructor with `allowed_drivers` argument
  ([\#793](https://github.com/USDAForestService/gdalraster/issues/793))

#### Multidimensional raster

- add
  [`mdim_as_classic()`](https://usdaforestservice.github.io/gdalraster/reference/mdim_as_classic.md):
  return a view of an MDArray as a “classic” GDALDataset (i.e., 2D), and
  add column `multidim_raster` (`TRUE`\|`FALSE`) in the data frame
  output of
  [`gdal_formats()`](https://usdaforestservice.github.io/gdalraster/reference/gdal_formats.md)
  ([\#799](https://github.com/USDAForestService/gdalraster/issues/799))
- add
  [`mdim_info()`](https://usdaforestservice.github.io/gdalraster/reference/mdim_info.md):
  report structure and content of a multidimensional dataset, interface
  to `gdalmdiminfo` utility
- add
  [`mdim_translate()`](https://usdaforestservice.github.io/gdalraster/reference/mdim_translate.md):
  convert multidimensional data between different formats, and subset,
  interface to `gdalmdimtranslate` utility
  ([\#803](https://github.com/USDAForestService/gdalraster/issues/803))

#### Geometry API

- add
  [`g_geom_count()`](https://usdaforestservice.github.io/gdalraster/reference/g_query.md):
  return the number of elements in a geometry or number of geometries in
  container
- add
  [`g_get_geom()`](https://usdaforestservice.github.io/gdalraster/reference/g_factory.md):
  fetch a geometry from a geometry container
- add
  [`g_unary_union()`](https://usdaforestservice.github.io/gdalraster/reference/g_unary_op.md):
  wrapper for `OGR_G_UnaryUnion()`
- add
  [`g_concave_hull()`](https://usdaforestservice.github.io/gdalraster/reference/g_unary_op.md):
  wrapper for `OGR_G_ConcaveHull()`
- add
  [`g_normalize()`](https://usdaforestservice.github.io/gdalraster/reference/g_util.md):
  wrapper for `OGR_G_Normalize()`
- [`g_delaunay_triangulation()`](https://usdaforestservice.github.io/gdalraster/reference/g_unary_op.md):
  add argument `constrained`, wrapper for
  `OGR_G_ConstrainedDelaunayTriangulation()`
- add
  [`plot_geom()`](https://usdaforestservice.github.io/gdalraster/reference/plot_geom.md):
  plot WKT or WKB geometries

#### Other features

- add
  [`gdal_get_driver_md()`](https://usdaforestservice.github.io/gdalraster/reference/gdal_get_driver_md.md):
  retrieve driver metadata
  ([\#795](https://github.com/USDAForestService/gdalraster/issues/795))
- add
  [`vsi_stat_exists()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_stat.md),
  [`vsi_stat_type()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_stat.md)
  and
  [`vsi_stat_size()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_stat.md):
  specializations of
  [`vsi_stat()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_stat.md)
  operating on a vector of potentially multiple file system object names
  ([\#825](https://github.com/USDAForestService/gdalraster/issues/825))

### Bug fix

- fixes for
  [`ogr_ds_delete_field_domain()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_manage.md),
  and condition tests on GDAL 3.12
  ([\#823](https://github.com/USDAForestService/gdalraster/issues/823))
- ensure that the global error handler is restored when the input
  dataset cannot be opened in some OGR management functions
  ([\#787](https://github.com/USDAForestService/gdalraster/issues/787))

### Internal

- various C++ code linting, small refactorings and performance
  improvements
  ([\#788](https://github.com/USDAForestService/gdalraster/issues/788),
  [\#801](https://github.com/USDAForestService/gdalraster/issues/801),
  [\#818](https://github.com/USDAForestService/gdalraster/issues/818))

## gdalraster 2.2.1

CRAN release: 2025-08-29

### Bug fix and maintenance release

- updates in class `GDALAlg`, experimental bindings to the new GDAL CLI
  API
  ([\#782](https://github.com/USDAForestService/gdalraster/issues/782))
  - improve input validation
  - ensure algorithm names originating in `Rcpp::CharacterVector` are
    always passed explicitly as `const char *` in C API calls
  - fix clang compiler warning from `-Wunused-const-variable`
- add
  [`gdal_global_reg_names()`](https://usdaforestservice.github.io/gdalraster/reference/gdal_cli.md):
  return a character vector containing the names of the algorithms in
  the GDAL global algorithm registry
  ([\#782](https://github.com/USDAForestService/gdalraster/issues/782))
- handle GDAL CPL error messages via **Rcpp**, and suppress driver
  warnings during package initialization
  ([\#784](https://github.com/USDAForestService/gdalraster/issues/784))
- [`plot_raster()`](https://usdaforestservice.github.io/gdalraster/reference/plot_raster.md):
  use [`on.exit()`](https://rdrr.io/r/base/on.exit.html) for
  [`par()`](https://rdrr.io/r/graphics/par.html) reset
  ([\#780](https://github.com/USDAForestService/gdalraster/issues/780))
- fixes in several unit tests, mainly fixing
  [`on.exit()`](https://rdrr.io/r/base/on.exit.html) cleanup code
  ([\#783](https://github.com/USDAForestService/gdalraster/issues/783))
- Documentation
  - update text in the “Development Status” section describing the
    experimental GDAL CLI bindings
  - condition the example code for the CLI bindings on the availability
    of top-level algorithm names from the global algorithm registry (for
    now, until the upstream API is declared stable)
  - remove some non-ASCII characters found in .Rd files and other
    miscellaneous improvements in .Rd formatting

## gdalraster 2.2.0

CRAN release: 2025-08-22

### Features

- initial bindings to the “gdal” command line interface (CLI) added in
  GDAL 3.11
  - [functions](https://usdaforestservice.github.io/gdalraster/reference/gdal_cli.html)
    for using “gdal” CLI algorithms:
    [`gdal_commands()`](https://usdaforestservice.github.io/gdalraster/reference/gdal_cli.md),
    [`gdal_usage()`](https://usdaforestservice.github.io/gdalraster/reference/gdal_cli.md),
    [`gdal_run()`](https://usdaforestservice.github.io/gdalraster/reference/gdal_cli.md),
    [`gdal_alg()`](https://usdaforestservice.github.io/gdalraster/reference/gdal_cli.md)
  - bindings to the GDALAlgorithm API implemented in class
    [`GDALAlg`](https://usdaforestservice.github.io/gdalraster/reference/GDALAlg-class.html)
  - requires **gdalraster** built against GDAL \>= 3.11.3

### Other updates and fixes

- [`pixel_extract()`](https://usdaforestservice.github.io/gdalraster/reference/pixel_extract.md):
  add checks for potential ctrl-c user interrupt during extract for a
  large number of points
- allow reporting a raster driver name as empty string for a dataset
  that has no driver
  ([\#761](https://github.com/USDAForestService/gdalraster/issues/761),
  thanks to [@mdsumner](https://github.com/mdsumner))
- changes to address crash in
  [`ogr_reproject()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_reproject.md)
  following GDAL `ERROR 1: Reprojection failed` on certain CRAN
  platforms with alternative BLAS/LAPACK implementations
  - disable for now the Arrow code path for GDAL vector translate in the
    examples and tests for
    [`ogr_reproject()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_reproject.md)
    (cf. <https://gdal.org/en/stable/programs/ogr2ogr.html#known-issues>)
  - retain basic tests of
    [`ogr_reproject()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_reproject.md)
    across all GDAL versions but skip more complex tests if GDAL \<
    3.11.3 (“stable” version as of July 2025)
  - move [*Vector API
    Overview*](https://usdaforestservice.github.io/gdalraster/articles/vector-api-overview.html)
    from vignette to a web article

### Documentation

- add conda-forge installation instructions in the README and web docs
  ([\#762](https://github.com/USDAForestService/gdalraster/issues/762),
  thanks to [@matthewfeickert](https://github.com/matthewfeickert))
- update the sample data elevation raster (inst/extdata/storml_elev.tif)
  to avoid visual artifacts
  ([\#768](https://github.com/USDAForestService/gdalraster/issues/768))
- add web article [*Using “gdal” CLI algorithms from
  R*](https://usdaforestservice.github.io/gdalraster/articles/use-gdal-cli-from-r.html)

### Internal

- avoid clang warning from `-Wimplicit-const-int-float-conversion` in
  src/vsifile.cpp
- method `setGDALDatasetH_()` in classes `GDALRaster` and `GDALVector`:
  check shared flag of the incoming `GDALDatasetH` and set the
  `m_shared` member variable
- class `GDALRaster`: call `GDALReleaseDataset()` instead of
  `GDALClose()` in the
  [`close()`](https://rdrr.io/r/base/connections.html) method and
  destructor when the dataset is non-shared
- skip tests that require PostGIS drivers if they are not available
- code linting

## gdalraster 2.1.0

CRAN release: 2025-07-14

### Features / enhancements

#### Raster

- [`pixel_extract()`](https://usdaforestservice.github.io/gdalraster/reference/pixel_extract.md):
  support optional point IDs in the first column of an input data frame
  and add argument `as_data_frame`
  ([\#751](https://github.com/USDAForestService/gdalraster/issues/751))
- performance improvement for
  [`plot_raster()`](https://usdaforestservice.github.io/gdalraster/reference/plot_raster.md)
  via `col_tbl`
  ([\#735](https://github.com/USDAForestService/gdalraster/issues/735)
  thanks [@mdsumner](https://github.com/mdsumner))

#### Vector

- add write support for field domains
  ([\#738](https://github.com/USDAForestService/gdalraster/issues/738),
  [\#739](https://github.com/USDAForestService/gdalraster/issues/739))
- `GDALVector$getFieldDomain()`: a coded values domain is now returned
  as a two-column data frame of (codes, values) instead of character
  vector of `"CODE=VALUE"`, **a breaking change** but the return value
  was not previously documented. The returned field domain definition
  now aligns with the specification added under the `ogr_define` help
  topic in
  [\#738](https://github.com/USDAForestService/gdalraster/issues/738)

#### Geometry API

- add
  [`g_boundary()`](https://usdaforestservice.github.io/gdalraster/reference/g_unary_op.md):
  compute the boundary of input geometry, wrapper of `OGR_G_Boundary()`
  ([\#727](https://github.com/USDAForestService/gdalraster/issues/727))
- add
  [`g_convex_hull()`](https://usdaforestservice.github.io/gdalraster/reference/g_unary_op.md):
  compute convex hull for input geometries, wrapper of
  `OGR_G_ConvexHull()`
  ([\#726](https://github.com/USDAForestService/gdalraster/issues/726))
- add
  [`g_delaunay_triangulation()`](https://usdaforestservice.github.io/gdalraster/reference/g_unary_op.md):
  return a Delaunay triangulation of the vertices of the input geometry,
  wrapper of `OGR_G_DelaunayTriangulation()`
  ([\#729](https://github.com/USDAForestService/gdalraster/issues/729))
- add
  [`g_set_3D()`](https://usdaforestservice.github.io/gdalraster/reference/g_util.md)/[`g_set_measured()`](https://usdaforestservice.github.io/gdalraster/reference/g_util.md):
  add/remove the explicit Z/M coordinate dimension on the input
  geometries, wrappers of `OGR_G_Set3D()`/`OGR_G_SetMeasured()`
  ([\#743](https://github.com/USDAForestService/gdalraster/issues/743))
- add
  [`g_is_ring()`](https://usdaforestservice.github.io/gdalraster/reference/g_query.md):
  tests whether a geometry is a ring, `TRUE` if the coordinates of the
  geometry form a ring by checking length and closure (self-intersection
  is not checked), otherwise `FALSE` (`OGR_G_IsRing()`)
- support one-to-many input (this-to-others) in binary predicate
  functions and in
  [`g_distance()`](https://usdaforestservice.github.io/gdalraster/reference/g_measures.md)
  ([\#728](https://github.com/USDAForestService/gdalraster/issues/728))
- [`g_transform()`](https://usdaforestservice.github.io/gdalraster/reference/g_transform.md):
  performance improvement when multiple input geometries
  ([\#745](https://github.com/USDAForestService/gdalraster/issues/745))
- [`g_envelope()`](https://usdaforestservice.github.io/gdalraster/reference/g_envelope.md):
  add argument `as_3d`, wrapper of `OGR_G_GetEnvelope3D()` in the GDAL
  Geometry API
  ([\#724](https://github.com/USDAForestService/gdalraster/issues/724))
- fixes and improvements in handling of NULL geometries
  ([\#732](https://github.com/USDAForestService/gdalraster/issues/732))
- minor fixes and improvements to input validation and error handling
  ([\#737](https://github.com/USDAForestService/gdalraster/issues/737))

#### Spatial Reference System API

- add
  [`srs_to_projjson()`](https://usdaforestservice.github.io/gdalraster/reference/srs_convert.md),
  wrapper for `OSRExportToPROJJSON()` in the GDAL Spatial Reference
  System API
  ([\#721](https://github.com/USDAForestService/gdalraster/issues/721))
- [`srs_to_wkt()`](https://usdaforestservice.github.io/gdalraster/reference/srs_convert.md):
  add argument `gcs_only`, wrapper for `OSRCloneGeogCS()` in the GDAL
  SRS API

#### Coordinate transformation

- [`transform_bounds()`](https://usdaforestservice.github.io/gdalraster/reference/transform_bounds.md):
  support multiple input bounding boxes given as rows of a matrix or
  data frame
  ([\#744](https://github.com/USDAForestService/gdalraster/issues/744))

### Bug fixes

- [`g_envelope()`](https://usdaforestservice.github.io/gdalraster/reference/g_envelope.md):
  fix the order of xy coordinates in the returned envelope(s), fixes
  [\#725](https://github.com/USDAForestService/gdalraster/issues/725)
  **a breaking change**
- [`pixel_extract()`](https://usdaforestservice.github.io/gdalraster/reference/pixel_extract.md):
  fix input validation for `xy_srs`

### Internal

- improve input validation for the `ogr_manage` functions
  ([\#749](https://github.com/USDAForestService/gdalraster/issues/749))
- add tests in `tests/testthat/test-gdal_create.R` for the the
  [`create()`](https://usdaforestservice.github.io/gdalraster/reference/create.md)
  and
  [`createCopy()`](https://usdaforestservice.github.io/gdalraster/reference/createCopy.md)
  R public interfaces, and improve input validation
  ([\#748](https://github.com/USDAForestService/gdalraster/issues/748))
- additional tests for class `GDALVector` with a few minor code
  improvements
  ([\#747](https://github.com/USDAForestService/gdalraster/issues/747))
- class `GDALVector`: improve error handling in
  `setIgnoredFields()`/`setSelectedFields()` and add several new tests
  ([\#731](https://github.com/USDAForestService/gdalraster/issues/731))
- make variables `constexpr` or `const` in several places

### Documentation

- add web article [Vector Read
  Benchmarks](https://usdaforestservice.github.io/gdalraster/articles/vector-read-benchmarks.html)

## gdalraster 2.0.0

CRAN release: 2025-05-11

### Summary of main changes

- add bindings to the GDAL Vector API, implemented in the exposed C++
  class
  [`GDALVector`](https://usdaforestservice.github.io/gdalraster/reference/GDALVector-class.html)
  along with several additions and enhancements to [`ogr_*()`
  stand-alone
  functions](https://usdaforestservice.github.io/gdalraster/reference/index.html#ogr-vector-utilities)
- enhance and expand existing [bindings to the Geometry
  API](https://usdaforestservice.github.io/gdalraster/reference/index.html#geometry),
  now operating on raw vectors of WKB or WKT strings
- enhance and expand existing [bindings to the Spatial Reference Systems
  API](https://usdaforestservice.github.io/gdalraster/reference/index.html#spatial-reference-systems)
- add several new features and improvements to the Raster API, Virtual
  Systems Interface (VSI) API, and coordinate transformation functions
- add helper functions for working with GDAL raster data types (see
  [`?data_type_helpers`](https://usdaforestservice.github.io/gdalraster/reference/data_type_helpers.md))
- add an S4 `show()` method for all **Rcpp** exposed classes
- bug fixes, code linting and other internal improvements

### Dependencies

- package **nanoarrow** has been added to Imports and LinkingTo
- package **wk** has been added to Imports
- C++17 is now a System Requirement
- PROJ is removed as a stand-alone external System Requirement since:
  - **gdalraster** as of 1.12 requires GDAL \>= 3.1
  - PROJ has been a GDAL build requirement since GDAL 3.0
  - **gdalraster** only uses PROJ via GDAL headers (i.e., there is no
    longer a requirement for linking to external libproj in this case)

### Vector API bindings

- add class
  [`GDALVector`](https://usdaforestservice.github.io/gdalraster/reference/GDALVector-class.html):
  encapsulates an `OGRLayer` object and the `GDALDataset` that contains
  it, exposing 48 class methods for obtaining vector layer information,
  attribute and spatial filtering, and reading/writing feature data
- wrap GDAL’s Arrow C Stream interface for reading vector data by
  exposing an ArrowArrayStream on a layer as a `nanoarrow_array_stream`
  object (GDAL \>= 3.6)
  ([\#591](https://github.com/USDAForestService/gdalraster/issues/591))
- add
  [`ogr_reproject()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_reproject.md):
  reproject a vector layer (purpose-built wrapper of
  `GDALVectorTranslate()`)
- add
  [`ogr_proc()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_proc.md):
  interface to GDAL OGR facilities for vector geoprocessing
- add
  [`ogr_layer_rename()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_manage.md):
  rename an existing layer in a vector dataset (GDAL \>= 3.5)
- [`ogr_execute_sql()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_manage.md)
  now returns an object of class `GDALVector` for SQL SELECT statements
- add argument `return_obj` in
  [`ogr_ds_create()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_manage.md)
  and
  [`ogr_layer_create()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_manage.md),
  `TRUE` to return a `GDALVector` object for write access on the created
  layer
- the `layer` argument in certain `ogr_manage` functions now supports
  `NULL` or empty string, which will default to the first layer by index
  (mainly as a convenience for single-layer formats)
- remove element `$is_ignored` from the list object for a feature class
  definition (affects certain `ogr_define` and `ogr_manage` functions):
  potentially breaking change but assumed to be unused in layer / field
  creation
  ([\#513](https://github.com/USDAForestService/gdalraster/issues/513))

### Geometry API

- update the geometry interface to support the vector API in gdalraster
  2.0, with changes to existing functions, and add
  [`g_wk2wk()`](https://usdaforestservice.github.io/gdalraster/reference/g_wk2wk.md)
  (WKB \<–\> WKT conversion),
  [`g_create()`](https://usdaforestservice.github.io/gdalraster/reference/g_factory.md)
  (create geometries from input points),
  [`g_add_geom()`](https://usdaforestservice.github.io/gdalraster/reference/g_factory.md)
  (build container geometry types from sub-geometries)
  ([\#572](https://github.com/USDAForestService/gdalraster/issues/572))
- the geometry wrapper functions now operate on input of either WKB raw
  vector, list of WKB, or character vector of WKT strings, with returned
  geometries as either WKB raw vector (or list of) or WKT character
  string (vector of)
- **breaking changes**: breaks some existing functions by renaming
  argument `wkt` to `geom`, and by returning raw vectors of WKB by
  default from functions that return geometries (`as_wkb = TRUE` by
  default)
- update
  [`g_is_valid()`](https://usdaforestservice.github.io/gdalraster/reference/g_query.md),
  [`g_is_empty()`](https://usdaforestservice.github.io/gdalraster/reference/g_query.md),
  [`g_name()`](https://usdaforestservice.github.io/gdalraster/reference/g_query.md)
  and
  [`g_buffer()`](https://usdaforestservice.github.io/gdalraster/reference/g_unary_op.md)
  to the new style Geometry API
- add
  [`g_make_valid()`](https://usdaforestservice.github.io/gdalraster/reference/g_util.md):
  attempt to make invalid geometries valid
- add
  [`g_summary()`](https://usdaforestservice.github.io/gdalraster/reference/g_query.md):
  obtain text summaries of WKB/WKT geometries (GDAL \>= 3.7)
- add
  [`g_simplify()`](https://usdaforestservice.github.io/gdalraster/reference/g_unary_op.md):
  simplify WKB/WKT geometries, optionally preserving topology
- add
  [`g_swap_xy()`](https://usdaforestservice.github.io/gdalraster/reference/g_util.md):
  swap x and y coordinates of the input geometry
- add
  [`g_geodesic_area()`](https://usdaforestservice.github.io/gdalraster/reference/g_measures.md)
  and
  [`g_geodesic_length()`](https://usdaforestservice.github.io/gdalraster/reference/g_measures.md)
  ([\#671](https://github.com/USDAForestService/gdalraster/issues/671))
- add
  [`g_is_3D()`](https://usdaforestservice.github.io/gdalraster/reference/g_query.md)
  and
  [`g_is_measured()`](https://usdaforestservice.github.io/gdalraster/reference/g_query.md)
  ([\#650](https://github.com/USDAForestService/gdalraster/issues/650))
- add
  [`g_envelope()`](https://usdaforestservice.github.io/gdalraster/reference/g_envelope.md):
  computes and returns the bounding envelope(s) for input geometries
- add
  [`g_coords()`](https://usdaforestservice.github.io/gdalraster/reference/g_coords.md):
  extract coordinate values from geometries
- [`g_transform()`](https://usdaforestservice.github.io/gdalraster/reference/g_transform.md):
  add argument `traditional_gis_order` with default `TRUE`, to support
  authority compliant order for geometries using geographic coordinates
- [`bbox_transform()`](https://usdaforestservice.github.io/gdalraster/reference/bbox_transform.md):
  add argument `use_transform_bounds` (`TRUE` by default but requires
  GDAL \>= 3.4)

### Spatial Reference Systems API

- improve and expand bindings to a subset of the GDAL Spatial Reference
  System API
  ([\#584](https://github.com/USDAForestService/gdalraster/issues/584))
- **breaking change** in
  [`srs_is_same()`](https://usdaforestservice.github.io/gdalraster/reference/srs_query.md):
  argument `srs1` is renamed to `srs` and argument `srs2` to `srs_other`
- functions that previously required input as a WKT string now accept
  SRS definitions in any of the formats supported by
  [`srs_to_wkt()`](https://usdaforestservice.github.io/gdalraster/reference/srs_convert.md)
  (wrapper of `OSRSetFromUserInput()`)
- the documentation is grouped under two topics,
  [`?srs_convert`](https://usdaforestservice.github.io/gdalraster/reference/srs_convert.md)
  and
  [`?srs_query`](https://usdaforestservice.github.io/gdalraster/reference/srs_query.md),
  and now includes the following:
  - **`srs_convert`** functions:
    [`epsg_to_wkt()`](https://usdaforestservice.github.io/gdalraster/reference/srs_convert.md),
    [`srs_to_wkt()`](https://usdaforestservice.github.io/gdalraster/reference/srs_convert.md)
  - **`srs_query`** functions:
    [`srs_find_epsg()`](https://usdaforestservice.github.io/gdalraster/reference/srs_query.md),
    [`srs_get_name()`](https://usdaforestservice.github.io/gdalraster/reference/srs_query.md),
    [`srs_is_geographic()`](https://usdaforestservice.github.io/gdalraster/reference/srs_query.md),
    [`srs_is_derived_gcs()`](https://usdaforestservice.github.io/gdalraster/reference/srs_query.md),
    [`srs_is_local()`](https://usdaforestservice.github.io/gdalraster/reference/srs_query.md),
    [`srs_is_projected()`](https://usdaforestservice.github.io/gdalraster/reference/srs_query.md),
    [`srs_is_compound()`](https://usdaforestservice.github.io/gdalraster/reference/srs_query.md),
    [`srs_is_geocentric()`](https://usdaforestservice.github.io/gdalraster/reference/srs_query.md),
    [`srs_is_vertical()`](https://usdaforestservice.github.io/gdalraster/reference/srs_query.md),
    [`srs_is_dynamic()`](https://usdaforestservice.github.io/gdalraster/reference/srs_query.md),
    [`srs_is_same()`](https://usdaforestservice.github.io/gdalraster/reference/srs_query.md),
    [`srs_get_angular_units()`](https://usdaforestservice.github.io/gdalraster/reference/srs_query.md),
    [`srs_get_linear_units()`](https://usdaforestservice.github.io/gdalraster/reference/srs_query.md),
    [`srs_get_coord_epoch()`](https://usdaforestservice.github.io/gdalraster/reference/srs_query.md),
    [`srs_get_utm_zone()`](https://usdaforestservice.github.io/gdalraster/reference/srs_query.md),
    [`srs_get_axis_mapping_strategy()`](https://usdaforestservice.github.io/gdalraster/reference/srs_query.md)

### GDALRaster-class

- add method `$get_block_indexing()`: helper to get indexing values for
  the block layout of a raster band
  ([\#667](https://github.com/USDAForestService/gdalraster/issues/667))
- add method `$addBand()`: add a band to a dataset if the underlying
  format supports this action, e.g., `MEM` and `VRT`
- add method `$clearColorTable()`: clear the color table associated with
  a raster band
- add method `$getMaskBand()`: return the mask filename and band number
  associated with a given band of the dataset
- add method `$getMaskFlags()`: return the status flags of the mask band
  associated with a given band of the dataset
- add method `$setMetadata()`: set dataset or band-level metadata from a
  character vector of `NAME=VALUE` pairs (instead of per metadata item
  with the existing method `$setMetadataItem()`)
- support `band = 0` in the existing method `$setDescription()` to set
  the dataset-level description
- improve methods `$bbox()` and `$res()` to handle south-up / rotated
  raster (with related fixes in
  [`read_ds()`](https://usdaforestservice.github.io/gdalraster/reference/read_ds.md)
  and
  [`plot_raster()`](https://usdaforestservice.github.io/gdalraster/reference/plot_raster.md))
  ([\#569](https://github.com/USDAForestService/gdalraster/issues/569))

### GDAL VSI for operations on virtual file systems

- add
  [`vsi_get_actual_url()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_get_actual_url.md):
  return the actual URL of a supplied VSI filename, wrapper of
  `VSIGetActualURL()` in the GDAL API
- add
  [`vsi_get_signed_url()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_get_signed_url.md):
  return a signed URL for a supplied VSI filename, wrapper of
  `VSIGetSignedURL()` in the GDAL API
- add
  [`vsi_is_local()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_is_local.md):
  returns whether the file/filesystem is “local”, wrapper of
  `VSIIsLocal()` in the GDAL API

### GDAL configuration

- add
  [`get_cache_max()`](https://usdaforestservice.github.io/gdalraster/reference/get_cache_max.md)
  and
  [`set_cache_max()`](https://usdaforestservice.github.io/gdalraster/reference/set_cache_max.md):
  get/set maximum memory size available for the GDAL block cache
- [`get_cache_used()`](https://usdaforestservice.github.io/gdalraster/reference/get_cache_used.md):
  change the return value to R `numeric` type carrying the `integer64`
  class attribute, and add argument `units` defaulting to `"MB"`
- expose
  [`gdal_version_num()`](https://usdaforestservice.github.io/gdalraster/reference/gdal_version.md)
  (previously internal/undocumented): return the full version number as
  an `integer` value (convenience for `as.integer(gdal_version()[2])`)
- add
  [`gdal_compute_version()`](https://usdaforestservice.github.io/gdalraster/reference/gdal_compute_version.md):
  compute the integer version number (`GDAL_VERSION_NUM`) from the
  individual components (major, minor, revision)

### Other stand-alone functions

- add
  [`pixel_extract()`](https://usdaforestservice.github.io/gdalraster/reference/pixel_extract.md):
  extract pixel values at geospatial point locations, with options for
  multiple interpolation methods and returning the individual pixel
  values from a N x N kernel
  ([\#570](https://github.com/USDAForestService/gdalraster/issues/570))
- add
  [`transform_bounds()`](https://usdaforestservice.github.io/gdalraster/reference/transform_bounds.md):
  transform a bounding box, densifying the edges to account for
  nonlinear transformations along these edges and extracting the
  outermost bounds (wrapper of `OCTTransformBounds()` in the GDAL
  Spatial Reference System API, requires GDAL \>= 3.4)
- add
  [`autoCreateWarpedVRT()`](https://usdaforestservice.github.io/gdalraster/reference/autoCreateWarpedVRT.md):
  create a warped virtual dataset representing the input raster warped
  into the target coordinate system (wrapper of
  `GDALAutoCreateWarpedVRT()`)
- add
  [`validateCreationOptions()`](https://usdaforestservice.github.io/gdalraster/reference/validateCreationOptions.md):
  validate the list of creation options that are handled by a driver
  ([\#663](https://github.com/USDAForestService/gdalraster/issues/663))
- [`getCreationOptions()`](https://usdaforestservice.github.io/gdalraster/reference/getCreationOptions.md):
  change the return value to a named list of options with their
  information
  ([\#662](https://github.com/USDAForestService/gdalraster/issues/662))
- [`getCreationOptions()`](https://usdaforestservice.github.io/gdalraster/reference/getCreationOptions.md):
  include `$min` and `$max` attributes in the returned list if GDAL \>=
  3.11
- [`create()`](https://usdaforestservice.github.io/gdalraster/reference/create.md):
  add argument `return_obj`, `TRUE` to return an object of class
  `GDALRaster` with update access on the created dataset
- [`createCopy()`](https://usdaforestservice.github.io/gdalraster/reference/createCopy.md):
  add argument `return_obj`, `TRUE` to return an object of class
  `GDALRaster` with update access on the created dataset
- [`createCopy()`](https://usdaforestservice.github.io/gdalraster/reference/createCopy.md):
  an object of class `GDALRaster` can be given for argument
  `src_filename` to specify the source dataset
- [`warp()`](https://usdaforestservice.github.io/gdalraster/reference/warp.md):
  source and destination rasters may be given as objects of class
  `GDALRaster`
- [`translate()`](https://usdaforestservice.github.io/gdalraster/reference/translate.md):
  source raster can be given as a `GDALRaster` object
- [`inspectDataset()`](https://usdaforestservice.github.io/gdalraster/reference/inspectDataset.md):
  handle the case if separate raster and vector drivers are identified
  but a specific flag for the dataset type was not given
  ([\#693](https://github.com/USDAForestService/gdalraster/issues/693))
- [`rasterize()`](https://usdaforestservice.github.io/gdalraster/reference/rasterize.md):
  support passing a `GDALRaster` object for in-place updating
  ([\#660](https://github.com/USDAForestService/gdalraster/issues/660))
- accept one (x, y) as a vector in functions that expect matrix or data
  frame input
  ([`transform_xy()`](https://usdaforestservice.github.io/gdalraster/reference/transform_xy.md),
  `inverse_proj()`,
  [`apply_geotransform()`](https://usdaforestservice.github.io/gdalraster/reference/apply_geotransform.md),
  [`get_pixel_line()`](https://usdaforestservice.github.io/gdalraster/reference/get_pixel_line.md))
- [`transform_xy()`](https://usdaforestservice.github.io/gdalraster/reference/transform_xy.md)
  /
  [`inv_project()`](https://usdaforestservice.github.io/gdalraster/reference/inv_project.md):
  support input points with z vertices (three column xyz) or time values
  (four column xyzt), and optionally accept input of point geometries as
  character vector of WKT strings, WKB raw vector, or list of WKB raw
  vectors
- [`transform_xy()`](https://usdaforestservice.github.io/gdalraster/reference/transform_xy.md)
  /
  [`inv_project()`](https://usdaforestservice.github.io/gdalraster/reference/inv_project.md):
  account for behavior change in the GDAL `Transform()` methods of
  `OGRCoordinateTransformation` at GDAL 3.11
  ([\#631](https://github.com/USDAForestService/gdalraster/issues/631))
- [`plot_raster()`](https://usdaforestservice.github.io/gdalraster/reference/plot_raster.md):
  add argument `pixel_fn` to specify a function that will be applied to
  the input data for plotting, and handle input raster with complex data
  type
  ([\#582](https://github.com/USDAForestService/gdalraster/issues/582))
- [`inspectDataset()`](https://usdaforestservice.github.io/gdalraster/reference/inspectDataset.md)
  and
  [`warp()`](https://usdaforestservice.github.io/gdalraster/reference/warp.md):
  set a quiet error handler around code checks that are based on a
  [`try()`](https://rdrr.io/r/base/try.html) statement that attempts to
  open a raster dataset
  ([\#709](https://github.com/USDAForestService/gdalraster/issues/709))

### Bug fixes

- handle `NA` in the input coordinates to
  [`transform_xy()`](https://usdaforestservice.github.io/gdalraster/reference/transform_xy.md),
  [`inv_project()`](https://usdaforestservice.github.io/gdalraster/reference/inv_project.md),
  [`transform_bounds()`](https://usdaforestservice.github.io/gdalraster/reference/transform_bounds.md),
  [`apply_geotransform()`](https://usdaforestservice.github.io/gdalraster/reference/apply_geotransform.md)
  and
  [`get_pixel_line()`](https://usdaforestservice.github.io/gdalraster/reference/get_pixel_line.md)
  ([\#592](https://github.com/USDAForestService/gdalraster/issues/592))
- fix
  [`plot_raster()`](https://usdaforestservice.github.io/gdalraster/reference/plot_raster.md)
  to avoid integer overflow
  ([\#613](https://github.com/USDAForestService/gdalraster/issues/613)
  thanks to [@mdsumner](https://github.com/mdsumner))
- fix potential integer overflow when multiplying raster dimensions in R
  without explicit [`as.numeric()`](https://rdrr.io/r/base/numeric.html)
  ([\#620](https://github.com/USDAForestService/gdalraster/issues/620)
  thanks to [@mdsumner](https://github.com/mdsumner))
- configure.ac: fix test for GDAL \>= 3.1.0
- fix class method `VSIFile$read()`: the `nbytes` argument should accept
  values carrying the `integer64` class attribute (fixes
  [\#677](https://github.com/USDAForestService/gdalraster/issues/677)
  thanks to [@mdsumner](https://github.com/mdsumner))

### Internal

- configure.ac: remove configuration of PROJ include directory and libs
  as no longer needed
  ([\#702](https://github.com/USDAForestService/gdalraster/issues/702))
- implement
  [`create()`](https://usdaforestservice.github.io/gdalraster/reference/create.md),
  [`createCopy()`](https://usdaforestservice.github.io/gdalraster/reference/createCopy.md)
  and
  [`autoCreateWarpedVRT()`](https://usdaforestservice.github.io/gdalraster/reference/autoCreateWarpedVRT.md)
  as `GDALRaster` object factories
  ([\#606](https://github.com/USDAForestService/gdalraster/issues/606))
- implement
  [`ogr_ds_create()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_manage.md)
  and
  [`ogr_layer_create()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_manage.md)
  as `GDALVector` object factories
  ([\#609](https://github.com/USDAForestService/gdalraster/issues/609))
- code linting for `cppcheck` style in `src/geom_api.cpp` and
  `src/srs_api.cpp`
  ([\#658](https://github.com/USDAForestService/gdalraster/issues/658))
- various C++ code linting throughout
  ([\#624](https://github.com/USDAForestService/gdalraster/issues/624))
- add header `src/gdal_vsi.h`, and minor code cleanups in
  `src/gdal_vsi.cpp`
- rename files: `src/wkt_conv.cpp` -\> `src/srs_api.cpp`,
  `src/wkt_conv.h` -\> `src/srs_api.h`, `tests/testthat/test-wkt_conv.R`
  -\> `tests/testthat/test-srs_api.R`
- use consistent naming convention for C++ class member variables
- pass arguments by `const` reference in some srs and geom functions
- use try/catch if potentially large allocation of `std::vector`
  ([\#485](https://github.com/USDAForestService/gdalraster/issues/485))
- replace `CPLIsNan()` with `std::isnan()`
  ([\#485](https://github.com/USDAForestService/gdalraster/issues/485))
- [`read_ds()`](https://usdaforestservice.github.io/gdalraster/reference/read_ds.md):
  pre-allocate the output vector based on the bit size of the unioned
  data type across all bands
- validate input columns in the *data frame-to-matrix* internal C++
  conversion functions

### Documentation

- update the package Description
- add package vignette [Vector API
  Overview](https://usdaforestservice.github.io/gdalraster/articles/vector-api-overview.html)
- update descriptions of the C++ exposed classes and emphasize lack of
  named argument support in those cases (i.e., argument order matters,
  mainly of concern with class constructors)
- update [Raster API
  Tutorial](https://usdaforestservice.github.io/gdalraster/articles/raster-api-tutorial.html)
  to use the argument `return_obj` with
  [`create()`](https://usdaforestservice.github.io/gdalraster/reference/create.md)
  and
  [`createCopy()`](https://usdaforestservice.github.io/gdalraster/reference/createCopy.md)
- additions to the section on Azure (/vsiaz/) in [GDAL Config Quick
  Reference](https://usdaforestservice.github.io/gdalraster/articles/gdal-config-quick-ref.html)
- add the `-srcband` and `-dstband` command-line options in the
  documentation for
  [`warp()`](https://usdaforestservice.github.io/gdalraster/reference/warp.md)
- add an example in
  [`ogr2ogr()`](https://usdaforestservice.github.io/gdalraster/reference/ogr2ogr.md)
  for dissolving features based on an attribute value
- minor updates for
  [`g_transform()`](https://usdaforestservice.github.io/gdalraster/reference/g_transform.md)
- restructure and edit
  [`?ogr_define`](https://usdaforestservice.github.io/gdalraster/reference/ogr_define.md)
- update examples for
  [`inspectDataset()`](https://usdaforestservice.github.io/gdalraster/reference/inspectDataset.md)
  and
  [`vsi_get_file_metadata()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_get_file_metadata.md)
  to use the new sample data file `inst/extdata/ynp_features.zip`
  ([\#691](https://github.com/USDAForestService/gdalraster/issues/691))
- add alt text to images in the package vignettes and other web-only
  articles
- code that cleans up temp files in the examples is now wrapped in
  `\dontshow{}`
- add [Development
  practices](https://usdaforestservice.github.io/gdalraster/CONTRIBUTING.html#development-practices)
  in CONTRIBUTING.md
- add the [OpenSSF best
  practices](https://www.bestpractices.dev/projects/9382) badge in
  README.md
- add the [OpenSSF
  Scorecard](https://scorecard.dev/viewer/?uri=github.com/USDAForestService/gdalraster)
  badge in README.md

## gdalraster 1.12.0

CRAN release: 2025-01-19

### Features / enhancements

- add
  [`inspectDataset()`](https://usdaforestservice.github.io/gdalraster/reference/inspectDataset.md):
  obtain information about a GDAL raster or vector dataset
  ([\#552](https://github.com/USDAForestService/gdalraster/issues/552))
- add
  [`identifyDriver()`](https://usdaforestservice.github.io/gdalraster/reference/identifyDriver.md):
  identify the driver that can open a given filename, connection string,
  etc.
  ([\#553](https://github.com/USDAForestService/gdalraster/issues/553))
- [`transform_xy()`](https://usdaforestservice.github.io/gdalraster/reference/transform_xy.md)
  and
  [`inv_project()`](https://usdaforestservice.github.io/gdalraster/reference/inv_project.md):
  accept the SRS arguments in any format supported by
  [`srs_to_wkt()`](https://usdaforestservice.github.io/gdalraster/reference/srs_convert.md)
- [`vsi_read_dir()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_read_dir.md):
  add the `recursive` argument
  ([\#426](https://github.com/USDAForestService/gdalraster/issues/426));
  omit “.” and “..” from the output directory listing; add the
  `all_files` argument, `TRUE` to include hidden files; sort the
  directory listing alphabetically

### Bug fixes

- fix the mode name for `"color-relief"` in `DEFAULT_DEM_PROC`
  ([\#430](https://github.com/USDAForestService/gdalraster/issues/430))
- fix a unit test for class `VSIFile`: create file with “w+” access for
  read/write
  ([\#546](https://github.com/USDAForestService/gdalraster/issues/546))
- [`ogr_def_geom_field()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_define.md):
  fix input validation for the `srs` argument
  ([\#507](https://github.com/USDAForestService/gdalraster/issues/507))
- [`createCopy()`](https://usdaforestservice.github.io/gdalraster/reference/createCopy.md):
  check the driver for GDAL_DCAP_CREATE capability as well as
  GDAL_DCAP_CREATECOPY
  ([\#479](https://github.com/USDAForestService/gdalraster/issues/479))
- [`plot_raster()`](https://usdaforestservice.github.io/gdalraster/reference/plot_raster.md):
  default to no stretch when the input is an RGB Byte raster
  ([\#435](https://github.com/USDAForestService/gdalraster/issues/435))

### Documentation

- fix an example for class `VSIFile` that needs “w+” access instead of
  “w”
- fix missing double quote in
  [`transform_xy()`](https://usdaforestservice.github.io/gdalraster/reference/transform_xy.md)
  examples
- document the `fillRaster()` method in class `GDALRaster`

## gdalraster 1.11.1

CRAN release: 2024-06-13

- fix test in test-ogr_manage.R: the test for GeoJSON layer did not need
  to check existence using `with_update = TRUE` on a file in extdata
  ([\#410](https://github.com/USDAForestService/gdalraster/issues/410))

- add
  [`apply_geotransform()`](https://usdaforestservice.github.io/gdalraster/reference/apply_geotransform.md):
  convert raster column/row to geospatial x/y coordinates, wrapper of
  `GDALApplyGeoTransform()` in the GDAL API, operating on a matrix of
  input col/row coordinates (the internal wrapper
  `.apply_geotransform()` is unchanged)

- add `GDALRaster$apply_geotransform()`: class method alternative to
  calling the stand-alone function
  [`apply_geotransform()`](https://usdaforestservice.github.io/gdalraster/reference/apply_geotransform.md)
  on an object of class `GDALRaster`

- [`vsi_curl_clear_cache()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_curl_clear_cache.md):
  add parameter `quiet` to wrap the API call in a quiet error handler,
  `TRUE` by default

- Documentation: document the `w+` access flag for class `VSIFile`; add
  `CPL_VSIL_USE_TEMP_FILE_FOR_RANDOM_WRITE` configuration option in
  vignette [GDAL Config Quick
  Reference](https://usdaforestservice.github.io/gdalraster/articles/gdal-config-quick-ref.html);
  replace [`paste0()`](https://rdrr.io/r/base/paste.html) with
  [`file.path()`](https://rdrr.io/r/base/file.path.html) in the examples
  throughout

- code linting

## gdalraster 1.11.0

CRAN release: 2024-06-03

### System requirements

- GDAL \>= 3.1.0 is now required (previously \>= 2.4.0)
- package `bit64` has been added to Imports
- package `RcppInt64` has been added in LinkingTo

### New utility functions for managing vector data sources

- initial bindings to the GDAL/OGR Vector API supporting data source
  management:
  [`ogr_ds_exists()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_manage.md),
  [`ogr_ds_format()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_manage.md),
  [`ogr_ds_test_cap()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_manage.md),
  [`ogr_ds_create()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_manage.md),
  [`ogr_ds_layer_count()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_manage.md),
  [`ogr_ds_layer_names()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_manage.md),
  [`ogr_layer_exists()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_manage.md),
  [`ogr_layer_test_cap()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_manage.md),
  [`ogr_layer_create()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_manage.md),
  [`ogr_layer_field_names()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_manage.md),
  [`ogr_layer_delete()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_manage.md),
  [`ogr_field_index()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_manage.md),
  [`ogr_field_create()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_manage.md),
  [`ogr_geom_field_create()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_manage.md),
  [`ogr_field_rename()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_manage.md),
  [`ogr_field_delete()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_manage.md),
  [`ogr_execute_sql()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_manage.md)
- documentation and helper functions for feature class definition:
  [`ogr_def_field()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_define.md),
  [`ogr_def_geom_field()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_define.md),
  [`ogr_def_layer()`](https://usdaforestservice.github.io/gdalraster/reference/ogr_define.md)

### New bindings to the GDAL VSIVirtualHandle API

- class `VSIFile` wraps `VSIVirtualHandle` for Standard C binary file
  I/O on regular file systems, URLs, cloud storage services,
  Zip/GZip/7z/RAR, and in-memory files

### GDAL VSI for operations on virtual file systems (bug fix / enhancements)

- bug fix in
  [`vsi_mkdir()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_mkdir.md):
  the file mode was set incorrectly because `mode` was not passed
  correctly as octal literal. `mode` is now passed as a character string
  containing the file mode as octal.
- add
  [`vsi_get_file_metadata()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_get_file_metadata.md):
  returns metadata for network filesystem objects (/vsicurl/, /vsis3/,
  /vsiaz/, etc.), and with GDAL \>= 3.7, /vsizip/ SOZip metadata
- add
  [`vsi_set_path_option()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_set_path_option.md):
  set a path specific option for a given path prefix, e.g., credential
  setting for a virtual file system (GDAL \>= 3.6)
- add
  [`vsi_clear_path_options()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_clear_path_options.md):
  clear path specific configuration options previously set with
  [`vsi_set_path_option()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_set_path_option.md)
  (GDAL \>= 3.6)
- [`vsi_rmdir()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_rmdir.md):
  add argument `recursive`, `TRUE` to delete the directory and its
  content
- [`vsi_mkdir()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_mkdir.md):
  add argument `recursive`, `TRUE` to create the directory and its
  ancestors
- several VSI functions returned `0` or `-1` invisibly indicating
  success/failure, consistent with GDAL return values. Those return
  values are now visible to be consistent with return values from
  `VSIFile` class methods.
- [`vsi_stat()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_stat.md)
  with `info = "size"`, and
  [`vsi_get_disk_free_space()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_get_disk_free_space.md)
  now return
  [`bit64::integer64`](https://rdrr.io/pkg/bit64/man/bit64-package.html)
  type

### GDALRaster-class

- behavior change: the class methods `$info()` and `$infoAsJSON()` now
  use the default command-line arguments for the underlying `gdalinfo`
  utility. Arguments are configurable in the new read/write
  `$infoOptions` field, which is an empty vector by default
  (`character(0)`).
- add support for I/O of Byte raster as R `raw` type, and add the
  setting `$readByteAsRaw` as a class field
  ([\#314](https://github.com/USDAForestService/gdalraster/issues/314),
  thanks to [@mdsumner](https://github.com/mdsumner))
- add read/write fields `$infoOptions` and `$quiet` for applying
  per-object settings
- add an optional constructor to allow specifying whether the dataset is
  opened in shared mode, `TRUE` by default
- add method `$getActualBlockSize()`: retrieve the actual block size for
  a given block offset
- add method `$get_pixel_line()`: class method alternative to calling
  the stand-alone function
  [`get_pixel_line()`](https://usdaforestservice.github.io/gdalraster/reference/get_pixel_line.md)
  on an object of class
  `GDALRaster`([\#339](https://github.com/USDAForestService/gdalraster/issues/339))
- add method `$getProjection()`: equivalent to `$getProjectionRef()`
  (consistent with `osgeo.gdal.Dataset.getProjection()` /
  `osgeo.gdal.Dataset.getProjectionRef()` in the GDAL Python API)
- method `$getDefaultRAT()`: add progress bar since retrieving large
  raster attribute tables could take \>30 sec

### Stand-alone processing functions

- [`calc()`](https://usdaforestservice.github.io/gdalraster/reference/calc.md):
  add support for multiband output
  ([\#319](https://github.com/USDAForestService/gdalraster/issues/319))
- [`calc()`](https://usdaforestservice.github.io/gdalraster/reference/calc.md):
  add input validation for `var.names`, must be in `expr`
- [`get_pixel_line()`](https://usdaforestservice.github.io/gdalraster/reference/get_pixel_line.md):
  an object of class `GDALRaster` can now be passed for the `gt`
  parameter, in which case the geotransform will be obtained from the
  object and bounds checking on the raster extent will be done (original
  behavior for `gt` as numeric vector is unchanged)
  ([\#339](https://github.com/USDAForestService/gdalraster/issues/339))
- [`ogr2ogr()`](https://usdaforestservice.github.io/gdalraster/reference/ogr2ogr.md):
  add parameter `open_options` to support options on the source dataset
- [`read_ds()`](https://usdaforestservice.github.io/gdalraster/reference/read_ds.md):
  add parameter `as_raw` to read a Byte raster as R `raw` type
  ([\#314](https://github.com/USDAForestService/gdalraster/issues/314),
  thanks to [@mdsumner](https://github.com/mdsumner))

### GDAL configuration

- add
  [`dump_open_datasets()`](https://usdaforestservice.github.io/gdalraster/reference/dump_open_datasets.md):
  dump a list of all open datasets (shared or not) to the console
- add
  [`get_num_cpus()`](https://usdaforestservice.github.io/gdalraster/reference/get_num_cpus.md):
  get the number of processors detected by GDAL
- add
  [`get_usable_physical_ram()`](https://usdaforestservice.github.io/gdalraster/reference/get_usable_physical_ram.md):
  get usable physical RAM reported by GDAL
- add
  [`has_spatialite()`](https://usdaforestservice.github.io/gdalraster/reference/has_spatialite.md):
  returns `TRUE` if GDAL was built with SpatiaLite support
- add
  [`http_enabled()`](https://usdaforestservice.github.io/gdalraster/reference/http_enabled.md):
  returns `TRUE` if GDAL was built with libcurl support
- add `.cpl_http_cleanup()`: wrapper of `CPLHTTPCleanup()` for internal
  use (2024-05-29)

### Geometry functions

- new additional geometry functions operating on WKT (GEOS via GDAL
  headers):
  [`g_is_empty()`](https://usdaforestservice.github.io/gdalraster/reference/g_query.md),
  [`g_is_valid()`](https://usdaforestservice.github.io/gdalraster/reference/g_query.md),
  [`g_name()`](https://usdaforestservice.github.io/gdalraster/reference/g_query.md),
  [`g_intersects()`](https://usdaforestservice.github.io/gdalraster/reference/g_binary_pred.md),
  [`g_equals()`](https://usdaforestservice.github.io/gdalraster/reference/g_binary_pred.md),
  [`g_disjoint()`](https://usdaforestservice.github.io/gdalraster/reference/g_binary_pred.md),
  [`g_touches()`](https://usdaforestservice.github.io/gdalraster/reference/g_binary_pred.md),
  [`g_contains()`](https://usdaforestservice.github.io/gdalraster/reference/g_binary_pred.md),
  [`g_within()`](https://usdaforestservice.github.io/gdalraster/reference/g_binary_pred.md),
  [`g_crosses()`](https://usdaforestservice.github.io/gdalraster/reference/g_binary_pred.md),[`g_overlaps()`](https://usdaforestservice.github.io/gdalraster/reference/g_binary_pred.md),
  [`g_intersection()`](https://usdaforestservice.github.io/gdalraster/reference/g_binary_op.md),
  [`g_union()`](https://usdaforestservice.github.io/gdalraster/reference/g_binary_op.md),
  [`g_difference()`](https://usdaforestservice.github.io/gdalraster/reference/g_binary_op.md),
  [`g_sym_difference()`](https://usdaforestservice.github.io/gdalraster/reference/g_binary_op.md),
  [`g_distance()`](https://usdaforestservice.github.io/gdalraster/reference/g_measures.md),
  [`g_length()`](https://usdaforestservice.github.io/gdalraster/reference/g_measures.md),
  [`g_area()`](https://usdaforestservice.github.io/gdalraster/reference/g_measures.md),
  [`g_centroid()`](https://usdaforestservice.github.io/gdalraster/reference/g_measures.md)
- add
  [`bbox_transform()`](https://usdaforestservice.github.io/gdalraster/reference/bbox_transform.md):
  transform a bounding box to a different projection
- [`g_transform()`](https://usdaforestservice.github.io/gdalraster/reference/g_transform.md):
  now uses `OGR_GeomTransformer_Create()` and
  `OGR_GeomTransformer_Transform()` in the GDAL API, enhanced version of
  `OGR_G_Transform()`; add arguments `wrap_date_line` and
  `date_line_offset`

### Documentation

- add
  [Discussions](https://github.com/USDAForestService/gdalraster/discussions)
  on the GitHub repository
- add a section for HTTP/HTTPS (/vsicurl/) in the vignette [GDAL Config
  Quick
  Reference](https://usdaforestservice.github.io/gdalraster/articles/gdal-config-quick-ref.html)
- DESCRIPTION file: add Michael D. Sumner in `Authors@R`

### Other internal changes and fixes

- fix memory leaks detected by Valgrind in `GDALRaster` class methods
  `$info()`, `$infoAsJSON()` and `$getDefaultRAT()`
- register a finalizer to call `CPLHTTPCleanup()` upon R session exit
- add `GDALRaster` class method `$setFilename()`: set the filename of an
  uninitialized `GDALRaster` object, currently undocumented / for
  internal use
- add `GDALRaster` class method `_getGDALDatasetH()`: get the GDAL
  dataset handle for internal use
- [`buildRAT()`](https://usdaforestservice.github.io/gdalraster/reference/buildRAT.md):
  if the input raster is an object of class `GDALRaster`, use it by
  reference rather than instantiating another `GDALRaster` object
  internally
- [`calc()`](https://usdaforestservice.github.io/gdalraster/reference/calc.md):
  close input raster dataset before exit when a differing extent is
  detected
- add some missing null checks, and object destruction on error
  conditions, in src/geos_wkt.cpp
- improve the check for `"-json"` as a `cl_arg` to
  [`ogrinfo()`](https://usdaforestservice.github.io/gdalraster/reference/ogrinfo.md)
- code linting

## gdalraster 1.10.0

CRAN release: 2024-03-26

### System requirements

- GDAL built against GEOS is now required

### GDAL VSI for operations on virtual file systems

- add
  [`vsi_get_fs_prefixes()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_get_fs_prefixes.md):
  get the list of prefixes for virtual file system handlers currently
  registered
- add
  [`vsi_get_fs_options()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_get_fs_options.md):
  get the list of options associated with a virtual file system handler
  (for setting with
  [`set_config_option()`](https://usdaforestservice.github.io/gdalraster/reference/set_config_option.md))
- add
  [`vsi_supports_rnd_write()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_supports_rnd_write.md)
  and
  [`vsi_supports_seq_write()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_supports_seq_write.md):
  test whether the filesystem supports random write or sequential write,
  conditional on whether a local temp file is allowed before uploading
  to the target location
- add
  [`vsi_get_disk_free_space()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_get_disk_free_space.md):
  return the free disk space available on the filesystem
- fixed misspelled argument in
  [`vsi_copy_file()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_copy_file.md)
  and
  [`vsi_sync()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_sync.md)
  ([\#233](https://github.com/USDAForestService/gdalraster/issues/233))

### Other stand-alone functions

- add
  [`ogrinfo()`](https://usdaforestservice.github.io/gdalraster/reference/ogrinfo.md):
  wrapper of the `ogrinfo` command-line utility, retrieve information
  about a vector data source and potentially edit data with SQL
  statements (GDAL \>= 3.7)
- add
  [`ogr2ogr()`](https://usdaforestservice.github.io/gdalraster/reference/ogr2ogr.md):
  wrapper of the `ogr2ogr` command-line utility, convert vector data
  between different formats
- add
  [`g_transform()`](https://usdaforestservice.github.io/gdalraster/reference/g_transform.md):
  apply a coordinate transformation to a WKT geometry
- add
  [`geos_version()`](https://usdaforestservice.github.io/gdalraster/reference/geos_version.md):
  get version information for the GEOS library in use by GDAL
- add
  [`push_error_handler()`](https://usdaforestservice.github.io/gdalraster/reference/push_error_handler.md):
  wrapper for `CPLPushErrorHandler()` in the GDAL Common Portability
  Library
- add
  [`pop_error_handler()`](https://usdaforestservice.github.io/gdalraster/reference/pop_error_handler.md):
  wrapper for `CPLPopErrorHandler()` in the GDAL Common Portability
  Library
- [`calc()`](https://usdaforestservice.github.io/gdalraster/reference/calc.md):
  the argument `usePixelLonLat` is deprecated as unnecessary, variables
  `pixelLon` / `pixelLat` are now auto-detected if used in the calc
  expression; small performance improvement from computing `pixelY` only
  when needed
- add optional argument `quiet` in several functions to configure
  progress reporting
  ([\#237](https://github.com/USDAForestService/gdalraster/issues/237))
- make the dataset management functions quieter
  ([\#282](https://github.com/USDAForestService/gdalraster/issues/282))
- [`gdal_formats()`](https://usdaforestservice.github.io/gdalraster/reference/gdal_formats.md)
  now returns a data frame with the supported raster and vector formats,
  and information about the capabilities of each format driver

### CmbTable-class

- `new()`: assign default variable names in the constructor if names are
  not given

### Internal

- `src/geos_wkt.cpp`, `src/transform.cpp`, `src/wkt_conv.cpp`:
  deallocate some OGR geometry and OSR spatial ref objects to fix memory
  leaks
- add more unit tests for geometry operations using GEOS via GDAL
  headers
- `GDALRaster::getMetadataDomainList()`: deallocate the returned string
  list to avoid memory leak
- `GDALRaster::close()`: clear cache if needed, and check the return
  values of `GDALClose()` and `GDALFlushCache()` if GDAL \>= 3.7
- `configure.ac`: add back `proj-include` and `proj-lib`, the latter
  needed in some cases for source install on macOS; rework for the
  system requirement of GDAL built against GEOS
- remove internal
  [`has_geos()`](https://usdaforestservice.github.io/gdalraster/reference/has_geos.md)
  checks and update the documentation, since GDAL with GEOS is now
  required
- add `.editorconfig` file and bulk reformat code style
- fix up R code for `lintr` and add `.lintr` file
- mass replace `NULL` -\> `nullptr` in C++ code
- format diagnostic messages throughout for consistency and follow
  guidelines given in “Writing R Extensions”
- clean up temp files in the examples throughout

## gdalraster 1.9.0

CRAN release: 2024-02-25

### Behavior change

- remove `OSR_DEFAULT_AXIS_MAPPING_STRATEGY=TRADITIONAL_GIS_ORDER` from
  `_gdal_init()`
  ([\#209](https://github.com/USDAForestService/gdalraster/issues/209)),
  since this could give a different SRS comparison result depending on
  GDAL version in cases where axis mapping matters and the default
  options in `OSRIsSameEx()` are used

### GDAL VSI for operations on virtual file systems

- add
  [`vsi_copy_file()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_copy_file.md):
  wrapper for `VSICopyFile()`
- add
  [`vsi_curl_clear_cache()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_curl_clear_cache.md):
  wrapper for `VSICurlClearCache()` and `VSICurlPartialClearCache()`
- add
  [`vsi_mkdir()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_mkdir.md):
  wrapper for `VSIMkdir()`
- add
  [`vsi_read_dir()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_read_dir.md):
  wrapper for `VSIReadDirEx()`
- add
  [`vsi_rename()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_rename.md):
  wrapper for `VSIRename()`
- add
  [`vsi_rmdir()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_rmdir.md):
  wrapper for `VSIRmdir()`
- add
  [`vsi_stat()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_stat.md):
  wrapper for `VSIStatExL()`
- add
  [`vsi_sync()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_sync.md):
  wrapper for `VSISync()`
- add
  [`vsi_unlink()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_unlink.md):
  wrapper for `VSIUnlink()`
- add
  [`vsi_unlink_batch()`](https://usdaforestservice.github.io/gdalraster/reference/vsi_unlink_batch.md):
  wrapper for `VSIUnlinkBatch()`

### GDALRaster-class

- `GDALRaster$new()`: add a constructor for passing dataset open options

### Stand-alone functions

- add
  [`footprint()`](https://usdaforestservice.github.io/gdalraster/reference/footprint.md):
  wrapper of the `gdal_footprint` command-line utility, compute
  footprint of a raster (GDAL \>= 3.8)
- [`read_ds()`](https://usdaforestservice.github.io/gdalraster/reference/read_ds.md):
  add `as_list` argument for option to return multi-band output in list
  form; attach attribute `gis` to the output, a list containing bbox,
  dimension and spatial reference (thanks to input from
  [@mdsumner](https://github.com/mdsumner)
  [\#185](https://github.com/USDAForestService/gdalraster/issues/185))
- [`plot_raster()`](https://usdaforestservice.github.io/gdalraster/reference/plot_raster.md):
  accept pixel data in list form (band vectors as list elements), and
  make use of `gis` attribute if present (thanks to input from
  [@mdsumner](https://github.com/mdsumner)
  [\#185](https://github.com/USDAForestService/gdalraster/issues/185))
- [`srs_is_same()`](https://usdaforestservice.github.io/gdalraster/reference/srs_query.md):
  add arguments for `criterion`, `ignore_axis_mapping` and
  `ignore_coord_epoch`

### Documentation

- organize the website [reference
  index](https://usdaforestservice.github.io/gdalraster/reference/index.html)
- add Microsoft Azure information in [GDAL Config Quick
  Reference](https://usdaforestservice.github.io/gdalraster/articles/gdal-config-quick-ref.html)
- update DESCRIPTION

### Internal

- add `_check_gdal_filename()`: minimal filename check and UTF-8
- add `_get_physical_RAM()`: wrapper for `CPLGetUsablePhysicalRAM()` for
  internal use
- set test coverage target minimum to 90%, codecov range: 70..90
  (previously 80%, 60..80)

## gdalraster 1.8.0

CRAN release: 2024-01-27

### Dependencies

- libxml2 is listed in `SystemRequirements` since it is required by the
  imported package **xml2**

### GDALRaster-class

- `read_only=TRUE` is now an optional default in the class constructor
- add `clearStatistics()`: clear statistics on PAM supported datasets
  (GDAL \>= 3.2)

### Stand-alone functions

- add
  [`addFilesInZip()`](https://usdaforestservice.github.io/gdalraster/reference/addFilesInZip.md):
  create/append to Seek-Optimized ZIP files (GDAL \>= 3.7)
- [`plot_raster()`](https://usdaforestservice.github.io/gdalraster/reference/plot_raster.md):
  now uses georeferenced coordinates by default instead of pixel/line
  ([\#184](https://github.com/USDAForestService/gdalraster/issues/184)
  thanks to [@mdsumner](https://github.com/mdsumner))

### Documentation

- example code for
  [`calc()`](https://usdaforestservice.github.io/gdalraster/reference/calc.md)
  using Landsat bands should have applied scale/offset as given in the
  .json metadata - this has been corrected
- add `AWS_REGION`, and a section for SOZip to [GDAL Config Quick
  Ref](https://usdaforestservice.github.io/gdalraster/articles/gdal-config-quick-ref.html)
- update documentation and examples throughout for default
  `read_only=TRUE` in the constructor for `GDALRaster`
- add
  [`translate()`](https://usdaforestservice.github.io/gdalraster/reference/translate.md)
  and
  [`warp()`](https://usdaforestservice.github.io/gdalraster/reference/warp.md)
  in [Raster API
  Tutorial](https://usdaforestservice.github.io/gdalraster/articles/raster-api-tutorial.html#see-also)

### Internal

- configure.ac: remove parts of PROJ config that were unneeded since
  **gdalraster** only uses PROJ via GDAL headers; use `autoconf` macros
  for compiler invocations; get the PROJ data directory from
  `pkg-config` if possible; add `action-if-cross-compiling` argument
  ([\#190](https://github.com/USDAForestService/gdalraster/issues/190)
  and
  [\#197](https://github.com/USDAForestService/gdalraster/issues/197);
  thanks to Simon Urbanek for debugging an initial issue and providing
  helpful feedback)
- get GDAL libs from `pkg-config` on Windows when possible
  ([\#125](https://github.com/USDAForestService/gdalraster/issues/125)
  thanks to [@kalibera](https://github.com/kalibera))
- [`translate()`](https://usdaforestservice.github.io/gdalraster/reference/translate.md)
  and
  [`warp()`](https://usdaforestservice.github.io/gdalraster/reference/warp.md):
  close the output dataset before closing source dataset(s) in case the
  output is VRT
- switch to using the R convention for x.x.x.9000 development versioning

## gdalraster 1.7.0

CRAN release: 2023-11-29

### GDAL API stand-alone functions

- add
  [`polygonize()`](https://usdaforestservice.github.io/gdalraster/reference/polygonize.md):
  create a polygon feature layer from raster data, wrapper for
  `GDALPolygonize` in the GDAL Algorithms API.
- add
  [`rasterize()`](https://usdaforestservice.github.io/gdalraster/reference/rasterize.md):
  burn vector geometries (points, lines, or polygons) into a raster,
  wrapper for the `gdal_rasterize` command-line utility
- add
  [`buildVRT()`](https://usdaforestservice.github.io/gdalraster/reference/buildVRT.md):
  build a GDAL virtual raster mosaic from a list of datasets, wrapper
  for the `gdalbuildvrt` command-line utility
- add
  [`translate()`](https://usdaforestservice.github.io/gdalraster/reference/translate.md):
  convert raster data between different formats, wrapper for the
  `gdal_translate` command-line utility
- make `t_srs` optional in
  [`warp()`](https://usdaforestservice.github.io/gdalraster/reference/warp.md)
- the GDAL configuration option
  `TRADITIONAL_GIS_ORDER=OSR_DEFAULT_AXIS_MAPPING_STRATEGY` is now set
  on package load

### Documentation

- add [GDAL Config Quick
  Reference](https://usdaforestservice.github.io/gdalraster/articles/gdal-config-quick-ref.html)
  to vignettes
- [`warp()`](https://usdaforestservice.github.io/gdalraster/reference/warp.md)
  has additional documentation covering several processing options
- add the `COMPRESS_OVERVIEW` configuration option to the documentation
  for `GDALRaster::buildOverviews()`
- add [`str()`](https://rdrr.io/r/utils/str.html) of the `GDALRaster`
  object in the [Raster API
  Tutorial](https://usdaforestservice.github.io/gdalraster/articles/raster-api-tutorial.html)
- add display of the EVT raster itself along with its attribute table in
  [Raster Attribute
  Tables](https://usdaforestservice.github.io/gdalraster/articles/raster-attribute-tables.html)

### Internal

- add src/ogr_util.cpp and src/ogr_util.h: OGR utility functions for
  vector data sources
- src/gdal_exp.cpp: additional error handling in several wrapper
  functions, increased test coverage

## gdalraster 1.6.0

CRAN release: 2023-10-27

### Dependencies

- GDAL \>= 2.4.0 (previously \>= 2.3.0)
- package [**xml2**](https://CRAN.R-project.org/package=xml2) is now
  required (previously in Suggests)
- package [**gt**](https://CRAN.R-project.org/package=gt) has been added
  to Suggests (required for
  [`displayRAT()`](https://usdaforestservice.github.io/gdalraster/reference/displayRAT.md))

### GDALRaster-class

- add methods for Raster Attribute Tables: `getDefaultRAT()`,
  `setDefaultRAT()`
- add `getDefaultHistogram()`: fetch default raster histogram for a band
- add `getHistogram()`: compute raster histogram for a band
- add `getMinMax()`: compute min/max for a raster band
- add `getMetadataDomainList()`: get a list of metadata domains for a
  dataset or raster band
- fix `getMetadataItem()` for a specific domain at dataset level
  ([\#109](https://github.com/USDAForestService/gdalraster/issues/109))

### GDAL API stand-alone functions

- add
  [`buildRAT()`](https://usdaforestservice.github.io/gdalraster/reference/buildRAT.md):
  compute for a raster band the set of unique pixel values and their
  counts, and build a GDAL Raster Attribute Table as data frame
- add
  [`displayRAT()`](https://usdaforestservice.github.io/gdalraster/reference/displayRAT.md):
  generate a presentation Raster Attribute Table, showing colors if the
  table contains RGB columns
- add
  [`gdal_formats()`](https://usdaforestservice.github.io/gdalraster/reference/gdal_formats.md):
  report the supported raster formats
- add
  [`getCreationOptions()`](https://usdaforestservice.github.io/gdalraster/reference/getCreationOptions.md):
  get the list of creation options of a raster format
- add
  [`copyDatasetFiles()`](https://usdaforestservice.github.io/gdalraster/reference/copyDatasetFiles.md):
  copy all the files associated with a dataset
- add
  [`deleteDataset()`](https://usdaforestservice.github.io/gdalraster/reference/deleteDataset.md):
  delete a dataset in a format-specific way
- add
  [`renameDataset()`](https://usdaforestservice.github.io/gdalraster/reference/renameDataset.md):
  rename a dataset in a format-specific way
- add some missing error checks in src/gdal_exp.cpp
  ([\#104](https://github.com/USDAForestService/gdalraster/issues/104))

### CmbTable-class

- argument `incr` for the count increment in `CmbTable::update()` can be
  zero

### RunningStats-class

- use `uint64_t` for the count accumulator (previously `long long`) and
  make explicit the return cast in `get_count()` (no user-visible
  changes)
- slightly faster update

### Other miscellaneous

- [`plot_raster()`](https://usdaforestservice.github.io/gdalraster/reference/plot_raster.md):
  normalize legend correctly for `minmax_def` and `minmax_pct_cut`
  ([\#131](https://github.com/USDAForestService/gdalraster/issues/131))

### Documentation

- add vignette [Raster Attribute
  Tables](https://usdaforestservice.github.io/gdalraster/articles/raster-attribute-tables.html)
- add
  [notes](https://usdaforestservice.github.io/gdalraster/reference/RunningStats-class.html#note)
  for `RunningStats-class`
- update vignette [Raster API
  Tutorial](https://usdaforestservice.github.io/gdalraster/articles/raster-api-tutorial.html)
  with
  [`gdal_formats()`](https://usdaforestservice.github.io/gdalraster/reference/gdal_formats.md)
  and
  [`getCreationOptions()`](https://usdaforestservice.github.io/gdalraster/reference/getCreationOptions.md)
- update installation instructions in
  [README](https://github.com/USDAForestService/gdalraster/blob/main/README.md)

## gdalraster 1.5.0

CRAN release: 2023-09-25

### GDALRaster-class

- add methods for color tables: `getColorTable()`, `getPaletteInterp()`,
  `setColorTable()`
- add `getRasterColorInterp()`, `setRasterColorInterp()`: get/set color
  interpretation for raster bands
- add `getDescription()`, `setDescription()`: get/set description for
  raster band objects
- add `flushCache()`: flush all write cached data to disk
- add `getFileList()`: returns a list of files forming the dataset
- add `infoAsJSON()`: returns output of the `gdalinfo` command-line
  utility as a JSON-formatted string
- `new()`: add a warning in the class constructor if the raster has an
  int64 data type (would be handled as double for now)

### GDAL API stand-alone functions

- add
  [`bandCopyWholeRaster()`](https://usdaforestservice.github.io/gdalraster/reference/bandCopyWholeRaster.md):
  wrapper for `GDALRasterBandCopyWholeRaster()`, efficiently copy a
  whole raster band
- add
  [`createColorRamp()`](https://usdaforestservice.github.io/gdalraster/reference/createColorRamp.md):
  wrapper for `GDALCreateColorRamp()`, automatically create a ramp from
  one color to another
- add
  [`sieveFilter()`](https://usdaforestservice.github.io/gdalraster/reference/sieveFilter.md):
  wrapper for `GDALSieveFilter()` in the Algorithms API, remove small
  raster polygons
- add PROJ utility functions:
  [`proj_version()`](https://usdaforestservice.github.io/gdalraster/reference/proj_version.md),
  [`proj_search_paths()`](https://usdaforestservice.github.io/gdalraster/reference/proj_search_paths.md),
  [`proj_networking()`](https://usdaforestservice.github.io/gdalraster/reference/proj_networking.md)
  (via GDAL headers)
- add
  [`g_buffer()`](https://usdaforestservice.github.io/gdalraster/reference/g_unary_op.md):
  compute buffer of a WKT geometry (GEOS convenience function via GDAL
  headers)

### CmbTable-class

- add `updateFromMatrixByRow()`: update the hash table from a matrix
  having integer combinations arranged in rows
- add `asMatrix()`: return the combinations table as a numeric matrix
  (alternative to `asDataFrame()`)

### Bug fix

- [`warp()`](https://usdaforestservice.github.io/gdalraster/reference/warp.md)
  caused segfault if proj.db could not be found
  ([\#96](https://github.com/USDAForestService/gdalraster/issues/96))

### Other miscellaneous

- [`plot_raster()`](https://usdaforestservice.github.io/gdalraster/reference/plot_raster.md):
  default value of the `legend` argument has been changed to `FALSE`;
  legend can now use a color table for continuous data; add argument
  `maxColorValue` (e.g., to use RGB 0:255 instead of 0:1 in `col_tbl`)
- [`bbox_from_wkt()`](https://usdaforestservice.github.io/gdalraster/reference/bbox_from_wkt.md),
  [`bbox_to_wkt()`](https://usdaforestservice.github.io/gdalraster/reference/bbox_to_wkt.md):
  add arguments `extend_x`, `extend_y`
- on Windows, reset GDAL environment variables on package unload if they
  were previously set on load
- add inst/extdata/storml_tcc.tif: example dataset of NLCD Tree Canopy
  Cover
- update the package vignette for color tables and raster display

## gdalraster 1.4.0

CRAN release: 2023-08-29

- add
  [`dem_proc()`](https://usdaforestservice.github.io/gdalraster/reference/dem_proc.md):
  wrapper for the `gdaldem` command-line utility to generate DEM
  derivatives

- add the following set methods in class `GDALRaster`:
  `setMetadataItem()`, `setUnitType()`, `setScale()`, `setOffset()`

- add `GDALRaster$buildOverviews()`: build raster overviews

- add `GDALRaster$dim()`: returns a vector of xsize, ysize, nbands

- [`transform_xy()`](https://usdaforestservice.github.io/gdalraster/reference/transform_xy.md)
  and
  [`inv_project()`](https://usdaforestservice.github.io/gdalraster/reference/inv_project.md):
  `pts` can be a data frame or matrix

- [`plot_raster()`](https://usdaforestservice.github.io/gdalraster/reference/plot_raster.md)
  now accepts a `GDALRaster` object for the `data` argument

- [`plot_raster()`](https://usdaforestservice.github.io/gdalraster/reference/plot_raster.md):
  make the legend narrower and add argument `digits` to format legend
  labels when raster data are floating point

- add test suite and code coverage report

## gdalraster 1.3.0

CRAN release: 2023-06-29

- `GDALRaster::read()`: data are now read as R `integer` type when
  possible for the raster data type
  ([\#23](https://github.com/USDAForestService/gdalraster/issues/23))

- add
  [`fillNodata()`](https://usdaforestservice.github.io/gdalraster/reference/fillNodata.md):
  wrapper for `GDALFillNodata()` in the GDAL Algorithms API

- add
  [`read_ds()`](https://usdaforestservice.github.io/gdalraster/reference/read_ds.md):
  convenience wrapper for `GDALRaster$read()`

- add
  [`plot_raster()`](https://usdaforestservice.github.io/gdalraster/reference/plot_raster.md):
  display raster data using base R graphics

- add
  [`get_cache_used()`](https://usdaforestservice.github.io/gdalraster/reference/get_cache_used.md):
  wrapper for `GDALGetCacheUsed64()` with return value in MB

- add `GDALRaster$getOverviewCount()`: return the number of overview
  layers available

- `GDALRaster$info()`: drop `-nomd` argument from the internal call

- [`bbox_from_wkt()`](https://usdaforestservice.github.io/gdalraster/reference/bbox_from_wkt.md):
  return `NA` if creation of the geometry object fails
  ([\#27](https://github.com/USDAForestService/gdalraster/issues/27))

- fix `GDALRaster$getMetadata()`: requesting band-level metadata for a
  domain other than the default metadata domain was returning
  dataset-level metadata instead

- add vignette containing an [R port of the GDAL Raster API
  tutorial](https://usdaforestservice.github.io/gdalraster/articles/raster-api-tutorial.html)

- add description of the `GDAL_RASTERIO_RESAMPLING` configuration option
  in the documentation for `GDALRaster$read()`

- add web article on the [GDAL block
  cache](https://usdaforestservice.github.io/gdalraster/articles/gdal-block-cache.html)
  and configuration of `GDAL_CACHEMAX`

## gdalraster 1.2.0

CRAN release: 2023-05-28

- starting at v. 1.2.0,
  [gdalraster](https://usdaforestservice.github.io/gdalraster/) will
  require R \>= 4.2.0

- fix: check for GEOS availability in bbox geometry functions

- fix: wrong array dimensions in `read()`
  ([\#5](https://github.com/USDAForestService/gdalraster/issues/5)).
  Starting at v. 1.2.0, `read()` will return vector instead of matrix
  which better matches the concept of a native GDAL-like interface
  (thanks to Michael Sumner).

- add:
  [`has_geos()`](https://usdaforestservice.github.io/gdalraster/reference/has_geos.md)
  exported to R

- add:
  [`srs_is_same()`](https://usdaforestservice.github.io/gdalraster/reference/srs_query.md) -
  wrapper for OSRIsSame() in the GDAL Spatial Reference System C API

- documentation - minor edits throughout to improve clarity

## gdalraster 1.1.1

CRAN release: 2023-05-24

- Initial public release.
