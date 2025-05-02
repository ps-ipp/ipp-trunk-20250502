#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <assert.h>
#include "psConstants.h"
#include "psTrace.h"
#include "psEllipse.h"

// we have four representations of the elliptical profile of a Gaussian
// f = exp(-z) where z describes the elliptical contour at any flux level:

// NOTE: major, minor are the 1-sigma lengths in the major,minor directions assuming the
// moments represent a Gaussian profile

// sigma shape: z = 0.5((x/sx)^2 + (y/sy)^2 + sxy*x*y)
// sigma axes : z = 0.5((x/sa)^2 + (y/sb)^2), x,y rotated by theta 
// moments    : z = 0.5(x^2/Mxx + y^2/Myy + x*y*Mxy)
// polarization : e0, e1, e2

// ellipse rotation (major, minor, theta) -> (Mxx, Mxy, Myy)
psEllipseMoments psEllipseAxesToMoments(psEllipseAxes axes)
{
    psEllipseMoments moments;

    double f1 = PS_SQR(axes.major) + PS_SQR(axes.minor);
    double f2 = PS_SQR(axes.major) - PS_SQR(axes.minor);

    moments.x2 = +0.5*f1 + 0.5*f2*cos(2*axes.theta);
    moments.y2 = +0.5*f1 - 0.5*f2*cos(2*axes.theta);
    moments.xy = +0.5*f2*sin(2*axes.theta);

    assert (isfinite(moments.x2));
    assert (isfinite(moments.y2));
    assert (isfinite(moments.xy));

    return moments;
}

// ellipse rotation (Mxx, Mxy, Myy) -> (major, minor, theta).
psEllipseAxes psEllipseMomentsToAxes(psEllipseMoments moments, double maxAR)
{
    psEllipseAxes axes;
    psEllipseAxes badValue = {NAN, NAN, NAN};

    if (!isfinite(moments.x2)) return badValue;
    if (!isfinite(moments.y2)) return badValue;
    if (!isfinite(moments.xy)) return badValue;

    if (moments.x2 < 0) return badValue;
    if (moments.y2 < 0) return badValue;

    double g1 = moments.x2 + moments.y2;
    double g2 = moments.x2 - moments.y2;
    double g3 = sqrt(PS_SQR(g2) + 4*PS_SQR(moments.xy));

    axes.major = sqrt (0.5*(g1 + g3));
    axes.theta = +0.5 * atan2 (+2.0*moments.xy, g2); // theta in radians

    // long, thin objects are likely to have a poorly measured minor axis
    // the angle and major axis are likely to be ok.
    // restrict the axis ratio
    double rAR2 = (g1 - g3) / (g1 + g3);
    if (rAR2 < 1.0/PS_SQR(maxAR)) {
        axes.minor = axes.major / maxAR;
    } else {
        axes.minor = sqrt (0.5*(g1 - g3));
    }

    assert (isfinite(axes.major));
    assert (isfinite(axes.minor));
    assert (isfinite(axes.theta));

    return axes;
}

// ellipse rotation (major, minor, theta) -> (sx, sy, sxy)
// theta is postive rotation of major axis away from x-axis
psEllipseShape psEllipseAxesToShape(psEllipseAxes axes)
{
    psEllipseShape shape;
    psEllipseShape badValue = {NAN, NAN, NAN};

    if (!isfinite(axes.minor)) return badValue;
    if (!isfinite(axes.major)) return badValue;
    if (!isfinite(axes.theta)) return badValue;

    if (axes.minor <= 0) return badValue;
    if (axes.major <= 0) return badValue;

    double f1 = 1.0 / PS_SQR(axes.minor) + 1.0 / PS_SQR(axes.major);
    double f2 = 1.0 / PS_SQR(axes.minor) - 1.0 / PS_SQR(axes.major);

    double sxr = 0.5*f1 - 0.5*f2*cos(2*axes.theta);
    double syr = 0.5*f1 + 0.5*f2*cos(2*axes.theta);

    // sxr, syr cannot be < 0 (f1 >= f2)

    shape.sx  = +1.0 / sqrt(sxr);
    shape.sy  = +1.0 / sqrt(syr);
    shape.sxy = -0.5*f2*sin(2*axes.theta);

    assert (isfinite(shape.sx));
    assert (isfinite(shape.sy));
    assert (isfinite(shape.sxy));

    return shape;
}

