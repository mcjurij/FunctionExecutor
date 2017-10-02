/*
 *
 * Parse a function string and execute the intermediate "code".
 *
 */
#include <cassert>
#include <iostream>
#include <stack>
#include <iomanip>
#include <cmath>
#include <list>

#include "FunctionParser.h"

using namespace std;

// intentionally not using cctype here -- locale sucks
#define is_digit(c) ((c)>='0' && (c)<='9')
#define is_alpha(c) (((c)>='a' && (c)<='z') || ((c)>='A' && (c)<='Z'))

#define is_white(c) ((c)==' ' || (c)=='\t' || (c)=='\n')

#define is_entity_beg(c)  (is_alpha(c))
#define is_entity_char(c) (is_alpha(c) || (c)=='_' || is_digit(c))

#define is_fpnum_beg(c) (is_digit(c) || (c)=='.')


const char *FunctionParser::token_type_to_str( token_type_t tt )
{
    switch( tt )
    {
    case T_INVALID:
        return "INVALID";
    case T_ISWHITE:
        return "is white";

    case T_LPAREN:
        return " ( ";
    case T_RPAREN:
        return " ) ";

    case T_COMMA:
        return " , ";
    
    case T_MINUS:
        return " MINUS ";
    case T_ADD:
        return "+";
    case T_MUL:
        return "*";
    case T_DIV:
        return "/";
    case T_POWER:
        return "^";
        
    case T_FPNUMBER:
        return "FP NUMBER";
    case T_INTNUMBER:
        return "INTNUMBER";
    case T_IDENT:
        return "IDENT";
        
    case T_ERROR:
        return "error";
    case T_EOF:
        return "EOF";
    default:
        return "? unknown token ?";
    }
    return "";
}



typedef stack<double, vector<double> > value_stack_t;

// base class for function binders
class FctPFunctions {

public:
    FctPFunctions() {};
    
    virtual ~FctPFunctions(){};
    
    virtual void f( value_stack_t & vs ) const = 0;
    virtual int getNumOfArgs() const = 0;
};


// function binder for one argument functions
class FctPFunctionsBind1 : public FctPFunctions {
private:
    FctPFunctionsBind1();

public:
    FctPFunctionsBind1( double (*f)(double) ) : fp(f)
    {}

    int getNumOfArgs() const
    {
        return 1;
    }
    
public:
    virtual void f(value_stack_t & vs) const;

    double      (*fp)(double);
};

void FctPFunctionsBind1::f(value_stack_t & vs) const
{
    assert( vs.size() > 0 );
    double v1 = vs.top();
    vs.pop();
    
    vs.push( fp( v1 ) );
}


// function binder for two argument functions
class FctPFunctionsBind2 : public FctPFunctions {
private:
    FctPFunctionsBind2();

public:
    FctPFunctionsBind2( double (*f)( double, double) ) : fp(f)
    {}
    
    int getNumOfArgs() const
    {
        return 2;
    }
    
public:
    virtual void f(value_stack_t & vs) const;

    double      (*fp)(double,double);
};

void FctPFunctionsBind2::f(value_stack_t & vs) const
{
    assert( vs.size() > 0 );
    double v2 = vs.top();
    vs.pop();
    assert( vs.size() > 0 );
    double v1 = vs.top();
    vs.pop();
    
    vs.push( fp( v1, v2) );
}


// variable binder
class FctPVariable {
private:
    FctPVariable();
    
public:
    FctPVariable( const string &n ) : name(n), val_addr(0) {}

    string getName() const
    { return name; }
    
    void bind( double *a )
    { val_addr = a; }

    double value() const
    { return *val_addr; }
    
private:
    string name;
    double *val_addr;
};


// parser helper class
class FunctionParserException {

    FunctionParserException();
public:
    FunctionParserException( const string & s ) : desc(s)
    {
    }

    string reason() const
    {
        return desc;
    }

private:
    string desc;
};


// instructions the executor understands
struct FunctionParserInstr {
    typedef enum { INVALID, PLUS, MINUS, MULT, DIV, POW, UNARY_MINUS,
                   FUNCTION, VARIABLE, CONSTANT} ins_type_t;
    
