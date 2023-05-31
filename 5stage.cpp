



// #include <unordered_map>
// #include <string>
// #include <functional>
// #include <vector>
// #include <fstream>
// #include <exception>
// #include <iostream>
#include <boost/tokenizer.hpp>
#include <bits/stdc++.h>

using namespace std;



struct MIPS_Architecture
{
	int registers[32] = {0}, PCcurr = 0, PCnext=0; // registers contain the values of the registers
	std::unordered_map<std::string, std::function<int(MIPS_Architecture &, std::string, std::string, std::string)>> instructions;
	std::unordered_map<std::string, int> registerMap, address;
	static const int MAX = (1 << 20);
	int data[MAX >> 2] = {0}; // for memory management
	std::vector<std::vector<std::string>> commands; //vector of vector of strings
	std::vector<int> commandCount; //cycle count for each command;
	enum exit_code
	{
		SUCCESS = 0,
		INVALID_REGISTER,
		INVALID_LABEL,
		INVALID_ADDRESS,
		SYNTAX_ERROR,
		MEMORY_ERROR
	};
	int pip[5];//5 stage pipeline IF,ID,EX,MM,WR
	//int ful[5];// whether a stage is completed or not(similar to a latch)
	unordered_map<string,int> breg;// whether a reg is getting dependant on that value
	vector<int> mem;


	// constructor to initialise the instruction set
	MIPS_Architecture(std::ifstream &file)
	{
		instructions = {{"add", &MIPS_Architecture::add}, {"sub", &MIPS_Architecture::sub}, {"mul", &MIPS_Architecture::mul}, {"beq", &MIPS_Architecture::beq}, {"bne", &MIPS_Architecture::bne}, {"slt", &MIPS_Architecture::slt}, {"j", &MIPS_Architecture::j}, {"lw", &MIPS_Architecture::lw}, {"sw", &MIPS_Architecture::sw}, {"addi", &MIPS_Architecture::addi} };

		for (int i = 0; i < 32; ++i)
			registerMap["$" + std::to_string(i)] = i;
		registerMap["$zero"] = 0;
		registerMap["$at"] = 1;
		registerMap["$v0"] = 2;
		registerMap["$v1"] = 3;
		for (int i = 0; i < 4; ++i)
			registerMap["$a" + std::to_string(i)] = i + 4;
		for (int i = 0; i < 8; ++i)
			registerMap["$t" + std::to_string(i)] = i + 8, registerMap["$s" + std::to_string(i)] = i + 16;
		registerMap["$t8"] = 24;
		registerMap["$t9"] = 25;
		registerMap["$k0"] = 26;
		registerMap["$k1"] = 27;
		registerMap["$gp"] = 28;
		registerMap["$sp"] = 29;
		registerMap["$s8"] = 30;
		registerMap["$ra"] = 31;

		constructCommands(file);
		commandCount.assign(commands.size(), 0);
		for(int i=0;i<5;i++){
			pip[i]=-1;
			//ful[i]=0;
		}
		unordered_map<string,int>::iterator it;
		for(it=registerMap.begin();it!=registerMap.end();it++){
			breg[it->first]=0;
			// cout<<it->first<<endl;
		}
		//cout<<MAX<<endl;
	}

	// perform add operation
	int add(std::string r1, std::string r2, std::string r3)
	{
		return op(r1, r2, r3, [&](int a, int b)
				  { return a + b; });
	}

	

	// perform subtraction operation
	int sub(std::string r1, std::string r2, std::string r3)
	{
		return op(r1, r2, r3, [&](int a, int b)
				  { return a - b; });
	}

	// perform multiplication operation
	int mul(std::string r1, std::string r2, std::string r3)
	{
		return op(r1, r2, r3, [&](int a, int b)
				  { return a * b; });
	}

	// perform the binary operation
	int op(std::string r1, std::string r2, std::string r3, std::function<int(int, int)> operation)
	{
		if (!checkRegisters({r1, r2, r3}) || registerMap[r1] == 0)
			return 1;
		registers[registerMap[r1]] = operation(registers[registerMap[r2]], registers[registerMap[r3]]);
		//PCnext = PCcurr + 1;
		return 0;
	}

	// perform the beq operation
	int beq(std::string r1, std::string r2, std::string label)
	{
		return bOP(r1, r2, label, [](int a, int b)
				   { return a == b; });
	}

	// perform the bne operation
	int bne(std::string r1, std::string r2, std::string label)
	{
		return bOP(r1, r2, label, [](int a, int b)
				   { return a != b; });
	}

