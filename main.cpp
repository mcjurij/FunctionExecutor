// interactive program to try out FunctionParser

#include <vector>
#include <iostream>
#include <string>

#include "FunctionParser.h"

using namespace std;


// helpers for main -------------------------------------------------------------

struct var_helper {
    string name;
    double value;
    double start, stop, step;
};


bool nextVal( vector<var_helper> &vars )
{
    size_t k = vars.size()-1;
    
    while( k>=0 )
    {
        var_helper &var = vars[k];
        if( var.value < var.stop )
        {
            var.value += var.step;
            return true;
        }
        else if( k > 0 )
        {
            var.value = var.start;
            k--;
        }
        else
            return false;
    }
    
    return false;
}


void loopThrough( FunctionParser &parser, vector<var_helper> &vars)
{
    while( nextVal( vars ) )
    {        
        for( size_t j = 0; j < vars.size(); j++)
        {
            var_helper &v = vars[j];
            cout << v.name << " = " << v.value;
            if( j < vars.size()-1 )
                cout << ", ";
            else
                cout << "    ";
        }
        cout << "result :   " << parser.execute() << "\n";
    }
}


int main( int argc, char *argv[])
{
    //FunctionParser parser( " 5*5*5*5*5*5*5*5*5 " );
    //FunctionParser parser( " 1+sin(x)*cos(y)+2*sqrt(x+y)+ sin(x)*cos(y)+2*sqrt(x+y)" );
    
    string func;
    cout << "function > ";
    getline( cin, func);
    FunctionParser parser( func );

    parser.addConstant( "pi", M_PI);    // add constants, if you don't they will be treated as variables
    
    parser.parse();

    vector<var_helper> vars;
    vector<string> var_names = parser.getVariables();

    if( var_names.size() > 0 )
    {
        for( size_t i = 0; i < var_names.size(); i++)
        {
            var_helper var;
            var.name = var_names[i];
            
            string h;
            cout << "variable " << var.name << "  start > ";
            getline( cin, h);
            var.start = atof( h.c_str() );  // doing it the old school way for now, C++11 has stod
            
            cout << "variable " << var.name << "  stop  > ";
            getline( cin, h);
            var.stop  = atof( h.c_str() );
            
            cout << "variable " << var.name << "  step  > ";
            getline( cin, h);
            var.step = atof( h.c_str() );
            
            var.value = var.start;
            vars.push_back( var );
        }

        vars.back().value -= vars.back().step;  // a tiny bit of trickery needed for nextVal()
        
        for( size_t i = 0; i < vars.size(); i++)
        {
            var_helper &var = vars[i];
            parser.bindVariable( var.name, &(var.value));
        }
        
        loopThrough( parser, vars);
    }
    else   // no variables found
    {
        cout << "result :   " << parser.execute() << "\n";
    }

    return 0;
}
