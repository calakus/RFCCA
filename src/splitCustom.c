#define USE_FC_LEN_T
#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>

#include <stdlib.h>
#include <math.h>
#include "splitCustom.h"

#include <R_ext/Lapack.h>
#include <R_ext/BLAS.h>

#ifndef FCONE
# define FCONE
#endif


void registerCustomFunctions(void) {

  // Register the custom classification split rule in the first slot.
  registerThis (&getCustomSplitStatisticMultivariateClassification, CLAS_FAM, 1);

  // Register the custom regression split rule in the first slot.
  registerThis (&getCustomSplitStatisticMultivariateRegression, REGR_FAM, 1);

  // Register the custom survival split rule in the first slot.
  registerThis (&getCustomSplitStatisticSurvival, SURV_FAM, 1);

  // Register the custom competing risk split rule in the first slot.
  registerThis (&getCustomSplitStatisticCompetingRisk, CRSK_FAM, 1);

  // If you have more than one function of each type, you would
  // uncomment one or more of the following statements and complete
  // the coding for the named functions.  Note that you can call these
  // functions anything you want.  In a multivariate scenario you must
  // be sure that the index of both the classification and regression
  // split rule are identical.  For example, in a multivariate
  // scenario, splitrule = "custom2", will call the classification
  // split rule in the second slot, along with the regression split
  // rule in the second slot if the respones contain factors and reals.

    registerThis (&ccaSplitAbsoluteDifference, REGR_FAM, 2);
//    registerThis (&getCustomSplitStatisticMultivariateClassificationTwo, CLAS_FAM, 2);
//    registerThis (&getCustomSplitStatisticMultivariateClassificationThree, CLAS_FAM, 3);
    

}


/*
  Generic Custom Split Rule Harness

  FUNCTION INPUTS:
  n - number of replicates in the parent node to be split

  membership     - vector of length n representing daughter node
  membership (LEFT or RIGHT) as a result of the
  test split.

  time     - vector of length n of time variable for survival data sets.
  This will be NULL for non-survival data sets.
  event   - vector of length n of event/censoring variable for
  survival data sets.  This will be NULL for non-survival
  data sets.

  response - vector of length n of response variable (y-outcome) in the
  node. In the multivariate case this function will be called 
  once for each response.  The multivariate split statistic 
  is the sum of the individual response statistic.  It is 
  important to normalize the statistic for each response to 
  avoid one response overwhelming another's value.  This will 
  be NULL for survival data sets.

  mean     - convenience value representing the mean of the response vector
  variance - convenience value representing the variance of the response vector

  maxLevel - convenience value representing the maximum level for this factor respone
  in the data set as a whole.  This will be NULL for non-factor respones.

  feature - matrix of user specified features that can be sent into
  the split rule and acted on as desired.  The matrix is of dimension
  [featureCount] x [n].  Features are neither y-variables or
  x-variables. However, for expediency, they are specified by the user
  as y-variables, but are tagged as having zero weight via the
  y-variable weight vector.  Thus, they are never used in the the
  pre-defined split rules, and have no predicted value. The pointer will be NULL
  when features are absent.

  featureCount - count of features in the above matrix, specifically the
  number of rows.  The count will be zero when features are absent.


  FUNCTION OUTPUT:  returns a double value representing the custom split statistic


  Depending on the split statistic, the user may need to allocate and 
  de-allocate arrays of various dimensions.  A typical alloc/de-alloc is
  defined in this file:

  unsigned int *alloc_uivector(...)
  void        dealloc_uivector(...)

  Always remember to deallocate what has been allocated.

  Always remember to declare the function in the corresponding ".h" file.

*/

