      subroutine jtpsf(nx, ny, a, eadu, iaprad, iskyrad, 
     $     nwpar, wpar, flux, ierr)
* Data array A(NX,NY)
* Noise scale EADU (i.e. variance = EADU * Sky_level)
* Radius for aperture flux = IAPRAD
* Radius for sky estimates = ISKYRAD
* Order of Waussian fit = NWPAR
* Coords of star = WPAR(1),WPAR(2)
* Results of Waussian fit = WPAR:
*   WPAR( 1) = x0 of fit (TK convention!)
*   WPAR( 2) = y0 of fit
*   WPAR( 3) = peak of Waussian
*   WPAR( 4) = sky of Waussian fit
*   WPAR( 5) = sx2
*   WPAR( 6) = sxy
*   WPAR( 7) = sy2
*   WPAR( 8) = beta4 
*   WPAR( 9) = beta6
*   WPAR(10) = major axis fwhm of Waussian fit
*   WPAR(11) = minor axis fwhm of Waussian fit
*   WPAR(12) = angle of major axis (CCW from x axis) [rad]
*   WPAR(13) = chi^2/Ndof of fit
*   WPAR(14) = rms residual within a radius of 2 sigma
*   WPAR(15) = average absolute residual within a radius of 2 sigma
*   WPAR(16) = max residual within a radius of 2 sigma
* Results of sky and aperture flux fit = FLUX:
*   FLUX(1) = sky
*   FLUX(2) = sky uncertainty
*   FLUX(3) = flux
*   FLUX(4) = flux uncertainty
*   FLUX(5) = rms in sky
* Error code = IERR
*                   0: no error, 
*                  -1: off image, 
*                   1: fit didn't converge,
*                N*10: N zero'ed pixels within FWHM
*
* Function WAUSS(X,Y,WPAR,Z2) provided below for Waussian evaluation at (x,y)
*
* v. 1.11 030527 (JT) added zero pixel error flag
*
* v. 1.10 030330 (JT) fixed a few bugs, cleaned up the logic a bit, 
*      added proper calculation of chi^2/N, FLUX(5) is a new argument, BEWARE
*
* Initial version 1.0 030301 John Tonry, adapted from JT Vista fondle()
*
      parameter (RESIDCUT=2.0)
      real a(nx,ny)
      real wpar(16), flux(5)
      parameter (MAXRAD=64)
      real pmed(MAXRAD+1), pave(MAXRAD+1), prms(MAXRAD+1)
      integer npix(MAXRAD+1), ntot(MAXRAD+1)
      real smooth(MAXRAD+1), profile(MAXRAD+1)
* 768 = MAXPT (from pixmedave) * sqrt(2) * (1+fudge)
      real buf(768*MAXRAD)
      real ring(MAXRAD), area(MAXRAD), avering(MAXRAD), ering(MAXRAD)
      real waux(10)
      real*8 cov(9*9)
      character*1000 DEBUG

      IGAUSSRAD = 2
      FWHMGAUSS = 2*sqrt(2*alog(2.))

      profile(1) = 0

      ix = nint(wpar(1))
      iy = nint(wpar(2))

* Abort this star if it's off the image
      if(ix.lt.1 .or. ix.gt.nx .or. iy.lt.1 .or. iy.gt.ny) then
         ierr = -1
         return
      end if

****************************************************************
C Find the highest pixel and get some crude information
      call crudemax(ix,iy,nx,ny,a,mx,my,peak,crudefw,crudeback)

C      WRITE(DEBUG,*) 'MX, MY, PEAK, CRUDEFW, CRUDEBACK',
C    $     MX, MY, PEAK, CRUDEFW, CRUDEBACK
C     CALL F77MSG(DEBUG)

C     Get medians and averages at all radii...
C      mxr = max(16, min(nint(15*crudefw),MAXRAD))
      mxr = iskyrad
      if(mxr .le. 0 .or. mxr .gt. MAXRAD) mxr = MAXRAD
      call pixmedave(mx,my,nx,ny,a,mxr,npix,ntot,pave,pmed,prms,buf)

C     Get an estimated values for SKY and FWHM
      call estsky(fwhm,mxr+1,pmed,prms,esky,eskyerr,rms)

C      WRITE(DEBUG,*) 'ESKY, ESKYERR, RMS',
C     $     ESKY, ESKYERR, RMS
C      CALL F77MSG(DEBUG)

C     Get a fitted value for SKY and FWHM
      do 5 i = 1,mxr+1
         smooth(i) = ((3.*fwhm)/max(1,i-1))**3
 5    continue
      m1 = (mxr+1)/2
      nfit = mxr+1 - m1 + 1

      call fitsky(nfit,pmed(m1),smooth(m1),ampl,amperr,sky,dsky)

C      WRITE(DEBUG,*) 'AMPL, APERR, SKY, DSKY',
C     $     AMPL, APERR, SKY, DSKY
C      CALL F77MSG(DEBUG)

C     Add up flux in annuli of width fwhm/2 or 5
      iwidth = max(5,nint(fwhm/2))
      nrad = mxr/iwidth
      do 6 i = 1,nrad
         area(i) = 0
         ring(i) = 0
         ering(i) = 0
         avering(i) = 0
 6    continue
      do 7 i = 1,mxr+1
         idx = (i-1) / iwidth + 1
*     Get the total flux = AVESUM
         avesum = (pave(i)-sky) * ntot(i)
         sum = avesum
*     Make a more robust estimate of total flux from SUM = MEDIAN * N
         if(ntot(i).gt.60) sum = (pmed(i)-sky) * ntot(i)
         area(idx) = area(idx) + ntot(i)
         ring(idx) = ring(idx) + sum
         avering(idx) = avering(idx) + avesum
         ering(idx) = ering(idx) + sum + (sky-esky)*ntot(i)
 7    continue