	// implements beq and bne by taking the comparator
	int bOP(std::string r1, std::string r2, std::string label, std::function<bool(int, int)> comp)
	{
		if (!checkLabel(label))
			return 4;
		if (address.find(label) == address.end() || address[label] == -1)
			return 2;
		if (!checkRegisters({r1, r2}))
			return 1;
		if(comp(registers[registerMap[r1]], registers[registerMap[r2]])){
			PCnext = address[label];
		}
		//PCnext = comp(registers[registerMap[r1]], registers[registerMap[r2]]) ? address[label] : PCcurr + 1;
		return 0;
	}

	// implements slt operation
	int slt(std::string r1, std::string r2, std::string r3)
	{
		if (!checkRegisters({r1, r2, r3}) || registerMap[r1] == 0)
			return 1;
		registers[registerMap[r1]] = registers[registerMap[r2]] < registers[registerMap[r3]];
		//PCnext = PCcurr + 1;
		return 0;
	}

	// perform the jump operation
	int j(std::string label, std::string unused1 = "", std::string unused2 = "")
	{
		if (!checkLabel(label))
			return 4;
		if (address.find(label) == address.end() || address[label] == -1)
			return 2;
		PCnext = address[label];
		return 0;
	}

	// perform load word operation
	int lw(std::string r, std::string location, std::string unused1 = "")
	{
		if (!checkRegister(r) || registerMap[r] == 0)
			return 1;
		int address = locateAddress(location);
		if (address < 0)
			return abs(address);
		registers[registerMap[r]] = data[address];
		//PCnext = PCcurr + 1;
		return 0;
	}

	// perform store word operation
	int sw(std::string r, std::string location, std::string unused1 = "")
	{
		if (!checkRegister(r))
			return 1;
		int address = locateAddress(location);
		if (address < 0)
			return abs(address);
		data[address] = registers[registerMap[r]];
		//cout<<1<<" "<<address*4<<" "<<registers[registerMap[r]]<<endl;;
		mem.push_back(address*4);
		mem.push_back(registers[registerMap[r]]);
		//PCnext = PCcurr + 1;
		return 0;
	}

	int locateAddress(std::string location)
	{
		if (location.back() == ')')
		{
			try
			{
				int lparen = location.find('('), offset = stoi(lparen == 0 ? "0" : location.substr(0, lparen));
				std::string reg = location.substr(lparen + 1);
				reg.pop_back();
				if (!checkRegister(reg))
					return -3;
				int address = registers[registerMap[reg]] + offset;
				if (address % 4 || address < int(4 * commands.size()) || address >= MAX)
					return -3;
				return address / 4;
			}
			catch (std::exception &e)
			{
				return -4;
			}
		}
		try
		{
			int address = stoi(location);
			if (address % 4 || address < int(4 * commands.size()) || address >= MAX)
				return -3;
			return address / 4;
		}
		catch (std::exception &e)
		{
			return -4;
		}
	}

	// perform add immediate operation
	int addi(std::string r1, std::string r2, std::string num)
	{
		if (!checkRegisters({r1, r2}) || registerMap[r1] == 0)
			return 1;
		try
		{
			registers[registerMap[r1]] = registers[registerMap[r2]] + stoi(num);
			//PCnext = PCcurr + 1;
			return 0;
		}
		catch (std::exception &e)
		{
			return 4;
		}
	}

	// checks if label is valid
	inline bool checkLabel(std::string str)
	{
		return str.size() > 0 && isalpha(str[0]) && all_of(++str.begin(), str.end(), [](char c)
														   { return (bool)isalnum(c); }) &&
			   instructions.find(str) == instructions.end();
	}

	// checks if the register is a valid one
	inline bool checkRegister(std::string r)
	{
		return registerMap.find(r) != registerMap.end();
	}

	// checks if all of the registers are valid or not
	bool checkRegisters(std::vector<std::string> regs)
	{
		return std::all_of(regs.begin(), regs.end(), [&](std::string r)
						   { return checkRegister(r); });
	}

	bool completed(){
		for(int i=0;i<5;i++){
			if(pip[i]>-1){
				return false;
			}
		}
		return true;
	}

