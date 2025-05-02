
macro go
  mcreate hdr 0 0
  mcreate zdr 100 100
  set xdr = xramp(zdr)
  set ydr = yramp(zdr)

  keyword xdr EXTNAME -w XDR
  keyword ydr EXTNAME -w YDR

  wd hdr test.fits
  exec ftable -list test.fits

  wd -extend xdr test.fits
  exec ftable -list test.fits

  wd -extend ydr test.fits
  exec ftable -list test.fits
end

macro t2
  mcreate hdr 0 0
  mcreate zdr 100 100
  set xdr = xramp(zdr)
  set ydr = yramp(zdr)

  keyword xdr EXTNAME -w XDR
  keyword ydr EXTNAME -w YDR

  wd hdr test.fits
  exec ftable -list test.fits

  wd -extend xdr test.fits -compress
  exec ftable -list test.fits

  wd -extend ydr test.fits -compress
  exec ftable -list test.fits
end

macro t3
  mcreate hdr 0 0
  mcreate zdr 100 100
  set xdr = xramp(zdr)
  set ydr = yramp(zdr)

  keyword xdr EXTNAME -w XDR
  keyword ydr EXTNAME -w YDR

  wd hdr test.fits
  exec ftable -list test.fits

  wd -extend xdr test.fits -compress
  exec ftable -list test.fits

  wd -extend ydr test.fits -compress
  exec ftable -list test.fits

  wd -extend xdr test.fits
  exec ftable -list test.fits

  wd -extend ydr test.fits
  exec ftable -list test.fits
end
