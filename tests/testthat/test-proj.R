test_that("PROJ utility functions run", {
    expect_length(proj_version(), 4)
    expect_no_error(proj_search_paths())
    expect_no_error(proj_networking(TRUE))
    expect_no_error(proj_networking(FALSE))
    if (as.integer(gdal_version()[2]) >= 3000300) {
        proj_paths <- proj_search_paths()
        expect_no_error(proj_search_paths(proj_paths))
    }
})

