
/******************************************************************************
    params->data.F32[0] = So;
    params->data.F32[1] = Zo;
    params->data.F32[2] = Xo;
    params->data.F32[3] = Yo;
    params->data.F32[4] = 1 / SigmaX;
    params->data.F32[5] = 1 / SigmaY;
    params->data.F32[6] = Sxy;
    params->data.F32[7] = length;
    params->data.F32[8] = theta;
*****************************************************************************/

# define PM_MODEL_FUNC            pmModelFunc_STRAIL
# define PM_MODEL_FLUX            pmModelFlux_STRAIL
# define PM_MODEL_GUESS           pmModelGuess_STRAIL
# define PM_MODEL_LIMITS          pmModelLimits_STRAIL
# define PM_MODEL_RADIUS          pmModelRadius_STRAIL
# define PM_MODEL_FROM_PSF        pmModelFromPSF_STRAIL
# define PM_MODEL_PARAMS_FROM_PSF pmModelParamsFromPSF_STRAIL
# define PM_MODEL_FIT_STATUS      pmModelFitStatus_STRAIL

psF32 PM_MODEL_FUNC(psVector *deriv,
                         const psVector *params,
                         const psVector *x)
{
    psF32 *PAR = params->data.F32;

    psF32 trailLength = PAR[7];
    psF32 theta = PAR[8];

    psF32 x0 = PAR[2];  //streak center
    psF32 y0 = PAR[3];  //streak center

    //S values (1/sigma for x and y case, sigma for xy case)
    psF32 sx = PAR[4];
    psF32 sy = PAR[5];
    psF32 sxy = PAR[6];

    psF32 sinT=sin(theta);
    psF32 cosT=cos(theta);
    psF32 sin2T=sin(2.0*theta);
    psF32 cos2T=cos(2.0*theta);

    //    printf("Trying object at %4.1f,%4.1f with length %3.1f and angle %1.3f\r", x0, y0, length, theta);

    //current location relative to trail center
    psF32 X  = x->data.F32[0] - x0;
    psF32 Y  = x->data.F32[1] - y0;

    //x' and y' location (trail-orienter coords)
    psF32 xs = X*cosT + Y*sinT;
    psF32 ys = -1.0*X*sinT + Y*cosT;

    //initialize variables to be changed below
    psF32 x1 = 0;
    psF32 y1 = 0;
    psF32 px = 0;
    psF32 py = 0;
    psF32 z  = 0;
    psF32 zx = 0;
    psF32 t  = 0;
    psF32 tx = 0;
    psF32 r  = 0;
    psF32 rx = 0;
    psF32 f  = 0;

    psF32 sxrot = 0;
    psF32 syrot = 0;
    psF32 sxyrot = 0;
    psF32 dsxrot = 0;
    psF32 dsyrot = 0;
    psF32 dsxyrot = 0;

    //    psF32 Rx = 0;
    //    psF32 Ry = 0;
    //    psF32 Rxy = 0;


    //calculate new S values (1/sigma) for rotated frame
    psF32 sxrotsq = PS_SQR(cosT*sx) + PS_SQR(sinT*sy) + cosT*sinT*sxy;
    psF32 syrotsq = PS_SQR(cosT*sy) + PS_SQR(sinT*sx) - cosT*sinT*sxy;

    //    psF32 testtwo=10.1;
    //    psF32 testone=fabsf(testtwo);
    //    fprintf (stderr, "Test: %f is the absolute value of %f?\n",testone,testtwo);
    if (sxrotsq<0) {
      sxrot = sqrt(-(sxrotsq));
      syrot = sqrt(syrotsq);
      fprintf (stderr, "error in sxrotsq: Neg,  sxrotsq=%f sx=%f sy=%f sxy=%f theta=%f\n",sxrotsq,sx,sy,sxy,theta);
    } else if (syrotsq<0) {
      sxrot = sqrt(sxrotsq);
      syrot = sqrt(-(syrotsq));
      fprintf (stderr, "error in syrotsq: Neg,  syrotsq=%f sx=%f sy=%f sxy=%f theta=%f\n",syrotsq,sx,sy,sxy,theta);
    } else if (sxrotsq==0){
      sxrot = 0.01;
      syrot = sqrt(syrotsq);
      fprintf (stderr, "error in sxrotsq: Zero,  sxrotsq=%f \n",sxrotsq);
    } else if (syrotsq==0) {
      syrot = 0.01;
      sxrot = sqrt(sxrotsq);
      fprintf (stderr, "error in syrotsq: Zero,  syrotsq=%f \n",syrotsq);
      //      return(0);
    }else {
      sxrot = sqrt(sxrotsq);
      syrot = sqrt(syrotsq);
    }

    sxyrot = sxy*cos2T + (PS_SQR(sy) - PS_SQR(sx))*sin2T;

      if (isnan(sxrot)) {
        fprintf (stderr, "error in sxrot  syrot=%f sx=%f sy=%f sxy=%f cosT=%f sinT=%f\n",syrot,sx,sy,sxy,cosT,sinT);
      } else if (isnan(syrot)) {
        fprintf (stderr, "error in syrot  sxrot=%f sx=%f sy=%f sxy=%f cosT=%f sinT=%f\n",sxrot,sx,sy,sxy,cosT,sinT);
      }

    //calculate length of pipe (not of trail motion)
    psF32 length = trailLength - 2.0*2.0/sxrot;


    if (xs < length/(-2.0)) {
      x1 = (xs+length/2.0)*cosT - ys*sinT; //Endpoint1
      y1 = (xs+length/2.0)*sinT + ys*cosT; //Endpoint1
      //1.6 factor comes from by-eye testing of fits, sqrt from the later squaring of it
      //1.6 ~= phi (golden mean)...coincidence?
      px = sxrot*x1/sqrt(1.6);
      py = syrot*y1;

      //first find out what the falloff in the x direction is...
      zx  = 0.5*PS_SQR(px);
      tx = 1 + zx + zx*zx/2.0 + zx*zx*zx/6.0;
      rx = 1.0 / (tx + zx*zx*zx*zx/24.0);

      //...and now in the y direction
      z  = 0.5*PS_SQR(py)+sxyrot*x1*y1;
      t  = 1 + z + z*z/2.0;
      r  = 1.0 / (t + z*z*z/6.0); /* exp (-Z) */
      f  = PAR[1]*rx*r + PAR[0];

    } else if (xs > length/2.0){
      x1 = (xs-length/2.0)*cosT - ys*sinT; //Endpoint2
      y1 = (xs-length/2.0)*sinT + ys*cosT; //Endpoint2
      px = sxrot*x1/sqrt(1.6);
      py = syrot*y1;
      zx  = 0.5*PS_SQR(px);
      tx = 1 + zx + zx*zx/2.0 + zx*zx*zx/6.0;
      rx = 1.0 / (tx + zx*zx*zx*zx/24.0);

      z  = 0.5*PS_SQR(py)+sxyrot*x1*y1;
      t  = 1 + z + z*z/2.0;
      r  = 1.0 / (t + z*z*z/6.0); /* exp (-Z) */
      f  = PAR[1]*rx*r + PAR[0];

      if (isnan(r)) {
        fprintf (stderr, "error in +r  t=%f z=%f\n",t,z);
      }

    } else {
      x1 = -ys*sinT;
      y1 = ys*cosT;
      px = sx*x1;
      py = sy*y1;
      z  = 0.5*PS_SQR(px) + 0.5*PS_SQR(py) + sxy*x1*y1;
      t  = 1 + z + z*z/2.0;
      r  = 1.0 / (t + z*z*z/6.0); /* exp (-Z) */
      rx = 1.0;  //so that dF/dF0 can be generalized
      f  = PAR[1]*r + PAR[0];

      if (isnan(r)) {
        fprintf (stderr, "error in midr  t=%f z=%f\n",t,z);
      }
    }


    //ok...so this is df/dPAR[X]
    if (deriv != NULL) {
        psF32 q = 1;
        //stable
        deriv->data.F32[0] = +1.0;
        deriv->data.F32[1] = +r * rx;

          if (isnan(deriv->data.F32[0])) {
            fprintf (stderr, "error in deriv0\n");
          } else if (isnan(deriv->data.F32[1])) {
            fprintf (stderr, "error in deriv1 r=%f rx=%f\n",r,rx);
          }

        dsxrot = 2.0*PS_SQR(sy)*sinT*cosT - 2.0*PS_SQR(sx)*sinT*cosT + sxy*(PS_SQR(cosT)-PS_SQR(sinT));
        dsyrot = 2.0*PS_SQR(sx)*sinT*cosT - 2.0*PS_SQR(sy)*sinT*cosT - sxy*(PS_SQR(cosT)-PS_SQR(sinT));
        dsxyrot = 2.0*cos2T*(PS_SQR(sy)-PS_SQR(sx)) - 2.0*sxy*sin2T;

          if (isnan(dsxrot)) {
            fprintf (stderr, "error in dsxrot\n");
          } else if (isnan(dsyrot)) {
            fprintf (stderr, "error in dsyrot\n");
          } else if (isnan(dsxyrot)) {
            fprintf (stderr, "error in dsxyrot\n");
          }

        //initialize variables definied in the if statements
        //      psF32 XXX=0;
        //      psF32 YYY=0;


        // variable over piecewise func
        // change the endcaps to be 4th order gaussians with sxrot_fit=1.6*sxrot
        // y' direction can ge adequately modelled by a 3rd order gaussian with syrot
        if (xs < length/(-2.0)) {

          q=PAR[1]*r*rx;
          deriv->data.F32[2] = -q*(rx*tx/1.6*x1*PS_SQR(sxrot) + r*t*y1*sxyrot);
          deriv->data.F32[3] = -q*r*t*(y1*PS_SQR(syrot) + x1*sxyrot);
          deriv->data.F32[4] = -q*(rx*tx*x1*sx*PS_SQR(cosT)/1.6*(x1+2.0*cosT/sxrot) + r*t*y1*sx*sinT*(y1*sinT+2.0*PS_SQR(syrot)*PS_SQR(cosT)/(sxrot*sxrot*sxrot)) + 2.0*r*t*sx*(-1.0*x1*y1*sin2T+y1*sxyrot*(cosT*cosT*cosT)/(sxrot*sxrot*sxrot)+x1*sxyrot*(sinT*cosT*cosT)/(sxrot*sxrot*sxrot)));
          deriv->data.F32[5] = -q*(rx*tx*x1*sy*PS_SQR(sinT)/1.6*(x1+2.0*cosT/sxrot) + r*t*y1*sy*(y1*PS_SQR(cosT)+2.0*PS_SQR(syrot)*(sinT*sinT*sinT)/(sxrot*sxrot*sxrot)) + 2.0*r*t*sy*(x1*y1*sin2T+y1*sxyrot*(sinT*sinT*cosT)/(sxrot*sxrot*sxrot)+x1*sxyrot*(sinT*sinT*sinT)/(sxrot*sxrot*sxrot)));
          deriv->data.F32[6] = -q*(rx*tx*x1*cosT*sinT/3.2*(x1+2.0*cosT/sxrot) + r*t*y1*cosT*sinT/2.0*(-y1+2.0*PS_SQR(syrot)*sinT/(sxrot*sxrot*sxrot)) + r*t*(x1*y1*cos2T+y1*sxyrot*sinT*cosT*cosT/(sxrot*sxrot*sxrot)+x1*sxyrot*sinT*sinT*cosT/(sxrot*sxrot*sxrot)));
          deriv->data.F32[7] = -q*(rx*tx*PS_SQR(sxrot)*x1*cosT/3.2 + r*t*PS_SQR(syrot)*y1*sinT/2.0 + r*t*sxyrot/2.0*(y1*cosT+x1*sinT));
          deriv->data.F32[8] = -q*(rx*tx*x1/3.2*(x1*(2.0*sinT*cosT*(PS_SQR(sy)-PS_SQR(sx))+sxy*(PS_SQR(cosT)-PS_SQR(sinT)))+PS_SQR(sxrot)*(2.0*cosT*(2.0*sinT*cosT*(PS_SQR(sy)-PS_SQR(sx))+sxy*(PS_SQR(cosT)-PS_SQR(sinT)))/(sxrot*sxrot*sxrot) - sinT*length)) + r*t*y1/2.0*(y1*(2.0*sinT*cosT*(PS_SQR(sx)-PS_SQR(sy))-sxy*(PS_SQR(cosT)-PS_SQR(sinT)))+PS_SQR(syrot)*(2.0*sinT*(2.0*sinT*cosT*(PS_SQR(sy)-PS_SQR(sx))+sxy*(PS_SQR(cosT)-PS_SQR(sinT)))/(sxrot*sxrot*sxrot)+cosT*length)) + r*t*(2.0*x1*y1*(cos2T*(PS_SQR(sy)-PS_SQR(sx))-sxy*sin2T)+y1*sxyrot/2.0*(2.0*cosT*(2.0*sinT*cosT*(PS_SQR(sy)-PS_SQR(sx))+sxy*(PS_SQR(cosT)-PS_SQR(sinT)))/(sxrot*sxrot*sxrot)-length*sinT)+x1*sxyrot/2.0*(2.0*sinT*(2.0*sinT*cosT*(PS_SQR(sy)-PS_SQR(sx))+sxy*(PS_SQR(cosT)-PS_SQR(sinT)))/(sxrot*sxrot*sxrot)+length*cosT)));

          /*      if (isnan(deriv->data.F32[2])) {
            fprintf (stderr, "error in deriv2\n");
          } else if (isnan(deriv->data.F32[3])) {
            fprintf (stderr, "error in deriv3\n");
          } else if (isnan(deriv->data.F32[4])) {
            fprintf (stderr, "error in deriv4\n");
          } else if (isnan(deriv->data.F32[5])) {
            fprintf (stderr, "error in deriv5\n");
          } else if (isnan(deriv->data.F32[6])) {
            fprintf (stderr, "error in deriv6\n");
          } else if (isnan(deriv->data.F32[7])) {
            fprintf (stderr, "error in deriv7\n");
          } else if (isnan(deriv->data.F32[8])) {
            fprintf (stderr, "error in deriv8\n");
          }
          */



        } else if (xs > length/2.0){

          q=PAR[1]*r*rx;
          deriv->data.F32[2] = -q*(rx*tx/1.6*x1*PS_SQR(sxrot) + r*t*y1*sxyrot);
          deriv->data.F32[3] = -q*r*t*(y1*PS_SQR(syrot) + x1*sxyrot);
          deriv->data.F32[4] = -q*(rx*tx*x1*sx*PS_SQR(cosT)/1.6*(x1-2.0*cosT/sxrot) + r*t*y1*sx*sinT*(y1*sinT-2.0*PS_SQR(syrot)*PS_SQR(cosT)/(sxrot*sxrot*sxrot)) + 2.0*r*t*sx*(-1.0*x1*y1*sin2T-y1*sxyrot*(cosT*cosT*cosT)/(sxrot*sxrot*sxrot)-x1*sxyrot*(sinT*cosT*cosT)/(sxrot*sxrot*sxrot)));
          deriv->data.F32[5] = -q*(rx*tx*x1*sy*PS_SQR(sinT)/1.6*(x1-2.0*cosT/sxrot) + r*t*y1*sy*(y1*PS_SQR(cosT)-2.0*PS_SQR(syrot)*(sinT*sinT*sinT)/(sxrot*sxrot*sxrot)) + 2.0*r*t*sy*(x1*y1*sin2T-y1*sxyrot*(sinT*sinT*cosT)/(sxrot*sxrot*sxrot)-x1*sxyrot*(sinT*sinT*sinT)/(sxrot*sxrot*sxrot)));
          deriv->data.F32[6] = -q*(rx*tx*x1*cosT*sinT/3.2*(x1-2.0*cosT/sxrot) + r*t*y1*cosT*sinT/2.0*(-y1-2.0*PS_SQR(syrot)*sinT/(sxrot*sxrot*sxrot)) + r*t*(x1*y1*cos2T-y1*sxyrot*sinT*cosT*cosT/(sxrot*sxrot*sxrot)-x1*sxyrot*sinT*sinT*cosT/(sxrot*sxrot*sxrot)));
          deriv->data.F32[7] = q*(rx*tx*PS_SQR(sxrot)*x1*cosT/3.2 + r*t*PS_SQR(syrot)*y1*sinT/2.0 + r*t*sxyrot/2.0*(y1*cosT+x1*sinT));
          deriv->data.F32[8] = -q*(rx*tx*x1/3.2*(x1*(2.0*sinT*cosT*(PS_SQR(sy)-PS_SQR(sx))+sxy*(PS_SQR(cosT)-PS_SQR(sinT)))-PS_SQR(sxrot)*(2.0*cosT*(2.0*sinT*cosT*(PS_SQR(sy)-PS_SQR(sx))+sxy*(PS_SQR(cosT)-PS_SQR(sinT)))/(sxrot*sxrot*sxrot) - sinT*length)) + r*t*y1/2.0*(y1*(2.0*sinT*cosT*(PS_SQR(sx)-PS_SQR(sy))-sxy*(PS_SQR(cosT)-PS_SQR(sinT)))-PS_SQR(syrot)*(2.0*sinT*(2.0*sinT*cosT*(PS_SQR(sy)-PS_SQR(sx))+sxy*(PS_SQR(cosT)-PS_SQR(sinT)))/(sxrot*sxrot*sxrot)+cosT*length)) + r*t*(2.0*x1*y1*(cos2T*(PS_SQR(sy)-PS_SQR(sx))-sxy*sin2T)-y1*sxyrot/2.0*(2.0*cosT*(2.0*sinT*cosT*(PS_SQR(sy)-PS_SQR(sx))+sxy*(PS_SQR(cosT)-PS_SQR(sinT)))/(sxrot*sxrot*sxrot)-length*sinT)-x1*sxyrot/2.0*(2.0*sinT*(2.0*sinT*cosT*(PS_SQR(sy)-PS_SQR(sx))+sxy*(PS_SQR(cosT)-PS_SQR(sinT)))/(sxrot*sxrot*sxrot)+length*cosT)));


          /*      if (isnan(deriv->data.F32[2])) {
            fprintf (stderr, "error in deriv2\n");
          } else if (isnan(deriv->data.F32[3])) {
            fprintf (stderr, "error in deriv3\n");
          } else if (isnan(deriv->data.F32[4])) {
            fprintf (stderr, "error in deriv4\n");
          } else if (isnan(deriv->data.F32[5])) {
            fprintf (stderr, "error in deriv5\n");
          } else if (isnan(deriv->data.F32[6])) {
            fprintf (stderr, "error in deriv6\n");
          } else if (isnan(deriv->data.F32[7])) {
            fprintf (stderr, "error in deriv7\n");
          } else if (isnan(deriv->data.F32[8])) {
            fprintf (stderr, "error in deriv8\n");
          }
          */

        } else {
          // this does not change from before, as the y' falloff can be modelled by the standard 3rd order gaussian
          // note difference from a pure gaussian: q = PAR[1]*r
          q = PAR[1]*r*r*t;
          deriv->data.F32[2] = q*(PS_SQR(sx*sinT)*x1 - PS_SQR(sy)*y1*cosT*sinT - sxy*x1*sinT*cosT + sxy*y1*PS_SQR(sinT));
          deriv->data.F32[3] = q*(-1*PS_SQR(sx)*x1*sinT*cosT + PS_SQR(sy*cosT)*y1 + sxy*x1*PS_SQR(cosT) - sxy*y1*sinT*cosT);
          deriv->data.F32[4] = -q*sx*PS_SQR(x1);
          deriv->data.F32[5] = -q*sy*PS_SQR(y1);
          deriv->data.F32[6] = -q*x1*y1;
          deriv->data.F32[7] = 0;
          deriv->data.F32[8] = -q*( PS_SQR(sx)*x1*(xs*sinT - ys*cosT) + PS_SQR(sy)*y1*(xs*cosT - ys*sinT) + sxy*x1*(xs*cosT - ys*sinT) + sxy*y1*(xs*sinT - ys*cosT) );

          if (isnan(deriv->data.F32[2])) {
            fprintf (stderr, "error in deriv2\n");
          } else if (isnan(deriv->data.F32[3])) {
            fprintf (stderr, "error in deriv3\n");
          } else if (isnan(deriv->data.F32[4])) {
            fprintf (stderr, "error in deriv4\n");
          } else if (isnan(deriv->data.F32[5])) {
            fprintf (stderr, "error in deriv5\n");
          } else if (isnan(deriv->data.F32[6])) {
            fprintf (stderr, "error in deriv6\n");
          } else if (isnan(deriv->data.F32[7])) {
            fprintf (stderr, "error in deriv7\n");
          } else if (isnan(deriv->data.F32[8])) {
            fprintf (stderr, "error in deriv8\n");
          }


        }
    }
    return(f);
}

