// PL.cpp : Defines the entry point for the console application.
//

#pragma warning(disable:4996)
#include "common.h"

////////////////////////////////////////////////////////////
////////////////////////函数头定义如下//////////////////////
////////////////////////////////////////////////////////////

SYMLIST * listsAdd(SYMLIST * list1,SYMLIST * list2);
SYMLIST * listAddSym(SYMLIST * list,SYMBOL sym);
int SYMINLIST(SYMBOL sym,SYMLIST * list);
void COPYLIST(SYMLIST * list1,SYMLIST * list2);

void error(int);//负责显示出错信息的函数
void INITIAL();//编译程序初始化
//void ENTERID();
//修改
void ENTERID(const char* name, OBJECT kind, TYPES type, int value); //向符号表中填入信息要用的函数
void ENTERPREID();//预先填入符号表的信息，将这些信息作为“过程零”里面的数据

void getSymbols(FILE *);//从源文件读入字符，获得符号链表
void getASymbol();//采用递归下降的语法分析，逐个的获取一个单词符号
void destroySymbols();//编译完毕，将符号链表释放
 
void GEN(OPCOD func,int level,int address);//将产生的保存到代码数组CODE里面
void WriteObjCode(char *); //将产生的代码写进*.pld文件里面（二进制形式）
void WriteCodeList(char *); //将产生的代码写进*.lst文件（可见字符形式）

void ENTERARRAY(SYMLIST * list,TYPES type,int low,int high);//向数组信息表填入一个表项
void ENTERBLOCK();//用来向程序体表填入一个表项以登记此过程体
void ENTER(OBJECT object);//在编译的过程中将遇到的符号填入符号表中
int GETPOSITION(char * id);//通过标识符的内容在名字表里面查找其索引
void CONSTANT(SYMLIST * list,CONSTREC & constRec);//从源程序里面获取一个常量类型的数据
void ARRAYTYP(SYMLIST * list,int & aref,int & arsz); //获取一个数组类型的信息，传引用参数arrayRef和arraySize
void TYP(SYMLIST * list,TYPES & tp,int & rf,int & sz);//获取（到名字表里面查找）当前的标识符的类型信息
void PARAMENTERLIST(SYMLIST * list);//编译某个过程的参数列表
SymbolItem* Symbols = NULL;
SymbolItem* CurSymbol = NULL;
int nError;

void CONSTDECLARATION(SYMLIST * list);//常量声明
void TYPEDECLARATION(SYMLIST * list);//类型声明
void VARDECLARATION(SYMLIST * list);//变量声明
void PROCDECLARATION(SYMLIST * list);//过程声明

void FACTOR(SYMLIST * list,TYPEITEM & typeItem);//获取下一个“因子”的信息，传引用参数typeItem用来保存信息
void TERM(SYMLIST * list,TYPEITEM & typeItem); //获取一个“项”的信息
void SIMPLEEXPRESSION(SYMLIST * list,TYPEITEM & typeItem);//获取简单表达式信息
void EXPRESSION(SYMLIST * list,TYPEITEM & typeItem);//获取表达式信息
void ARRAYELEMENT(SYMLIST * list,TYPEITEM & typeItem);//获取一个数组元素的信息

void ASSIGNMENT(SYMLIST * list);//分析赋值语句
void IFSTATEMENT(SYMLIST * list);//分析if语句
void WHILESTATEMENT(SYMLIST * list); //while 语句的分析
void COMPOUND(SYMLIST * list); //begin 开头的组合语句的分析
void STANDPROC(SYMLIST * list,int i);//两个基本过程read和write的调用，他们在符号表里面分别在第六项和第七项
void CALL(SYMLIST * list); //调用语句的分析
void STATEMENT(SYMLIST * list);  //普通语句的分析，这些语句有几种形式，分别以不同的标识符开头
void BLOCK(SYMLIST * list,int level); //过程体的分析

//添加
void REPEATSTATEMENT(SYMLIST* list);

////////////////////////////////////////////////////////////
////////////////////函数过程定义如下////////////////////////
////////////////////////////////////////////////////////////

void GEN(OPCOD func,int level,int address) //将产生的代码保存到代码数组CODE里面
{										   //并将代码索引CX增加 1	
	static int lineNumber=0;
	if(CX>MAXNUMOFCODEADDRESS)
	{
		printf("PROGRAM TOO LONG!");
		exit(0);
	}

	printf("%d\t",lineNumber);                 //下面三个语句用来在编译的过程中显示代码
	printf(ObjCodeScript[func],level,address); //注意：现实的代码不是完全的，但是可以帮助理解
	printf("\n");

	CODE[CX].lineNumber=lineNumber++;
	CODE[CX].func=func;
	CODE[CX].level=level;
	CODE[CX].address=address;
	CX++;
}



void WriteObjCode(char *filename)  //将产生的代码写进*.pld文件里面（二进制形式）
{
	FILE *fcode;
	
	fcode=fopen(filename,"wb");
	if(!fcode)
		error(40);  //不能打开.pld文件
	for(int i=0;i<CX;i++)
	{
		fwrite(&CODE[i].func,sizeof(OPCOD),1,fcode);
		fwrite(&CODE[i].level,sizeof(int),1,fcode);
		fwrite(&CODE[i].address,sizeof(int),1,fcode);
	}
	fclose(fcode);	
}

void WriteCodeList(char *filename)  //将产生的代码写进*.lst文件（可见字符形式）
{
	FILE *flist;
	
	flist=fopen(filename,"wb");
	if(!flist)
		error(39);  //不能打开.lst文件
	for(int i=0;i<CX;i++)
	{
		fprintf(flist,"%d\t",i);
		fprintf(flist,ObjCodeScript[CODE[i].func],CODE[i].level,CODE[i].address);
		fprintf(flist,"\n");
	}
	fclose(flist);
}

void WriteLabelCode(char *filename)  //将产生的代码写进*.lab文件（可见字符形式）
{
	FILE *flabel;
	
	flabel=fopen(filename,"wb");
	if(!flabel)
		error(41);  //不能打开.lst文件
	for(int i=0;i<JX;i++)
	{
		fprintf(flabel,"第%d项:\t",i+1);
		fprintf(flabel,"%d",JUMADRTAB[i]);
		fprintf(flabel,"\n");
	}
	fclose(flabel);
}

void ENTERARRAY(TYPES type,int low,int high)  //向数组信息表填入一个表项
{
	if(low>high)
	{
		error(19); //"数组上下大小关系界错误"
	}
	if (AX==MAXNUMOFARRAYTABLE)
	{
		error(24);  //数组表溢出
		printf("TOO LONG ARRAYS IN PROGRAM!");
	}
	else
	{
		AX++;
		ATAB[AX].intType=type;
		ATAB[AX].low=low;
		ATAB[AX].high=high;
	}
}

void ENTERBLOCK()           //每当编译一个过程开始时，保证调用该函数一次
{							//用来向程序体表填入一个表项以登记此过程体
	if(BX==MAXNUMOFBLOCKTABLE)
	{
		error(26);  //程序体表溢出
		printf("TOO MANY PROCEDURE IN PROGRAM!");
	}
	else
	{
		BX++;
		displayLevel++;  //编译到了一个过程，层次加一
		BTAB[BX].last=0;
		BTAB[BX].lastPar=0;
	}
}

void QUITBLOCK()        //每当一个过程编译完时，保证调用该函数一次
{
	displayLevel--;     //层次减一
}


void ENTER(OBJECT kind) //在编译的过程中将遇到的符号填入符号表中
{						//如果在同一层次内有两次定义性质的出现，则报错
	int j,l;
	if(TX==MAXNUMOFNAMETABLE)
	{
		error(25);  //名字表溢出
		printf("PROGRAM TOO LONG!");
	}
	else
	{
		strcpy(NAMETAB[0].name,CurSymbol->value.lpValue);    //当前标识符（注意：只能是标识符）的内容填入符号表的第一项？？？？？？？？？？
		j=BTAB[DISPLAY[displayLevel]].last;   //将临时变量j设成当前DISPLAY表项所指向的程序体表项的last域
		l=j;
		while(stricmp(NAMETAB[j].name,CurSymbol->value.lpValue))  
			j=NAMETAB[j].link;               //顺藤摸瓜
		if(j>0)
			error(31); //如果摸着了，则本程序体内符号重复定义
	    else
		{             //否则，填入相应的信息
			TX++;
			strcpy(NAMETAB[TX].name,CurSymbol->value.lpValue);
			NAMETAB[TX].link=l;
			NAMETAB[TX].kind=kind;
			NAMETAB[TX].type=NOTYP;
			NAMETAB[TX].ref=0;
			NAMETAB[TX].level=displayLevel;
			NAMETAB[TX].normal=0;
			switch(kind)
			{
			case VARIABLE:
			case PROCEDURE:
				NAMETAB[TX].unite.address=0;break;
			case KONSTANT:
				NAMETAB[TX].unite.value=0;break;
			case TYPEL:
				NAMETAB[TX].unite.size=0;break;
			}
			BTAB[DISPLAY[displayLevel]].last=TX;
		}		
	}
}

