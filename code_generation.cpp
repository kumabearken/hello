//// ============================================================================
//   Author:  Ken Kumagai
//   Date:  10/01/2018
//   File: Lexer.cpp
//   Description: The following demonstrates the implementation of
//     Lexer using a table driven Finite State Machine.
//	Parser for rat18s language
// 	code generation for Assembly language
// ============================================================================

#include <iostream>
#include <cstdlib>
#include <cctype>
#include <string>
#include <vector>
#include <fstream>
#include <stack>
using namespace std;

//global array size
const int ARRSEP = 9;
const int ARRKEYW = 11;

// memory address
int memAddress = 2000;

// global token state array
// idenState[0] = first identifier tester
// idenState[1] = test for 2nd identifier
// idenState[2] = for all identifiers
bool idenState[3] = { true,false,false };

//comment[0] switch for comment;
bool comment[1] = { false };

//opState[0] tests if first operator was an assignment
bool opState[1] = { false };

// if triggered statement breaks syntax
bool invalid[1] = { false };
bool sepstate[2];

//keywords
bool fun[99] = { false,false,false,false,false,false,false };

//body
bool body[1] = { false };

//expression
bool expression[1] = { false };

//integer considered identifier
bool intIsID[1] = { false };

//checks main function
bool mainTest[1] = { false };

//checks if loop statement
bool loops[3] = { false,false,false };

//checks for completion
bool complete[1] = { true };

// variable declaration
// varDec[0] is integer
// varDec[1] is bool
bool varDec[2] = { false,false };

// instruction
int ins[1];

// location to jump to
int jumpLoc[1];
int jumpzLoc[1];

bool putCheat[1] = { false };

// relative operators
// 1 is <
// 2 is >
// 3 is ==
// 4 is !=
// 5 is <=
// 6 is >=
int relop[1];

// arithmetic operators
// 1 is +
//2 is -
// 3 is *
// 4 is /
int arOp[1];

enum FSM_TRANSITIONS
{
	REJECT = 0,
	INTEGER,
	REAL,
	OPERATOR,
	IDENTIFIER,
	UNKNOWN,
	SPACE,
	KEYWORD,
	BOOL,
	SEPARATOR
};

//KEYWORDS
string keywords[ARRKEYW] = { "integer","if","else","endif","while","return","get","put"
,"boolean","function","real" };
string bools[2] = { "true","false" };
char separator[ARRSEP] = { '(',')','{','}','[',']',';', ':',',' };

/*                           integer,    real,    operator,   identifier, unknown, space,  keyword, bool,   separator*/
int stateTable[][10] = { { 0,  INTEGER,    REAL,    OPERATOR,   IDENTIFIER, UNKNOWN, SPACE,  KEYWORD, BOOL,   SEPARATOR },
/* STATE 1 */{ INTEGER,    INTEGER,    REAL,    REJECT,     REJECT,     REJECT,  REJECT, REJECT,  REJECT, REJECT },
/* STATE 2 */{ REAL,       REAL,       UNKNOWN, REJECT,     REJECT,     REJECT,  REJECT, REJECT,  REJECT, REJECT },
/* STATE 3 */{ OPERATOR,   REJECT,     REJECT,  REJECT,     IDENTIFIER, REJECT,  REJECT, REJECT,  REJECT, REJECT },
/* STATE 4 */{ IDENTIFIER, IDENTIFIER, REJECT,  IDENTIFIER, IDENTIFIER, REJECT,  REJECT, REJECT,  REJECT, REJECT },
/* STATE 5 */{ UNKNOWN,    UNKNOWN,    UNKNOWN, UNKNOWN,    UNKNOWN,    UNKNOWN, REJECT, REJECT,  REJECT, REJECT },
/* STATE 6 */{ SPACE,      REJECT,     REJECT,  REJECT,     REJECT,     REJECT,  REJECT, REJECT,  REJECT, REJECT },
/* STATE 7 */{ KEYWORD,    KEYWORD,    REJECT,  KEYWORD,    KEYWORD,    REJECT,  REJECT, KEYWORD, REJECT, REJECT },
/* STATE 8 */{ BOOL,       REJECT,     REJECT,  REJECT,     REJECT,     REJECT,  REJECT, REJECT,  BOOL,   REJECT },
/* STATE 9 */{ SEPARATOR,  REJECT,     REJECT,  REJECT,     REJECT,     REJECT,  REJECT, REJECT,  REJECT, REJECT }
};

//Structures&Classes