* Add up the total flux
      call pixsum(mx,my,nx,ny,a,iaprad,ntotal,ave,apflux,peak)
      apflux = apflux - ntotal*sky
      dflux = apflux/eadu + (ntotal*dsky)**2 + rms*rms*ntotal
      if(dflux .ge. 0) then
         dflux = sqrt(dflux)
      else
         dflux = -sqrt(-dflux)
      end if
      flux(1) = sky
      flux(2) = dsky
      flux(3) = apflux
      flux(4) = dflux
      flux(5) = rms

C      WRITE(DEBUG,*) 'SKY, DSKY, APFLUX, DFLUX, RMS', 
C     $     DSKY, APFLUX, DFLUX, RMS, SKY
C      CALL F77MSG(DEBUG)


************************************************************************
* Now do a Waussian fit to the data using all we know for initial values
* How many FWHM do we fit?
      WRAD = 2.5
* How many FWHM do we use for evaluating chi/N?
      WCHI = 2.0
* Guess at extrasky to meet noise seen by estsky
      if(rms.ne.0 .and. esky.ne.0) then
         extrasky = eadu*rms*rms - esky
      else
* Disable weighting if something's really wrong with the "sky" level and rms
         extrasky = -1e10
      end if
C WRITE(6,*) EADU, RMS, ESKY, EXTRASKY

* What do we want for a center guess?
      if(nwpar.ge.7) then
* Use the positive peak...
         n = max(3,min(31,max(IGAUSSRAD,nint(WRAD*fwhm))))
         iwxs = max(0,mx-n)
         iwys = max(0,my-n)
      else
         call domajmin(wpar)
         n = max(3,min(31,max(IGAUSSRAD,
     $        nint(WRAD*sqrt(abs(wpar(10)*wpar(11)))))))
         iwxs = max(0, nint(wpar(1))-n)
         iwys = max(0, nint(wpar(2))-n)
      end if


* WPAR: X0, Y0, P, SKY, SX2, SXY, SY2, B4, B6, FWX, FWY, PHI
      if(nwpar.ge.7) then
         wpar(1) = mx
         wpar(2) = my
      end if
* Waussian fit (WPAR(8) = BETA4 = 1, WPAR(9) = BETA6 = 0.5)
      wpar(8) = 1.0
      wpar(9) = 0.5
* WAUX: NXPATCH, NYPATCH, NX, XOFF, YOFF, EADU, EXTRASKY, IGNORE_VALUE, INIT
C      waux(1) = min(n,nx-1-mx) + n + 1
C      waux(2) = min(n,ny-1-my) + n + 1
      waux(1) = min(2*n+1, nx-1-iwxs)
      waux(2) = min(2*n+1, ny-1-iwys)
      waux(3) = nx
      waux(4) = iwxs
      waux(5) = iwys
      waux(6) = eadu
      waux(7) = extrasky
* Bad data value, by convention 0.0 for JT
      waux(8) = 0.0
      waux(9) = 1
* Don't dare leap to all 9 params at once
      if(nwpar.gt.7) then
         niter = 20
         call waussfit(a(iwxs+1,iwys+1),7,wpar,waux,niter,cov,chisq)
      end if
      if(nwpar.gt.2) then
         niter = 20
         call waussfit(a(iwxs+1,iwys+1),nwpar,wpar,waux,niter,cov,chisq)
      else
         call wausstwo(a(iwxs+1,iwys+1),wpar,waux,chisq)
      end if
      wpar(13) = chisq

      if(niter.eq.20 .or. chisq.eq.0) then
C         ierr = 1
         ierr = niter
         return
      end if

      if(chisq.lt.0) then
         ierr = nint(chisq)
         return
      end if

      ierr = 0
* Improved estimate of chi^2/N
      if(rms .gt. 0 .and. wpar(10).gt.0 .and. wpar(11).gt.0) then
         fw = sqrt(wpar(10)*wpar(11))
         i0 = max(1,  nint(wpar(1)-WCHI*fw+0.5))
         i1 = min(nx, nint(wpar(1)+WCHI*fw+0.5))
         j0 = max(1,  nint(wpar(2)-WCHI*fw+0.5))
         j1 = min(ny, nint(wpar(2)+WCHI*fw+0.5))
         chisq = 0
         npt = 0
         resid = 0
         nresid = 0
         biggie = 0
         absave = 0
         do 20 j = j0, j1
            y = j - 0.5
            do 21 i = i0, i1
               if(a(i,j) .eq. waux(8)) then
                  if((i-wpar(1))*(i-wpar(1))+(j-wpar(2))*(j-wpar(2)) 
     $                 .le. fw*fw) ierr = ierr + 10
                  goto 21
               end if
               x = i - 0.5
               diff = a(i,j) - (wauss(x, y, wpar, z2) + wpar(4))
               chisq = chisq + (diff/rms)**2
               npt = npt + 1
               if(z2 .lt. RESIDCUT) then
                  resid = resid + diff**2
                  nresid = nresid + 1
                  if(abs(diff) .gt. abs(biggie)) biggie = diff
                  absave = absave + abs(diff)
               end if
 21         continue
 20      continue
* Replace wpar(13) with this better chi/N
         wpar(13) = chisq / max(1,npt-nwpar)
         wpar(14) = sqrt(resid / max(1,nresid))
         wpar(15) = absave / max(1,nresid)
         wpar(16) = biggie
      end if

      return
      end