int GETPOSITION(char * id)  //通过标识符的内容在名字表里面查找其索引
{
	int i=0,j=displayLevel;
	strcpy(NAMETAB[0].name,id);
	//j=displayLevel;

	do              //在所有的活动记录（注意：是活动的）里面顺藤摸瓜
	{
		i=BTAB[DISPLAY[j]].last;
		while (stricmp(NAMETAB[i].name,CurSymbol->value.lpValue))
			i=NAMETAB[i].link;
		j--;        //一个记录里面没有摸到，则到上一级里面去
	}while( j>=0 && i==0);   
	if(i==0)      // 表示没有摸到
		error(33);  //没有定义符号
	return (int)i;
}

void CONSTANT(SYMLIST * list,CONSTREC & constRec)  //从源程序里面获取一个常量类型的数据
{												   //传引用参数constRec 用来记录获取的结果
	int x,sign;

	constRec.type=NOTYP;
	constRec.value=0;
	if(SYMINLIST(CurSymbol->type,&CONSTBEGSYS))
	{
		if(CurSymbol->type==CHARCON)
		{
			constRec.type=CHARS;
			constRec.value=CurSymbol->value.iValue;
			getASymbol();
		}
		else
		{
			sign=1;       //将常量的符号默认置为正
			if(CurSymbol->type==PLUS || CurSymbol->type==MINUS)
			{
				if(CurSymbol->type==MINUS)
					sign=-1;  //置为负
				getASymbol();
			}

			if(CurSymbol->type==IDENT)
			{
				x=GETPOSITION(CurSymbol->value.lpValue);  //查表
				if(x!=0)
				{
					if(NAMETAB[x].kind!=KONSTANT)
						error(17);// 应该是常量或者常量标识符
					else
					{
						constRec.type=NAMETAB[x].type;
						constRec.value=sign*NAMETAB[x].unite.value;
					}
				}
				getASymbol();
			}
			else if(CurSymbol->type==INTCON)
			{
				constRec.type=INTS;
				constRec.value=sign*CurSymbol->value.iValue;
				getASymbol();
			}
		}
	}
}

void ARRAYTYP(SYMLIST * list,int & arrayRef,int & arraySize)  //获取一个数组类型的信息，传引用参数arrayRef和arraySize
{																  //用来记录从数组获取的引用信息和大小信息	
	TYPES eleType;
	CONSTREC low,high;
	int eleRef,eleSize;

	//////////////////////////////////////////////////////////
	SYMLIST * tempList=new SYMLIST;
	tempList->AddHead(COLON);	tempList->AddHead(RBRACK);
	tempList->AddHead(RPAREN);	tempList->AddHead(OFSYM);
	CONSTANT(tempList,low);         //获得数组下标的下限
	delete tempList;
	//////////////////////////////////////////////////////////

	if(low.type!=INTS && low.type!=CHARS)
		error(20);  //下标元素类型错误
    if(CurSymbol->type==DPOINT)
		getASymbol();
	else
		error(42);//应该是'..'

	//////////////////////////////////////////////////////////
    SYMLIST * tempList1=new SYMLIST;
	tempList1->AddHead(COMMA);	tempList1->AddHead(RBRACK);
	tempList1->AddHead(RPAREN);	tempList1->AddHead(OFSYM);
	CONSTANT(tempList1,high);        //获得数组下标的上限
	delete tempList1;
	//////////////////////////////////////////////////////////

	if(low.type!=high.type)
	{
		error(21);  //上下界的类型不一致
		high.value=low.value;
	}

	ENTERARRAY(low.type,low.value,high.value);   //向数组信息表登记

	arrayRef=AX;
	if(CurSymbol->type==COMMA)   //如果当前符号是逗号，表示数组是多维的
	{
		getASymbol();
		eleType=ARRAYS;    //数组的“子维”的类型是数组
		ARRAYTYP(list,eleRef,eleSize);   //获取较低维的数组的信息
	}
	else
	{
		if(CurSymbol->type==RBRACK)
			getASymbol();
		else
		{
			error(5);  //应该是']'
			if(CurSymbol->type==RPAREN)
				getASymbol();
		}

		if(CurSymbol->type==OFSYM)
			getASymbol();
		else
			error(10);  //应该是'of'
		TYP(list,eleType,eleRef,eleSize);   //获取数组元素的类型
	}
	arraySize=(high.value-low.value+1)*eleSize;   //计算本数组类型的大小
	ATAB[arrayRef].size=arraySize;
	ATAB[arrayRef].eleType=eleType;          //填信息
	ATAB[arrayRef].elRef=eleRef;
	ATAB[arrayRef].elSize=eleSize;
}


void TYP(SYMLIST * list,TYPES & type,int & ref,int & size)//获取（到名字表里面查找）当前的标识符的类型信息
{	
	int x;		

	type=NOTYP;
	ref=0;
	size=0;
	if(SYMINLIST(CurSymbol->type,&TYPEBEGSYS))
	{
		if(CurSymbol->type==IDENT)
		{
			x=GETPOSITION(CurSymbol->value.lpValue);   //查表
			if(x!=0)
			{
				if(NAMETAB[x].kind!=TYPEL)
				{
					error(15);  //应该是类型标识符
				}
				else
				{
					type=NAMETAB[x].type;
					ref=NAMETAB[x].ref;
					size=NAMETAB[x].unite.size;
					if(type==NOTYP)
						error(36);  //类型定义出错
				}
				getASymbol();
			}
		}
		else if (CurSymbol->type==ARRAYSYM)  //如果是数组类型
		{
			getASymbol();
			if(CurSymbol->type==LBRACK)
				getASymbol();
			else
			{
				error(4);  //应该是'['
				if(CurSymbol->type==LPAREN)
					getASymbol();
			}
			type=ARRAYS;
			ARRAYTYP(list,ref,size);  //返回数组类型的信息
		}
	}
}


void PARAMENTERLIST(SYMLIST * list)  //编译某个过程的参数列表
{
	TYPES type;	
	int ref,size,x,helper;

	type=NOTYP;
	ref=0;
	size=0;

	getASymbol();	
	while(CurSymbol->type==IDENT || CurSymbol->type==VARSYM)
	{
		int valuePar=0;  //默认是变量参数
		if(CurSymbol->type!=VARSYM)
			valuePar=1;  //如果当前符号不是var则表明是值参数
		else
			getASymbol();
			
		helper=TX;
		if(CurSymbol->type==IDENT)
		{
			ENTER(VARIABLE);  //编译到一个变量，将其信息填入符号表
			getASymbol();
		}
		else 
			error(14);  //应该是标识符

		while(CurSymbol->type==COMMA)  //如果当前符号是逗号，则说明是一个参数列表，继续读入下一个标识符
		{
			getASymbol();
			if(CurSymbol->type==IDENT)
			{
				ENTER(VARIABLE);
				getASymbol();
			}
			else
				error(14);  //应该是标识符
		}

		if(CurSymbol->type==COLON)  //如果是':'，则后面应该是类型标识符，用来声明变量
		{
			getASymbol();
			if(CurSymbol->type!=IDENT)
				error(14);  //应该是标识符
			else
			{
				x=GETPOSITION(CurSymbol->value.lpValue);
				getASymbol();
				if(x!=0)
				{
					if(NAMETAB[x].kind!=TYPEL)
						error(15);  //应该是类型标识符
					else
					{
						type=NAMETAB[x].type;
						ref=NAMETAB[x].ref;
						if(valuePar)
							size=NAMETAB[x].unite.size;
						else
							size=1;
					}
				}
			}
		}
		else
			error(0);  //应该是':'

		while(helper<TX)  //将编译识别出来的变量的信息填入符号表
		{
			helper++;
			NAMETAB[helper].type=type;
			NAMETAB[helper].ref=ref;
			NAMETAB[helper].unite.address=DX;
			NAMETAB[helper].level=displayLevel;
			NAMETAB[helper].normal=valuePar;
			DX+=size;   //DX用来记录已经分配的局部空间的大小
		}

		if(CurSymbol->type!=RPAREN)
		{
			if(CurSymbol->type==SEMICOLON)
				getASymbol();
			else
			{	
				error(1);  //应该是';'
				if(CurSymbol->type==COMMA)
					getASymbol();
			}
		}
	}
	
	if(CurSymbol->type==RPAREN)
	{
		getASymbol();
	}
	else
		error(2);  //应该是')'
}

