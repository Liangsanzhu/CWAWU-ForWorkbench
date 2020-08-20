#ifndef M_L_D_H
#define M_L_D_H
#include"Detector.h"



struct variable
{
   
    int id;
    string name;
    int num;
    string field;
    int lineno;
    int colno;
    string error;
   
};
struct get_memory
{
  string type;
  string field;
  int size;
  int id;
  int lineno;
  int colno;
  bool isarray;
  bool isreleased;
  int num;
  string error;
 
};
map<int,get_memory*> table;//变量id->分配内存块
map<int,get_memory*>mem;//内存块id->内存块
map<int,variable*>sym;//变量id->变量

map<int,int>free_temp;

string FIELD;
int COL;
int LINE;
bool goto_flag=false;

void getmem(const Stmt*S,get_memory*&g)
{
  
  for(auto bt=S->child_begin();bt!=S->child_end();bt++)
    {
          getmem(*bt,g);
    }
  if(strcmp(S->getStmtClassName(),"CXXNewExpr")==0)
  {
  CXXNewExpr*H=(CXXNewExpr*)S;
 
    int id=S->getBeginLoc().getRawEncoding();
    if(mem.find(id)!=mem.end())
    {
      g=mem.find(id)->second;
      g->num++;
    }  
    
  }
}
void getmalloc(const Stmt*S,get_memory*&flag)
{

  for(auto bt=S->child_begin();bt!=S->child_end();bt++)
    {
          getmalloc(*bt,flag);
    }
    
  if(strcmp(S->getStmtClassName(),"DeclRefExpr")==0)
    {
      ValueStmt*F=(ValueStmt*)S;
      Expr*G=F->getExprStmt();
      DeclRefExpr*H=(DeclRefExpr*)G;

      if(flag==NULL&&(strcmp(H->getDecl()->getName().data(),"malloc")==0))
      {   
        int id=S->getBeginLoc().getRawEncoding();
       // cout<<LINE<<H->getDecl()->getName().data()<<endl; 
        if(mem.find(id)!=mem.end())
        {
          get_memory*g;
          g=mem.find(id)->second;
          g->num++;
          flag=g;
        }else
        {
          get_memory*g=new get_memory;
          g->isarray=true;
          g->id=id;

	if(free_temp.find(id)!=free_temp.end()&&free_temp[id]<0)
	{
	g->num=free_temp[id];
	free_temp[id]++;
	}else{
          g->num=0;}

          g->lineno=LINE;
          g->colno=COL;
          g->field=FIELD;
	if(g->num<0)
	g->isreleased=true;
	else
          g->isreleased=false;
         // g->lineno=H->get;
          //g->type=
         
          mem.insert(pair<int,get_memory*>(id,g));
          flag=g;
        }
         
      }
    }   
}
void getmalloc(const Stmt*S,get_memory*&flag,int myid)
{

  for(auto bt=S->child_begin();bt!=S->child_end();bt++)
    {
          getmalloc(*bt,flag,myid);
    }
    
  if(strcmp(S->getStmtClassName(),"DeclRefExpr")==0)
    {
      ValueStmt*F=(ValueStmt*)S;
      Expr*G=F->getExprStmt();
      DeclRefExpr*H=(DeclRefExpr*)G;

      if(flag==NULL&&(strcmp(H->getDecl()->getName().data(),"malloc")==0))
      {   
        int id=S->getBeginLoc().getRawEncoding();
       // cout<<LINE<<H->getDecl()->getName().data()<<endl; 
        if(mem.find(id)!=mem.end())
        {
          get_memory*g;
          g=mem.find(id)->second;
          g->num++;
          
	if(free_temp.find(myid)!=free_temp.end()&&free_temp[myid]<0)
	{
	g->num+=free_temp[myid];
	free_temp[myid]++;
//cout<<"hello"<<endl;
	}
	if(g->num<=0)
	g->isreleased=true;
	else
          g->isreleased=false;
	flag=g;//cout<<"myid= "<<myid<<" "<<g->num<<endl;
	
        }else
        {
          get_memory*g=new get_memory;
          g->isarray=true;
          g->id=id;
//cout<<"myid= "<<myid<<endl;
	if(free_temp.find(myid)!=free_temp.end()&&free_temp[myid]<0)
	{
	g->num=free_temp[myid];
	free_temp[myid]++;
//cout<<"hello"<<endl;
	}else{
          g->num=0;}

          g->lineno=LINE;
          g->colno=COL;
          g->field=FIELD;
	if(g->num<0)
	g->isreleased=true;
	else
          g->isreleased=false;
         // g->lineno=H->get;
          //g->type=
         
          mem.insert(pair<int,get_memory*>(id,g));
          flag=g;
        }
         
      }
    }   
}
void getfree(const Stmt*S,bool&flag)
{
  
  for(auto bt=S->child_begin();bt!=S->child_end();bt++)
    {
          getfree(*bt,flag);
    }
  if(strcmp(S->getStmtClassName(),"DeclRefExpr")==0)
    {
      ValueStmt*F=(ValueStmt*)S;
      Expr*G=F->getExprStmt();
      DeclRefExpr*H=(DeclRefExpr*)G;
      
     
  if(flag==false&&strcmp(H->getDecl()->getName().data(),"free")==0)
    {
     flag=true;
    
    }   
    }
}
void getvar(const Stmt*S,int&a)
{
     
    for(auto bt=S->child_begin();bt!=S->child_end();bt++)
    {
          getvar(*bt,a);
    }
    if(strcmp(S->getStmtClassName(),"DeclRefExpr")==0)
    {
      ValueStmt*F=(ValueStmt*)S;
      Expr*G=F->getExprStmt();
      DeclRefExpr*H=(DeclRefExpr*)G;
      
       if(a==-1&&(strcmp(H->getDecl()->getName().data(),"malloc")!=0&&\
      strcmp(H->getDecl()->getName().data(),"realloc")!=0&&\
      strcmp(H->getDecl()->getName().data(),"calloc")!=0)&&strcmp(H->getDecl()->getName().data(),"free")!=0)
       {
        int id=H->getDecl()->getGlobalID();
        variable*var=new variable;
        var->name=H->getDecl()->getName().data();
        var->id=id;
        var->field=FIELD;
        var->lineno=LINE;
        var->colno=COL;
        sym.insert(pair<int,variable*>(id,var));
        a=id;
      }
    }   
}
    
