
# check various layouts to see if we get things right in SetGraphSize and SetImageSize

macro init
 dev -n 0
 close -n 0
 dev -n 0
end

macro check1
 if ($0 != 2)
   echo "USAGE: check1 (dir)"
   break
 end

 mcreate a 100 100 

 resize 900 900
 section default 0.2 0.2 0.6 0.6 -bg red
 section default -imtool $1

 box -labels 1111
 tv a -10 20
 label -x foobar -y foo +x bar +y baz
end

macro check2
 if ($0 != 2)
   echo "USAGE: check1 (dir)"
   break
 end

 mcreate a 100 100 

 resize 900 900
 section default 0.2 0.2 0.6 0.6 -bg red
 section default -imtool $1

 # box -labels 1111
 tv a -10 20
 # label -x foobar -y foo +x bar +y baz
end