void CONSTDECLARATION(SYMLIST * list)  //常量声明
{
	CONSTREC constRec;

	if(CurSymbol->type==IDENT)
	{
		ENTER(KONSTANT);   //被声明的常量标识符
		getASymbol();
		if(CurSymbol->type==EQL)
			getASymbol();
		else
		{
			error(6);  //应该是'='
			if(CurSymbol->type==BECOMES)
				getASymbol();
		}

		////////////////////////////////////////////////////////////////
		SYMLIST * tempList1=new SYMLIST;
		COPYLIST(tempList1,listAddSym(listAddSym(listAddSym(list,SEMICOLON),COMMA),IDENT));
		CONSTANT(tempList1,constRec);   //获取下一个常量的信息
		delete tempList1;
		////////////////////////////////////////////////////////////////

		NAMETAB[TX].type=constRec.type;    //填表
		NAMETAB[TX].ref=0;
		NAMETAB[TX].unite.value=constRec.value;
		if(CurSymbol->type==SEMICOLON)
			getASymbol();
		else
			error(1);//应该是';'
	}
	else
		error(14);//应该是标识符
}

void TYPEDECLARATION(SYMLIST * list)  //类型声明
{
	TYPES type;
	int ref,size,helper;

	if(CurSymbol->type==IDENT)
	{
		ENTER(TYPEL);  //被声明的类型标识符
		helper=TX;	
		getASymbol();

		if(CurSymbol->type==EQL)
			getASymbol();
		else
		{
			error(6);//应该是'='
			if(CurSymbol->type==SEMICOLON)
				getASymbol();
		}

		//////////////////////////////////////////////////////////////
		SYMLIST * tempList=new SYMLIST;
		COPYLIST(tempList,listAddSym(listAddSym(listAddSym(list,SEMICOLON),COMMA),IDENT));
		TYP(tempList,type,ref,size);    //获取类型信息
		delete tempList;
		//////////////////////////////////////////////////////////////

		NAMETAB[TX].type=type;         //填表
		NAMETAB[TX].ref=ref;
		NAMETAB[TX].unite.size=size;

		if(CurSymbol->type==SEMICOLON)
			getASymbol();
		else
			error(1);//应该是';'
	}
	else
		error(14);//应该是标识符
}

void VARDECLARATION(SYMLIST * list)   //变量声明
{
	TYPES type;
	int ref,size,helper1,helper2;

	if(CurSymbol->type==IDENT)
	{
		helper1=TX;
		ENTER(VARIABLE);   //被声明的变量
		getASymbol();
		while(CurSymbol->type==COMMA)  //如果是逗号，则面临的是同类型的一串变量列表
		{
			getASymbol();
			if(CurSymbol->type==IDENT)
			{
				ENTER(VARIABLE);  //被声明的变量
				getASymbol();
			}
			else
				error(14);//应该是标识符
		}

		if(CurSymbol->type==COLON)
			getASymbol();
		else
			error(0);//应该是':'

		helper2=TX;

	    //////////////////////////////////////////////////////////
		SYMLIST * tempList=new SYMLIST;
		COPYLIST(tempList,listAddSym(listAddSym(listAddSym(list,SEMICOLON),COMMA),IDENT));
		TYP(tempList,type,ref,size);  //获取变量的类型信息
		delete tempList;
		//////////////////////////////////////////////////////////

		while(helper1<helper2)  //将一串变量的信息填表
		{
			helper1++;
			NAMETAB[helper1].type=type;
			NAMETAB[helper1].ref=ref;
			NAMETAB[helper1].level=displayLevel;
			NAMETAB[helper1].unite.address=DX;
			NAMETAB[helper1].normal=1;
			DX+=size;
		}

		if (CurSymbol->type == SEMICOLON)
			getASymbol();
		else
			error(1);//应该是';'
	}
	else
		error(14);//应该是标识符
}

void PROCDECLARATION(SYMLIST * list)  //过程声明
{
	getASymbol();
	if(CurSymbol->type!=IDENT)
	{
		error(14);  //应该是标识符
		strcpy(CurSymbol->value.lpValue,"");
	}

	ENTER(PROCEDURE);  //被声明的过程名
	NAMETAB[TX].normal=1;
	getASymbol();

	///////////////////////////////////////////////////////////
	SYMLIST * tempList1=new SYMLIST;
	COPYLIST(tempList1,listAddSym(list,SEMICOLON));
	BLOCK(tempList1,displayLevel);   //编译过程体
	delete tempList1;
	///////////////////////////////////////////////////////////

	if(CurSymbol->type==SEMICOLON)
		getASymbol();
	else
		error(1);//应该是';'
}

void error(int errCode)   //负责显示出错信息的函数
{
	char errorScript[][100]=
	{
		"应该是\':\'",//0
        "应该是\';\'",//1
		"应该是\')\'",//2
		"应该是\'(\'",//3
		"应该是\'[\'",//4
		"应该是\']\'",//5
		"应该是\'=\'",//6
		"应该是\':=\'",//7
		"应该是\'.\'",//8
		"应该是\'do\'",//9
		"应该是\'of\'",//10
		"应该是\'then\'",//11
		"应该是\'end\'",//12
		"应该是\'program\'",//13
		"应该是标识符",//14
		"应该是类型标识符",//15
		"应该是变量",//16
		"应该是常量或常量标识符",//17
		"应该是过程名",//18
		"数组上下界大小关系错误",//19
		"数组定义时，下标元素类型错误",//20
		"数组定义时，上下界类型不一致",//21
		"引用时，数组元素下标元素类型出错",//22
		"下标个数不正确",//23
		"数组表溢出",//24
		"名字表溢出",//25
		"程序体表溢出",//26
		"系统为本编译程序分配的堆不够用",//27
		"实参与形参个数不等",//28
		"实参与形参类型不一致",//29
		"实参个数不够",//30
		"程序体内符号重定义",//31
		"if或while for后面表达式必须为布尔类型",//32
		"标识符没有定义",//33
		"过程名或类型名不能出现在表达式中",//34
		"操作数类型错误",//35
		"类型定义出错",//36
		"类型不一致",//37
		"不认识的字符出现在源程序",//38
		"不能打开.lst文件",//39
		"不能打开.pld文件",//40
		"不能打开.lab文件",//41
		"应该是\'..\'",//42
		"分析时缺少标识符",//43
		"for后面必须时赋值语句",//44
		"应该是\'to\'",//45
		"应该是\'until\'",//46

};

	if(CurSymbol && CurSymbol->lineNumber>0)
		printf("\n<<<<<<  Line number %d ",CurSymbol->lineNumber);
	printf("found error %d : %s !  >>>>>>\n\n",errCode,errorScript[errCode]);
	nError++;
	
}


void FACTOR(SYMLIST * list,TYPEITEM & typeItem)  //获取下一个“因子”的信息，传引用参数typeItem用来保存信息
{
	int i;

	typeItem.typ=NOTYP;
	typeItem.ref=0;

	////////////////////////////////////////////////////////
	SYMLIST * tempList2=new SYMLIST;
	COPYLIST(tempList2,listAddSym(list,RPAREN));	
	////////////////////////////////////////////////////////

	while(SYMINLIST(CurSymbol->type,&FACBEGSYS))
	{
		switch(CurSymbol->type)
		{
		case IDENT:
			i=GETPOSITION(CurSymbol->value.lpValue);  //如果是标识符，则查表
			getASymbol();
			if(i!=0)
			{
				switch(NAMETAB[i].kind)
				{
				case KONSTANT:
					typeItem.typ=NAMETAB[i].type;
					typeItem.ref=0;
					GEN(LIT,0,NAMETAB[i].unite.value);
					break;
				case VARIABLE:
					typeItem.typ=NAMETAB[i].type;
					typeItem.ref=NAMETAB[i].ref;
					if(NAMETAB[i].type==INTS || NAMETAB[i].type==BOOLS || NAMETAB[i].type==CHARS)
					{
						if(NAMETAB[i].normal)
							GEN(LOD,NAMETAB[i].level,NAMETAB[i].unite.address);
						else
							GEN(ILOD,NAMETAB[i].level,NAMETAB[i].unite.address);
					}
					else
					{
						if(NAMETAB[i].type==ARRAYS)
						{
							if(NAMETAB[i].normal)
								GEN(LODA,NAMETAB[i].level,NAMETAB[i].unite.address);
							else
								GEN(LOD,NAMETAB[i].level,NAMETAB[i].unite.address);
							if(CurSymbol->type==LBRACK)
								ARRAYELEMENT(list,typeItem);
							if(typeItem.typ!=ARRAYS)
								GEN(LODT,0,0);
						}
					}
					break;
				case PROCEDURE:
				case TYPEL:
					error(34);  //过程名或类型名不能出现在表达始中
					break;
				}
			}
			break;

		case INTCON:
		case CHARCON:
			if(CurSymbol->type==INTCON)
				typeItem.typ=INTS;
			else
				typeItem.typ=CHARS;
			typeItem.ref=0;
			GEN(LIT,0,CurSymbol->value.iValue);
			getASymbol();
			break;
		case LPAREN:  //如果是左括号
			getASymbol();
			EXPRESSION(tempList2,typeItem);  //获取一个“表达式”的信息
			if(CurSymbol->type==RPAREN)
				getASymbol();
			else 
				error(2); //应该是')'
			break;
		case NOTSYM:   //如果是“非”
			getASymbol();
			FACTOR(list,typeItem);   //获取一个“因子”的信息
			if(typeItem.typ==BOOLS)
				GEN(NOTS,0,0);
			else
				error(35);  //操作数类型错误
			break;
		}

	}
	delete tempList2;
}