    ins_type_t ins_type;
    
    union {
        double        constant;
        FctPVariable  *var;
        FctPFunctions *func;
    } u;

    FunctionParserInstr():ins_type(INVALID) {}
    FunctionParserInstr( ins_type_t t ) :  ins_type(t) {}
    
    FunctionParserInstr( double c ) : ins_type(CONSTANT) { u.constant = c; }
    FunctionParserInstr( FctPVariable *v ) : ins_type(VARIABLE) { u.var = v; }
    FunctionParserInstr( FctPFunctions *f ) : ins_type(FUNCTION) { u.func = f; }
private:
};


// emit code and execute
class FunctionParserOperators {

    double powerTo( double b, double e )
    {
        return pow( b, e);
    }
        
    
public:
    FunctionParserOperators(): ins(0) {}

    ~FunctionParserOperators()
    { if( ins ) delete [] ins; }
    
private:
    void push( double v )
    {
        vstack.push(v);
    }


    double pop()
    {
        assert( vstack.size() > 0 );
        double v = vstack.top();
        vstack.pop();
        return v;
    }
    
public:
    void op( const FunctionParser::token_t& op_token );
    void unary_op( FunctionParser::token_type_t op_type );
    void function_op( FctPFunctions *func );
    void variable_op( FctPVariable *v );
    void constant_op( double constant );
    
    void printTop() const   // DEBUG
    {
        if( vstack.size() > 0 )
            cout << vstack.top() << "\n";
        else
            cerr << "no value on stack\n";
    }
    
public:
    void assembleInstructions();
    double executor();
    
private:
    value_stack_t vstack;

    list<FunctionParserInstr> tmp_inst_list;
    FunctionParserInstr *ins;
};


void FunctionParserOperators::assembleInstructions()
{
    delete [] ins;
    
    if( tmp_inst_list.size() == 0)
    {
        ins = 0;
        return;
    }
    
    ins = new FunctionParserInstr[ tmp_inst_list.size() ];
    
    list<FunctionParserInstr>::const_iterator it;
    int i=0;
    for( it = tmp_inst_list.begin(); it != tmp_inst_list.end(); ++it)
    {
        ins[i] = *it;
        i++;
    }
}


double FunctionParserOperators::executor()
{
    int i;
    double v1, v2;

    if( ins == 0 )
        return 0.0;

    while( ! vstack.empty() )
        vstack.pop();
    
    for( i=0; i<(int)tmp_inst_list.size(); i++)
        switch( ins[i].ins_type )
        {
            case FunctionParserInstr::INVALID:
                assert(0);
                break;
            case FunctionParserInstr::PLUS:
                v2 = pop();
                v1 = pop();
                vstack.push( v1 + v2 );
                break;
            case FunctionParserInstr::MINUS:
                v2 = pop();
                v1 = pop();
                vstack.push( v1 - v2 );
                break;
            case FunctionParserInstr::MULT:
                v2 = pop();
                v1 = pop();
                vstack.push( v1 * v2 );
                break;
            case FunctionParserInstr::DIV:
                v2 = pop();
                v1 = pop();
                vstack.push( v1 / v2 );
                break;
            case FunctionParserInstr::POW:
                v2 = pop();
                v1 = pop();
                vstack.push( powerTo( v1, v2) );
                break;
                
            case FunctionParserInstr::UNARY_MINUS:
                v1 = pop();
                vstack.push( v1 * -1.0 );
                break;

            case FunctionParserInstr::FUNCTION:
                ins[i].u.func->f( vstack );
                break;
            case FunctionParserInstr::VARIABLE:
                vstack.push( ins[i].u.var->value() );
                break;
            case FunctionParserInstr::CONSTANT:
                vstack.push( ins[i].u.constant );
                break;
        }
    
    return pop();
}


