
macro test

  #$Ro = 185.382364
  #$Do =  34.697608
  #catdir /data/pikake.1/eugene/dvodist.20120514/dvodist/catdir.syn.test/
  $Gname = g_SYNTH
  $Rname = r_SYNTH

  $Ro = 2.59
  $Do = 1.23
  catdir catdir.merge
  $Gname = g
  $Rname = r

  skyregion {$Ro - 0.3} {$Ro + 0.3} {$Do - 0.3} {$Do + 0.3}
  avextract ra dec $Gname $Rname
  subset R = ra  if (ra > $Ro - 0.1) && (ra > $Ro + 0.1) && (dec > $Do - 0.1) && (dec < $Do + 0.1)
  subset D = dec if (ra > $Ro - 0.1) && (ra > $Ro + 0.1) && (dec > $Do - 0.1) && (dec < $Do + 0.1)
  subset g_ave = $Gname  if (ra > $Ro - 0.1) && (ra > $Ro + 0.1) && (dec > $Do - 0.1) && (dec < $Do + 0.1)
  subset r_ave = $Rname  if (ra > $Ro - 0.1) && (ra > $Ro + 0.1) && (dec > $Do - 0.1) && (dec < $Do + 0.1)
  vectors 

  # mmatch -v -parallel R D 1.0 RA DEC MAG PHOTCODE -index index
  mmatch -v -parallel R D 1.0 RA DEC MAG PHOTCODE externID mean_airmass -index index

  reindex g_ave_match = g_ave using index
  reindex r_ave_match = r_ave using index

  set dg = g_ave_match - MAG
  set dr = r_ave_match - MAG
  
  subset  g_match = MAG if (PHOTCODE == 3001)
  subset dg_match = dg if (PHOTCODE == 3001)

  subset dr_match = dr if (PHOTCODE == 3002)
  subset  r_match = MAG if (PHOTCODE == 3002)
end
