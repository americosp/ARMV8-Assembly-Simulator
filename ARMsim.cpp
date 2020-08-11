#include <iostream>
#include <stdio.h>
#include <fstream>
#include <ostream>
#include <string>
#include <sstream>
#include <vector>
#include <math.h>
#include <istream>
#include <iomanip>
using namespace std;

//HELPER FUNCTIONS
string twosComplement(string& number)
{
	for (auto it = number.begin(); it != number.end(); ++it)
	{
		auto& bit = *it;

		bit = (bit == '0') ? '1' : '0';
	}
	return number;
}

unsigned long long stringToDec(string bits)	// Used for everything else
{
	string s = bits;
	unsigned long long value = std::stoull(s, 0, 2);
	return value;
}

signed long long stringToSignedDec(string bits)	// Used for immediate values and offsets
{
	if (bits.substr(0, 1) == "1")
	{
		string s = twosComplement(bits);
		signed long long value = std::stoll(s, 0, 2);
		return (-1)*(value + 1);
	}
	else
	{
		string s = bits;
		signed long long value = std::stoll(s, 0, 2);
		return value;
	}
}

unsigned long long getCategory(string bits)
{
	unsigned long long cat = stringToDec(bits.substr(0, 3));
	return cat;
}

vector<string> readInstructionVector(ifstream &file)
{
	vector<string> instructionVector;
	string line;

	if (file.is_open()) {
		while (getline(file, line))
		{
			instructionVector.push_back(line);
		}
		file.close();
	}
	return instructionVector;
}

string isZR(int s)
{
	string temp;
	if (s == 31)
	{
		temp = "ZR";
	}
	else if (s != 31)
	{
		temp = to_string(s);
	}
	return temp;
}



//INSTRUCTION CLASSES
struct Instruction {
	int category;
	string instructionName;
	int instructionPC;

	Instruction() {};
	virtual ~Instruction() {};
};

struct CAT1 : Instruction {
	int src1;
	int branchOffset;
	int newPCtarget;
	string opcode;

	CAT1(string bits, int PC)
	{
		category = 1;
		opcode = bits.substr(3, 5);
		if (opcode == "10000")
		{
			instructionName = "CBZ";
		}
		else if (opcode == "10001")
		{
			instructionName = "CBNZ";
		}
		instructionPC = PC;
		src1 = (int)stringToDec(bits.substr(8, 5));
		branchOffset = stringToSignedDec(bits.substr(13, 19));
		newPCtarget = (branchOffset << 2) + PC;
	}

	int CBZ(int* reg)
	{
		int test = reg[src1];

		if (test == 0)
		{
			return newPCtarget;
		}
		else
		{
			return instructionPC;
		}
	}

	int CBNZ(int* reg)
	{
		int test = reg[src1];

		if (test != 0)
		{
			return newPCtarget;
		}
		else
		{
			return instructionPC;
		}
	}


};

struct CAT2 : Instruction {
	int dest;
	int src1;
	signed long long immediateValue;
	string opcode;

	CAT2(string bits, int PC)
	{
		category = 2;
		opcode = bits.substr(3, 7);
		if (opcode == "1000000")
		{
			instructionName = "ORRI";
		}
		else if (opcode == "1000001")
		{
			instructionName = "EORI";
		}
		else if (opcode == "1000010")
		{
			instructionName = "ADDI";
		}
		else if (opcode == "1000011")
		{
			instructionName = "SUBI";
		}
		else if (opcode == "1000100")
		{
			instructionName = "ANDI";
		}
		instructionPC = PC;
		dest = (int)stringToDec(bits.substr(10, 5));
		src1 = (int)stringToDec(bits.substr(15, 5));
		immediateValue = stringToSignedDec(bits.substr(20, 12));
	}
};

struct CAT3 : Instruction {
	int dest;
	int src1;
	int src2;
	string opcode;

	CAT3(string bits, int PC)
	{
		category = 3;
		opcode = bits.substr(3, 8);
		if (opcode == "10100000")
		{
			instructionName = "EOR";
		}
		else if (opcode == "10100010")
		{
			instructionName = "ADD";
		}
		else if (opcode == "10100011")
		{
			instructionName = "SUB";
		}
		else if (opcode == "10100100")
		{
			instructionName = "AND";
		}
		else if (opcode == "10100101")
		{
			instructionName = "ORR";
		}
		else if (opcode == "10100110")
		{
			instructionName = "LSR";
		}
		else if (opcode == "10100111")
		{
			instructionName = "LSL";
		}
		instructionPC = PC;
		dest = (int)stringToDec(bits.substr(11, 5));
		src1 = (int)stringToDec(bits.substr(16, 5));
		src2 = (int)stringToDec(bits.substr(21, 5));
	}
};

