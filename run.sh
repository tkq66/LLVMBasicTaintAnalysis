if  [ $1 = "1" ]; then
    bin/TaintPass resources/example1.ll
elif [ $1 = "2" ]; then
    bin/TaintPass resources/example2.ll
elif [ $1 = "3" ]; then
    bin/TaintPass resources/example3.ll
else
    echo "Please enter the option 1, 2, or 3 for the example file you would like to run taint analysis on."
fi