* Compute the value of a Waussian fit at (x,y)  [leaving out the sky]
      function wauss(x, y, wpar, z2)
      real wpar(9)
      x0   = wpar(1)
      y0   = wpar(2)
      peak = wpar(3)
      sky  = wpar(4)
      sx2  = wpar(5)
      sxy  = wpar(6)
      sy2  = wpar(7)
      b4   = wpar(8)
      b6   = wpar(9)
      z2 = sx2*(x-x0)*(x-x0)+sxy*(x-x0)*(y-y0)+sy2*(y-y0)*(y-y0)
      wauss = peak/(1 + z2*(1 + z2*(0.5*b4 + z2*b6/6)))
      return
      end

      subroutine crudemax(ix,iy,nx,ny,a,mx,my,peak,fwhm,back)
C Find from a rough position (IX,IY), the highest point PEAK at (MX,MY),
C the rough FWHM, and a rough BACKground of the image in the wings of the star.
      parameter (pi=3.14159265)
      real a(nx,ny)

C Find the local maximum
      maxstep = 30
      mx = ix
      my = iy
      peak = a(mx+1,my+1)
      do 10 nstep = 1,maxstep
         peak2 = -1e10
         do 11 j = my-1,my+1
            if(j.ge.ny.or.j.lt.0) goto 11
            do 12 i = mx-1,mx+1
               if(i.ge.nx.or.i.lt.0) goto 12
               if(a(i+1,j+1).gt.peak2) then
                  peak2 = a(i+1,j+1)
                  im = i
                  jm = j
               end if
 12         continue
 11      continue
         if(peak2.gt.peak) then
            mx = im
            my = jm
            peak = peak2
         else
            goto 15
         end if
 10   continue
 15   continue

C Run down the profile in the +/-x and y directions to where it turns up
      back = 0
      n = 0
      do 22 k = 0,3
         idx = nint(cos(k*pi/2))
         idy = nint(sin(k*pi/2))
         do 20 l = 1,maxstep
            i = mx + idx*l
            j = my + idy*l
            if(i.ge.nx.or.i.lt.0.or.j.ge.ny.or.j.lt.0) goto 21
            if(a(i+1,j+1).gt.a(i+1-idx,j+1-idy)) then
               n = n + 1
               back = back + a(i+1,j+1)
               goto 21
            end if
 20      continue
 21      continue
 22   continue
      back = back / max(n,1)

C Find the FWHM using this value for BACK
      fwhm = 0
      n = 0
      do 32 k = 0,3
         idx = nint(cos(k*pi/2))
         idy = nint(sin(k*pi/2))
         do 30 l = 1,maxstep
            i = mx + idx*l
            j = my + idy*l
            if(i.ge.nx.or.i.lt.0.or.j.ge.ny.or.j.lt.0) goto 31
            if(a(i+1,j+1).lt.peak-(peak-back)/2) then
               n = n + 1
               fwhm = fwhm + l
               goto 31
            end if
 30      continue
 31      continue
 32   continue
      fwhm = 2 * fwhm / max(n,1) - 1

      return
      end

      subroutine pixmedave(mx,my,nx,ny,a,ir,np,ntot,ave,med,rms,buf)
C Collect average and median of all annuli out to a radius IR
      parameter (maxpt=512)
      real a(nx,ny)
      real ave(1), med(1), rms(1), buf(maxpt,1)
      integer np(1), ntot(1)

      do 10 i = 1,ir+1
         np(i) = 0
         ntot(i) = 0
         ave(i) = 0
 10   continue

C Accumulate pixels from the bottom to top on the right and
C then top to bottom on the left to get the pixels in azimuthal order
      do 20 jp = 0,2*(2*ir+1)
         if(jp.lt.(2*ir+1)) then
            j = jp - ir
            ix1 = 0
            ix2 = ir
         else
            j = 3*ir + 1 - jp
            ix1 = -ir
            ix2 = -1
         end if
         do 21 i = ix1,ix2 
            r = sqrt(float(i*i+j*j))
            k = nint(r) + 1
            if(k.gt.ir+1) goto 21
            ntot(k) = ntot(k) + 1
            if(j+my.ge.ny.or.j+my.lt.0) goto 21
            if(i+mx.ge.nx.or.i+mx.lt.0) goto 21
            pixel = a(i+mx+1,j+my+1)
            np(k) = np(k) + 1
            if(np(k).gt.maxpt) then
C               write(6,*) 'FONDLE (pixmedave): np exceeded maxpt!'
*               call f77msg('FONDLE (pixmedave): np exceeded maxpt!')
               np(k) = maxpt
            end if
            ave(k) = ave(k) + pixel
            buf(np(k),k) = pixel
 21      continue
 20   continue

C Now compute medians.  If there are sufficient points, correct the median
C for skewness by assessing the rms, and counting all points around
C points of greater than 3-sigma which themselves exceed 1-sigma, and
C throwing out those points from the sorted data.

      do 30 k = 1,ir+1
         ave(k) = ave(k) / max(np(k),1)
         do 31 i = 1,np(k)
            buf(i,1) = buf(i,k)
 31      continue

         call qsort4(np(k),buf(1,1))
         amed = 0.5*(buf((np(k)+1)/2,1)+buf((np(k)+2)/2,1))
         nsig = max(1,nint(0.1587*np(k)))
         arms = amed - buf(nsig,1)

C Don't do any funny stuff with radii less than 10 pixels... (N ~ 60)
         if(np(k).le.60) then
            med(k) = amed
            rms(k) = arms
         else
            trigger = amed + 3*arms
C            reset = amed + arms
            reset = amed


C Find a spot which is lower than one-sigma
            n1 = 1
C 32         if(buf(n1,k).gt.reset .and. n1.lt.np(k))  then
 32         if(buf(n1,k).gt.reset)  then
               n1 = n1 + 1
               goto 32
            end if


C Now advance around the circle, counting all pixels higher than RESET
C which are adjacent to a pixel higher than TRIGGER
            nuke = 0