struct CAT4 : Instruction {
	int srcdst;
	int src1;
	signed long long immediateValue;
	string opcode;

	CAT4(string bits, int PC)
	{
		category = 4;
		opcode = bits.substr(3, 8);
		if (opcode == "10101010")
		{
			instructionName = "LDUR";
		}
		else if (opcode == "10101011")
		{
			instructionName = "STUR";
		}
		instructionPC = PC;
		srcdst = (int)stringToDec(bits.substr(11, 5));
		src1 = (int)stringToDec(bits.substr(16, 5));
		immediateValue = stringToSignedDec(bits.substr(21, 11));
	}

	void LDUR(vector<Instruction*> &vec, int* reg, vector<int> &mem, int firstMem)
	{
		int memData = reg[src1] + immediateValue;
		int data = mem[(memData-firstMem)/4];
		reg[srcdst] = data;
	}

	void STUR(vector<Instruction*> &vec, int* reg, vector<int> &mem, int firstMem)
	{
		int storeValue = reg[srcdst];
		int memLoc = (reg[src1] + immediateValue - firstMem) / 4;
		mem[memLoc] = storeValue;
	}
};

struct DUMMY : Instruction {

	DUMMY(string bits, int PC)
	{
		category = 5;
		instructionName = "DUMMY";
		instructionPC = PC;
	}
};

struct MEM : Instruction {
	int data;

	MEM(string bits, int PC)
	{
		category = 6;
		instructionName = "MEMLOCATIONS";
		instructionPC = PC;
		data = stringToSignedDec(bits);
	}
};

//REGISTER CLASS
struct REG {
	int value;
	int regNum;

	REG() {

	}
};


//NEW FILE METHODS

int getFirstMemLocation(vector<Instruction*> &vec)
{
	int temp;

	for (vector<Instruction*>::iterator p = vec.begin(); p != vec.end(); ++p)	// iterate through vector and determine which type of object we are dealing with 
	{
		CAT1* pCAT1 = dynamic_cast<CAT1*>(*p);
		CAT2* pCAT2 = dynamic_cast<CAT2*>(*p);
		CAT3* pCAT3 = dynamic_cast<CAT3*>(*p);
		CAT4* pCAT4 = dynamic_cast<CAT4*>(*p);
		DUMMY* pDUMMY = dynamic_cast<DUMMY*>(*p);
		MEM* pMEM = dynamic_cast<MEM*>(*p);

		if (pDUMMY)
		{
			temp = pDUMMY->instructionPC + 4;
		}
	}
	return temp;
}