double getCustomSplitStatisticMultivariateRegression (unsigned int n,
                                                      char        *membership,
                                                      double      *time,
                                                      double      *event,

                                                      unsigned int eventTypeSize,
                                                      unsigned int eventTimeSize,
                                                      double      *eventTime,

                                                      double      *response,
                                                      double       mean,
                                                      double       variance,
                                                      unsigned int maxLevel,

                                                      double     **feature,
                                                      unsigned int featureCount)
{

  // EXAMPLE:  Multivariate Regression

  // Local variables needed for this example:
  double sumLeft, sumRght;
  double sumLeftSqr, sumRghtSqr;
  double delta;

  // Left and right normalization sizes.
  unsigned int leftSize, rghtSize;

  unsigned int i;

  // Initialization of local variables:
  sumLeft = sumRght = 0.0;
  leftSize = rghtSize = 0;

  delta = 0.0;

  // In general, calculating a split statistic will require iterating
  // over all members in the parent node, and ascertaining daughter
  // membership, and performing a well defined calculation based on
  // membership.  In this example, the sum of the difference from the
  // mean for the y-outcome in each daughter node is calculated.

  for (i = 1; i <= n; i++) {
    // Membership will be either LEFT or RIGHT.
    if (membership[i] == LEFT) {
      // Add the left member to the sum.
      sumLeft += response[i] - mean;
      leftSize ++;
    }
    else {
      // Add the right member to the sum.
      sumRght += response[i] - mean;
      rghtSize ++;
    }
  }

  // Finally, we calculate the composite mean square error for each daughter.
  sumLeftSqr = pow(sumLeft, 2.0) / ((double) leftSize * variance);
  sumRghtSqr = pow(sumRght, 2.0) / ((double) rghtSize * variance);

  delta = sumLeftSqr + sumRghtSqr;

  return delta;
}

double getCustomSplitStatisticMultivariateClassification (unsigned int n,
                                                          char        *membership,
                                                          double      *time,
                                                          double      *event,

                                                          unsigned int eventTypeSize,
                                                          unsigned int eventTimeSize,
                                                          double      *eventTime,
                                                          
                                                          double      *response,
                                                          double       mean,
                                                          double       variance,
                                                          unsigned int maxLevel,

                                                          double     **feature,
                                                          unsigned int featureCount)
{

  // EXAMPLE:  Multivariate Classification

  // Local variables needed for this example:
  unsigned int   *leftClassProp, *rghtClassProp;
  double sumLeftSqr, sumRghtSqr;
  double delta;

  // Arrays needed for multivariate regression:
  double sumLeft, sumRght;

  // Left and right normalization sizes.
  unsigned int leftSize, rghtSize;

  unsigned int i, p;

  // Initialization of local variables:
  sumLeft = sumRght = 0.0;
  leftSize = rghtSize = 0;

  delta = 0.0;

  //---------- *** WARNING MEMORY MANIPULATION WARNING *** -----------//
  // Vector for class counts.
  leftClassProp = alloc_uivector(maxLevel);
  rghtClassProp = alloc_uivector(maxLevel);
  //---------- *** WARNING MEMORY MANIPULATION WARNING *** -----------//

  // Initialize the class counts.
  for (p = 1; p <= maxLevel; p++) {
    leftClassProp[p] = rghtClassProp[p] = 0;
  }
  
  // In general, calculating a split statistic will require iterating
  // over all members in the parent node, and ascertaining daughter
  // membership, and performing a well defined calculation based on
  // membership.  In this example, the left and right class counts for
  // the y-outcome in each daughter node are calculated.
  for (i = 1; i <= n; i++) {
    // Membership will be either LEFT or RIGHT.  Package specific
    // constants are contained in global.h.
    if (membership[i] == LEFT) {

      // Add the left member to the left class proportion.
      leftClassProp[(unsigned int) response[i]] ++;
      leftSize ++;

    }
    else {

      // Add the right member to the left class proportion.
      rghtClassProp[(unsigned int) response[i]] ++;
      rghtSize ++;

    }
  }

  for (p = 1; p <= maxLevel; p++) {
    sumLeft += pow((double) leftClassProp[p], 2.0);
    sumRght += pow((double) rghtClassProp[p], 2.0);
  }

  //---------- *** WARNING MEMORY MANIPULATION WARNING *** -----------//
  // Vector for class counts.
  dealloc_uivector(leftClassProp, maxLevel);
  dealloc_uivector(rghtClassProp, maxLevel);
  //---------- *** WARNING MEMORY MANIPULATION WARNING *** -----------//

  sumLeftSqr = sumLeft / leftSize;
  sumRghtSqr = sumRght / rghtSize;
  
  
  delta = sumLeftSqr + sumRghtSqr;

  return delta;
}