C            n2 = n1 + 1
* But fed to a mod we want to start at n1+1 - 1
            n2 = n1
 33         if(n2-n1.lt.np(k)) then
               if(buf(mod(n2,np(k))+1,k).gt.trigger) then

C Back up to find out where the pixels higher than RESET began
                  i = n2 - 1
 34               if(buf(mod(i,np(k))+1,k).gt.reset) then
                     nuke = nuke + 1
                     i = i - 1
                     goto 34
                  end if

C Advance to find out where the pixels higher than RESET end
 35               if(buf(mod(n2,np(k))+1,k).gt.reset) then
                     nuke = nuke + 1
                     n2 = n2 + 1
                     goto 35
                  end if

               else
                  n2 = n2 + 1
               end if

               goto 33
            end if

C Now recompute the median and rms with NUKE pixels removed off the top
            n = np(k) - nuke
            med(k) = 0.5*(buf((n+1)/2,1)+buf((n+2)/2,1))
            rms(k) = med(k) - buf(nint(0.1587*n),1)
C            WRITE(6,*) K, MED(K), RMS(K)

         end if

 30   continue

      return
      end

      subroutine pixsum(mx,my,nx,ny,a,ir,np,ave,total,peak)
C Collect average and sum of the flux out to a radius IR
      real a(nx,ny)

      total = 0
      np = 0
      peak = 0
      do 20 j = -ir,ir
         iy = j + my + 1
         if(iy.lt.1.or.iy.gt.ny) goto 20
         do 21 i = -ir,ir
            ix = i + mx + 1
            if(ix.lt.1.or.ix.gt.nx) goto 21
            ir2 = i*i + j*j
            if(ir2.gt.ir*ir) goto 21
            np = np + 1
            total = total + a(ix,iy)
            peak = amax1(peak,a(ix,iy))
 21      continue
 20   continue
      ave = total / max(1,np)

      return
      end

      subroutine estsky(fwhm,np,profile,prms,sky,err,rms)
C Get a decent estimate of SKY and RMS from the PROFILE and PRMS, 
C and then get a decent estimate of the FWHM
      real profile(np), prms(np), buf(21)

C First get SKY from a median of the last points in PROFILE
      n = min(np/2,21)
      do 10 i = 1,n
         buf(i) = profile(np-i+1)
 10   continue
      call qsort4(n,buf)
      sky = 0.5*(buf((n+1)/2)+buf((n+2)/2))
      err = (sky - buf(max(1,(n+1)/6))) / sqrt(float(n))

C Now get FWHM from the estimated SKY and the central intensity
      minus = +1
      if(profile(1).lt.sky) minus = -1
      half = 0.5*(profile(1) + sky)
      do 11 i = 1,np
         if(minus*profile(i).le.minus*half) then
            ir2 = i
            goto 12
         end if
         if(minus*profile(i).gt.minus*half) ir1 = i
 11   continue
 12   continue

      budge = 0
      if(profile(ir1).ne.profile(ir2)) 
     $     budge = (ir2-ir1)*(profile(ir1)-half) /
     $     (profile(ir1)-profile(ir2))
      fwhm = 2*(ir1 - 1 + budge)

C Finally get the RMS as the median of the last several rms points
      do 20 i = 1,n
         buf(i) = prms(np-i+1)
 20   continue
      call qsort4(n,buf)
      rms = 0.5*(buf((n+1)/2)+buf((n+2)/2))

      return
      end

      subroutine fitsky(n,profile,smooth,ampl,amperr,sky,err)
C Fit the profile as AMPL * SMOOTH + SKY, and return the error as ERR
      real profile(n), smooth(n)
      real*8 v(2), am(3), sum, sum2, det

      v(1) = 0
      v(2) = 0
      am(1) = 0
      am(2) = 0
      am(3) = 0

C Accumulate sums
      do 20 j = 1,n
         v(1) = v(1) + profile(j)
         v(2) = v(2) + profile(j)*smooth(j)
         am(1) = am(1) + 1
         am(2) = am(2) + smooth(j)*smooth(j)
         am(3) = am(3) + smooth(j)
 20   continue

      det = am(1)*am(2) - am(3)*am(3)
      if(det.eq.0) then
         sky = 0
         ampl = 0
         amperr = 0
         err = 0
         return
      end if
      tmp = am(2)
      am(2) = am(1) / det
      am(1) = tmp / det
      am(3) = -am(3) / det

      sky = am(1)*v(1) + am(3)*v(2)
      ampl = am(3)*v(1) + am(2)*v(2)

      if(ampl.lt.0) ampl = 0

      sum = 0
      sum2 = 0
      do 30 j = 1,n
         sum = sum + profile(j)-(sky + ampl*smooth(j))
         sum2 = sum2 + (profile(j)-(sky + ampl*smooth(j)))**2
 30   continue

      sum = sum / n
      rms = dsqrt(dabs(sum2/n-sum*sum))

      if(ampl.eq.0) sky = sky + sum

      err = sqrt(am(1)) * rms
      amperr = sqrt(am(2)) * rms
      cov = am(3) / sqrt(am(1)*am(2))

      return
      end


      SUBROUTINE QSORT4(N,X)
* Sorting program that uses a quicksort algorithm
* c. 1978 JT
      parameter (maxstack=256)
      REAL X(N)
      REAL KEY, KL, KR, KM, TEMP
      INTEGER L, R, M, LSTACK(maxstack+1), RSTACK(maxstack+1), SP
      INTEGER NSTOP
      LOGICAL MGTL, LGTR, RGTM
      DATA NSTOP /15/

      IF(N.LE.NSTOP) GOTO 100
      SP = 0
      SP = SP + 1
      LSTACK(SP) = 1
      RSTACK(SP) = N

