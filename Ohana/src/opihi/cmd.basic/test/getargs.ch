
macro t1
  getargs -myopt bool  -var tbool
  getargs -myval value -var tvalue

  # echo $0
  for i 1 $0
    echo -no-return "$$i "
  end
  echo "EOL"
end

macro g1
  t1;                               echo result: $tbool ..$tvalue..
  t1 -ignore;                       echo result: $tbool ..$tvalue..
  t1 -myopt;                        echo result: $tbool ..$tvalue..
  t1 -myval 5;                      echo result: $tbool ..$tvalue..
  t1 -myopt -myval 5;               echo result: $tbool ..$tvalue..
  t1 -myopt -myval 5 extra1 extra2; echo result: $tbool ..$tvalue..
end

macro t2
  getargs -myopt BOOL  -var tbool
  getargs -myval VALUE -var tvalue

  # echo $0
  for i 1 $0
    echo -no-return "$$i "
  end
  echo "EOL"
end

macro g2
  t2;                               echo result: $tbool ..$tvalue..
  t2 -ignore;                       echo result: $tbool ..$tvalue..
  t2 -myopt;                        echo result: $tbool ..$tvalue..
  t2 -myval 5;                      echo result: $tbool ..$tvalue..
  t2 -myopt -myval 5;               echo result: $tbool ..$tvalue..
  t2 -myopt -myval 5 extra1 extra2; echo result: $tbool ..$tvalue..
end
