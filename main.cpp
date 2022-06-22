#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/phoenix/object/construct.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/spirit/home/qi/numeric/real.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/recursive_variant.hpp>
#include <boost/variant/variant.hpp>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace client { namespace ast {
struct PlusMinusExpr;
using PrimaryExpr = boost::variant<double, boost::recursive_wrapper<PlusMinusExpr>>;
using PostfixExpr = PrimaryExpr;
struct UnaryExpr
{
    std::vector<char> ops;
    PostfixExpr tail;
};
using RaiseExpr = std::vector<UnaryExpr>;
struct MulDivTail
{
    char op;
    RaiseExpr raiseExpr;
};
struct MulDivExpr
{
    RaiseExpr first;
    std::vector<MulDivTail> tail;
};
struct PlusMinusTail
{
    char op;
    MulDivExpr mulDivExpr;
};
struct PlusMinusExpr
{
    MulDivExpr first;
    std::vector<PlusMinusTail> tail;
};
using Expression = PlusMinusExpr;
}} // namespace client::ast
BOOST_FUSION_ADAPT_STRUCT(client::ast::UnaryExpr, ops, tail);
BOOST_FUSION_ADAPT_STRUCT(client::ast::MulDivTail, op, raiseExpr);
BOOST_FUSION_ADAPT_STRUCT(client::ast::MulDivExpr, first, tail);
BOOST_FUSION_ADAPT_STRUCT(client::ast::PlusMinusTail, op, mulDivExpr);
BOOST_FUSION_ADAPT_STRUCT(client::ast::PlusMinusExpr, first, tail);

namespace client {
namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;

template<typename Iterator>
struct calculator : qi::grammar<Iterator, ast::Expression(), ascii::space_type>
{
    calculator()
        : calculator::base_type(Start, "Start")
    {
        using qi::char_;
        using qi::double_;

        Start = Expression >> !char_;
        Expression = PlusMinusExpr.alias();
        PlusMinusExpr = MulDivExpr >> *((char_('+') | char_('-')) > MulDivExpr);
        MulDivExpr = RaiseExpr >> *((char_('*') | char_('/')) > RaiseExpr);
        RaiseExpr = UnaryExpr >> *("**" > UnaryExpr);
        UnaryExpr = *(char_('+') | char_('-')) >> PostfixExpr;
        PostfixExpr = PrimaryExpr.alias();
        PrimaryExpr = Number | ('(' > Expression > ')');
        Number = double_;

        Start.name("Start");
        Expression.name("Expression");
        PlusMinusExpr.name("PlusMinusExpr");
        MulDivExpr.name("MulDivExpr");
        RaiseExpr.name("RaiseExpr");
        UnaryExpr.name("UnaryExpr");
        PostfixExpr.name("PostfixExpr");
        PrimaryExpr.name("PrimaryExpr");
        Number.name("Number");

        using boost::phoenix::construct;
        using boost::phoenix::val;
        using qi::on_error;
        on_error<qi::fail>(
            Start,
            std::cout << val("Error! Expecting ") << qi::_4 // what failed?
                      << val(" here: \"")
                      << construct<std::string>(
                             qi::_3,
                             qi::_2) // iterators to error-pos, end
                      << val("\"") << std::endl);
    }

    qi::rule<Iterator, ast::Expression(), ascii::space_type> Start;
    qi::rule<Iterator, ast::Expression(), ascii::space_type> Expression;
    qi::rule<Iterator, ast::PlusMinusExpr(), ascii::space_type> PlusMinusExpr;
    qi::rule<Iterator, ast::MulDivExpr(), ascii::space_type> MulDivExpr;
    qi::rule<Iterator, ast::RaiseExpr(), ascii::space_type> RaiseExpr;
    qi::rule<Iterator, ast::UnaryExpr(), ascii::space_type> UnaryExpr;
    qi::rule<Iterator, ast::PostfixExpr(), ascii::space_type> PostfixExpr;
    qi::rule<Iterator, ast::PrimaryExpr(), ascii::space_type> PrimaryExpr;
    qi::rule<Iterator, double()> Number;
};
} // namespace client

namespace client ::ast {
struct printer
{
    typedef void result_type;

    void operator()(double n) const
    {
        std::cout << n << ' ';
    }