* Sort a subrecord off the stack
* Set KEY = median of X(L), X(M), X(R)
1     L = LSTACK(SP)
      R = RSTACK(SP)
      SP = SP - 1
      M = (L + R) / 2
      KL = X(L)
      KM = X(M)
      KR = X(R)
      MGTL = KM .GT. KL
      RGTM = KR .GT. KM
      LGTR = KL .GT. KR
      IF(MGTL .EQV. RGTM) THEN
          IF(MGTL .EQV. LGTR) THEN
              KEY = KR
          ELSE
              KEY = KL
          ENDIF
      ELSE
          KEY = KM
      ENDIF

      I = L
      J = R

* Find a big record on the left
10    IF(X(I).GE.KEY) GOTO 11
      I = I + 1
      GOTO 10
11    CONTINUE
* Find a small record on the right
20    IF(X(J).LE.KEY) GOTO 21
      J = J - 1
      GOTO 20
21    CONTINUE
      IF(I.GE.J) GOTO 2
* Exchange records
      TEMP = X(I)
      X(I) = X(J)
      X(J) = TEMP
      I = I + 1
      J = J - 1
      GOTO 10

* Subfile is partitioned into two halves, left .le. right
* Push the two halves on the stack
2     IF(J-L+1 .GT. NSTOP) THEN
          SP = SP + 1
          LSTACK(SP) = L
          RSTACK(SP) = J
      ENDIF
      IF(R-J .GT. NSTOP) THEN
          SP = SP + 1
          LSTACK(SP) = J+1
          RSTACK(SP) = R
      ENDIF
      IF(SP.GT.MAXSTACK) THEN
C         WRITE(6,*) 'QSORT4: Fatal error from stack overflow'
C         WRITE(6,*) 'Fall back on sort by insertion'
*         call f77msg('QSORT4: Fatal error from stack overflow')
*         call f77msg('Fall back on sort by insertion')
         GOTO 100
      END IF

* Anything left to process?
      IF(SP.GT.0) GOTO 1

* Sorting routine that sorts the N elements of single precision
* array X by straight insertion between previously sorted numbers
100   DO 110 J = N-1,1,-1
      K = J
      DO 120 I = J+1,N
      IF(X(J).LE.X(I)) GOTO 121
120   K = I
121   CONTINUE
      IF(K.EQ.J) GOTO 110
      TEMP = X(J)
      DO 130 I = J+1,K
130   X(I-1) = X(I)
      X(K) = TEMP
110   CONTINUE
      RETURN
      END

      subroutine waussfit(data,npar,par,aux,niter,cov,chisq)
*     Program to fit a source with a Wingy Gaussian
*
*     Input:	data	Pixel array
*     NPAR      How many parameters to fit: 4, 7, 9
*               Note: NPAR=9 wants parameters to be first fitted with NPAR=7!
*     NITER     Max number of iterations requested
*     PAR(1)    Initial guess for x0 (includes XOFF)
*     PAR(2)    Initial guess for y0 (includes YOFF)
*     PAR(8)    BETA4: r^4 coeff of Waussian (< 0 => Fit Gaussian instead)
*     PAR(9)    BETA6: r^6 coeff of Waussian
*
*     AUX(1)    NX	Number of columns in DATA
*     AUX(2)    NY	Number of rows in DATA
*     AUX(3)    NXDIM   Column dimension of DATA
*     AUX(4)    XOFF	X offset of subarray in entire array
*     AUX(5)    YOFF	Y offset of subarray in entire array
*     AUX(6)    EPERADU	E/ADU for calculating sigma
*     AUX(7)    EXTRA	Added value so that variance = E/ADU*(DATA+EXTRA)
*     AUX(8)    IGNORE	Pixel value to ignore in fitting
*     AUX(9)    INIT	Initialize params?
*                       0, start with what's in PAR already
*                       1, initialize all but x0,y0 = PAR(1,2)
*                       2, also find peak to start x0,y0 = PAR(1,2)
*                       
*      Output:	PAR(1:12)   (8,9 left alone if NPAR <= 7)
*  1   XC	x center of Gaussian; first pixel is 0 < x < 1
*  2   YC	y center of Gaussian; first pixel is 0 < y < 1
*  3   PEAK	Central intensity of Gaussian
*  4   SKY	Constant added to Gaussian
*  5   SX2	Squared Gaussian width in the x direction
*  6   SXY	Cross term of Gaussian width
*  7   SY2	Squared Gaussian width in the y direction
*  8   BETA4	r^4 coefficient in Waussian (< 0 => Fit Gaussian instead)
*  9   BETA6	r^6 coefficient in Waussian
* 10   FWMAJ	Gaussian FWHM width - major axis
* 11   FWMIN	Gaussian FWHM width - minor axis
* 12   PHI      Angle of major axis
*      NITER 	Number of iterations carried out
*
      parameter (pi=3.14159265, gausshalf=2.3548)
      real*8 cov(npar,npar)
      real par(12), aux(9)
      real data(1)
      character*1024 blabline
      external weval

      IBLAB = 0

      nx = nint(aux(1))
      ny = nint(aux(2))
      nxdim = nint(aux(3))
      init = nint(aux(9))

* Set up initial values for parameters...
      if(init.gt.1) then
* Find the peak for INIT .GE. 2
         imax = 1
         jmax = 1
         do 10 j = 1,ny
            do 11 i = 1,nx
               if(data(i+(j-1)*nxdim).gt.data(imax+(jmax-1)*nxdim)) then
                  imax = i
                  jmax = j
               end if
 11         continue
 10      continue
         par(1) = imax - 0.5 + aux(4)
         par(2) = jmax - 0.5 + aux(5)
      end if

* Internal to waussfit the position is relative to the subarray...
      par(1) = par(1) - aux(4)
      par(2) = par(2) - aux(5)

