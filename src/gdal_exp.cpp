/* Exported stand-alone functions for gdalraster
   Chris Toney <chris.toney at usda.gov> */

#include <cmath>
#include <unordered_map>

#include "gdal.h"
#include "cpl_port.h"
#include "cpl_conv.h"
#include "cpl_string.h"
#include "gdal_alg.h"
#include "gdal_utils.h"

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
//'   * "GDAL_VERSION_NUM" - formatted as a string, e.g., “3060300” for 
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


//' Report all configured GDAL drivers for raster formats
//'
//' `gdal_formats()` prints to the console a list of the supported raster
//' formats.
//'
//' @returns No return value, called for reporting only.
//' @examples
//' gdal_formats()
// [[Rcpp::export]]
void gdal_formats() {
	Rprintf("Supported raster formats:\n");
	for (int i=0; i < GDALGetDriverCount(); ++i) {
		GDALDriverH hDriver = GDALGetDriver(i);
		char **papszMD = GDALGetMetadata(hDriver, NULL);
		const char *pszRFlag = "", *pszWFlag;
		std::string rw_flag = "";
		if (!CPLFetchBool(papszMD, GDAL_DCAP_RASTER, false))
			continue;
		if (CPLFetchBool(papszMD, GDAL_DCAP_OPEN, false))
			pszRFlag = "r";
		if (CPLFetchBool(papszMD, GDAL_DCAP_CREATE, false))
			pszWFlag = "w+";
		else if (CPLFetchBool(papszMD, GDAL_DCAP_CREATECOPY, false))
			pszWFlag = "w";
		else
			pszWFlag = "o";
		Rprintf("  %s (%s%s): %s\n", GDALGetDriverShortName(hDriver),
				pszRFlag, pszWFlag, GDALGetDriverLongName(hDriver));
	}
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
//' @returns No return value, called for side effects.
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


//' Get the size of memory in use by the GDAL block cache
//'
//' `get_cache_used()` returns the amount of memory currently in use for
//' GDAL block caching. This a wrapper for `GDALGetCacheUsed64()` with return
//' value as MB.
//'
//' @returns Integer. Amount of cache memory in use in MB.
//'
//' @examples
//' get_cache_used()
// [[Rcpp::export]]
int get_cache_used() {
	GIntBig nCacheUsed = GDALGetCacheUsed64();
	return static_cast<int>(nCacheUsed / (1000 * 1000));
}


//' Create a new uninitialized raster
//'
//' `create()` makes an empty raster in the specified format.
//'
//' @param format Raster format short name (e.g., "GTiff").
//' @param dst_filename Filename to create.
//' @param xsize Integer width of raster in pixels.
//' @param ysize Integer height of raster in pixels.
//' @param nbands Integer number of bands.
//' @param dataType Character data type name.
//' (e.g., common data types include Byte, Int16, UInt16, Int32, Float32).
//' @param options Optional list of format-specific creation options in a
//' vector of `"NAME=VALUE"` pairs 
//' (e.g., \code{options = c("COMPRESS=LZW")} to set LZW 
//' compression during creation of a GTiff file).
//' The APPEND_SUBDATASET=YES option can be 
//' specified to avoid prior destruction of existing dataset.
//' @returns Logical indicating success (invisible \code{TRUE}).
//' An error is raised if the operation fails.
//' @seealso
//' [`GDALRaster-class`][GDALRaster], [createCopy()], [rasterFromRaster()],
//' [getCreationOptions()]
//' @examples
//' new_file <- paste0(tempdir(), "/", "newdata.tif")
//' create(format="GTiff", dst_filename=new_file, xsize=143, ysize=107,
//'        nbands=1, dataType="Int16")
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
//' vector of `"NAME=VALUE"` pairs 
//' (e.g., \code{options = c("COMPRESS=LZW")} to set \code{LZW}
//' compression during creation of a GTiff file).
//' The APPEND_SUBDATASET=YES option can be 
//' specified to avoid prior destruction of existing dataset.
//' @returns Logical indicating success (invisible \code{TRUE}).
//' An error is raised if the operation fails.
//' @seealso
//' [`GDALRaster-class`][GDALRaster], [create()], [rasterFromRaster()],
//' [getCreationOptions()]
//' @examples
//' lcp_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
//' tif_file <- paste0(tempdir(), "/", "storml_lndscp.tif")
//' opt <- c("COMPRESS=LZW")
//' createCopy(format="GTiff", dst_filename=tif_file, src_filename=lcp_file,
//'            options=opt)
//' file.size(lcp_file)
//' file.size(tif_file)
//' ds <- new(GDALRaster, tif_file, read_only=FALSE)
//' ds$getMetadata(band=0, domain="IMAGE_STRUCTURE")
//' for (band in 1:ds$getRasterCount())
//'     ds$setNoDataValue(band, -9999)
//' ds$getStatistics(band=1, approx_ok=FALSE, force=TRUE)
//' ds$close()
// [[Rcpp::export(invisible = true)]]
bool createCopy(std::string format, std::string dst_filename,
		std::string src_filename, bool strict = false,
		Rcpp::Nullable<Rcpp::CharacterVector> options = R_NilValue) {

	GDALDriverH hDriver = GDALGetDriverByName(format.c_str());
	if (hDriver == NULL)
		Rcpp::stop("Failed to get driver from format name.");
		
	char **papszMetadata = GDALGetMetadata(hDriver, NULL);
	if (!CPLFetchBool(papszMetadata, GDAL_DCAP_CREATECOPY, FALSE))
		Rcpp::stop("Driver does not support create copy.");
		
	GDALDatasetH hSrcDS = GDALOpenShared(src_filename.c_str(), GA_ReadOnly);
	if(hSrcDS == NULL)
		Rcpp::stop("Open source raster failed.");
	
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
// [[Rcpp::export(name = ".apply_geotransform")]]
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
//' ds <- new(GDALRaster, raster_file, read_only=TRUE)
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
			if (!dst_ds.setGeoTransform(gt))
				Rcpp::warning("Failed to set output geotransform.");
			if (!dst_ds.setProjection(srs))
				Rcpp::warning("Failed to set output projection.");
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
	if (nrasters == 1)
		Rcpp::Rcout << "Scanning raster...\n";
	else
		Rcpp::Rcout << "Combining " << nrasters << " rasters...\n";
	for (int y = 0; y < nrows; ++y) {
		for (std::size_t i = 0; i < nrasters; ++i)
			rowdata.row(i) = Rcpp::as<Rcpp::IntegerVector>(
								src_ds[i].read(
									bands[i], 0, y, ncols, 1, ncols, 1) );
									
		cmbid = tbl.updateFromMatrix(rowdata, 1);
		
		if (out_raster) {
			dst_ds.write( 1, 0, y, ncols, 1, cmbid );
		}
		pfnProgress(y / (nrows-1.0), NULL, pProgressData);
	}

	if (out_raster)
		dst_ds.close();

	for (std::size_t i = 0; i < nrasters; ++i)
		src_ds[i].close();
	
	return tbl.asDataFrame();
}


//' Compute for a raster band the set of unique pixel values and their counts
//' 
//' @noRd
// [[Rcpp::export(name = ".value_count")]]
Rcpp::DataFrame _value_count(std::string src_filename, int band = 1) {

	GDALRaster src_ds = GDALRaster(src_filename, true);
	int nrows = src_ds.getRasterYSize();
	int ncols = src_ds.getRasterXSize();
	GDALProgressFunc pfnProgress = GDALTermProgressR;
	void* pProgressData = NULL;
	Rcpp::DataFrame df_out = Rcpp::DataFrame::create();
	
	Rcpp::Rcout << "Scanning raster...\n";
	
	if (src_ds._readableAsInt(band)) {
		// read pixel values as int
		Rcpp::IntegerVector rowdata(ncols);
		std::unordered_map<int, double> tbl;
		for (int y = 0; y < nrows; ++y) {
			rowdata = Rcpp::as<Rcpp::IntegerVector>(
							src_ds.read(band, 0, y, ncols, 1, ncols, 1) );
			for (auto const& i : rowdata) {
				tbl[i] += 1.0;
			}
			pfnProgress(y / (nrows-1.0), NULL, pProgressData);
		}

		Rcpp::IntegerVector value(tbl.size());
		Rcpp::NumericVector count(tbl.size());
		std::size_t this_idx = 0;
		for(auto iter = tbl.begin(); iter != tbl.end(); ++iter) {
			value[this_idx] = iter->first;
			count[this_idx] = iter->second;
			++this_idx;
		}
		
		df_out.push_back(value, "VALUE");
		df_out.push_back(count, "COUNT");
	}
	else {
		// UInt32, Float32, Float64
		// read pixel values as double
		Rcpp::NumericVector rowdata(ncols);
		std::unordered_map<double, double> tbl;
		for (int y = 0; y < nrows; ++y) {
			rowdata = Rcpp::as<Rcpp::NumericVector>(
							src_ds.read(band, 0, y, ncols, 1, ncols, 1) );
			for (auto const& i : rowdata) {
				tbl[i] += 1.0;
			}
			pfnProgress(y / (nrows-1.0), NULL, pProgressData);
		}

		Rcpp::NumericVector value(tbl.size());
		Rcpp::NumericVector count(tbl.size());
		std::size_t this_idx = 0;
		for(auto iter = tbl.begin(); iter != tbl.end(); ++iter) {
			value[this_idx] = iter->first;
			count[this_idx] = iter->second;
			++this_idx;
		}
		
		df_out.push_back(value, "VALUE");
		df_out.push_back(count, "COUNT");
	}
	
	return df_out;
}


//' Wrapper for GDALDEMProcessing in the GDAL Algorithms C API
//'
//' Called from and documented in R/gdalraster_proc.R
//' @noRd
// [[Rcpp::export(name = ".dem_proc")]]
bool _dem_proc(std::string mode,
			std::string src_filename, 
			std::string dst_filename,
			Rcpp::Nullable<Rcpp::CharacterVector> cl_arg = R_NilValue,
			Rcpp::Nullable<Rcpp::String> col_file = R_NilValue) {

	GDALDatasetH src_ds = GDALOpenShared(src_filename.c_str(), GA_ReadOnly);
	if (src_ds == NULL)
		Rcpp::stop("Open source raster failed.");

	std::vector<char *> argv = {NULL};
	if (cl_arg.isNotNull()) {
		// cast Nullable to the underlying type
		Rcpp::CharacterVector cl_arg_in(cl_arg);
		argv.resize(cl_arg_in.size() + 1);
		for (R_xlen_t i = 0; i < cl_arg_in.size(); ++i) {
			argv[i] = (char *) (cl_arg_in[i]);
		}
		argv[cl_arg_in.size()] = NULL;
	}
	
	GDALDEMProcessingOptions* psOptions;
	psOptions = GDALDEMProcessingOptionsNew(argv.data(), NULL);
	if (psOptions == NULL)
		Rcpp::stop("DEM processing failed (could not create options struct).");
	GDALDEMProcessingOptionsSetProgress(psOptions, GDALTermProgressR, NULL);
	
	GDALDatasetH hDstDS;
	if (col_file.isNotNull()) {
		// cast Nullable to the underlying type
		Rcpp::String col_file_in(col_file);
		hDstDS = GDALDEMProcessing(dst_filename.c_str(), src_ds, mode.c_str(),
					col_file_in.get_cstring(), psOptions, NULL);
	}
	else {
		hDstDS = GDALDEMProcessing(dst_filename.c_str(), src_ds, mode.c_str(),
					NULL, psOptions, NULL);
	}
							
	GDALDEMProcessingOptionsFree(psOptions);
	GDALClose(src_ds);
	if (hDstDS != NULL) {
		GDALClose(hDstDS);
		return true;
	}
	else {
		Rcpp::stop("DEM processing failed.");
	}
}


//' Fill selected pixels by interpolation from surrounding areas
//'
//' `fillNodata()` is a wrapper for `GDALFillNodata()` in the GDAL Algorithms
//' API. This algorithm will interpolate values for all designated nodata 
//' pixels (pixels having an intrinsic nodata value, or marked by zero-valued
//' pixels in the optional raster specified in `mask_file`). For each nodata 
//' pixel, a four direction conic search is done to find values to interpolate
//' from (using inverse distance weighting).
//' Once all values are interpolated, zero or more smoothing iterations
//' (3x3 average filters on interpolated pixels) are applied to smooth out 
//' artifacts.
//'
//' @note
//' The input raster will be modified in place. It should not be open in a
//' `GDALRaster` object while processing with `fillNodata()`.
//'
//' @param filename Filename of input raster in which to fill nodata pixels.
//' @param band Integer band number to modify in place.
//' @param mask_file Optional filename of raster to use as a validity mask
//' (band 1 is used, zero marks nodata pixels, non-zero marks valid pixels).
//' @param max_dist Maximum distance (in pixels) that the algorithm 
//' will search out for values to interpolate (100 pixels by default).
//' @param smooth_iterations The number of 3x3 average filter smoothing
//' iterations to run after the interpolation to dampen artifacts
//' (0 by default).
//' @returns Logical indicating success (invisible \code{TRUE}).
//' An error is raised if the operation fails.
//' @examples
//' ## fill nodata edge pixels in the elevation raster
//' elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
//' 
//' ## get count of nodata
//' tbl <- buildRAT(elev_file)
//' head(tbl)
//' tbl[is.na(tbl$VALUE),]
//' 
//' ## make a copy that will be modified
//' mod_file <- paste0(tempdir(), "/", "storml_elev_fill.tif")
//' file.copy(elev_file,  mod_file)
//' 
//' fillNodata(mod_file, band=1)
//' 
//' mod_tbl = buildRAT(mod_file)
//' head(mod_tbl)
//' mod_tbl[is.na(mod_tbl$VALUE),]
// [[Rcpp::export(invisible = true)]]
bool fillNodata(std::string filename, int band, std::string mask_file = "",
		double max_dist = 100, int smooth_iterations = 0) {

	GDALDatasetH hDS = NULL;
	GDALRasterBandH hBand = NULL;
	GDALDatasetH hMaskDS = NULL;
	GDALRasterBandH hMaskBand = NULL;
	CPLErr err;
	
	hDS = GDALOpenShared(filename.c_str(), GA_Update);
	if (hDS == NULL)
		Rcpp::stop("Open raster failed.");
	hBand = GDALGetRasterBand(hDS, band);
	
	if (mask_file != "") {
		hMaskDS = GDALOpenShared(mask_file.c_str(), GA_ReadOnly);
		if (hMaskDS == NULL)
			Rcpp::stop("Open raster failed.");
		hMaskBand = GDALGetRasterBand(hMaskDS, 1);
	}
	
	err = GDALFillNodata(hBand, hMaskBand, max_dist, 0, smooth_iterations,
			NULL, GDALTermProgressR, NULL);
	
	if (err != CE_None)
		Rcpp::stop("fillNodata() failed.");
		
	GDALClose(hDS);
	if (hMaskDS != NULL)
		GDALClose(hMaskDS);
		
	return true;
}


//' Remove small raster polygons
//'
//' `sieveFilter()` is a wrapper for `GDALSieveFilter()` in the GDAL Algorithms
//' API. It removes raster polygons smaller than a provided threshold size
//' (in pixels) and replaces them with the pixel value of the largest neighbour
//' polygon.
//'
//' @details
//' Polygons are determined as regions of the raster where the pixels all have
//' the same value, and that are contiguous (connected).
//' Pixels determined to be "nodata" per the mask band will not be
//' treated as part of a polygon regardless of their pixel values. Nodata areas
//' will never be changed nor affect polygon sizes. Polygons smaller than the
//' threshold with no neighbours that are as large as the threshold will not be
//' altered. Polygons surrounded by nodata areas will therefore not be altered.
//'
//' The algorithm makes three passes over the input file to enumerate the
//' polygons and collect limited information about them. Memory use is
//' proportional to the number of polygons (roughly 24 bytes per polygon), but
//' is not directly related to the size of the raster. So very large raster
//' files can be processed effectively if there aren't too many polygons. But
//' extremely noisy rasters with many one pixel polygons will end up being
//' expensive (in memory) to process.
//'
//' The input dataset is read as integer data which means that floating point
//' values are rounded to integers.
//'
//' @param src_filename Filename of the source raster to be processed.
//' @param src_band Band number in the source raster to be processed.
//' @param dst_filename Filename of the output raster. It may be the same as
//' `src_filename` to update the source file in place.
//' @param dst_band Band number in `dst_filename` to write output. It may be
//' the same as `src_band` to update the source raster in place.
//' @param size_threshold Integer. Raster polygons with sizes (in pixels)
//' smaller than this value will be merged into their largest neighbour.
//' @param connectedness Integer. Either `4` indicating that diagonal pixels
//' are not considered directly adjacent for polygon membership purposes, or
//' `8` indicating they are.
//' @param mask_filename Optional filename of raster to use as a mask.
//' @param mask_band Band number in `mask_filename` to use as a mask. All
//' pixels in the mask band with a value other than zero will be considered
//' suitable for inclusion in polygons.
//' @param options Algorithm options as a character vector of name=value pairs.
//' None currently supported.
//' @returns Logical indicating success (invisible \code{TRUE}).
//' An error is raised if the operation fails.
//'
//' @examples
//' ## remove single-pixel polygons from the vegetation type layer (EVT)
//' evt_file <- system.file("extdata/storml_evt.tif", package="gdalraster")
//'
//' # create a blank raster to hold the output
//' evt_mmu_file <- paste0(tempdir(), "/", "storml_evt_mmu2.tif")
//' rasterFromRaster(srcfile = evt_file,
//'                  dstfile = evt_mmu_file,
//'                  init = 32767)
//'
//' # create a mask to exclude water pixels from the algorithm
//' # recode water (7292) to 0
//' expr <- "ifelse(EVT == 7292, 0, EVT)"
//' mask_file <- calc(expr = expr,
//'                   rasterfiles = evt_file,
//'                   var.names = "EVT")
//'
//' # create a version of EVT with two-pixel minimum mapping unit
//' sieveFilter(src_filename = evt_file,
//'             src_band = 1,
//'             dst_filename = evt_mmu_file,
//'             dst_band = 1,
//'             size_threshold = 2,
//'             connectedness = 8,
//'             mask_filename = mask_file,
//'             mask_band = 1)
// [[Rcpp::export(invisible = true)]]
bool sieveFilter(std::string src_filename, int src_band,
		std::string dst_filename, int dst_band,
		int size_threshold, int connectedness,
		std::string mask_filename = "", int mask_band = 0,
		Rcpp::Nullable<Rcpp::CharacterVector> options = R_NilValue) {

	GDALDatasetH hSrcDS = NULL;
	GDALRasterBandH hSrcBand = NULL;
	GDALDatasetH hMaskDS = NULL;
	GDALRasterBandH hMaskBand = NULL;
	GDALDatasetH hDstDS = NULL;
	GDALRasterBandH hDstBand = NULL;
	bool in_place = false;
	CPLErr err;
	
	if (size_threshold < 1)
		Rcpp::stop("size_threshold must be 1 or larger.");
		
	if (connectedness != 4 && connectedness != 8)
		Rcpp::stop("connectedness must be 4 or 8.");
	
	if (src_filename == dst_filename && src_band == dst_band)
		in_place = true;
	
	if (in_place)
		hSrcDS = GDALOpenShared(src_filename.c_str(), GA_Update);
	else
		hSrcDS = GDALOpenShared(src_filename.c_str(), GA_ReadOnly);
	if (hSrcDS == NULL)
		Rcpp::stop("Open source raster failed.");
	hSrcBand = GDALGetRasterBand(hSrcDS, src_band);
	
	if (mask_filename != "") {
		hMaskDS = GDALOpenShared(mask_filename.c_str(), GA_ReadOnly);
		if (hMaskDS == NULL)
			Rcpp::stop("Open mask raster failed.");
		hMaskBand = GDALGetRasterBand(hMaskDS, mask_band);
	}
	
	if (!in_place) {
		hDstDS = GDALOpenShared(dst_filename.c_str(), GA_Update);
		if (hDstDS == NULL)
			Rcpp::stop("Open destination raster failed.");
		hDstBand = GDALGetRasterBand(hDstDS, dst_band);
	}
	
	if (in_place)
		err = GDALSieveFilter(hSrcBand, hMaskBand, hSrcBand, size_threshold,
				connectedness, NULL, GDALTermProgressR, NULL);
	else
		err = GDALSieveFilter(hSrcBand, hMaskBand, hDstBand, size_threshold,
				connectedness, NULL, GDALTermProgressR, NULL);	
			
	if (err != CE_None)
		Rcpp::stop("sieveFilter() failed.");
		
	GDALClose(hSrcDS);
	if (hMaskDS != NULL)
		GDALClose(hMaskDS);
	if (hDstDS != NULL)
		GDALClose(hDstDS);
		
	return true;
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
		GDALDatasetH hDS = GDALOpenShared(src_files[i].c_str(), GA_ReadOnly);
		if (hDS == NULL) {
			Rcpp::Rcerr << "Error on source: " << src_files[i].c_str() << "\n";
			Rcpp::stop("Open source raster failed.");
		}
		else {
			src_ds[i] = hDS;
		}
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
	if (psOptions == NULL)
		Rcpp::stop("Warp raster failed (could not create options struct).");
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


//' Create a color ramp
//'
//' `createColorRamp()` is a wrapper for `GDALCreateColorRamp()` in the GDAL
//' API. It automatically creates a color ramp from one color entry to another.
//' Output is an integer matrix in color table format for use with
//' [`GDALRaster$setColorTable()`][GDALRaster].
//'
//' @note
//' `createColorRamp()` could be called several times, using `rbind()` to
//' combine multiple ramps into the same color table. Possible duplicate rows
//' in the resulting table are not a problem when used in
//' `GDALRaster$setColorTable()` (i.e., when `end_color` of one ramp is the
//' same as `start_color` of the next ramp).
//'
//' @param start_index Integer start index (raster value).
//' @param start_color Integer vector of length three or four.
//' A color entry value to start the ramp (e.g., RGB values).
//' @param end_index Integer end index (raster value).
//' @param end_color Integer vector of length three or four.
//' A color entry value to end the ramp (e.g., RGB values).
//' @param palette_interp One of "Gray", "RGB" (the default), "CMYK" or "HLS"
//' describing interpretation of `start_color` and `end_color` values
//' (see \href{https://gdal.org/user/raster_data_model.html#color-table}{GDAL 
//' Color Table}).
//' @returns Integer matrix with five columns containing the color ramp from
//' `start_index` to `end_index`, with raster index values in column 1 and
//' color entries in columns 2:5).
//'
//' @seealso
//' [`GDALRaster$getColorTable()`][GDALRaster],
//' [`GDALRaster$getPaletteInterp()`][GDALRaster]
//'
//' @examples
//' # create a color ramp for tree canopy cover percent
//' # band 5 of an LCP file contains canopy cover
//' lcp_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
//' ds <- new(GDALRaster, lcp_file, read_only=TRUE)
//' ds$getDescription(band=5)
//' ds$getMetadata(band=5, domain="")
//' ds$close()
//'
//' # create a GTiff file with Byte data type for the canopy cover band
//' # recode nodata -9999 to 255
//' tcc_file <- calc(expr = "ifelse(CANCOV == -9999, 255, CANCOV)",
//'                  rasterfiles = lcp_file,
//'                  bands = 5,
//'                  var.names = "CANCOV",
//'                  fmt = "GTiff",
//'                  dtName = "Byte",
//'                  nodata_value = 255,
//'                  setRasterNodataValue = TRUE)
//' 
//' ds_tcc <- new(GDALRaster, tcc_file, read_only=FALSE)
//' 
//' # create a color ramp from 0 to 100 and set as the color table
//' colors <- createColorRamp(start_index = 0,
//'                           start_color = c(211, 211, 211),
//'                           end_index = 100,
//'                           end_color = c(0, 100, 0))
//' 
//' print(colors)
//' ds_tcc$setColorTable(band=1, col_tbl=colors, palette_interp="RGB")
//' ds_tcc$setRasterColorInterp(band=1, col_interp="Palette")
//' 
//' # close and re-open the dataset in read_only mode
//' ds_tcc$open(read_only=TRUE)
//' 
//' plot_raster(ds_tcc, interpolate=FALSE, legend=TRUE,
//'             main="Storm Lake Tree Canopy Cover (%)")
//' ds_tcc$close()
// [[Rcpp::export]]
Rcpp::IntegerMatrix createColorRamp(int start_index,
		Rcpp::IntegerVector start_color,
		int end_index,
		Rcpp::IntegerVector end_color,
		std::string palette_interp = "RGB") {
		
	if (end_index <= start_index)
		Rcpp::stop("end_index must be greater than start_index.");
	if (start_color.size() < 3 || start_color.size() > 4)
		Rcpp::stop("Length of start_color must be 3 or 4.");
	if (end_color.size() < 3 || end_color.size() > 4)
		Rcpp::stop("Length of end_color must be 3 or 4.");
	
	if (start_color.size() == 3)
		start_color.push_back(255);
	if (end_color.size() == 3)
		end_color.push_back(255);

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
		Rcpp::stop("Invalid palette_interp.");

	GDALColorTableH hColTbl = GDALCreateColorTable(gpi);
	if (hColTbl == NULL)
		Rcpp::stop("Could not create GDAL color table.");
	
	const GDALColorEntry colStart = {
			static_cast<short>(start_color(0)),
			static_cast<short>(start_color(1)),
			static_cast<short>(start_color(2)),
			static_cast<short>(start_color(3)) };
	const GDALColorEntry colEnd = {
			static_cast<short>(end_color(0)),
			static_cast<short>(end_color(1)),
			static_cast<short>(end_color(2)),
			static_cast<short>(end_color(3)) };

	GDALCreateColorRamp(hColTbl, start_index, &colStart, end_index, &colEnd);
	
	//int nEntries = GDALGetColorEntryCount(hColTbl);
	int nEntries = (end_index - start_index) + 1;
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

	int idx = start_index;
	for (int i=0; i < nEntries; ++i) {
		const GDALColorEntry* colEntry = GDALGetColorEntry(hColTbl, idx);
		col_tbl(i, 0) = idx;
		col_tbl(i, 1) = colEntry->c1;
		col_tbl(i, 2) = colEntry->c2;
		col_tbl(i, 3) = colEntry->c3;
		col_tbl(i, 4) = colEntry->c4;
		++idx;
	}
	
	GDALDestroyColorTable(hColTbl);
	
	return col_tbl;
}


//' Copy a whole raster band efficiently
//'
//' `bandCopyWholeRaster()` copies the complete raster contents of one band to
//' another similarly configured band. The source and destination bands must
//' have the same xsize and ysize. The bands do not have to have the same data
//' type. It implements efficient copying, in particular "chunking" the copy in
//' substantial blocks. This is a wrapper for `GDALRasterBandCopyWholeRaster()`
//' in the GDAL API.
//'
//' @param src_filename Filename of the source raster.
//' @param src_band Band number in the source raster to be copied.
//' @param dst_filename Filename of the destination raster.
//' @param dst_band Band number in the destination raster to copy into.
//' @param options Optional list of transfer hints in a vector of `"NAME=VALUE"`
//' pairs. The currently supported `options` are:
//'   * `"COMPRESSED=YES"` to force alignment on target dataset block sizes to
//'   achieve best compression. 
//'    * `"SKIP_HOLES=YES"` to skip chunks that contain only empty blocks.
//'    Empty blocks are blocks that are generally not physically present in the
//'    file, and when read through GDAL, contain only pixels whose value is the
//'    nodata value when it is set, or whose value is 0 when the nodata value is
//'    not set. The query is done in an efficient way without reading the actual
//'    pixel values (if implemented by the raster format driver, otherwise will
//'    not be skipped).
//' @returns Logical indicating success (invisible \code{TRUE}).
//' An error is raised if the operation fails.
//'
//' @seealso
//' [`GDALRaster-class`][GDALRaster], [create()], [createCopy()],
//' [rasterFromRaster()]
//'
//' @examples
//' ## copy Landsat data from a single-band file to a new multi-band image
//' b5_file <- system.file("extdata/sr_b5_20200829.tif", package="gdalraster")
//' dst_file <- paste0(tempdir(), "/", "sr_multi.tif")
//' rasterFromRaster(b5_file, dst_file, nbands=7, init=0)
//' opt <- c("COMPRESSED=YES", "SKIP_HOLES=YES")
//' bandCopyWholeRaster(b5_file, 1, dst_file, 5, options=opt)
//' ds <- new(GDALRaster, dst_file, read_only=TRUE)
//' ds$getStatistics(band=5, approx_ok=FALSE, force=TRUE)
//' ds$close()
// [[Rcpp::export(invisible = true)]]
bool bandCopyWholeRaster(std::string src_filename, int src_band,
		std::string dst_filename, int dst_band,
		Rcpp::Nullable<Rcpp::CharacterVector> options = R_NilValue) {

	GDALDatasetH hSrcDS = NULL;
	GDALRasterBandH hSrcBand = NULL;
	GDALDatasetH hDstDS = NULL;
	GDALRasterBandH hDstBand = NULL;
	CPLErr err;
	
	hSrcDS = GDALOpenShared(src_filename.c_str(), GA_ReadOnly);
	if (hSrcDS == NULL)
		Rcpp::stop("Open source raster failed.");
	hSrcBand = GDALGetRasterBand(hSrcDS, src_band);
	
	hDstDS = GDALOpenShared(dst_filename.c_str(), GA_Update);
	if (hDstDS == NULL)
		Rcpp::stop("Open destination raster failed.");
	hDstBand = GDALGetRasterBand(hDstDS, dst_band);
	
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
	
	err = GDALRasterBandCopyWholeRaster(hSrcBand, hDstBand, opt_list.data(),
			GDALTermProgressR, NULL);
	
	if (err != CE_None)
		Rcpp::stop("bandCopyWholeRaster() failed.");
		
	GDALClose(hSrcDS);
	GDALClose(hDstDS);
		
	return true;
}


//' Delete named dataset
//'
//' `deleteDataset()` will attempt to delete the named dataset in a format
//' specific fashion. Full featured drivers will delete all associated files,
//' database objects, or whatever is appropriate. The default behavior when no
//' format specific behavior is provided is to attempt to delete all the files
//' that would be returned by `GDALRaster$getFileList()` on the dataset.
//' The named dataset should not be open in any existing `GDALRaster` objects
//' when `deleteDataset()` is called. Wrapper for `GDALDeleteDataset()` in the
//' GDAL API.
//'
//' @note
//' If `format` is set to an empty string `""` (the default) then the function
//' will try to identify the driver from `filename`. This is done internally in
//' GDAL by invoking the `Identify` method of each registered `GDALDriver` in
//' turn. The first driver that successful identifies the file name will be
//' returned. An error is raised if a format cannot be determined from the
//' passed file name.
//'
//' @param filename Filename to delete (should not be open in a `GDALRaster`
//' object).
//' @param format Raster format short name (e.g., "GTiff"). If set to empty
//' string `""` (the default), will attempt to guess the raster format from
//' `filename`.
//' @returns Logical `TRUE` if no error or `FALSE` on failure.
//'
//' @seealso
//' [`GDALRaster-class`][GDALRaster], [create()], [createCopy()],
//' [copyDatasetFiles()], [renameDataset()]
//'
//' @examples
//' b5_file <- system.file("extdata/sr_b5_20200829.tif", package="gdalraster")
//' b5_tmp <- paste0(tempdir(), "/", "b5_tmp.tif")
//' file.copy(b5_file,  b5_tmp)
//' 
//' ds <- new(GDALRaster, b5_tmp, read_only=TRUE)
//' ds$buildOverviews("BILINEAR", levels = c(2, 4, 8), bands = c(1))
//' files <- ds$getFileList()
//' print(files)
//' ds$close()
//' file.exists(files)
//' deleteDataset(b5_tmp)
//' file.exists(files)
// [[Rcpp::export]]
bool deleteDataset(std::string filename, std::string format = "") {

	GDALDriverH hDriver;
	if (format == "") {
		hDriver = GDALIdentifyDriver(filename.c_str(), NULL);
		if (hDriver == NULL)
			Rcpp::stop("Failed to get driver from file name.");
	}
	else {
		hDriver = GDALGetDriverByName(format.c_str());
		if (hDriver == NULL)
			Rcpp::stop("Failed to get driver from format name.");
	}
	
	CPLErr err = GDALDeleteDataset(hDriver, filename.c_str());
	if (err != CE_None) {
		Rcpp::Rcerr << "Error from GDALDeleteDataset().\n";
		return false;
	}
	else {
		return true;
	}
}


//' Rename a dataset
//'
//' `renameDataset()` renames a dataset in a format-specific way (e.g.,
//' rename associated files as appropriate). This could include moving the
//' dataset to a new directory or even a new filesystem.
//' The dataset should not be open in any existing `GDALRaster` objects
//' when `renameDataset()` is called. Wrapper for `GDALRenameDataset()` in the
//' GDAL API.
//'
//' @note
//' If `format` is set to an empty string `""` (the default) then the function
//' will try to identify the driver from `old_filename`. This is done
//' internally in GDAL by invoking the `Identify` method of each registered
//' `GDALDriver` in turn. The first driver that successful identifies the file
//' name will be returned. An error is raised if a format cannot be determined
//' from the passed file name.
//'
//' @param new_filename New name for the dataset.
//' @param old_filename Old name for the dataset (should not be open in a
//' `GDALRaster` object).
//' @param format Raster format short name (e.g., "GTiff"). If set to empty
//' string `""` (the default), will attempt to guess the raster format from
//' `old_filename`.
//' @returns Logical `TRUE` if no error or `FALSE` on failure.
//'
//' @seealso
//' [`GDALRaster-class`][GDALRaster], [create()], [createCopy()],
//' [deleteDataset()], [copyDatasetFiles()]
//'
//' @examples
//' b5_file <- system.file("extdata/sr_b5_20200829.tif", package="gdalraster")
//' b5_tmp <- paste0(tempdir(), "/", "b5_tmp.tif")
//' file.copy(b5_file,  b5_tmp)
//' 
//' ds <- new(GDALRaster, b5_tmp, read_only=TRUE)
//' ds$buildOverviews("BILINEAR", levels = c(2, 4, 8), bands = c(1))
//' ds$getFileList()
//' ds$close()
//' b5_tmp2 <- paste0(tempdir(), "/", "b5_tmp_renamed.tif")
//' renameDataset(b5_tmp2, b5_tmp)
//' ds <- new(GDALRaster, b5_tmp2, read_only=TRUE)
//' ds$getFileList()
//' ds$close()
// [[Rcpp::export]]
bool renameDataset(std::string new_filename, std::string old_filename,
		std::string format = "") {

	GDALDriverH hDriver;
	if (format == "") {
		hDriver = GDALIdentifyDriver(old_filename.c_str(), NULL);
		if (hDriver == NULL)
			Rcpp::stop("Failed to get driver from file name.");
	}
	else {
		hDriver = GDALGetDriverByName(format.c_str());
		if (hDriver == NULL)
			Rcpp::stop("Failed to get driver from format name.");
	}
	
	CPLErr err = GDALRenameDataset(hDriver, new_filename.c_str(),
			old_filename.c_str());
	if (err != CE_None) {
		Rcpp::Rcerr << "Error from GDALRenameDataset().\n";
		return false;
	}
	else {
		return true;
	}
}


//' Copy the files of a dataset
//'
//' `copyDatasetFiles()` copies all the files associated with a dataset.
//' Wrapper for `GDALCopyDatasetFiles()` in the GDAL API.
//'
//' @note
//' If `format` is set to an empty string `""` (the default) then the function
//' will try to identify the driver from `old_filename`. This is done
//' internally in GDAL by invoking the `Identify` method of each registered
//' `GDALDriver` in turn. The first driver that successful identifies the file
//' name will be returned. An error is raised if a format cannot be determined
//' from the passed file name.
//'
//' @param new_filename New name for the dataset (copied to).
//' @param old_filename Old name for the dataset (copied from).
//' @param format Raster format short name (e.g., "GTiff"). If set to empty
//' string `""` (the default), will attempt to guess the raster format from
//' `old_filename`.
//' @returns Logical `TRUE` if no error or `FALSE` on failure.
//'
//' @seealso
//' [`GDALRaster-class`][GDALRaster], [create()], [createCopy()],
//' [deleteDataset()], [renameDataset()]
//'
//' @examples
//' lcp_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
//' ds <- new(GDALRaster, lcp_file, read_only=TRUE)
//' ds$getFileList()
//' ds$close()
//' 
//' lcp_tmp <- paste0(tempdir(), "/", "storm_lake_copy.lcp")
//' copyDatasetFiles(lcp_tmp, lcp_file)
//' ds_copy <- new(GDALRaster, lcp_tmp, read_only=TRUE)
//' ds_copy$getFileList()
//' ds_copy$close()
// [[Rcpp::export]]
bool copyDatasetFiles(std::string new_filename, std::string old_filename,
		std::string format = "") {

	GDALDriverH hDriver;
	if (format == "") {
		hDriver = GDALIdentifyDriver(old_filename.c_str(), NULL);
		if (hDriver == NULL)
			Rcpp::stop("Failed to get driver from file name.");
	}
	else {
		hDriver = GDALGetDriverByName(format.c_str());
		if (hDriver == NULL)
			Rcpp::stop("Failed to get driver from format name.");
	}
	
	CPLErr err = GDALCopyDatasetFiles(hDriver, new_filename.c_str(),
			old_filename.c_str());
	if (err != CE_None) {
		Rcpp::Rcerr << "Error from GDALCopyDatasetFiles().\n";
		return false;
	}
	else {
		return true;
	}
}


//' Return the list of creation options of a GDAL driver as XML string
//'
//' Called from and documented in R/gdal_helpers.R
//' @noRd
// [[Rcpp::export(name = ".getCreationOptions")]]
std::string _getCreationOptions(std::string format) {

	GDALDriverH hDriver = GDALGetDriverByName(format.c_str());
	if (hDriver == NULL)
		Rcpp::stop("Failed to get driver from format name.");

	return GDALGetDriverCreationOptionList(hDriver);
}

