test_that("VSIFile works", {
    lcp_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")

    ## Constructors
    expect_no_error(vf <- new(VSIFile, lcp_file))
    expect_true(is(vf, "Rcpp_VSIFile"))
    expect_equal(vf$close(), 0)
    expect_no_error(vf <- new(VSIFile, lcp_file, "r"))
    expect_true(is(vf, "Rcpp_VSIFile"))
    expect_equal(vf$close(), 0)
    expect_no_error(vf <- new(VSIFile, lcp_file, "r", character(0)))
    expect_true(is(vf, "Rcpp_VSIFile"))
    expect_equal(vf$close(), 0)

    ## Methods

    # function to identify LCP file
    is_lcp <- function(bytes) {
        values <- readBin(bytes, "integer", 3)
        if ((values[1] == 20 || values[1] == 21) &&
                (values[2] == 20 || values[2] == 21) &&
                (values[3] >= -90 && values[3] <= 90)) {

            return(TRUE)
        } else {
            return(FALSE)
        }
    }

    vf <- new(VSIFile, lcp_file)
    bytes <- vf$read(12)
    expect_vector(bytes, ptype = raw(), size = 12)
    expect_true(is_lcp(bytes))
    expect_equal(vf$tell(), bit64::as.integer64(12))
    expect_false(vf$eof())
    expect_silent(vf$rewind())
    expect_equal(vf$tell(), bit64::as.integer64(0))

    bytes <- vf$ingest(-1)
    expect_vector(bytes, raw())
    vf$close()
    # if the file is not open
    expect_equal(vf$close(), -1)

    # write to an in-memory file
    lcp_size <- vsi_stat(lcp_file, "size")
    mem_file <- "/vsimem/storml_copy.lcp"
    vf <- new(VSIFile, mem_file, "w")
    expect_equal(vf$write(bytes), bit64::as.integer64(lcp_size))
    expect_equal(vf$flush(), 0)
    expect_equal(vf$tell(), bit64::as.integer64(lcp_size))
    expect_equal(vf$seek(0, SEEK_SET), 0)
    expect_equal(vf$tell(), bit64::as.integer64(0))
    expect_equal(vf$seek(0, SEEK_END), 0)
    expect_equal(vf$tell(), bit64::as.integer64(lcp_size))
    vf$rewind()
    expect_true(is_lcp(vf$read(12)))

    # read/write an integer field
    # write invalid data for the Latitude field and then set back
    # save original
    vf$seek(8, SEEK_SET)
    lat_orig <- vf$read(4)
    # latitude -99 out of range
    bytes <- writeBin(-99L, raw())
    vf$seek(8, SEEK_SET)
    vf$write(bytes)
    vf$rewind()
    expect_false(is_lcp(vf$read(12)))
    vf$seek(8, SEEK_SET)
    expect_equal(readBin(vf$read(4), "integer"), -99L)
    # set back to original
    vf$seek(8, SEEK_SET)
    vf$write(lat_orig)
    vf$rewind()
    expect_true(is_lcp(vf$read(12)))

    # read double - min X coord
    vf$seek(4180, SEEK_SET)
    expect_equal(readBin(vf$read(8), "double"), 323476.071970863151364)

    # read short int - canopy cover units, 1 = percent
    vf$seek(4232, SEEK_SET)
    expect_equal(readBin(vf$read(2), "integer", size = 2), 1L)

    # read the Description field
    vf$seek(6804, SEEK_SET)
    bytes <- vf$read(512)
    expect_equal(rawToChar(bytes), "LCP file created by GDAL.")

    # edit the Description
    desc <- paste(rawToChar(bytes),
                  "Storm Lake AOI,",
                  "Beaverhead-Deerlodge National Forest, Montana.")

    vf$seek(6804, SEEK_SET)
    expect_equal(vf$write(charToRaw(desc)), bit64::as.integer64(nchar(desc)))
    vf$close()

    # be sure it works as a raster dataset
    ds <- new(GDALRaster, mem_file)
    desc_mdi <- ds$getMetadataItem(band = 0,
                                   mdi_name = "DESCRIPTION",
                                   domain = "")
    expect_equal(desc_mdi, desc)
    ds$close()

    # re-open and test eof
    expect_equal(vf$get_filename(), mem_file)
    expect_equal(vf$get_access(), "w")
    expect_equal(vf$set_access("r+"), 0)
    expect_no_error(vf$open())
    vf$seek(0, SEEK_END)
    expect_equal(vf$tell(), bit64::as.integer64(lcp_size))
    expect_true(is.null(vf$read(1)))
    expect_true(vf$eof())
    vf$seek(0, SEEK_SET)
    expect_false(vf$eof())

    # expand the file, giving new_size as integer64 type
    lcp_size_x2 <- bit64::as.integer64(lcp_size * 2)
    expect_equal(vf$truncate(lcp_size_x2), 0)
    vf$close()
    expect_equal(as.integer(vsi_stat(mem_file, "size")),
                 as.integer(lcp_size_x2))

    # truncate back to original size
    vf$open()
    expect_equal(vf$truncate(lcp_size), 0)
    vf$rewind()
    expect_true(is_lcp(vf$read(12)))
    vf$close()

    # verify it still works as a raster dataset
    ds$open(TRUE)
    expect_equal(ds$dim(), c(143, 107, 8))
    ncols <- ds$getRasterXSize()
    rowdata <- ds$read(band=1, xoff=0, yoff=9,
                       xsize=ncols, ysize=1,
                       out_xsize=ncols, out_ysize=1)
    expect_equal(head(rowdata), c(-9999, -9999, -9999, 2456, 2466, 2479))
    ds$close()

    # errors
    expect_error(vf$seek(0, SEEK_SET))
    expect_error(vf$tell())
    expect_error(vf$rewind())
    expect_error(vf$read(1))
    expect_error(vf$write(bytes))
    expect_error(vf$eof())
    expect_error(vf$truncate(100))
    expect_error(vf$flush())
    expect_error(vf$ingest(-1))

    vsi_unlink(mem_file)
})