void printSim(vector<Instruction*> &vec, vector<string> &encoded, int* reg, vector<int> &mem, ofstream &file, int cycle, int step)
{
	int firstMemLocation = getFirstMemLocation(vec);
	int count = 0;

	for (vector<Instruction*>::iterator p = vec.begin() + step - 1; p != vec.end(); ++p)	// iterate through vector and determine which type of object we are dealing with 
	{
		CAT1* pCAT1 = dynamic_cast<CAT1*>(*p);
		CAT2* pCAT2 = dynamic_cast<CAT2*>(*p);
		CAT3* pCAT3 = dynamic_cast<CAT3*>(*p);
		CAT4* pCAT4 = dynamic_cast<CAT4*>(*p);
		DUMMY* pDUMMY = dynamic_cast<DUMMY*>(*p);
		MEM* pMEM = dynamic_cast<MEM*>(*p);

		bool found = false;
		file << "--------------------" << "\n" << "Cycle " << cycle << ":" << "\t";
		if (pCAT1)
		{
			file << pCAT1->instructionPC << "\t"
				<< pCAT1->instructionName << " X" << pCAT1->src1 << ", #" << pCAT1->branchOffset << "\n";
			found = true;
		}
		else if (pCAT2)
		{
			file << pCAT2->instructionPC << "\t"
				<< pCAT2->instructionName << " X" << pCAT2->dest << ", X" << pCAT2->src1 << ", #" << pCAT2->immediateValue << "\n";
			found = true;
		}
		else if (pCAT3)
		{
			file << pCAT3->instructionPC << "\t"
				<< pCAT3->instructionName << " X" << pCAT3->dest << ", X" << pCAT3->src1 << ", X" << pCAT3->src2 << "\n";
			found = true;
		}
		else if (pCAT4)
		{
			file << pCAT4->instructionPC << "\t"
				<< pCAT4->instructionName << " X" << pCAT4->srcdst << ", [X" << isZR(pCAT4->src1) << ", #" << pCAT4->immediateValue << "]\n";
			found = true;
		}
		else if (pDUMMY)
		{
			file << pDUMMY->instructionPC << "\t"
				<< pDUMMY->instructionName << "\n";
			found = true;
		}
		else if (pMEM)
		{
			file << pMEM->instructionPC << "\t"
				<< pMEM->data << "\n";
			found = true;
		}
		
		if (found = true)
		{
			file << "\n" << "Registers" << "\n" << "X00:" << "\t" <<
				reg[0] << "\t" << reg[1] << "\t" << reg[2] << "\t" << reg[3] << "\t" << reg[4] << "\t" << reg[5] << "\t" << reg[6] << "\t" << reg[7] << "\t" << "\n" << "X08:" << "\t" <<
				reg[8] << "\t" << reg[9] << "\t" << reg[10] << "\t" << reg[11] << "\t" << reg[12] << "\t" << reg[13] << "\t" << reg[14] << "\t" << reg[15] << "\t" << "\n" << "X16:" << "\t" <<
				reg[16] << "\t" << reg[17] << "\t" << reg[18] << "\t" << reg[19] << "\t" << reg[20] << "\t" << reg[21] << "\t" << reg[22] << "\t" << reg[23] << "\t" << "\n" << "X24:" << "\t" <<
				reg[24] << "\t" << reg[25] << "\t" << reg[26] << "\t" << reg[27] << "\t" << reg[28] << "\t" << reg[29] << "\t" << reg[30] << "\t" << reg[31] << "\t" << "\n" <<
				"\n" << "Data" << "\n" << firstMemLocation << ":" << "\t" <<
				mem[0] << "\t" << mem[1] << "\t" << mem[2] << "\t" << mem[3] << "\t" << mem[4] << "\t" << mem[5] << "\t" << mem[6] << "\t" << mem[7] << "\t" <<
				"\n" << firstMemLocation + 32 << ":" << "\t" <<
				mem[8] << "\t" << mem[9] << "\t" << mem[10] << "\t" << mem[11] << "\n" << endl;
			break;
		}
		++count;
		++cycle;
		++step;
	}
}

void newDisassemblyFile(vector<Instruction*> &vec, vector<string> &encoded, vector<int> &mem)
{
	ofstream file("disassembly_output.txt");

	if (file.is_open())
	{
		int count = 0;
		for (vector<Instruction*>::iterator p = vec.begin(); p != vec.end(); ++p)	// iterate through vector and determine which type of object we are dealing with 
		{
			CAT1* pCAT1 = dynamic_cast<CAT1*>(*p);
			CAT2* pCAT2 = dynamic_cast<CAT2*>(*p);
			CAT3* pCAT3 = dynamic_cast<CAT3*>(*p);
			CAT4* pCAT4 = dynamic_cast<CAT4*>(*p);
			DUMMY* pDUMMY = dynamic_cast<DUMMY*>(*p);
			MEM* pMEM = dynamic_cast<MEM*>(*p);

			if (pCAT1)
			{
				file << encoded.at(count) << "\t" << pCAT1->instructionPC << "\t"
					<< pCAT1->instructionName << " X" << pCAT1->src1 << ", #" << pCAT1->branchOffset << "\n";
			}
			else if (pCAT2)
			{
				file << encoded.at(count) << "\t" << pCAT2->instructionPC << "\t"
					<< pCAT2->instructionName << " X" << pCAT2->dest << ", X" << pCAT2->src1 << ", #" << pCAT2->immediateValue << "\n";
			}
			else if (pCAT3)
			{
				file << encoded.at(count) << "\t" << pCAT3->instructionPC << "\t"
					<< pCAT3->instructionName << " X" << pCAT3->dest << ", X" << pCAT3->src1 << ", X" << pCAT3->src2 << "\n";
			}
			else if (pCAT4)
			{
				file << encoded.at(count) << "\t" << pCAT4->instructionPC << "\t"
					<< pCAT4->instructionName << " X" << pCAT4->srcdst << ", [X" << isZR(pCAT4->src1) << ", #" << pCAT4->immediateValue << "]\n";
			}
			else if (pDUMMY)
			{
				file << encoded.at(count) << "\t" << pDUMMY->instructionPC << "\t"
					<< pDUMMY->instructionName << "\n";
			}
			else if (pMEM)
			{
				file << encoded.at(count) << "\t" << pMEM->instructionPC << "\t"
					<< pMEM->data << "\n";
				mem.push_back(pMEM->data);

			}

			++count; //file << pCAT1->branchOffset << endl;		example of accessing the object in Instruction vector
		}
		file.close();
	}
}

