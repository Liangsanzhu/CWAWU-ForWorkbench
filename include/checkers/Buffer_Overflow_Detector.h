#ifndef B_O_F_H
#define B_O_F_H
#include "Detector.h"

class Buffer_Overflow
{
private:
    //void handleDeclStmt(const Stmt *S);                     //处理DeclStmt
    //void handleCallExpr(const Stmt *S);                     //处理CallExpr
    //int strLen(string type);                                //得到数组变量char[]的长度
    //void detect_bcopy(const Stmt *S, FunctionDecl *callee); //检测bcopy()函数

    map<std::string, int> Var; //[变量名，变量长度] or [变量名，变量长度]
    map<int, int> Format_param;
    map<int, SourceLocation> FuncLocation;
    int ifindex;
    defuse_node *all_node;

    struct var_info
    {
        int var_ID;
        string name;
        string type;
        int length;
        bool cinit;
        int value_int;
        string value_array;
    };
    map<std::string, var_info> Decl_Var; //[变量名，变量信息]

    struct bof_error
    {
        string filename;
        int line;
        int col;
        string info;
    };
    map<int, bof_error> bof_info;

    int strLen(string type)
    {
        string t;
        int len = 0;
        for (int i = 0; i < 6; i++)
            t = t + type[i];

        if (t == "char [")
            for (int i = 6; type[i] != ']'; i++)
                len = len * 10 + type[i] - '0';
        else
            len = -1;
        return len;
    }

