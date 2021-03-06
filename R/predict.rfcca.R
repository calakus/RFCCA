#' Predict method for rfcca objects
#'
#' Obtain predicted canonical correlations using a rfcca forest for training or
#'   new data.
#'
#' @param object An object of class \code{(rfcca,grow)} created by the function
#'   \code{rfcca}.
#' @param newdata Test data of the set of subject-related covariates (Z). A
#'  data.frame with numeric values and factors. If missing, the out-of-bag
#'  predictions in \code{object} is returned.
#' @param membership Should terminal node membership information be returned?
#' @param finalcca Which CCA should be used for final canonical correlation
#'   estimation? Choices are \code{cca}, \code{scca} and \code{rcca}, see
#'   \code{\link{rfcca}} for details. The default is \code{cca}.
#' @param ... Optional arguments to be passed to other methods.
#'
#' @return An object of class \code{(rfcca,predict)} which is a list with the
#'   following components:
#'
#'   \item{call}{The original grow call to \code{rfcca}.}
#'   \item{n}{Sample size of the test data (\code{NA}'s are omitted). If
#'     \code{newdata} is missing, sample size of the training set.}
#'   \item{ntree}{Number of trees grown.}
#'   \item{xvar}{Data frame of x-variables.}
#'   \item{xvar.names}{A character vector of the x-variable names.}
#'   \item{yvar}{Data frame of y-variables.}
#'   \item{yvar.names}{A character vector of the y-variable names.}
#'   \item{zvar}{Data frame of test z-variables. If \code{newdata} is missing,
#'     data frame of training z-variables.}
#'   \item{zvar.names}{A character vector of the z-variable names.}
#'   \item{forest}{The \code{(rfcca,grow)} forest.}
#'   \item{membership}{A matrix recording terminal node membership for the test
#'     data where each cell represents the node number that an observation falls
#'     in for that tree.}
#'   \item{predicted}{Test set predicted canonical correlations based on the
#'     selected final canonical correlation estimation method. If \code{newdata}
#'     is missing, OOB predictions for training observations.}
#'   \item{predicted.coef}{Predicted canonical weight vectors for x- and y-
#'     variables.}
#'   \item{finalcca}{The selected CCA used for final canonical correlation
#'     estimations.}
#'
#' @examples
#' \donttest{
#' ## load generated example data
#' data(data, package = "RFCCA")
#' set.seed(2345)
#'
#' ## define train/test split
#' smp <- sample(1:nrow(data$X), size = round(nrow(data$X) * 0.7),
#'   replace = FALSE)
#' train.data <- lapply(data, function(x) {x[smp, ]})
#' test.Z <- data$Z[-smp, ]
#'
#' ## train rfcca
#' rfcca.obj <- rfcca(X = train.data$X, Y = train.data$Y, Z = train.data$Z,
#'   ntree = 100)
#'
#' ## predict without new data (OOB predictions will be returned)
#' pred.obj <- predict(rfcca.obj)
#' pred.oob <- pred.obj$predicted
#'
#' ## predict with new test data
#' pred.obj2 <- predict(rfcca.obj, newdata = test.Z)
#' pred <- pred.obj2$predicted
#'
#' ## print predict objects
#' print(pred.obj)
#' print(pred.obj2)
#' }
#'
#' @seealso
#'   \code{\link{rfcca}}
#'   \code{\link{vimp.rfcca}}
#'   \code{\link{print.rfcca}}

predict.rfcca <- function(object,
                          newdata,
                          membership = FALSE,
                          finalcca = c("cca", "scca", "rcca"),
                          ...)
{
  result.predict <- generic.predict.rfcca(object,
                                          newdata,
                                          membership = membership,
                                          finalcca = finalcca,
                                          ...)
  return(result.predict)
}
