/* Exported stand-alone functions for gdalraster
   Chris Toney <chris.toney at usda.gov> */

#include <Rcpp.h> 
// [[Rcpp::plugins(cpp11)]]

#include <cmath>

#include "gdal.h"
#include "gdal_utils.h"
#include "cpl_conv.h"
#include "cpl_string.h"

#include <errno.h>

#include "gdalraster.h"
#include "cmb_table.h"

//' Get GDAL version
//'
//' `gdal_version()` returns runtime version information.
//'
//' @returns Character vector of length four containing:
//'   * "–version" - one line version message, e.g., “GDAL 3.6.3, released 
//'   2023/03/12”
//'   * "GDAL_VERSION_NUM" - formatted as a string, e.g., “30603000” for 
//'   GDAL 3.6.3.0
//'   * "GDAL_RELEASE_DATE" - formatted as a string, e.g., “20230312”
//'   * "GDAL_RELEASE_NAME" - e.g., “3.6.3”
//' @examples
//' gdal_version()
// [[Rcpp::export]]
Rcpp::CharacterVector gdal_version() {
	Rcpp::CharacterVector ret(4);
	ret(0) = GDALVersionInfo("-version");
	ret(1) = GDALVersionInfo("VERSION_NUM");
	ret(2) = GDALVersionInfo("RELEASE_DATE");
	ret(3) = GDALVersionInfo("RELEASE_NAME");
	return ret;
}

//' Get GDAL configuration option
//'
//' `get_config_option()` gets the value of GDAL runtime configuration option.
//' Configuration options are essentially global variables the user can set.
//' They are used to alter the default behavior of certain raster format 
//' drivers, and in some cases the GDAL core. For a full description and 
//' listing of available options see 
//' \url{https://gdal.org/user/configoptions.html}.
//'
//' @param key Character name of a configuration option.
//' @returns Character. The value of a (key, value) option previously set with 
//' `set_config_option()`. An empty string (`""`) is returned if `key` is not 
//' found.
//' @seealso
//' [set_config_option()]
//' @examples
//' ## this option is set during initialization of the gdalraster package
//' get_config_option("OGR_CT_FORCE_TRADITIONAL_GIS_ORDER")
// [[Rcpp::export]]
std::string get_config_option(std::string key) {
	const char* default_ = "";
	std::string ret(CPLGetConfigOption(key.c_str(), default_));
	return ret;
}

//' Set GDAL configuration option
//'
//' `set_config_option()` sets a GDAL runtime configuration option. 
//' Configuration options are essentially global variables the user can set.
//' They are used to alter the default behavior of certain raster format 
//' drivers, and in some cases the GDAL core. For a full description and 
//' listing of available options see 
//' \url{https://gdal.org/user/configoptions.html}.
//'
//' @param key Character name of a configuration option.
//' @param value Character value to set for the option. 
//' `value = ""` (empty string) will unset a value previously set by 
//' `set_config_option()`.
//' @returns Nothing.
//' @seealso
//' [get_config_option()]
//' @examples
//' set_config_option("GDAL_CACHEMAX", "64")
//' get_config_option("GDAL_CACHEMAX")
//' ## unset:
//' set_config_option("GDAL_CACHEMAX", "")
// [[Rcpp::export]]
void set_config_option(std::string key, std::string value) {
	const char* value_ = NULL;
	if (value != "")
		value_ = value.c_str();
		
	CPLSetConfigOption(key.c_str(), value_);
}