void TERM(SYMLIST * list,TYPEITEM & typeItem)  //获取一个“项”的信息
{
	SYMBOL mulop;
	TYPEITEM helpTypeItem;

	///////////////////////////////////////////////////////////
	SYMLIST * tempList=new SYMLIST;
	SYMLIST * tempList1=new SYMLIST;
	tempList1->AddHead(TIMES);	tempList1->AddHead(DIVSYM);	tempList1->AddHead(MODSYM);	tempList1->AddHead(ANDSYM);
	COPYLIST(tempList,listsAdd(tempList1,list));
	FACTOR(tempList,typeItem);    //获取一个“因子”的信息
	///////////////////////////////////////////////////////////

	while(SYMINLIST(CurSymbol->type,tempList1))
	{
		mulop=CurSymbol->type;  
		getASymbol();
		FACTOR(tempList,helpTypeItem);   //获取一个“因子”的信息
		if(typeItem.typ!=helpTypeItem.typ)
		{
			error(37);//类型不一致
			typeItem.typ=NOTYP;
			typeItem.ref=0;
		}
		else
		{
			switch(mulop)
			{
			case TIMES:
				if(typeItem.typ==INTS)
					GEN(MULT,0,0);
				else
					error(35);//操作数类型错误
				break;
			case DIVSYM:
				if(typeItem.typ==INTS)
					GEN(IDIV,0,0);
				else
					error(35);//操作数类型错误
				break;
			case MODSYM:
				if(typeItem.typ==INTS)
					GEN(IMOD,0,0);
				else
					error(35);//操作数类型错误
				break;
			case ANDSYM:
				if(typeItem.typ==INTS)
					GEN(ANDS,0,0);
				else
					error(35);//操作数类型错误
				break;
			}
		}
	}
	delete tempList;delete tempList1;
}

void SIMPLEEXPRESSION(SYMLIST * list,TYPEITEM & typeItem)  //获取一个“简单表达式”的信息
{
	SYMBOL addop;
	TYPEITEM helpTypeItem;

	/////////////////////////////////////////////////
	SYMLIST * tempList=new SYMLIST;
	SYMLIST * tempList1=new SYMLIST;
	tempList->AddHead(PLUS);	tempList->AddHead(MINUS);	tempList->AddHead(ORSYM);
	COPYLIST(tempList1,listsAdd(tempList,list));
	/////////////////////////////////////////////////

	if(CurSymbol->type==PLUS || CurSymbol->type==MINUS)
	{
		addop=CurSymbol->type;
		getASymbol();
		TERM(tempList1,typeItem);  //获取一个“项”的信息
		if(addop==MINUS)
			GEN(MUS,0,0);
	}
	else
		TERM(tempList1,typeItem);   //获取一个“项”的信息

	while(SYMINLIST(CurSymbol->type,tempList))
	{
		addop=CurSymbol->type;
		getASymbol();
		TERM(tempList1,helpTypeItem);   //获取一个“项”的信息
		if(typeItem.typ!=helpTypeItem.typ)  //如果两个项的信息发生冲突
		{
			error(37);//类型不一致
			typeItem.typ=NOTYP;
			typeItem.ref=0;
		}
		else
		{
			switch(addop)
			{
			case PLUS:
				if(typeItem.typ==INTS)
					GEN(ADD,0,0);
				else
					error(35);//操作数类型错误
				break;
			case MINUS:
				if(typeItem.typ==INTS)
					GEN(SUB,0,0);
				else
					error(35);//操作数类型错误
				break;
			case ORSYM:
				if(typeItem.typ==BOOLS)
					GEN(ORS,0,0);
				else
					error(35);//操作数类型错误
				break;
			}
		}
	}
	delete tempList;delete tempList1;
}

void EXPRESSION(SYMLIST * list,TYPEITEM & typeItem)   //获取一个“表达式”的信息
{
	SYMBOL relationop;
	TYPEITEM helpTypeItem;

	/////////////////////////////////////////////////////////////////
	SYMLIST * tempList=new SYMLIST;
	SYMLIST * tempList1=new SYMLIST;
	tempList->AddHead(EQL);	tempList->AddHead(NEQ);	tempList->AddHead(LSS);
	tempList->AddHead(GTR);	tempList->AddHead(LEQ);	tempList->AddHead(GEQ);
	COPYLIST(tempList1,listsAdd(tempList,list));
	SIMPLEEXPRESSION(tempList1,typeItem);  //获取一个“简单表达式”的信息
	/////////////////////////////////////////////////////////////////

	while(SYMINLIST(CurSymbol->type,tempList))
	{
		relationop=CurSymbol->type;
		getASymbol();
		SIMPLEEXPRESSION(list,helpTypeItem);   ///获取一个“简单表达式”的信息
		if(typeItem.typ!=helpTypeItem.typ)
			error(37);//类型不一致
		else
		{
			switch(relationop)
			{
			case EQL: GEN(EQ,0,0);break;
			case NEQ: GEN(NE,0,0);break;
			case LSS: GEN(LS,0,0);break;
			case GEQ: GEN(GE,0,0);break;
			case GTR: GEN(GT,0,0);break;
			case LEQ: GEN(LE,0,0);break;
			}
		}
		typeItem.typ=BOOLS;
	}
	delete tempList;delete tempList1;
}



void ARRAYELEMENT(SYMLIST * list,TYPEITEM & typeItem)  //获取一个数组元素的信息
{
	int p;
	TYPEITEM helpTypeItem;

	p=typeItem.ref;
	if(CurSymbol->type==LBRACK)
	{
		SYMLIST * tempList=new SYMLIST;
		COPYLIST(tempList,listAddSym(list,COMMA));
		do
		{
			getASymbol();
			EXPRESSION(tempList,helpTypeItem);   //引用数组元素时所使用的数组下标的信息
			if(typeItem.typ!=ARRAYS)
				error(23);  //下标个数不正确
			else
			{
				if(helpTypeItem.typ!=ATAB[p].intType)
					error(22);  //数组元素引用时，下标元素类型出错
				GEN(LIT,0,ATAB[p].low);    //下面是产生的计算数组元素地址的代码
				GEN(SUB,0,0);
				GEN(LIT1,0,ATAB[p].elSize);
				GEN(MULT,0,0);
				GEN(ADD1,0,0);
				typeItem.typ=ATAB[p].eleType;
				typeItem.ref=ATAB[p].elRef;
				p=ATAB[p].elRef;
			}
		}
		while(CurSymbol->type==COMMA);  //多维数组

		if(CurSymbol->type==RBRACK)
			getASymbol();
		else
			error(5);//应该是']'
	}
	else
		error(4);//应该是'['	
}


void INITIAL()  //编译程序初始化所作的一些工作
{
	nError=0;
	displayLevel=0;
	DISPLAY[0]=0;

	DX=0;
	CX=0;
	BX=1;
	TX=-1;
	JX=0;

	

	// FIVE SYMLISTS' INITIALIZATION

	DECLBEGSYS.AddHead(CONSTSYM);	DECLBEGSYS.AddHead(VARSYM);
	DECLBEGSYS.AddHead(TYPESYM);	DECLBEGSYS.AddHead(PROCSYM);

	STATBEGSYS.AddHead(BEGINSYM);	STATBEGSYS.AddHead(CALLSYM);
	STATBEGSYS.AddHead(IFSYM);	STATBEGSYS.AddHead(WHILESYM);

	STATBEGSYS.AddHead(REPEATSYM); STATBEGSYS.AddHead(FORSYM); STATBEGSYS.AddHead(CASESYM);

	FACBEGSYS.AddHead(IDENT);	FACBEGSYS.AddHead(INTCON);	FACBEGSYS.AddHead(LPAREN);
	FACBEGSYS.AddHead(NOTSYM);	FACBEGSYS.AddHead(CHARCON);

	TYPEBEGSYS.AddHead(IDENT);	TYPEBEGSYS.AddHead(ARRAYSYM);

	CONSTBEGSYS.AddHead(PLUS);	CONSTBEGSYS.AddHead(MINUS);	CONSTBEGSYS.AddHead(INTCON);
	CONSTBEGSYS.AddHead(CHARCON);	CONSTBEGSYS.AddHead(IDENT);
}


void ENTERID(const char * name,OBJECT kind,TYPES type,int value)  //向符号表中填入信息要用的函数
{
	TX++;
	strcpy(NAMETAB[TX].name,name);
	NAMETAB[TX].link=TX-1;
	NAMETAB[TX].kind=kind;
	NAMETAB[TX].type=type;
	NAMETAB[TX].ref=0;
	NAMETAB[TX].normal=1;
	NAMETAB[TX].level=0;
	switch(kind)
	{
	case VARIABLE:
	case PROCEDURE:
		NAMETAB[TX].unite.address=value;break;
	case KONSTANT:
		NAMETAB[TX].unite.value=value;break;
	case TYPEL:
		NAMETAB[TX].unite.size=value;break;
	}
	return;
}