    void handleDeclStmt(const Stmt *S)
    {
        DeclStmt *declstmt = (DeclStmt *)S;

        auto it = declstmt->decl_begin();
        VarDecl *vdecl = static_cast<VarDecl *>(*it);

        string var_tpye = vdecl->getType().getAsString();           //变量类型
        string var_name = vdecl->getIdentifier()->getName().data(); //变量名

        var_info temp_info;
        temp_info.var_ID = vdecl->getID();
        temp_info.name = vdecl->getIdentifier()->getName().data(); //变量名
        temp_info.type = vdecl->getType().getAsString();           //变量类型
        temp_info.cinit = vdecl->hasInit();                        //是否初始化

        int len = strLen(var_tpye);
        temp_info.length = len; //变量长度

        for (auto i = S->child_begin(); i != S->child_end(); i++)
        {
            //std::cout << "StmtClass:  " << i->getStmtClassName() << std::endl;
            if (strcmp(i->getStmtClassName(), "IntegerLiteral") == 0) //整型变量
            {
                IntegerLiteral *intltr = (IntegerLiteral *)*i;
                int int_val = intltr->getValue().getSExtValue();
                if (temp_info.type == "int" || temp_info.type == "long" || temp_info.type == "short" || temp_info.type == "unsigned")
                {
                    temp_info.value_int = int_val;
                    temp_info.length = int_val;
                    temp_info.value_array = "";
                    //cout << "var value: " << int_val << endl;
                }
                //Var[var_name] = int_val;
                //cout << var_name << "=" << Var[var_name] << endl;
            }
            else if (strcmp(i->getStmtClassName(), "StringLiteral") == 0)
            {
                StringLiteral *strltr = (StringLiteral *)*i;
                string str_value = strltr->getString().data();
                temp_info.value_array = str_value;
                temp_info.value_int = -1;
                //cout << "string data: " << strltr->getString().data() << endl;
            }
            else if (strcmp(i->getStmtClassName(), "CStyleCastExpr") == 0)
            {
                auto j = i->child_begin();
                if (strcmp(j->getStmtClassName(), "CallExpr") == 0)
                {
                    bool ju = isCallFunc(*j);
                    if (ju)
                    {
                        CallExpr *callexpr = (CallExpr *)*j;
                        FunctionDecl *callee = callexpr->getDirectCallee();
                        string callee_name = callee->getNameAsString();
                        if (callee_name == "malloc")
                        {
                            ArrayRef<Stmt *> s = callexpr->getRawSubExprs();
                            for (int i = 0; i < s.size(); i++)
                            {
                                //cout << s[i]->getStmtClassName() << endl;
                                char const *temp_name = s[i]->getStmtClassName();
                                if (i >= 1)
                                {
                                    if (strcmp(temp_name, "ImplicitCastExpr") == 0)
                                    {
                                        auto k = s[i]->child_begin();
                                        if (strcmp(k->getStmtClassName(), "IntegerLiteral") == 0)
                                        {
                                            IntegerLiteral *v_int = (IntegerLiteral *)*k;
                                            ////!!!
                                            if (temp_info.length == -1)
                                                temp_info.length = v_int->getValue().getSExtValue();
                                        }
                                    }
                                    else if (strcmp(temp_name, "BinaryOperator") == 0)
                                    {
                                        const Stmt *t = s[i];
                                        int res = countBinaryOperator(t);
                                        //cout << "res=         " << res << endl;
                                        temp_info.length = res;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                temp_info.length = -1;
            }
        }

        //cout << "var tpye:   " << temp_info.type << endl;
        //cout << "var name:   " << temp_info.name << endl;
        //cout << "var len= " << temp_info.length << endl;
        //cout << "var int value=   " << temp_info.value_int << endl;
        //cout << "var str value=   " << temp_info.value_array << endl;
        Decl_Var.insert(pair<string, var_info>(temp_info.name, temp_info)); //插入[变量名，变量信息]
        Decl_Var[temp_info.name] = temp_info;
        //cout <<"Decl_var ID:    "<<Decl_Var[temp_info.name].var_ID<< "Decl_var:    " << Decl_Var[temp_info.name].name << "     " << Decl_Var[temp_info.name].length << "   " << Decl_Var[temp_info.name].type << endl;
        //cout << endl;
    }

    void handleCallExpr(const Stmt *S)
    {
        CallExpr *callexpr = (CallExpr *)S;
        FunctionDecl *callee = callexpr->getDirectCallee();
        string callee_name = callee->getNameAsString();

        if (callee_name == "bcopy" || callee_name == "memcpy" || callee_name == "memset" || callee_name == "strncpy" || callee_name == "strcpy")
        {

            //std::cout << "callee name:  " << callee_name << std::endl;
            int callee_num = FuncLocation.size();
            FuncLocation.insert(pair<int, SourceLocation>(callee_num, S->getBeginLoc()));
            detect_bcopy(S, callee, callee_num); //检测bcopy()函数
        }
        if (callee_name == "sprintf")
        {
            //std::cout << "callee name:  " << callee_name << std::endl;
            int callee_num = FuncLocation.size();
            FuncLocation.insert(pair<int, SourceLocation>(callee_num, S->getBeginLoc()));
            detect_sprintf(S, callee, callee_num);
        }
        if (callee_name == "scanf")
        {
            //std::cout << "callee name:  " << callee_name << std::endl;
            int callee_num = FuncLocation.size();
            FuncLocation.insert(pair<int, SourceLocation>(callee_num, S->getBeginLoc()));
            detect_scanf(S, callee, callee_num);
        }
        if (callee_name == "snprintf")
        {
            //std::cout << "callee name:  " << callee_name << std::endl;
            int callee_num = FuncLocation.size();
            FuncLocation.insert(pair<int, SourceLocation>(callee_num, S->getBeginLoc()));
            detect_snprintf(S, callee, callee_num);
        }
    }

    void detect_bcopy(const Stmt *S, FunctionDecl *callee, int callee_num)
    {
        //3个参数的长度
        int len_dest = -1;
        int len_src = -1;
        int len_n = -1;
        //记录3个参数的顺序
        int param_order[3]; //0-src,1-dest,2-n
        for (int i = 0; i < 3; i++)
            param_order[i] = -1;

        /*****确定被调用函数中参数的顺序******/
        int num = 1;
        for (auto param = callee->param_begin(); param != callee->param_end(); param++)
        {
            string param_name = (*param)->getIdentifier()->getName().data();
            //cout << "!!!----------------param name:   " << param_name << endl;
            //std::cout << "!!!-------------param type:   " << (*param)->getType().getAsString() << std::endl;
            if (param_name == "__src" || param_name == "__c") //源参数
                param_order[0] = num;
            else if (param_name == "__dest" || param_name == "__s") //目的参数
                param_order[1] = num;
            else if (param_name == "__n") //长度
                param_order[2] = num;
            num++;
        }
        //cout << param_order[0] << " " << param_order[1] << " " << param_order[2] << endl;
        //cout << endl;

        /*****处理并记录各个参数的长度*****/
        num = 0;
        string Func_name = callee->getNameAsString();
        //auto i = S->child_begin();
        for (auto i = S->child_begin(); i != S->child_end(); i++)
        {
            auto j = i;
            if (Func_name != "memset" || num != 2)
            {
                string param_name;
                string param_type;
                int param_ID;
                while (true)
                {

                    if (strcmp(j->getStmtClassName(), "DeclRefExpr") == 0)
                    {
                        DeclRefExpr *declre = (DeclRefExpr *)*j;
                        param_name = declre->getNameInfo().getAsString();
                        param_type = declre->getType().getAsString();
                        break;
                    }
                    else if (strcmp(j->getStmtClassName(), "IntegerLiteral") == 0)
                    {
                        IntegerLiteral *IntLitrl = (IntegerLiteral *)*j;
                        int temp_len = IntLitrl->getValue().getSExtValue();
                        if (num == 3)
                            len_n = temp_len;
                        break;
                    }
                    else if (strcmp(j->getStmtClassName(), "BinaryOperator") == 0)
                    {
                        int res = countBinaryOperator(*j);
                        //cout << "countBinaryOperator:res=     " << res << endl;
                        len_n = res;
                        break;
                    }
                    else if (strcmp(j->getStmtClassName(), "UnaryExprOrTypeTraitExpr") == 0)
                    {
                        UnaryExprOrTypeTraitExpr *Ueotte = (UnaryExprOrTypeTraitExpr *)*j;
                        //
                        len_n = -2;
                        break;
                    }
                    else if (j->child_begin() == j->child_end())
                    {
                        break;
                    }
                    j = j->child_begin();
                    //cout << "j: " << j->getStmtClassName() << endl;
                }

                //cout << "param name:  " << param_name << endl;
                //cout << "param type:  " << param_type << endl;
                //cout << "num=" << num << endl;
                //cout << Decl_Var[param_name].name << " " << Decl_Var[param_name].length << endl;

                if (num == param_order[1]) //如果是目标参数
                {
                    len_dest = Decl_Var[param_name].length;
                    //cout << "len_dest=" << len_dest << endl;
                }
                else if (num == param_order[2]) //如果是长度参数
                {
                    if (len_n == -1)
                        len_n = Decl_Var[param_name].length;
                    //cout << "len_n=" << len_n << endl;
                }
                else if (num == param_order[0]) //如果是源参数
                {
                    if (Func_name != "memset")
                        len_src = Decl_Var[param_name].length;
                }
            }
            num++;
        }
        if (param_order[2] == -1) //该函数中没有长度参数n，将len_n设为源参数的长度
        {
            len_n = len_src;
        }

        /*****根据参数长度判断是否error或warning*****/

        //cout << "len_dest:    " << len_dest << "  len_n:  " << len_n << endl;
        if (len_n == -1) //长度参数n不确定
        {
            //warning:maybe right or wrong
        }
        else if (len_n > 0 && len_dest > 0)
        {
            if (len_dest < len_n) //目的参数长度<比长度参数n
            {
                clang::SourceManager &srcmgr = callee->getASTContext().getSourceManager();
                //error
                //cout << "error:" << endl;
                string Func_name = callee->getNameAsString();

                bof_error tmp_info;
                tmp_info.filename = srcmgr.getFilename(FuncLocation[callee_num]).str();
                tmp_info.line = srcmgr.getSpellingLineNumber(FuncLocation[callee_num]);
                tmp_info.col = srcmgr.getSpellingColumnNumber(FuncLocation[callee_num]);
                tmp_info.info = "buffer overflow caused by use " + Func_name + " to copy data";

                int tmp_num = bof_info.size();
                bof_info.insert(pair<int, bof_error>(tmp_num, tmp_info));

                //error_info *e = new_error_info(NULL, filename, line, col, TYPE_ERROR, info, ifindex);
                //result.push(e);
            }
            else
            {
                //no error or warning
            }
        }
    }

    void detect_sprintf(const Stmt *S, FunctionDecl *callee, int callee_num)
    {
        int len_dest = -1;
        int len_src = -1;
        int len_n = -1;
        string format_str;
        int num = 0;
        auto i = S->child_begin();
        while (i != S->child_end())
        {
            auto j = i;
            while (true)
            {
                //cout << "j: " << j->getStmtClassName() << endl;
                if (strcmp(j->getStmtClassName(), "DeclRefExpr") == 0)
                {
                    DeclRefExpr *declre = (DeclRefExpr *)*j;
                    string param_name = declre->getNameInfo().getAsString();
                    string param_type = declre->getType().getAsString();
                    if (num >= 3)
                    {
                        int temp = Var[param_name];
                        if (temp == 0)
                            temp = -1;
                        Format_param.insert(pair<int, int>(num - 3, temp));
                        Format_param[num - 3] = temp;
                    }
                    if (num == 1)
                    {
                        len_dest = Var[param_name];
                        //cout << "len_dest:  " << len_dest << endl;
                    }
                    //cout << "param name:  " << param_name << endl;
                    //cout << "param type:  " << param_type << endl;
                    break;
                }
                else if (strcmp(j->getStmtClassName(), "StringLiteral") == 0)
                {
                    StringLiteral *strltr = (StringLiteral *)*j;
                    if (num <= 2)
                    {
                        format_str = strltr->getString().str();
                        //cout << "str: " << format_str << endl;
                    }
                    else
                    {
                        string temp_str = strltr->getString().str();
                        Format_param.insert(pair<int, int>(num - 3, temp_str.size()));
                        Format_param[num - 3] = temp_str.size();
                        //cout << "str: " << temp_str << endl;
                    }
                    break;
                }
                else if (strcmp(j->getStmtClassName(), "IntegerLiteral") == 0)
                {
                    IntegerLiteral *intltr = (IntegerLiteral *)*j;
                    int temp_int = intltr->getValue().getSExtValue();
                    Format_param.insert(pair<int, int>(num - 3, temp_int));
                    Format_param[num - 3] = temp_int;
                    //cout << "int: " << temp_int << endl;
                    break;
                }
                else if (j->child_begin() == nullptr)
                {
                    //cout << "none child" << endl;
                    break;
                }
                else
                    j = j->child_begin();
            }
            //cout << "num=" << num << endl;

            i++;
            num++;
            //cout << endl;
        }

        int dLen;
        int totalLen = count_totalLen(format_str, dLen);

        //cout << "len_dest" << len_dest << endl;
        //cout << "totalLen" << totalLen << endl;
        if (len_dest <= totalLen)
        {
            clang::SourceManager &srcmgr = callee->getASTContext().getSourceManager();
            //error
            //cout << "\033[32m error:there is a wrong \033[0m" << endl;
            //cout << "error:" << endl;
            string Func_name = callee->getNameAsString();

            bof_error tmp_info;
            tmp_info.filename = srcmgr.getFilename(FuncLocation[callee_num]).str();
            tmp_info.line = srcmgr.getSpellingLineNumber(FuncLocation[callee_num]);
            tmp_info.col = srcmgr.getSpellingColumnNumber(FuncLocation[callee_num]);
            tmp_info.info = "buffer overflow caused by use " + Func_name + " to format data";

            int tmp_num = bof_info.size();
            bof_info.insert(pair<int, bof_error>(tmp_num, tmp_info));
        }
        else if (dLen == -1)
        {
            //warning:maybe right or wrong
            //cout << "\033[32m warning:maybe right or wrong \033[0m" << endl;
        }
        else
        {
            //no error or warning
            //cout << "\033[32m no error or warning \033[0m" << endl;
        }
    }

    void detect_scanf(const Stmt *S, FunctionDecl *callee, int callee_num)
    {
        map<int, int> len_dest;
        map<int, int> len_n;
        string format_str;
        int num = 0;
        auto i = S->child_begin();
        while (i != S->child_end())
        {
            auto j = i;

            while (true)
            {
                //cout << "j: " << j->getStmtClassName() << endl;
                if (strcmp(j->getStmtClassName(), "DeclRefExpr") == 0)
                {
                    DeclRefExpr *declre = (DeclRefExpr *)*j;
                    string param_name = declre->getNameInfo().getAsString();
                    string param_type = declre->getType().getAsString();
                    if (num >= 2)
                    {
                        int temp = Var[param_name];
                        if (temp == 0)
                            temp = -1;
                        len_dest.insert(pair<int, int>(num - 2, temp));
                        len_dest[num - 2] = temp;
                    }
                    break;
                }
                else if (strcmp(j->getStmtClassName(), "StringLiteral") == 0)
                {
                    StringLiteral *strltr = (StringLiteral *)*j;
                    if (num <= 1)
                    {
                        format_str = strltr->getString().str();
                        //cout << "str: " << format_str << endl;
                    }
                    break;
                }
                else if (j->child_begin() == nullptr)
                {
                    //cout << "none child" << endl;
                    break;
                }
                else
                    j = j->child_begin();
            }
            //cout << "num=" << num << endl;

            i++;
            num++;
            //cout << endl;
        }

        int cur = 0;
        for (int i = 0; i < format_str.size(); i++)
        {
            if (format_str[i] == '%')
            {
                i++;
                int temp = 0;
                while (format_str[i] <= '9' && format_str[i] >= '0')
                {
                    temp = temp * 10 + format_str[i] - '0';
                    i++;
                }
                if (format_str[i] == 's')
                {
                    len_n.insert(pair<int, int>(cur, temp));
                    len_n[cur] = temp;
                    cur++;
                }
                else
                {
                    temp = -2;
                    len_n.insert(pair<int, int>(cur, temp));
                    len_n[cur] = temp;
                    cur++;
                }
            }
        }

        bool buffover_error = false;
        bool buffover_warnning = false;

        for (int i = 0; i < cur; i++)
        {
            // std::cout << "\033[32m" << i << "----"
            //         << "len_n:" << len_n[i] << "   len_dest" << len_dest[i] << "\033[0m" << std::endl;
            if (len_n[i] != -2)
            {
                if (len_n[i] >= len_dest[i])
                {
                    buffover_error = true;
                }
            }
        }

        if (buffover_error)
        {
            clang::SourceManager &srcmgr = callee->getASTContext().getSourceManager();
            //error
            //cout << "\033[32m error:there is a wrong \033[0m" << endl;
            //cout << "error:" << endl;
            string Func_name = callee->getNameAsString();

            bof_error tmp_info;
            tmp_info.filename = srcmgr.getFilename(FuncLocation[callee_num]).str();
            tmp_info.line = srcmgr.getSpellingLineNumber(FuncLocation[callee_num]);
            tmp_info.col = srcmgr.getSpellingColumnNumber(FuncLocation[callee_num]);
            tmp_info.info = "buffer overflow caused by use " + Func_name + " to format data";

            int tmp_num = bof_info.size();
            bof_info.insert(pair<int, bof_error>(tmp_num, tmp_info));
        }
        else if (buffover_warnning)
        {
            //warning:maybe right or wrong
            //cout << "\033[32m warning:maybe right or wrong \033[0m" << endl;
        }
        else
        {
            //no error or warning
            //cout << "\033[32m no error or warning \033[0m" << endl;
        }
    }

    void detect_snprintf(const Stmt *S, FunctionDecl *callee, int callee_num)
    {
        int len_dest = -1;
        int len_src = -1;
        int len_n = -1;
        string format_str;
        int num = 0;
        auto i = S->child_begin();
        while (i != S->child_end())
        {
            auto j = i;
            while (true)
            {
                //cout << "j: " << j->getStmtClassName() << endl;
                if (strcmp(j->getStmtClassName(), "DeclRefExpr") == 0)
                {
                    DeclRefExpr *declre = (DeclRefExpr *)*j;
                    string param_name = declre->getNameInfo().getAsString();
                    string param_type = declre->getType().getAsString();
                    if (num >= 4)
                    {
                        int temp = Var[param_name];
                        if (temp == 0)
                            temp = -1;
                        Format_param.insert(pair<int, int>(num - 4, temp));
                        Format_param[num - 4] = temp;
                    }
                    if (num == 2)
                    {
                        len_n = Var[param_name];
                    }
                    if (num == 1)
                    {
                        len_dest = Var[param_name];
                        //cout << "len_dest:  " << len_dest << endl;
                    }
                    //cout << "param name:  " << param_name << endl;
                    //cout << "param type:  " << param_type << endl;
                    break;
                }
                else if (strcmp(j->getStmtClassName(), "StringLiteral") == 0)
                {
                    StringLiteral *strltr = (StringLiteral *)*j;
                    if (num <= 3)
                    {
                        format_str = strltr->getString().str();
                        //cout << "str: " << format_str << endl;
                    }
                    else if (num >= 4)
                    {
                        string temp_str = strltr->getString().str();
                        Format_param.insert(pair<int, int>(num - 4, temp_str.size()));
                        Format_param[num - 4] = temp_str.size();
                        //cout << "str: " << temp_str << endl;
                    }
                    break;
                }
                else if (strcmp(j->getStmtClassName(), "IntegerLiteral") == 0)
                {

                    IntegerLiteral *intltr = (IntegerLiteral *)*j;
                    int temp_int = intltr->getValue().getSExtValue();
                    if (num >= 4)
                    {
                        Format_param.insert(pair<int, int>(num - 4, temp_int));
                        Format_param[num - 4] = temp_int;
                        //cout << "int: " << temp_int << endl;
                    }
                    else if (num == 2)
                    {
                        len_n = temp_int;
                    }

                    break;
                }
                else if (j->child_begin() == nullptr)
                {
                    //cout << "none child" << endl;
                    break;
                }
                else
                    j = j->child_begin();
            }
            //cout << "num=" << num << endl;

            i++;
            num++;
            //cout << endl;
        }

        int dLen;
        int totalLen = count_totalLen(format_str, dLen);
        //std::cout << "\033[32m" << len_dest<<"   "<<len_n<<"    "<<totalLen<< "\033[0m" << std::endl;
        //cout << "len_dest" << len_dest << endl;
        //cout << "totalLen" << totalLen << endl;
        //if (len_dest <= totalLen)
        if (totalLen >= len_dest && len_n > len_dest)
        {
            clang::SourceManager &srcmgr = callee->getASTContext().getSourceManager();
            //error
            //cout << "\033[32m error:there is a wrong \033[0m" << endl;
            //cout << "error:" << endl;
            string Func_name = callee->getNameAsString();

            bof_error tmp_info;
            tmp_info.filename = srcmgr.getFilename(FuncLocation[callee_num]).str();
            tmp_info.line = srcmgr.getSpellingLineNumber(FuncLocation[callee_num]);
            tmp_info.col = srcmgr.getSpellingColumnNumber(FuncLocation[callee_num]);
            tmp_info.info = "buffer overflow caused by use " + Func_name + " to format data";

            int tmp_num = bof_info.size();
            bof_info.insert(pair<int, bof_error>(tmp_num, tmp_info));
        }
        else if (dLen == -1)
        {
            //warning:maybe right or wrong
            //cout << "\033[32m warning:maybe right or wrong \033[0m" << endl;
        }
        else
        {
            //no error or warning
            //cout << "\033[32m no error or warning \033[0m" << endl;
        }
    }

    int count_totalLen(string format_str, int &dLen)
    {
        int totalLen = 0;
        dLen = 0;
        //cout << "str size:  " << format_str.size() << endl;
        int num_param = 0;
        for (int cur = 0; cur < format_str.size(); cur++)
        {
            //cout << "str[" << cur << "]: "
            //<< " " << format_str[cur] << endl;
            if (format_str[cur] == '%')
            {
                if (format_str[cur + 1] == '-')
                {
                    cur++;
                }
                int len_1 = -1;
                int len_2 = -1;
                int len_str = 0;
                cur++;
                char fmt_type = detect_formattingStr(format_str, cur, len_1, len_2);
                //cout << "len_1: " << len_1 << "   "
                // << "len_2:  " << len_2 << endl;
                //cout << "fmt_type: " << fmt_type << endl;
                if (len_1 != -1)
                {
                    totalLen += len_1;
                    if (len_2 != -1 && dLen != -1)
                        dLen += len_2;
                }
                else
                {
                    int temp = Format_param[num_param];
                    if (fmt_type == 's')
                    {
                        if (temp != -1)
                        {
                            totalLen += temp;
                        }
                        else
                        {
                            dLen = -1;
                        }
                    }
                    else if (fmt_type == 'd')
                    {
                        int temp_len = 0;
                        while (temp != 0)
                        {
                            //cout << "temp=  " << temp << endl;
                            temp_len++;
                            temp = temp / 10;
                        }
                        totalLen += temp_len;
                    }
                }
                num_param++;
            }
            else
            {
                totalLen++;
            }
            cout << endl;
        }

        //cout << "totlaLen:  " << totalLen << endl;
        //cout << "dLen:  " << dLen << endl;
        return totalLen;
    }

    char detect_formattingStr(string str, int &cur, int &len_1, int &len_2)
    {
        /* auto i = S->child_begin();
  for (int temp = 0; temp < num; temp++)
  {
    i++;
  }*/
        //cout << "////start////" << endl;
        char format_type;
        len_1 = -1;
        len_2 = -1;
        while (cur < str.size())
        {
            if (str[cur] >= '0' && str[cur] < '9')
            {
                if (len_2 == -1)
                {
                    if (len_1 == -1)
                        len_1 = 0;
                    len_1 = len_1 * 10 + str[cur] - '0';
                    //cout << "len_1: " << len_1 << endl;
                }
                else
                {
                    len_2 = len_2 * 10 + str[cur] - '0';
                    //cout << "len_2: " << len_1 << endl;
                }
            }
            else if (str[cur] == '.')
            {
                len_2 == 0;
            }
            else
            {
                format_type = str[cur];
                break;
            }
            cur++;
        }
        return format_type;
    }

    bool isCallFunc(const Stmt *S)
    {
        bool jump = false;
        for (auto it_ImpCE = S->child_begin(); it_ImpCE != S->child_end(); it_ImpCE++)
        {
            //cout << it_ImpCE->getStmtClassName() << endl;
            char const *ImpCEname = it_ImpCE->getStmtClassName();
            int isImplicitCastExpr = strcmp(ImpCEname, "ImplicitCastExpr");
            if (isImplicitCastExpr == 0)
            {
                ImplicitCastExpr *tempICE = (ImplicitCastExpr *)*it_ImpCE;
                //cout << "!-----------------" << tempICE->getCastKindName() << endl;
                char const *calltype = tempICE->getCastKindName();
                int isFunction = strcmp(calltype, "FunctionToPointerDecay");
                if (isFunction == 0)
                {
                    //jump = true;
                    for (auto it_DeclRE = it_ImpCE->child_begin(); it_DeclRE != it_ImpCE->child_end(); it_DeclRE++)
                    {
                        char const *DeclREname = it_DeclRE->getStmtClassName();
                        //cout << "   " << DeclREname << endl;
                        int isDeclRefExpr = strcmp(DeclREname, "DeclRefExpr");
                        if (isDeclRefExpr == 0)
                        {
                            //Stmt *t1 = (Stmt *)*it_DeclRE;
                            DeclRefExpr *tempDRE = (DeclRefExpr *)*it_DeclRE;
                            jump = true;
                            break;
                        }
                    }
                    if (jump)
                    {
                        break;
                    }
                }
            }
        }
        return jump;
    }

    int countBinaryOperator(const Stmt *S)
    {
        int res;
        int v[2];
        v[0] = v[1] = 0;

        BinaryOperator *BO = (BinaryOperator *)S;
        auto p1 = S->child_begin();
        auto p2 = p1++;
        /* auto p[2] = {p1,p2};
        p[0] = p1;
        p[1] = p2;*/

        //p1
        int i = 1;
        if (strcmp(p1->getStmtClassName(), "BinaryOperator") == 0) //二元运算
        {
            v[i] = countBinaryOperator(*p1);
        }
        else if (strcmp(p1->getStmtClassName(), "ParenExpr") == 0) //括号
        {
            p1 = p1->child_begin();
            if (strcmp(p1->getStmtClassName(), "BinaryOperator") == 0) //二元运算
            {
                v[i] = countBinaryOperator(*p1);
            }
        }
        else if (strcmp(p1->getStmtClassName(), "IntegerLiteral") == 0)
        {
            IntegerLiteral *IntLitrl = (IntegerLiteral *)*p1;
            v[i] = IntLitrl->getValue().getSExtValue();
        }
        else if (strcmp(p1->getStmtClassName(), "ImplicitCastExpr") == 0)
        {
            p1 = p1->child_begin();
            if (strcmp(p1->getStmtClassName(), "IntegerLiteral") == 0)
            {
                IntegerLiteral *v_int = (IntegerLiteral *)*p1;
                v[i] = v_int->getValue().getSExtValue();
            }
            else if (strcmp(p1->getStmtClassName(), "DeclRefExpr") == 0)
            {
                DeclRefExpr *declre = (DeclRefExpr *)*p1;
                string param_name = declre->getNameInfo().getAsString();
                string param_type = declre->getType().getAsString();
                v[i] = Decl_Var[param_name].length;
                if (v[i] < 0)
                {
                    v[i] = 0;
                }
            }
            else if (strcmp(p1->getStmtClassName(), "ImplicitCastExp") == 0)
            {
                p1 = p1->child_begin();
                if (strcmp(p1->getStmtClassName(), "IntegerLiteral") == 0)
                {
                    IntegerLiteral *v_int = (IntegerLiteral *)*p1;
                    v[i] = v_int->getValue().getSExtValue();
                }
                else if (strcmp(p1->getStmtClassName(), "DeclRefExpr") == 0)
                {
                    DeclRefExpr *declre = (DeclRefExpr *)*p1;
                    string param_name = declre->getNameInfo().getAsString();
                    string param_type = declre->getType().getAsString();
                    v[i] = Decl_Var[param_name].length;
                    if (v[i] < 0)
                    {
                        v[i] = 0;
                    }
                }
            }
        }
        else if (strcmp(p1->getStmtClassName(), "UnaryExprOrTypeTraitExpr") == 0)
        {
            UnaryExprOrTypeTraitExpr *temp = (UnaryExprOrTypeTraitExpr *)*p1;

            v[i] = 1;
        }

        //p2
        i = 0;
        if (strcmp(p2->getStmtClassName(), "BinaryOperator") == 0) //二元运算
        {
            v[i] = countBinaryOperator(*p2);
        }
        else if (strcmp(p2->getStmtClassName(), "ParenExpr") == 0) //括号
        {
            p2 = p2->child_begin();
            if (strcmp(p2->getStmtClassName(), "BinaryOperator") == 0) //二元运算
            {
                v[i] = countBinaryOperator(*p2);
            }
        }
        else if (strcmp(p2->getStmtClassName(), "IntegerLiteral") == 0)
        {
            IntegerLiteral *IntLitrl = (IntegerLiteral *)*p2;
            v[i] = IntLitrl->getValue().getSExtValue();
        }
        else if (strcmp(p2->getStmtClassName(), "ImplicitCastExpr") == 0)
        {
            p2 = p2->child_begin();
            if (strcmp(p2->getStmtClassName(), "IntegerLiteral") == 0)
            {
                IntegerLiteral *v_int = (IntegerLiteral *)*p2;
                v[i] = v_int->getValue().getSExtValue();
            }
            else if (strcmp(p2->getStmtClassName(), "DeclRefExpr") == 0)
            {
                DeclRefExpr *declre = (DeclRefExpr *)*p2;
                string param_name = declre->getNameInfo().getAsString();
                string param_type = declre->getType().getAsString();
                v[i] = Decl_Var[param_name].length;
                if (v[i] < 0)
                {
                    v[i] = 0;
                }
            }
            else if (strcmp(p2->getStmtClassName(), "ImplicitCastExp") == 0)
            {
                p2 = p2->child_begin();
                if (strcmp(p2->getStmtClassName(), "IntegerLiteral") == 0)
                {
                    IntegerLiteral *v_int = (IntegerLiteral *)*p2;
                    v[i] = v_int->getValue().getSExtValue();
                }
                else if (strcmp(p2->getStmtClassName(), "DeclRefExpr") == 0)
                {
                    DeclRefExpr *declre = (DeclRefExpr *)*p2;
                    string param_name = declre->getNameInfo().getAsString();
                    string param_type = declre->getType().getAsString();
                    v[i] = Decl_Var[param_name].length;
                    if (v[i] < 0)
                    {
                        v[i] = 0;
                    }
                }
            }
        }
        else if (strcmp(p2->getStmtClassName(), "UnaryExprOrTypeTraitExpr") == 0)
        {
            UnaryExprOrTypeTraitExpr *temp = (UnaryExprOrTypeTraitExpr *)*p2;

            v[i] = 1;
        }

        string op = BO->getOpcodeStr();
        //cout << "op:  " << op << endl;
        if (op == "+")
        {
            res = v[0] + v[1];
        }
        else if (op == "-")
        {
            res = v[0] - v[1];
        }
        else if (op == "*")
        {
            res = v[0] * v[1];
        }
        else if (op == "/")
        {
            res = v[0] / v[1];
        }
        else
        {
            res = -2;
        }
        return res;
    }

public:
    void BOF_Entry_old(clang::FunctionDecl *fd)
    {
        //std::cout << common::getFullName(fd) << std::endl;
        auto fd_cfg = common::buildCFG(fd);
        // Traverse CFG

        //////////////////////////////////////////////////////
        //std::cout << "- - - - - - Start - - - - - -" << std::endl;
        const Stmt *all_S[5000];
        bool toDetect = false;
        bool jump = false;
        int count = 0;
        int call_num = 0;
        auto it_block = fd_cfg->begin();
        for (; it_block != fd_cfg->end(); it_block++)
        {
            auto it_element = (*it_block)->begin();
            for (; it_element != (*it_block)->end(); it_element++)
            {
                Optional<CFGStmt> it_stmt = it_element->getAs<CFGStmt>();
                const Stmt *S = it_stmt->getStmt();
                char const *StmtClassName = S->getStmtClassName();
                //std::cout << "\033[32m" << StmtClassName << "\033[0m" << std::endl;

                int isDeclStmt = strcmp(StmtClassName, "DeclStmt");
                int isCallExpr = strcmp(StmtClassName, "CallExpr");
                if (isDeclStmt == 0)
                {
                    all_S[count] = S;
                    count++;
                }
                else if (isCallExpr == 0)
                {
                    jump = isCallFunc(S);
                    if (jump)
                    {
                        CallExpr *callexpr = (CallExpr *)S;
                        FunctionDecl *callee = callexpr->getDirectCallee();
                        string callee_name = callee->getNameAsString();

                        if (callee_name == "bcopy" || callee_name == "memcpy" || callee_name == "memset" || callee_name == "strncpy" || callee_name == "strcpy")
                        {
                            all_S[count] = S;
                            count++;
                            call_num++;
                            toDetect = true;
                        }
                        /*if (callee_name == "sprintf" || callee_name == "scanf" || callee_name == "snprintf")
                        {
                            all_S[count] = S;
                            count++;
                            call_num++;
                            toDetect = true;
                        }*/
                    }
                    jump = false;
                }
            }
        }

        if (toDetect)
        {
            for (int i = 0; i < count && call_num > 0; i++)
            {
                if (strcmp(all_S[i]->getStmtClassName(), "DeclStmt") == 0)
                {
                    handleDeclStmt(all_S[i]); //处理DeclStmt
                }
                else if (strcmp(all_S[i]->getStmtClassName(), "CallExpr") == 0)
                {
                    handleCallExpr(all_S[i]); //处理CallExpr
                    call_num--;
                }
            }
        }
        // std::cout << "- - - - - - End - - - - - -" << std::endl;
    }

    void BOF_Entry(SourceManager &SrcMgr, Stmt *S, int index, defuse_node *ano)
    {
        all_node = ano;
        ifindex = index;
        char const *StmtClassName = S->getStmtClassName();
        //std::cout << "\033[32m" << StmtClassName << "\033[0m" << std::endl;

        int isDeclStmt = strcmp(StmtClassName, "DeclStmt");
        int isCallExpr = strcmp(StmtClassName, "CallExpr");
        if (isDeclStmt == 0)
            handleDeclStmt(S); //处理DeclStmt

        if (isCallExpr == 0)
            handleCallExpr(S); //处理CallExprzhe
    }

    void BOF_Detect()
    {
        for (int i = 0; i < bof_info.size(); i++)
        {
            bof_error tmp = bof_info[i];
            error_info *e = new_error_info(NULL, tmp.filename, tmp.line, tmp.col, TYPE_ERROR, tmp.info,bof);
            result.push(e);
        }
    }
};
#endif

