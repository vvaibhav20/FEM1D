/*============================================================================+/
 | File: CUnit_pfem1d.c 
 | Description: Test 
 | These tests do not measure the performance gain but correctness of the library 
 | functions as compared to that of serial implementation: fem1d.c.
 |
 |
/+============================================================================*/
#include <pthread.h>
#include <math.h>
#include <time.h>
#include <complex.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

/* MKL */ 
#include "mkl.h"
#include "omp.h"

//#define NDEBUG
#define MATLIB_NTRACE_DATA

#include "legendre.h"
#include "fem1d.h"
#include "pfem1d.h"
#include "pthpool.h"
#include "assert.h"

/* CUnit modules */
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <CUnit/TestDB.h>

static const double TOL = 1e-9;
#define MAX_NUM_CPU 7


/*============================================================================*/
int init_suite(void)
{
      return 0;
}
int clean_suite(void)
{
      return 0;
}

/*============================================================================*/
void Gaussian
( 
    matlib_dv x, 
    matlib_dv u
)
{

    debug_enter("%s", "");
    
    double *xptr;

    for (xptr = x.elem_p; xptr<(x.elem_p+x.len); xptr++)
    {
        *(u.elem_p) = exp(-*xptr**xptr/4.0);
        u.elem_p++;
    }
    debug_exit("%s", "");
}


void test_pfem1d_DFLT_general(matlib_index p)
{
    
    /* define number of timing experiments and repeatitions */ 
    matlib_index num_exp    = 5;
    matlib_index num_cycles = 10;

    /* Create pthreads */
    matlib_index num_threads = 5;
    pthpool_data_t mp[num_threads];
    
    pthpool_create_threads(num_threads, mp);

    matlib_index i, j;


    matlib_index N, N0 = 200;
    matlib_index P = 4*p;

    /* define the domain */ 
    double x_l = -5.0;
    double x_r =  5.0;

    matlib_dv xi, quadW;
    legendre_LGLdataLT1( P, TOL, &xi, &quadW);
    
    matlib_dm FM, IM;

    matlib_create_dm( p+1, xi.len, &FM, MATLIB_ROW_MAJOR, MATLIB_NO_TRANS);    
    matlib_create_dm( xi.len, p+1, &IM, MATLIB_COL_MAJOR, MATLIB_NO_TRANS);    

    legendre_LGLdataFM( xi, FM);
    legendre_LGLdataIM( xi, IM);

    matlib_dv x, u, U, V;
    double dim;
    double norm_actual, e_relative;

    for(j=0; j<num_exp; j++)
    {
        /* generate the grid */ 
        N = (j+1)*N0;

        fem1d_ref2mesh (xi, N, x_l, x_r, &x);
        debug_body("length of x: %d", x.len);


        /* Provide the initial condition */
        matlib_create_dv( x.len, &u, MATLIB_COL_VECT);

        Gaussian(x, u);
        dim = N*(p+1);
        matlib_create_dv( dim, &U, MATLIB_COL_VECT);
        matlib_create_dv( dim, &V, MATLIB_COL_VECT);

        for(i=0; i<num_cycles; i++)
        {
            fem1d_DFLT( N, FM, u, U);
            pfem1d_DFLT(N, FM, u, V, num_threads, mp);

            BEGIN_DTRACE
                for(i=0; i<U.len; i++)
                {
                    debug_print( "[%d] -> serial: %0.16f, parallel: %0.16f", 
                                 i, U.elem_p[i], V.elem_p[i]);
                }
            END_DTRACE
            /* Analyze error */ 
            norm_actual = matlib_dnrm2(U);
            matlib_daxpy(-1.0, U, V);
            e_relative = matlib_dnrm2(V)/norm_actual;
            debug_body("Relative error: % 0.16g", e_relative);
            
            CU_ASSERT_TRUE(e_relative<TOL);
        }
        matlib_free((void*)x.elem_p);
        matlib_free((void*)u.elem_p);
        matlib_free((void*)U.elem_p);
        matlib_free((void*)V.elem_p);
    }
    
    debug_body("%s", "signal threads to exit!");
    pthpool_destroy_threads(num_threads, mp);


}
void test_pfem1d_DFLT(void)
{
    matlib_index p_max = 15;
    for (matlib_index p=2; p<p_max; p++)
    {
        test_pfem1d_DFLT_general(p);
    }
}