void TraverseCfg(const Stmt*S,int lay)
{
  if(S==NULL)
  return;
 

 if(strcmp(S->getStmtClassName(),"BinaryOperator")==0)
  {
      BinaryOperator*b=(BinaryOperator*)S;
      if(b->getOpcodeStr().str()!="=")
      {
        return;
      }
      int id=-1;
      getvar(S,id);
      if(id!=-1)
      {
        get_memory*g=NULL;
        getmem(S,g);
        if(g!=NULL)
        { //cout<<g->type<<"$$$$"<<endl;
          table.insert(pair<int,get_memory*>(id,g));
        }
        else
        {
          get_memory*flag=NULL;;
          getmalloc(S,flag,id);
          if(flag!=NULL)
          { //cout<<g->type<<"$$$$"<<endl;
            table.insert(pair<int,get_memory*>(id,flag));
          }
        }
      }

  }

else if(strcmp(S->getStmtClassName(),"DeclStmt")==0)
{
  DeclStmt*F=(DeclStmt*)S;
  VarDecl*v=(VarDecl*)F->getSingleDecl();

  if(lay==1)
  {
    
    int id=v->getGlobalID();
   
    get_memory*g=NULL;
    int id_temp=-1;
    getvar(S,id_temp);
    if(table.find(id_temp)!=table.end())
    {
      variable*var=new variable;
      var->name=v->getNameAsString();
      var->id=v->getGlobalID();
      //cout<<var->id<<"***"endl;
      var->field=FIELD;
        var->lineno=LINE;
        var->colno=COL;
      sym.insert(pair<int,variable*>(id,var));
      table.insert(pair<int,get_memory*>(id,table[id_temp]));
      return;
    }
    
    getmem(S,g);
    if(g!=NULL)
   { //cout<<g->type<<"$$$$"<<endl;
    variable*var=new variable;
    var->name=v->getNameAsString();
    var->id=v->getGlobalID();
    //cout<<var->id<<"***"endl;
    var->field=FIELD;
      var->lineno=LINE;
        var->colno=COL;
    sym.insert(pair<int,variable*>(id,var));
    table.insert(pair<int,get_memory*>(id,g));
   }
   else
   {
      get_memory*flag=NULL;
      getmalloc(S,flag,id);
//if(LINE==196)
//cout<<"insert"<<id<<endl;
      if(flag!=NULL)
      { //cout<<g->type<<"$$$$"<<endl;

        variable*var=new variable;
        var->name=v->getNameAsString();
        var->id=v->getGlobalID();
        //cout<<var->id<<"***"endl;
        var->field=FIELD;
        var->lineno=LINE;
        var->colno=COL;
        sym.insert(pair<int,variable*>(id,var));
        table.insert(pair<int,get_memory*>(id,flag));
      }
   }
  }

}
else if(strcmp(S->getStmtClassName(),"CallExpr")==0)
{
  get_memory*gm=NULL;;
  getmalloc(S,gm);
  bool flag=false;
  getfree(S,flag);
  if(flag==true)
  {

    int id=-1;
    getvar(S,id);
    if(id!=-1)
    {

    if(table.find(id)!=table.end())
    {
//cout<<LINE<<"find free"<<" "<<id<<endl;
      get_memory*g=table.find(id)->second;
      g->num--;
	free_temp[id]--;
    }else{
	if(goto_flag==true)
	return;
	if(free_temp.find(id)!=free_temp.end())
	{
		free_temp.insert(pair<int,int>(id,0));
	}
	free_temp[id]--;
//cout<<LINE<<"find new free"<<id<<","<<free_temp[id]<<endl;
	}
      
    }
  }
}
else if(strcmp(S->getStmtClassName(),"CXXNewExpr")==0)
{
  CXXNewExpr*H=(CXXNewExpr*)S;
  if(lay==1)
  {
    get_memory*g=new get_memory;
    g->id=S->getBeginLoc().getRawEncoding();
    g->isarray=H->isArray();
    string a=H->getAllocatedType().getAsString();
    g->type=a;
    g->size=0;
    g->num=0;
    g->lineno=LINE;
    g->colno=COL;
    g->field=FIELD;
    g->isreleased=false;
    mem.insert(pair<int,get_memory*>(g->id,g));
  }
}
else if(strcmp(S->getStmtClassName(),"CXXDeleteExpr")==0)
{
  ValueStmt*F=(ValueStmt*)S;
  Expr*G=F->getExprStmt();
  CXXDeleteExpr*H=(CXXDeleteExpr*)G;
  int id=-1;
  getvar(S,id);
  if(id!=-1)
  {
   if(table.find(id)!=table.end())
   {
     get_memory*g=table.find(id)->second;
     if(g->isarray==H->isArrayForm())
     {
       g->num--;
     }
     else
     {
       error_info* e=new_error_info(NULL,g->field,g->lineno,g->colno,TYPE_NOTE,ML_ERROR_TYPE_LOCATION);
        //result.push(e);
        error_info* te=new_error_info(e,FIELD,LINE,COL,TYPE_ERROR,ML_ERROR_TYPE_NOTMATCH);
        result.push(te);
     }
     
   }
    
  }
}
  

}


