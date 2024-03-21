# Tests for src/gdal_vsi.cpp - VSI wrapper functions for virtual file systems

test_that("vsi_stat works", {
    data_dir <- system.file("extdata", package="gdalraster")
    expect_true(vsi_stat(data_dir))
    expect_equal(vsi_stat(data_dir, "type"), "dir")
    elev_file <- file.path(data_dir, "storml_elev.tif")
    expect_true(vsi_stat(elev_file))
    expect_equal(vsi_stat(elev_file, "type"), "file")
    expect_equal(vsi_stat(elev_file, "size"), 31152)
    nonexistent <- file.path(data_dir, "wrong_filename.tif")
    expect_false(vsi_stat(nonexistent))
    expect_equal(vsi_stat(nonexistent, "type"), "")
    expect_equal(vsi_stat(nonexistent, "size"), -1)
    expect_error(vsi_stat(elev_file, "invalid"))
})

test_that("vsi_read_dir works", {
    data_dir <- system.file("extdata", package="gdalraster")
    expect_type(vsi_read_dir(data_dir), "character")
})

test_that("vsi_copy_file works", {
    skip_if(as.integer(gdal_version()[2]) < 3070000)

    elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
    tmp_file <- tempfile(fileext = ".tif")
    expect_equal(vsi_copy_file(elev_file, tmp_file), 0)
    expect_equal(vsi_unlink(tmp_file), 0)
})

test_that("vsi_unlink works", {
    elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
    tmp_file <- paste0(tempdir(), "/", "tmp.tif")
    file.copy(elev_file, tmp_file)
    expect_equal(vsi_unlink(tmp_file), 0)
})

test_that("vsi_unlink_batch works", {
    skip_if(as.integer(gdal_version()[2]) < 3010000)

    elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
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
    expect_equal(result <- vsi_rmdir(new_dir), 0)
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
    lapply( vsi_read_dir(target_dir),
            function(f) {f <- file.path(target_dir, f); vsi_unlink(f)}
            )
    expect_equal(vsi_rmdir(target_dir), 0)
})

test_that("vsi_curl_clear_cache runs without error", {
    expect_no_error(vsi_curl_clear_cache())
    expect_no_error(vsi_curl_clear_cache(partial=TRUE, file_prefix="test"))
})

test_that("vsi_rename works", {
    elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
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
    expect_vector(x, ptype = numeric(), 1)
    vsi_rmdir(tmp_dir)
})