// TBD TBD put documentation in for eventTime.

double getCustomSplitStatisticSurvival (unsigned int n,
                                        char        *membership,
                                        double      *time,
                                        double      *event,

                                        unsigned int eventTypeSize,
                                        unsigned int eventTimeSize,
                                        double      *eventTime,

                                        double      *response,
                                        double       mean,
                                        double       variance,
                                        unsigned int maxLevel,

                                        double     **feature,
                                        unsigned int featureCount)
{

  // EXAMPLE:  Survival (logrank)

  // unsigned int eventTimeSize
  //   - number of unique event times in the node.
  // double *eventTime
  //   - vector of sorted uniquified times of death in the node, of
  //     length eventTimeSize.
  // unsigned int eventTypeSize
  //   - number of unique event types, trivially one (1) in survival.

  // Local variables needed for this example:
  unsigned int   *nodeLeftEvent,  *nodeParentEvent;
  unsigned int   *nodeLeftAtRisk, *nodeParentAtRisk;

  double delta, deltaNum, deltaDen;
  unsigned int i, k;

  // Initialization of local variables:
  deltaNum = deltaDen = 0.0;

  //---------- *** WARNING MEMORY MANIPULATION WARNING *** -----------//
  // Vector for event counts.
  nodeLeftEvent   = alloc_uivector(eventTimeSize);
  nodeParentEvent = alloc_uivector(eventTimeSize);

  // Vector for at risk counts.
  nodeLeftAtRisk    = alloc_uivector(eventTimeSize);
  nodeParentAtRisk  = alloc_uivector(eventTimeSize);
  //---------- *** WARNING MEMORY MANIPULATION WARNING *** -----------//

  // Reset the node specific counts needed for the statistic.
  for (k = 1; k <= eventTimeSize; k++) {
    nodeParentEvent[k]  = 0;
    nodeParentAtRisk[k] = 0;
    nodeLeftEvent[k]    = 0;
    nodeLeftAtRisk[k]   = 0;
  }

  k = eventTimeSize;
  i = n;

  // Iterate over all individuals.  Note that they arrive sorted by time
  // in increasing order.  We parse them in decreasing order.
  while ((i > 0) && (k > 0)) {

    // Initialize the parent event count.
    if (eventTime[k] <= time[i]) {

      // The member is still at risk!
      nodeParentAtRisk[k] ++;

      // Membership will be either LEFT or RIGHT.  
      if (membership[i] == LEFT) {
        nodeLeftAtRisk[k] ++;
      }
        
      // Did the member experience an event?
      if (eventTime[k] == time[i]) {
        if (event[i] > 0) {
          nodeParentEvent[k] ++;

          // Membership will be either LEFT or RIGHT.  
          if (membership[i] == LEFT) {
            nodeLeftEvent[k] ++;
          }
        }
      }

      // Examine the previous individual.
      i--;
    }
    else {

      // Examine the previous event time.
      k--;
    }
  }

  // Adjust the at risk counts to achieve the step function.
  for (k = eventTimeSize; k > 1; k--) {
    nodeParentAtRisk[k-1] = nodeParentAtRisk[k] + nodeParentAtRisk[k-1];
    nodeLeftAtRisk[k-1] = nodeLeftAtRisk[k] + nodeLeftAtRisk[k-1];
  }

  // Iterate over the distinct event times and acquire the numerator and denominator of the test.
  for (k = 1; k <= eventTimeSize; k++) {
    deltaNum = deltaNum + ((double) nodeLeftEvent[k] - ((double) ( nodeLeftAtRisk[k] * nodeParentEvent[k]) / nodeParentAtRisk[k]));

    // Log-Rank denominator requires that there be at least two at risk.
    if (nodeParentAtRisk[k] >= 2) {
      deltaDen = deltaDen + (
                             ((double) nodeLeftAtRisk[k] / nodeParentAtRisk[k]) *
                             (1.0 - ((double) nodeLeftAtRisk[k] / nodeParentAtRisk[k])) *
                             ((double) (nodeParentAtRisk[k] - nodeParentEvent[k]) / (nodeParentAtRisk[k] - 1)) * nodeParentEvent[k]
                             );

    }
  }


  //---------- *** WARNING MEMORY MANIPULATION WARNING *** -----------//
  // Vector for event counts.
  dealloc_uivector(nodeLeftEvent, eventTimeSize);
  dealloc_uivector(nodeParentEvent, eventTimeSize);

  // Vector for at risk counts.
  dealloc_uivector(nodeLeftAtRisk, eventTimeSize);
  dealloc_uivector(nodeParentAtRisk, eventTimeSize);
  //---------- *** WARNING MEMORY MANIPULATION WARNING *** -----------//
  
  deltaNum = fabs(deltaNum);
  deltaDen = sqrt(deltaDen);
  if (deltaDen <= 1.0e-9) {
    if (deltaNum <= 1.0e-9) {
      delta = 0.0;
    }
    else {
      delta = deltaNum / deltaDen;
    }
  }
  else {
    delta = deltaNum / deltaDen;
  }

  return delta;
}