* Continue initialization for INIT .GE. 1
      if(init.gt.0) then
         imax = nint(par(1) + 0.5)
         jmax = nint(par(2) + 0.5)

         par(4) = amin1(data(1), data(1+(ny-1)*nxdim),
     $        data(nx), data(nx+(ny-1)*nxdim))
         par(3) = data(imax+(jmax-1)*nxdim) - par(4)
         half = 0.5*(par(3)-par(4)) + par(4)
         if(npar.gt.4) then
            do 12 i = imax+1,nx
               if(data(i+(jmax-1)*nxdim).lt.half) then
                  width = i - imax
                  goto 13
               end if
 12         continue
            width = nx / 2
 13         continue
            par(5) = 1 / (width/1.2)**2
            par(6) = 0.001
            par(7) = par(5)
         end if
         if(npar.gt.7) then
            par(8) = 1
            par(9) = 1
         end if
      end if

      acc = 0.001

      alamb = -1
      if(iblab.gt.0) then
C         WRITE(6,*) 'ATTEMPTING FITMRQ', aux(6), aux(7)
*         WRITE(blabline,*) 'ATTEMPTING FITMRQ', aux(6), aux(7)
*         call f77msg(blabline)
      end if

      nvar = 9

      if(iblab.gt.0) then
C         WRITE(6,1511) -1,-20,0.0,(PAR(J),J=1,7)
*         WRITE(blabline,1511) -1,-20,0.0,(PAR(J),J=1,7)
*         call f77msg(blabline)
      end if
      chiold = fitmrq(nx*ny,aux,v,data,nvar,npar,par,cov,alamb,weval)
      if(chiold .lt. 0) return
      if(iblab.gt.0) then
C         WRITE(6,1511) 0,nint(alog10(alamb)),CHIOLD,(PAR(J),J=1,7)
*         WRITE(blabline,1511) 0, nint(alog10(alamb)), CHIOLD,
*     $        (PAR(J),J=1,7)
* 1511    FORMAT(2I4,1pe12.4,0p2F9.3,2F9.1,5F9.4)
*         call f77msg(blabline)
      end if
      miter = 0
      do 20 i = 1,niter
         chisq = fitmrq(nx*ny,aux,v,data,nvar,npar,par,cov,alamb,weval)
         if(chisq .lt. 0) return
         miter = miter + 1
         if(iblab.gt.0) then
C            WRITE(6,1511) MITER,nint(alog10(alamb)),CHISQ,(PAR(J),J=1,7)
*            WRITE(blabline,1511) MITER, nint(alog10(alamb)), CHISQ,
*     $           (PAR(J),J=1,7)
*            call f77msg(blabline)
         end if
         if(abs(chisq-chiold) .lt. acc*chiold .and.
     $        alamb.le.0.001) goto 21
         chiold = amin1(chiold,chisq)
 20   continue
 21   alamb = 0
      chisq = fitmrq(nx*ny,aux,v,data,nvar,npar,par,cov,alamb,weval)
      if(chisq .lt. 0) return
      if(iblab.gt.0) then
C         WRITE(6,1511) miter,-20,CHISQ,(PAR(J),J=1,7)
*         WRITE(blabline,1511) miter,-20,CHISQ,(PAR(J),J=1,7)
*         call f77msg(blabline)
      end if
      niter = miter

* Patch up and fill in the parameters
      par(1) = par(1) + aux(4)
      par(2) = par(2) + aux(5)

* Calculate sigma's and position angles from sx2, sxy, sy2
      call domajmin(par)

C      WRITE(6,*) PAR

      return
      end

      subroutine wausstwo(data,par,aux,chisq)
C Fit a Waussian using only PEAK and SKY; he's linear, Jim.
      real par(12), aux(9)
      real data(1)
      real*8 v(2), am(3), det, dat, fit

      nx = nint(aux(1))
      ny = nint(aux(2))
      nxdim = nint(aux(3))
* Internal to waussfit the position is relative to the subarray...
      x0 = par(1) - aux(4)
      y0 = par(2) - aux(5)
      sx2 = par(5)
      sxy = par(6)
      sy2 = par(7)
      b4 = par(8) / 2
      b6 = par(9) / 6

      v(1) = 0
      v(2) = 0
      am(1) = 0
      am(2) = 0
      am(3) = 0

      baddata = aux(8)

C Accumulate sums
      do 10 j = 0,ny-1
         do 11 i = 0,nx-1
            x = i + 0.5
            y = j + 0.5
            if(data(i+1+j*nxdim) .eq. baddata) goto 11
            z2 = sx2*(x-x0)*(x-x0)+sxy*(x-x0)*(y-y0)+sy2*(y-y0)*(y-y0)
            fit = 1 / (1+z2*(1+z2*(b4+z2*b6)))
            dat = data(i+1+j*nxdim)
            v(1) = v(1) + dat
            v(2) = v(2) + dat*fit
            am(1) = am(1) + 1
            am(2) = am(2) + fit*fit
            am(3) = am(3) + fit
 11      continue
 10   continue

      det = am(1)*am(2) - am(3)*am(3)
      if(det.eq.0) then
         par(3) = 0
         par(4) = 0
         chisq = -2
         return
      end if
      tmp = am(2)
      am(2) = am(1) / det
      am(1) = tmp / det
      am(3) = -am(3) / det

      par(3) = am(3)*v(1) + am(2)*v(2)
      par(4) = am(1)*v(1) + am(3)*v(2)

      call domajmin(par)
      chisq = 1
      return
      end


      subroutine domajmin(par)