struct TokenType
{
	string token;
	int lexeme;
	string lexemeName;
};

struct SymbolTable
{
	string name;
	int memAdd;
	int value;
	string type;
};

struct AssemblyCodeInstructions
{
	int order = 0;
	string instruction = "";
	string addOrVal = "";
};

//vectors
vector <SymbolTable> symTab;

//global stack frame;
stack<char> sepMatch;
stack<int> assemblyCode;
stack<char> prevOp;
stack<TokenType> variables;

//arrays and vars for assembly code instructions
AssemblyCodeInstructions ACI[1000];
int counter = 0;

// Function prototypes
vector<TokenType> Lexer(string expression);
int Get_FSM_Col(char currentChar);
string GetLexemeName(int lexeme);
void parser(vector<TokenType> toks, int pos, ofstream& file);
void SymbolHandler(string symName);
void AssemblyCodeGeneration(string IV, string varName);

int main()
{
	ifstream infile;
	ofstream outfile("new.txt");
	string fileName = "";
	string expression = "";
	vector<TokenType> tokens;
	int line = 1;
	cout << "Please enter the name of the file: ";

	getline(cin, fileName);
	infile.open(fileName.c_str());

	if (infile.fail())
	{
		cout << "n** ERROR - the file ""<<fileName<<"" cannot be found!";
		system("pause");
		exit(1);
	}

	outfile << "Token					Lexeme" << endl;
	outfile << "_______________________________________________" << endl;
	while (getline(infile, expression))
	{

		tokens = Lexer(expression);

		for (unsigned i = 0; i < tokens.size(); ++i)
		{
			outfile << tokens[i].lexemeName << "					"
				<< tokens[i].token << endl;
			parser(tokens, i, outfile);
			if (invalid[0])
			{
				outfile << "ERROR at LINE " << line << ".\n";
			}
			outfile << '\n';
		}
		line++;
	}
	cout << "Assembly Code Listing\n";
	for (int i = 0; i < counter; ++i)
	{
		cout << ACI[i].order << '\t' << ACI[i].instruction << '\t' << ACI[i].addOrVal << endl;
	}
	cout << "Identifier	MemoryLocation	Type\n";
	for (int i = 0; i < symTab.size(); ++i)
	{
		cout << symTab[i].name << "\t\t" << symTab[i].memAdd << "\t\t" << symTab[i].type << endl;
	}
	infile.close();
	outfile.close();
	cin.get();
	return 0;
}

/**
* FUNCTION: Lexer
* USE: Parses the "expression" string using the Finite State Machine to
*     isolate each individual token and lexeme name in the expression.
* param expression - A std::string containing text.
* return - Returns a vector containing the tokens found in the string
*/
vector<TokenType> Lexer(string expression)
{
	TokenType access;
	vector<TokenType> tokens;
	char currentChar = ' ';
	int col = REJECT;
	int currentState = REJECT;
	int prevState = REJECT;
	string currentToken = "";

	for (unsigned x = 0; x < expression.length();)
	{
		currentChar = expression[x];

		col = Get_FSM_Col(currentChar);

		currentState = stateTable[currentState][col];

		if (currentState == REJECT)
		{
			if (prevState != SPACE)
			{
				for (int i = 0; i < ARRKEYW; ++i)
				{
					if (currentToken == keywords[i])
					{
						prevState = KEYWORD;
					}
				}
				for (int i = 0; i < 2; ++i)
				{
					if (currentToken == bools[i])
					{
						prevState = BOOL;
					}
				}
				access.token = currentToken;
				access.lexeme = prevState;
				access.lexemeName = GetLexemeName(access.lexeme);
				tokens.push_back(access);
			}
			currentToken = "";
		}
		else
		{
			currentToken += currentChar;
			++x;
		}
		prevState = currentState;

	}
	if (currentState != SPACE && currentToken != "")
	{
		access.token = currentToken;
		access.lexeme = currentState;
		access.lexemeName = GetLexemeName(access.lexeme);
		tokens.push_back(access);
	}
	return tokens;
}