//fixed
// XXX this needs to apply the axis ratio limits to prevent avoid solutions
# define AR_MAX 20.0
# define AR_RATIO 0.99
bool PM_MODEL_LIMITS (psMinConstraintMode mode, int nParam, float *params, float *beta) {

    float beta_lim = 0;
    float params_min = 0;
    float params_max = 0;
    float f1, f2, q1;
    float q2 = 0;

    // we need to calculate the limits for SXY specially
    if (nParam == PM_PAR_SXY) {
        f1 = 1.0 / PS_SQR(params[PM_PAR_SYY]) + 1.0 / PS_SQR(params[PM_PAR_SXX]);
        f2 = 1.0 / PS_SQR(params[PM_PAR_SYY]) - 1.0 / PS_SQR(params[PM_PAR_SXX]);
        q1 = PS_SQR(f1)*AR_RATIO - PS_SQR(f2);
        assert (q1 > 0);
        q2  = 0.5*sqrt (q1);
    }

    switch (mode) {
      case PS_MINIMIZE_BETA_LIMIT:
        switch (nParam) {
          case PM_PAR_SKY:  beta_lim = 1000;   break;
          case PM_PAR_I0:   beta_lim = 10000;    break; // too small?
          case PM_PAR_XPOS: beta_lim = 50;     break;
          case PM_PAR_YPOS: beta_lim = 50;     break;
          case PM_PAR_SXX:  beta_lim = 0.5;    break;
          case PM_PAR_SYY:  beta_lim = 0.5;    break;
          case PM_PAR_SXY:  beta_lim = 1.0;    break;  // set this to q2?
          case 7:           beta_lim = 10.0;     break;
          case 8:           beta_lim = M_PI/6.0; break;

          default:
            psAbort("invalid parameter %d for beta test", nParam);
        }
        if (fabs(beta[nParam]) > fabs(beta_lim)) {
            beta[nParam] = (beta[nParam] > 0) ? fabs(beta_lim) : -fabs(beta_lim);
            return false;
        }
        return true;
      case PS_MINIMIZE_PARAM_MIN:
        switch (nParam) {
          case PM_PAR_SKY:  params_min = -1000; break;
          case PM_PAR_I0:   params_min =     0; break;
          case PM_PAR_XPOS: params_min =  -100; break;
          case PM_PAR_YPOS: params_min =  -100; break;
          case PM_PAR_SXX:  params_min =   0.5; break;
          case PM_PAR_SYY:  params_min =   0.5; break;
          case PM_PAR_SXY:  params_min =  -5.0; break; // set this to -q2?
          case 7:           params_min =     0;  break;
          case 8:           params_min = -1*M_PI; break;

          default:
            psAbort("invalid parameter %d for param min test", nParam);
        }
        if (params[nParam] < params_min) {
            params[nParam] = params_min;
            return false;
        }
        return true;
      case PS_MINIMIZE_PARAM_MAX:
        switch (nParam) {
          case PM_PAR_SKY:  params_max =   1e5; break;
          case PM_PAR_I0:   params_max =   1e8; break;
          case PM_PAR_XPOS: params_max =   1e4; break;
          case PM_PAR_YPOS: params_max =   1e4; break;
          case PM_PAR_SXX:  params_max =   100; break;
          case PM_PAR_SYY:  params_max =   100; break;
          case PM_PAR_SXY:  params_max =   +q2; break;
          case 7:           params_max =   150; break;
          case 8:           params_max =  M_PI; break;
          default:
            psAbort("invalid parameter %d for param max test", nParam);
        }
        if (params[nParam] > params_max) {
            params[nParam] = params_max;
            return false;
        }
        return true;
      default:
        psAbort("invalid choice for limits");
    }
    psAbort("should not reach here");
    return false;
}

