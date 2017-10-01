# FunctionExecutor
A function given as a string is parsed and intermediate code is executed

Example:
```
FunctionParser parser( "sin(x)" );
parser.parse();     // parse once

double x;
parser.bindVariable( "x", &x);

for( x = 0.; x < 4.; x+=0.2)
{
    parser.execute();          // "sin(x)" not parsed again, but executed
    cout << "result :   " << parser.getResult() << "\n";
}
```