/**
* FUNCTION: Get_FSM_Col
* USE: Determines the state of the type of character being examined.
* param currentChar - A character.
* return - Returns the state of the type of character being examined.
*/
int Get_FSM_Col(char currentChar)
{
	// check for whitespace
	if (isspace(currentChar))
	{
		return SPACE;
	}

	// check for integer numbers
	else if (isdigit(currentChar))
	{
		return INTEGER;
	}

	// check for real numbers
	else if (currentChar == '.')
	{
		return REAL;
	}

	// check for characters
	else if (isalpha(currentChar))
	{
		return IDENTIFIER;
	}

	// check for operators and separators
	else if (ispunct(currentChar))
	{
		for (int i = 0; i < ARRSEP; ++i)
		{
			if (currentChar == separator[i])
			{
				return SEPARATOR;
			}
		}
		return OPERATOR;
	}
	return UNKNOWN;
}

/*
* FUNCTION: GetLexemeName
* USE: Returns the string equivalent of an integer lexeme token type.
* param lexeme - An integer lexeme token type.
* return - An std::string string representing the name of the integer
*        lexeme token type.
*/
string GetLexemeName(int lexeme)
{
	switch (lexeme)
	{
	case INTEGER:
		return "INTEGER";
		break;
	case REAL:
		return "REAL  ";
		break;
	case OPERATOR:
		return "OPERATOR";
		break;
	case IDENTIFIER:
		return "IDENTIFIER";
		break;
	case UNKNOWN:
		return "UNKNOWN";
		break;
	case SPACE:
		return "SPACE";
		break;
	case KEYWORD:
		return "KEYWORD";
		break;
	case BOOL:
		return "BOOL";
		break;
	case SEPARATOR:
		return "SEPARATOR";
		break;
	default:
		return "ERROR";
		break;
	}
}
/*
*	Function: Parser
*	Use - takes a token and determines whether it is being used in a proper format
*	Param - a vector of struct TokenType, integer of location of token in vector, and a output file to write to
*	Return - void
*/
void parser(vector<TokenType> toks, int pos, ofstream& file)
{
	char tokChar;
	string lexName;
	string tokName;
	TokenType someToken;
	someToken = toks.at(pos);
	lexName = someToken.lexemeName;
	tokName = someToken.token;
	tokChar = tokName[0];
	if (comment[0] == true)
	{
		if (tokChar == '!')
		{
			comment[0] = false;
			sepMatch.pop();
			complete[0] = true;
			file << "end of comment\n";
		}
	}
	else if (invalid[0] == true)
	{
		if (tokChar == ';')
			invalid[0] = false;
	}
	else
	{
		if (lexName == "INTEGER")
		{
			variables.push(someToken);
		}
		if ((lexName == "KEYWORD") && (complete[0]))
		{
			if (tokName == "function")
			{
				file << "<Function> -> <Identifier> [ <Opt Parameter List>] <Opt Declaration List> <Body>\n";
				fun[0] = true;
				complete[0] = false;
			}
			if (tokName == "integer")
			{
				if (fun[4])
				{
					file << "<Parameter> -> <Parameter> || ]\n";
					fun[4] = false;
					fun[5] = true;
				}
				if (complete[0])
				{
					idenState[0] = false;
					varDec[0] = true;
				}

			}
			if (tokName == "boolean")
			{
				if (fun[3])
				{
					file << "<Parameter> -> <Parameter> || ]\n";
					fun[4] = true;
					fun[3] = false;
				}
				if (complete[0])
				{
					idenState[0] = false;
					varDec[1] = true;
				}
			}
			if (tokName == "real")
			{
				if (fun[3])
				{
					file << "<Parameter> -> <Parameter> || ]\n";
					fun[4] = true;
					fun[3] = false;
				}
			}
			if (tokName == "return")
			{
				opState[0] = true;
				idenState[1] = true;
				file << "return -> <Expression>\n";
				complete[0] = false;
			}
			if (tokName == "while")
			{
				loops[0] = true;
				complete[0] = false;
				ins[0] = 99;
				idenState[0] = false;
				idenState[2] = true;
				AssemblyCodeGeneration("", "");
			}
			if (tokName == "get")
			{
				idenState[1] = true;
				ins[0] = 4;
			}
			if (tokName == "put")
			{
				idenState[1] = true;
				opState[0] = true;
				putCheat[0] = true;
			}
		}

		if ((lexName == "IDENTIFIER") || (intIsID[0] == true))
		{
			//function is triggered
			if (fun[0])
			{
				file << "<Identifier> -> [<Opt Paramter List>] <Opt Declartion List>\n";
				fun[1] = true;
				fun[0] = false;
			}
			else if (fun[2])
			{
				file << "<Identifier> -> :<Qualifier>]";
				fun[2] = false;
				fun[3] = true;
			}
			else
			{
				if (idenState[0] == true)
				{
					file << "<Statement> -> <Assign>\n";
					file << "<Assign> -> <Identifier> = <Expression>;\n";
					complete[0] = false;
					idenState[0] = false;
					idenState[1] = true;
					SymbolHandler(tokName);
					variables.push(someToken);
				}
				else if (idenState[1] == true)
				{
					file << "<Expression> -> <Term> <Expression Prime>\n";
					idenState[1] = false;
					idenState[2] = true;
					complete[0] = false;
					SymbolHandler(tokName);
					variables.push(someToken);
				}
				else if ((idenState[2] == true) || ((varDec[0]) || (varDec[1])))
				{
					file << "<Term> -> <Factor> <Term Prime>\n";
					file << "<Factor> -> <Identifier>\n";
					if ((varDec[0]) || (varDec[1]) || (loops[1]) || (loops[2]) || (putCheat[0]))
					{
						SymbolHandler(tokName);
						variables.push(someToken);
					}
				}
			}
		}
		if (lexName == "OPERATOR")
		{
			if (tokChar == '=')
			{
				char tempChar = 'a';
				if (!prevOp.empty())
				{
					char tempChar = prevOp.top();
				}
				if (tempChar == '=')
				{
					relop[0] = 3;
				}
				if (tempChar == '!')
				{
					relop[0] = 4;
				}
				if (tempChar == '<')
				{
					relop[0] = 5;
				}
				if (tempChar == '>')
				{
					relop[0] = 6;
				}
				opState[0] = true;
			}
			else if ((opState[0] == true) && (tokChar == '+'))
			{
				file << "<Term Prime> -> E\n";
				file << "<Expression Prime> -> + <Term><Expression Prime>\n";
				arOp[0] = 1;
			}
			else if ((opState[0] == true) && (tokChar == '-'))
			{
				file << "<Term Prime> -> E\n";
				file << "<Expression Prime> -> - <Term><Expression Prime>\n";
				arOp[0] = 2;
			}
			else if ((opState[0] == true) && (tokChar == '*'))
			{
				file << "<Factor Prime> -> E\n";
				file << "< Term Prime> -> * <Factor><Term Prime>\n";
				arOp[0] = 3;
			}
			else if ((opState[0] == true) && (tokChar == '/'))
			{
				file << "<Factor Prime> -> E\n";
				file << "< Term Prime> -> / <Factor><Term Prime>\n";
				arOp[0] = 4;
			}
			else if ((opState[0] == true) && (tokChar == '%'))
			{
				file << "<Factor Prime> -> E\n";
				file << "< Term Prime> -> % <Factor><Term Prime>\n";
			}
			else if (tokChar == '<')
			{
				relop[0] = 1;
				file << "<Factor Prime> -> E\n";
				file << "< Term Prime> -> < <Factor><Term Prime>\n";
			}
			else if (tokChar == '>')
			{
				relop[0] = 2;
				file << "<Factor Prime> -> E\n";
				file << "< Term Prime> -> > <Factor><Term Prime>\n";
			}
			else if (tokChar == '!')
			{
				comment[0] = true;
				sepMatch.push('!');
				complete[0] = false;
				file << "comment";
			}
			else if (tokChar == '%')
			{
				if (mainTest[0])
				{
					file << "<main> -> <opt Declaration List> <Statement List>\n";
				}
				else
				{
					mainTest[0] = true;
				}
			}
			else
			{
				file << "Invalid Input!\n";
				invalid[0] = true;
			}
			prevOp.push(tokChar);
		}
		if (lexName == "SEPARATOR")
		{
			if (tokChar == ';')
			{
				while (!prevOp.empty())
				{
					prevOp.pop();
				}
				file << "<Term Prime> -> E\n";
				file << "<Expression Prime> -> E\n";
				//assign cases
				if (opState[0])
				{
					TokenType tempToken;
					TokenType tempToken2;
					// need to  swapsince order backwards
					if (arOp[0] != 0)
					{
						tempToken = variables.top();
						variables.pop();
						tempToken2 = variables.top();
						variables.pop();
						variables.push(tempToken);
						tempToken = tempToken2;
					}
					else
					{
						tempToken = variables.top();
						variables.pop();
					}
					//arithemtic for put()
					if ((arOp[0] != 0) && (putCheat[0]))
					{
						for (int i = 0; i < 2; i++)
						{
							if (tempToken.lexemeName == "INTEGER")
							{
								ins[0] = 0;
								AssemblyCodeGeneration(tempToken.token, "");
								tempToken = variables.top();
								variables.pop();
							}
							else if (tempToken.lexemeName == "IDENTIFIER")
							{
								ins[0] = 1;
								AssemblyCodeGeneration("", tempToken.token);
								tempToken = variables.top();
								variables.pop();
							}
						}
						//+
						if (arOp[0] == 1)
						{
							ins[0] = 5;
							AssemblyCodeGeneration("", tempToken.token);
							ins[0] = 3;
							AssemblyCodeGeneration("", tempToken.token);
							arOp[0] = 0;
						}
						//-
						if (arOp[0] == 2)
						{
							ins[0] = 6;
							AssemblyCodeGeneration("", tempToken.token);
							ins[0] = 3;
							AssemblyCodeGeneration("", tempToken.token);
							arOp[0] = 0;
						}
						//*
						if (arOp[0] == 3)
						{
							ins[0] = 7;
							AssemblyCodeGeneration("", tempToken.token);
							ins[0] = 3;
							AssemblyCodeGeneration("", tempToken.token);
							arOp[0] = 0;
						}
						// /
						if (arOp[0] == 4)
						{
							ins[0] = 8;
							AssemblyCodeGeneration("", tempToken.token);
							ins[0] = 3;
							AssemblyCodeGeneration("", tempToken.token);
							arOp[0] = 0;
						}
					}
					//arithmetic assignment
					else if (arOp[0] != 0)
					{
						for (int i = 0; i < 2; i++)
						{
							if (tempToken.lexemeName == "INTEGER")
							{
								ins[0] = 0;
								AssemblyCodeGeneration(tempToken.token, "");
								tempToken = variables.top();
								variables.pop();
							}
							else if (tempToken.lexemeName == "IDENTIFIER")
							{
								ins[0] = 1;
								AssemblyCodeGeneration("", tempToken.token);
								tempToken = variables.top();
								variables.pop();
							}
						}
						//add
						if (arOp[0] == 1)
						{
							ins[0] = 5;
							AssemblyCodeGeneration("", tempToken.token);
							ins[0] = 2;
							AssemblyCodeGeneration("", tempToken.token);
							arOp[0] = 0;
						}
						//sub
						if (arOp[0] == 2)
						{
							ins[0] = 6;
							AssemblyCodeGeneration("", tempToken.token);
							ins[0] = 2;
							AssemblyCodeGeneration("", tempToken.token);
							arOp[0] = 0;
						}
						//mul
						if (arOp[0] == 3)
						{
							ins[0] = 7;
							AssemblyCodeGeneration("", tempToken.token);
							ins[0] = 2;
							AssemblyCodeGeneration("", tempToken.token);
							arOp[0] = 0;
						}
						// div
						if (arOp[0] == 4)
						{
							ins[0] = 8;
							AssemblyCodeGeneration("", tempToken.token);
							ins[0] = 2;
							AssemblyCodeGeneration("", tempToken.token);
							arOp[0] = 0;
						}
					}
					//simple assignment
					else
					{
						if (tempToken.lexemeName == "INTEGER")
						{
							ins[0] = 0;
							AssemblyCodeGeneration(tempToken.token, "");
							ins[0] = 2;
							tempToken = variables.top();
							variables.pop();
							AssemblyCodeGeneration("", tempToken.token);

						}
						else if (tempToken.lexemeName == "IDENTIFIER")
						{
							ins[0] = 1;
							AssemblyCodeGeneration("", tempToken.token);
							tempToken = variables.top();
							variables.pop();
							ins[0] = 2;
							AssemblyCodeGeneration("", tempToken.token);
						}
					}
				}
				//stdin get()
				if (ins[0] == 4)
				{
					TokenType tempToken = variables.top();
					variables.pop();
					if (tempToken.lexemeName == "IDENTIFIER")
					{
						AssemblyCodeGeneration("", tempToken.token);
						ins[0] = 2;
						AssemblyCodeGeneration("", tempToken.token);
					}
				}
				idenState[0] = true;
				idenState[1] = false;
				idenState[2] = false;
				opState[0] = false;
				invalid[0] = false;
				complete[0] = true;
				varDec[0] = false;
				varDec[1] = false;
			}
			if (tokChar == '[')
			{
				if (pos == 0)
				{
					invalid[0] = true;
				}
				else
				{
					TokenType thisToken;
					thisToken = toks.at(pos - 1);
					string thisLexeme;
					thisLexeme = thisToken.lexemeName;
					if (thisLexeme == "IDENTIFIER")
					{
						sepMatch.push('[');
						file << "<Parameter>  -> <Identifier> : <Qualifier> ]\n";
						fun[2] = true;
					}
				}
			}
			if (tokChar == ']')
			{
				TokenType thisToken;
				thisToken = toks.at(pos - 1);
				string thisLexeme;
				thisLexeme = thisToken.lexemeName;
				if ((thisLexeme == "KEYWORD") || (thisLexeme == "SEPARATOR"))
				{
					char thisChar = sepMatch.top();
					sepMatch.pop();
					if (thisChar == '[')
					{
						file << "<Parameter List> -> <Opt Declaration> <Body>\n";
						body[0] = true;
					}
				}
			}
			if (tokChar == '{')
			{
				if (loops[2])
				{
					sepMatch.push('{');
					file << "<Declaration> -> <Body> }\n";
				}
				else if (!body[0])
				{
					invalid[0] = true;
				}
				else
				{
					sepMatch.push('{');
					file << "<Declaration> -> <Body> }\n";
					complete[0] = true;
				}
			}
			if (tokChar == '}')
			{
				TokenType thisToken;
				thisToken = toks.at(pos);
				string thisLexeme;
				thisLexeme = thisToken.lexemeName;
				while (!prevOp.empty())
				{
					prevOp.pop();
				}

				char thisChar = sepMatch.top();
				sepMatch.pop();
				if ((thisChar == '{') && (loops[2] == true))
				{
					file << "<Declaration>\n";
					body[0] = false;
					loops[2] = false;
					ins[0] = 16;
					AssemblyCodeGeneration("", "");
					ACI[jumpzLoc[0]].addOrVal = to_string(counter + 1);
				}
				else if (thisChar == '{')
				{
					file << "<Declaration>\n";
					body[0] = false;
				}
				else
				{
					invalid[0] = true;
				}
			}
			if (tokChar == '(')
			{
				if (pos == 0)
				{
					invalid[0] = true;
				}
				else
				{
					TokenType thisToken;
					thisToken = toks.at(pos - 1);
					string thisLexeme;
					thisLexeme = thisToken.lexemeName;
					if (loops[0] == true)
					{
						loops[0] = false;
						loops[1] = true;
					}
					if ((thisLexeme == "IDENTIFIER") || (thisLexeme == "SEPARATOR") || (thisLexeme == "KEYWORD") || (thisLexeme == "OPERATOR"))
					{
						sepMatch.push('(');
						file << "<Parameter> -> );\n";
						idenState[0] = false;
						idenState[1] = true;
					}
				}
			}
			if (tokChar == ')')
			{
				TokenType thisToken;
				thisToken = toks.at(pos);
				string thisLexeme;
				thisLexeme = thisToken.lexemeName;
				if (loops[1] == true)
				{
					loops[1] = false;
					loops[2] = true;
					idenState[0] = true;
					idenState[1] = false;
					idenState[2] = false;
					opState[0] = false;
					invalid[0] = false;
					while (!prevOp.empty())
					{
						prevOp.pop();
					}


					TokenType tempToken = variables.top();
					variables.pop();
					TokenType tempToken2 = variables.top();
					variables.pop();
					if (tempToken.lexemeName == "INTEGER")
					{
						ins[0] = 0;
						AssemblyCodeGeneration(tempToken.token, "");
						ins[0] = 2;
						tempToken = variables.top();
						variables.pop();
						AssemblyCodeGeneration("", tempToken.token);

					}
					else if (tempToken.lexemeName == "IDENTIFIER")
					{
						ins[0] = 1;
						AssemblyCodeGeneration("", tempToken2.token);
						AssemblyCodeGeneration("", tempToken.token);
					}
					//<
					if (relop[0] == 1)
					{
						ins[0] = 10;
						AssemblyCodeGeneration("", "");
						ins[0] = 15;
						AssemblyCodeGeneration("", "");
						relop[0] = 0;
					}
					//>
					if (relop[0] == 2)
					{
						ins[0] = 9;
						AssemblyCodeGeneration("", "");
						ins[0] = 15;
						AssemblyCodeGeneration("", "");
						relop[0] = 0;
					}
					//==
					if (relop[0] == 3)
					{
						ins[0] = 11;
						AssemblyCodeGeneration("", "");
						ins[0] = 15;
						AssemblyCodeGeneration("", "");
						relop[0] = 0;
					}
					//!=
					if (relop[0] == 4)
					{
						ins[0] = 12;
						AssemblyCodeGeneration("", "");
						ins[0] = 15;
						AssemblyCodeGeneration("", "");
						relop[0] = 0;
					}
					//<=
					if (relop[0] == 5)
					{
						ins[0] = 14;
						AssemblyCodeGeneration("", "");
						ins[0] = 15;
						AssemblyCodeGeneration("", "");
						relop[0] = 0;
					}
					//>=
					if (relop[0] == 6)
					{
						ins[0] = 13;
						AssemblyCodeGeneration("", "");
						ins[0] = 15;
						AssemblyCodeGeneration("", "");
						relop[0] = 0;
					}
				}
				else if (thisLexeme == "SEPARATOR")
				{
					char thisChar = sepMatch.top();
					sepMatch.pop();
					if (thisChar == '(')
						file << "<Parameter> -> ;\n";
				}
			}
			if (tokChar == ':')
			{
				if (fun[3])
				{
					fun[4] = true;
				}
			}
			if (tokChar == ',')
			{

			}
		}
	}
}

