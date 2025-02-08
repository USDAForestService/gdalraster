test_that("data type convenience functions work", {
    dt <- "Byte"
    expect_equal(dt_size(dt), 1)
    expect_equal(dt_size(dt, FALSE), 8)
    expect_false(dt_is_complex(dt))
    expect_true(dt_is_integer(dt))
    expect_false(dt_is_floating(dt))
    expect_false(dt_is_signed(dt))

    dt <- "int16"  # case-insensitive
    expect_equal(dt_size(dt), 2)
    expect_equal(dt_size(dt, FALSE), 16)
    expect_false(dt_is_complex(dt))
    expect_true(dt_is_integer(dt))
    expect_false(dt_is_floating(dt))
    expect_true(dt_is_signed(dt))

    dt <- "UInt32"
    expect_equal(dt_size(dt), 4)
    expect_equal(dt_size(dt, FALSE), 32)
    expect_false(dt_is_complex(dt))
    expect_true(dt_is_integer(dt))
    expect_false(dt_is_floating(dt))
    expect_false(dt_is_signed(dt))

    dt <- "Float64"
    expect_equal(dt_size(dt), 8)
    expect_equal(dt_size(dt, FALSE), 64)
    expect_false(dt_is_complex(dt))
    expect_false(dt_is_integer(dt))
    expect_true(dt_is_floating(dt))
    expect_true(dt_is_signed(dt))

    dt <- "CFloat32"
    expect_equal(dt_size(dt), 8)
    expect_equal(dt_size(dt, FALSE), 64)
    expect_true(dt_is_complex(dt))
    expect_false(dt_is_integer(dt))
    expect_true(dt_is_floating(dt))
    expect_true(dt_is_signed(dt))

    dt <- "dt_unknown"
    expect_equal(dt_size(dt), 0)
    expect_equal(dt_size(dt, FALSE), 0)
    expect_false(dt_is_complex(dt))
    expect_false(dt_is_integer(dt))
    expect_false(dt_is_floating(dt))
    # fails on macos with GDAL 3.5.3:
    # expect_false(dt_is_signed(dt))

    expect_equal(dt_union("Byte", "Int16"), "Int16")
    expect_equal(dt_union_with_value("Byte", -1), "Int16")
    expect_equal(dt_union_with_value("Byte", 256), "UInt16")
    expect_equal(dt_union_with_value("Float32", -99999.9876), "Float64")

    expect_equal(dt_find(bits = 32, is_signed = FALSE, is_floating = FALSE),
                 "UInt32")

    expect_equal(dt_find_for_value(0), "Byte")
    expect_equal(dt_find_for_value(NaN), "Float64")
    expect_equal(dt_find_for_value(.Machine$integer.max), "UInt32")
    expect_equal(dt_find_for_value(0.5), "Float32")
    expect_equal(dt_find_for_value(-99999.9876), "Float64")
    expect_equal(dt_find_for_value(-99999.9876, is_complex = TRUE), "CFloat64")
})
