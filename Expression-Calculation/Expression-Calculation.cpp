#include <iostream>
#include "Calculation.h"
//判断字符串是否是ascii范围
bool isAllAscii(const std::string& s)
{
    for (char ch : s)
    {
        unsigned char c = static_cast<unsigned char>(ch);
        if (c > 127)
            return false;
    }
    return true;
}
int main()
{
    Calculation calc;
    //创建函数 
    calc.createFx("f", "10000*x+1000*y+100*z+10*w+k", { "x","y", "z","w", "k" });
    //创建变量a
    calc.createVal("a");
    //创建函数
    calc.addFunction("sum", 3, [](Args_ args) {return args[0] + args[1] + args[2];});
    //把变量a重命名为b
    calc.reValname("a", "b");
    std::cout << calc.command("help") << "\n";
    std::string cmd = "";
    while (std::cin.good()) {
        std::cout << "\033[95m>>>\033[0m ";
        std::getline(std::cin, cmd);
        if (cmd == "cls") {
            system("cls");
            continue;
        }
        if (!isAllAscii(cmd)) {
            std::cout << "\033[31m只支持ASCII范围内的字符\033[0m" << "\n";
            continue;
        };
        std::cout << calc.command(cmd).c_str() << '\n';
    }
}