void FunctionParserOperators::op( const FunctionParser::token_t& op_token )
{
    switch( op_token.type )
    {
        case FunctionParser::T_ADD:
            tmp_inst_list.push_back( FunctionParserInstr::PLUS );
            break;
        case FunctionParser::T_MINUS:
            tmp_inst_list.push_back( FunctionParserInstr::MINUS );
            break;
        case FunctionParser::T_MUL:
            tmp_inst_list.push_back( FunctionParserInstr::MULT );
            break;
        case FunctionParser::T_DIV:
            tmp_inst_list.push_back( FunctionParserInstr::DIV );
            break;
        case FunctionParser::T_POWER:   // ^
            tmp_inst_list.push_back( FunctionParserInstr::POW );
            break;
        default:
            throw FunctionParserException( "unsupported operand" );
    }
}


void FunctionParserOperators::unary_op( FunctionParser::token_type_t op_type )
{
    if( op_type == FunctionParser::T_MINUS )
        tmp_inst_list.push_back( FunctionParserInstr::UNARY_MINUS );
}


void FunctionParserOperators::function_op( FctPFunctions *func )
{
    tmp_inst_list.push_back( func );
}


void FunctionParserOperators::variable_op( FctPVariable *v )
{
    tmp_inst_list.push_back( v );
}


void FunctionParserOperators::constant_op( double constant )
{
    tmp_inst_list.push_back( constant );
}


// FunctionParser --------------------------------------------------------------
FunctionParser::FunctionParser( const std::string &fct )
{
    scanner_init( fct.c_str() );
    err_state = false;
    done = false;
    
    addDefaultFunctions();

    opera = new FunctionParserOperators();
}


FunctionParser::~FunctionParser()
{
    Functions_t::iterator it;
    for( it = functions.begin(); it != functions.end(); ++it)
        delete it->second;
    
    Variables_t::iterator itv;
    for( itv = variables.begin(); itv != variables.end(); ++itv)
        delete itv->second;

    delete opera;
}


void FunctionParser::addFunction1Arg( double (*f)(double), const char *name)
{
    functions[ name ] = new FctPFunctionsBind1( f );
}


void FunctionParser::addFunction2Arg( double (*f)(double,double), const char *name)
{
    functions[ name ] = new FctPFunctionsBind2( f );
}


FctPVariable *FunctionParser::addVariable( const string &name )
{
    Variables_t::iterator it = variables.find( name );
    
    FctPVariable *var = 0;
    if( it == variables.end() )
        var = new FctPVariable( name );
    else
        var = it->second;
    
    variables[ name ] = var;
    
    return var;
}


void FunctionParser::bindVariable( const string &name, double *addr) const
{
    Variables_t::const_iterator it = variables.find( name );
    
    if( it != variables.end() )
        it->second->bind( addr );
    else
        cerr << "error: no such variable '" << name << "'\n";
}


vector<string> FunctionParser::getVariables() const
{
    Variables_t::const_iterator itv;
    vector<string> names;
    
    for( itv = variables.begin(); itv != variables.end(); ++itv)
        names.push_back( itv->second->getName() );

    return names;
}


void FunctionParser::addConstant( const string &name, double val)
{
    constants[ name ] = val;
}

    
void FunctionParser::scanner_init( const char *fkt ) 
{
    current_char = 0;
    at_eof = true;
    current_pos = 0;

    scanner_fct = fkt;
    if( (current_char = scanner_fct[0] ) != 0 )
    {
        at_eof = false;
    }
    err_state = false;
}


void FunctionParser::scanner_reset() 
{
    at_eof = false;
    current_pos = 0;
    current_token.type = T_INVALID;

    current_char = scanner_fct[0];
}


char FunctionParser::consume_char()
{
    char cc = current_char;
    if( (current_char = scanner_fct[++current_pos]) != 0 )
    {
        return cc;
    }
    else
    {
        at_eof = true;
        current_char = 0;
        current_token.type = T_EOF;
        return cc;
    }
}


char FunctionParser::peek_char()
{
    if( current_char != 0 )
    {
        return current_char;
    }
    else
    {
        at_eof = true;
        return (char)0;
    }
}