void test_pfem1d_DILT_general(matlib_index p)
{
    
    /* define number of timing experiments and repeatitions */ 
    matlib_index num_exp    = 5;
    matlib_index num_cycles = 10;

    /* Create pthreads */
    matlib_index num_threads = 5;
    pthpool_data_t mp[num_threads];
    
    pthpool_create_threads(num_threads, mp);

    matlib_index i, j;


    matlib_index N, N0 = 200;
    matlib_index P = 4*p;

    /* define the domain */ 
    double x_l = -5.0;
    double x_r =  5.0;

    matlib_dv xi, quadW;
    legendre_LGLdataLT1( P, TOL, &xi, &quadW);
    
    matlib_dm FM, IM;

    matlib_create_dm( p+1, xi.len, &FM, MATLIB_ROW_MAJOR, MATLIB_NO_TRANS);    
    matlib_create_dm( xi.len, p+1, &IM, MATLIB_COL_MAJOR, MATLIB_NO_TRANS);    

    legendre_LGLdataFM( xi, FM);
    legendre_LGLdataIM( xi, IM);

    matlib_dv x, u, v, U;
    double dim;
    double norm_actual, e_relative;

    for(j=0; j<num_exp; j++)
    {
        /* generate the grid */ 
        N = (j+1)*N0;

        fem1d_ref2mesh (xi, N, x_l, x_r, &x);
        debug_body("length of x: %d", x.len);


        /* Provide the initial condition */
        matlib_create_dv( x.len, &u, MATLIB_COL_VECT);
        matlib_create_dv( x.len, &v, MATLIB_COL_VECT);

        Gaussian(x, u);
        dim = N*(p+1);
        matlib_create_dv( dim, &U, MATLIB_COL_VECT);
        fem1d_DFLT( N, FM, u, U);

        for(i=0; i<num_cycles; i++)
        {
            fem1d_DILT( N, IM, U, u);
            pfem1d_DILT(N, IM, U, v, num_threads, mp);

            BEGIN_DTRACE
                for(i=0; i<U.len; i++)
                {
                    debug_print( "[%d] -> serial: %0.16f, parallel: %0.16f", 
                                 i, u.elem_p[i], v.elem_p[i]);
                }
            END_DTRACE
            /* Analyze error */ 
            norm_actual = matlib_dnrm2(u);
            matlib_daxpy(-1.0, u, v);
            e_relative = matlib_dnrm2(v)/norm_actual;
            debug_body("Relative error: % 0.16g", e_relative);
            
            CU_ASSERT_TRUE(e_relative<TOL);
        }
        matlib_free((void*)x.elem_p);
        matlib_free((void*)u.elem_p);
        matlib_free((void*)v.elem_p);
        matlib_free((void*)U.elem_p);
    }
    
    debug_body("%s", "signal threads to exit!");
    pthpool_destroy_threads(num_threads, mp);


}
void test_pfem1d_DILT(void)
{
    matlib_index p_max = 15;
    for (matlib_index p=2; p<p_max; p++)
    {
        test_pfem1d_DILT_general(p);
    }
}
/*============================================================================*/
void Gaussian_dfunc2
( 
    const matlib_dv x, 
    const matlib_dv t, 
          matlib_dm y
)
{
    matlib_index i, j;
    bool dim_OK = (y.lenc == x.len) && (t.len == y.lenr);

    if(dim_OK)
    {
        for(j=0;j<t.len; j++)
        {
            for (i=0; i<x.len; i++)
            {
                *(y.elem_p) = exp(-(0.5)*(*(x.elem_p+i)**(x.elem_p+i))/(1+*(t.elem_p+j)));
                y.elem_p++;
            }
        }
    }
}

void test_pfem1d_DFLT2_general(matlib_index p)
{

    debug_enter("polynomial degree: %d", p);
    
    /* define number of timing experiments and repeatitions */ 
    matlib_index num_exp    = 5;
    matlib_index num_cycles = 10;
    
    /* Create pthreads */
    matlib_index num_threads = 4;
    pthpool_data_t mp[num_threads];
    
    pthpool_create_threads(num_threads, mp);

    matlib_index i, j, k;

    matlib_index N, N0 = 200;
    matlib_index P = 4*p;

    /* define the domain */ 
    double x_l = -5.0;
    double x_r =  5.0;

    matlib_dv xi, quadW;
    legendre_LGLdataLT1( P, TOL, &xi, &quadW);
    
    matlib_dm FM, IM;

    matlib_create_dm( p+1, xi.len, &FM, MATLIB_ROW_MAJOR, MATLIB_NO_TRANS);    
    matlib_create_dm( xi.len, p+1, &IM, MATLIB_COL_MAJOR, MATLIB_NO_TRANS);    

    legendre_LGLdataFM( xi, FM);
    legendre_LGLdataIM( xi, IM);

    matlib_dv x, t;

    matlib_index Nt = 2;
    double dt = 1.0e-3;
    matlib_create_dv( Nt, &t, MATLIB_COL_VECT);

    t.elem_p[0] = 0;
    for(i=1; i<t.len; i++)
    {
        t.elem_p[i] = t.elem_p[i-1] + dt;
    }
    matlib_dm u, U, V;
    double dim;
    double norm_actual, e_relative;
    matlib_dv u_tmp1, u_tmp2;

    for(j=0; j<num_exp; j++)
    {
        /* generate the grid */ 
        N = (j+1)*N0;

        fem1d_ref2mesh (xi, N, x_l, x_r, &x);
        debug_body("length of x: %d", x.len);


        /* Provide the initial condition */
        matlib_create_dm( x.len, t.len, &u, MATLIB_COL_MAJOR, MATLIB_NO_TRANS);    

        Gaussian_dfunc2(x, t, u);
        dim = N*(p+1);
        matlib_create_dm( dim, t.len, &U, MATLIB_COL_MAJOR, MATLIB_NO_TRANS);    
        matlib_create_dm( dim, t.len, &V, MATLIB_COL_MAJOR, MATLIB_NO_TRANS);    

        for(i=0; i<num_cycles; i++)
        {
            fem1d_DFLT2( N, FM, u, U);

            pfem1d_DFLT2(N, FM, u, V, num_threads, mp);

            /* Analyze error */ 
            BEGIN_DTRACE
                for(i=0; i<U.lenr; i++)
                {
                    for(k=0; k<U.lenc; k++)
                    {
                        debug_print( "[%d][%d] -> serial: % 0.16f, "
                                     "parallel: % 0.16f", k, i, 
                                     U.elem_p[k+U.lenc*i], 
                                     V.elem_p[k+V.lenc*i]);
                    }
                }
            END_DTRACE

            u_tmp1.elem_p = U.elem_p;
            u_tmp2.elem_p = V.elem_p;

            u_tmp1.len = U.lenc * U.lenr;
            u_tmp2.len = V.lenc * V.lenr;

            norm_actual = matlib_dnrm2(u_tmp1);
            matlib_daxpy(-1.0, u_tmp2, u_tmp1);
            e_relative = matlib_dnrm2(u_tmp1)/norm_actual;

            debug_body("Relative error: % 0.16g", e_relative);
            
            CU_ASSERT_TRUE(e_relative<TOL);
        }
        matlib_free((void*)x.elem_p);
        matlib_free((void*)u.elem_p);
        matlib_free((void*)U.elem_p);
        matlib_free((void*)V.elem_p);
    }
    
    debug_body("%s", "signal threads to exit!");
    pthpool_destroy_threads(num_threads, mp);
}