//fixed
psF64 PM_MODEL_FLUX(const psVector *params)
{
    float f, norm, z;

    psF32 *PAR = params->data.F32;

    psF64 A1   = PS_SQR(PAR[4]);
    psF64 A2   = PS_SQR(PAR[5]);
    psF64 A3   = PS_SQR(PAR[6]);
    psF32 Rx=2./PS_SQR(PAR[4]);
    psF32 Ry=2./PS_SQR(PAR[5]);
    psF32 Rxy=PAR[6];


    psF32 theta = PAR[8];
    psF32 sinT=sin(theta);
    psF32 cosT=cos(theta);

    psF32 Syrot = ( PS_SQR(sinT)/Rx + PS_SQR(cosT)/Ry - Rxy*sinT*cosT );  //rotated sigma y

    psF64 A4   = Syrot*PAR[7];

    psF64 Area = 2.0 * M_PI / sqrt(A1*A2 - A3) + A4;
    // Area is equivalent to 2 pi sigma^2 + rectangle

    // the area needs to be multiplied by the integral of f(z)
    norm = 0.0;
    for (z = 0.005; z < 50; z += 0.01) {
        f = 1.0 / (1 + z + z*z/2 + z*z*z/6);
        norm += f;
    }
    norm *= 0.01;

    psF64 Flux = params->data.F32[1] * Area * norm;

    return(Flux);
}