void SymbolHandler(string symName)
{
	bool hit = false;
	if (symTab.size() == 0)
	{
		SymbolTable newTab;
		if (varDec[0])
			newTab.type = "integer";
		if (varDec[1])
			newTab.type = "boolean";
		newTab.name = symName;
		newTab.memAdd = memAddress;
		symTab.push_back(newTab);
		memAddress++;
	}
	else if ((varDec[0]) || (varDec[1]))
	{
		for (int i = 0; i < symTab.size(); ++i)
		{
			if ((symTab[i].name == symName) && ((varDec[0]) || (varDec[1])))
			{
				cout << "ERROR:: cannot declare another variable with the same name \n";
				hit = true;
			}
		}
		if (!hit)
		{
			SymbolTable newTab;
			if (varDec[0])
				newTab.type = "integer";
			if (varDec[1])
				newTab.type = "boolean";
			newTab.name = symName;
			newTab.memAdd = memAddress;
			symTab.push_back(newTab);
			memAddress++;
		}
	}
	else
	{
		for (int i = 0; i < symTab.size(); ++i)
		{
			//good
			if ((symTab[i].name == symName) && ((!varDec[0]) || (!varDec[1])))
			{
				hit = true;
			}
		}
		if (!hit)
		{
			cout << "ERROR:: variable not declared\n";
		}
	}
}

