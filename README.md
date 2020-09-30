# RFCCA
R package which implements **R**andom **F**orest with **C**anonical **C**orrelation **A**nalysis (**RFCCA**).

**RFCCA** is a random forest method for estimating the canonical correlations between two sets of variables, *X* and *Y*, depending on 
the subject-related covariates, *Z*. The trees are built with a splitting rule specifically designed to partition the data to maximize 
the canonical correlation heterogeneity between child nodes.

For theoretical details and example data analysis, you can look at the vignette from within `R` by using the following command:

```R
vignette("RFCCA")
```

## Installation
You can install **RFCCA** from GitHub using the devtools package. Run the following code in `R` to install:

```R
if (!require(devtools)) {
  install.packages("devtools")
  library(devtools)
}
devtools::install_github('calakus/RFCCA', build_vignettes = TRUE)
```   