void test_pfem1d_DFLT2(void)
{
    matlib_index p_max = 15;
    for (matlib_index p=2; p<p_max; p++)
    {
        test_pfem1d_DFLT2_general(p);
    }
}

/*============================================================================*/

void test_pfem1d_DF2L_general(matlib_index p)
{
    
    /* define number of timing experiments and repeatitions */ 
    matlib_index num_exp    = 5;
    matlib_index num_cycles = 10;
    
    /* Create pthreads */
    matlib_index num_threads = 5;
    pthpool_data_t mp[num_threads];
    
    pthpool_create_threads(num_threads, mp);

    matlib_index i, j;


    matlib_index N, N0 = 200;
    matlib_index P = 4*p;

    /* define the domain */ 
    double x_l = -5.0;
    double x_r =  5.0;

    matlib_dv xi, quadW;
    legendre_LGLdataLT1( P, TOL, &xi, &quadW);
    
    matlib_dm FM, IM;

    matlib_create_dm( p+1, xi.len, &FM, MATLIB_ROW_MAJOR, MATLIB_NO_TRANS);    
    matlib_create_dm( xi.len, p+1, &IM, MATLIB_COL_MAJOR, MATLIB_NO_TRANS);    

    legendre_LGLdataFM( xi, FM);
    legendre_LGLdataIM( xi, IM);

    matlib_dv x, u, vb, U, V;
    double dim;
    double norm_actual, e_relative;

    for(j=0; j<num_exp; j++)
    {
        /* generate the grid */ 
        N = (j+1)*N0;

        fem1d_ref2mesh (xi, N, x_l, x_r, &x);
        debug_body("length of x: %d", x.len);


        /* Provide the initial condition */
        matlib_create_dv( x.len, &u, MATLIB_COL_VECT);

        Gaussian(x, u);
        dim = N*(p+1);
        matlib_create_dv( dim, &U, MATLIB_COL_VECT);
        matlib_create_dv( dim, &V, MATLIB_COL_VECT);
        matlib_create_dv( N*p+1, &vb, MATLIB_COL_VECT);
        fem1d_DFLT( N, FM, u, U);
        fem1d_DL2F( p, U, vb);

        for(i=0; i<num_cycles; i++)
        {
            fem1d_DF2L( p, vb, U);

            pfem1d_DF2L(p, vb, V, num_threads, mp);

            /* Analyze error */ 
            BEGIN_DTRACE
                for(i=0; i<U.len; i++)
                {
                    debug_print( "[%d] -> serial: %0.16f, parallel: %0.16f", 
                                 i, U.elem_p[i], V.elem_p[i]);
                }
            END_DTRACE

            norm_actual = matlib_dnrm2(U);
            matlib_daxpy(-1.0, U, V);
            e_relative = matlib_dnrm2(V)/norm_actual;
            debug_body("Relative error: % 0.16g", e_relative);
            
            CU_ASSERT_TRUE(e_relative<TOL);
        }
        matlib_free((void*)x.elem_p);
        matlib_free((void*)vb.elem_p);
        matlib_free((void*)u.elem_p);
        matlib_free((void*)U.elem_p);
        matlib_free((void*)V.elem_p);
    }
    
    debug_body("%s", "signal threads to exit!");
    pthpool_destroy_threads(num_threads, mp);
}

void test_pfem1d_DF2L(void)
{
    matlib_index p_max = 15;
    for (matlib_index p=2; p<p_max; p++)
    {
        test_pfem1d_DF2L_general(p);
    }
}

/*============================================================================*/