void newSimulationFile(vector<Instruction*> &vec, vector<string> &encoded, vector<int> &mem)
{
	int reg[32] = { 0 };
	ofstream file("simulation_output.txt");
	int cycle = 1;
	int step = 1;
	if (file.is_open())
	{
		for (vector<Instruction*>::iterator p = vec.begin(); p != vec.end(); ++p)	// iterate through vector and determine which type of object we are dealing with 
		{
			CAT1* pCAT1 = dynamic_cast<CAT1*>(*p);
			CAT2* pCAT2 = dynamic_cast<CAT2*>(*p);
			CAT3* pCAT3 = dynamic_cast<CAT3*>(*p);
			CAT4* pCAT4 = dynamic_cast<CAT4*>(*p);
			DUMMY* pDUMMY = dynamic_cast<DUMMY*>(*p);
			MEM* pMEM = dynamic_cast<MEM*>(*p);
			int PCoffset;

			if (pCAT1)
			{
				if (pCAT1->instructionName == "CBZ")
				{
					int PC = pCAT1->instructionPC;
					PCoffset = pCAT1->CBZ(reg) - PC;

					if (PCoffset != 0)
					{
						p += (PCoffset / 4) - 1;
						step += (PCoffset / 4);
					}

				}
				else if (pCAT1->instructionName == "CBZ")
				{
					int PC = pCAT1->instructionPC;
					PCoffset = pCAT1->CBNZ(reg) - PC;
					if (PCoffset != 0)
					{
						p += (PCoffset / 4) - 1;
						step += (PCoffset / 4);
					}
				}
			}
			else if (pCAT2)
			{
				
			}
			else if (pCAT3)
			{
				
			}
			else if (pCAT4)
			{
				if (pCAT4->instructionName == "LDUR")
				{
					pCAT4->LDUR(vec, reg, mem, getFirstMemLocation(vec));
				}
				else if (pCAT4->instructionName == "STUR")
				{
					pCAT4->STUR(vec, reg, mem, getFirstMemLocation(vec));
				}
			}
			else if (pDUMMY)
			{
				
			}
			else if (pMEM)
			{

			}
			reg[31] = 0;
			printSim(vec, encoded, reg, mem, file, cycle, step);
			++cycle;
			++step;
		}
	}
	file.close();
}


int main(int argc, char *argv[])
{
	ifstream inputFile(argv[1]);
	//ifstream inputFile("sample.txt");	// DONT FORGET TO SWITCH TO COMMAND INPUT
	vector <string> encodedIntructions = readInstructionVector(inputFile);
	vector<Instruction*> decodedInstructions;
	
	vector<int> memory;

	bool dummyFlag = false;
	int programCounter = 64;
	for (int i = 0; i < encodedIntructions.size(); i++)
	{
		if (dummyFlag != true) {

			unsigned long long category = getCategory(encodedIntructions.at(i));
			switch (category)
			{
			case 1:
				decodedInstructions.push_back(new CAT1(encodedIntructions.at(i), programCounter));
				break;

			case 2:
				decodedInstructions.push_back(new CAT2(encodedIntructions.at(i), programCounter));
				break;

			case 3:
				decodedInstructions.push_back(new CAT3(encodedIntructions.at(i), programCounter));
				break;

			case 4:
				decodedInstructions.push_back(new CAT4(encodedIntructions.at(i), programCounter));
				break;

			case 5: // DUMMY	|	 output "DUMMY" and set DUMMY flag true
				decodedInstructions.push_back(new DUMMY(encodedIntructions.at(i), programCounter));
				dummyFlag = true;
				break;
			}
			programCounter+=4;
		}
		else if (dummyFlag == true)
		{
			decodedInstructions.push_back(new MEM(encodedIntructions.at(i), programCounter));
			programCounter+=4;
		}
	}

	newDisassemblyFile(decodedInstructions, encodedIntructions, memory);
	newSimulationFile(decodedInstructions, encodedIntructions, memory);

	return 0;
}