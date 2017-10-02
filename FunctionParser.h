#ifndef FUNCTIONPARSER_H
#define FUNCTIONPARSER_H

#include <string>
#include <vector>
#include <cmath>
#include <map>

class FunctionParserOperators;
class FctPFunctions;
class FctPVariable;

class FunctionParser {
public:
    typedef enum { T_INVALID = 0, T_ISWHITE,
                   T_LPAREN, T_RPAREN,
                   T_COMMA,
                   T_MINUS,T_ADD, T_MUL, T_DIV, T_POWER,
                   T_FPNUMBER,T_INTNUMBER,
                   T_IDENT,            
                   T_ERROR, T_EOF
    }  token_type_t;

       // a token has a value and a type
    typedef struct {
        token_type_t type;
        char        *value;
    } token_t;

private:
    static const char *token_type_to_str( token_type_t tt );
 
    FunctionParserOperators *opera;
    
    typedef std::map<std::string,FctPFunctions *> Functions_t;
    typedef std::map<std::string,FctPVariable *> Variables_t;
    typedef std::map<std::string,double> Constants_t;

public:
    FunctionParser( const std::string &fct );
    
    ~FunctionParser();
    
    void addFunction1Arg( double (*f)(double), const char *name );
    
    void addFunction2Arg( double (*f)(double,double), const char *name );
    
    FctPVariable *addVariable( const std::string &name );

    void bindVariable( const std::string &name, double *addr) const;
    
    std::vector<std::string> getVariables() const;
    
    void addConstant( const std::string &name, double val);
    
    double getResult() const
    {
        return result;
    }
    
private:
    void scanner_init( const char *fkt );
    void scanner_reset();
    char consume_char();
    char peek_char();
    
    bool scanDecimalLiteral( char *s, bool *is_int );
    bool scanExponentPart( char *s );
    
    token_t tokenize();
    
    const token_t& get_current_token()
    {
        return current_token;
    }

    void consume()
    {
        do {
            current_token = tokenize();
            if( current_token.type == T_EOF )
                break;
            
        } while( current_token.type == T_ISWHITE );

//      cout << "just consumed: " << token_type_to_str( current_token.type ) << ": "
//           << ((current_token.value  && *(current_token.value))?current_token.value:"") << "\n";
    }
    
    bool is_here( token_type_t tt )
    {
        return current_token.type == tt;
    }
    
    token_type_t peek()
    {
        return current_token.type;
    }
    
    std::string expect( token_type_t tt, bool advance = true);
    
    bool has_next_token()
    {
        return !is_here( T_EOF );
    }

    void eval_function( const std::string &name );
    void eval_variable( const std::string &name );
    
    void eval_simple_expr();
    void eval_unary_expr();
    void eval_primary_expr();
    void eval_exponent();
    void eval_multiplicative();
    void eval_additive();
    
    void eval_expr();

    void addDefaultFunctions()
    {
        addFunction1Arg( log, "log");         // natural logarithm (base e)
        addFunction1Arg( log10, "log10");     // base-10 logarithm
        addFunction1Arg( exp, "exp");         // returns the value of e raised to the power of x (= e^x)
        addFunction1Arg( sqrt, "sqrt");       // returns the non-negative square root of x
        addFunction1Arg( sin, "sin" );
        addFunction1Arg( cos, "cos" );
        addFunction1Arg( tan, "tan" );
        addFunction2Arg( pow, "pow");         // pow(x,y); returns the value of x raised to the power of y (= x^y)
    }

public:
    bool parse();
    
    double execute();
    
private:
    char current_token_value[1024];
    char current_char;

    int current_pos;
    const char *scanner_fct;
    token_t current_token;
    
    bool err_state;
    bool at_eof,done;

    Functions_t functions;   //! maps function name to binder object
    Variables_t variables;   //! maps variable name to binder object
    Constants_t constants;   //! maps constant name to double value
    
    double result;
};

#endif