bool FunctionParser::scanDecimalLiteral( char *s, bool *is_int )
{
    char c = peek_char();
    int before_dot = 0;
    
    assert( is_digit(c) || (c == '.') );

    *is_int = false;
    
    if( is_digit(c) ) {
        before_dot++;
        *s = consume_char(); s++;
        while( is_digit(peek_char()) )
        {
            *s = consume_char(); s++;
        }   
    }
    if( peek_char()=='.' ) {
        *s = consume_char();
        s++;
        
        if( before_dot && (peek_char() == 'e' || peek_char() == 'E') ) {  // 3.E+10 case
            return scanExponentPart( s );
        }
        else
        {
            if( before_dot == 0 && !is_digit(peek_char()) ) // .E+10 not allowed
            {
                *s = 0;
                return false;
            }
            else
            {
                while( is_digit(peek_char()) )
                {
                    *s = consume_char(); s++;
                }
                *s = 0;
                if( peek_char() == 'e' || peek_char() == 'E' )
                    return scanExponentPart( s );
                else
                    return true;
            }
        }
    }
    else if(before_dot && (peek_char() == 'e' || peek_char() == 'E'))   // 3E+10 case
        return scanExponentPart( s );

    if( before_dot )
        *is_int = true;
    *s = 0;
    return true;
}


bool FunctionParser::scanExponentPart( char *s )  // exp. part is always optional
{
    char c = peek_char();
    
    assert( (c == 'e') || (c == 'E') );
    
    *s = consume_char(); s++;
    
    c = peek_char();
    if( (c == '+') || (c == '-') )      // exponent can have a sign
    {
        *s = consume_char(); s++;
    }

    c = peek_char();
    if( is_digit(c) ){                        // consume digits after e,e- or e+
        *s = consume_char(); s++;
        while( is_digit(peek_char()) )
        {
            *s = consume_char(); s++;
        }
    }
    else     // this is an error since exponent indicated but not followed by integer
        return false;
    *s = 0;
    return true;
}


FunctionParser::token_t FunctionParser::tokenize()
{
    char *s = current_token_value ;
    token_t t;
    int errors=0;
    token_type_t tt;
    char c = peek_char();
    
    t.value = current_token_value;
    *s = 0;
    
    if( !at_eof )
    {
        if( is_white(c) )
        {
            while( !at_eof && is_white(peek_char()) )
                consume_char(); 
            tt = T_ISWHITE;
        }
        else if( is_entity_beg(c) )
        {
            *s = consume_char(); s++;
            
            while( !at_eof && is_entity_char(peek_char()) )
            {
                *s = consume_char(); s++;
            }
            *s = 0;
            
            tt = T_IDENT;
        }
        else if( c=='(' )
        {
            consume_char();
            tt = T_LPAREN;
        }
        else if( c==')' )
        {
            consume_char();
            tt = T_RPAREN;
        }
        else if( is_fpnum_beg(c) ) {
            bool is_int;
            if( !scanDecimalLiteral( s, &is_int) )
                errors++;
            tt = (is_int)?T_INTNUMBER:T_FPNUMBER;
        }
        else if( c=='-' )
        {
            consume_char();
            tt = T_MINUS;
        }
        else if( c=='+' )
        {
            consume_char();
            tt = T_ADD;
        }
        else if( c=='*' )
        {
            consume_char();
            tt = T_MUL;
        }
        else if( c=='/' )
        {
            consume_char();
            tt = T_DIV;
        }
        else if( c=='^' )
        {
            consume_char();
            tt = T_POWER;
        }
        else if( c==',' )
        {
            consume_char();
            tt = T_COMMA;
        }
        else
        {
            cerr << "unexpected char";
            if( !at_eof )
                cerr << " '" << c << "'";
            cerr << "\n";
            errors++;
        }
        if( !errors )
        {
            t.type  = tt;
        }
    }
    else
        t.type  = T_EOF;
    
    if( errors )
    {
        t.type  = T_ERROR;
        *t.value = 0;
        current_token_value[0] = '\0'; 
        cerr << "tokenizer has errors.\n";
    }
    current_token.type = t.type;
    current_token.value = current_token_value;
    
    return t;
}


string FunctionParser::expect( token_type_t tt, bool advance)
{
    string s = current_token.value;
    
    if( !is_here( tt ) )
    {
        if( tt == T_FPNUMBER && current_token.type == T_INTNUMBER )  // fp relax for ints
        {
            if( advance )
                consume();
        }
        else
            throw FunctionParserException(
                string("expected ") + token_type_to_str( tt ) + " but found " +
                token_type_to_str( current_token.type ) );
    }
    else if( advance )
        consume();
    
    return s;
}