// ellipse derotation (sx, sy, sxy) -> (major, minor, theta)
psEllipseAxes psEllipseShapeToAxes(psEllipseShape shape, double maxAR)
{
    psEllipseAxes axes;

    double f1 = 1.0 / PS_SQR(shape.sy) + 1.0 / PS_SQR(shape.sx);
    double f2 = 1.0 / PS_SQR(shape.sy) - 1.0 / PS_SQR(shape.sx);
    double f3 = sqrt(PS_SQR(f2) + 4*PS_SQR(shape.sxy));

    axes.minor = sqrt (2.0 / (f1 + f3));
    axes.theta = -0.5 * atan2 (+2.0*shape.sxy, f2);

    // long, thin objects are likely to have a poorly measured major axis
    // the angle and minor axis are likely to be ok.
    // restrict the axis ratio
    double rAR2 = (f1 - f3) / (f1 + f3);
    if (rAR2 < 1.0/PS_SQR(maxAR)) {
        axes.major = axes.minor * maxAR;
    } else {
        axes.major = sqrt (2.0 / (f1 - f3));
    }

    assert (isfinite(axes.theta));
    assert (isfinite(axes.major));
    assert (isfinite(axes.minor));

    return axes;
}
// ellipse derotation (sx, sy, sxy) -> (major, minor, theta)
psEllipseAxes psEllipseShapeToAxesWithErrors(psEllipseShape shape, psEllipseShape shapeErrors, double maxAR, psEllipseAxes *pShapeErrors)
{
    psEllipseAxes axes;
    psEllipseAxes errors;

    double f1 = 1.0 / PS_SQR(shape.sy) + 1.0 / PS_SQR(shape.sx);
    double f2 = 1.0 / PS_SQR(shape.sy) - 1.0 / PS_SQR(shape.sx);

    double f32 = PS_SQR(f2) + 4*PS_SQR(shape.sxy);
    double f3 = sqrt(f32);

    double df1 = - 2.0 * shapeErrors.sy / (PS_SQR(shape.sy) * shape.sy)
                 - 2.0 * shapeErrors.sx / (PS_SQR(shape.sx) * shape.sx);
    double df2 = - 2.0 * shapeErrors.sy / (PS_SQR(shape.sy) * shape.sy)
                 + 2.0 * shapeErrors.sx / (PS_SQR(shape.sx) * shape.sx);

    // df3 = 0.5 * df32 / sqrt(f32) = 0.5 * df32 / f3
    double df3 = 0.5 * (2.0*f2*df2 + 2 * 4 * shape.sxy * shapeErrors.sxy) / f3;

    double f13 = f1 + f3;
    axes.minor = sqrt (2.0 / f13);

    // dminor = -0.5 * sqrt (2) * df13 * f13**-3/2  = - 0.5 * df13 * minor / f13
    errors.minor = - 0.5 * (df1 + df3) * axes.minor / f13;

    axes.theta = -0.5 * atan2 (+2.0*shape.sxy, f2);

    // according to wikipedia the derivitive of atan2(y, x) is
    //
    //  dAtan2(y, x) = - y * dx / (x**2 + y**2)) + x * dy / (x**2 + y**2) (for x > 0 and y!=0)
    //  where
    //  y = 2 * sxy  dy = 2 * dsxy   x = f2 so dx = df2

    // dtheta = -0.5 * dAtan2(y, x)

    errors.theta = -0.5 * ( -2.0 * shape.sxy * df2  +  f2 * 2 *shapeErrors.sxy ) / 
                            (PS_SQR(f2) + 4 * PS_SQR(shape.sxy)) ;

    // long, thin objects are likely to have a poorly measured major axis
    // the angle and minor axis are likely to be ok.
    // restrict the axis ratio
    double rAR2 = (f1 - f3) / (f1 + f3);
    if (rAR2 < 1.0/PS_SQR(maxAR)) {
        axes.major = axes.minor * maxAR;
        errors.major = errors.minor * maxAR;
    } else {
        axes.major = sqrt (2.0 / (f1 - f3));

        // dmajor = -0.5 * (df1 - df3) * sqrt(2) * (f1 - f3)**-3/2
        //        = -0.5 * (df2 - df3) * major / (f1 - f3)
        errors.major = -0.5 * axes.major * (df1 - df3) / (f1 - f3);
    }

    assert (isfinite(axes.theta));
    assert (isfinite(axes.major));
    assert (isfinite(axes.minor));

    *pShapeErrors = errors;

    return axes;
}