void test_pfem1d_DPrjL2F_general(matlib_index p)
{
    
    debug_enter("polynomial degree: %d", p);
    /* define number of timing experiments and repeatitions */ 
    matlib_index num_exp    = 5;
    matlib_index num_cycles = 10;

    /* Create pthreads */
    matlib_index num_threads = 3;
    pthpool_data_t mp[num_threads];
    
    pthpool_create_threads(num_threads, mp);

    matlib_index i, j;


    matlib_index N, N0 = 200;
    matlib_index P = 4*p;

    /* define the domain */ 
    double x_l = -5.0;
    double x_r =  5.0;

    matlib_dv xi, quadW;
    legendre_LGLdataLT1( P, TOL, &xi, &quadW);
    
    matlib_dm FM, IM;

    matlib_create_dm( p+1, xi.len, &FM, MATLIB_ROW_MAJOR, MATLIB_NO_TRANS);    
    matlib_create_dm( xi.len, p+1, &IM, MATLIB_COL_MAJOR, MATLIB_NO_TRANS);    

    legendre_LGLdataFM( xi, FM);
    legendre_LGLdataIM( xi, IM);

    matlib_dv x, u, Pvb1, Pvb2, U;
    double dim;
    double norm_actual, e_relative;

    for(j=0; j<num_exp; j++)
    {
        /* generate the grid */ 
        N = (j+1)*N0;

        fem1d_ref2mesh (xi, N, x_l, x_r, &x);
        debug_body("length of x: %d", x.len);


        /* Provide the initial condition */
        matlib_create_dv( x.len, &u, MATLIB_COL_VECT);

        Gaussian(x, u);
        dim = N*(p+1);
        matlib_create_dv( dim, &U, MATLIB_COL_VECT);
        matlib_create_dv( N*p+1, &Pvb1, MATLIB_COL_VECT);
        matlib_create_dv( N*p+1, &Pvb2, MATLIB_COL_VECT);
        fem1d_DFLT( N, FM, u, U);

        for(i=0; i<num_cycles; i++)
        {
            fem1d_DPrjL2F( p, U, Pvb1);

            pfem1d_DPrjL2F(p, U, Pvb2, num_threads, mp);

            /* Analyze error */ 
            BEGIN_DTRACE
                for(i=0; i<Pvb1.len; i++)
                {
                    debug_print( "[%d] -> serial: %0.16f, parallel: %0.16f", 
                                 i, Pvb1.elem_p[i], Pvb2.elem_p[i]);
                }
            END_DTRACE

            norm_actual = matlib_dnrm2(Pvb1);
            matlib_daxpy(-1.0, Pvb1, Pvb2);
            e_relative = matlib_dnrm2(Pvb2)/norm_actual;
            debug_body("Relative error: % 0.16g", e_relative);
            
            CU_ASSERT_TRUE(e_relative<TOL);
        }
        matlib_free((void*)x.elem_p);
        matlib_free((void*)Pvb1.elem_p);
        matlib_free((void*)Pvb2.elem_p);
        matlib_free((void*)u.elem_p);
        matlib_free((void*)U.elem_p);
    }
    
    debug_body("%s", "signal threads to exit!");
    pthpool_destroy_threads(num_threads, mp);
    debug_exit("%s", "");

}


void test_pfem1d_DPrjL2F(void)
{
    matlib_index p_max = 15;
    for (matlib_index p=2; p<p_max; p++)
    {
        test_pfem1d_DPrjL2F_general(p);
    }
}

/*============================================================================*/

void test_pfem1d_DNorm2_general(matlib_index p)
{
    
    debug_enter("polynomial degree: %d", p);
    /* define number of timing experiments and repeatitions */ 
    matlib_index num_exp    = 5;
    matlib_index num_cycles = 10;

    /* Create pthreads */
    matlib_index num_threads = 4;
    pthpool_data_t mp[num_threads];
    
    pthpool_create_threads(num_threads, mp);

    matlib_index i, j;


    matlib_index N, N0 = 200;
    matlib_index P = 4*p;

    /* define the domain */ 
    double x_l = -5.0;
    double x_r =  5.0;

    matlib_dv xi, quadW;
    legendre_LGLdataLT1( P, TOL, &xi, &quadW);
    
    matlib_dm FM, IM;

    matlib_create_dm( p+1, xi.len, &FM, MATLIB_ROW_MAJOR, MATLIB_NO_TRANS);    
    matlib_create_dm( xi.len, p+1, &IM, MATLIB_COL_MAJOR, MATLIB_NO_TRANS);    

    legendre_LGLdataFM( xi, FM);
    legendre_LGLdataIM( xi, IM);

    matlib_dv x, u, U;
    double dim;
    double norm1, norm2, e_relative;

    for(j=0; j<num_exp; j++)
    {
        /* generate the grid */ 
        N = (j+1)*N0;

        fem1d_ref2mesh (xi, N, x_l, x_r, &x);
        debug_body("length of x: %d", x.len);


        /* Provide the initial condition */
        matlib_create_dv( x.len, &u, MATLIB_COL_VECT);

        Gaussian(x, u);
        dim = N*(p+1);
        matlib_create_dv( dim, &U, MATLIB_COL_VECT);
        fem1d_DFLT( N, FM, u, U);

        for(i=0; i<num_cycles; i++)
        {
            norm1 = fem1d_DNorm2( p, N, U);
            debug_body("serial norm: %0.16f", norm1);

            norm2 = pfem1d_DNorm2(p, N, U, num_threads, mp);
            debug_body("parallel norm: %0.16f", norm2);

            /* Analyze error */ 
            e_relative = fabs(norm1-norm2)/norm1;
            debug_body("Relative error: % 0.16g", e_relative);
            
            CU_ASSERT_TRUE(e_relative<TOL);
        }
        matlib_free((void*)x.elem_p);
        matlib_free((void*)u.elem_p);
        matlib_free((void*)U.elem_p);
    }
    
    debug_body("%s", "signal threads to exit!");
    pthpool_destroy_threads(num_threads, mp);
    debug_exit("%s", "");

}


void test_pfem1d_DNorm2(void)
{
    matlib_index p_max = 15;
    for (matlib_index p=2; p<p_max; p++)
    {
        test_pfem1d_DNorm2_general(p);
    }
}
/*============================================================================+/
 | COMPLEX VERSION 
/+============================================================================*/

void zGaussian
( 
    matlib_dv x, 
    matlib_zv u
)
{

    debug_enter("%s", "");
    
    double *xptr;

    for (xptr = x.elem_p; xptr<(x.elem_p+x.len); xptr++)
    {
        *(u.elem_p) = cexp(-(1.0+I*1.0)**xptr**xptr/4.0);
        u.elem_p++;
    }
    debug_exit("%s", "");
}