/*
*	Function: AssemblyCodeGeneration
*	Use - uses a code and generates assembly code instructions
*	Param - strings that represents an int value or variable name
*	Return - void
*/

void AssemblyCodeGeneration(string IV, string varName)
{
	int here;
	for (int i = 0; i < symTab.size(); ++i)
	{
		if (varName == symTab[i].name)
		{
			here = i;
		}
	}
	int subs;
	int subs2;

	//PUSHI
	if (ins[0] == 0)
	{
		ACI[counter].order = counter + 1;
		ACI[counter].instruction = "PUSHI";
		ACI[counter].addOrVal = IV;
		counter++;
		assemblyCode.push(stoi(IV));
	}
	//PUSHM
	else if (ins[0] == 1)
	{
		ACI[counter].order = counter + 1;
		ACI[counter].instruction = "PUSHM";
		ACI[counter].addOrVal = to_string(symTab[here].memAdd);
		counter++;
		subs = symTab[here].value;
		assemblyCode.push(subs);
	}
	//POPM
	else if (ins[0] == 2)
	{
		ACI[counter].order = counter + 1;
		ACI[counter].instruction = "POPM";
		ACI[counter].addOrVal = to_string(symTab[here].memAdd);
		counter++;
		subs = assemblyCode.top();
		assemblyCode.pop();
		symTab[here].value = subs;
	}
	//STDOUT
	else if (ins[0] == 3)
	{
		ACI[counter].order = counter + 1;
		ACI[counter].instruction = "STDOUT";
		counter++;
		subs = assemblyCode.top();
		//cout << subs << '\n';
	}
	//STDIN
	else if (ins[0] == 4)
	{
		ACI[counter].order = counter + 1;
		ACI[counter].instruction = "STDIN";
		cout << "Enter number: ";
		int userIn = 0;
		//cin >> userIn;
		counter++;
		assemblyCode.push(userIn);
	}
	//ADD
	else if (ins[0] == 5)
	{
		ACI[counter].order = counter + 1;
		ACI[counter].instruction = "ADD";
		counter++;
		subs = assemblyCode.top();
		assemblyCode.pop();
		subs2 = assemblyCode.top();
		assemblyCode.pop();
		assemblyCode.push(subs + subs2);
	}
	//SUB
	else if (ins[0] == 6)
	{
		ACI[counter].order = counter + 1;
		ACI[counter].instruction = "SUB";
		counter++;
		subs = assemblyCode.top();
		assemblyCode.pop();
		subs2 = assemblyCode.top();
		assemblyCode.pop();
		assemblyCode.push(subs2 - subs);
	}
	//MUL
	else if (ins[0] == 7)
	{
		ACI[counter].order = counter + 1;
		ACI[counter].instruction = "MUL";
		counter++;
		subs = assemblyCode.top();
		assemblyCode.pop();
		subs2 = assemblyCode.top();
		assemblyCode.pop();
		assemblyCode.push(subs2 * subs);
	}
	//DIV
	else if (ins[0] == 8)
	{
		ACI[counter].order = counter + 1;
		ACI[counter].instruction = "DIV";
		counter++;
		subs = assemblyCode.top();
		assemblyCode.pop();
		subs2 = assemblyCode.top();
		assemblyCode.pop();
		assemblyCode.push(subs2 / subs);
	}
	//GRT
	else if (ins[0] == 9)
	{
		ACI[counter].order = counter + 1;
		ACI[counter].instruction = "GRT";
		counter++;
		subs = assemblyCode.top();
		assemblyCode.pop();
		subs2 = assemblyCode.top();
		assemblyCode.pop();
		if (subs2 > subs)
			assemblyCode.push(1);
		else
			assemblyCode.push(0);
	}
	//LES
	else if (ins[0] == 10)
	{
		ACI[counter].order = counter + 1;
		ACI[counter].instruction = "LES";
		counter++;
		subs = assemblyCode.top();
		assemblyCode.pop();
		subs2 = assemblyCode.top();
		assemblyCode.pop();
		if (subs2 < subs)
			assemblyCode.push(1);
		else
			assemblyCode.push(0);
	}
	//EQU
	else if (ins[0] == 11)
	{
		ACI[counter].order = counter + 1;
		ACI[counter].instruction = "EQU";
		counter++;
		subs = assemblyCode.top();
		assemblyCode.pop();
		subs2 = assemblyCode.top();
		assemblyCode.pop();
		if (subs2 == subs)
			assemblyCode.push(1);
		else
			assemblyCode.push(0);
	}
	//NEQ
	else if (ins[0] == 12)
	{
		ACI[counter].order = counter + 1;
		ACI[counter].instruction = "GRT";
		counter++;
		subs = assemblyCode.top();
		assemblyCode.pop();
		subs2 = assemblyCode.top();
		assemblyCode.pop();
		if (subs2 != subs)
			assemblyCode.push(1);
		else
			assemblyCode.push(0);
	}
	//GEQ
	else if (ins[0] == 13)
	{
		ACI[counter].order = counter + 1;
		ACI[counter].instruction = "GEQ";
		counter++;
		subs = assemblyCode.top();
		assemblyCode.pop();
		subs2 = assemblyCode.top();
		assemblyCode.pop();
		if (subs2 >= subs)
			assemblyCode.push(1);
		else
			assemblyCode.push(0);
	}
	//LEQ
	else if (ins[0] == 14)
	{
		ACI[counter].order = counter + 1;
		ACI[counter].instruction = "LEQ";
		counter++;
		subs = assemblyCode.top();
		assemblyCode.pop();
		subs2 = assemblyCode.top();
		assemblyCode.pop();
		if (subs2 <= subs)
			assemblyCode.push(1);
		else
			assemblyCode.push(0);
	}
	//JUMPZ
	else if (ins[0] == 15)
	{
		ACI[counter].order = counter + 1;
		ACI[counter].instruction = "JUMPZ";
		jumpzLoc[0] = counter;
		counter++;
		subs = assemblyCode.top();
		assemblyCode.pop();
		if (subs == 0)
		{
			counter = counter;
		}
	}
	//JUMP
	else if (ins[0] == 16)
	{
		ACI[counter].order = counter + 1;
		ACI[counter].instruction = "JUMP";
		ACI[counter].addOrVal = to_string(jumpLoc[0]);
		++counter;
		//counter = jumpLoc[0];
	}
	//LABEL
	else
	{
		ACI[counter].order = counter + 1;
		ACI[counter].instruction = "LABEL";
		jumpLoc[0] = counter + 1;
		counter++;
	}
}