// define this function so it never returns Inf or NaN
// also prevent 0 returns, and just send a v small number
// return the radius which yields the requested flux

//fixed, but need to change how it is called to accomodate 2 radii
psF64 PM_MODEL_RADIUS  (const psVector *params, psF64 flux)
{
    if (flux <= 0) return (1.0);
    if (params->data.F32[1] <= 0) return (1.0);
    if (flux >= params->data.F32[1]) return (1.0);

    psF32 *PAR = params->data.F32;
    psF32 sigma  = sqrt(2.0) * hypot (1.0 / params->data.F32[4], 1.0 / params->data.F32[5]);

    psF32 theta = PAR[8];
    psF32 sinT=sin(theta);
    psF32 cosT=cos(theta);
    psF32 Rx=2./PS_SQR(PAR[4]);
    psF32 Ry=2./PS_SQR(PAR[5]);
    psF32 Rxy=PAR[6];
    psF32 length=PAR[7];

    psF32 Syrot = ( PS_SQR(sinT)/Rx + PS_SQR(cosT)/Ry - Rxy*sinT*cosT );  //rotated sigma y

    psF64 radius = 0;
    if (flux > 0){
      psF64 radius0 = sigma * sqrt (2.0 * log(params->data.F32[1] / flux));
      psF64 radius1 = Syrot * sqrt (2.0 * log(params->data.F32[1] / flux));

      if (radius0 > radius1) {
        radius=radius0+length/2.0;
      } else {
        radius=radius1+length/2.0;
      }
    } else {
      radius = 1000;
    }

    if (radius < 0.01){
      radius = 0.01;
    }

    if (isnan(radius)) {
      fprintf (stderr, "error in code\n");
    }
    return (radius);
}