* Calculate sigma's and position angles from sx2, sxy, sy2
      parameter (pi=3.14159265, gausshalf=2.3548)
      real par(12), aux(9)

      if(par(5).eq.par(7)) then
         par(12) = pi/4
         par(10) = par(5) + par(7)
         par(11) = par(10)
      else
         par(12) = 0.5*atan(par(6)/(par(5)-par(7)))
         par(10) = par(5) + par(7) + (par(5)-par(7))/cos(2*par(12))
         par(11) = par(5) + par(7) - (par(5)-par(7))/cos(2*par(12))
         if(par(10).gt.par(11)) then
            tmp = par(10)
            par(10) = par(11)
            par(11) = tmp
            par(12) = par(12) - pi/2
         end if
      end if
      if(par(12).lt.0) par(12) = par(12) + pi

* Take careful square roots
      if(par(10).gt.0) then
         par(10) = gausshalf / sqrt(par(10))
      else if(par(10).eq.0) then
         par(10) = 0
      else
         par(10) = -gausshalf / sqrt(-par(10))
      end if

      if(par(11).gt.0) then
         par(11) = gausshalf / sqrt(par(11))
      else if(par(11).eq.0) then
         par(11) = 0
      else
         par(11) = -gausshalf / sqrt(-par(11))
      end if
      return
      end

      function weval(k,aux,v,data,ydat,wgt, npar,par,yfit,dyda)
* Evaluate things for fitmrq
      parameter (half=0.5, sixth=0.1666666666)
      real data(1)
      real dyda(npar), par(9), aux(8)
      character*1024 blabline

C      WRITE(6,*) 'ENTERED WEVAL', 
C      WRITE(6,*) PAR
C      WRITE(6,*) AUX
      x0 = par(1)
      y0 = par(2)
      peak = par(3)
      sky = par(4)
      sx2 = par(5)
      sxy = par(6)
      sy2 = par(7)
      b4 = par(8)
      b6 = par(9)

      nx = nint(aux(1))
      ny = nint(aux(2))
      nxdim = nint(aux(3))
      eperadu = aux(6)
      extrasky = aux(7)
      baddata = aux(8)

*      WRITE(blabline, *) 'Entering weval', (par(i),i=1,9),(aux(i),i=1,8)
*      call f77msg(blabline)

      if(nx.eq.0) then
C         write(6,*) 'WEVAL: zero dimension???'
         call f77msg('WEVAL: zero dimension???')
         weval = -1
         return
      end if
      i = mod(k-1,nx)
      j = (k-1)/nx

C      WEVAL = DATA(NXDIM*J + I + 1)
C      WRITE(6,*) 'WEVAL COORDS', I, J

      x = i + 0.5
      y = j + 0.5

      z2 = sx2*(x-x0)*(x-x0) + sxy*(x-x0)*(y-y0) + sy2*(y-y0)*(y-y0)
      fn = 0
      if(z2.lt.85) then
         if(b4.lt.0 .and. npar.le.7) then
            fn = exp(-z2)
            dfdz2 = 1/fn
         else
            fn = 1/(1 + z2*(1 + z2*(half*b4 + z2*sixth*b6)))
            dfdz2 = 1 + z2*(b4 + z2*half*b6)
         end if
      end if

      yfit = sky + peak*fn

*      WRITE(blabline, *) 'weval:', sx2,sxy,sy2, z2, fn, sky, peak, yfit
*      call f77msg(blabline)

      dyda(1) = peak*fn*fn*dfdz2*(2*sx2*(x-x0) + sxy*(y-y0))
      dyda(2) = peak*fn*fn*dfdz2*(sxy*(x-x0) + 2*sy2*(y-y0))
      dyda(3) = fn
      dyda(4) = 1
      if(npar.gt.4) then
         dyda(5) = -peak*fn*fn*dfdz2*(x-x0)*(x-x0)
         dyda(6) = -peak*fn*fn*dfdz2*(x-x0)*(y-y0)
         dyda(7) = -peak*fn*fn*dfdz2*(y-y0)*(y-y0)
      end if
      if(npar.gt.7) then
         dyda(8) = -peak*fn*fn*half*z2*z2
         dyda(9) = -peak*fn*fn*sixth*z2*z2*z2
      end if

      weval = 1
      ydat = data(nxdim*j + i + 1)
      if(ydat.eq.baddata) then
         ydat = yfit
         weval = 0
      end if

      if(extrasky.gt.-9e9) then
         wgt = eperadu/(ydat+extrasky)
         if(wgt.lt.0) wgt = 1
      else
         wgt = 1
      end if

*      WRITE(blabline, *) 'Leaving weval', weval, yfit, wgt, 
*     $     (par(i),i=1,9), (dyda(i),i=1,9)
*      call f77msg(blabline)

      return
      end

      function fitmrq(ndata,u,v,y,nvar,npar,par,cov,alamb,func)
* Levenberg-Marquardt method, fitting PAR(NPAR) to data Y(NDATA)
* In order to provide as much flexibility as possible, the arrays
* (U,V are convenience arrays) are all passed through FUNC:
*
*     FUNC(I,U,V,Y,YDAT,WGT, NPAR,PAR,YFIT,DYDA) 
*
*        should return the 0/1 for data value used as well as filling in
*        YDAT, YFIT, DYDA(NPAR), and WGT.
*
* Note that NVAR parameters are maintained, but only NPAR are varied.
*
* ALAMB < 0 => initialize; otherwise usual LM parameter; 
* COV = scratch space (returns slightly messed up covariance matrix)
* A final call with ALAMB = 0 will fill COV with proper covariance matrix
* FITMRQ returns Chi^2/NDOF
*
      parameter (maxpar=20)
      real*8 cov(npar,npar)
      real par(nvar), ptry(maxpar)
      real*8 alpha(maxpar*maxpar), beta(maxpar), dpar(maxpar)
      external func
      character*1024 blabline