void detect()
{
  //cout<<"================== I'm detecting ================"<<endl;
  for(map<int ,get_memory*>::iterator it=table.begin();it!=table.end();it++)
  {
    get_memory*g=it->second;
  int v=it->first;
    if(g->num<=0)
      g->isreleased=true;
    
    if(g->isreleased==false)
    {
      //cout<<"Help! Memory leak!\n ID:"<<v;
      if(sym.find(v)!=sym.end())
      {
        variable*var=sym.find(v)->second;
        g->isreleased=true;
        /*cout<<"  Func:"<<var->field;
        cout<<"< Line "<<var->lineno<<" , "<<"Col "<<var->colno<<" >";
        add_result(g->field,g->lineno,g->colno,TYPE_NOTE,ML_ERROR_TYPE_LOCATION);
        
        cout<<"  Name:"<<var->name<<endl;*/
        error_info*e=new_error_info(NULL,g->field,g->lineno,g->colno,TYPE_ERROR,ML_ERROR_TYPE_LOCATION);
        //error_info* te=new_error_info(e,var->field,var->lineno,var->colno,TYPE_ERROR,ML_ERROR_TYPE_MISS);
        result.push(e);
      }
      //<<" I'm allocated at "<<g->field<<"< Line "<<g->lineno<<" , "<<"Col "<<g->colno<<" >"<<endl;
     }
  }
  for(map<int ,get_memory*>::iterator it=mem.begin();it!=mem.end();it++)
  {
    get_memory*g=it->second;
    if(g->isreleased==false)
    {
       error_info* e=new_error_info(NULL,g->field,g->lineno,g->colno,TYPE_ERROR,ML_ERROR_TYPE_MISS);
       result.push(e);
    }

  }
 
}

void ML_Entry(clang::FunctionDecl*fd)
{
  auto fd_cfg = common::buildCFG(fd);
  clang::SourceManager&srcMgr(fd->getASTContext().getSourceManager());

      for(CFG::iterator it=fd_cfg->begin();it!=fd_cfg->end();it++)
      {
	goto_flag=false;
        if(it+1!=fd_cfg->end()){
	 if((*(it+1))->getTerminatorStmt()!=NULL)
      	if(strcmp((*(it+1))->getTerminatorStmt()->getStmtClassName(),"GotoStmt")==0){
			goto_flag=true;
		}
	}
        
      
      for(CFGBlock::iterator at=(*it)->begin();at!=(*it)->end();at++)
      {
        //cout<<at->getgetmemnd()<<endl;
        
         if (Optional<CFGStmt> CS = at->getAs<CFGStmt>()) {

          const Stmt *S = CS->getStmt();
          //S->dump();
         // vector<string>t;
          string a=srcMgr.getFilename(srcMgr.getLocForStartOfFile(srcMgr.getMainFileID()));
   
         // string s2="/";
          //split(a,s2,&t);
          FIELD=a;
         // t.clear();
          //S->getBeginLoc().dump(srcMgr);
          COL=srcMgr.getSpellingColumnNumber(S->getBeginLoc());
          LINE=srcMgr.getSpellingLineNumber(S->getBeginLoc());
         
          //cout<<S->getStmtClassName()<<"***"<<endl;

//cout<<LINE<<endl;
//cout<<S->getStmtClassName()<<endl;
          TraverseCfg(S,1);
         // bool k=false;
         
         }
      }
      }
}
void ML_Detect()
{
  detect();
}
#endif // M_L_D_H
