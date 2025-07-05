test_that("srs functions work", {
    elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
    ds <- new(GDALRaster, elev_file, read_only=TRUE)
    srs <- ds$getProjectionRef()
    ds$close()

    expect_false(srs_is_geographic(srs))
    expect_true(srs_is_projected(srs))
    # EPSG:26912 - NAD83 / UTM zone 12N
    utm <- epsg_to_wkt(26912)
    expect_true(srs_is_same(srs, utm))

    expect_true(srs_is_same(srs_to_wkt("EPSG:4326"),
                            srs_to_wkt("OGC:CRS84")))

    expect_true(srs_is_same(srs_to_wkt("EPSG:4326"),
                            srs_to_wkt("OGC:CRS84"),
                            ignore_axis_mapping=TRUE))

    expect_true(srs_is_same(srs_to_wkt("EPSG:4326"),
                            srs_to_wkt("OGC:CRS84"),
                            ignore_coord_epoch=TRUE))

    expect_false(srs_is_same(srs_to_wkt("EPSG:4326"),
                             srs_to_wkt("OGC:CRS84"),
                             ignore_axis_mapping=TRUE,
                             criterion="STRICT"))

    expect_equal(srs_to_wkt("NAD83"), epsg_to_wkt(4269))
    expect_equal(srs_to_wkt("NAD83", pretty=TRUE),
                 epsg_to_wkt(4269, pretty=TRUE))
    expect_true(srs_to_wkt("EPSG:5070", gcs_only = TRUE) |> srs_is_geographic())

    expect_true(srs_to_projjson("WGS84") != "")
    expect_true(srs_to_projjson("WGS84", multiline = FALSE) != "")
    expect_true(srs_to_projjson("WGS84", indent_width = 4) != "")
    expect_true(srs_to_projjson("WGS84", schema = "") != "")

    expect_equal(srs_find_epsg("WGS84"), "EPSG:4326")
    df_matches <- srs_find_epsg("WGS84", all_matches = TRUE)
    expect_true(is.data.frame(df_matches))

    expect_true(nzchar(srs_get_name("EPSG:5070")))

    wkt <- 'GEOGCRS["Coordinate System imported from GRIB file",BASEGEOGCRS["Coordinate System imported from GRIB file",DATUM["unnamed",ELLIPSOID["Sphere",6367470,0,LENGTHUNIT["metre",1,ID["EPSG",9001]]]],PRIMEM["Greenwich",0,ANGLEUNIT["degree",0.0174532925199433,ID["EPSG",9122]]]],DERIVINGCONVERSION["Pole rotation (GRIB convention)",METHOD["Pole rotation (GRIB convention)"],PARAMETER["Latitude of the southern pole (GRIB convention)",-30,ANGLEUNIT["degree",0.0174532925199433,ID["EPSG",9122]]],PARAMETER["Longitude of the southern pole (GRIB convention)",-15,ANGLEUNIT["degree",0.0174532925199433,ID["EPSG",9122]]],PARAMETER["Axis rotation (GRIB convention)",0,ANGLEUNIT["degree",0.0174532925199433,ID["EPSG",9122]]]],CS[ellipsoidal,2],AXIS["latitude",north,ORDER[1],ANGLEUNIT["degree",0.0174532925199433,ID["EPSG",9122]]],AXIS["longitude",east,ORDER[2],ANGLEUNIT["degree",0.0174532925199433,ID["EPSG",9122]]]]'
    expect_true(srs_is_derived_gcs(wkt))
    expect_false(srs_is_compound(wkt))

    wkt <- 'COMPD_CS["COMPD_CS_name",LOCAL_CS["None",LOCAL_DATUM["None",32767],UNIT["Foot_US",0.304800609601219],AXIS["None",OTHER]],VERT_CS["VERT_CS_Name",VERT_DATUM["Local",2005],UNIT["Meter",1,AUTHORITY["EPSG","9001"]],AXIS["Gravity-related height",UP]]]'
    expect_true(srs_is_compound(wkt))

    expect_true(srs_is_geocentric("EPSG:7789"))

    expect_true(srs_is_vertical("EPSG:5705"))

    angular <- srs_get_angular_units("EPSG:4326")
    expect_equal(tolower(angular$unit_name), "degree")
    expect_true(is.numeric(angular$to_radians))

    linear <- srs_get_linear_units("EPSG:5070")
    expect_equal(tolower(linear$unit_name), "metre")
    expect_equal(linear$to_meters, 1.0)

    expect_equal(srs_get_utm_zone("EPSG:26912"), 12)
    expect_equal(srs_get_utm_zone("EPSG:32701"), -1)

    expect_equal(srs_get_axis_mapping_strategy("WGS84"),
                 "OAMS_AUTHORITY_COMPLIANT")

    # errors
    expect_error(epsg_to_wkt(-1))
    expect_equal(srs_to_wkt(""), "")
    expect_error(srs_to_wkt("invalid"))
    expect_error(srs_is_geographic("invalid"))
    expect_error(srs_is_derived_gcs("invalid"))
    expect_error(srs_is_projected("invalid"))
    expect_error(srs_is_compound("invalid"))
    expect_error(srs_is_geocentric("invalid"))
    expect_error(srs_is_vertical("invalid"))
    expect_error(srs_is_same("invalid", "invalid"))
    expect_error(srs_is_same(srs, "invalid"))
    expect_true(is.null(srs_get_angular_units("")))
    expect_true(is.null(srs_get_linear_units("")))
    expect_error(srs_get_angular_units("invalid"))
    expect_error(srs_get_linear_units("invalid"))
    expect_error(srs_get_utm_zone("invalid"))
    expect_error(srs_get_axis_mapping_strategy("invalid"))


    # dynamic srs GDAL >= 3.4
    skip_if(gdal_version_num() < 3040000)

    expect_true(srs_is_dynamic("EPSG:4326"))
    expect_false(srs_is_dynamic("EPSG:4171"))
    expect_error(srs_is_dynamic("invalid"))
    expect_equal(srs_get_coord_epoch("WGS84"), 0.0)
    expect_error(srs_get_coord_epoch("invalid"))
})