    void operator()(const PlusMinusExpr& x) const
    {
        (*this)(x.first);
        for(auto&& tail : x.tail)
        {
            (*this)(tail.mulDivExpr);
            std::cout << tail.op << ' ';
        }
    }

    void operator()(const MulDivExpr& x) const
    {
        (*this)(x.first);
        for(auto&& tail : x.tail)
        {
            (*this)(tail.raiseExpr);
            std::cout << tail.op << ' ';
        }
    }

    void operator()(const RaiseExpr& x) const
    {
        std::string_view comma = "";
        for(auto&& a : x)
        {
            (*this)(a);
            std::cout << comma;
            comma = "** ";
        }
    }

    void operator()(const UnaryExpr& x) const
    {
        boost::apply_visitor(*this, x.tail);
        for(const char op : x.ops | boost::adaptors::reversed)
            std::cout << op << "u ";
    }
};
struct eval
{
    using result_type = double;

    result_type operator()(result_type n) const
    {
        return n;
    }

    result_type operator()(const PlusMinusExpr& x) const
    {
        result_type lhs = (*this)(x.first);
        for(auto&& a : x.tail)
        {
            const result_type rhs = (*this)(a.mulDivExpr);
            switch(a.op)
            {
            case '+':
                lhs += rhs;
                break;
            case '-':
                lhs -= rhs;
                break;
            default:
                BOOST_ASSERT_MSG(false, "unreachable");
            }
        }
        return lhs;
    }

    result_type operator()(const MulDivExpr& x) const
    {
        result_type lhs = (*this)(x.first);
        for(auto&& a : x.tail)
        {
            const result_type rhs = (*this)(a.raiseExpr);
            switch(a.op)
            {
            case '*':
                lhs *= rhs;
                break;
            case '/':
                lhs /= rhs;
                break;
            default:
                BOOST_ASSERT_MSG(false, "unreachable");
            }
        }
        return lhs;
    }

    result_type operator()(const RaiseExpr& x) const
    {
        auto it = x.rbegin();
        result_type rhs = (*this)(*it);
        ++it;
        for(; it != x.rend(); ++it)
        {
            const result_type lhs = (*this)(*it);
            rhs = std::pow(lhs, rhs);
        }
        return rhs;
    }

    result_type operator()(const UnaryExpr& x) const
    {
        result_type rhs = boost::apply_visitor(*this, x.tail);
        for(const char op : x.ops)
        {
            switch(op)
            {
            case '+':
                rhs = +rhs;
                break;
            case '-':
                rhs = -rhs;
                break;
            default:
                BOOST_ASSERT_MSG(false, "unreachable");
            }
        }
        return rhs;
    }
};

} // namespace client::ast

int main()
{
    std::cout << "/////////////////////////////////////////////////////////\n\n";
    std::cout << "Expression parser...\n\n";
    std::cout << "/////////////////////////////////////////////////////////\n\n";
    std::cout << "Type an expression...or [q or Q] to quit\n\n";

    typedef std::string::const_iterator iterator_type;
    typedef client::calculator<iterator_type> calculator;
    typedef client::ast::Expression ast_program;
    typedef client::ast::printer ast_print;
    typedef client::ast::eval ast_eval;

    std::string str;
    while(std::getline(std::cin, str))
    {
        if(str.empty() || str[0] == 'q' || str[0] == 'Q')
            break;

        calculator calc;     // Our grammar
        ast_program program; // Our program (AST)
        ast_print print;     // Prints the program
        ast_eval eval;       // Evaluates the program

        std::string::const_iterator iter = str.begin();
        std::string::const_iterator end = str.end();
        boost::spirit::ascii::space_type space;
        bool r = phrase_parse(iter, end, calc, space, program);

        if(r)
        {
            std::cout << "-------------------------\n";
            std::cout << "Parsing succeeded\n";
            print(program);
            std::cout << "\nResult: " << eval(program) << std::endl;
            std::cout << "-------------------------\n";
        }
        else
        {
            std::string rest(iter, end);
            std::cout << "-------------------------\n";
            std::cout << "Parsing failed\n";
            std::cout << "stopped at: \" " << rest << "\"\n";
            std::cout << "-------------------------\n";
        }
    }

    return 0;
}