	/*
		handle all exit codes:
		0: correct execution
		1: register provided is incorrect
		2: invalid label
		3: unaligned or invalid address
		4: syntax error
		5: commands exceed memory limit
	*/
	void handleExit(exit_code code, int cycleCount)
	{
		std::cout << '\n';
		switch (code)
		{
		case 1:
			std::cerr << "Invalid register provided or syntax error in providing register\n";
			break;
		case 2:
			std::cerr << "Label used not defined or defined too many times\n";
			break;
		case 3:
			std::cerr << "Unaligned or invalid memory address specified\n";
			break;
		case 4:
			std::cerr << "Syntax error encountered\n";
			break;
		case 5:
			std::cerr << "Memory limit exceeded\n";
			break;
		default:
			break;
		}
		if (code != 0)
		{
			std::cerr << "Error encountered at:\n";
			for (auto &s : commands[PCcurr])
				std::cerr << s << ' ';
			std::cerr << '\n';
		}
		std::cout << "\nFollowing are the non-zero data values:\n";
		for (int i = 0; i < MAX / 4; ++i)
			if (data[i] != 0)
				std::cout << 4 * i << '-' << 4 * i + 3 << std::hex << ": " << data[i] << '\n'
						  << std::dec;
		std::cout << "\nTotal number of cycles: " << cycleCount << '\n';
		std::cout << "Count of instructions executed:\n";
		for (int i = 0; i < (int)commands.size(); ++i)
		{
			std::cout << commandCount[i] << " times:\t";
			for (auto &s : commands[i])
				std::cout << s << ' ';
			std::cout << '\n';
		}
	}

	// parse the command assuming correctly formatted MIPS instruction (or label)
	void parseCommand(std::string line)
	{
		// strip until before the comment begins
		line = line.substr(0, line.find('#'));
		std::vector<std::string> command;
		boost::tokenizer<boost::char_separator<char>> tokens(line, boost::char_separator<char>(", \t"));
		for (auto &s : tokens)
			command.push_back(s);
		// empty line or a comment only line
		if (command.empty())
			return;
		else if (command.size() == 1)
		{
			std::string label = command[0].back() == ':' ? command[0].substr(0, command[0].size() - 1) : "?";
			if (address.find(label) == address.end())
				address[label] = commands.size();
			else
				address[label] = -1;
			command.clear();
		}
		else if (command[0].back() == ':')
		{
			std::string label = command[0].substr(0, command[0].size() - 1);
			if (address.find(label) == address.end())
				address[label] = commands.size();
			else
				address[label] = -1;
			command = std::vector<std::string>(command.begin() + 1, command.end());
		}
		else if (command[0].find(':') != std::string::npos)
		{
			int idx = command[0].find(':');
			std::string label = command[0].substr(0, idx);
			if (address.find(label) == address.end())
				address[label] = commands.size();
			else
				address[label] = -1;
			command[0] = command[0].substr(idx + 1);
		}
		else if (command[1][0] == ':')
		{
			if (address.find(command[0]) == address.end())
				address[command[0]] = commands.size();
			else
				address[command[0]] = -1;
			command[1] = command[1].substr(1);
			if (command[1] == "")
				command.erase(command.begin(), command.begin() + 2);
			else
				command.erase(command.begin(), command.begin() + 1);
		}
		if (command.empty())
			return;
		if (command.size() > 4)
			for (int i = 4; i < (int)command.size(); ++i)
				command[3] += " " + command[i];
		command.resize(4);
		commands.push_back(command);
	}

	// construct the commands vector from the input file
	void constructCommands(std::ifstream &file)
	{
		std::string line;
		while (getline(file, line))
			parseCommand(line);
		file.close();
	}

	string getReg(string location){
		string reg;
		if(location.back()==')'){
			int lparen = location.find('('), offset = stoi(lparen == 0 ? "0" : location.substr(0, lparen));
			reg = location.substr(lparen + 1);
			reg.pop_back();
		}else{
			return location;
		}
		
		return reg;
	}

