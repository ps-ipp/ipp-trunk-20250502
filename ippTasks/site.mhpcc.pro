## site.mhpcc.pro : example configuration script for the MHPCC IPP cluster : -*- sh -*-

echo "do not use this configuration file (site.mhpcc.pro)"
break

## a site configuration needs to define the following pieces of information:
## 1) the machines used for processing
## 2) the default_host and workdir_template variables
## 3) the ipphost table

macro init.cluster.mhpcc
  $PARALLEL = 1
  controller exit true

#  controller host add ipp014
  controller host add ipp021
  controller host add ipp015
  controller host add ipp023
  controller host add ipp024
  controller host add ipp025
  controller host add ipp026
  controller host add ipp028
  controller host add ipp029
  controller host add ipp030
  controller host add ipp031
  controller host add ipp032
  controller host add ipp033
  controller host add ipp034
  controller host add ipp035
  controller host add ipp036
  controller host add ipp038
  controller host add ipp039
  controller host add ipp040
  controller host add ipp041
  controller host add ipp042
  controller host add ipp043
  controller host add ipp044
  controller host add ipp045
  controller host add ipp046
  controller host add ipp047
  controller host add ipp048
  controller host add ipp049
  controller host add ipp050
  controller host add ipp051
  controller host add ipp052
#  controller host add ipp053
end

## override the basic inits set in pantasks.pro
macro init.copy.mhpcc
 if ($0 != 2)
   echo "USAGE: init.copy.mhpcc (nebulous)"
   echo "nebulous may be 'on' or 'off'"
   break
 end

 if (("$1" != "on") && ("$1" != "off")) 
   echo "USAGE: init.copy.mhpcc (nebulous)"
   echo "nebulous may be 'on' or 'off'"
   break
 end
 
 # XXX this is only used by summit.copy.pro.  move this as a check into summit.copy.pro?
 $COMPRESS = 1

 # the templates are used if we have a class_id/host relationship; 
 # if none is found, the default values are used
 # XXX not sure how to handle the .N value if we need to use more than one

 if ("$1" == "on")
  $NEBULOUS = 1
  $default_host     = any
  $workdir_template = neb://@HOST@.0
 else
  $NEBULOUS = 0
  $default_host     = ipp023
  $workdir_template = /data/@HOST@.0
 end
end

macro init.site
  init.cluster.mhpcc
  init.copy.mhpcc on

  queueload tmp -x "cat $MODULES:0/ipphosts.mhpcc.config"
  ipptool2book tmp ipphosts -key camera

  # set autoprocessing for chip
  queueload tmp -x "cat $MODULES:0/surveys.mhpcc.config"
  ipptool2book tmp surveys -key survey
end

macro init.site.nohosts
  init.copy.mhpcc on

  queueload tmp -x "cat $MODULES:0/ipphosts.mhpcc.config"
  ipptool2book tmp ipphosts -key camera
end
