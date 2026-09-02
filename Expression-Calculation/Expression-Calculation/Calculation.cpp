#include "Calculation.h"
#include <vector>
#include <ctype.h>
#include <iostream>
#include <ostream>
#include <sstream>
#include <iomanip>
#include <exception>
#include <cmath>
#include <span>
#include <charconv>
#include <stack>
#include <format>
#include <string>
#include <unordered_set>
//判断数字相等的误差阈值
const double EPSILON = 1e-12;
//生成一个随机字符组成的名字
static std::string randName() {
	static int count = 1;
	static time_t sec = std::time(nullptr);
	srand(sec + count);
	std::string result;
	for (int i = 0; i < 5; ++i) {
		result += char('A' + rand() % 26);
	}
	result += std::to_string(count);
	++count;
	return result;
}
//小数转字符串(不带科学计数法)
static std::string doubleTostring(double val) {
	if (!std::isfinite(val)) {
		throw std::invalid_argument("not finite double(nan/inf)");
	}

	std::ostringstream oss;
	oss << std::fixed;
	oss.precision(10);
	oss << val;
	std::string s = oss.str();
	while (s.find('.') != std::string::npos && s.back() == '0') {
		s.pop_back();
	}
	if (s.back() == '.') {
		s.pop_back();
	}
	return s;
}
//把字符串按某个字符串切割
static std::vector<std::string> split_str(const std::string& str, const std::string& clip)
{
	std::vector<std::string> result;
	size_t start = 0;
	size_t pos;
	const size_t clip_len = clip.size();
	if (clip_len == 0)
		return result;

	while ((pos = str.find(clip, start)) != std::string::npos)
	{
		result.push_back(str.substr(start, pos - start));
		start = pos + clip_len;
	}
	// 末尾剩余部分
	result.push_back(str.substr(start));
	return result;
}
//判断是否是运算符的字符
static bool is_operatorCh(char c) {
	switch (c) {
	case '!':
	case '@':
	case '#':
	case '$':
	case '%':
	case '^':
	case '&':
	case '*':
	case '-':
	case '+':
	case '=':
	case '/':
	case '\\':
	case '`':
	case '<':
	case '>':
	case '?':
	case '|':
	case ':':
	case ';':
	case '\'':
	case '\"':
	case '~':
		return true;
	default:
		return false;
	}
}
//判断是不是运算符的字符串
static bool is_operatorStr(std::string str) {
	for (auto c : str) {
		if (!is_operatorCh(c)) {
			return false;
		}
	}
	return true;
}
//判断str是否遵循函数名或者变量名规则
static bool is_FuncAlgName(std::string str) {
	if (!std::isalpha(str[0])) {
		return false;
	}
	for (int i = 1; i < str.size(); ++i) {
		char cur = str[i];
		if (!(std::isalpha(cur) || std::isdigit(cur) || cur == '_')) {
			return false;
		}
	}
	return true;
}
//判断括号是否规范
static bool is_balanced_paren(std::string expr) {
	std::stack<char> stack;
	auto ispair = [](char open, char close)->bool {
		switch (open) {
		case '(':
			return close == ')';
		}
		};
	auto c_is_open = [&](char c)->short {
		switch (c) {
		case '(':
			return 1;
		case ')':
			return 0;
		default:
			return -1;
		}
		};
	for (char c : expr) {
		if (c_is_open(c) == 1) {
			stack.push(c);
		}
		else if (c_is_open(c) == 0) {
			if (stack.empty()) {
				return false;
			}
			if (ispair(stack.top(), c)) {
				stack.pop();
			}
			else {
				return false;
			}
		}
	}
	return stack.empty();
}
/*ExpressionException*/
ExpressionException::ExpressionException(const std::string& _Message) : msg(_Message) {}
char const* ExpressionException::what() const noexcept
{
	return msg.c_str();
}
//ExprExcep 快捷throw异常
class ExprExcep {
private:
	std::string msg_;
public:
	ExprExcep() = delete;
	ExprExcep& operator=(const ExprExcep& see) = delete;
	ExprExcep(std::string msg) : msg_(msg) {};
	template <typename...Args>
	void operator()(Args ...args) const {
		throw ExpressionException(std::vformat(msg_, std::make_format_args(args...)));
	}
};
const ExprExcep err_Func_WrongNumArgs(R"(Function "{}": incorrect number of parameters, expected {}, received {})");
const ExprExcep err_Func_Unknown(R"(Function "{}": unknown)");
const ExprExcep err_Func_NoneArgs(R"(Function "{}": not supported 0 arguments)");
const ExprExcep err_Func_MaximumArgs(R"(Function "{}": receive {} arguments, maximum support is 64)");
const ExprExcep err_Func_DuplicateArgs(R"(Function "{}": has duplicate parameter)");
const ExprExcep err_Func_MissingArgs(R"(Function: missing parameter)");
const ExprExcep err_Token_Unknown(R"(Token "{}": unknown)");
const ExprExcep err_Num_FormatHasTwoPoint(R"(Num : invalid numeric format "{}" has two point.)");
const ExprExcep err_Expr_FormatHasTwoPoint(R"(Expression: have a mistake format.)");
const ExprExcep err_Expr_Invalid(R"(Expression: invalid expression.)");
const ExprExcep err_Expr_ParenUnbalanced(R"err(Expression: paren "( )" are not balanced.)err");
const ExprExcep err_Operator_UnknownUsage(R"(Operator "{}": unknown usage.)");
const ExprExcep err_Operator_Unknown(R"(Operator "{}": unknown.)");
const ExprExcep err_Operator_NotBinary(R"(Operator "{}": is not a binary operator)");
const ExprExcep err_Operator_NotPrefix(R"(Operator "{}": is not a prefix operator)");
const ExprExcep err_Algebra_Undifined(R"(Algebra "{}": undefined)");
/*Token*/
Token::Token() : type_t(TokenType::UNKNOWN) {};
Token::Token(std::string str) : str_t(str) {
	if (str == "(") {
		type_t = TokenType::LPAREN;
	}
	else if (str == ")") {
		type_t = TokenType::RPAREN;
	}
	else if (str == ",") {
		type_t = TokenType::COMMA;
	}
	else if (str == std::string(str.size(), ' ')) {
		type_t = TokenType::SPACE;
	}
	else if (is_operatorStr(str)) {
		type_t = TokenType::BINO;
	}
	else if (is_FuncAlgName(str)) {
		type_t = TokenType::ALGEBRA;
	}
	else if (str[str.size() - 1] == '(' && is_FuncAlgName(str.substr(0, str.size() - 1))) {
		str_t.erase(str_t.size() - 1);
		type_t = TokenType::FUNC;
	}
	else {
		type_t = TokenType::UNKNOWN;
	}
}
Token::Token(std::string str, OperatorBaseSymbol* op) : Token(str) {
	oper_ptr = op;
};
Token::Token(double num) : num_t(num), type_t(TokenType::NUM) {};
Token::Token(const Token& t)
{
	type_t = t.type_t;
	str_t = t.str_t;
	num_t = t.num_t;
	oper_ptr = t.oper_ptr;
}
double Token::n_data() const
{
	return num_t;
}
std::string Token::s_data() const
{
	return str_t;
}
std::string Token::data() const
{
	switch (type_t) {
	case TokenType::NUM:
		return doubleTostring(num_t);
	default:
		return str_t;
	}
}
//是否是普通数组
bool Token::is_num() const
{
	return type_t == TokenType::NUM;
}
//是否是代数
bool Token::is_algebra() const {
	return type_t == TokenType::ALGEBRA;
}
bool Token::is_alg_num() const
{
	return is_algebra() || is_num();
}
//是否是左括号
bool Token::is_leftparen() const {
	return type_t == TokenType::LPAREN;
};
//是否是右括号
bool Token::is_rightparen() const {
	return type_t == TokenType::RPAREN;
};
//是否是二元运算符
bool Token::is_binary_operator() const {
	return type_t == TokenType::BINO;
}
//是否是一元前缀运算符
bool Token::is_prefix_operator() const {
	return type_t == TokenType::PREO;
}
//是否是运算符
bool Token::is_operator() const
{
	return is_binary_operator() || is_prefix_operator() || is_unknown_operator();
}
//是否是逗号
bool Token::is_comma() const {
	return type_t == TokenType::COMMA;
};
//是否是函数
bool Token::is_function() const {
	return type_t == TokenType::FUNC;
}
//是否是空格
bool Token::is_space() const
{
	return type_t == TokenType::SPACE;
}
//是否是未知的运算符
bool Token::is_unknown_operator() const
{
	return type_t == TokenType::UNKNOWNO;
}
//这个Token在某个运算符Token的左边 能否有数字的意义
bool Token::is_ableToNum_at_left() const
{
	return is_rightparen() || is_alg_num();
}
//这个Token在某个运算符Token的右边 能否有数字的意义
bool Token::is_ableToNum_at_right() const
{
	return is_leftparen() || is_function() || is_alg_num();
}
//重新设置存的字符串数据
void Token::set_s_data(const std::string d)
{
	str_t = d;
}
BinaryOperatorSymbol* Token::get_binary_operator() const
{
	if (!is_binary_operator()) {
		err_Operator_NotBinary(data());
	}
	return dynamic_cast<BinaryOperatorSymbol*>(oper_ptr);
}
PreFixOperatorSymbol* Token::get_prefix_operator() const
{
	if (!is_prefix_operator()) {
		err_Operator_NotPrefix(data());
	}
	return dynamic_cast<PreFixOperatorSymbol*>(oper_ptr);
}
void Token::set_operator(OperatorBaseSymbol* op)
{
	oper_ptr = op;
}
//返回这个Token的类型字符串表示
std::string Token::TypeS() const
{
	if (is_num()) {
		return "NUM";
	}
	else if (is_algebra()) {
		return "ALGEBRA";
	}
	else if (is_leftparen()) {
		return "LPAREN";
	}
	else if (is_rightparen()) {
		return "RPAREN";
	}
	else if (is_binary_operator()) {
		return "BINARY OPERATOR";
	}
	else if (is_prefix_operator()) {
		return "PREFIX OPERATOR";
	}
	else if (is_unknown_operator()) {
		return "UNKNOWN OPERATOR";
	}
	else if (is_function()) {
		return "FUNC";
	}
	else if (is_comma())
	{
		return "COMMA";
	}
	else if (is_space()) {
		return "SPACE";
	}
	else {
		return "UNKNOWN";
	}
}
std::ostream& operator<<(std::ostream& os, const Token& t)
{
	os << t.data();
	return os;
}
//用于调试：输出Token数组中的每个token的值
std::ostream& operator<<(std::ostream& os, const std::vector<Token>& vec) {
	for (auto& v : vec) {
		os << v << " " << v.TypeS() << '\n';
	};
	return os;
}
Token& Token::operator=(const Token& t)
{
	if (&t == this) return *this;
	type_t = t.type_t;
	str_t = t.str_t;
	num_t = t.num_t;
	oper_ptr = t.oper_ptr;
	return *this;
}
/*BaseSymbol*/
BaseSymbol::BaseSymbol(const std::string& id) : id_(id) {}
std::string BaseSymbol::ID() const
{
	return id_;
}
/*OpratorBaseSymbol*/
OperatorBaseSymbol::OperatorBaseSymbol(const std::string& id, std::function<double(const std::vector<double>&)> func) : BaseSymbol(id)
{
	func_ = func;
}
/*PreFixOperatorSymbol*/
PreFixOperatorSymbol::PreFixOperatorSymbol(const std::string& id, double (*func)(double vale)) : OperatorBaseSymbol(id, 
	[func](const std::vector<double>& args) -> double
		{
			return func(args[0]);
		}) {}