	// execute the commands sequentially (no pipelining)
	void executeCommandsUnpipelined()
	{
		//printCommands();
		if (commands.size() >= MAX / 4)
		{
			handleExit(MEMORY_ERROR, 0);
			return;
		}

		int clockCycles = 0;
		while (PCcurr < commands.size())
		{
			++clockCycles;
			std::vector<std::string> &command = commands[PCcurr];
			if (instructions.find(command[0]) == instructions.end())
			{
				handleExit(SYNTAX_ERROR, clockCycles);
				return;
			}
			exit_code ret = (exit_code) instructions[command[0]](*this, command[1], command[2], command[3]);
			if (ret != SUCCESS)
			{
				handleExit(ret, clockCycles);
				return;
			}
			++commandCount[PCcurr];
			PCcurr = PCnext;
			printRegInt(clockCycles);
		}
		std::cout<<"exit "<<address["exit"]<<'\n';
		handleExit(SUCCESS, clockCycles);
	}

	
	void executeCommandspipelined()
	{
	
		//printCommands();
		// for(int i=0;i<5;i++){
		// 		cout<<pip[i]<<" ";
		// 	}
		// cout<<'\n'<<endl;
		int cyc=0;
		while(!completed() || PCnext<commands.size()){
			// first half cycle

			// writing registers (WR)
			if(cyc>100){
				return ;
			}
			if(pip[4]>=0){
				if(commands[pip[4]][0]=="add" || commands[pip[4]][0]=="addi" || commands[pip[4]][0]=="sub" || commands[pip[4]][0]=="mul" || commands[pip[4]][0]== "slt" || commands[pip[4]][0]=="lw" ){
					exit_code ret = (exit_code) instructions[commands[pip[4]][0]](*this, commands[pip[4]][1], commands[pip[4]][2], commands[pip[4]][3]);
					breg[commands[pip[4]][1]]=0;
				}
				
				++commandCount[pip[4]];
			}
			// ful[4]=0;
			pip[4]=-1;
			//cout<<"Writing to registers completed"<<endl;
			// for(int i=0;i<5;i++){
			// 	cout<<pip[i]<<" ";
			// }
			// cout<<'\n'<<endl;
			


			// memory management(MM)
			if(pip[3]>=0){
				++commandCount[pip[3]];
				if(commands[pip[3]][0]=="sw"){
					exit_code ret = (exit_code) instructions[commands[pip[3]][0]](*this, commands[pip[3]][1], commands[pip[3]][2], commands[pip[3]][3]);
					//breg[commands[pip[3]][1]]=0;
				}
				//ful[4]=1;	
				
			}
			// ful[3]=0;
			pip[4]=pip[3];
			pip[3]=-1;
			//cout<<"Memory Management done"<<endl;
			// for(int i=0;i<5;i++){
			// 	cout<<pip[i]<<" ";
			// }
			// cout<<'\n'<<endl;


			// ALU(EX))
			if(pip[2]>=0){
				++commandCount[pip[2]];
				if(commands[pip[2]][0]=="beq" || commands[pip[2]][0]=="bne" ){
					int oldn = PCnext;
					exit_code ret = (exit_code) instructions[commands[pip[2]][0]](*this, commands[pip[2]][1], commands[pip[2]][2], commands[pip[2]][3]);
					if(PCnext!=oldn){
						cout<<PCnext<<" "<<pip[1]<<endl;
						//cout<<"branch taken"<<endl;
						if(pip[1]>-1 && breg[commands[pip[1]][1]]==1){
							breg[getReg(commands[pip[1]][1])]=0;
						}
						for(int i=0;i<2;i++){
							pip[i]=-1;
							// ful[i]=0;
						}
					}else{
						//cout<<"branch not taken"<<endl;
					}
					pip[2]=-1;
					pip[3]=pip[2];
				}else{
					pip[3]=pip[2];
					pip[2]=-1;	
				}
				
			}else{
				pip[3]=pip[2];
			}
			// ful[2]=0;
			
			// cout<<"Arithmetic logical unit done"<<endl;

			// for(int i=0;i<5;i++){
			// 	cout<<pip[i]<<" ";
			// }
			// cout<<'\n'<<endl;


			// reading registrs(ID)
			if(pip[1]>=0 ){
				++commandCount[pip[1]];
				if( commands[pip[1]][0]== "add" || commands[pip[1]][0]=="sub" || commands[pip[1]][0]=="mul" || commands[pip[1]][0]=="slt" ){
					if(breg[getReg(commands[pip[1]][2])]!=1 && breg[getReg(commands[pip[1]][3])]!=1){
						pip[2]=pip[1];
						breg[getReg(commands[pip[1]][1])]=1;
						pip[1]=-1;
					}else{
						//cout<<"stalling"<<endl;
					}
				} else if( commands[pip[1]][0]=="beq" || commands[pip[1]][0]=="bne" ){
					if(breg[getReg(commands[pip[1]][1])]!=1 && breg[getReg(commands[pip[1]][2])]!=1){
						pip[2]=pip[1];
						pip[1]=-1;
					}else{
						//cout<<"stalling"<<endl;
					}
				}else if(commands[pip[1]][0]=="sw" ){
					if(breg[getReg(commands[pip[1]][2])]!=1 && breg[getReg(commands[pip[1]][1])]!=1 ){
						pip[2]=pip[1];
						pip[1]=-1;
					}else{
						//cout<<"stalling"<<endl;
					}
				}else if(commands[pip[1]][0]=="lw" ){
					if(breg[getReg(commands[pip[1]][2])]!=1 ){
						breg[getReg(commands[pip[1]][1])]=1;
						pip[2]=pip[1];
						pip[1]=-1;
					}else{
						//cout<<"stalling"<<endl;
					}
				}else if(commands[pip[1]][0]=="addi" ){
					//cout<<commands[pip[1]][0]<<" "<<commands[pip[1]][1]<<" "<<commands[pip[1]][2]<<" "<<commands[pip[1]][3]<<" "<<endl;
					if(breg[getReg(commands[pip[1]][2])]!=1 ){
						pip[2]=pip[1];
						breg[commands[pip[1]][1]]=1;
						pip[1]=-1;
					}else{
						//cout<<"stalling"<<endl;
					}
				}else if(commands[pip[1]][0]=="j"){
					exit_code ret = (exit_code) instructions[commands[pip[1]][0]](*this, commands[pip[1]][1], commands[pip[1]][2], commands[pip[1]][3]);
					pip[2]=-1;
					pip[1]=-1;
					pip[0]=-1;

				}else{
					pip[2]=pip[1];
					pip[1]=-1;
				}
				
			}else{
				pip[2]=-1;
			}
			//cout<<"Reading Registers done"<<endl;

			// for(int i=0;i<5;i++){
			// 	cout<<pip[i]<<" ";
			// }
			// cout<<'\n'<<endl;



			// Instruction fetch
			if(pip[0]>-1){
				++commandCount[pip[0]];
				if(pip[1]==-1){

					pip[1]=pip[0];
					// ful[1]=1;
					if(PCnext>=commands.size()){
						pip[0]=-1;
						// ful[0]=0;
					}else{
						pip[0]=PCnext;
						// ful[0]=1;
						PCnext++;
					}
					
				}
			}else{
				if(PCnext>=commands.size()){
					pip[0]=-1;
					// ful[0]=0;
				}else{
					pip[0]=PCnext;
					// ful[0]=1;
					PCnext++;
				}
				if(pip[1]==-1){
					pip[1]=-1;
				}
			}
			//cout<<"Fetching Instruction done"<<endl;
			//cout<<"PCnext "<<PCnext<<endl;

			// for(int i=0;i<5;i++){
			// 	cout<<pip[i]<<" ";
			// }
			// cout<<'\n';
			cyc++;
			printRegInt(cyc);
			if(mem.size()>0){
				cout<<1<<" "<<mem[0]<<" "<<mem[1]<<endl;
				mem.clear();
			}else{
				cout<<0<<endl;
			}
			//cout<<'\n';
			

			
		}
		// for(int i=0;i<commandCount.size();i++){
		// 	cout<<commandCount[i]<<endl;
		// }
		// cout<<"No of cycles "<<cyc<<endl;



		
	}

	// print the register data in hexadecimal
	void printRegisters(int clockCycle)
	{
		std::cout << " Cycle number: " << clockCycle << '\n'
				  << std::hex;
		for (int i = 0; i < 32; ++i)
			std::cout << registers[i] << ' ';
		std::cout << std::dec << '\n';
	}

	void printRegInt(int clockCycle)
	{
		//std::cout << "After Cycle number: " << clockCycle << '\n';
		for (int i = 0; i < 32; ++i)
			std::cout << registers[i] << ' ';
		std::cout  << '\n';
	}

	void printCommands(){
		for(int i=0;i<commands.size();i++){
			for(int j=0;j<commands[i].size();j++){
				std::cout<<commands[i][j]<<" ";
			}
			std::cout<<'\n';
		}
	}
};


int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		std::cerr << "Required argument: file_name\n./MIPS_interpreter <file name>\n";
		return 0;
	}
	std::ifstream file(argv[1]);
	MIPS_Architecture *mips;
	if (file.is_open())
		mips = new MIPS_Architecture(file);
	else
	{
		std::cerr << "File could not be opened. Terminating...\n";
		return 0;
	}
	mips->executeCommandspipelined();
	//mips->executeCommandsUnpipelined();
	return 0;
}