//' Create a new uninitialized raster
//'
//' `create()` makes an empty raster in the specified format.
//'
//' @param format Raster format short name (e.g., "GTiff" or "HFA").
//' @param dst_filename Filename to create.
//' @param xsize Integer width of raster in pixels.
//' @param ysize Integer height of raster in pixels.
//' @param nbands Integer number of bands.
//' @param dataType Character data type name.
//' (e.g., common data types include Byte, Int16, UInt16, Int32, Float32).
//' @param options Optional list of format-specific creation options in a
//' vector of "NAME=VALUE" pairs 
//' (e.g., \code{options = c("COMPRESS=LZW")} to set LZW 
//' compression during creation of a GTiff file).
//' The APPEND_SUBDATASET=YES option can be 
//' specified to avoid prior destruction of existing dataset.
//' @returns Logical indicating success (invisible \code{TRUE}).
//' An error is raised if the operation fails.
//' @seealso
//' [`GDALRaster-class`][GDALRaster], [createCopy()], [rasterFromRaster()]
//' @examples
//' new_file <- paste0(tempdir(), "/", "newdata.tif")
//' create("GTiff", new_file, 143, 107, 1, "Int16")
//' ds <- new(GDALRaster, new_file, read_only=FALSE)
//' ## EPSG:26912 - NAD83 / UTM zone 12N
//' ds$setProjection(epsg_to_wkt(26912))
//' gt <- c(323476.1, 30, 0, 5105082.0, 0, -30)
//' ds$setGeoTransform(gt)
//' ds$setNoDataValue(band = 1, -9999)
//' ds$fillRaster(band = 1, -9999, 0)
//' ## ...
//' ## close the dataset when done
//' ds$close()
// [[Rcpp::export(invisible = true)]]
bool create(std::string format, std::string dst_filename,
		int xsize, int ysize, int nbands, std::string dataType,
		Rcpp::Nullable<Rcpp::CharacterVector> options = R_NilValue) {

	GDALDriverH hDriver = GDALGetDriverByName( format.c_str() );
	if (hDriver == NULL)
		Rcpp::stop("Failed to get driver for the specified format.");
		
	char **papszMetadata = GDALGetMetadata(hDriver, NULL);
	if (!CPLFetchBool(papszMetadata, GDAL_DCAP_CREATE, FALSE))
		Rcpp::stop("Driver does not support create.");
		
	GDALDataType dt = GDALGetDataTypeByName( dataType.c_str() );
	
	std::vector<char *> opt_list = {NULL};
	if (options.isNotNull()) {
		// cast to the underlying type
		// https://gallery.rcpp.org/articles/optional-null-function-arguments/
		Rcpp::CharacterVector options_in(options);
		opt_list.resize(options_in.size() + 1);
		for (R_xlen_t i = 0; i < options_in.size(); ++i) {
			opt_list[i] = (char *) (options_in[i]);
		}
		opt_list[options_in.size()] = NULL;
	}
	
	GDALDatasetH hDstDS = NULL;
	hDstDS = GDALCreate(hDriver, dst_filename.c_str(),
						xsize, ysize, nbands, dt,
                    	opt_list.data());

	if (hDstDS != NULL) {
		GDALClose(hDstDS);
		return true;
	}
	else {
		Rcpp::stop("Create raster failed.");
	}
}

//' Create a copy of a raster
//'
//' `createCopy()` copies a raster dataset, optionally changing the format.
//' The extent, cell size, number of bands, data type, projection, and 
//' geotransform are all copied from the source raster.
//'
//' @param format Format short name for the output raster 
//' (e.g., "GTiff" or "HFA").
//' @param dst_filename Filename to create.
//' @param src_filename Filename of source raster.
//' @param strict Logical. TRUE if the copy must be strictly equivalent, 
//' or more normally FALSE indicating that the copy may adapt as needed for  
//' the output format.
//' @param options Optional list of format-specific creation options in a
//' vector of "NAME=VALUE" pairs 
//' (e.g., \code{options = c("COMPRESS=LZW")} to set \code{LZW}
//' compression during creation of a GTiff file).
//' The APPEND_SUBDATASET=YES option can be 
//' specified to avoid prior destruction of existing dataset.
//' @returns Logical indicating success (invisible \code{TRUE}).
//' An error is raised if the operation fails.
//' @seealso
//' [`GDALRaster-class`][GDALRaster], [create()], [rasterFromRaster()]
//' @examples
//' lcp_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
//' tif_file <- paste0(tempdir(), "/", "storml_lndscp.tif")
//' options <- c("COMPRESS=LZW")
//' createCopy("GTiff", tif_file, lcp_file, options=options)
//' file.size(lcp_file)
//' file.size(tif_file)
//' ds <- new(GDALRaster, tif_file, read_only=FALSE)
//' ds$getMetadata(0, "IMAGE_STRUCTURE")
//' for (band in 1:ds$getRasterCount())
//'     ds$setNoDataValue(band, -9999)
//' ds$getStatistics(band=1, approx_ok=FALSE, force=TRUE)
//' ds$close()
// [[Rcpp::export(invisible = true)]]
bool createCopy(std::string format, std::string dst_filename,
		std::string src_filename, bool strict = false,
		Rcpp::Nullable<Rcpp::CharacterVector> options = R_NilValue) {

	GDALDriverH hDriver = GDALGetDriverByName( format.c_str() );
	if (hDriver == NULL)
		Rcpp::stop("Failed to get driver for the specified format.");
		
	char **papszMetadata = GDALGetMetadata(hDriver, NULL);
	if (!CPLFetchBool(papszMetadata, GDAL_DCAP_CREATECOPY, FALSE))
		Rcpp::stop("Driver does not support create copy.");
		
	GDALDatasetH hSrcDS = GDALOpenShared(src_filename.c_str(), GA_ReadOnly);
	
	std::vector<char *> opt_list = {NULL};
	if (options.isNotNull()) {
		// cast to the underlying type
		// https://gallery.rcpp.org/articles/optional-null-function-arguments/
		Rcpp::CharacterVector options_in(options);
		opt_list.resize(options_in.size() + 1);
		for (R_xlen_t i = 0; i < options_in.size(); ++i) {
			opt_list[i] = (char *) (options_in[i]);
		}
		opt_list[options_in.size()] = NULL;
	}
	
	GDALDatasetH hDstDS = NULL;
	hDstDS = GDALCreateCopy(hDriver, dst_filename.c_str(), hSrcDS, strict,
				opt_list.data(), GDALTermProgressR, NULL);

	bool ret = false;
	if (hDstDS != NULL) {
		GDALClose(hDstDS);
		ret = true;
	}
	else {
		Rcpp::stop("Create raster failed.");
	}
	
	GDALClose(hSrcDS);
	
	return ret;
}

