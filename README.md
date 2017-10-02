# FunctionExecutor
A function given as a string is parsed and intermediate code is executed

Example:
```
FunctionParser parser( "sin(pi*x)" );

parser.addConstant( "pi", M_PI);    // add constants, if you don't they will be treated as variables

parser.parse();     // parse once

double x;
parser.bindVariable( "x", &x);

for( x = 0.; x < 4.; x+=0.2)
{
    cout << "result : " << parser.execute() << "\n";    // "sin(pi*x)" not parsed again, but executed
}
```

Any function with any number of variables is allowed. Variable names will be
detected automagically, so you can name them as you like. See main() in
FunctionParser.cpp with interactive intput of a function string and input of
values for start, stop and step for every variable detected.