* Initialize with first value of Chi^2
      if(alamb .lt. 0) then
         alamb = 0.001
         chisq = chimrq(ndata,u,v,y,npar,par,alpha,beta,func)
         ochisq = chisq
         fitmrq = chisq
         do 13 j=1,nvar
            ptry(j) = par(j)
 13      continue
         return
      end if
*      write(blabline, *) 'frmq1: ', chisq, (par(i),i=1,npar)
*      call f77msg(blabline)

* Build L-M matrix
      do 15 j=1,npar
        do 14 k=1,npar
          cov(j,k) = alpha(k+(j-1)*npar)
14      continue
        cov(j,j) = alpha(j+(j-1)*npar)*(1.+alamb)
        dpar(j) = beta(j)
15    continue

* Solve for new DPAR
      ierr = jordangauss(cov,npar,npar,dpar,1,1)
      if(ierr .ne. 0) then
         fitmrq = -1
         return
      end if

* ALAMB = 0 implies COV is now covariance matrix, so just return
      if(alamb .eq. 0.0) then
        fitmrq = chisq
        return
      end if

* Evaluate new Chi^2
      do 16 j=1,npar
         ptry(j) = par(j) + dpar(j)
16    continue

C      WRITE(6,*) 'ABOUT TO TRY NEW DATA'
C      WRITE(6,*) PTRY
C      WRITE(6,*) DPAR

      chisq = chimrq(ndata,u,v,y,npar,ptry,cov,dpar,func)

*      write(blabline, *) 'frmq2: ', chisq, (ptry(i),i=1,npar)
*      call f77msg(blabline)

* Test for improvement and adjust L-M parameter
      if(chisq.lt.ochisq)then
         alamb = 0.1*alamb
         ochisq = chisq
         do 18 j=1,npar
            do 17 k=1,npar
               alpha(k+(j-1)*npar) = cov(j,k)
 17         continue
            beta(j) = dpar(j)
            par(j) = ptry(j)
 18      continue
      else
         alamb = 10.*alamb
         chisq = ochisq
      end if
      fitmrq = chisq
      return
      end

      function chimrq(ndata,u,v,y,npar,par,alpha,beta,func)
      parameter (maxpar=20)
      real*8 alpha(npar,npar), beta(npar)
      real dyda(maxpar)
      external func
      character*1024 blabline

* Zero out second derivative matrix and y vector
      do 12 j=1,npar
         do 11 k=1,j
            alpha(j,k) = 0.0
 11      continue
         beta(j) = 0.0
 12   continue
      chimrq=0.0

* Add up sums 
      dof = -npar
      do 15 i = 1,ndata
         used = func(i,u,v,y,ydata,wgt,npar,par,ymod,dyda)
*         write(blabline, *) i, ymod, used
*         call f77msg(blabline)
         dof = dof + used
         dy = ydata - ymod
         do 14 j=1,npar
            wt = dyda(j) * wgt
            do 13 k=1,j
               alpha(j,k) = alpha(j,k) + wt*dyda(k)
 13         continue
            beta(j) = beta(j) + dy*wt
 14      continue
         chimrq = chimrq + dy*dy*wgt
 15   continue
* Convert to Chi^2/N
      chimrq = chimrq / amax1(1.0, dof)

* Symmetrize
      do 17 j=2,npar
         do 16 k=1,j-1
            alpha(k,j) = alpha(j,k)
 16      continue
 17   continue

      return
      end

      function jordangauss(a,n,np,b,m,mp)
      implicit real*8 (a-h,o-z)
      parameter (nmax=50)
      real*8 a(np,np), b(np,mp)
      integer ipiv(nmax), indxr(nmax), indxc(nmax)
      do 11 j=1,n
         ipiv(j)=0
 11   continue
      do 22 i=1,n
         big=0.
         do 13 j=1,n
            if(ipiv(j).ne.1)then
               do 12 k=1,n
                  if (ipiv(k).eq.0) then
                     if (abs(a(j,k)).ge.big)then
                        big=abs(a(j,k))
                        irow=j
                        icol=k
                     endif
                  else if (ipiv(k).gt.1) then
C                     write(6,*) 'singular matrix'
                     jordangauss = -1
                     return
                  endif
 12            continue
            endif
 13      continue
         ipiv(icol)=ipiv(icol)+1
         if (irow.ne.icol) then
            do 14 l=1,n
               dum=a(irow,l)
               a(irow,l)=a(icol,l)
               a(icol,l)=dum
 14         continue
            do 15 l=1,m
               dum=b(irow,l)
               b(irow,l)=b(icol,l)
               b(icol,l)=dum
 15         continue
         endif
         indxr(i)=irow
         indxc(i)=icol
         if (a(icol,icol).eq.0.) then
C            write(6,*) 'singular matrix.'
            jordangauss = -1
            return
         end if
         pivinv=1./a(icol,icol)
         a(icol,icol)=1.
         do 16 l=1,n
            a(icol,l)=a(icol,l)*pivinv
 16      continue
         do 17 l=1,m
            b(icol,l)=b(icol,l)*pivinv
 17      continue
         do 21 ll=1,n
            if(ll.ne.icol)then
               dum=a(ll,icol)
               a(ll,icol)=0.
               do 18 l=1,n
                  a(ll,l)=a(ll,l)-a(icol,l)*dum
 18            continue
               do 19 l=1,m
                  b(ll,l)=b(ll,l)-b(icol,l)*dum
 19            continue
            endif
 21      continue
 22   continue
      do 24 l=n,1,-1
         if(indxr(l).ne.indxc(l))then
            do 23 k=1,n
               dum=a(k,indxr(l))
               a(k,indxr(l))=a(k,indxc(l))
               a(k,indxc(l))=dum
 23         continue
         endif
 24   continue
      jordangauss = 0
      return
      end