//' Apply geotransform
//'
//' `_apply_geotransform()` applies geotransform coefficients to a raster 
//' coordinate in pixel/line space (colum/row), converting into a 
//' georeferenced (x, y) coordinate.
//'
//' @param gt Numeric vector of length six containing the geotransform to 
//' apply.
//' @param pixel Numeric scalar. A raster pixel (column) coordinate.
//' @param line Numeric scalar. A raster line (row) coordinate.
//' @returns Numeric vector of length two containing a geospatial x/y 
//' coordinate (spatial reference system of `gt`).
//' @seealso [`GDALRaster$getGeoTransform()`][GDALRaster], [get_pixel_line()], 
//' [inv_geotransform()]
//' @noRd
// [[Rcpp::export(name = ".apply_geotransform"]]
Rcpp::NumericVector _apply_geotransform(const std::vector<double> gt, 
		double pixel, double line) {
		
	double geo_x, geo_y;
	GDALApplyGeoTransform((double *) (gt.data()), pixel, line, &geo_x, &geo_y);
	Rcpp::NumericVector geo_xy = {geo_x, geo_y};
	return geo_xy;
}

//' Invert geotransform
//'
//' `inv_geotransform()` inverts a vector of geotransform coefficients. This 
//' converts the equation from being:\cr
//' raster pixel/line (column/row) -> geospatial x/y coordinate\cr
//' to:\cr
//' geospatial x/y coordinate -> raster pixel/line (column/row)
//'
//' @param gt Numeric vector of length six containing the geotransform to 
//' invert.
//' @returns Numeric vector of length six containing the inverted 
//' geotransform. The output vector will contain NAs if the input geotransform
//' is uninvertable.
//' @seealso [`GDALRaster$getGeoTransform()`][GDALRaster], [get_pixel_line()]
//' @examples
//' elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
//' ds <- new(GDALRaster, elev_file, read_only=TRUE)
//' gt <- ds$getGeoTransform()
//' ds$close()
//' invgt <- inv_geotransform(gt)
//' 
//' ptX = 324181.7
//' ptY = 5103901.4
//' 
//' ## for a point x, y in the spatial reference system of elev_file
//' ## raster pixel (column number):
//' pixel <- floor(invgt[1] +
//'                invgt[2] * ptX +
//'                invgt[3] * ptY)
//' 
//' ## raster line (row number):
//' line <- floor(invgt[4] +
//'               invgt[5] * ptX +
//'               invgt[6] * ptY)
//' 
//' ## get_pixel_line() applies this conversion
// [[Rcpp::export]]
Rcpp::NumericVector inv_geotransform(const std::vector<double> gt) {
	std::vector<double> gt_inv(6);
	if (GDALInvGeoTransform( (double *) (gt.data()), gt_inv.data() ))
		return Rcpp::wrap(gt_inv);
	else
		return Rcpp::NumericVector(6, NA_REAL);
}

