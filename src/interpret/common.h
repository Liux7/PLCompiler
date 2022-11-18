#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>

const STACKSIZE=2047,CXMAX=10000,LEVMAX=100;

typedef enum _OPCOD  //操作码的定义
{
	LIT,LIT1,LOD,ILOD,LODA,LODT,LODB,STO,CPYB,JMP,JPC,RED,WRT,CAL,RETP,UDIS,OPAC,ENTP,
	ENDP,ANDS,ORS,NOTS,IMOD,MUS,ADD,ADD1,SUB,MULT,IDIV,EQ,NE,LS,LE,GT,GE
} OPCOD;

typedef struct _INSTRUCTION  //指令结构定义
{
	OPCOD func;
	int level;
	int address;
} INSTRUCTION;

INSTRUCTION * CODE=NULL;  //一个存放INSTRUCTION纪录的数组


int pc,bp,top;
int oldTop;
INSTRUCTION instruction;  //当前被翻译的指令
int S[STACKSIZE];         //代码运行需要的栈
int DISPLAY[LEVMAX];      //DISPLAY表
int stop;                 //判断是否翻译结束的标志
int h,hh,hhh;             //三个临时变量
char ch;                  //临时变量
int temp=0;               //临时变量