void ENTERPREID()  //预先填入符号表的信息，将这些信息作为“过程零”里面的数据
{
	ENTERID("",VARIABLE,NOTYP,0);
	//ENTERID("real",TYPEL,REALS,1);
	ENTERID("char",TYPEL,CHARS,1);
	ENTERID("integer",TYPEL,INTS,1);
	ENTERID("boolean",TYPEL,BOOLS,1);
	ENTERID("false",KONSTANT,BOOLS,0);
	ENTERID("true",KONSTANT,BOOLS,1);
	ENTERID("read",PROCEDURE,NOTYP,1);
	ENTERID("write",PROCEDURE,NOTYP,2);

	BTAB[0].last=TX;
	BTAB[0].lastPar=1;
	BTAB[0].pSize=0;
	BTAB[0].vSize=0;
}

void ASSIGNMENT(SYMLIST * list)  //分析赋值语句
{
	TYPEITEM typeItem1,typeItem2;
	int i;

	i=GETPOSITION(CurSymbol->value.lpValue);
	if(i==0)
		error(33);   //标识符没有定义
	else
	{
		if (NAMETAB[i].kind!=VARIABLE)  //':='左边应该是变量
		{
			error(16);  //应该是变量
            i=0;
		}
        getASymbol();
        typeItem1.typ=NAMETAB[i].type;
        typeItem1.ref=NAMETAB[i].ref;
		typeItem2.typ = NAMETAB[i].type;
		typeItem2.ref = NAMETAB[i].ref;
        if(NAMETAB[i].normal)
			GEN(LODA,NAMETAB[i].level,NAMETAB[i].unite.address); //变量地址入栈
		else
			GEN(LOD,NAMETAB[i].level,NAMETAB[i].unite.address);  //变量值入栈

		if(CurSymbol->type==LBRACK)  //如果当前标识符是左大括号，那么正在为一个数组的元素赋值
		{
			//////////////////////////////////////////////////////////
			SYMLIST * tempList=new SYMLIST;
			COPYLIST(tempList,listAddSym(list,BECOMES));
            ARRAYELEMENT(tempList,typeItem1);  //获得此数组元素的信息
			delete tempList;
            //////////////////////////////////////////////////////////
		}
			
		if (CurSymbol->type == BECOMES)
		{
			getASymbol();
			EXPRESSION(list, typeItem2);//':='右边是表达式
		}	
//添加		
		else if (CurSymbol->type == PLUSBECOMES)
		{
			GEN(LOD, NAMETAB[i].level, NAMETAB[i].unite.address);
			getASymbol();
			EXPRESSION(list, typeItem2);
			GEN(ADD, 0, 0);
		}
		else if (CurSymbol->type == MINUSBECOMES)
		{
			GEN(LOD, NAMETAB[i].level, NAMETAB[i].unite.address);
			getASymbol();
			EXPRESSION(list, typeItem2);
			GEN(SUB, 0, 0);
		}
		else
		{
			error(7);  //应该是':='
            if(CurSymbol->type==EQL)
				getASymbol();
		}
		// EXPRESSION(list,typeItem2);  //':='右边是表达式
		if (typeItem1.typ != typeItem2.typ)
			error(37);//类型不一致
		else
		{
			if(typeItem1.typ==ARRAYS)
			{
				if(typeItem1.ref==typeItem2.ref)
					GEN(CPYB,0,ATAB[typeItem1.ref].size);
				else
					error(35);//操作数类型出错
			}
			else
				GEN(STO,0,0);
		}
	}
}


void IFSTATEMENT(SYMLIST * list)  //分析if语句
{
	TYPEITEM typeItem;
	int fillBackFalse,fillBackTrue;

	getASymbol();
	
	/////////////////////////////////////////////////////////////////
	SYMLIST * tempList=new SYMLIST;
	COPYLIST(tempList,listAddSym(listAddSym(list,DOSYM),THENSYM));
	EXPRESSION(tempList,typeItem);  //获取布尔表达式的信息
	delete tempList;
	/////////////////////////////////////////////////////////////////

	if(typeItem.typ!=BOOLS)
		error(32);  //if或while后面的表达式类型必须为布尔类型
	if(CurSymbol->type==THENSYM)
		getASymbol();
	else 
		error(11);  //应该是'then'

	fillBackFalse=CX;  //回填
	GEN(JPC,0,0);

	/////////////////////////////////////////////////////////////////
	SYMLIST * tempList1=new SYMLIST;
	COPYLIST(tempList1,listAddSym(list,ELSESYM));
	STATEMENT(tempList1);  //语句分析
	delete tempList1;
	/////////////////////////////////////////////////////////////////

	if(CurSymbol->type==ELSESYM)
	{
		getASymbol();
		fillBackTrue=CX;  //回填
		GEN(JMP,0,0);
		CODE[fillBackFalse].address=CX;
		JUMADRTAB[JX]=CX;
		JX++;
		STATEMENT(list);
		CODE[fillBackTrue].address=CX;
		JUMADRTAB[JX]=CX;
		JX++;
	}
	else
	{
		CODE[fillBackFalse].address=CX;
		JUMADRTAB[JX]=CX;
		JX++;
	}
}


void WHILESTATEMENT(SYMLIST * list)  //while 语句的分析
{
	TYPEITEM typeItem;
	int jumpback,fillBackFalse;

	getASymbol();
	JUMADRTAB[JX]=CX;
	JX++;
	jumpback=CX;  //纪录while循环执行时需要往前返回的地址

	/////////////////////////////////////////////////////////////////
	SYMLIST * tempList=new SYMLIST;
	COPYLIST(tempList,listAddSym(list,DOSYM));
	EXPRESSION(tempList,typeItem);
	delete tempList;
	/////////////////////////////////////////////////////////////////

	if(typeItem.typ!=BOOLS)
		error(32);  //if或while后面的表达式类型应该是布尔类型
	fillBackFalse=CX;  //条件测试失败后从哪里往循环“外”跳
	GEN(JPC,0,0);
	if(CurSymbol->type==DOSYM)
		getASymbol();
	else
		error(9);//应该是'do'

	STATEMENT(list);  //分析语句
	GEN(JMP,0,jumpback);
	CODE[fillBackFalse].address=CX;  //回填
	JUMADRTAB[JX]=CX;
	JX++;
}


// FOR <循环变量> := <初值> TO/DOWNTO <终值> DO
//   BEGIN
//     <循环体>
//   END;

void FORSTATEMENT(SYMLIST * list)  //for 语句的分析
{
	TYPEITEM typeItem;
	int jumpback,fillBackFalse;

	getASymbol();
	JUMADRTAB[JX]=CX;
	JX++;

	if(CurSymbol->type != IDENT)
	{
		printf("CurSymbol->%d\n",CurSymbol->type);
		error(44);//应该是赋值语句
	}
	ASSIGNMENT(list); //分析赋值语句

	if(CurSymbol->type==TOSYM) //分析TO
		getASymbol();
	else
	{
		printf("CurSymbol->%d\n",CurSymbol->type);	
		error(45);//应该是'to'
	}
	// if (GETPOSITION(CurSymbol->value.lpValue) == 0) error(33); //检测标识符

	jumpback=CX;  //纪录for循环执行时需要往前返回的地址
	/////////////////////////////////////////////////////////////////
	SYMLIST * tempList=new SYMLIST;
	COPYLIST(tempList,listAddSym(list,DOSYM));
	EXPRESSION(tempList,typeItem);
	delete tempList;
	/////////////////////////////////////////////////////////////////
	if(typeItem.typ!=BOOLS)
		error(32);  //if或while后面的表达式类型应该是布尔类型
	fillBackFalse=CX;  //条件测试失败后从哪里往循环“外”跳
	GEN(JPC,0,0);
	if(CurSymbol->type==DOSYM)
		getASymbol();
	else
		error(9);//应该是'do'

	STATEMENT(list);  //分析语句
	GEN(JMP,0,jumpback);
	CODE[fillBackFalse].address=CX;  //回填
	JUMADRTAB[JX]=CX;
	JX++;
}

// REPEAT
//     <循环体>
// UNTIL <布尔表达式>


