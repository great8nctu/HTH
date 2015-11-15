#ifndef CIRCUIT_H
#define CIRCUIT_H
#include<string>
#include<vector>
#include<map>
#include"typedef.h"
#include"aging.h"

using namespace std;

void ReadCircuit(string filename);
void ReadPath_l(string filename);
//void ReadPath_s(string filename);
void CalVertexWeight();
void ChooseVertexWithGreedyMDS(int year);
int HashAllClockBuffer();
void GenerateSAT(string filename,int year);
bool CallSatAndReadReport();
void CheckPathAttackbility(int year);
double CalQuality(int year);
bool RefineResult(int year);
void EstimateTimeEV(double year);
void CalSolMines(double year, int p);

class GATE;

class WIRE{
private:
	WIRETYPE type;
	string name;
	GATE* input;
	vector<GATE*> output;
public:
	WIRE(string n,WIRETYPE t) :name(n), type(t),input(NULL){ output.clear(); }
	~WIRE(){}
	WIRETYPE GetType(){ return type; }
	string GetName(){ return name; }
	GATE* GetInput(){ return input; }
	GATE* GetOutput(int i){ return output[i]; }
	int No_Output(){ return output.size(); }
	void SetInput(GATE* g){ input = g; }
	void SetOutput(GATE* g){ output.push_back(g); }
};

class GATE{
private:
	string name;
	string type;
	vector<WIRE*>input;
	//vector<WIRE*>output;
	WIRE* output;
	bool clock_flag;	//flip-flop��clock-path�O�_�w�g�إߧ���	
	AGINGTYPE add_dcc;
	vector<GATE*>clock_path;	//flip-flop�M��,�O���즹filp-flop��clock path
	double in_time, out_time;		//clock-buffer��,�O��clock��F���ɶ�(�o���|�]�����PFF�����|�Ӥ��P)
public:
	GATE(string n,string t) :name(n),type(t),in_time(-1),out_time(-1),clock_flag(false),add_dcc(DCC_NONE){
		input.clear();
		//output.clear();
		output = NULL;
		clock_path.clear();
	}
	~GATE(){}
	string GetName(){ return name; }
	string GetType(){ return type; }
	int No_Input(){ return input.size(); }
	void SetInput(WIRE* w){ input.push_back(w);	}
	void SetOutput(WIRE* w){ 
		//output.push_back(w);
		output = w;
	}
	WIRE* GetInput(int i){ return input[i]; }
	//WIRE* GetOutput(int i){ return output[i]; }
	WIRE* GetOutput(){ return output; }
	void SetClockPath(GATE* g){
		if (clock_flag)
			return;
		clock_path.push_back(g); 
	}
	GATE* GetClockPath(int i){ return clock_path[i]; }			//���|�I? -> �q�_�I�]�줣�ۦP�B�Y�i
	int Clock_Length(){ return clock_path.size(); }
	void SetInTime(double t){ in_time = t; }					//in/out time�bshort/long path �ۮt1.0
	void SetOutTime(double t){ out_time = t; }
	double GetInTime(){ return in_time; }
	double GetOutTime(){ return out_time; }
	void Setflag(){ clock_flag = true; }
	void SetDcc(AGINGTYPE dcc){ add_dcc = dcc; }
	AGINGTYPE GetDcc(){ return add_dcc; }
};

class CIRCUIT{
private:
	string name;
	vector<GATE*>gate_list;
	vector<WIRE*>wire_list;
	map<string, GATE*>nametogate;
	map<string, WIRE*>nametowire;
	
public:
	CIRCUIT(string n):name(n){
		gate_list.clear();
		wire_list.clear();
	}	
	string GetName(){ return name; }
	void PutWire(WIRE* w){ 
		wire_list.push_back(w);
		nametowire[w->GetName()] = w;
	}
	void PutGate(GATE* g){ 
		gate_list.push_back(g);
		nametogate[g->GetName()] = g;
	}
	WIRE* GetWire(int i){ return wire_list[i]; }
	WIRE* GetWire(string name){
		if (nametowire.find(name) == nametowire.end()){
			WIRE* t = new WIRE(name, PI);
			wire_list.push_back(t);
			nametowire[name] = t;
			cout << name << endl;
		}
		return nametowire[name]; 
	}
	GATE* GetGate(string name){
		if (nametogate.find(name) == nametogate.end())	return NULL;
		return nametogate[name];
	}
	GATE* GetGate(int i){ return gate_list[i]; }
	void PutClockSource();
};

class PATH{		//�����Otrans time
private:
	class TIMING{
	private:
		double in, out;
	public:
		TIMING(double i, double o) :in(i), out(o){}
		~TIMING(){}
		double in_time(){ return in; }
		double out_time(){ return out; }
	};
	//���ΰOgate�������s�u�W(�����e�@��gate����X�u),�e�@��gate����X�M��@��gate����J�t���s�u����
	vector<GATE*> gate_list;		//�Ĥ@�ӫ�̫�@�ӥ���flip-flop/PI/PO
	vector<TIMING>timing;
	double setuptime;
	double holdtime;
	double clock_to_end;
	double estime;
	int weight;
	bool attackable;
	bool choose;
	PATHTYPE type;
public:
	PATH():attackable(false),choose(false){
		setuptime = holdtime = -1;
		gate_list.clear();
		timing.clear();
	}
	~PATH(){}
	void AddGate(GATE* g, double i,double o){
		gate_list.push_back(g);
		TIMING temp(i, o);
		timing.push_back(temp);
	}

	GATE* Gate(int i){ return gate_list[i]; }
	double In_time(int i){ return timing[i].in_time(); }
	double Out_time(int i){ return timing[i].out_time(); }	
	
	/*
	int Find_Gate(string name){		//��"���X"
		for (unsigned i = 0; i < gate_list.size();i++){
			if (gate_list[i]->GetName() == name)
				return i;
		}
		return -1;
	}
	*/
	
	void SetST(double t){ setuptime = t; }
	void SetHT(double t){ holdtime = t; }
	void SetCTE(double t){ clock_to_end = t; }
	double GetST(){ return setuptime; }
	double GetHT(){ return holdtime; }
	double GetCTE(){ return clock_to_end; }
	double GetCTH(){ return timing[0].in_time(); }
	double GetAT(){ return timing[timing.size()-1].in_time(); }
	void CalWeight();
	int GetWeight(){ return weight; }
	void SetType(PATHTYPE t){ type = t; }
	PATHTYPE GetType(){ return type; }
	int length(){ return gate_list.size(); }
	void SetAttack(bool a){ attackable = a; }
	bool CheckAttack(){ return attackable; }
	bool Is_Chosen(){ return choose; }
	void SetChoose(bool c){ choose = c; }
	void SetEstimateTime(double t){ estime = t; }
	double GetEstimateTime(){ return estime; }
};


#endif