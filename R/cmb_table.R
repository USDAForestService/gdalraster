#' @name CmbTable
#'
#' @aliases
#' Rcpp_CmbTable Rcpp_CmbTable-class CmbTable-class
#'
#' @title Hash table class for counting unique combinations of integers
#'
#' @description
#' Implements a hash table having a vector of integers as the key, and 
#' the count of occurrences of each unique integer combination as the value.
#' A unique ID is assigned to each unique combination of input values.
#'
#' @section Usage:
#' \preformatted{
#' cmb <- new(CmbTable, keyLen, varNames)
#' 
#' cmb$update(int_cmb, incr)
#' cmb$updateFromMatrix(int_cmbs, incr)
#' cmb$asDataFrame()
#' }
#'
#' @section Details:
#'
#' \code{new(CmbTable, keyLen, varNames)}
#' Creates a new CmbTable object.
#' \code{keyLen} is the number of integer values comprising each combination.
#' \code{varNames} is a character vector of names for the variables in
#' the combination. Returns an object of class CmbTable.
#'
#' \code{$update(int_cmb, incr)}
#' Updates the hash table for the integer combination in the numeric vector 
#' \code{int_cmb} (coerced to integer by truncation). 
#' If this combination exists in the table, its count will be
#' incremented by \code{incr}. If the combination is not found in the table,
#' it will be inserted with count set to \code{incr}.
#' Returns the unique ID assigned to this combination.
#' Combination IDs are sequential integers starting at 1.
#'
#' \code{$updateFromMatrix(int_cmbs, incr)}
#' This method is the same as \code{$update()} but for a numeric matrix of 
#' integer combinations \code{int_cmbs} (coerced to integer by truncation). 
#' The matrix is arranged with each column forming a vector integer 
#'  combination. For example, the rows of the matrix could be 
#' one row each from a set of \code{keyLen} rasters all read at the 
#' same extent and pixel resolution (i.e., row-by-row raster overlay).
#' The method calls \code{$update()} on each combination (each column of
#' \code{int_cmbs}), incrementing count by \code{incr} for existing 
#' combinations, or inserting new combinations with count set to \code{incr}.
#' Returns a vector of length \code{ncol(int_cmbs)} containing the 
#' combination IDs assigned to the combinations in \code{int_cmbs}.
#'
#' \code{$asDataFrame()}
#' Returns the CmbTable as a data frame with column \code{cmbid} containing 
#' the unique combination IDs, column \code{count} containing the counts of 
#' occurrences, and \code{keyLen} columns named \code{varNames} containing
#' the integer values comprising each unique combination.
#'
#' @examples
#' m <- matrix(c(1,2,3,1,2,3,4,5,6,1,3,2,4,5,6,1,1,1), 3, 6, byrow=FALSE)
#' row.names(m) <- c("v1","v2","v3")
#' print(m)
#' cmb <- new(CmbTable, 3, row.names(m))
#' cmb$updateFromMatrix(m, 1)
#' cmb$asDataFrame()
#' cmb$update(c(4,5,6), 1)
#' cmb$update(c(1,3,5), 1)
#' cmb$asDataFrame()
NULL

Rcpp::loadModule("mod_cmb_table", TRUE)
