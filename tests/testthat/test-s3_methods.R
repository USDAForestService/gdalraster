test_that("plot.OGRFeature / plot.OGRFeatureSet work", {
    f <- system.file("extdata/ynp_fires_1984_2022.gpkg", package = "gdalraster")
    lyr <- new(GDALVector, f, "mtbs_perims")
    on.exit(lyr$close())

    lyr$returnGeomAs <- "WKB"
    feat <- lyr$getNextFeature()
    expect_identical(plot(feat), feat)
    feat_set <- lyr$fetch(3)
    expect_identical(plot(feat_set), feat_set)

    lyr$returnGeomAs <- "WKB_ISO"
    feat <- lyr$getNextFeature()
    expect_identical(plot(feat), feat)
    feat_set <- lyr$fetch(3)
    expect_identical(plot(feat_set), feat_set)

    lyr$returnGeomAs <- "WKT"
    feat <- lyr$getNextFeature()
    expect_identical(plot(feat), feat)
    feat_set <- lyr$fetch(3)
    expect_identical(plot(feat_set), feat_set)

    lyr$returnGeomAs <- "WKT_ISO"
    feat <- lyr$getNextFeature()
    expect_identical(plot(feat), feat)
    feat_set <- lyr$fetch(3)
    expect_identical(plot(feat_set), feat_set)

    lyr$returnGeomAs <- "BBOX"
    feat <- lyr$getNextFeature()
    expect_identical(plot(feat), feat)
    feat_set <- lyr$fetch(3)
    expect_identical(plot(feat_set), feat_set)
})

test_that("print.OGRFeature / print.OGRFeatureSet work", {
    f <- system.file("extdata/ynp_fires_1984_2022.gpkg", package = "gdalraster")
    lyr <- new(GDALVector, f, "mtbs_perims")
    on.exit(lyr$close())

    lyr$returnGeomAs <- "WKB"
    feat <- lyr$getNextFeature()
    expect_identical(print(feat), feat)
    feat_set <- lyr$fetch(3)
    expect_identical(print(feat_set), feat_set)

    lyr$returnGeomAs <- "WKB_ISO"
    feat <- lyr$getNextFeature()
    expect_identical(print(feat), feat)
    feat_set <- lyr$fetch(3)
    expect_identical(print(feat_set), feat_set)

    lyr$returnGeomAs <- "WKT"
    feat <- lyr$getNextFeature()
    expect_identical(print(feat), feat)
    feat_set <- lyr$fetch(3)
    expect_identical(print(feat_set), feat_set)

    lyr$returnGeomAs <- "WKT_ISO"
    feat <- lyr$getNextFeature()
    expect_identical(print(feat), feat)
    feat_set <- lyr$fetch(3)
    expect_identical(print(feat_set), feat_set)

    lyr$returnGeomAs <- "BBOX"
    feat <- lyr$getNextFeature()
    expect_identical(print(feat), feat)
    feat_set <- lyr$fetch(3)
    expect_identical(print(feat_set), feat_set)

    lyr$returnGeomAs <- "NONE"
    feat <- lyr$getNextFeature()
    expect_identical(print(feat), feat)
    feat_set <- lyr$fetch(3)
    expect_identical(print(feat_set), feat_set)

    # GeoJSON with NULL geometries
    f <- system.file("extdata/test_ogr_geojson_mixed_timezone.geojson",
                     package = "gdalraster")
    lyr2 <- new(GDALVector, f, "test")
    on.exit(lyr2$close())

    feat <- lyr$getNextFeature()
    expect_identical(print(feat), feat)
    feat_set <- lyr$fetch(-1)
    expect_identical(print(feat_set), feat_set)
})