double getCustomSplitStatisticCompetingRisk (unsigned int n,
                                             char        *membership,
                                             double      *time,
                                             double      *event,

                                             unsigned int eventTypeSize,
                                             unsigned int eventTimeSize,
                                             double      *eventTime,

                                             double      *response,
                                             double       mean,
                                             double       variance,
                                             unsigned int maxLevel,
                                             
                                             double     **feature,
                                             unsigned int featureCount)
{

  // EXAMPLE:  Competing Risk (logrankCR)

  // unsigned int eventTimeSize
  //   - number of unique event times in the node.
  // double *eventTime
  //   - vector of sorted uniquified times of death in the node, of
  //     length eventTimeSize.
  // unsigned int eventTypeSize
  //   - number of unique event types in the data set.

  // Local variables needed for this example:
  unsigned int   *nodeLeftEvent,  *nodeParentEvent;
  unsigned int   *nodeLeftAtRisk, *nodeParentAtRisk;

  // Local CR variable needed for this example:
  unsigned int   **nodeLeftEventCR,  **nodeParentEventCR;
  unsigned int   **nodeLeftInclusiveAtRisk, **nodeParentInclusiveAtRisk;

  double delta, deltaNum, deltaSubNum, deltaDen, deltaSubDen;
  unsigned int i, j, k, r, s;

  // Initialization of local variables:
  deltaNum = deltaDen = 0.0;

  //---------- *** WARNING MEMORY MANIPULATION WARNING *** -----------//
  // Vector for event counts.
  nodeLeftEvent   = alloc_uivector(eventTimeSize);
  nodeParentEvent = alloc_uivector(eventTimeSize);

  // Vector for at risk counts.
  nodeLeftAtRisk    = alloc_uivector(eventTimeSize);
  nodeParentAtRisk  = alloc_uivector(eventTimeSize);
  //---------- *** WARNING MEMORY MANIPULATION WARNING *** -----------//

  //---------- *** WARNING MEMORY MANIPULATION WARNING *** -----------//
  // Matrix containing event counts at each event time.
  nodeParentEventCR = alloc_uimatrix(eventTypeSize, eventTimeSize);
  nodeLeftEventCR = alloc_uimatrix(eventTypeSize, eventTimeSize);

  // Matrix containing event inclusive at risk counts at each event time.
  nodeParentInclusiveAtRisk = alloc_uimatrix(eventTypeSize, eventTimeSize);
  nodeLeftInclusiveAtRisk = alloc_uimatrix(eventTypeSize, eventTimeSize);
  //---------- *** WARNING MEMORY MANIPULATION WARNING *** -----------//

  // Reset the node specific counts needed for the statistic.
  for (k = 1; k <= eventTimeSize; k++) {
    nodeParentEvent[k]  = 0;
    nodeParentAtRisk[k] = 0;
    nodeLeftEvent[k]    = 0;
    nodeLeftAtRisk[k]   = 0;

    for (j = 1; j <= eventTypeSize; j++) {
      nodeParentEventCR[j][k]         = 0;
      nodeLeftEventCR[j][k]           = 0;
      nodeParentInclusiveAtRisk[j][k] = 0;
      nodeLeftInclusiveAtRisk[j][k]   = 0;
    }
  }

  k = eventTimeSize;
  i = n;

  // Iterate over all individuals.  Note that they arrive sorted by time
  // in increasing order.  We parse them in decreasing order.
  while ((i > 0) && (k > 0)) {

    // Initialize the parent event count.
    if (eventTime[k] <= time[i]) {

      // The member is still at risk!
      nodeParentAtRisk[k] ++;

      // Membership will be either LEFT or RIGHT.  
      if (membership[i] == LEFT) {
        nodeLeftAtRisk[k] ++;
      }
        
      // Did the member experience an event?
      if (eventTime[k] == time[i]) {
        if (event[i] > 0) {

          nodeParentEventCR[(unsigned int) event[i]][k] ++;
          nodeParentEvent[k] ++;

          // Membership will be either LEFT or RIGHT.  
          if (membership[i] == LEFT) {
            nodeLeftEventCR[(unsigned int) event[i]][k] ++;
          }
        }
      }

      // Examine the previous individual.
      i--;
    }
    else {

      // Examine the previous event time.
      k--;
    }

  }

  // Adjust the at risk counts to achieve the step function.
  for (k = eventTimeSize; k > 1; k--) {
    nodeParentAtRisk[k-1] = nodeParentAtRisk[k] + nodeParentAtRisk[k-1];
    nodeLeftAtRisk[k-1] = nodeLeftAtRisk[k] + nodeLeftAtRisk[k-1];
  }

  // Finalize the left and right inclusive at risk counts.
  for (k = 1; k <= eventTimeSize; k++) {
    for (j = 1; j <= eventTypeSize; j++) {
      nodeParentInclusiveAtRisk[j][k] = nodeParentAtRisk[k];
      nodeLeftInclusiveAtRisk[j][k] = nodeLeftAtRisk[k];
      for (s = 1; s < k; s++) {
        for (r = 1; r <= eventTypeSize; r++) {
          if (j != r) {
            nodeParentInclusiveAtRisk[j][k]  += nodeParentEventCR[r][s];
            nodeLeftInclusiveAtRisk[j][k]  += nodeLeftEventCR[r][s];
          }
        }
      }
    }
  }

  // Iterate over the event types and distinct event times and acquire the numerator and denominator of the test.
  for (j = 1; j <= eventTypeSize; j++) {

    deltaSubNum = 0;
    for (k = 1; k <= eventTimeSize; k++) {
      deltaSubNum = deltaSubNum + (nodeLeftEventCR[j][k] - (nodeParentEventCR[j][k] * ((double) nodeLeftInclusiveAtRisk[j][k] / nodeParentInclusiveAtRisk[j][k])));
    }
    deltaNum = deltaNum + deltaSubNum;

    deltaSubDen = 0;
    for (k = 1; k <= eventTimeSize; k++) {

      // Log-Rank CR denominator requires that there be at least two at risk.
      if (nodeParentAtRisk[k] >= 2) {
        deltaSubDen = deltaSubDen  + (
                                      (nodeParentEventCR[j][k] * ((double) nodeLeftInclusiveAtRisk[j][k] / nodeParentInclusiveAtRisk[j][k])) *
                                      (1.0 - ((double) nodeLeftInclusiveAtRisk[j][k] / nodeParentInclusiveAtRisk[j][k])) *
                                      ((double) (nodeParentInclusiveAtRisk[j][k] - nodeParentEventCR[j][k]) / (nodeParentInclusiveAtRisk[j][k] - 1))
                                      );
      }
    }
    deltaDen = deltaDen + deltaSubDen;
  }


  
  //---------- *** WARNING MEMORY MANIPULATION WARNING *** -----------//
  // Vector for event counts.
  dealloc_uivector(nodeLeftEvent, eventTimeSize);
  dealloc_uivector(nodeParentEvent, eventTimeSize);

  // Vector for at risk counts.
  dealloc_uivector(nodeLeftAtRisk, eventTimeSize);
  dealloc_uivector(nodeParentAtRisk, eventTimeSize);
  //---------- *** WARNING MEMORY MANIPULATION WARNING *** -----------//

  //---------- *** WARNING MEMORY MANIPULATION WARNING *** -----------//
  // Matrix containing event counts at each event time.
  dealloc_uimatrix(nodeParentEventCR, eventTypeSize, eventTimeSize);
  dealloc_uimatrix(nodeLeftEventCR, eventTypeSize, eventTimeSize);

  // Matrix containing event inclusive at risk counts at each event time.
  dealloc_uimatrix(nodeParentInclusiveAtRisk, eventTypeSize, eventTimeSize);
  dealloc_uimatrix(nodeLeftInclusiveAtRisk, eventTypeSize, eventTimeSize);
  //---------- *** WARNING MEMORY MANIPULATION WARNING *** -----------//

  deltaNum = fabs(deltaNum);
  deltaDen = sqrt(deltaDen);
  if (deltaDen <= 1.0e-9) {
    if (deltaNum <= 1.0e-9) {
      delta = 0.0;
    }
    else {
      delta = deltaNum / deltaDen;
    }
  }
  else {
    delta = deltaNum / deltaDen;
  }

  return delta;
}