double PreFixOperatorSymbol::evaluate(Args_ args) const
{
	if (args.size() != 1) {
		err_Func_WrongNumArgs(ID(), 1, args.size());
	}
	return func_(args);
};
/*BinaryOperatorSymbol*/
BinaryOperatorSymbol::BinaryOperatorSymbol(const std::string& id, unsigned short priority, double(*func)(double left, double right)) : OperatorBaseSymbol(id,
	[func](const std::vector<double>& args) -> double
	{
		return func(args[0], args[1]);
	}), priority_(priority) { }
size_t BinaryOperatorSymbol::priority() const
{
	return priority_;
}
double BinaryOperatorSymbol::evaluate(Args_ args) const
{
	if (args.size() != 2) {
		err_Func_WrongNumArgs(ID(), 2, args.size());
	}
	return func_(args);
}
/*FunBaseSymbol*/
FunBaseSymbol::FunBaseSymbol(const std::string& id, param_num args_num)
	: BaseSymbol(id), args_num_(args_num) {
	//参数数量过多 过少检查
	if (args_num_ == 0) {
		err_Func_NoneArgs(id);
	}
	else if (args_num_ > 64) {
		err_Func_MaximumArgs(id, args_num_);
	}
}
unsigned short FunBaseSymbol::ParamNum() const
{
	return args_num_;
}
/*FunctionSymbol*/
FunctionSymbol::FunctionSymbol(const std::string& id, unsigned short args_num, double(*func)(Args_ args))
	:FunBaseSymbol(id, args_num)
{
	func_ = func;
}
double FunctionSymbol::evaluate(Args_ args) const
{
	return func_(args);
}
/*FxSymbol*/
FxSymbol::FxSymbol(const std::string& id, const std::vector<std::string>& args_name
	, const std::string& fxexpress, ICalculation& i_calc)
	:FunBaseSymbol(id, args_name.size()), expr_(fxexpress)
	, i_calc_(i_calc), default_args_name_(args_name)
{
	if (std::unordered_set<std::string>(args_name.begin(), args_name.end()).size() != args_name.size()) {
		err_Func_DuplicateArgs(id);
	};
	post_expr_tokens_ = ExpressionParser::infix_to_postfixVec(fxexpress, i_calc_);
	for (int i = 0; i < ParamNum();++i) {
		obfuscated_args_name_.push_back(randName());
	}
	for (int i = 0; i < ParamNum();++i) {
		for (auto& token : post_expr_tokens_) {
			if (token.is_algebra() && (token.data() == args_name[i])) {
				token.set_s_data(obfuscated_args_name_[i]);
			}
		}
	}
}
double FxSymbol::evaluate(Args_ args) const
{
	for (int i = 0; i < ParamNum(); ++i) {
		i_calc_.createVal(obfuscated_args_name_[i], args[i]);
	}
	double result = ExpressionParser::postfixVec_calculate(post_expr_tokens_, i_calc_);
	for (int i = 0; i < ParamNum(); ++i) {
		i_calc_.deleteVal(obfuscated_args_name_[i]);
	}
	return result;
}
const std::vector<std::string>& FxSymbol::get_args_name() const
{
	return default_args_name_;
}
const std::string& FxSymbol::get_expression() const
{
	return expr_;
}
/*ExpressionParser*/
//把表达式拆分成Token
std::vector<Token> ExpressionParser::tokenize(const std::string& expression, ICalculation& i_calc)
{
	std::vector<Token> result;
	for (int i = 0; i < expression.size(); ++i) {
		char cur = expression[i];
		char next = (i == expression.size() - 1) ? '\0' : expression[i + 1];
		//判断是否为( ) , (LPAREN RPAREN COMMA)
		if (cur == '(' || cur == ')' || cur == ',') {
			result.push_back(Token(std::string(1, cur)));
		}
		//判断是否为数字的特征符号 0~9 . - (NUM)
		else if (isdigit(cur) || cur == '.' || //数字或者小数点/*这里是预防3-3把-识别为符号 而不是一元前缀运算符*/
			(cur == '-' && isdigit(next) && (result.size() == 0 || !result.back().is_ableToNum_at_left()))) {
			std::string num_str = ""; //存储读取的数字
			bool has_a_point = false; //是否已经读取到一个小数点
			if (cur == '.') {
				num_str += "0.";
				has_a_point = true;
			}
			else if (cur == '-') {
				num_str += "-";
			}
			else {
				num_str += cur;
			}
			i++;
			for (int j = 0, i_org = i;j + i_org < expression.size();++j, ++i) {
				char last = cur;
				cur = expression[i_org + j];
				//识别到小数点 如果之前早就识别过小数点 则会报错
				if (cur == '.') {
					if (has_a_point == true) {
						err_Num_FormatHasTwoPoint(num_str);
					}
					else {
						num_str += '.';
						has_a_point = true;
					}
				}
				//识别到数字 就把这个数字加到num_str
				else if (isdigit(cur)) {
					num_str += cur;
				}
				//如果没识别到数字 结束数字识别状态
				else if (!isdigit(cur)) {
					//如果上一个字符是. 那么自动补一个0
					if (j > 0 && last == '.') {
						num_str += '0';
					}
					i--;
					break;
				}
			}
			result.push_back(Token(std::stod(num_str)));
		}
		//判断函数(FUNC) 还是代数(ALGEBRA)
		else if (isalpha(cur)) {
			std::string name = "";
			for (int j = 0, i_org = i;j + i_org < expression.size();++j, ++i) {
				cur = expression[i_org + j];
				//如果是函数名或者代数名的特征字符就加到name
				if (std::isalpha(cur) || std::isdigit(cur) || cur == '_') {
					name += cur;
				}
				//如果遇到括号 函数识别截止
				else if (cur == '(') {
					name += '(';
					result.push_back(Token(name));
					break;
				}
				else
				{
					result.push_back(Token(name));
					--i;
					break;
				}
				if (j + i_org + 1 == expression.size()) {
					result.push_back(Token(name));
					break;
				}
			}
		}
		//判断运算符 (OPERATER)
		else if (is_operatorCh(cur)) {
			std::string name = "";
			//如果下一个字符也是运算符的字符
			if (is_operatorCh(next)) {
				//先考虑cur是不是二元运算符的第一个字符
				if (isBinaryOperatorFirstCh(cur, i_calc)) {
					name += cur;
					++i;
					cur = expression[i];
					//若是 则检测cur 和next组合是不是两个字符的二元运算符
					if (isHavedBinaryOperator(name + cur, i_calc)) {
						name += cur;
						Token token = Token(name, get_binary_operator(name, i_calc));
						token.type_t = TokenType::BINO;
						result.push_back(token);
					}
					//如果不是 则检测cur是不是一个字符的二元运算符
					else if (isHavedBinaryOperator(name, i_calc)) {
						--i;
						//若是则将他移入UNKNOWN 因为不能确定是前缀一元还是二元
						Token token = Token(name);
						token.type_t = TokenType::UNKNOWNO;
						result.push_back(token);
					}
					else {
						err_Operator_Unknown(name + cur);
					}
				}
				//再考虑cur是不是前缀运算符
				else if (isHavedPrefixOperator(std::string(1, cur), i_calc)) {
					//若是则将他移入UNKNOWN 因为不能确定是前缀一元还是二元
					Token token = Token(name);
					token.type_t = TokenType::UNKNOWNO;
					result.push_back(token);
				}
				else {
					err_Operator_Unknown(std::string(1, cur));
				}
			}
			//如果下一个字符不是运算符的字符
			else if (isChCommonAtBinAndPre(std::string(1, cur), i_calc)) {
				name += cur;
				Token token = Token(name);
				token.type_t = TokenType::UNKNOWNO;//可能是前缀也可能是二元 不确定
				result.push_back(token);
			}
			else {
				err_Operator_Unknown(std::string(1, cur));
			}

		}
		//判断空格
		else if (cur == ' ') {
			std::string space_str;
			for (int j = 0, i_org = i;j + i_org < expression.size();++j, ++i) {
				cur = expression[i_org + j];
				//如果是函数名或者代数名的特征字符就加到name
				if (cur == ' ') {
					space_str += ' ';
				}
				else if (cur != ' ') {
					--i;
					result.push_back(Token(space_str));
					break;
				}
				if (j + i_org + 1 == expression.size()) {

					result.push_back(Token(space_str));
					break;
				}
			}
		}
	}
	for (int i = 0; i < result.size(); ++i) {
		Token* pre = i >= 1 ? &result[i - 1] : nullptr;
		Token* cur = &result[i];
		Token* next = i < result.size() - 1 ? &result[i + 1] : nullptr;
		if (cur->type_t == TokenType::UNKNOWNO) {
			if (pre == nullptr) {
				cur->type_t = TokenType::PREO;
				cur->set_operator(get_prefix_operator(cur->data(), i_calc));
			}
			else if (next == nullptr || next->is_comma()) {
				cur->type_t = TokenType::UNKNOWNO;
			}
			else if (!pre->is_ableToNum_at_left() && next->is_ableToNum_at_right()) {
				cur->type_t = TokenType::PREO;
				cur->set_operator(get_prefix_operator(cur->data(), i_calc));

			}
			else if (pre->is_ableToNum_at_left() && (next->type_t == TokenType::UNKNOWNO || next->is_ableToNum_at_right())) {
				cur->type_t = TokenType::BINO;
				cur->set_operator(get_binary_operator(cur->data(), i_calc));
			}
		}
	}
	return result;
}
BinaryOperatorSymbol* ExpressionParser::get_binary_operator(const std::string& c, ICalculation& i_calc)
{
	return BaseSymbol::getSymbol<BinaryOperatorSymbol>(c, i_calc.get_funs_and_operators());
}
PreFixOperatorSymbol* ExpressionParser::get_prefix_operator(const std::string& c, ICalculation& i_calc)
{
	return BaseSymbol::getSymbol<PreFixOperatorSymbol>(c, i_calc.get_funs_and_operators());
}
FunBaseSymbol* ExpressionParser::get_function(const std::string& c, ICalculation& i_calc)
{
	return BaseSymbol::getSymbol<FunBaseSymbol>(c, i_calc.get_funs_and_operators());
}
//判断某个字符是否是某个二元运算符的第一个字符
bool ExpressionParser::isBinaryOperatorFirstCh(char c, ICalculation& i_calc)
{
	for (auto* item : i_calc.get_funs_and_operators()) {
		if (typeid(BinaryOperatorSymbol) == typeid(*item) && item->ID()[0] == c) {
			return true;
		}
	}
	return false;
}
//判断某个字符是否是某个运算符的第一个字符
bool ExpressionParser::isHavedOperatorFirstCh(char c, ICalculation& i_calc)
{
	for (auto* item : i_calc.get_funs_and_operators()) {
		if ((typeid(BinaryOperatorSymbol) == typeid(*item)
			|| typeid(PreFixOperatorSymbol) == typeid(*item))
			&& item->ID()[0] == c) {
			return true;
		}
	}
	return false;
}
//判断某个运算符是否是已经注册的二元运算符
bool ExpressionParser::isHavedBinaryOperator(const std::string& c, ICalculation& i_calc)
{
	for (auto* item : i_calc.get_funs_and_operators()) {
		if (typeid(BinaryOperatorSymbol) == typeid(*item)
			&& item->ID() == c) {
			return true;
		}
	}
	return false;
}
//判断某个运算符是否是已经注册的前缀一元运算符
bool ExpressionParser::isHavedPrefixOperator(const std::string& c, ICalculation& i_calc)
{
	for (auto* item : i_calc.get_funs_and_operators()) {
		if (typeid(PreFixOperatorSymbol) == typeid(*item)
			&& item->ID() == c) {
			return true;
		}
	}
	return false;
}
//判断某个函数名是否是已经注册的
bool ExpressionParser::isHavedFunction(const std::string& c, ICalculation& i_calc)
{
	for (auto* item : i_calc.get_funs_and_operators()) {
		if ((typeid(FunctionSymbol) == typeid(*item)
			|| typeid(FunctionSymbol) == typeid(*item))
			&& item->ID() == c) {
			return true;
		}
	}
	return false;
}
//判断某个运算符是不是既属于二元运算符也属于一元运算符
bool ExpressionParser::isChCommonAtBinAndPre(const std::string& c, ICalculation& i_calc)
{
	return isHavedBinaryOperator(c, i_calc) || isHavedPrefixOperator(c, i_calc);
}
//中缀表达式转后缀表达式 返回token数组
std::vector<Token> ExpressionParser::infixVec_to_postfixVec(const std::vector<Token>& expr_tokens, ICalculation& i_calc)
{
	std::stack<Token> ostack;
	std::vector<Token> put;
	//把Token推到输出区
	auto toPut = [&](const Token& t) { put.push_back(t);};
	//把Token推到栈
	auto toStack = [&](const Token& t) { ostack.push(t);};
	//栈顶元素
	auto Top = [&]() -> const Token& { return ostack.top();};
	//移除栈顶元素
	auto Pop = [&]() { ostack.pop();};
	//栈是否为空
	auto Empty = [&]() -> bool { return ostack.empty();};
	//把栈顶元素移动到输出区
	auto StackToPut = [&]() { 
		auto top_element = ostack.top();
		put.push_back(top_element);
		ostack.pop();
		};

	size_t comma_num = 0;
	std::stack<Token> param_stack;
	//用来检查括号以及逗号(函数参数数量)是否正确
	auto check = [&](const Token& t) {
		//原理是左括号和逗号入栈 遇到右括号逐个统计逗号数量直到左括号
		if (t.is_leftparen() || t.is_function() || t.is_comma()) {
			param_stack.push(t);
		}
		else if (t.is_rightparen()) {
			while (param_stack.top().is_comma()) {
				++comma_num;
				param_stack.pop();
			}
			if (param_stack.top().is_function()) {
				auto* fun = get_function(param_stack.top().data(), i_calc);
				if (fun == nullptr) {
					err_Func_Unknown(param_stack.top().data());
				}
				else {
					if (fun->ParamNum() == comma_num + 1) {
						param_stack.pop();
						comma_num = 0;
					}
					else {
						err_Func_WrongNumArgs(fun->ID(), fun->ParamNum(), comma_num + 1);
					}
				}
			}
			else if (param_stack.top().is_leftparen()) {
				param_stack.pop();
			}
		}
		};
	const Token* last = nullptr;//表示上一个Token 一般用来检查数字与代数函数 或者代数函数之间省略乘号的情况 以便自动添加乘号
	Token multiply_sign("*");//自动添加的乘号
	multiply_sign.set_operator(get_binary_operator("*", i_calc));
	//自动补乘号 机制为当前Token如果是 代数 数字 左括号 函数 且上一个Token是代数 数字 右括号那么自动填充一个乘号
	auto AutoMultiplySign = [&]() {
		if (last && last->is_ableToNum_at_left()) {
			toStack(multiply_sign);
		}
		};
	for (const Token& t : expr_tokens) {
		if (t.is_ableToNum_at_right()) {
			AutoMultiplySign();
		}
		//token是数字
		if (t.is_num()) {
			toPut(t);
		}
		//token是变量
		else if (t.is_algebra()) {
			toPut(t);
		}
		//左括号(
		else if (t.is_leftparen()) {
			toStack(t);
			check(t);
		}
		//右括号
		else if (t.is_rightparen()) {
			//如果括号前是数字或者函数就报错(参数填充错误)
			if (last && (last->is_comma() || last->is_function())) {
				err_Func_MissingArgs();
			}
			check(t);
			if (!Empty() && last && last->is_leftparen()) {
				//这个是遇到纯括号()的情况 自动补一个0
				toPut(Token(0.0));
				Pop();
				continue;
			}
			while (!Empty() && (!Top().is_leftparen() && !Top().is_function())) {
				//栈顶有运算符转移到输出直到左括号
				StackToPut();
			}
			//转移运算符后清除栈顶的左括号
			if (!Empty() && Top().is_function()) {
				StackToPut();
			}
			else if (!Empty() && Top().is_leftparen()) {
				Pop();
			}
		}
		//二元运算符
		else if (t.is_binary_operator()) {
			while (!Empty() && Top().is_binary_operator() && 
				Top().get_binary_operator()->priority() <= t.get_binary_operator()->priority()) {
				StackToPut();
			}
			toStack(t);
		}
		//前缀一元运算符
		else if (t.is_prefix_operator()) {
			toStack(t);
		}
		//函数
		else if (t.is_function()) {
			check(t);
			toStack(t);
		}
		//逗号
		else if (t.is_comma()) {
			if (last && (last->is_function() || last->is_comma())) {
				err_Func_MissingArgs();
			}
			check(t);
			while (!Empty() && !Top().is_function()) {
				StackToPut();
			}
		}
		else if (t.is_unknown_operator()) {
			err_Operator_UnknownUsage(t.data());
		}
		else {
			err_Token_Unknown(t.data());
		}
		last = &t;
	}
	if (!param_stack.empty()) {
		err_Expr_ParenUnbalanced();
	}
	while (!ostack.empty()) {
		StackToPut();
	}
	return put;
}
std::vector<Token> ExpressionParser::infix_to_postfixVec(const std::string& expr, ICalculation& i_calc)
{
	return infixVec_to_postfixVec(tokenize(expr, i_calc), i_calc);
}
double ExpressionParser::postfixVec_calculate(const std::vector<Token>& tokens, ICalculation& i_calc)
{
	std::stack<Token> stack;
	auto TopNum = [&]() -> double { 
		double top_num = stack.top().n_data();
		stack.pop();
		return top_num;
		};
	auto PushNum = [&](double num) { return stack.push(Token(num));};
	for (auto t : tokens) {
		if (t.is_num()) {
			PushNum(t.n_data());
		}
		else if (t.is_algebra()) {
			PushNum(i_calc.getVal(t.data()));
		}
		else if (t.is_binary_operator()) {
			BinaryOperatorSymbol* ps = t.get_binary_operator();
			double right = TopNum();
			double left = TopNum();
			PushNum(ps->evaluate({ left, right }));
		}
		else if (t.is_prefix_operator()) {
			PreFixOperatorSymbol* ps = t.get_prefix_operator();
			double num = TopNum();
			PushNum(ps->evaluate({ num }));
		}
		else if (t.is_function()) {
			FunBaseSymbol* fs = BaseSymbol::getSymbol<FunBaseSymbol>(t.data(), i_calc.get_funs_and_operators());
			if (fs == nullptr) {
				err_Func_Unknown(t.data());
			}
			std::vector<double> args(fs->ParamNum());
			for (int i = 0; i < fs->ParamNum(); ++i) {
				args[fs->ParamNum() - 1 - i] = TopNum();
			}
			PushNum(fs->evaluate(args));
		}
	}
	if (stack.empty()) {
		err_Expr_FormatHasTwoPoint();
	}
	if (stack.size() > 1) {

	}
	return stack.top().n_data();
}
/*Expression*/
double ExpressionParser::calculate(const std::string& expr, ICalculation& i_calc)
{
	return calculate(tokenize(expr, i_calc), i_calc);
}
double ExpressionParser::calculate(const std::vector<Token>& expr_tokens, ICalculation& i_calc)
{
	return postfixVec_calculate(infixVec_to_postfixVec(expr_tokens, i_calc), i_calc);
}
/*Calculation*/
void Calculation::addBinOperation(const std::string& id, unsigned short priority, double(*func)(double left, double right))
{
	funs_and_operators.push_back(new BinaryOperatorSymbol(id, priority, func));
}
void Calculation::addPreOperation(const std::string& id, double(*func)(double value))
{
	funs_and_operators.push_back(new PreFixOperatorSymbol(id, func));
}
std::string Calculation::getFunctions() const
{
	std::string funslist = "";
	for (auto* item : funs_and_operators) {
		funslist += (item->ID() + ",");
	}
	if (funslist.size() > 0)
		funslist.erase(funslist.size() - 1);
	return funslist;
}
FunBaseSymbol* Calculation::get_function(const std::string& id) const
{
	return BaseSymbol::getSymbol<FunBaseSymbol>(id, funs_and_operators);
}
void Calculation::default_registFuncAndOpera()
{
	createVal("pi", 3.141592653589793);
	createVal("e", 2.718281828459045);
	addBinOperation("=", 1000, [](double l, double r)->double {return r;});
	addBinOperation("&&", 8, [](double l, double r)->double {return (fabs(l - 1) <= EPSILON) && (fabs(r - 1) <= EPSILON);});
	addBinOperation("||", 7, [](double l, double r)->double {return (fabs(l - 1) <= EPSILON) || (fabs(r - 1) <= EPSILON);});
	addBinOperation("==", 6, [](double l, double r)->double {return fabs(l - r) < EPSILON;});
	addBinOperation("!=", 6, [](double l, double r)->double {return l != r;});

	addBinOperation("<", 5, [](double l, double r)->double {return l < r;});
	addBinOperation("<=", 5, [](double l, double r)->double {return l <= r;});
	addBinOperation(">", 5, [](double l, double r)->double {return l > r;});
	addBinOperation(">=", 5, [](double l, double r)->double {return l >= r;});

	addBinOperation("+", 4, [](double l, double r) {return l + r;});
	addBinOperation("-", 4, [](double l, double r) {return l - r;});

	addBinOperation("*", 3, [](double l, double r) {return l * r;});
	addBinOperation("/", 3, [](double l, double r) {return l / r;});
	addBinOperation("%", 3, [](double l, double r)->double {return (size_t)l % (size_t)r;});
	addBinOperation("^", 2, [](double l, double r) {return pow(l, r);});

	addPreOperation("+", [](double n) {return +n;});
	addPreOperation("-", [](double n) {return -n;});
	addPreOperation("!", [](double n) {
		if (fabs(n - 1) <= EPSILON) {
			return 0.0;
		}
		else if(fabs(n) <= EPSILON) {
			return 1.0;
		}
		return n;
		});

	addFunction(new FunctionSymbol("max", 2, [](Args_ ns)->double
		{
			return std::fmax(ns[0], ns[1]);
		}));
	addFunction(new FunctionSymbol("min", 2, [](Args_ ns)->double
		{
			return std::fmin(ns[0], ns[1]);
		}));
	addFunction(new FunctionSymbol("abs", 1, [](Args_ ns)->double
		{
			return std::abs(ns[0]);
		}));
	addFunction(new FunctionSymbol("pow", 2, [](Args_ ns)->double
		{
			return std::pow(ns[0], ns[1]);
		}));
	addFunction(new FunctionSymbol("sqrt", 1, [](Args_ ns)->double
		{
			return std::sqrt(ns[0]);
		}));
	addFunction(new FunctionSymbol("round", 1, [](Args_ ns)->double
		{
			return std::round(ns[0]);
		}));
	addFunction(new FunctionSymbol("floor", 1, [](Args_ ns)->double
		{
			return std::floor(ns[0]);
		}));
	addFunction(new FunctionSymbol("ceil", 1, [](Args_ ns)->double
		{
			return std::ceil(ns[0]);
		}));
	addFunction(new FunctionSymbol("exp", 1, [](Args_ ns)->double
		{
			return std::exp(ns[0]);
		}));
	addFunction(new FunctionSymbol("fmod", 2, [](Args_ ns)->double
		{
			return std::fmod(ns[0], ns[1]);
		}));
	addFunction(new FunctionSymbol("sin", 1, [](Args_ ns)->double
		{
			return std::sin(ns[0]);
		}));
	addFunction(new FunctionSymbol("cos", 1, [](Args_ ns)->double
		{
			return std::cos(ns[0]);
		}));
	addFunction(new FunctionSymbol("tan", 1, [](Args_ ns)->double
		{
			return std::tan(ns[0]);
		}));
	addFunction(new FunctionSymbol("atan", 1, [](Args_ ns)->double
		{
			return std::atan(ns[0]);
		}));
	addFunction(new FunctionSymbol("atan2", 2, [](Args_ ns)->double
		{
			return std::atan2(ns[0], ns[1]);
		}));
	addFunction(new FunctionSymbol("asin", 1, [](Args_ ns)->double
		{
			return std::asin(ns[0]);
		}));
	addFunction(new FunctionSymbol("acos", 1, [](Args_ ns)->double
		{
			return std::acos(ns[0]);
		}));
	addFunction(new FunctionSymbol("log10", 1, [](Args_ ns)->double
		{
			return std::log10(ns[0]);
		}));
	addFunction(new FunctionSymbol("log2", 1, [](Args_ ns)->double
		{
			return std::log2(ns[0]);
		}));
	addFunction(new FunctionSymbol("log", 2, [](Args_ ns)->double
		{
			return std::log(ns[1]) / std::log(ns[0]);
		}));
}
const std::vector<BaseSymbol*>& Calculation::get_funs_and_operators() const
{
	return funs_and_operators;
}
Calculation::Calculation()
{
	default_registFuncAndOpera();
}
Calculation::~Calculation()
{
	for (auto item : funs_and_operators) {
		if (item != nullptr) {
			delete item;
		}
	}
	for (auto item : algebras) {
		if (item != nullptr) {
			delete item;
		}
	}
}
double Calculation::getVal(const std::string& name)const
{
	for (auto* alg : algebras) {
		if (alg->id == name) {
			return alg->value;
		}
	}
	err_Algebra_Undifined(name);
}
void Calculation::addFunction(const std::string& id, unsigned short param_num, double(*func)(Args_))
{
	addFunction(new FunctionSymbol(id, param_num, func));
}
void Calculation::createVal(const std::string& name, double default_num)
{
	for (auto* alg : algebras) {
		if (alg->id == name) {
			alg->value = default_num;
			return;
		}
	}
	algebras.push_back(new Algebra(name, default_num));
}
void Calculation::setVal(const std::string& name, double set_num)
{
	for (auto* alg : algebras) {
		if (alg->id == name) {
			alg->value = set_num;
			return;
		}
	}
	err_Algebra_Undifined(name);
}
void Calculation::deleteVal(const std::string& name)
{
	int i = 0;
	for (auto* alg : algebras) {
		if (alg->id == name) {
			delete alg;
			algebras.erase(algebras.begin() + i);
			return;
		}
		++i;
	}
	err_Algebra_Undifined(name);
}
void Calculation::reValname(const std::string& old_name, const std::string& new_name)
{
	for (auto* alg : algebras) {
		if (alg->id == old_name) {
			alg->id = new_name;
			return;
		}
	}
	err_Algebra_Undifined(old_name);
}
std::string Calculation::getAllVal() const
{
	std::string alglist;
	for (auto item : algebras) {
		alglist += (item->id + "=" + doubleTostring(item->value) + ",");
	}
	if (alglist.size() > 0)
		alglist.erase(alglist.size() - 1);
	return alglist;
}
void Calculation::createFx(const std::string& name, std::string expr, const std::vector<std::string>& args_name)
{
	if (get_function(name)) {
		deleteFxAndFunc(name);
	}
	FxSymbol* fxsy = new FxSymbol(name, args_name, expr, *this);
	addFunction(fxsy);
}
double Calculation::getFxVal(const std::string& name, Args_ args) const
{
	auto* f = get_function(name);
	if (f) {
		return f->evaluate(args);
	}
	err_Func_Unknown(name);
}
void Calculation::setFx(const std::string& name, const std::string& expr, const std::vector<std::string>& args_name)
{
	deleteFxAndFunc(name);
	createFx(name, expr, args_name);
}
void Calculation::deleteFxAndFunc(const std::string& name)
{
	int i = 0;
	for (auto* item : funs_and_operators) {
		if (item->ID() == name) {
			delete item;
			funs_and_operators.erase(funs_and_operators.begin() + i);
			return;
		}
		++i;
	}
	throw ExpressionException("Function \"" + name + "\": undefined");
}
std::string Calculation::command(const std::string& cmd, bool ColorOpen)noexcept
{
	auto color_tokens = [&](const std::vector<Token>& ts) {
		std::string result = "";
		std::string PAREN1 = ColorOpen ? "\033[33m" : "";
		std::string PAREN2 = ColorOpen ? "\033[93m" : "";
		std::string NUM = ColorOpen ? "\033[92m" : "";
		std::string ALG = ColorOpen ? "\033[96m" : "";
		std::string FUNC = ColorOpen ? "\033[33m" : "";
		std::string OPERA = ColorOpen ? "\033[31m" : "";
		std::string COMMA = ColorOpen ? "\033[33m" : "";
		std::string UNKNOWN = ColorOpen ? "\033[0m" : "";
		std::stack<Token> paren_comma_stack;
		auto PAREN = [&]() {
			return paren_comma_stack.size() % 2 == 0 ? PAREN1 : PAREN2;
			};
		for (auto token : ts) {
			if (token.is_num()) {
				result += NUM;
				result += doubleTostring(token.n_data());
			}
			else if (token.is_algebra()) {
				result += ALG;
				result += token.data();
			}
			else if (token.is_comma()) {
				result += PAREN();
				result += ",";
			}
			else if (token.is_function()) {
				paren_comma_stack.push(token);
				result += PAREN();
				result += token.data();
				result += "(";
			}
			else if (token.is_operator()) {
				result += OPERA;
				result += token.data();
			}
			else if (token.is_leftparen()) {
				paren_comma_stack.push(token);
				result += PAREN();
				result += token.data();
			}
			else if (token.is_rightparen()) {
				result += PAREN();
				result += token.data();
				if (paren_comma_stack.top().is_leftparen() || paren_comma_stack.top().is_function()) {
					paren_comma_stack.pop();
				}
			}
			else {
				result += UNKNOWN;
				result += token.data();
			}
		}
		result += UNKNOWN;
		return result;
		};
	auto color = [&](const std::string& str)->std::string {
		if (!ColorOpen) return str;
		return color_tokens(ExpressionParser::tokenize(str, *this));
		};
	auto color_string = [&](const std::string& str, const std::string& clip = "$") {
		std::string result;
		int i = 0;
		for (auto& s : split_str(str, clip)) {
			result += i % 2 == 0 ? s : color(s);
			++i;
		}
		return result;
		};
	try {
		if (!is_balanced_paren(cmd)) {
			err_Expr_ParenUnbalanced();
		}
		auto tokens = ExpressionParser::tokenize(cmd, *this);
		if (tokens.size() == 1 && tokens[0].data() == "help") {
			return color_string("1.$help$ 帮助\n"
				"2.直接输入表达式 将返回计算结果\n"
				"3.$x=1+2+3$ 设置或者创建一个变量x\n"
				"4.$x$将返回x的值\n"
				"5.$f(x)$将返回函数值\n"
				"6.$f(x)=x^2+2*x+1$ 将设置一个函数\n"
				"7.$f()$ 返回函数f的定义(参数和表达式)\n"
				"8.$del f()$将删除一个函数\n"
				"9.$del a$将删除一个变量\n"
				"10.$list_func$将列出所有已经有的函数和运算符\n"
				"11.$list_alg$将列出所有的变量\n"
				"12.$list$将列出所有已经有的函数和运算符和变量\n"
				"13.$for <i> <begin> <end> <step> <expr>$\n"
				"循环:  $i$表示迭代变量 $step$表示步长 $expr$表示表达式 $i$在[$begin, end$]区间迭代 并输出每次迭代的结果\n"
				"14.$sum <i> <begin> <end> <step> <expr> [out]$\n"
				"求和 : $i$表示迭代变量 $step$表示步长 $expr$表示表达式 $i$在[$begin, end$]区间迭代 并输出表达式之和 $out$可选可不选 表示输出到哪个变量\n"
				"15.$rename <old_name> <new_name>$ 重命名变量");

		}
		//list_func
		if (tokens.size() == 1 && tokens[0].data() == "list_func") {
			return getFunctions();
		}
		//list_alg
		else if (tokens.size() == 1 && tokens[0].data() == "list_alg") {
			return color(getAllVal());
		}
		//list
		else if (tokens.size() == 1 && tokens[0].data() == "list") {
			return getFunctions() + "," + color(getAllVal());
		}
		//输入单个数字
		else if (tokens.size() == 1 && tokens[0].is_num()) {
			return color(doubleTostring(tokens[0].n_data()));
		}
		//输入一个代数
		else if (tokens.size() == 1 && tokens[0].is_algebra()) {
			return color(doubleTostring(getVal(tokens[0].data())));
		}
		//for和sum
		else if (tokens.size() >= 11 && tokens[0].is_algebra() && (tokens[0].data() == "for" || tokens[0].data() == "sum")) {
			if (tokens[1].is_space() && tokens[2].is_algebra()
				&& tokens[3].is_space() && tokens[4].is_num()
				&& tokens[5].is_space() && tokens[6].is_num()
				&& tokens[7].is_space() && tokens[8].is_num()
				&& tokens[9].is_space()) {
				double begin = tokens[4].n_data();
				double end = tokens[6].n_data();
				double step = tokens[8].n_data();
				double sum = 0;
				bool isFor = (tokens[0].data() == "for") ? true : false;
				bool isSum = (tokens[0].data() == "sum") ? true : false;
				if (step == 0 || (begin < end && step < 0) || (begin > end && step > 0)) {
					return "It is not allowed the situation that step == 0, begin < end  step < 0, begin > end && step > 0.";
				}
				//尾巴比开头大就置换
				if (begin > end) {
					double temp = end;
					end = begin;
					begin = temp;
					step = -step;
				}
				std::vector<Token> expr_tokens;
				if (isFor) {
					expr_tokens.resize(tokens.size() - 10);
					std::copy(tokens.begin() + 10, tokens.end(), expr_tokens.begin());
				}
				else if (isSum) {
					if (tokens.size() > 11 && tokens[tokens.size() - 2].is_space() && tokens.back().is_algebra()) {
						expr_tokens.resize(tokens.size() - 12);
						std::copy(tokens.begin() + 10, tokens.end() - 2, expr_tokens.begin());
					}
					else {
						expr_tokens.resize(tokens.size() - 10);
						std::copy(tokens.begin() + 10, tokens.end(), expr_tokens.begin());
					}
				}
				std::string cmd_result;
				std::string org_alg_name = tokens[2].data();
				//为了迭代变量的变量不会与外界的同名的变量混淆 因重命名为一个随机变量名
				std::string rand_i_name = randName();
				std::string expression_str = color_tokens(expr_tokens);
				for (auto& t : expr_tokens) {
					if (t.is_algebra() && t.data() == tokens[2].data()) {
						t.set_s_data(rand_i_name);
					}
				}
				auto postfix_expression_tokens = ExpressionParser::infixVec_to_postfixVec(expr_tokens, *this);
				createVal(rand_i_name);
				for (double i = begin; i <= end + EPSILON; i += step) {
					setVal(rand_i_name, i);
					if (isFor) {
						cmd_result += color(org_alg_name + "=" + doubleTostring(i)) + "\t" + expression_str + color("=" + doubleTostring(ExpressionParser::postfixVec_calculate(postfix_expression_tokens, *this))) + "\n";
					}
					if (isSum) {
						sum += ExpressionParser::postfixVec_calculate(postfix_expression_tokens, *this);
					}
				}
				deleteVal(rand_i_name);
				if (isFor) {
					return cmd_result;
				}
				else if (isSum) {
					if (tokens.size() > 11 && tokens[tokens.size() - 2].is_space() && tokens.back().is_algebra()) {
						setVal(tokens.back().data(), sum);
						return tokens.back().data() + "=" + doubleTostring(sum);
					}
					else {
						return doubleTostring(sum);
					}
				}
				else {
					return "Unknown command.";
				}
			}
			else {
				return "Unknown command.";
			}
		}
		//f()
		else if (tokens.size() == 2 && tokens[0].is_function() && tokens[1].is_rightparen()) {
			auto* f = get_function(tokens[0].data());
			if (f) {
				std::string param_str;
				//数学函数
				if (typeid(*f) == typeid(FxSymbol)) {
					for (auto& name : dynamic_cast<FxSymbol*>(f)->get_args_name()) {
						param_str += name + ",";
					}
					if (!param_str.empty()) {
						param_str.erase(param_str.size() - 1);
					}
					return color(f->ID() + "(" + param_str + ")=" + dynamic_cast<FxSymbol*>(f)->get_expression());
				}
				//lambda设定函数
				else {
					for (size_t i = 0; i < f->ParamNum(); ++i) {
						param_str += (i > 25) ? (std::string(1, i / 26 + 'a' - 1) + char(i % 26+'a')) : std::string(1, char(i + 'a'));
						param_str += ",";
					}
					if (param_str.size() > 0) {
						param_str.erase(param_str.size() - 1);
					}
					return color(f->ID() + "(" + param_str + ")");
				}
			}
			else {
				return "Function \"" + tokens[0].data() + "\" is not defined.";
			}
		}
		//del
		else if (tokens.size() >= 3 && tokens[0].data() == "del" && tokens[1].is_space()) {
			//del f()
			if (tokens.size() == 4 && tokens[2].is_function() && tokens[3].is_rightparen()) {
				deleteFxAndFunc(tokens[2].data());
				return color(tokens[2].data()+"()") + " was deleted.";
			}
			//del a
			else if (tokens.size() == 3 && tokens[2].is_algebra()) {
				deleteVal(tokens[2].data());
				return color(tokens[2].data()) + " was deleted.";
			}
			else {
				return "mistake delete format";
			}
		}
		//rename
		else if (tokens.size() == 5 && tokens[0].data() == "rename" && tokens[1].is_space() && tokens[2].is_algebra() && tokens[3].is_space() && tokens[4].is_algebra()) {
			reValname(tokens[2].data(), tokens[4].data());
			return tokens[2].data() + " has been renamed to " + tokens[4].data();
		}
		//a=x f(x)=x
		else if (tokens.size() >= 3) {
			//第一个等于运算符出现的位置 用于观察它是否出现两次
			size_t first_equal_symbol_index = 0;
			//检查等号的出现
			for (int i = 0; i < tokens.size(); ++i) {
				if (tokens[i].data() == "=") {
					if (first_equal_symbol_index == 0) {
						first_equal_symbol_index = i;
						if (i == 0) {
							return "mistake command format, \"=\" has not meaning.";
						}
					}
					else {
						//出现两次则会报错
						return "mistake command format with more than one \"=\"";
					}
				}
			}
			//如果有等号
			if (first_equal_symbol_index != 0) {
				//前部分的token (变量 函数)
				std::vector<Token> front_tokens(first_equal_symbol_index);
				std::copy(tokens.begin(), tokens.begin() + first_equal_symbol_index, front_tokens.begin());
				//原先的token只剩下后半部分
				tokens.erase(tokens.begin(), tokens.begin() + first_equal_symbol_index + 1);

				//a=xxxx
				if (front_tokens.size() == 1) {

					double n = ExpressionParser::calculate(tokens, *this);
					createVal(front_tokens[0].data(), n);
					return color(front_tokens[0].data() + "=" + doubleTostring(ExpressionParser::calculate(tokens, *this)));
				}
				//f(x)=xxxx
				if (front_tokens.size() >= 2 && front_tokens[0].is_function() && front_tokens.back().is_rightparen()) {
					std::vector<std::string> params_name;
					for (int i = 1; i < front_tokens.size() - 1; ++i) {
						auto& cur = front_tokens[i];
						if (i % 2 == 1) {
							if (cur.is_algebra()) {
								params_name.push_back(cur.data());
							}
							else {
								return "mistake function format";
							}
						}
						else if (i % 2 == 0 && !cur.is_comma()) {
							return "mistake function format";
						}

					}
					createFx(front_tokens[0].data(), cmd.substr(cmd.find('=') + 1), params_name);
					return "created " + color(cmd) + ".";
				}
				return "";
			}
			return color(doubleTostring(ExpressionParser::calculate(tokens, *this)));
		}
		else {
			return doubleTostring(ExpressionParser::calculate(cmd, *this));
		}
	}
	catch (const std::exception& e) {
		return (ColorOpen ? "\033[31m" : "") + ("[ERROR] " + std::string(e.what())) + (ColorOpen ? "\033[0m" : "");
	}
}
double Calculation::calculate(const std::string& expression)
{
	return ExpressionParser::calculate(expression, *this);
}
void Calculation::addFunction(FunBaseSymbol* fp)
{
	funs_and_operators.push_back(fp);
}