void test_pfem1d_ZFLT_general(matlib_index p)
{

    debug_enter("polynomial degree: %d", p);
    
    /* define number of timing experiments and repeatitions */ 
    matlib_index num_exp    = 5;
    matlib_index num_cycles = 10;
    
    /* Create pthreads */
    matlib_index num_threads = 5;
    pthpool_data_t mp[num_threads];
    
    pthpool_create_threads(num_threads, mp);

    matlib_index i, j;

    matlib_index N, N0 = 200;
    matlib_index P = 4*p;

    /* define the domain */ 
    double x_l = -5.0;
    double x_r =  5.0;

    matlib_dv xi, quadW;
    legendre_LGLdataLT1( P, TOL, &xi, &quadW);
    
    matlib_dm FM, IM;

    matlib_create_dm( p+1, xi.len, &FM, MATLIB_ROW_MAJOR, MATLIB_NO_TRANS);    
    matlib_create_dm( xi.len, p+1, &IM, MATLIB_COL_MAJOR, MATLIB_NO_TRANS);    

    legendre_LGLdataFM( xi, FM);
    legendre_LGLdataIM( xi, IM);

    matlib_dv x;
    matlib_zv u, U, V;
    double dim;
    double norm_actual, e_relative;

    for(j=0; j<num_exp; j++)
    {
        /* generate the grid */ 
        N = (j+1)*N0;

        fem1d_ref2mesh (xi, N, x_l, x_r, &x);
        debug_body("length of x: %d", x.len);


        /* Provide the initial condition */
        matlib_create_zv( x.len, &u, MATLIB_COL_VECT);

        zGaussian(x, u);
        dim = N*(p+1);
        matlib_create_zv( dim, &U, MATLIB_COL_VECT);
        matlib_create_zv( dim, &V, MATLIB_COL_VECT);

        for(i=0; i<num_cycles; i++)
        {
            fem1d_ZFLT( N, FM, u, U);

            pfem1d_ZFLT(N, FM, u, V, num_threads, mp);

            /* Analyze error */ 
            BEGIN_DTRACE
                for(i=0; i<U.len; i++)
                {
                    debug_print( "[%d] -> serial: %0.16f, parallel: %0.16f", 
                                 i, U.elem_p[i], V.elem_p[i]);
                }
            END_DTRACE

            norm_actual = matlib_znrm2(U);
            matlib_zaxpy(-1.0, U, V);
            e_relative = matlib_znrm2(V)/norm_actual;
            debug_body("Relative error: % 0.16g", e_relative);
            
            CU_ASSERT_TRUE(e_relative<TOL);
        }
        matlib_free((void*)x.elem_p);
        matlib_free((void*)u.elem_p);
        matlib_free((void*)U.elem_p);
        matlib_free((void*)V.elem_p);
    }
    
    debug_body("%s", "signal threads to exit!");
    pthpool_destroy_threads(num_threads, mp);
}

void test_pfem1d_ZFLT(void)
{
    matlib_index p_max = 15;
    for (matlib_index p=2; p<p_max; p++)
    {
        test_pfem1d_ZFLT_general(p);
    }
}
/*============================================================================*/

void test_pfem1d_ZILT_general(matlib_index p)
{

    debug_enter("polynomial degree: %d", p);
    
    /* define number of timing experiments and repeatitions */ 
    matlib_index num_exp    = 5;
    matlib_index num_cycles = 10;
    
    /* Create pthreads */
    matlib_index num_threads = 5;
    pthpool_data_t mp[num_threads];
    
    pthpool_create_threads(num_threads, mp);

    matlib_index i, j;

    matlib_index N, N0 = 200;
    matlib_index P = 4*p;

    /* define the domain */ 
    double x_l = -5.0;
    double x_r =  5.0;

    matlib_dv xi, quadW;
    legendre_LGLdataLT1( P, TOL, &xi, &quadW);
    
    matlib_dm FM, IM;

    matlib_create_dm( p+1, xi.len, &FM, MATLIB_ROW_MAJOR, MATLIB_NO_TRANS);    
    matlib_create_dm( xi.len, p+1, &IM, MATLIB_COL_MAJOR, MATLIB_NO_TRANS);    

    legendre_LGLdataFM( xi, FM);
    legendre_LGLdataIM( xi, IM);

    matlib_dv x;
    matlib_zv u, v, U;
    double dim;
    double norm_actual, e_relative;

    for(j=0; j<num_exp; j++)
    {
        /* generate the grid */ 
        N = (j+1)*N0;

        fem1d_ref2mesh (xi, N, x_l, x_r, &x);
        debug_body("length of x: %d", x.len);


        /* Provide the initial condition */
        matlib_create_zv( x.len, &u, MATLIB_COL_VECT);
        matlib_create_zv( x.len, &v, MATLIB_COL_VECT);

        zGaussian(x, u);
        dim = N*(p+1);
        matlib_create_zv( dim, &U, MATLIB_COL_VECT);
        fem1d_ZFLT( N, FM, u, U);

        for(i=0; i<num_cycles; i++)
        {
            fem1d_ZILT( N, IM, U, u);

            pfem1d_ZILT(N, IM, U, v, num_threads, mp);

            /* Analyze error */ 
            BEGIN_DTRACE
                for(i=0; i<U.len; i++)
                {
                    debug_print( "[%d] -> serial: %0.16f, parallel: %0.16f", 
                                 i, u.elem_p[i], v.elem_p[i]);
                }
            END_DTRACE

            norm_actual = matlib_znrm2(u);
            matlib_zaxpy(-1.0, u, v);
            e_relative = matlib_znrm2(v)/norm_actual;
            debug_body("Relative error: % 0.16g", e_relative);
            
            CU_ASSERT_TRUE(e_relative<TOL);
        }
        matlib_free((void*)x.elem_p);
        matlib_free((void*)u.elem_p);
        matlib_free((void*)v.elem_p);
        matlib_free((void*)U.elem_p);
    }
    
    debug_body("%s", "signal threads to exit!");
    pthpool_destroy_threads(num_threads, mp);
}