// ellipse rotation (major, minor, theta) -> (e0, e1, e2)
psEllipsePol psEllipseAxesToPol(psEllipseAxes axes)
{
    psEllipsePol pol;

    pol.e0 = PS_SQR(axes.major) + PS_SQR(axes.minor);
    double ds = PS_SQR(axes.major) - PS_SQR(axes.minor);

    pol.e1 = ds*cos(2*axes.theta);
    pol.e2 = ds*sin(2*axes.theta);

    assert (isfinite(pol.e0));
    assert (isfinite(pol.e1));
    assert (isfinite(pol.e2));

    return pol;
}

// ellipse rotation (e0, e1, e2) -> (major, minor, theta)
psEllipseAxes psEllipsePolToAxes(const psEllipsePol pol,
				 const float minMinorAxis)
{
    psEllipseAxes axes;

    axes.theta = +0.5 * atan2 (pol.e2, pol.e1); // theta in radians

    double cs = cos(2*axes.theta);
    double sn = sin(2*axes.theta);
    double ds = 0;

    if ((cs > 0.707) || (cs < -0.707)) {
	ds = pol.e1 / cs;
    } else {
	ds = pol.e2 / sn;
    }

    float LIMIT = PS_SQR(minMinorAxis);
    if (pol.e0 < LIMIT) {
	// if e0 is too small, we are really out of luck
	axes.major = minMinorAxis;
	axes.minor = minMinorAxis;
    } else {
	if (2.0*(pol.e0 - ds) < LIMIT) {
	    // if e0 - ds is too small, saturate the minor axis at minMinorAxis
	    axes.major = sqrt(pol.e0 - LIMIT);
	    axes.minor = sqrt(LIMIT);
	} else {
	    // normal values for e0 & ds
	    axes.major = sqrt(0.5*(pol.e0 + ds));
	    axes.minor = sqrt(0.5*(pol.e0 - ds));
	}
    }

    if (!isfinite(axes.major) || !isfinite(axes.minor) || !isfinite(axes.theta)) {
	psTrace ("psLib.math", 5, "Shape of object is NaN");
    }
    return axes;
}

// ellipse rotation (sx, sy, sxy) -> (e0, e1, e2)
psEllipsePol psEllipseShapeToPol(psEllipseShape shape)
{
    psEllipsePol pol;

    double r = 1.0 / (1.0 - PS_SQR(shape.sx)*PS_SQR(shape.sy)*PS_SQR(shape.sxy));

    pol.e0 = r*(PS_SQR(shape.sx) + PS_SQR(shape.sy));
    pol.e1 = r*(PS_SQR(shape.sx) - PS_SQR(shape.sy));
    // XXX I do not understand this negative sign
    pol.e2 = -r*(2.0*PS_SQR(shape.sx)*PS_SQR(shape.sy)*shape.sxy);

    assert (isfinite(pol.e0));
    assert (isfinite(pol.e1));
    assert (isfinite(pol.e2));

    return pol;
}

// ellipse rotation (e0, e1, e2) -> (sx, sy, sxy)
psEllipseShape psEllipsePolToShape(psEllipsePol pol)
{
    psEllipseShape shape;

    double q = sqrt (PS_SQR(pol.e1) + PS_SQR(pol.e2));
    double p = PS_SQR(pol.e0) + PS_SQR(q) - 2.0*q*pol.e0;

    // double f1 = 4*pol.e0 / p;
    // double f2 = 4*q      / p;
    
    double sxr = 2.0*(pol.e0 - pol.e1) / p;
    double syr = 2.0*(pol.e0 + pol.e1) / p;

    shape.sx = sqrt(1.0 / sxr);
    shape.sy = sqrt(1.0 / syr);

    shape.sxy = -2.0 * pol.e2 / p;

    assert (isfinite(shape.sx));
    assert (isfinite(shape.sy));
    assert (isfinite(shape.sxy));

    return shape;
}

// XXX keep this construction?
// force the axis ratio to be less than 10
// double r1 = 0.5*0.95*sqrt (PS_SQR(f1) - PS_SQR(f2));

//    double f = sqrt (0.25*PS_SQR(moments.x2 - moments.y2) + PS_SQR(moments.xy));
//    if (f > (moments.x2 + moments.y2) / 2.0) {
//        f = 0.98*(moments.x2 + moments.y2) / 2.0;