double ccaSplitAbsoluteDifference(unsigned int n,
                                  char        *membership,
                                  double      *time,
                                  double      *event,

                                  unsigned int eventTypeSize,
                                  unsigned int eventTimeSize,
                                  double      *eventTime,

                                  double      *response,
                                  double       mean,
                                  double       variance,
                                  unsigned int maxLevel,

                                  double     **feature,
                                  unsigned int featureCount)
{
    double ccaCor = 0.00, ccaCorRight = 0.00, ccaCorLeft = 0.00;
    int leftSize = 0, rghtSize= 0;
    int i, rowLeft = 0, rowRight = 0;
    int dimX, dimY;
    
    dimX = feature[featureCount][1];
    dimY = featureCount - dimX - 1;
    
    int minDim = dimY;
    if(dimX < dimY){
        minDim = dimX;
    }
    
    if(dimX > 0 && dimY > 0){
        // Initialization of local variables:
        leftSize = rghtSize = 0;
        for (i = 1; i <= n; i++) {
          // Membership will be either LEFT or RIGHT.
          if (membership[i] == LEFT) {leftSize ++;}
          else {rghtSize ++;}
        }
        
        if( (leftSize > (dimX+dimY)) && (rghtSize > (dimX+dimY)) ){
            // Initializion of left and right X and Y datasets
            double leftX[leftSize][dimX], leftY[leftSize][dimY];
            double rightX[rghtSize][dimX], rightY[rghtSize][dimY];
            
            // Assignment of left and right X and Y
            for (i = 1; i <= n; i++) {
                if(membership[i] == LEFT) {
                    for(int col = 1; col <= dimX; col++){
                        leftX[rowLeft][col-1] = feature[col][i];
                    }
                    for(int col = (dimX+1); col <= (dimX+dimY); col++){
                        leftY[rowLeft][col-dimX-1] = feature[col][i];
                    }
                    rowLeft++;
                }
                else{
                    for(int col = 1; col <= dimX; col++){
                        rightX[rowRight][col-1] = feature[col][i];
                    }
                    for(int col = (dimX+1); col <= (dimX+dimY); col++){
                        rightY[rowRight][col-dimX-1] = feature[col][i];
                    }
                    rowRight++;
                }
            }
            
            char transa = 'T', transb = 'N';
            char jobu = 'N', jobvt = 'N';
            int ldu = 1, ldvt = 1;
            double alpha = 1, beta = 0;
            int info, lwork;
            
            // CCA for left
            int nRow = leftSize;
            int lda = nRow;
            int Kx = dimX, Ky = dimY;
            if(nRow < dimX){Kx = nRow;}
            if(nRow < dimY){Ky = nRow;}
            
            double *qrX = alloc_dvector(leftSize * dimX);
            double *qrY = alloc_dvector(leftSize * dimY);
            double *tauX = alloc_dvector(Kx);
            double *tauY = alloc_dvector(Ky);
            double *qMul = alloc_dvector(Kx * Ky);
            double *S = alloc_dvector(minDim);
            
            for(int row = 0; row < leftSize; row++){
                for(int col = 0; col < dimX; col++){
//                    qrX[row * dimX + col] = leftX[row][col];
                    qrX[row + col * leftSize] = leftX[row][col];
                }
                for(int col = 0; col < dimY; col++){
                    qrY[row + col * leftSize] = leftY[row][col];
                }
            }

            double *work = NULL;
            double work_query;
            
            info = 0, lwork = -1;
            F77_CALL(dgeqrf)(&nRow, &dimX, qrX, &lda, tauX, &work_query, &lwork, &info);
            lwork = (int)work_query;
            work = (double*)malloc(sizeof(double) * lwork);
//            double *work = (double *)malloc(lwork * sizeof(double));
//            double *work = alloc_dvector(1);
            F77_CALL(dgeqrf)(&nRow, &dimX, qrX, &lda, tauX, work, &lwork, &info);
            free(work);
            
            info = 0, lwork = -1;
            F77_CALL(dorgqr)(&nRow, &Kx, &Kx, qrX, &lda, tauX, &work_query, &lwork, &info);
            lwork = (int)work_query;
            work = (double*)malloc(sizeof(double) * lwork);
            F77_CALL(dorgqr)(&nRow, &Kx, &Kx, qrX, &lda, tauX, work, &lwork, &info);
            free(work);
            
            info = 0, lwork = -1;
            F77_CALL(dgeqrf)(&nRow, &dimY, qrY, &lda, tauY, &work_query, &lwork, &info);
            lwork = (int)work_query;
            work = (double*)malloc(sizeof(double) * lwork);
            F77_CALL(dgeqrf)(&nRow, &dimY, qrY, &lda, tauY, work, &lwork, &info);
            free(work);
            
            info = 0, lwork = -1;
            F77_CALL(dorgqr)(&nRow, &Ky, &Ky, qrY, &lda, tauY, &work_query, &lwork, &info);
            lwork = (int)work_query;
            work = (double*)malloc(sizeof(double) * lwork);
            F77_CALL(dorgqr)(&nRow, &Ky, &Ky, qrY, &lda, tauY, work, &lwork, &info);
            free(work);
            
            F77_CALL(dgemm)(&transa, &transb, &Kx, &Ky, &nRow, &alpha, qrX, &nRow, qrY, &nRow, &beta, qMul, &Kx FCONE FCONE);
            
            info = 0, lwork = -1;
            F77_CALL(dgesvd)(&jobu, &jobvt, &Kx, &Ky, qMul, &Kx, S, NULL, &ldu, NULL, &ldvt, &work_query, &lwork, &info FCONE FCONE);
            lwork = (int)work_query;
            work = (double*)malloc(sizeof(double) * lwork);
            F77_CALL(dgesvd)(&jobu, &jobvt, &Kx, &Ky, qMul, &Kx, S, NULL, &ldu, NULL, &ldvt, work, &lwork, &info FCONE FCONE);
            
            if (info == 0) {
                ccaCorLeft = S[0];
            } else if (info > 0) {
                ccaCorLeft = S[0];
                for (int i = 1; i < lwork; i++) {
                    if (work[i] > ccaCorLeft) {
                        ccaCorLeft = work[i];
                    }
                }
            }
            free(work);
            
            dealloc_dvector(qrX);
            dealloc_dvector(qrY);
            dealloc_dvector(qMul);
            dealloc_dvector(tauX);
            dealloc_dvector(tauY);
            
            // CCA for right
            nRow = rghtSize;
            lda = nRow;
            Kx = dimX, Ky = dimY;
            if(nRow < dimX){Kx = nRow;}
            if(nRow < dimY){Ky = nRow;}
            
            qrX = alloc_dvector(rghtSize * dimX);
            qrY = alloc_dvector(rghtSize * dimY);
            tauX = alloc_dvector(Kx);
            tauY = alloc_dvector(Ky);
            qMul = alloc_dvector(Kx * Ky);
            
            for(int row = 0; row < rghtSize; row++){
                for(int col = 0; col < dimX; col++){
                    qrX[row + col * rghtSize] = rightX[row][col];
                }
                for(int col = 0; col < dimY; col++){
                    qrY[row + col * rghtSize] = rightY[row][col];
                }
            }
            
            info = 0, lwork = -1;
            F77_CALL(dgeqrf)(&nRow, &dimX, qrX, &lda, tauX, &work_query, &lwork, &info);
            lwork = (int)work_query;
            work = (double*)malloc(sizeof(double) * lwork);
            F77_CALL(dgeqrf)(&nRow, &dimX, qrX, &lda, tauX, work, &lwork, &info);
            free(work);
            
            info = 0, lwork = -1;
            F77_CALL(dorgqr)(&nRow, &Kx, &Kx, qrX, &lda, tauX, &work_query, &lwork, &info);
            lwork = (int)work_query;
            work = (double*)malloc(sizeof(double) * lwork);
            F77_CALL(dorgqr)(&nRow, &Kx, &Kx, qrX, &lda, tauX, work, &lwork, &info);
            free(work);
            
            info = 0, lwork = -1;
            F77_CALL(dgeqrf)(&nRow, &dimY, qrY, &lda, tauY, &work_query, &lwork, &info);
            lwork = (int)work_query;
            work = (double*)malloc(sizeof(double) * lwork);
            F77_CALL(dgeqrf)(&nRow, &dimY, qrY, &lda, tauY, work, &lwork, &info);
            free(work);
            
            info = 0, lwork = -1;
            F77_CALL(dorgqr)(&nRow, &Ky, &Ky, qrY, &lda, tauY, &work_query, &lwork, &info);
            lwork = (int)work_query;
            work = (double*)malloc(sizeof(double) * lwork);
            F77_CALL(dorgqr)(&nRow, &Ky, &Ky, qrY, &lda, tauY, work, &lwork, &info);
            free(work);
            
            F77_CALL(dgemm)(&transa, &transb, &Kx, &Ky, &nRow, &alpha, qrX, &nRow, qrY, &nRow, &beta, qMul, &Kx FCONE FCONE);
            
            info = 0, lwork = -1;
            F77_CALL(dgesvd)(&jobu, &jobvt, &Kx, &Ky, qMul, &Kx, S, NULL, &ldu, NULL, &ldvt, &work_query, &lwork, &info FCONE FCONE);
            lwork = (int)work_query;
            work = (double*)malloc(sizeof(double) * lwork);
            F77_CALL(dgesvd)(&jobu, &jobvt, &Kx, &Ky, qMul, &Kx, S, NULL, &ldu, NULL, &ldvt, work, &lwork, &info FCONE FCONE);
            
            if (info == 0) {
                ccaCorRight = S[0];
            } else if (info > 0) {
                ccaCorRight = S[0];
                for (int i = 1; i < lwork; i++) {
                    if (work[i] > ccaCorRight) {
                        ccaCorRight = work[i];
                    }
                }
            }
            free(work);
         
            dealloc_dvector(qrX);
            dealloc_dvector(qrY);
            dealloc_dvector(qMul);
            dealloc_dvector(tauX);
            dealloc_dvector(tauY);
            dealloc_dvector(S);
            
            ccaCor = sqrt(leftSize * rghtSize) * fabs(ccaCorLeft - ccaCorRight);
         }
    }

    return ccaCor;
}

/*
  Memory allocation and deallocation.
  Multi-dimensional array allocationis 
  accomplished via multiple one-dimensional
  array allocations.
*/

unsigned int *alloc_uivector(unsigned int nh)
{
  return (unsigned int *) malloc((size_t) ((nh+1) * (sizeof(unsigned int))));
}

void dealloc_uivector(unsigned int *v, unsigned int nh)
{
  free((char *) v);
}

unsigned int **alloc_uimatrix(unsigned int n2h, unsigned int nh)
{
  unsigned int **v = (unsigned int **) malloc((size_t) ((n2h+1) * (sizeof(unsigned int *))));

  for (unsigned int i = 1; i <= n2h; i++) {
    v[i] = alloc_uivector(nh);
  }
  return v;
}

void dealloc_uimatrix(unsigned int **v, unsigned int n2h, unsigned int nh)
{
  for (unsigned int i = 1; i <= n2h; i++) {
    dealloc_uivector(v[i], nh);
  }
  free((char *) v);
}

double *alloc_dvector(unsigned int nh)
{
  return (double *) malloc((size_t) (nh * (sizeof(double))));
}

void dealloc_dvector(double *v)
{
  free((char *) v);
}
