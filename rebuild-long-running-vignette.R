# The idea of pre-computing long-running vignettes is from here:
# https://www.kloppenborg.ca/2021/06/long-running-vignettes/

old_wd <- getwd()

setwd("vignettes")
knitr::knit("RFCCA.Rmd.orig", output = "RFCCA.Rmd")
knitr::purl("RFCCA.Rmd.orig", output = "RFCCA.R")

setwd(old_wd)
