      SUBROUTINE locate(xx,n,x,j)
      INTEGER j,n
      REAL x,xx(n)
      INTEGER jl,jm,ju
      jl=0
      ju=n+1
10    if(ju-jl.gt.1)then
        jm=(ju+jl)/2
        if((xx(n).gt.xx(1)).eqv.(x.gt.xx(jm)))then
          jl=jm
        else
          ju=jm
        endif
      goto 10
      endif
      j=jl
      return
      END

      FUNCTION julday(mm,id,iyyy)
      INTEGER julday,id,iyyy,mm,IGREG
      PARAMETER (IGREG=15+31*(10+12*1582))
      INTEGER ja,jm,jy
      jy=iyyy
      if (jy.eq.0) pause 'julday: there is no year zero'
      if (jy.lt.0) jy=jy+1
      if (mm.gt.2) then
        jm=mm+1
      else
        jy=jy-1
        jm=mm+13
      endif
      julday=int(365.25*jy)+int(30.6001*jm)+id+1720995
      if (id+31*(mm+12*iyyy).ge.IGREG) then
        ja=int(0.01*jy)
        julday=julday+2-ja+int(0.25*ja)
      endif
      return
      END

function mjd(yr,hr,sc)
	implicit none
	integer iyr, mn, n, jd, j, julday, idy
	real*8 yr,hr,sc,mjd
	real*4 month(12), dy
	n = 12
	data month/0,31,59,90,120,151,181,212,243,273,304,334/
	iyr = yr
	if((mod(iyr,4).eq.0.and.mod(iyr,100).ne.0).or.(mod(iyr,400).eq.0)) then
	  dy = int(366.*(yr-iyr)+.0001) + 1
	  do j = 3, 12
	    month(j) = month(j) + 1.
	  end do
	else
	  dy = int(365.*(yr-iyr)+.0001) + 1
	end if
c 
c locate, from Numrec library
c
	call locate(month,n,dy,mn)
	dy = dy - month(mn)
	idy = dy
c
c julday, from Numrec library
c
	jd = julday(mn,idy,iyr)
	mjd = jd - 2440000.5
	mjd = mjd + hr / 24. + sc / 86400. / 2.
	end


z