//' Raster pixel/line from geospatial x,y coordinates
//'
//' `get_pixel_line()` converts geospatial coordinates to pixel/line (raster 
//' column, row numbers).
//' The upper left corner pixel is the raster origin (0,0) with column, row
//' increasing left to right, top to bottom.
//'
//' @param xy Numeric array of geospatial x,y coordinates in the same
//' spatial reference system as \code{gt}. 
//' @param gt Numeric vector of length six. The affine geotransform for the 
//' raster.
//' @returns Integer array of raster pixel/line.
//'
//' @seealso [`GDALRaster$getGeoTransform()`][GDALRaster], [inv_geotransform()]
//'
//' @examples
//' pt_file <- system.file("extdata/storml_pts.csv", package="gdalraster")
//' ## id, x, y in NAD83 / UTM zone 12N
//' pts <- read.csv(pt_file)
//' print(pts)
//' raster_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
//' ds <- new(GDALRaster, raster_file, TRUE)
//' gt <- ds$getGeoTransform()
//' get_pixel_line(as.matrix(pts[,-1]), gt)
//' ds$close()
// [[Rcpp::export]]
Rcpp::IntegerMatrix get_pixel_line(const Rcpp::NumericMatrix xy,
		const std::vector<double> gt) {
									
	Rcpp::NumericVector inv_gt = inv_geotransform(gt);
	if (Rcpp::any(Rcpp::is_na(inv_gt)))
		Rcpp::stop("Could not get inverse geotransform.");
	Rcpp::IntegerMatrix pixel_line(xy.nrow(), 2);
	for (R_xlen_t i = 0; i < xy.nrow(); ++i) {
			double geo_x = xy(i, 0);
			double geo_y = xy(i, 1);
			// column
			pixel_line(i, 0) = static_cast<int>(std::floor(inv_gt[0] + 
											inv_gt[1] * geo_x +
											inv_gt[2] * geo_y));
			// row
			pixel_line(i, 1) = static_cast<int>(std::floor(inv_gt[3] + 
											inv_gt[4] * geo_x +
											inv_gt[5] * geo_y));
	}
	return pixel_line;
}

//' Raster reprojection
//'
//' `warp()` is a wrapper for the \command{gdalwarp} command-line utility.
//' See \url{https://gdal.org/programs/gdalwarp.html} for details.
//'
//' @param src_files Character vector of source file(s) to be reprojected.
//' @param dst_filename Filename of the output raster.
//' @param t_srs Character. Target spatial reference system. Usually an EPSG 
//' code ("EPSG:#####") or a well known text (WKT) SRS definition.
//' @param cl_arg Optional character vector of command-line arguments to 
//' \code{gdalwarp} in addition to -t_srs.
//' @returns Logical indicating success (invisible \code{TRUE}).
//' An error is raised if the operation fails.
//'
//' @seealso
//' [`GDALRaster-class`][GDALRaster], [srs_to_wkt()]
//'
//' @examples
//' ## reproject the elevation raster to NAD83 / CONUS Albers (EPSG:5070)
//' elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
//'
//' ## command-line arguments for gdalwarp
//' ## resample to 90-m resolution using average and keep pixels aligned:
//' args = c("-tr", "90", "90", "-r", "average", "-tap")
//' ## output to Erdas Imagine format (HFA), creation option for compression:
//' args = c(args, "-of", "HFA", "-co", "COMPRESSED=YES")
//'
//' alb83_file <- paste0(tempdir(), "/", "storml_elev_alb83.img")
//' warp(elev_file, alb83_file, t_srs="EPSG:5070", cl_arg = args)
//' 
//' ds <- new(GDALRaster, alb83_file, read_only=TRUE)
//' ds$getDriverLongName()
//' ds$getProjectionRef()
//' ds$res()
//' ds$getStatistics(band=1, approx_ok=FALSE, force=TRUE)
//' ds$close()
// [[Rcpp::export(invisible = true)]]
bool warp(std::vector<std::string> src_files, std::string dst_filename,
		Rcpp::CharacterVector t_srs, 
		Rcpp::Nullable<Rcpp::CharacterVector> cl_arg = R_NilValue) {

	std::vector<GDALDatasetH> src_ds(src_files.size());
	for (std::size_t i = 0; i < src_files.size(); ++i) {
		src_ds[i] = GDALOpenShared(src_files[i].c_str(), GA_ReadOnly);
	}

	std::vector<char *> argv = {(char *) ("-t_srs"), (char *) (t_srs[0]), NULL};
	//Rcpp::Rcout << argv[0] << " " << argv[1] << " ";
	if (cl_arg.isNotNull()) {
		// cast to the underlying type
		// https://gallery.rcpp.org/articles/optional-null-function-arguments/
		Rcpp::CharacterVector cl_arg_in(cl_arg);
		argv.resize(cl_arg_in.size() + 3);
		for (R_xlen_t i = 0; i < cl_arg_in.size(); ++i) {
			argv[i+2] = (char *) (cl_arg_in[i]);
			//Rcpp::Rcout << argv[i+2] << " ";
		}
		argv[cl_arg_in.size() + 2] = NULL;
	}
	GDALWarpAppOptions* psOptions = GDALWarpAppOptionsNew(argv.data(), NULL);
	GDALWarpAppOptionsSetProgress(psOptions, GDALTermProgressR, NULL);
	
	GDALDatasetH hDstDS = GDALWarp(dst_filename.c_str(), NULL,
							src_files.size(), src_ds.data(),
							psOptions, NULL);
							
	GDALWarpAppOptionsFree(psOptions);
	for (std::size_t i = 0; i < src_files.size(); ++i) {
		GDALClose(src_ds[i]);
	}
	
	if (hDstDS != NULL) {
		GDALClose(hDstDS);
		return true;
	}
	else {
		Rcpp::stop("Warp raster failed.");
	}
}