void REPEATSTATEMENT(SYMLIST * list)  //repeat 语句的分析
{
	TYPEITEM typeItem;
	int jumpback,fillBackFalse;

	getASymbol();
	JUMADRTAB[JX]=CX;
	JX++;
	
	jumpback=CX;  //纪录while循环执行时需要往前返回的地址

	STATEMENT(list);  //分析语句
	
	if(CurSymbol->type==UNTILSYM) //分析until
		getASymbol();
	else
	{
		// printf("CurSymbol->%d\n",CurSymbol->type);	
		error(46);//应该是'until'
	}
	/////////////////////////////////////////////////////////////////
	SYMLIST * tempList=new SYMLIST;
	COPYLIST(tempList,listAddSym(list,UNTILSYM));
	EXPRESSION(tempList,typeItem); //bool 
	delete tempList;
	/////////////////////////////////////////////////////////////////
	if(typeItem.typ!=BOOLS)
		error(32);  //if或while后面的表达式类型应该是布尔类型

	fillBackFalse=CX;  //条件测试失败后从哪里往循环“外”跳
	GEN(JPC,0,0);
	GEN(JMP,0,jumpback);
	CODE[fillBackFalse].address=CX;  //回填
	JUMADRTAB[JX]=CX;
	JX++;
}

// CASE oper OF
//   '+' : result := x+y;
//   '-' : result := x-y;
//   '*' : result := x*y;
//   '/' : result := x/y; 
// END

void CASESTATEMENT(SYMLIST * list)  //case 语句的分析
{
	TYPEITEM typeItem;
	int level, CX1, jumpback, CX3;
	jumpback = CX;
	JUMADRTAB[JX] = CX;
	JX++;
	
	getASymbol(); //过掉“case”

	/////////////////////////////////////////////////////
	SYMLIST* tempList1 = new SYMLIST;
	COPYLIST(tempList1, listAddSym(list, OFSYM));
	EXPRESSION(tempList1, typeItem);
	delete tempList1;
	/////////////////////////////////////////////////////
	
	CX3 = CX;
	
	if (CurSymbol->type != OFSYM) {
		error(10);
	}
	/*记录 case 语句中情况的个数*/
	int case_num = 0;
	int caseN[100];

	memset(caseN, 0, sizeof caseN);
	do
	{
		int temp = jumpback;
		while (temp < CX3) {
			CODE[CX] = CODE[temp];
			temp++;
			CX++;
		}
		getASymbol();
		if (CurSymbol->type == ENDSYM) {
			getASymbol(); //识别到了end，过掉end
			break;
		}

		///////////////////////////////////////////////////////////////////////
		SYMLIST* tempList1 = new SYMLIST;
		COPYLIST(tempList1, listAddSym(list, SEMICOLON));
		SIMPLEEXPRESSION(tempList1, typeItem);
		delete tempList1;
		///////////////////////////////////////////////////////////////////////
		
		if (CurSymbol->type != COLON) error(0); //处理冒号
		getASymbol();

		GEN(EQ, 0, 0);
		CX1 = CX;
		GEN(JPC, 0, 0);

		STATEMENT(list); //识别冒号后的语句
		caseN[case_num++] = CX;
		GEN(JMP, 0, 0);
		CODE[CX1].address = CX;
	} while (CurSymbol->type == SEMICOLON);//是分号，继续执行

	for (int k = 0; k < case_num; k++){
	     CODE[caseN[k]].address = CX; //每条语句成功后都跳到结尾
		 JUMADRTAB[JX] = CX;
		 JX++;
    }
	
}

void COMPOUND(SYMLIST * list)  //begin 开头的组合语句的分析
{
	getASymbol();

    ////////////////////////////////////////////////////////////
	SYMLIST * tempList=new SYMLIST;
	COPYLIST(tempList,listAddSym(listAddSym(list,ENDSYM),SEMICOLON));
	STATEMENT(tempList);
	
	////////////////////////////////////////////////////////////
	SYMLIST * tempList1=new SYMLIST;
	COPYLIST(tempList1,listAddSym(&STATBEGSYS,SEMICOLON));
	///////////////////////////////////////////////////////////

	while(SYMINLIST(CurSymbol->type,tempList1))  //一条条的分析语句
	{
		if(CurSymbol->type==SEMICOLON)
			getASymbol();
		else
			error(1);//应该是';'
	    STATEMENT(tempList);  
	}
	if(CurSymbol->type==ENDSYM)
		getASymbol();
	else
		error(12);  //应该是'end'
	delete tempList;delete tempList1;
}

void STANDPROC(SYMLIST * list,int i)  //两个基本过程read和write的调用，他们在符号表里面分别在第六项和第七项
{
	int helper;
	TYPEITEM typeItem;

	if(i==6)//如果是第六项，read();
	{
		getASymbol();
		if(CurSymbol->type==LPAREN)
		{
			do
			{
				getASymbol();
				if(CurSymbol->type==IDENT)
				{
					helper=GETPOSITION(CurSymbol->value.lpValue);
					getASymbol();
					if (helper == 0)
					{
						error(33);//标识符没有定义
					}
					else
					{
						if(NAMETAB[helper].kind!=VARIABLE)
						{
							error(16);//应该是变量
							helper=0;
						}
						else
						{
							typeItem.typ=NAMETAB[helper].type;
							typeItem.ref=NAMETAB[helper].ref;

							if(NAMETAB[helper].normal)
								GEN(LODA,NAMETAB[helper].level,NAMETAB[helper].unite.address);
							else
								GEN(LOD,NAMETAB[helper].level,NAMETAB[helper].unite.address);
							if(CurSymbol->type==LBRACK)
							{
								//////////////////////////////////////////////////////
								SYMLIST * tempList1=new SYMLIST;
								COPYLIST(tempList1,listAddSym(list,COMMA));
								ARRAYELEMENT(tempList1,typeItem);
								delete tempList1;
								//////////////////////////////////////////////////////
							}
							if(typeItem.typ==INTS)
								GEN(RED,0,0);
							else if(typeItem.typ==CHARS)
								GEN(RED,0,1);
							else
								error(35);//操作数类型错误
						}
					}
				}
				else
					error(14);//应该是标识符
			}while(CurSymbol->type==COMMA);

			if(CurSymbol->type!=RPAREN)
			{	
				error(2);//应该是')'
			}
			else 
				getASymbol();
		}
		else
			error(3);//应该是'('
	}
	else if(i==7)//如果是第七项，write();
	{
		// printf("TEST:IN WRITE\n");
		getASymbol();
		if(CurSymbol->type==LPAREN)
		{
			do
			{
				getASymbol();
				///////////////////////////////////////////////////////
				SYMLIST * tempList=new SYMLIST;
				COPYLIST(tempList,listAddSym(listAddSym(list,RPAREN),COMMA));
				EXPRESSION(tempList,typeItem);
				delete tempList;
				////////////////////////////////////////////////////////
				
				if(typeItem.typ==INTS)
					GEN(WRT,0,0);
				else if(typeItem.typ==CHARS)
					GEN(WRT,0,1);
				else
					error(35);//操作数类型出错
			}
			while(CurSymbol->type==COMMA);
			
			if(CurSymbol->type!=RPAREN)
				error(2);//应该是')'
			else
				getASymbol();
		}
		else
			error(3);//应该是'('
	}
}

void CALL(SYMLIST * list)  //调用语句的分析
{
	int i,lastPar,cp,k;
	TYPEITEM typeItem;

	/////////////////////////////////////////////////////////////
	SYMLIST * tempList=new SYMLIST;
	COPYLIST(tempList,listAddSym(listAddSym(list,RPAREN),COMMA));
    /////////////////////////////////////////////////////////////

	// printf("TEST:call sth.\n");
	getASymbol();
	if(CurSymbol->type==IDENT)
	{
		i=GETPOSITION(CurSymbol->value.lpValue);
		if(NAMETAB[i].kind==PROCEDURE)
		{
			if(NAMETAB[i].level==0)  //如果在“过程零”，那么必定是read 或者 write
				STANDPROC(list,i);
			else
			{
				getASymbol();
				GEN(OPAC,0,0);//打开活动记录
				lastPar=BTAB[NAMETAB[i].ref].lastPar;
				cp=i;
				if(CurSymbol->type==LPAREN)
				{//实在参数列表
					/////////////////////////////////////////////////////////
					SYMLIST * tempList1=new SYMLIST;
					COPYLIST(tempList1,listAddSym(listAddSym(listAddSym(list,RPAREN),COLON),COMMA));					
					/////////////////////////////////////////////////////////
					do
					{
						getASymbol();
						if(cp>=lastPar)
							error(28);//实参与形参个数不等
						else
						{
							cp++;
							if(NAMETAB[cp].normal)
							{//值参数

								EXPRESSION(tempList1,typeItem);
								if(typeItem.typ==NAMETAB[cp].type)
								{
									if(typeItem.ref!=NAMETAB[cp].ref)
										error(29);//实参与形参类型不一致
									else if(typeItem.typ==ARRAYS)
										GEN(LODB,0,ATAB[typeItem.ref].size);
								}
								else
									error(29);//实参与形参类型不一致
							}
							else
							{//形式参数
								if(CurSymbol->type!=IDENT)
									error(14);//应该是标识符
								else
								{
									k=GETPOSITION(CurSymbol->value.lpValue);
									getASymbol();
									if(k!=0)
									{
										if(NAMETAB[k].kind!=VARIABLE)
											error(16);//应该是变量
										typeItem.typ=NAMETAB[k].type;
										typeItem.ref=NAMETAB[k].ref;
										if(NAMETAB[k].normal)
											GEN(LODA,NAMETAB[k].level,NAMETAB[k].unite.address);
										else
											GEN(LOD,NAMETAB[k].level,NAMETAB[k].unite.address);
										if(CurSymbol->type==LBRACK)
										{   
											ARRAYELEMENT(tempList,typeItem);
										}
										if(NAMETAB[cp].type!=typeItem.typ || NAMETAB[cp].ref!=typeItem.ref)
											error(29);//实参与形参类型不一致
									}
								}
							}
						}
					}
					while(CurSymbol->type==COMMA);
					delete tempList1;
					if(CurSymbol->type==RPAREN)
						getASymbol();
					else
						error(2);//应该是')'
				}
				if(cp<lastPar)
					error(30);//实在参数个数不够
				GEN(CAL1,NAMETAB[i].level,NAMETAB[i].unite.address);
				if(NAMETAB[i].level<displayLevel)
					GEN(UDIS,NAMETAB[i].level,displayLevel);
			}
		}
		else
			error(18);//应该是过程名
	}
	else
		error(14);//应该是标识符
	delete tempList;
}