void FunctionParser::eval_function(const string &name)
{
    assert( is_here( T_LPAREN ) );
    int count_args = 0;
    
    do {
        consume();  // consume first '(', ',' afterwards
        eval_expr();
        count_args++;
    } while( is_here( T_COMMA ) );
    
    expect( T_RPAREN );

    Functions_t::iterator it = functions.find( name );
    // FIXME check presence
    if( count_args != it->second->getNumOfArgs() )
    {
        throw FunctionParserException(
            string("wrong number of arguments for function '") + name + "'" );
    }
    
    opera->function_op( it->second );
}


void FunctionParser::eval_variable( const string &name )
{
    // let's first see if it is a known constant
    Constants_t::const_iterator it = constants.find( name );

    if( it != constants.end() )
        opera->constant_op( it->second );
    else
        opera->variable_op( addVariable( name ) );
}


void FunctionParser::eval_simple_expr()
{
    switch( peek() )
    {
        case T_FPNUMBER:
        case T_INTNUMBER:
            cout << current_token.value << endl;
            opera->constant_op( double(atof(current_token.value)) );
            consume();
            break;
            
        case T_IDENT:
            {
                cout << current_token.value << endl;
            
                string ident = current_token.value;
                consume();
                if( is_here( T_LPAREN ) ) // function ?
                {
                    eval_function(ident);
                }
                else {    // variable
                    eval_variable(ident);
                }
            }
            break;
            
        default:
            throw FunctionParserException(
                                      string("unexpected value (found ") +
                                      (*current_token.value?current_token.value:
                                       token_type_to_str( current_token.type )) + ")" );
            break;
    }
}


void FunctionParser::eval_unary_expr()
{
    if( is_here( T_MINUS)  )
    {
        consume();
        // cout << "found unary minus!" << endl;
        eval_primary_expr();
        cout << "* -1" << endl;
        opera->unary_op( T_MINUS );
    }
    else
        eval_simple_expr();
}


void FunctionParser::eval_primary_expr()
{
    if( is_here( T_LPAREN ) )
    {
        consume();
        eval_expr();

        expect( T_RPAREN );
    }
    else
    {
        eval_unary_expr();
    }
}


void FunctionParser::eval_exponent()
{
    eval_primary_expr();

    
    if( is_here( T_POWER ) ) {
        token_t t = current_token;
        consume();
        
        eval_exponent();  // evaluate from right
        cout << "^" << endl;
        opera->op( t );
    }    
}


void FunctionParser::eval_multiplicative()
{
    eval_exponent();
    
    while( is_here( T_MUL ) || is_here( T_DIV ) )
    {
        string op = is_here( T_MUL )?"*":"/";
        token_t t = current_token;
        
        consume();
        
        eval_exponent();
        cout << " " << op << endl;
        opera->op( t );
    }
}


void FunctionParser::eval_additive()
{
    eval_multiplicative();
    
    while( is_here( T_ADD ) || is_here( T_MINUS ) )
    {
        string op = is_here( T_ADD )?"+":"-";
        token_t t = current_token;
        
        consume();
        
        eval_multiplicative();
        cout << " " << op << endl;
        opera->op( t );
    }
}


void FunctionParser::eval_expr()
{
    eval_additive();
}


bool FunctionParser::parse()
{
    try {
        consume();
        eval_expr();
    }
    catch( FunctionParserException & e ) {
        cerr << e.reason() << endl;
        err_state = true;
    }

    if( !is_here( T_EOF ) )
    {
        // leftovers?
        if( !err_state && has_next_token() )
        {
            cerr << "syntax error near '"
                 << get_current_token().value << "' at end of input" << endl;
            consume();
            err_state = true;
        }
    }
    
    opera->assembleInstructions();
    
    scanner_reset();   // reset scanner
    return !err_state;
}


double FunctionParser::execute()
{
    result = opera->executor();
    return result;
}
