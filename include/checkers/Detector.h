#ifndef D_T_H
#define D_T_H
#include<iostream>
using namespace std;
#include <vector>
#include<map>
#include<queue>
#include<stack>
#include"Def_Use.h"
#include <iomanip>
#include "string.h"
#include "framework/ASTManager.h"

#define CLOSE printf("\033[0m"); //关闭彩色字体
#define LIGHT printf("\033[1m");
#define RED printf("\033[31m"); //红色字体
#define WHITE printf("\033[37m"); //bai色字体
#define GREEN printf("\033[32m");//绿色字体
#define BLACK printf("\033[30m");//hei色字体
#define YELLOW printf("\033[33m");//黄色字体
#define BLUE printf("\033[34m");//蓝色字体


#define FLAG_EMPTY -1
#define FLAG_BINARY 1
#define TYPE_ERROR 0
#define TYPE_NOTE 1
string ML_ERROR_TYPE_MISS="Miss memory realese";
string ML_ERROR_TYPE_NOTMATCH="Realese doesn't match allocation";
string ML_ERROR_TYPE_LOCATION="Allocated here";
string NPD_ERROR_TYPE_DEREFERENCE="Find Null Pointer Dereference Fault";
string NPD_ERROR_TYPE_SET_NULL="Set Null or Delete here";
string VU_ERROR_TYPE_DEL_A="Unintialized Varirbale '";
string VU_ERROR_TYPE_DEL_B="' is declared here";
string VU_ERROR_TYPE_USE="Used here";
string OI_ERROR_TYPE_ARRAY_A="Array '";
string OI_ERROR_TYPE_ARRAY_B="' is out of range";

map<string,vector<string>> SourceCode;

void split(std::string& s,std::string& delim,std::vector<std::string>* ret)
{
	size_t last = 0;
	size_t index = s.find_first_of(delim,last);
	while(index != string::npos)
	{
		ret->push_back(s.substr(last,index-last));
		last= index+1;
		index = s.find_first_of(delim,last);
	}
	if(index-last > 0)
	{
		ret->push_back(s.substr(last,index-last));
	}
}
struct error_info
{
  int lineno;
  int colno;
  string info;
  string filename;
  int type;
  error_info*next;
  string filter;
 
 bool operator<(const error_info& a) const
    {
      if(filename<a.filename)
      return true;
        if(filename==a.filename&&lineno<a.lineno)
        return true;
        if(filename==a.filename&&lineno==a.lineno&&colno<a.colno)
        return true;
        return false;
    }
};
struct cmp //重写仿函数
{
    bool operator() (error_info* a, error_info*b) 
    {
        return !(*a < *b); //大顶堆
    }
};

priority_queue<error_info*,vector<error_info*>,cmp >result;//
//string FILED;uanma 
error_info* new_error_info(error_info*g,string filename,int line,int col,int type,string info)
{
  error_info*e=new error_info;
  vector<string>t;
 // string a=filename;
  //string s2="/";
  //split(a,s2,&t);
  e->filename=filename;//t[t.size()-1];//文件名
  e->lineno=line;//行
  e->colno=col;//列
  e->type=type;//Error/Note/Warning这种类型
  e->info=info;//Error/Note/Warning:后面跟着的一些文字
  e->next=g;//需要关联报错的下一个指针，需要先创建好
  e->filter="";
  return e;
 
}

error_info* new_error_info(error_info*g,string filename,int line,int col,int type,string info,string filter)
{
  error_info*e=new error_info;
  vector<string>t;
 // string a=filename;
  //string s2="/";
  //split(a,s2,&t);
  e->filename=filename;//t[t.size()-1];//文件名
  e->lineno=line;//行
  e->colno=col;//列
  e->type=type;//Error/Note/Warning这种类型
  e->info=info;//Error/Note/Warning:后面跟着的一些文字
  e->next=g;//需要关联报错的下一个指针，需要先创建好
  e->filter=filter;
  return e;
 
}


void Get_SourceCode(clang::FunctionDecl*fd)
{

 clang::SourceManager&srcMgr(fd->getASTContext().getSourceManager());
string FILENAME=srcMgr.getFilename(srcMgr.getLocForStartOfFile(srcMgr.getMainFileID()));
       if(SourceCode.find(FILENAME)==SourceCode.end())
      {
        const char*sourcecode=srcMgr.getBuffer(srcMgr.getMainFileID())->getBufferStart();
        int size=srcMgr.getBuffer(srcMgr.getMainFileID())->getBufferSize();
        string p=sourcecode;
        string s2="\n";
        vector<string> SourceCodeItem;
        split(p,s2,&SourceCodeItem);
        SourceCode.insert(pair<string,vector<string>>(FILENAME,SourceCodeItem));
      }
}
bool print_error(error_info*e)
{
 // LIGHT
  //WHITE
 
  if(e==NULL)
  return false;
if(e->filter!=""&&SourceCode.find(e->filename)!=SourceCode.end()&&e->lineno>=1)
 { 
	if(SourceCode.find(e->filename)->second[e->lineno-1].find(e->filter)==string::npos)
		return false;
 }
  LIGHT
  cout<<e->filename<<":"<<e->lineno<<":"<<e->colno<<":";
  switch(e->type)
  {
    case TYPE_ERROR:
    RED
    cout<<" error: ";
    CLOSE
    LIGHT
    break;
    case TYPE_NOTE:
    BLACK
    cout<<" note: ";
    CLOSE
    break;
  }
  //LIGHT
  cout<<e->info<<endl;
  CLOSE
  WHITE
 
   
 if(SourceCode.find(e->filename)!=SourceCode.end()&&e->lineno>=1)
 { 
	cout<<SourceCode.find(e->filename)->second[e->lineno-1]<<endl;
 }
  for(int i=0;i<e->colno-1;i++)
    cout<<" ";
  LIGHT
  GREEN
  cout<<"^"<<endl;
  CLOSE
  if(e->next!=NULL)
  print_error(e->next);
return true;
}
void print_result()
{
    //for(auto it:SourceCode)
    //cout<<it<<endl;
    int count=0;
 while(!result.empty())
  {
    
    if(print_error(result.top()))
	{
		count++;
cout<<"--------------------------------------\n";
	}
    result.pop();
  }
  if(count>1)
  cout<<endl<<count<<" errors generated."<<endl;
  else if(count==1)
   cout<<endl<<count<<" error generated."<<endl;
}


#endif