void test_pfem1d_ZILT(void)
{
    matlib_index p_max = 15;
    for (matlib_index p=2; p<p_max; p++)
    {
        test_pfem1d_ZILT_general(p);
    }
}
/*============================================================================*/
void Gaussian_zfunc2
( 
    const matlib_dv x, 
    const matlib_dv t, 
          matlib_zm y
)
{
    matlib_index i, j;
    bool dim_OK = (y.lenc == x.len) && (t.len == y.lenr);

    if(dim_OK)
    {
        for(j=0;j<t.len; j++)
        {
            for (i=0; i<x.len; i++)
            {
                *(y.elem_p) = cexp(-(0.5+0.8*I)*(*(x.elem_p+i)**(x.elem_p+i))/(1+*(t.elem_p+j)));
                y.elem_p++;
            }
        }
    }
}

void test_pfem1d_ZFLT2_general(matlib_index p)
{

    debug_enter("polynomial degree: %d", p);
    
    /* define number of timing experiments and repeatitions */ 
    matlib_index num_exp    = 5;
    matlib_index num_cycles = 10;
    
    /* Create pthreads */
    matlib_index num_threads = 4;
    pthpool_data_t mp[num_threads];
    
    pthpool_create_threads(num_threads, mp);

    matlib_index i, j, k;

    matlib_index N, N0 = 200;
    matlib_index P = 4*p;

    /* define the domain */ 
    double x_l = -5.0;
    double x_r =  5.0;

    matlib_dv xi, quadW;
    legendre_LGLdataLT1( P, TOL, &xi, &quadW);
    
    matlib_dm FM, IM;

    matlib_create_dm( p+1, xi.len, &FM, MATLIB_ROW_MAJOR, MATLIB_NO_TRANS);    
    matlib_create_dm( xi.len, p+1, &IM, MATLIB_COL_MAJOR, MATLIB_NO_TRANS);    

    legendre_LGLdataFM( xi, FM);
    legendre_LGLdataIM( xi, IM);

    matlib_dv x, t;

    matlib_index Nt = 2;
    double dt = 1.0e-3;
    matlib_create_dv( Nt, &t, MATLIB_COL_VECT);

    t.elem_p[0] = 0;
    for(i=1; i<t.len; i++)
    {
        t.elem_p[i] = t.elem_p[i-1] + dt;
    }
    matlib_zm u, U, V;
    double dim;
    double norm_actual, e_relative;
    matlib_zv u_tmp1, u_tmp2;

    for(j=0; j<num_exp; j++)
    {
        /* generate the grid */ 
        N = (j+1)*N0;

        fem1d_ref2mesh (xi, N, x_l, x_r, &x);
        debug_body("length of x: %d", x.len);


        /* Provide the initial condition */
        matlib_create_zm( x.len, t.len, &u, MATLIB_COL_MAJOR, MATLIB_NO_TRANS);    

        Gaussian_zfunc2(x, t, u);
        dim = N*(p+1);
        matlib_create_zm( dim, t.len, &U, MATLIB_COL_MAJOR, MATLIB_NO_TRANS);    
        matlib_create_zm( dim, t.len, &V, MATLIB_COL_MAJOR, MATLIB_NO_TRANS);    

        for(i=0; i<num_cycles; i++)
        {
            fem1d_ZFLT2( N, FM, u, U);

            pfem1d_ZFLT2(N, FM, u, V, num_threads, mp);

            /* Analyze error */ 
            BEGIN_DTRACE
                for(i=0; i<U.lenr; i++)
                {
                    for(k=0; k<U.lenc; k++)
                    {
                        debug_print( "[%d][%d] -> "
                                     "serial: % 0.16f%+0.16f, "
                                     "parallel: % 0.16f%+0.16f", 
                                     k, i, 
                                     U.elem_p[k+U.lenc*i], 
                                     V.elem_p[k+V.lenc*i]);
                    }
                }
            END_DTRACE

            u_tmp1.elem_p = U.elem_p;
            u_tmp2.elem_p = V.elem_p;

            u_tmp1.len = U.lenc * U.lenr;
            u_tmp2.len = V.lenc * V.lenr;

            norm_actual = matlib_znrm2(u_tmp1);
            matlib_zaxpy(-1.0, u_tmp2, u_tmp1);
            e_relative = matlib_znrm2(u_tmp1)/norm_actual;

            debug_body("Relative error: % 0.16g", e_relative);
            
            CU_ASSERT_TRUE(e_relative<TOL);
        }
        matlib_free((void*)x.elem_p);
        matlib_free((void*)u.elem_p);
        matlib_free((void*)U.elem_p);
        matlib_free((void*)V.elem_p);
    }
    
    debug_body("%s", "signal threads to exit!");
    pthpool_destroy_threads(num_threads, mp);
}

void test_pfem1d_ZFLT2(void)
{
    matlib_index p_max = 15;
    for (matlib_index p=2; p<p_max; p++)
    {
        test_pfem1d_ZFLT2_general(p);
    }
}

/*============================================================================*/