//' Raster overlay for unique combinations
//' 
//' @description
//' `combine()` overlays multiple rasters so that a unique ID is assigned to 
//' each unique combination of input values. The input raster layers  
//' typically have integer data types (floating point will be coerced to 
//' integer by truncation), and must have the same projection, extent and cell 
//' size. Pixel counts for each unique combination are obtained, and 
//' combination IDs are optionally written to an output raster.
//'
//' Called from and documented in R/gdalraster_proc.R
//' @noRd
// [[Rcpp::export(name = ".combine")]]
Rcpp::DataFrame _combine(
		Rcpp::CharacterVector src_files,
		Rcpp::CharacterVector var_names,
		std::vector<int> bands, 
		std::string dst_filename = "",
		std::string fmt = "", 
		std::string dataType = "UInt32",
		Rcpp::Nullable<Rcpp::CharacterVector> options = R_NilValue) {

	std::size_t nrasters = (std::size_t) src_files.size();
	
	std::vector<GDALRaster> src_ds;
	src_ds.reserve(nrasters);
	
	int nrows = 0;
	int ncols = 0;
	std::vector<double> gt;
	std::string srs;
	GDALRaster dst_ds;
	bool out_raster = false;
	
	if ( (nrasters != ((std::size_t) var_names.size())) ||
			(nrasters !=  bands.size()) )
		Rcpp::stop("src_files, var_names and bands must be the same length.");
	
	if (dst_filename != "") {
		out_raster = true;
		if (fmt == "")
			Rcpp::stop("Output raster format must be specified.");
	}
	
	for (std::size_t i = 0; i < nrasters; ++i) {
		src_ds.push_back(GDALRaster(std::string(src_files[i]), true));
		// use the first raster as reference
		if (i==0) {
			nrows = src_ds[i].getRasterYSize();
			ncols = src_ds[i].getRasterXSize();
			gt = src_ds[i].getGeoTransform();
			srs = src_ds[i].getProjectionRef();
		}
	}
	
	if (out_raster) {
		if (create(fmt, dst_filename, ncols, nrows, 1, dataType, options)) {
			dst_ds = GDALRaster(dst_filename, false);
			dst_ds.setGeoTransform(gt);
			if (!dst_ds.setProjection(srs))
				Rcpp::Rcout << "WARNING: failed to set output projection.\n";
		}
		else {
			for (std::size_t i = 0; i < nrasters; ++i)
				src_ds[i].close();
			Rcpp::stop("Failed to create output raster.");
		}
	}
	
	CmbTable tbl(nrasters, var_names);
	Rcpp::IntegerMatrix rowdata(nrasters, ncols);
	Rcpp::NumericVector cmbid;
	GDALProgressFunc pfnProgress = GDALTermProgressR;
	void* pProgressData = NULL;
	Rcpp::Rcout << "Combining...\n";
	for (int y = 0; y < nrows; ++y) {
		for (std::size_t i = 0; i < nrasters; ++i)
			rowdata.row(i) = Rcpp::as<Rcpp::IntegerVector>(
								src_ds[i].read(
								bands[i], 0, y, ncols, 1, ncols, 1)
								);
									
		cmbid = tbl.updateFromMatrix(rowdata, 1);
		
		if (out_raster) {
			cmbid.attr("dim") = Rcpp::Dimension(1, ncols);
			dst_ds.write( 1, 0, y, ncols, 1, 
						Rcpp::as<Rcpp::NumericMatrix>(cmbid) );
		}
		pfnProgress(y / (nrows-1.0), NULL, pProgressData);
		if (y % 1000 == 0)
			Rcpp::checkUserInterrupt();
	}

	if (out_raster)
		dst_ds.close();

	for (std::size_t i = 0; i < nrasters; ++i)
		src_ds[i].close();
	
	return tbl.asDataFrame();
}