//fixed I think...no good way of guessing as far as I can tell
bool PM_MODEL_GUESS (pmModel *model, pmSource *source, psImageMaskType maskVal, psImageMaskType markVal)
{
    pmMoments *Smoments = source->moments;
    pmPeak    *peak    = source->peak;
    psF32     *params  = model->params->data.F32;

    psEllipseAxes axes;
    psEllipseShape shape;
    psEllipseMoments moments;

    moments.x2 = Smoments->Mxx;
    moments.y2 = Smoments->Myy;
    moments.xy = Smoments->Mxy;
    //sometimes these moment inputs are zero...why?

    // solve the math to go from Moments To Shape
    // limit the axis ratio to 20 (a guess)
    axes = psEllipseMomentsToAxes(moments, 20.0);
    shape = psEllipseAxesToShape(axes);

    params[PM_PAR_SKY]  = Smoments->Sky;
    params[PM_PAR_I0]   = peak->rawFlux;
    params[PM_PAR_XPOS] = peak->xf;
    params[PM_PAR_YPOS] = peak->yf;

    params[7] = 2 * axes.major;
    params[8] = axes.theta;

    if (moments.x2 == 0 || moments.y2 == 0){
      params[4] = 2.0;
      params[5] = 2.0;
      params[6] = 0.0;
    } else {
      params[4] = 1.0 / shape.sx;
      params[5] = 1.0 / shape.sy;
      params[6] = shape.sxy;
    }

    //    printf("Who is NaN? momx: %4.3f  momy: %4.3f  momxy: %4.3f\n", moments.x2,moments.y2, moments.xy);

    return(true);
}

