# Tests for src/gdal_vsi.cpp - VSI wrapper functions for virtual file systems

test_that("vsi_stat works", {
    data_dir <- system.file("extdata", package="gdalraster")
    expect_true(vsi_stat(data_dir))
    expect_equal(vsi_stat(data_dir, "type"), "dir")
    elev_file <- file.path(data_dir, "storml_elev_orig.tif")
    expect_true(vsi_stat(elev_file))
    expect_equal(vsi_stat(elev_file, "type"), "file")
    expect_equal(vsi_stat(elev_file, "size"), bit64::as.integer64(31152))
    nonexistent <- file.path(data_dir, "wrong_filename.tif")
    expect_false(vsi_stat(nonexistent))
    expect_equal(vsi_stat(nonexistent, "type"), "")
    expect_equal(vsi_stat(nonexistent, "size"), bit64::as.integer64(-1))
    expect_error(vsi_stat(elev_file, "invalid"))
})

test_that("vsi_read_dir works", {
    data_dir <- system.file("extdata", package="gdalraster")
    expect_type(vsi_read_dir(data_dir), "character")
})

test_that("vsi_copy_file works", {
    skip_if(as.integer(gdal_version()[2]) < 3070000)

    elev_file <- system.file("extdata/storml_elev_orig.tif", package="gdalraster")
    tmp_file <- tempfile(fileext = ".tif")
    expect_equal(vsi_copy_file(elev_file, tmp_file), 0)
    expect_equal(vsi_unlink(tmp_file), 0)
})

test_that("vsi_unlink works", {
    elev_file <- system.file("extdata/storml_elev_orig.tif", package="gdalraster")
    tmp_file <- paste0(tempdir(), "/", "tmp.tif")
    file.copy(elev_file, tmp_file)
    expect_equal(vsi_unlink(tmp_file), 0)
})

test_that("vsi_unlink_batch works", {
    skip_if(as.integer(gdal_version()[2]) < 3010000)

    elev_file <- system.file("extdata/storml_elev_orig.tif", package="gdalraster")
    tcc_file <- system.file("extdata/storml_tcc.tif", package="gdalraster")
    tmp_elev <- paste0(tempdir(), "/", "tmp_elev.tif")
    file.copy(elev_file,  tmp_elev)
    tmp_tcc <- paste0(tempdir(), "/", "tmp_tcc.tif")
    file.copy(tcc_file,  tmp_tcc)
    expect_true(all(vsi_unlink_batch(c(tmp_elev, tmp_tcc))))
    expect_false(all(vsi_unlink_batch(c(tmp_elev, tmp_tcc))))
    expect_vector(vsi_unlink_batch(c(tmp_elev, tmp_tcc)), logical(), 2)
})

test_that("vsi_mkdir and vsi_rmdir work", {
    new_dir <- file.path(tempdir(), "newdir")
    expect_equal(vsi_mkdir(new_dir), 0)
    expect_equal(vsi_rmdir(new_dir), 0)

    # recursive
    top_dir <- file.path(tempdir(), "topdir")
    sub_dir <- file.path(top_dir, "subdir")
    expect_equal(vsi_mkdir(sub_dir), -1)
    expect_equal(vsi_mkdir(sub_dir, recursive = TRUE), 0)
    expect_equal(vsi_rmdir(top_dir, recursive = TRUE), 0)
})

test_that("vsi_sync works", {
    src_dir <- file.path(system.file("extdata", package="gdalraster"), "/")
    target_dir <- file.path(tempdir(), "syncdir")
    if (as.integer(gdal_version()[2]) < 3010000) {
        expect_true(vsi_sync(src_dir, target_dir))
    }
    else {
        opt <- "NUM_THREADS=1"
        expect_true(vsi_sync(src_dir, target_dir, options=opt))
    }
    expect_true(length(vsi_read_dir(target_dir)) > 1)
    lapply(vsi_read_dir(target_dir),
           function(f) {f <- file.path(target_dir, f); vsi_unlink(f)})
    expect_equal(vsi_rmdir(target_dir, recursive = TRUE), 0)
})

test_that("vsi_curl_clear_cache runs without error", {
    expect_no_error(vsi_curl_clear_cache())
    expect_no_error(vsi_curl_clear_cache(partial=TRUE, file_prefix="test"))
})

test_that("vsi_rename works", {
    elev_file <- system.file("extdata/storml_elev_orig.tif", package="gdalraster")
    tmp_file <- tempfile(fileext = ".tif")
    file.copy(elev_file, tmp_file)
    new_file <- file.path(dirname(tmp_file), "storml_elev_copy.tif")
    expect_equal(vsi_rename(tmp_file, new_file), 0)
})

test_that("vsi_get_fs_prefixes works", {
    x <- vsi_get_fs_prefixes()
    expect_vector(x, ptype = character())
    expect_true(length(x) > 1)
})

test_that("vsi_get_fs_options works", {
    x <- vsi_get_fs_options("/vsizip/", as_list = FALSE)
    expect_vector(x, ptype = character(), size = 1)

    x <- vsi_get_fs_options("/vsizip/")
    expect_type(x, "list")
})

test_that("vsi_supports_seq_write and vsi_supports_rnd_write work", {
    skip_if(as.integer(gdal_version()[2]) < 3060000)

    expect_true(vsi_supports_seq_write("/vsimem/test-mem-file.gpkg", TRUE))
    expect_true(vsi_supports_rnd_write("/vsimem/test-mem-file.gpkg", TRUE))
})

test_that("vsi_get_disk_free_space returns length-1 numeric vector", {
    tmp_dir <- file.path(tempdir(), "tmpdir")
    vsi_mkdir(tmp_dir)
    x <- vsi_get_disk_free_space(tmp_dir)
    expect_vector(x, ptype = bit64::integer64(), 1)
    vsi_rmdir(tmp_dir)
})

test_that("vsi path specific options can be set/unset", {
    skip_if(as.integer(gdal_version()[2]) < 3060000)

    prefix <- "/vsiaz/sampledata"
    expect_no_error(vsi_set_path_option(prefix,
                                        "AZURE_STORAGE_CONNECTION_STRING",
                                        "connection_string_for_gdalraster"))
    expect_no_error(vsi_clear_path_options(prefix))
})

test_that("vsi_get_file_metadata works", {
    f <- system.file("extdata/ynp_fires_1984_2022.gpkg", package="gdalraster")
    expect_no_error(vsi_get_file_metadata(f, domain=""))

    skip_if(as.integer(gdal_version()[2]) < 3070000)

    zip_file <- tempfile(fileext=".zip")
    addFilesInZip(zip_file, f, full_paths=FALSE, sozip_enabled="YES")
    zip_vsi <- file.path("/vsizip", zip_file)
    expect_no_error(md <- vsi_get_file_metadata(zip_vsi, domain="ZIP"))
    expect_type(md, "list")
    expect_equal(md$SOZIP_VALID, "YES")
})

test_that("get URL functions work", {
    # currently not tested using network access
    # only checking the null return value for now
    expect_null(vsi_get_actual_url("/vsimem/test.tif"))
    opt <- "EXPIRATION_DELAY=604800"
    expect_null(vsi_get_signed_url("/vsimem/test.tif", opt))
})

test_that("vsi_is_local works", {
    skip_if(as.integer(gdal_version()[2]) < 3060000)

    expect_true(vsi_is_local("/vsimem/test-mem-file.tif"))
})
