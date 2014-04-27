#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "mex.h"
#include "matrix.h"
#include "legendre.h"
#include "fem1d.h"


void mexFunction(int nlhs, mxArray *plhs[],
int nrhs, const mxArray *prhs[])
{
    double *IM, *ur, *vr;

    Index p, N, P;    
    
    p = (Index)mxGetScalar(prhs[0]);
    N = (Index)mxGetScalar(prhs[1]);
    P = (Index)mxGetScalar(prhs[2]);
    IM = mxGetPr(prhs[3]);
    ur = mxGetPr(prhs[4]);    
    
    bool s = mxIsComplex(prhs[4]);
    Index tmp = P*N+1;
    if (s){
        double* ui  =   mxGetPi(prhs[4]);
        plhs[0] = mxCreateDoubleMatrix( tmp, 1, mxCOMPLEX);
        vr = mxGetPr(plhs[0]);
        double* vi = mxGetPi(plhs[0]);
        fem_dilt_ColMajor(p, N, P, IM, ur, vr);
        fem_dilt_ColMajor(p, N, P, IM, ui, vi);
    } else {
        plhs[0] = mxCreateDoubleMatrix( tmp, 1, mxREAL);
        vr = mxGetPr(plhs[0]);
        fem_dilt_ColMajor(p, N, P, IM, ur, vr);
    }    
    
}