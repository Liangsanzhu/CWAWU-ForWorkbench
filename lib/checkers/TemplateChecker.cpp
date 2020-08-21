#include "checkers/TemplateChecker.h"
#include "checkers/Memory_Leak_Detector.h"
#include "checkers/Null_Pointer_Detector.h"
#include "checkers/Var_Uninitialized.h"
#include "checkers/Out_Index.h" 
#include "checkers/Buffer_Overflow_Detector.h" 
#include "checkers/Detector.h"
#include <iostream>
using namespace std;

void TemplateChecker::check() {
  readline();
  //读取缺陷报错自定义配置
  auto astr_iter = getASTRsBegin();
 // VU_Entry1(astr_iter);//变量未初始化
 Out_Index OI;
 MemoryLeak ML;
 //Buffer_Overflow BOF;
  while (astr_iter != getASTRsEnd()) {
    OI.OI_Entry(astr_iter);//数组越界

    auto fds = astr_iter->second.GetFunctionDecls();
    for (auto fd : fds) {
      
     Get_SourceCode(fd);
     //BOF.BOF_Entry_old(fd);
      //VU_Entry2(fd);
      //NPD_Entry(fd);//空指针解引用
      ML.ML_Entry(fd);//内存泄漏
      }

    ++astr_iter;
  }
  OI.OI_Dectect();
  //VU_Detect();
 // NPD_Detect();
 //BOF.BOF_Detect();
 ML.ML_Detect();
  print_result();//打印所有出错信息
}