void STATEMENT(SYMLIST * list)  //普通语句的分析，这些语句有几种形式，分别以不同的标识符开头
{
	// printf("TEST:in STATEMENT\n");
	////////////////////////////////////////////////////
	SYMLIST * tempList=new SYMLIST;
	COPYLIST(tempList,listAddSym(&STATBEGSYS,IDENT));
	////////////////////////////////////////////////////

	if(SYMINLIST(CurSymbol->type,tempList))  //通过不同的标识符辨别是那种语句
	{
		switch(CurSymbol->type)
		{
		case IDENT: ASSIGNMENT(list);break;
		case CALLSYM: CALL(list);break;
		case IFSYM: IFSTATEMENT(list);break;
		case WHILESYM: WHILESTATEMENT(list);break;
		case BEGINSYM: COMPOUND(list);break;
		case REPEATSYM:REPEATSTATEMENT(list); break;
		case FORSYM:FORSTATEMENT(list); break;
		case CASESYM:CASESTATEMENT(list); break;
		}
	}
	delete tempList;
	// printf("TEST:out STATEMENT\n");
}


void BLOCK(SYMLIST * list,int level)  //过程体的分析
{
	// printf("TEST:in BLOCK\n");
	int cx,tx,programBlock;
    int dx;
	dx=DX;//纪录静态上层局部数据区大小
	DX=3;//将本程序体的活动记录留出三个单元出来，用做连接数据
	tx=TX;
	NAMETAB[tx].unite.address=CX;

	if(displayLevel>MAXLEVELDEPTH)
		error(26);//程序体表溢出
	ENTERBLOCK();  //登记过程体
	programBlock=BX;
	DISPLAY[displayLevel]=BX;
	NAMETAB[tx].type=NOTYP;
	NAMETAB[tx].ref=programBlock;

	if(CurSymbol->type==LPAREN && displayLevel>1)
	{
		PARAMENTERLIST(list);  //编译过程列表
		if(CurSymbol->type==SEMICOLON)
			getASymbol();
		else
			error(1);//应该是';'
	}
	else if(displayLevel>1)
	{
		if(CurSymbol->type==SEMICOLON)
			getASymbol();
		else
			error(1);//应该是';'
	}
	BTAB[programBlock].lastPar=TX;
	BTAB[programBlock].pSize=DX;
	GEN(JMP,0,0);
	do
	{
		switch(CurSymbol->type)  //针对当前不同的不好进行不同的声明
		{			
		case CONSTSYM:
			getASymbol();
			do
			{
				CONSTDECLARATION(list);  //常量声明，一次可声明多个
			}
			while(CurSymbol->type==IDENT);
			break;
		case TYPESYM:
			getASymbol();
			do
			{
				TYPEDECLARATION(list);  //类型声明，一次可声明多个
			}
			while(CurSymbol->type==IDENT);
			break;
		case VARSYM:
			getASymbol();
			do
			{
				VARDECLARATION(list);  //变量声明，一次可声明多个
			}
			while(CurSymbol->type==IDENT);				
			break;
		}
		while(CurSymbol->type==PROCSYM)
			PROCDECLARATION(list);    //过程声明，每次只能声明一个
	}while(SYMINLIST(CurSymbol->type,&DECLBEGSYS));
	CODE[NAMETAB[tx].unite.address].address=CX;//将执行语句的开始处地址回填
	JUMADRTAB[JX]=CX;
	JX++;
	NAMETAB[tx].unite.address=CX;//代码开始地址
	cx=CX;
	GEN(ENTP,displayLevel,DX);

	////////////////////////////////////////////////////
	SYMLIST * tempList=new SYMLIST;
	COPYLIST(tempList,listAddSym(listAddSym(list,ENDSYM),SEMICOLON));

	// printf("TEST:before STATEMENT\n");
	STATEMENT(tempList);
	// printf("TEST:after STATEMENT\n");
	delete tempList;
	////////////////////////////////////////////////////
    CODE[cx].address=DX;//回填数据区大小
	if(displayLevel>1)
		GEN(RETP,0,0);//从程序体返回
	else
		GEN(ENDP,0,0);//程序结束
	QUITBLOCK();
	DX=dx;//恢复静态上层局部数据区大小
	// printf("TEST:out BLOCK\n");
}

int Feof(FILE *fp)//判断是否到了源文件尾
{
	int getChar;
	getChar=fgetc(fp);
	if(getChar==-1)
	{
		if(feof(fp))
			return 1;//如果是，返回“真”
	}
	else
		fseek(fp,-1,SEEK_CUR);//否则，将指向文件流的指针向后移动一个字符
	return 0;
}


SYMBOL GetReserveWord(char *nameValue)//判断得到的符号是否是保留字
{
	int i;

	char reserveWord[NUMOFWORD][20]=
	{
		"and","begin","const","else","if","not","or","program","type","while","for",
		"array","call","do","end","mod","of","procedure","then","var","to","repeat",
		"until","case"
	};
	SYMBOL reserveType[NUMOFWORD]=
	{
		ANDSYM,BEGINSYM,CONSTSYM,ELSESYM,IFSYM,NOTSYM,ORSYM,PROGRAMSYM,TYPESYM,WHILESYM,FORSYM,
		ARRAYSYM,CALLSYM,DOSYM,ENDSYM,MODSYM,OFSYM,PROCSYM,THENSYM,VARSYM,TOSYM,REPEATSYM,UNTILSYM,
		CASESYM
	};

	for(i=0;i<NUMOFWORD;i++)  //这里采用了遍历的做法，但是效率不高，可以考虑采用二叉查找法
		if(!stricmp(reserveWord[i],nameValue))
			return reserveType[i];
	return (SYMBOL)0;
}

void AddSymbolNode(SymbolItem **current,int lineNumber,SYMBOL type,int iValue)  //在词法分析的时候向符号列表中加入一个分析出来的符号
{
		(*current)->next=new SymbolItem;
		if(!(*current)->next)
		{
			error(27);//系统为本编译程序分配的堆不够用
			exit(4);
		}
		(*current)=(*current)->next;
		(*current)->lineNumber=lineNumber;
		(*current)->type=type;
		(*current)->value.iValue=iValue; 
		(*current)->next=NULL; 
}

