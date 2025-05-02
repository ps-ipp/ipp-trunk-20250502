## site.mhpcc.pro : example configuration script for the MHPCC IPP cluster : -*- sh -*-

## a site configuration needs to define the following pieces of information:
## 1) the machines used for processing
## 2) the default_host and workdir_template variables
## 3) the ipphost table

macro init.cluster.po
  $PARALLEL = 1
  controller exit true

  # po05 -- using for pantasks 
  # po23 -- broken perl

  controller host add po02
  controller host add po03
  controller host add po04
# controller host add po05
  controller host add po06
  controller host add po07
# controller host add po08
# controller host add po09
# controller host add po10
# controller host add po11
# controller host add po12
# controller host add po13
# controller host add po14
# controller host add po15
# controller host add po16
# controller host add po17
# controller host add po18
# controller host add po19
# controller host add po20
# controller host add po21
# controller host add po22
# controller host add po23
# controller host add po24
end

macro init.cluster.sn
  $PARALLEL = 1
  controller exit true
  controller host add sn2
  controller host add sn3
  controller host add sn4
  controller host add sn5
end

macro init.site
  queueload tmp -x "cat $MODULES:0/ipphosts.manoa.config"
  ipptool2book tmp ipphosts -key camera
end
