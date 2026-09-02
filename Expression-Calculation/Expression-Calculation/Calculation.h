#pragma once 
#include <exception>
#include <string>
#include <vector>
#include <functional>
typedef const std::vector<double>& Args_;
typedef unsigned short param_num;
class BaseSymbol;
class OperatorBaseSymbol;
class PreFixOperatorSymbol;
class BinaryOperatorSymbol;
class ICalculation {
public:
	virtual void createVal(const std::string& name, double default_num) = 0;
	virtual void deleteVal(const std::string& name) = 0;
	virtual double getVal(const std::string& name) const = 0;
	virtual const std::vector<BaseSymbol*>& get_funs_and_operators() const = 0;
};
enum class TokenType
{
	NUM, //数字
	ALGEBRA, //代数
	LPAREN, //(
	RPAREN, //)
	BINO, //二元运算符
	PREO, //后缀一元运算符
	UNKNOWNO,//分不清是二元运算符还是后缀一元运算符
	FUNC, //函数
	COMMA, //逗号
	SPACE, //空格
	UNKNOWN//未知
};
//最小可识别字段
class Token {
private:
	std::string str_t;//如果Token表示非数字 它将
	double num_t = 0.0;//如果Token是数字 它将存储数字的值
	OperatorBaseSymbol* oper_ptr = nullptr;
public:
	TokenType type_t = TokenType::UNKNOWN;
	Token();
	Token(std::string str);
	Token(std::string str, OperatorBaseSymbol* op);
	Token(double num);
	Token(const Token& t);
	double n_data() const;
	std::string s_data() const;
	std::string data() const;
	//是否是普通数组
	bool is_num() const;
	//是否是代数
	bool is_algebra() const;
	//是否是代数或者数字
	bool is_alg_num() const;
	//是否是左括号
	bool is_leftparen() const;
	//是否是右括号
	bool is_rightparen() const;
	//是否是二元运算符
	bool is_binary_operator() const;
	//是否是一元前缀运算符
	bool is_prefix_operator() const;
	//是否是运算符
	bool is_operator() const;
	//是否是逗号
	bool is_comma() const;
	//是否是函数
	bool is_function() const;
	//是否是空格
	bool is_space() const;
	//是否是未知的运算符
	bool is_unknown_operator() const;
	//这个Token在某个运算符Token的左边 能否有数字的意义
	bool is_ableToNum_at_left() const;
	//这个Token在某个运算符Token的右边 能否有数字的意义
	bool is_ableToNum_at_right() const;
	//重新设置存的字符串数据
	void set_s_data(const std::string d);
	//如果是二元运算符 返回其对应的指针
	BinaryOperatorSymbol* get_binary_operator() const;
	//如果是一元前缀运算符 返回其对应的指针
	PreFixOperatorSymbol* get_prefix_operator() const;
	//设置运算符指针
	void set_operator(OperatorBaseSymbol* op);
	//返回这个Token的类型字符串表示
	std::string TypeS() const;
	Token& operator=(const Token& t);
	friend std::ostream& operator<<(std::ostream& os, const Token& t);

};
//表达式异常
class ExpressionException : public std::exception {
private:
	std::string msg;
public:
	ExpressionException(const std::string& _Message);
	char const* what() const noexcept override;
};
//运算符和函数的基类
class BaseSymbol {
private:
	std::string id_;//id 指可识别的运算符或函数的标签
protected:
	std::function<double(const std::vector<double>&)> func_;//传参数执行计算结果的核心函数
public:
	BaseSymbol(const std::string& id);
	BaseSymbol() = delete;
	BaseSymbol& operator=(const BaseSymbol& bs) = delete;
	BaseSymbol(const BaseSymbol& bs) = delete;
	std::string ID() const;
	//传参的接口
	virtual double evaluate(Args_ args) const = 0;
	//通过ID在BaseSymbol列表返回对应基类类型的BaseSymbol
	template<typename Symbol>
	static Symbol* getSymbol(const std::string& name, const std::vector<BaseSymbol*>& funs_and_operators);
};
template<typename Symbol>
inline Symbol* BaseSymbol::getSymbol(const std::string& name, const std::vector<BaseSymbol*>& funs_and_operators)
{
	for (auto* item : funs_and_operators) {
		if (dynamic_cast<Symbol*>(item) && item->ID() == name) {
			return dynamic_cast<Symbol*>(item);
		}
	}
	return nullptr;
}
//运算符的基类
class OperatorBaseSymbol : public BaseSymbol {
public:
	OperatorBaseSymbol(const std::string& id, std::function<double(Args_)> func);
	virtual double evaluate(Args_ args) const = 0;
};
//函数的基类
class FunBaseSymbol : public BaseSymbol {
private:
	param_num args_num_;
public:
	FunBaseSymbol(const std::string& id, param_num args_num);
	unsigned short ParamNum() const;
	virtual double evaluate(Args_ args) const = 0;
};
//前缀一元运算符
class PreFixOperatorSymbol : public OperatorBaseSymbol {
public:
	PreFixOperatorSymbol(const std::string& id, double (*func)(double value));
	double evaluate(Args_ args) const override;
};
//二元运算符
class BinaryOperatorSymbol : public OperatorBaseSymbol {
private:
	unsigned short priority_;
public:
	BinaryOperatorSymbol(const std::string& id, unsigned short priority, double (*func)(double left, double right));
	size_t priority() const;
	double evaluate(Args_ args) const override;
};
//函数
class FunctionSymbol : public FunBaseSymbol {
public:
	//普通函数的初始化
	FunctionSymbol(const std::string& id, unsigned short args_num, double(*func)(Args_ args) );
	double evaluate(Args_ args) const override;
};
//数学意义上的函数
class FxSymbol : public FunBaseSymbol {
private:
	ICalculation& i_calc_;
	std::string expr_;//存储初始化的表达式
	std::vector<std::string> default_args_name_;//存储初始化的时候参数名称
	std::vector<std::string> obfuscated_args_name_;//存储已经混淆过的参数名称
	std::vector<Token> post_expr_tokens_;//存储后缀表达式
public:
	FxSymbol(const std::string& id, const std::vector<std::string>& args_name,
		const std::string& fxexpress, ICalculation& i_calc);
	double evaluate(Args_ args) const override;
	const std::string& get_expression() const;
	const std::vector<std::string>& get_args_name() const;
};
//代数
struct Algebra {
	std::string id;
	double value = 0;
};
//表达式解析器
class ExpressionParser {
private:
	//判断某个字符是否是某个二元运算符的第一个字符
	static bool isBinaryOperatorFirstCh(char c, ICalculation& i_calc);
	//判断某个字符是否是某个运算符的第一个字符
	static bool isHavedOperatorFirstCh(char c, ICalculation& i_calc);
	//判断某个运算符是否是已经注册的二元运算符
	static bool isHavedBinaryOperator(const std::string& c, ICalculation& i_calc);
	//判断某个运算符是否是已经注册的前缀一元运算符
	static bool isHavedPrefixOperator(const std::string& c, ICalculation& i_calc);
	//判断某个函数名是否是已经注册的
	static bool isHavedFunction(const std::string& c, ICalculation& i_calc);
	//判断某个运算符是不是既属于二元运算符也属于一元运算符
	static bool isChCommonAtBinAndPre(const std::string& c, ICalculation& i_calc);
public:
	//把表达式拆分成Token
	static std::vector<Token> tokenize(const std::string& expression, ICalculation& i_calc);
	//获取二元运算符的指针
	static BinaryOperatorSymbol* get_binary_operator(const std::string& c, ICalculation& i_calc);
	//获取一元前缀运算符的指针
	static PreFixOperatorSymbol* get_prefix_operator(const std::string& c, ICalculation& i_calc);
	//获取函数的指针
	static FunBaseSymbol* get_function(const std::string& c, ICalculation& i_calc);
	//通过后缀表达式计算出结果
	static double postfixVec_calculate(const std::vector<Token>& tokens, ICalculation& i_calc);
	//中缀表达式转后缀表达式 返回token数组
	static std::vector<Token> infixVec_to_postfixVec(const std::vector<Token>& expr_tokens, ICalculation& i_calc);
	static std::vector<Token> infix_to_postfixVec(const std::string& expr, ICalculation& i_calc);
	static double calculate(const std::string& expr, ICalculation& i_calc);
	static double calculate(const std::vector<Token>& expr_tokens, ICalculation& i_calc);
};
//计算器主体
class Calculation : public ICalculation {
private:
	//存放已经注册的函数和运算符
	std::vector<BaseSymbol*> funs_and_operators;
	//存放代数符号
	std::vector<Algebra*> algebras;
	std::string getFunctions() const;
	//获取函数指针
	FunBaseSymbol* get_function(const std::string& id) const;
	//默认运算符 函数的注册
	void default_registFuncAndOpera();
	const std::vector<BaseSymbol*>& get_funs_and_operators() const override;
	void addBinOperation(const std::string& id, unsigned short pirority, double (*func)(double left, double right));
	void addPreOperation(const std::string& id, double (*func)(double value));
	void addFunction(FunBaseSymbol* fp);
public:
	Calculation();
	~Calculation();
	/**
	* @brief 按指令的方式执行表达式
	* @param cmd[in] 格式如下:
	* 0.help 帮助
	* 1.help 帮助
	* 2.直接输入表达式 将返回计算结果
	* 3.x=1+2+3 设置或者创建一个变量x
	* 4.x将返回x的值
	* 5.f(x)将返回函数值
	* 6.f(x)=x^2+2*x+1 将设置一个函数
	* 7.f() 返回函数f的定义(参数和表达式)
	* 8.del f()将删除一个函数
	* 9.del a将删除一个变量
	* 10.list_func将列出所有已经有的函数和运算符
	* 11.list_alg将列出所有的变量
	* 12.list将列出所有已经有的函数和运算符和变量
	* 13.for <i> <begin> <end> <step> <expr>
	* 循环:  i表示迭代变量 step表示步长 expr表示表达式 i在[begin, end]区间迭代 并输出每次迭代的结果
	* 14.sum <i> <begin> <end> <step> <expr> out
	* 求和 : i表示迭代变量 step表示步长 expr表示表达式 i在[begin, end]区间迭代 并输出表达式之和 out可选可不选 表示输出到哪个变量
	* 15.rename <old_name> <new_name> 重命名变量
	*/
	std::string command(const std::string& cmd, bool ColorOpen = true) noexcept;
	//计算一个表达式
	double calculate(const std::string& expression);
	//添加由自己写代码构成的函数 (不开放注册运算符的接口)
	void addFunction(const std::string& id, unsigned short param_num, double (*func)(Args_ args));
	//创建一个代数
	void createVal(const std::string& name, double default_num = 0.0) override;
	//获取某个代数的值
	double getVal(const std::string& name) const override;
	//设置代数的值
	void setVal(const std::string& name, double set_num);
	//删除变量
	void deleteVal(const std::string& name) override;
	//重置变量名
	void reValname(const std::string& old_name, const std::string& new_name);
	//获取已有的变量
	std::string getAllVal() const;
	//创建一个数学意义上的函数
	/**
	* @param name[in] 函数名称
	* @param expr[in] 函数表达式
	* @param args_name[in] 参数名集合
	*/
	void createFx(const std::string& name, std::string expr, const std::vector<std::string>& args_name);
	//获取函数值
	double getFxVal(const std::string& name, Args_ args) const;
	template <typename... Args>
	double getFxVal(const std::string& name, Args... args) const {
		Args_ nums = { args... };
		return getFxVal(name, nums);
	}
	//设置函数表达式
	void setFx(const std::string& name, const std::string& expr, const std::vector<std::string>& args_name);
	//删除函数
	void deleteFxAndFunc(const std::string& name);
};