void getSymbols(FILE *srcFile)  //从源文件读入字符，获得符号链表
{
	int lineNumber=1;
	char nameValue[MAXSYMNAMESIZE];
	int nameValueint;
	char readChar;
	SymbolItem head,*current=&head;

	printf("\n进行词法分析  -->-->-->-->-->-->-->-->  ");

	while(!Feof(srcFile))
	{
		readChar=fgetc(srcFile);

		if(iscsymf(readChar))
		{
			nameValueint=0;
			do
			{
				nameValue[nameValueint++]=readChar;
				readChar=fgetc(srcFile);
				if(Feof(srcFile)) 
					break;
			}while(iscsym(readChar) || isdigit(readChar)); //判断是不是字母数字或下划线
			nameValue[nameValueint]=0;
			fseek(srcFile,-1,SEEK_CUR);
			current->next=new SymbolItem;
			current=current->next;
			current->lineNumber=lineNumber;
			if(!(current->type=GetReserveWord(nameValue)))
			{
				current->type=IDENT;
				current->value.lpValue=new char[nameValueint]; 
				strcpy(current->value.lpValue,nameValue);
			}
			current->next=NULL; 
		}
		else if(isdigit(readChar))
		{
			nameValueint=0;
			do
			{
				nameValue[nameValueint++]=readChar;
				readChar=fgetc(srcFile);
				if(Feof(srcFile)) 
					break;
			}while(isdigit(readChar)); 
			nameValue[nameValueint]=0;
			fseek(srcFile,-1,SEEK_CUR);
			AddSymbolNode(&current,lineNumber,INTCON,atoi(nameValue));
		}
		else switch(readChar)
		{
			case '	':				//字符 'tab'
			case ' ':	
				break;
			case '\n':
				break;
			case '\r':
				lineNumber++;
				break;
			case ':':
				if(Feof(srcFile))
					break;
				readChar=fgetc(srcFile);
				if(readChar=='=')
					AddSymbolNode(&current,lineNumber,BECOMES,0);
				else
				{
					fseek(srcFile,-1,SEEK_CUR);
					AddSymbolNode(&current,lineNumber,COLON,0);
				}
				break;
			case '<':
				if(Feof(srcFile))
					break;
				readChar=fgetc(srcFile);
				if(readChar=='=')
					AddSymbolNode(&current,lineNumber,LEQ,0);
				else if(readChar=='>')
					AddSymbolNode(&current,lineNumber,NEQ,0);
				else
				{
					fseek(srcFile,-1,SEEK_CUR);
					AddSymbolNode(&current,lineNumber,LSS,0);
				}
				break;
			case '>':
				if(Feof(srcFile))
					break;
				readChar=fgetc(srcFile);
				if(readChar=='=')
					AddSymbolNode(&current,lineNumber,GEQ,0);
				else
				{
					fseek(srcFile,-1,SEEK_CUR);
					AddSymbolNode(&current,lineNumber,GTR,0);
				}
				break;
			case '.':
				if(Feof(srcFile))
				{
					AddSymbolNode(&current,lineNumber,PERIOD,0);
					break;
				}
				readChar=fgetc(srcFile);
				if(readChar=='.')
					AddSymbolNode(&current,lineNumber,DPOINT,0);
				else
				{
					fseek(srcFile,-1,SEEK_CUR);
					AddSymbolNode(&current,lineNumber,PERIOD,0);
				}
				break;
			case '\'':
				readChar=fgetc(srcFile);
				if(Feof(srcFile))
					break;
				if(fgetc(srcFile)=='\'')
					AddSymbolNode(&current,lineNumber,CHARCON,readChar);
				else
					error(1);//////////////////////////
				break;
			case '+':
//新添加
				if (Feof(srcFile))
					break;
				readChar = fgetc(srcFile);
				if (readChar == '=')
					AddSymbolNode(&current, lineNumber, PLUSBECOMES, 0);
				else 
				{
					fseek(srcFile, -1, SEEK_CUR);
				AddSymbolNode(&current,lineNumber,PLUS,0);
				}
//新添加					
				break;
			case '-':
				if (Feof(srcFile))
					break;
				readChar = fgetc(srcFile);
				if (readChar == '=')
					AddSymbolNode(&current, lineNumber, MINUSBECOMES, 0);
				else
				{
					fseek(srcFile, -1, SEEK_CUR);
				AddSymbolNode(&current,lineNumber,MINUS,0);
				}
				break;
			case '*':
				AddSymbolNode(&current,lineNumber,TIMES,0);
				break;
			case '/':
				AddSymbolNode(&current,lineNumber,DIVSYM,0);
				break;
			case '(':
				AddSymbolNode(&current,lineNumber,LPAREN,0);
				break;
			case ')':
				AddSymbolNode(&current,lineNumber,RPAREN,0);
				break;
			case '=':
				AddSymbolNode(&current,lineNumber,EQL,0);
				break;
			case '[':
				AddSymbolNode(&current,lineNumber,LBRACK,0);
				break;
			case ']':
				AddSymbolNode(&current,lineNumber,RBRACK,0);
				break;
			case ';':
				AddSymbolNode(&current,lineNumber,SEMICOLON,0);
				break;
			case ',':
				AddSymbolNode(&current,lineNumber,COMMA,0);
				break;
			default:
				error(38);
		}   //switch end
	}		//while end
	Symbols=head.next; 
	CurSymbol=Symbols;
	if(nError)
	{
		printf("\n%d errors found.",nError);
		exit(2);
	}
	else
		printf("词法分析成功！\n\n");
}

void getASymbol()  //采用递归下降的语法分析，逐个的获取一个单词符号
{
	if(CurSymbol->next)
		CurSymbol=CurSymbol->next;
	else
	{
		error(43);  //语法分析没有完毕，需要标识符
		exit(3);
	}

}

void destroySymbols()  //编译完毕，将符号链表释放
{
	SymbolItem *current,*needDel;
	current=Symbols;
	while(current)
	{
		needDel=current;
		current=current->next;
		delete needDel;
	}
}


/////////////下面这三个函数是为了模拟pascal源程序中的set类型而开发的////////////

SYMLIST * listsAdd(SYMLIST * list1,SYMLIST * list2)  //两个“集合”相加，返回一个“集合”
{
	SYMLIST * temp=new SYMLIST;
	COPYLIST(temp,list1);
	temp->AddTail(list2);
	return temp;
}
		
SYMLIST * listAddSym(SYMLIST * list,SYMBOL sym)  //一个“集合”加上一个“元素”，返回一个“集合”
{
	SYMLIST * temp=new SYMLIST;
	COPYLIST(temp,list);
	temp->AddTail(sym);
	return temp;
}

int SYMINLIST(SYMBOL sym,SYMLIST * list)  //判断一个“元素”是否在“集合”里面
{
	for(POSITION pos=list->GetHeadPosition();pos;)
	{
		SYMBOL temp;
		temp=list->GetNext(pos);
		if(temp==sym)
			return 1;  //如果在，返回非零
	}
	return 0;  //不在，则返回零
}

void COPYLIST(SYMLIST * list1,SYMLIST * list2)  //“集合”之间的拷贝
{
	for(POSITION pos=list2->GetHeadPosition();pos;)
	{
		SYMBOL temp;
		temp=list2->GetNext(pos);
		list1->AddTail(temp);
	}
}


/////////////////////  主程序  ///////////////////////
int main(int argc, char* argv[])  
{
	char srcFilename[FILENAMESIZE] = "test.pl";
	FILE *srcFile;
	char *srcFileNamePoint;
	
	// if(argc>1)
	// 	strcpy(srcFilename,argv[1]);
	// else
	// {
	// 	printf("Please input the source file name : ");
	// 	scanf("%s",srcFilename);
	// }
	if(!(srcFile=fopen(srcFilename,"rb")))
	{
		printf("Error : source file %s not found\n",srcFilename);
		exit(1);
	}

	printf("\n第一遍：词法分析");
	getSymbols(srcFile);//第一遍，取得所有的符号，第二遍才开始语法分析和代码生成
	printf("第二遍：语法分析和代码生成\n");

	INITIAL();  //初始化
	ENTERPREID();  //预填符号表

	printf("\n**************   下面是部分的生成代码   ***************\n\n");
	if (CurSymbol->type!=PROGRAMSYM)
		error(13);  //应该是'program'
	getASymbol();
	if(CurSymbol->type!=IDENT)
		error(14);  //应该是标识符
	getASymbol();
	if(CurSymbol->type!=SEMICOLON)
		error(1);  //应该是';'
	else 
		getASymbol();


	// printf("TEST:before BLOCK\n");
	//////////////////////////////////////////////////////////
	SYMLIST * tempList3=new SYMLIST;
	COPYLIST(tempList3,listsAdd(listAddSym(&DECLBEGSYS,PERIOD),&STATBEGSYS));
	BLOCK(tempList3,0);
	delete tempList3;
	//////////////////////////////////////////////////////////
	// printf("TEST:after BLOCK\n");

	if(CurSymbol->type!=PERIOD)
		error(8);  //应该是'.'
	if(nError==0)
	{
		for(srcFileNamePoint=&srcFilename[strlen(srcFilename)];*srcFileNamePoint!='.' && srcFileNamePoint!=srcFilename;srcFileNamePoint--)
		;
	    *srcFileNamePoint=0;  //删除后面的扩展名
		WriteCodeList(strcat(srcFilename,".lst"));
		for(srcFileNamePoint=&srcFilename[strlen(srcFilename)];*srcFileNamePoint!='.' && srcFileNamePoint!=srcFilename;srcFileNamePoint--)
			;
		*srcFileNamePoint=0;  //删除后面的扩展名
		WriteObjCode(strcat(srcFilename,".pld"));
		for(srcFileNamePoint=&srcFilename[strlen(srcFilename)];*srcFileNamePoint!='.' && srcFileNamePoint!=srcFilename;srcFileNamePoint--)
			;
		*srcFileNamePoint=0;  //删除后面的扩展名
		WriteLabelCode(strcat(srcFilename,".lab"));
		destroySymbols();
		fclose(srcFile);
		
		//printf("\n编译成功！请输入任何字符退出。");
		printf("\n编译成功！");
		
		//int a;
		//scanf("%d",a);       //一个简单的关卡，程序执行完毕后可以停下，您可以观看生成的代码，注意，代码是部分的，有些操作数没有填上去
		return 0;
	}
	destroySymbols();
	
	//int b;
	//scanf("%d",b);     //一个简单的关卡，程序执行完毕后可以停下，您可以观看生成的代码，注意，代码是部分的，有些操作数没有填上去
	return 0;
}

