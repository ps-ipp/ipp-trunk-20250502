
macro fakeimage-tan

  mcreate test 1000 1000

  keyword test CTYPE1  -w  RA---TAN               ; keyword test CTYPE1  -wc "Algorithm type for axis 1"
  keyword test CTYPE2  -w  DEC--TAN               ; keyword test CTYPE2  -wc "Algorithm type for axis 2"
  keyword test CRPIX1  -wf     2997.03245628555   ; keyword test CRPIX1  -wc "Dither offset Y"
  keyword test CRPIX2  -wf    -944.140242055343   ; keyword test CRPIX2  -wc "Dither offset Y"
  keyword test CRVAL1  -wf     207.605373039722   ; keyword test CRVAL1  -wc "[deg] Right ascension at the reference pixel"
  keyword test CRVAL2  -wf    0.113109076969833   ; keyword test CRVAL2  -wc "[deg] Declination at the reference pixel"
  keyword test CRUNIT1 -w  deg                    ; keyword test CRUNIT1 -wc "Unit of right ascension co-ordinates"
  keyword test CRUNIT2 -w  deg                    ; keyword test CRUNIT2 -wc "Unit of declination co-ordinates"
  keyword test CD1_1   -wf -3.34355087531346E-07  ; keyword test CD1_1   -wc "Transformation matrix element"
  keyword test CD1_2   -wf -0.000111484161257407  ; keyword test CD1_2   -wc "Transformation matrix element"
  keyword test CD2_1   -wf  0.000111383424202905  ; keyword test CD2_1   -wc "Transformation matrix element"
  keyword test CD2_2   -wf -3.65044007533155E-07  ; keyword test CD2_2   -wc "Transformation matrix element"
  # keyword test PV2_1   -wf         1.000000E+00   ; keyword test -c Pol.coeff. for pixel -> celestial coord       
  # keyword test PV2_2   -wf         0.000000E+00   ; keyword test -c Pol.coeff. for pixel -> celestial coord       
  # keyword test PV2_3   -wf                 -50.                                                 

end

macro fakeimage-zpn1

  mcreate test 1000 1000

  keyword test CTYPE1  -w  RA---ZPN               ; keyword test CTYPE1  -wc "Algorithm type for axis 1"
  keyword test CTYPE2  -w  DEC--ZPN               ; keyword test CTYPE2  -wc "Algorithm type for axis 2"
  keyword test CRPIX1  -wf     2997.03245628555   ; keyword test CRPIX1  -wc "Dither offset Y"
  keyword test CRPIX2  -wf    -944.140242055343   ; keyword test CRPIX2  -wc "Dither offset Y"
  keyword test CRVAL1  -wf     207.605373039722   ; keyword test CRVAL1  -wc "[deg] Right ascension at the reference pixel"
  keyword test CRVAL2  -wf    0.113109076969833   ; keyword test CRVAL2  -wc "[deg] Declination at the reference pixel"
  keyword test CRUNIT1 -w  deg                    ; keyword test CRUNIT1 -wc "Unit of right ascension co-ordinates"
  keyword test CRUNIT2 -w  deg                    ; keyword test CRUNIT2 -wc "Unit of declination co-ordinates"
  keyword test CD1_1   -wf -3.34355087531346E-07  ; keyword test CD1_1   -wc "Transformation matrix element"
  keyword test CD1_2   -wf -0.000111484161257407  ; keyword test CD1_2   -wc "Transformation matrix element"
  keyword test CD2_1   -wf  0.000111383424202905  ; keyword test CD2_1   -wc "Transformation matrix element"
  keyword test CD2_2   -wf -3.65044007533155E-07  ; keyword test CD2_2   -wc "Transformation matrix element"
  keyword test PV2_1   -wf         1.000000E+00   ; keyword test PV2_1   -wc "Pol.coeff. for pixel -> celestial coord"
  keyword test PV2_2   -wf         0.000000E+00   ; keyword test PV2_2   -wc "Pol.coeff. for pixel -> celestial coord"
  keyword test PV2_3   -wf                 -50. 

end

macro test.coords
  for ix 50 5000 500
    for iy 50 5000 500
      coords test -p $ix $iy
      coords test -c $RA $DEC
      if (abs($ix - $Xc) > 0.01) 
	echo bad
        break
      end
      if (abs($iy - $Yc) > 0.01) 
	echo bad
        break
      end
    end
  end
end