//fixed
bool PM_MODEL_FROM_PSF (pmModel *modelPSF, pmModel *modelFLT, const pmPSF *psf)
{
    psF32 *out = modelPSF->params->data.F32;
    psF32 *in  = modelFLT->params->data.F32;

    out[0] = in[0];
    out[1] = in[1];
    out[2] = in[2];
    out[3] = in[3];
    out[7] = in[7];
    out[8] = in[8];

    for (int i = 4; i < 7; i++) {
      pmTrend2D *trend = psf->params->data[i-4];
        out[i] = pmTrend2DEval (trend, out[2], out[3]);
    }
    return(true);
}

// construct the PSF model from the FLT model and the psf
// XXX is this sufficiently general do be a global function, not a pmModelClass function?
bool PM_MODEL_PARAMS_FROM_PSF (pmModel *model, const pmPSF *psf, float Xo, float Yo, float Io)
{
    psF32 *PAR = model->params->data.F32;

    // we require these two parameters to exist
    assert (psf->params->n > PM_PAR_YPOS);
    assert (psf->params->n > PM_PAR_XPOS);

    PAR[PM_PAR_SKY]  = 0.0;
    PAR[PM_PAR_I0]   = Io;
    PAR[PM_PAR_XPOS] = Xo;
    PAR[PM_PAR_YPOS] = Yo;

    // supply the model-fitted parameters, or copy from the input
    for (int i = 0; i < psf->params->n; i++) {
        if (i == PM_PAR_SKY) continue;
        pmTrend2D *trend = psf->params->data[i];
        PAR[i] = pmTrend2DEval(trend, Xo, Yo);
    }

    // the 2D PSF model fits polarization terms (E0,E1,E2)
    // convert to shape terms (SXX,SYY,SXY)
    // XXX user-defined value for limit?
    bool useReff = pmModelUseReff (model->type);
    if (!pmPSF_FitToModel (PAR, 0.1, useReff)) {
        psError(PM_ERR_PSF, false, "Failed to fit object at (r,c) = (%.1f,%.1f)", Xo, Yo);
        return false;
    }

    // apply the model limits here: this truncates excessive extrapolation
    // XXX do we need to do this still?  should we put in asserts to test?
    for (int i = 0; i < psf->params->n; i++) {
        // apply the limits to all components or just the psf-model parameters?
        if (psf->params->data[i] == NULL)
            continue;

        bool status = true;
        status &= PM_MODEL_LIMITS (PS_MINIMIZE_PARAM_MIN, i, PAR, NULL);
        status &= PM_MODEL_LIMITS (PS_MINIMIZE_PARAM_MAX, i, PAR, NULL);
        if (!status) {
            psTrace ("psModules.objects", 5, "Hitting parameter limits at (r,c) = (%.1f, %.1f)", Xo, Yo);
            model->flags |= PM_MODEL_STATUS_LIMITS;
        }
    }
    return(true);
}

//done I think
bool PM_MODEL_FIT_STATUS (pmModel *model)
{
    psF32 dP;
    bool  status;

    psF32 *PAR  = model->params->data.F32;
    psF32 *dPAR = model->dparams->data.F32;

    dP = 0;
    dP += PS_SQR(dPAR[4] / PAR[4]);
    dP += PS_SQR(dPAR[5] / PAR[5]);
    dP = sqrt (dP);

    status = true;
    status &= (dP < 0.5);
    status &= (PAR[1] > 0);
    status &= ((dPAR[1]/PAR[1]) < 0.5);
    //    status &= ((dPAR[7]/PAR[7]) < 0.5);
    //    status &= ((dPAR[8]/PAR[8]) < 0.5);

    if (status) return true;
    return false;
}

# undef PM_MODEL_FUNC
# undef PM_MODEL_FLUX
# undef PM_MODEL_GUESS
# undef PM_MODEL_LIMITS
# undef PM_MODEL_RADIUS
# undef PM_MODEL_FROM_PSF
# undef PM_MODEL_PARAMS_FROM_PSF
# undef PM_MODEL_FIT_STATUS