void test_pfem1d_ZF2L_general(matlib_index p)
{
    
    /* define number of timing experiments and repeatitions */ 
    matlib_index num_exp    = 5;
    matlib_index num_cycles = 10;

    /* Create pthreads */
    matlib_index num_threads = 5;
    pthpool_data_t mp[num_threads];
    
    pthpool_create_threads(num_threads, mp);

    matlib_index i, j;

    matlib_index N, N0 = 200;
    matlib_index P = 4*p;

    /* define the domain */ 
    double x_l = -5.0;
    double x_r =  5.0;

    matlib_dv xi, quadW;
    legendre_LGLdataLT1( P, TOL, &xi, &quadW);
    
    matlib_dm FM, IM;

    matlib_create_dm( p+1, xi.len, &FM, MATLIB_ROW_MAJOR, MATLIB_NO_TRANS);    
    matlib_create_dm( xi.len, p+1, &IM, MATLIB_COL_MAJOR, MATLIB_NO_TRANS);    

    legendre_LGLdataFM( xi, FM);
    legendre_LGLdataIM( xi, IM);

    matlib_dv x;
    matlib_zv u, U, V, vb;
    double dim;
    double norm_actual, e_relative;

    for(j=0; j<num_exp; j++)
    {
        /* generate the grid */ 
        N = (j+1)*N0;

        fem1d_ref2mesh (xi, N, x_l, x_r, &x);
        debug_body("length of x: %d", x.len);


        /* Provide the initial condition */
        matlib_create_zv( x.len, &u, MATLIB_COL_VECT);

        zGaussian(x, u);
        dim = N*(p+1);
        matlib_create_zv( dim, &U, MATLIB_COL_VECT);
        matlib_create_zv( dim, &V, MATLIB_COL_VECT);
        matlib_create_zv( N*p+1, &vb, MATLIB_COL_VECT);
        fem1d_ZFLT( N, FM, u, U);
        fem1d_ZL2F( p, U, vb);

        for(i=0; i<num_cycles; i++)
        {
            fem1d_ZF2L( p, vb, U);

            pfem1d_ZF2L(p, vb, V, num_threads, mp);

            /* Analyze error */ 
            BEGIN_DTRACE
                for(i=0; i<U.len; i++)
                {
                    debug_print( "[%d] -> serial: %0.16f% 0.16f, parallel: %0.16f% 0.16f", 
                                 i, U.elem_p[i], V.elem_p[i]);
                }
            END_DTRACE

            norm_actual = matlib_znrm2(U);
            matlib_zaxpy(-1.0, U, V);
            e_relative = matlib_znrm2(V)/norm_actual;
            debug_body("Relative error: % 0.16g", e_relative);
            
            CU_ASSERT_TRUE(e_relative<TOL);
        }
        matlib_free((void*)x.elem_p);
        matlib_free((void*)u.elem_p);
        matlib_free((void*)vb.elem_p);
        matlib_free((void*)U.elem_p);
        matlib_free((void*)V.elem_p);
    }
    
    debug_body("%s", "signal threads to exit!");
    pthpool_destroy_threads(num_threads, mp);
}

void test_pfem1d_ZF2L(void)
{
    matlib_index p_max = 15;
    for (matlib_index p=2; p<p_max; p++)
    {
        test_pfem1d_ZF2L_general(p);
    }
}

/*============================================================================*/

void test_pfem1d_ZPrjL2F_general(matlib_index p)
{
    
    debug_enter("polynomial degree: %d", p);
    /* define number of timing experiments and repeatitions */ 
    matlib_index num_exp    = 5;
    matlib_index num_cycles = 10;

    /* Create pthreads */
    matlib_index num_threads = 4;
    pthpool_data_t mp[num_threads];
    
    pthpool_create_threads(num_threads, mp);

    matlib_index i, j;

    matlib_index N, N0 = 200;
    matlib_index P = 4*p;

    /* define the domain */ 
    double x_l = -5.0;
    double x_r =  5.0;

    matlib_dv xi, quadW;
    legendre_LGLdataLT1( P, TOL, &xi, &quadW);
    
    matlib_dm FM, IM;

    matlib_create_dm( p+1, xi.len, &FM, MATLIB_ROW_MAJOR, MATLIB_NO_TRANS);    
    matlib_create_dm( xi.len, p+1, &IM, MATLIB_COL_MAJOR, MATLIB_NO_TRANS);    

    legendre_LGLdataFM( xi, FM);
    legendre_LGLdataIM( xi, IM);

    matlib_dv x;
    matlib_zv u, U, Pvb1, Pvb2;
    double dim;
    double norm_actual, e_relative;

    for(j=0; j<num_exp; j++)
    {
        /* generate the grid */ 
        N = (j+1)*N0;

        fem1d_ref2mesh (xi, N, x_l, x_r, &x);
        debug_body("length of x: %d", x.len);


        /* Provide the initial condition */
        matlib_create_zv( x.len, &u, MATLIB_COL_VECT);

        zGaussian(x, u);

        dim = N*(p+1);
        matlib_create_zv( dim, &U, MATLIB_COL_VECT);
        matlib_create_zv( N*p+1, &Pvb1, MATLIB_COL_VECT);
        matlib_create_zv( N*p+1, &Pvb2, MATLIB_COL_VECT);
        fem1d_ZFLT( N, FM, u, U);

        for(i=0; i<num_cycles; i++)
        {
            fem1d_ZPrjL2F( p, U, Pvb1);

            pfem1d_ZPrjL2F(p, U, Pvb2, num_threads, mp);

            /* Analyze error */ 
            BEGIN_DTRACE
                for(i=0; i<Pvb1.len; i++)
                {
                    debug_print( "[%d] -> serial: %0.16f% 0.16f, "
                                 "parallel: %0.16f% 0.16f", 
                                 i, Pvb1.elem_p[i], Pvb2.elem_p[i]);
                }
            END_DTRACE

            norm_actual = matlib_znrm2(Pvb1);
            matlib_zaxpy(-1.0, Pvb1, Pvb2);
            e_relative = matlib_znrm2(Pvb2)/norm_actual;
            debug_body("Relative error: % 0.16g", e_relative);
            
            CU_ASSERT_TRUE(e_relative<TOL);
        }
        matlib_free((void*)x.elem_p);
        matlib_free((void*)u.elem_p);
        matlib_free((void*)Pvb1.elem_p);
        matlib_free((void*)Pvb2.elem_p);
        matlib_free((void*)U.elem_p);
    }
    
    debug_body("%s", "signal threads to exit!");
    pthpool_destroy_threads(num_threads, mp);
}

void test_pfem1d_ZPrjL2F(void)
{
    matlib_index p_max = 15;
    for (matlib_index p=2; p<p_max; p++)
    {
        test_pfem1d_ZPrjL2F_general(p);
    }
}
/*============================================================================*/

void test_pfem1d_ZNorm2_general(matlib_index p)
{
    
    debug_enter("polynomial degree: %d", p);
    /* define number of timing experiments and repeatitions */ 
    matlib_index num_exp    = 5;
    matlib_index num_cycles = 10;

    /* Create pthreads */
    matlib_index num_threads = 4;
    pthpool_data_t mp[num_threads];
    
    pthpool_create_threads(num_threads, mp);

    matlib_index i, j;

    matlib_index N, N0 = 200;
    matlib_index P = 4*p;

    /* define the domain */ 
    double x_l = -5.0;
    double x_r =  5.0;

    matlib_dv xi, quadW;
    legendre_LGLdataLT1( P, TOL, &xi, &quadW);
    
    matlib_dm FM, IM;

    matlib_create_dm( p+1, xi.len, &FM, MATLIB_ROW_MAJOR, MATLIB_NO_TRANS);    
    matlib_create_dm( xi.len, p+1, &IM, MATLIB_COL_MAJOR, MATLIB_NO_TRANS);    

    legendre_LGLdataFM( xi, FM);
    legendre_LGLdataIM( xi, IM);

    matlib_dv x;
    matlib_zv u, U;
    double dim;
    double norm1, norm2, e_relative;

    for(j=0; j<num_exp; j++)
    {
        /* generate the grid */ 
        N = (j+1)*N0;

        fem1d_ref2mesh (xi, N, x_l, x_r, &x);
        debug_body("length of x: %d", x.len);


        /* Provide the initial condition */
        matlib_create_zv( x.len, &u, MATLIB_COL_VECT);

        zGaussian(x, u);

        dim = N*(p+1);
        matlib_create_zv( dim, &U, MATLIB_COL_VECT);
        fem1d_ZFLT( N, FM, u, U);

        for(i=0; i<num_cycles; i++)
        {
            norm1 = fem1d_ZNorm2( p, N, U);
            debug_body("serial norm: %0.16f", norm1);

            norm2 = pfem1d_ZNorm2(p, N, U, num_threads, mp);
            debug_body("parallel norm: %0.16f", norm2);

            e_relative = fabs(norm1-norm2)/norm1;
            debug_body("Relative error: % 0.16g", e_relative);
            
            CU_ASSERT_TRUE(e_relative<TOL);
        }
        matlib_free((void*)x.elem_p);
        matlib_free((void*)u.elem_p);
        matlib_free((void*)U.elem_p);
    }
    
    debug_body("%s", "signal threads to exit!");
    pthpool_destroy_threads(num_threads, mp);
}

void test_pfem1d_ZNorm2(void)
{
    matlib_index p_max = 15;
    for (matlib_index p=2; p<p_max; p++)
    {
        test_pfem1d_ZNorm2_general(p);
    }
}



/*============================================================================+/
 | Test runner
 |
 |
 +============================================================================*/

int main(void)
{

    //mkl_set_num_threads(1);
    mkl_domain_set_num_threads(1, MKL_BLAS);
    CU_pSuite pSuite = NULL;

    /* initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry())
    {
        return CU_get_error();
    }

    /* Create a test array */
    CU_TestInfo test_array[] = 
    {
        { "Parallel DFLT"           , test_pfem1d_DFLT    },
        { "Parallel DILT"           , test_pfem1d_DILT    },
        { "Parallel DFLT2"          , test_pfem1d_DFLT2   },
        { "Parallel DF2L"           , test_pfem1d_DF2L    },
        { "Parallel projection DL2F", test_pfem1d_DPrjL2F },
        { "Parallel D-L2 norm"      , test_pfem1d_DNorm2  },
        { "Parallel ZFLT"           , test_pfem1d_ZFLT    },
        { "Parallel ZFLT2"          , test_pfem1d_ZFLT2   },
        { "Parallel ZILT"           , test_pfem1d_ZILT    },
        { "Parallel ZF2L"           , test_pfem1d_ZF2L    },
        { "Parallel projection ZL2F", test_pfem1d_ZPrjL2F },
        { "Parallel Z-L2 norm"      , test_pfem1d_ZNorm2  },
        CU_TEST_INFO_NULL,
    };

    /* Create the test suite */ 
    CU_SuiteInfo suites[] = 
    {
        { "Parallelization of Routines", init_suite, clean_suite, test_array },
        CU_SUITE_INFO_NULL,
    }; 

    /* Register test suites */ 
    CU_ErrorCode CU_error = CU_register_suites(suites); 
    if (CU_error != CUE_SUCCESS) 
    {
        debug_body("%s", CU_get_error_msg());
        CU_cleanup_registry();
        return CU_get_error();
    }

   /* Run all tests using the CUnit Basic interface */
   CU_basic_set_mode(CU_BRM_VERBOSE);
   CU_basic_run_tests();
   CU_cleanup_registry();
   return CU_get_error();
}
