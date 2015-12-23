#include<iostream>
#include<fstream>
#include<sstream>
#include<math.h>
#include "circuit.h"
#include<stdlib.h>
#include<algorithm>
#define ERROR 1

using namespace std;

extern vector<CIRCUIT> Circuit;
extern vector<PATH> PathR;
extern vector<PATH*> PathC;
extern double **EdgeA;
extern double **EdgeB;
extern double **cor;
extern int **conf;
extern double **ser;
double period;


double TransStringToDouble(string s){
	stringstream ss;
	ss << s;
	double result;
	ss >> result;
	return result;
}

string RemoveSpace(string s){
	unsigned i;
	for (i = 0; i < s.length(); i++)
		if (s[i] != ' ' && s[i] != 9 && s[i] != 13)		//9������tab,13������
			break;
	s = s.substr(i);
	return s;
}

void CIRCUIT::PutClockSource(){
	GATE* gptr = new GATE("ClockSource", "PI");
	gptr->SetInTime(0);
	gptr->SetOutTime(0);
	PutGate(gptr);
}

void ReadCircuit(string filename){
	fstream file;
	file.open(filename.c_str(),ios::in);
	char temp[1000];
	bool cmt = false;
	int Nowmodule = -1;
	while (file.getline(temp, 1000)){		//�@��h�өR�O�L�k�B�z
		string temps = temp;		
		if (cmt){
			if (temps.find("*/") != string::npos)
				cmt = false;
			continue;
		}
		if (temps.find("/*") != string::npos){
			if (temps.find("*/") == string::npos)
				cmt = true;
			temps = temps.substr(0,temps.find("/*"));
		}
		if (temps.find("//") != string::npos)
			temps = temps.substr(0, temps.find("//"));
		temps = RemoveSpace(temps);
		if (temps.empty())
			continue;		
		if (temps.find("endmodule") != string::npos)	
			continue;
		else if (temps.find("assign")!=string::npos)
			continue;
		else if (temps.find("module")!=string::npos){
			int st = temps.find("module") + 7;
			CIRCUIT TC(temps.substr(st, temps.find(" (") - st));
			Circuit.push_back(TC);
			Nowmodule++;
			while (file.getline(temp, 1000)){
				temps = temp;
				if (temps.find(")") != string::npos)
					break;
			}
		}		
		else if (temps.find("input")!=string::npos){
			int st = temps.find("input") + 6;
			WIRE* w = new WIRE(temps.substr(st,temps.find(";") - st),PI);
			Circuit[Nowmodule].PutWire(w);
		}
		else if (temps.find("output") != string::npos){
			int st = temps.find("output") + 7;
			WIRE* w = new WIRE(temps.substr(st, temps.find(";") - st), PO);
			Circuit[Nowmodule].PutWire(w);

		}
		else if (temps.find("wire") != string::npos){			
			int st = temps.find("wire") + 5;
			WIRE* w = new WIRE(temps.substr(st, temps.find(";") - st), INN);
			Circuit[Nowmodule].PutWire(w);
		}
		else{
			temps = RemoveSpace(temps);
			if (temps.empty())	continue;
			bool ok = false;
			for (int i = 0; i < Nowmodule; i++){
				if (Circuit[i].GetName() == temps.substr(0, temps.find(" "))){
					//�i��A�[�J�ǥѤ��eŪ�J��module(�Dlibrary)�ӫإ�gate,in/output���W�٤��ΰO��(�bmodule����),���n�ΥH���쥿�T���f
					ok = true;
					break;
				}
			}
			if (!ok){	//���]�Ĥ@�Ӭ�output,�䥦��input
				string moduleN = temps.substr(0, temps.find(" "));	//module name,gate name,gate's output�|�g�b�@��
				temps = temps.substr(temps.find(" ") + 1);
				string gateN = temps.substr(0, temps.find(" ("));
				temps = temps.substr(temps.find(" ") + 2);
				GATE* g = new GATE(gateN,moduleN);
				int st = temps.find("(");
				string ioN = temps.substr(st + 1, temps.find(")") - st - 1);
				g->SetOutput(Circuit[Nowmodule].GetWire(ioN));
				Circuit[Nowmodule].GetWire(ioN)->SetInput(g);
				temps = temps.substr(temps.find(")") + 1);
				while (file.getline(temp,1000)){		//gate's input �|�b�᭱�C��g�@��
					temps = temp;
					st = temps.find("(");
					string ioN = temps.substr(st + 1, temps.find(")") - st - 1);		//�|���u����1'b1,1'b0(�`��)
					g->SetInput(Circuit[Nowmodule].GetWire(ioN));
					Circuit[Nowmodule].GetWire(ioN)->SetOutput(g);
					//temps = temps.substr(temps.find(")") + 1);
					if (temps.find(";") != string::npos)	break;
				}
				Circuit[Nowmodule].PutGate(g);
			}
		}
		
		//system("pause");
	}	
	file.close();
	return;
}

void ReadPath_l(string filename){
	fstream file;
	file.open(filename.c_str(), ios::in);	
	string line,sp,ep;
	GATE *gptr = NULL, *spptr = NULL, *epptr = NULL;
	PATH* p = NULL;
	unsigned Path_No = 0;
	while (getline(file, line)){
		//cout << line << endl;
		if (line.find("Startpoint") != string::npos){
			if (PathR.size() >= MAXPATHS)
				return;
			p = new PATH();
			sp = line.substr(line.find("Startpoint") + 12);
			sp = sp.substr(0, sp.find(" "));
			spptr = Circuit[0].GetGate(sp);	//0��top-module
			if (spptr == NULL)	//�_�I��PI
				spptr = new GATE(sp,"PI");
		}
		else if (line.find("Endpoint") != string::npos){
			ep = line.substr(line.find("Endpoint") + 10);
			ep = ep.substr(0, ep.find(" "));
			epptr = Circuit[0].GetGate(ep);
			if (epptr == NULL)
				epptr = new GATE(ep, "PO");			
		}		

		if (line.find("---") == string::npos || sp == "")	continue;
		if (spptr->GetType() == "PI" && epptr->GetType() == "PO"){
			Path_No++;
			while (line.find("slack (MET)") == string::npos)	getline(file, line);
			continue;
		}
		getline(file, line);
		getline(file, line);		//�o2�檺�|�����b�Ĥ@��gate�ɶ��W ���ΰO
		//if (spptr->Clock_Length() == 0 && spptr->GetType()!="PI"){
		if (spptr->GetType() != "PI"){
			while (getline(file, line)){	//clock-source -> startpoint			
				line = RemoveSpace(line);
				if (sp == line.substr(0, line.find("/")))	break;
				if (line.find("(net)") != string::npos)	continue;
				else if (line.find("(in)") != string::npos){	//PI�ɶ����p,�p�G���~������᭱���R�A�[�J
					spptr->SetClockPath(Circuit[0].GetGate("ClockSource"));										
				}
				//else if (line.find("(out)") != string::npos){}
				else{
					string name = line.substr(0, line.find("/"));
					double intime = TransStringToDouble(line.substr(line.find("&") + 1));
					getline(file, line);
					double outtime = TransStringToDouble(line.substr(line.find("&") + 1));
					gptr = Circuit[0].GetGate(name);
					spptr->SetClockPath(gptr);
					gptr->SetInTime(intime);
					gptr->SetOutTime(outtime);
				}
			}
		}		
		if (spptr->GetType() == "PI"){		//�_�I��PI�����p
			while (line.find("(in)") == string::npos)	getline(file, line);
			p->AddGate(spptr, 0, TransStringToDouble(line.substr(line.find("&") + 1)));		//clock �� �_�I���ɶ���0 (PI),tcq = �~��delay
			getline(file, line);
		}
		do{
			line = RemoveSpace(line);
			if (ep == line.substr(0, line.find("/")) || line.find("(out)")!=string::npos)	break;
			if (line.find("(net)") != string::npos)	continue;
			string name = line.substr(0, line.find("/"));
			double intime = TransStringToDouble(line.substr(line.find("&") + 1));
			getline(file, line);
			double outtime = TransStringToDouble(line.substr(line.find("&") + 1));
			gptr = Circuit[0].GetGate(name);
			p->AddGate(gptr, intime, outtime);
		} while (getline(file, line));

		p->AddGate(epptr, TransStringToDouble(line.substr(line.find("&") + 1)), -1);	//arrival time

		getline(file, line);
		while (line.find("edge)") == string::npos)	getline(file, line);	//��clock [clock source] (rise/fall edge)
		if (period < 1)
			period = TransStringToDouble(line.substr(line.find("edge)") + 5));		

		//�blong path�� ���I���ɶ����O�[�J�F�@��period�����p �n��h �]���᭱�|��ΧO��Tc'
		if (epptr->GetType() == "PO"){
			while (line.find("output external delay") == string::npos)	getline(file, line);
			double delay = TransStringToDouble(line.substr(line.find("-") + 1));
			p->SetCTE(0.0);	
			p->SetST(delay);	//PO��setup time���~��delay
		}
		else{
			while (line.find("clock source latency") == string::npos) getline(file, line);
			//if (epptr->Clock_Length() == 0){
				while (getline(file, line)){
					line = RemoveSpace(line);
					if (ep == line.substr(0, line.find("/"))){
						double cte = TransStringToDouble(line.substr(line.find("&") + 1));
						p->SetCTE(cte - period);		//�o�䤴�O��+1��period
						break;
					}
					if (line.find("(net)") != string::npos)	continue;
					else if (line.find("(in)") != string::npos){
						epptr->SetClockPath(Circuit[0].GetGate("ClockSource"));
					}
					else{
						string name = line.substr(0, line.find("/"));
						double intime = TransStringToDouble(line.substr(line.find("&") + 1));
						getline(file, line);
						double outtime = TransStringToDouble(line.substr(line.find("&") + 1));
						gptr = Circuit[0].GetGate(name);
						epptr->SetClockPath(gptr);
						gptr->SetInTime(intime - period);		//�ɮפ���source -> ff���ɶ� + 1��period �ݮ��h
						gptr->SetOutTime(outtime - period);
					}
				}
			//}
			while (line.find("setup") == string::npos)	getline(file, line);
			double setup = TransStringToDouble(line.substr(line.find("-") + 1));			
			p->SetST(setup);
		}
		spptr->Setflag();
		epptr->Setflag();
		p->SetType(LONG);
		p->SetNo(Path_No++);
		if (p->length()>2)		//������gate ���O�����s
			PathR.push_back(*p);
		sp = "";		
	}	
	file.close();
}
/*
void ReadPath_s(string filename){
	fstream file;
	file.open(filename.c_str(), ios::in);
	string line, sp, ep;
	GATE *gptr = NULL, *spptr = NULL, *epptr = NULL;
	PATH* p = NULL;	
	while (getline(file, line)){
		if (line.find("Startpoint") != string::npos){
			if (PathR.size() >= 2*MAXPATHS)
				return;
			p = new PATH();
			sp = line.substr(line.find("Startpoint") + 12);
			sp = sp.substr(0, sp.find(" "));
			spptr = Circuit[0].GetGate(sp);	//0��top-module
			if (spptr == NULL)	//�_�I��PI
				spptr = new GATE(sp, "PI");
		}
		else if (line.find("Endpoint") != string::npos){	//���|��output��PO(�S��holdtime���D)
			ep = line.substr(line.find("Endpoint") + 10);
			ep = ep.substr(0, sp.find(" "));
			epptr = Circuit[0].GetGate(ep);
			if (epptr == NULL)
				epptr = new GATE(ep, "PO");			
		}

		if (line.find("---") == string::npos || sp == "")	continue;
		getline(file, line);
		getline(file, line);	
		if (spptr->GetType() != "PI"){			
			while (getline(file, line)){	//clock-source -> startpoint			
				line = RemoveSpace(line);
				if (sp == line.substr(0, line.find("/")))	break;
				if (line.find("(net)") != string::npos)	continue;
				else if (line.find("(in)") != string::npos){	//clock source�ɶ����p,�p�G���~������᭱���R�A�[�J
					spptr->SetClockPath(Circuit[0].GetGate("ClockSource"));
				}
				//else if (line.find("(out)") != string::npos){}
				else{
					string name = line.substr(0, line.find("/"));
					double intime = TransStringToDouble(line.substr(line.find("&") + 1));
					getline(file, line);
					double outtime = TransStringToDouble(line.substr(line.find("&") + 1));
					gptr = Circuit[0].GetGate(name);
					spptr->SetClockPath(gptr);
					gptr->SetInTime(intime);
					gptr->SetOutTime(outtime);
				}
			}
		}
		if (spptr->GetType() == "PI"){		//�_�I��PI�����p
			while (line.find("(in)") == string::npos)	getline(file, line);		
			p->AddGate(spptr, 0, TransStringToDouble(line.substr(line.find("&") + 1)));		
			getline(file, line);
		}
		do{			
			line = RemoveSpace(line);
			if (ep == line.substr(0, line.find("/")) || line.find("(out)") != string::npos)	break;
			if (line.find("(net)") != string::npos)	continue;
			string name = line.substr(0, line.find("/"));
			double intime = TransStringToDouble(line.substr(line.find("&") + 1));
			getline(file, line);
			double outtime = TransStringToDouble(line.substr(line.find("&") + 1));
			gptr = Circuit[0].GetGate(name);
			p->AddGate(gptr, intime, outtime);			
		} while (getline(file, line));

		p->AddGate(epptr, TransStringToDouble(line.substr(line.find("&") + 1)), -1);	//arrival time


		if (epptr->GetType() == "PO"){	//short���������|��output��PO
			while (line.find("output external delay") == string::npos)	getline(file, line);
			double delay = TransStringToDouble(line.substr(line.find("-") + 1));
			p->SetCTE(0.0);
			p->SetHT(delay);
		}
		else{
			while (line.find("clock source latency") == string::npos) getline(file, line);			
			while (getline(file, line)){
				line = RemoveSpace(line);
				if (ep == line.substr(0, line.find("/"))){
					double cte = TransStringToDouble(line.substr(line.find("&") + 1));
					p->SetCTE(cte);
					break;
				}
				if (line.find("(net)") != string::npos)	continue;
				else if (line.find("(in)") != string::npos){
					epptr->SetClockPath(Circuit[0].GetGate("ClockSource"));
				}
				else{
					string name = line.substr(0, line.find("/"));
					double intime = TransStringToDouble(line.substr(line.find("&") + 1));
					getline(file, line);
					double outtime = TransStringToDouble(line.substr(line.find("&") + 1));
					gptr = Circuit[0].GetGate(name);
					epptr->SetClockPath(gptr);
					gptr->SetInTime(intime);
					gptr->SetOutTime(outtime);
				}
			}			
			while (line.find("hold") == string::npos)	getline(file, line);
			double hold = TransStringToDouble(line.substr(line.find("time") + 5));
			p->SetHT(hold);
		}
		spptr->Setflag();
		epptr->Setflag();
		p->SetType(SHORT);
		p->CalWeight();
		PathR.push_back(*p);
		sp = "";	
	}	
	file.close();
}
*/

void ReadCpInfo(string filename){
	fstream file;
	file.open(filename.c_str());
	map<unsigned, unsigned> mapping;	//��s��(�S���h��PI->PO & NO_GATE��) -> PathC���s��
	for (int i = 0; i < PathC.size(); i++){
		mapping[PathC[i]->No()] = i;
	}
	int im, jn;
	double a, b, cc, err;
	string line;
	getline(file, line);
	while (file >> im >> jn >> a >> b){			// aging(j) = aging(i)*EdgeA[i][j] + EdgeB[i][j] **�ݥ[�W�~�t
		file >> line;							//�����Y�Ʒ|��nan
		if (line == "nan")
			cc = 0;
		else
			cc = atof(line.c_str());
		file >> err;
		if (mapping.find(im) == mapping.end() || mapping.find(jn) == mapping.end())
			continue;
		int ii = mapping[im],jj = mapping[jn];
		EdgeA[ii][jj] = a;
		EdgeB[ii][jj] = b;
		cor[ii][jj] = cc;
		ser[ii][jj] = err;
	}
	file.close();
}

void CalPreInv(double x,double &upper, double &lower, int a, int b){		//�p��w���϶�
	double dis = 1.96;	//+-�h�֭ӼзǮt 90% 1.65 95% 1.96 99% 2.58
	double y1 = EdgeA[a][b] * x + EdgeB[a][b];
	upper = y1 + ser[a][b] * dis;
	lower = y1 - ser[a][b] * dis;
}

bool Vio_Check(PATH* pptr, int stn, int edn, AGINGTYPE ast, AGINGTYPE aed, int year){
	GATE* stptr = pptr->Gate(0);
	GATE* edptr = pptr->Gate(pptr->length() - 1);
	double clks = 0.0;
	if (stptr->GetType() != "PI"){
		clks = pptr->GetCTH();
		double smallest = stptr->GetClockPath(1)->GetOutTime() - stptr->GetClockPath(1)->GetInTime();
		for (int i = 2; i < stptr->Clock_Length(); i++)
		if (stptr->GetClockPath(i)->GetOutTime() - stptr->GetClockPath(i)->GetInTime() < smallest)
			smallest = stptr->GetClockPath(i)->GetOutTime() - stptr->GetClockPath(i)->GetInTime();
		for (int i = 0; i < stn; i++)
			clks += (stptr->GetClockPath(i)->GetOutTime() - stptr->GetClockPath(i)->GetInTime())*AgingRate(DCC_NONE, year);
		for (int i = stn; i < stptr->Clock_Length(); i++)
			clks += (stptr->GetClockPath(i)->GetOutTime() - stptr->GetClockPath(i)->GetInTime())*AgingRate(ast, year);
		switch (ast){
		case DCC_S:
		case DCC_M:
			clks += smallest*(AgingRate(DCC_NONE, year) + 1)*1.33;
			break;
		case DCC_F:
			clks += smallest*(AgingRate(DCC_NONE, year) + 1)*1.67;
			break;
		default:
			break;
		}
	}
	double clkt = 0.0;
	if (edptr->GetType() != "PO"){
		clkt = pptr->GetCTE();
		double smallest = edptr->GetClockPath(1)->GetOutTime() - edptr->GetClockPath(1)->GetInTime();
		for (int i = 2; i < edptr->Clock_Length(); i++)
		if (edptr->GetClockPath(i)->GetOutTime() - edptr->GetClockPath(i)->GetInTime() < smallest)
			smallest = edptr->GetClockPath(i)->GetOutTime() - edptr->GetClockPath(i)->GetInTime();
		for (int i = 0; i < edn; i++)
			clkt += (edptr->GetClockPath(i)->GetOutTime() - edptr->GetClockPath(i)->GetInTime())*AgingRate(DCC_NONE, year);
		for (int i = edn; i < edptr->Clock_Length(); i++)
			clkt += (edptr->GetClockPath(i)->GetOutTime() - edptr->GetClockPath(i)->GetInTime())*AgingRate(aed, year);
		switch (aed){
		case DCC_S:
		case DCC_M:
			clkt += smallest*(AgingRate(DCC_NONE, year) + 1)*1.33;
			break;
		case DCC_F:
			clkt += smallest*(AgingRate(DCC_NONE, year) + 1)*1.67;
			break;
		default:
			break;
		}
	}
	double Tcq = 0.0;
	if (stptr->GetType() != "PI")
		Tcq = (pptr->Out_time(0) - pptr->In_time(0))*(AgingRate(FF, year) + 1);
	double DelayP = pptr->In_time(pptr->length() - 1) - pptr->Out_time(0);
	//for (int i = 1; i < pptr->length() - 1; i++)		//�e�᪺ff/PO/PI���κ�
	//	DelayP += (pptr->Out_time(i) - pptr->In_time(i))*AgingRate(NORMAL,year);
	DelayP += DelayP*AgingRate(NORMAL, year);
	if (pptr->GetType() == LONG){
		if (clks + Tcq + DelayP < clkt - pptr->GetST() + period)
			return true;
		return false;
	}
	else{
		if (clks + Tcq + DelayP>clkt + pptr->GetHT())
			return true;
		return false;
	}
}

bool Vio_Check(PATH* pptr, double year, double Aging_P){
	GATE* stptr = pptr->Gate(0);
	GATE* edptr = pptr->Gate(pptr->length() - 1);
	int ls = stptr->Clock_Length();
	int le = edptr->Clock_Length();
	double clks = 0.0;
	if (stptr->GetType() != "PI"){
		clks = pptr->GetCTH();
		double smallest = stptr->GetClockPath(1)->GetOutTime() - stptr->GetClockPath(1)->GetInTime();
		for (int i = 2; i < stptr->Clock_Length(); i++)
		if (stptr->GetClockPath(i)->GetOutTime() - stptr->GetClockPath(i)->GetInTime() < smallest)
			smallest = stptr->GetClockPath(i)->GetOutTime() - stptr->GetClockPath(i)->GetInTime();
		AGINGTYPE DCC_insert = DCC_NONE;
		int i;
		for (i = 0; i < ls && stptr->GetClockPath(i)->GetDcc() == DCC_NONE; i++)
			clks += (stptr->GetClockPath(i)->GetOutTime() - stptr->GetClockPath(i)->GetInTime())*AgingRate(DCC_NONE, year);
		if (i < ls)
			DCC_insert = stptr->GetClockPath(i)->GetDcc();
		for (; i < ls; i++)
			clks += (stptr->GetClockPath(i)->GetOutTime() - stptr->GetClockPath(i)->GetInTime())*AgingRate(DCC_insert, year);
		switch (DCC_insert){
		case DCC_S:
		case DCC_M:
			clks += smallest*(AgingRate(DCC_NONE, year) + 1)*1.33;
			break;
		case DCC_F:
			clks += smallest*(AgingRate(DCC_NONE, year) + 1)*1.67;
			break;
		default:
			break;
		}
	}
	double clkt = 0.0;
	if (edptr->GetType() != "PO"){
		clkt = pptr->GetCTE();
		double smallest = edptr->GetClockPath(1)->GetOutTime() - edptr->GetClockPath(1)->GetInTime();	//���tclock-source
		for (int i = 2; i < edptr->Clock_Length(); i++)
		if (edptr->GetClockPath(i)->GetOutTime() - edptr->GetClockPath(i)->GetInTime() < smallest)
			smallest = edptr->GetClockPath(i)->GetOutTime() - edptr->GetClockPath(i)->GetInTime();
		int i;
		AGINGTYPE DCC_insert = DCC_NONE;
		for (i = 0; i < le && edptr->GetClockPath(i)->GetDcc() == DCC_NONE; i++)
			clkt += (edptr->GetClockPath(i)->GetOutTime() - edptr->GetClockPath(i)->GetInTime())*AgingRate(DCC_NONE, year);
		if (i < le)
			DCC_insert = edptr->GetClockPath(i)->GetDcc();
		for (; i < le; i++)
			clkt += (edptr->GetClockPath(i)->GetOutTime() - edptr->GetClockPath(i)->GetInTime())*AgingRate(DCC_insert, year);
		switch (DCC_insert){
		case DCC_S:
		case DCC_M:
			clkt += smallest*(AgingRate(DCC_NONE, year) + 1)*1.33;
			break;
		case DCC_F:
			clkt += smallest*(AgingRate(DCC_NONE, year) + 1)*1.67;
			break;
		default:
			break;
		}
	}
	double Tcq = 0.0;
	if (stptr->GetType() != "PI")
		Tcq = (pptr->Out_time(0) - pptr->In_time(0))*(AgingRate(FF, year) + 1);
	double DelayP = pptr->In_time(pptr->length() - 1) - pptr->Out_time(0);
	DelayP += DelayP*Aging_P;
	if (pptr->GetType() == LONG){
		if (clks + Tcq + DelayP < clkt - pptr->GetST() + period)
			return true;
		return false;
	}
	else{
		if (clks + Tcq + DelayP>clkt + pptr->GetHT())
			return true;
		return false;
	}
}

double thershold = 0.9;	//R����
double t_slope = 0.95;

bool Check_Connect(int a, int b){	
	if (cor[a][b]<0)	//�t����
		return false;
	if (EdgeA[a][b] > 1)
		return Check_Connect(b, a);	
	if ((cor[a][b]*cor[a][b])<thershold)		//�����Y�ƭn�W�Lthershold�~��������
		return false;
	if (EdgeA[a][b] < t_slope)	//�ײv�b�d��~ => �n�[�J�w���϶� or �άO�u�ݰ����������Y��
		return false;
	return true;
}


inline double absl(double x){
	if (x < 0)
		return -x;
	return x;
}

void EstimateTimeEV(double year){
	int No_node = PathC.size();
	for (int i = 0; i < No_node; i++){
		cout << "NODE " << i + 1 << '/' << No_node << endl;
		PATH* pptr = PathC[i];
		GATE* stptr = pptr->Gate(0);
		GATE* edptr = pptr->Gate(pptr->length() - 1);
		double max = 0;
		double max2 = 0;
		for (int k = 0; k < No_node;k++){			
			double pv = 0;		//	�����&�� (path i��, ��path k���o)
			double pv2 = 0;		//	���зǮt -> sigma(xi - avg)^2 /N  �ε��w�ɶ��N��avg
			int solc = 0;
			if (stptr->GetType() == "PI"){
				for (int j = 0; j < edptr->Clock_Length(); j++){
					for (int x = 1; x < 4; x++){
						edptr->GetClockPath(j)->SetDcc((AGINGTYPE)x);
						if (!Vio_Check(pptr, year+ERROR, AgingRate(WORST, year + ERROR)) && Vio_Check(pptr, year-ERROR, AgingRate(WORST, year - ERROR))){
							double st = year - ERROR, ed = year + ERROR, mid;
							while (ed - st>0.0001){
								mid = (st + ed) / 2;
								double Aging_P = AgingRate(WORST, mid)*EdgeA[k][i] + EdgeB[k][i];	//�q��k��path�I�����i��path ���i��path������� �����[�~�t����
								if (EdgeA[k][i]>1)
									Aging_P = AgingRate(WORST, mid);
								if (Vio_Check(pptr, mid, Aging_P))
									st = mid;
								else
									ed = mid;
							}
							pv += mid;
							pv2 += (mid - year)*(mid - year);
							solc++;
						}
						edptr->GetClockPath(j)->SetDcc(DCC_NONE);
					}
				}
			}
			else if (edptr->GetType() == "PO"){
				for (int j = 0; j < stptr->Clock_Length(); j++){
					for (int x = 1; x < 4; x++){
						stptr->GetClockPath(j)->SetDcc((AGINGTYPE)x);
						if (!Vio_Check(pptr, year+ERROR, AgingRate(WORST, year + ERROR)) && Vio_Check(pptr, year-ERROR, AgingRate(WORST, year - ERROR))){
							double st = year - ERROR, ed = year + ERROR, mid;
							while (ed - st>0.0001){
								mid = (st + ed) / 2;
								double Aging_P = AgingRate(WORST, mid)*EdgeA[k][i] + EdgeB[k][i];	//
								if (EdgeA[k][i]>1)
									Aging_P = AgingRate(WORST, mid);
								if (Vio_Check(pptr, mid, Aging_P))
									st = mid;
								else
									ed = mid;
							}
							pv += mid;
							pv2 += (mid - year)*(mid - year);
							solc++;
						}
						stptr->GetClockPath(j)->SetDcc(DCC_NONE);
					}
				}
			}
			else{
				int branch = 0;
				while (stptr->GetClockPath(branch) == edptr->GetClockPath(branch)){
					for (int x = 1; x < 4; x++){
						stptr->GetClockPath(branch)->SetDcc((AGINGTYPE)x);
						if (!Vio_Check(pptr, year + ERROR, AgingRate(WORST, year + ERROR)) && Vio_Check(pptr, year - ERROR, AgingRate(WORST, year - ERROR))){
							double st = year - ERROR, ed = year + ERROR, mid;
							while (ed - st>0.0001){
								mid = (st + ed) / 2;
								double Aging_P = AgingRate(WORST, mid)*EdgeA[k][i] + EdgeB[k][i];	//
								if (EdgeA[k][i]>1)
									Aging_P = AgingRate(WORST, mid);
								if (Vio_Check(pptr, mid, Aging_P))
									st = mid;
								else
									ed = mid;
							}
							pv += mid;
							pv2 += (mid - year)*(mid - year);
							solc++;
						}
						stptr->GetClockPath(branch)->SetDcc(DCC_NONE);
					}
					branch++;
				}
				for (int j = branch; j < stptr->Clock_Length(); j++){
					for (int j2 = branch; j2 < edptr->Clock_Length(); j2++){
						for (int x = 0; x < 4; x++){
							for (int y = 0; y<4; y++){
								if (x == 0 && y == 0)	continue;
								edptr->GetClockPath(j2)->SetDcc((AGINGTYPE)y);
								stptr->GetClockPath(j)->SetDcc((AGINGTYPE)x);
								if (!Vio_Check(pptr, year+ERROR, AgingRate(WORST, year + ERROR)) && Vio_Check(pptr, year-ERROR, AgingRate(WORST, year - ERROR))){
									double st = year - ERROR, ed = year + ERROR, mid;
									while (ed - st>0.0001){
										mid = (st + ed) / 2;
										double Aging_P = AgingRate(WORST, mid)*EdgeA[k][i] + EdgeB[k][i];	//
										if (EdgeA[k][i]>1)
											Aging_P = AgingRate(WORST, mid);
										if (Vio_Check(pptr, mid, Aging_P))
											st = mid;
										else
											ed = mid;
									}
									pv += mid;
									pv2 += (mid - year)*(mid - year);
									solc++;
								}
								stptr->GetClockPath(j)->SetDcc(DCC_NONE);
								edptr->GetClockPath(j2)->SetDcc(DCC_NONE);
							}
						}
					}
				}
			}
			pv /= (double)solc;
			pv2 /= (double)solc;
			if (absl(pv - (double)year) > absl(max-(double)year))
				max = pv;
			if (pv2 > max2)
				max2 = pv2;
		}
		pptr->SetEstimateTime(max);
		pptr->SetPSD(max2);
	}
}

double EstimateAddTimes(double year,int p){	//p�O�n�[�J���I
	double max = 0;
	for (int i = 0; i < PathC.size(); i++){
		if (PathC[i]->Is_Chosen() && absl(year - PathC[i]->GetEstimateTime())>max)
			max = absl(year - PathC[i]->GetEstimateTime());
	}
	if (absl(year - PathC[p]->GetEstimateTime()) > max)
		return absl(year - PathC[p]->GetEstimateTime()) - max;		//�Ǧ^���O�[�Jp��|�t�h�� => �p�G��쥻���٤p�N�O0 ���M�N�^�Ǯt��
	return 0.0;
}

double EstimatePSD(int p){
	double max = 0;
	double newmax = PathC[p]->GetPSD();
	for (int i = 0; i < PathC.size(); i++){
		if (PathC[i]->Is_Chosen() && PathC[i]->GetPSD()>max){		//�P�� �Ǧ^���зǮt�[�J�e�᪺�̤j�Ȯt => ��쥻�p�Y0
			max = PathC[i]->GetPSD();
		}
	}
	if (max > newmax)
		return 0.0;
	
	return newmax - max;
}

struct PATHSOL{
	GATE *a,*b;
	AGINGTYPE ta, tb;
	PATHSOL(GATE* x,GATE* y,AGINGTYPE m,AGINGTYPE n) :a(x),b(y),ta(m),tb(n){}
};

bool CheckSolConflict(PATHSOL f, PATH* p){
	GATE* stptr = p->Gate(0);
	GATE* edptr = p->Gate(p->length()-1);
	if (f.a != NULL){
		if (f.a->GetDcc() !=DCC_NONE && f.ta!=f.a->GetDcc())
			return false;
		f.a->SetDcc(f.ta);
	}
	if (f.b != NULL){
		if (f.b->GetDcc() != DCC_NONE && f.tb != f.b->GetDcc())
			return false;
		f.b->SetDcc(f.tb);
	}
	int c = 0;	//���W�q�L�����[DCC��gate��
	for (int i = 0; i < stptr->Clock_Length(); i++){
		if (stptr->GetClockPath(i)->GetDcc() != DCC_NONE)
			c++;
	}
	if (c >= 2)
		return false;
	c = 0;
	for (int i = 0; i < edptr->Clock_Length(); i++){
		if (edptr->GetClockPath(i)->GetDcc() != DCC_NONE)
			c++;
	}
	if (c >= 2)
		return false;
	return true;
}

inline void DCCrestore(PATHSOL f){
	if (f.a)
		f.a->SetDcc(DCC_NONE);
	if (f.b)
		f.b->SetDcc(DCC_NONE);
}

double EstimateSolMines(int p){	//�p��Ҧ��{���I�b�[�J��֪��Ѷq���X�󥭧�
	double res = 1.0;
	int c = 0;
	for (int i = 0; i < PathC.size(); i++){
		if (PathC[i]->Is_Chosen()){
			res *= (double)conf[p][i] / ((double)conf[p][p] * (double)conf[i][i]);
			c++;
		}
	}
	if (c == 0 || res<1)
		return 0;
	res = pow(res, 1 / (double)c);	
	return res;
}

void CalSolMines(double year, int p){
	int No_node = PathC.size();		//���p����|p���ѨüȦs
	vector<PATHSOL> tempsol;
	tempsol.clear();
	PATH *pptr = PathC[p];
	GATE *stptr = pptr->Gate(0), *edptr = pptr->Gate(pptr->length()-1);

	if (stptr->GetType() == "PI"){
		for (int i = 0; i < edptr->Clock_Length(); i++){
			for (int x = 1; x < 4; x++){	//�u�Ҽ{���񪺪��p
				edptr->GetClockPath(i)->SetDcc((AGINGTYPE)x);
				if (!Vio_Check(pptr, year + ERROR, AgingRate(WORST, year + ERROR)) && Vio_Check(pptr, year - ERROR, AgingRate(WORST, year - ERROR))){
					PATHSOL pp(NULL, edptr->GetClockPath(i), DCC_NONE, (AGINGTYPE)x);
					tempsol.push_back(pp);
				}
				edptr->GetClockPath(i)->SetDcc(DCC_NONE);
			}
		}
	}
	else if (edptr->GetType() == "PO"){
		for (int i = 0; i < stptr->Clock_Length(); i++){
			for (int x = 1; x < 4; x++){
				stptr->GetClockPath(i)->SetDcc((AGINGTYPE)x);
				if (!Vio_Check(pptr, year + ERROR, AgingRate(WORST, year + ERROR)) && Vio_Check(pptr, year - ERROR, AgingRate(WORST, year - ERROR))){
					PATHSOL pp(stptr->GetClockPath(i), NULL, (AGINGTYPE)x, DCC_NONE);
					tempsol.push_back(pp);
				}
				stptr->GetClockPath(i)->SetDcc(DCC_NONE);
			}
		}
	}
	else{
		int branch = 0;
		while (stptr->GetClockPath(branch) == edptr->GetClockPath(branch)){
			for (int x = 1; x < 4; x++){
				stptr->GetClockPath(branch)->SetDcc((AGINGTYPE)x);
				if (!Vio_Check(pptr, year + ERROR, AgingRate(WORST, year + ERROR)) && Vio_Check(pptr, year - ERROR, AgingRate(WORST, year - ERROR))){
					PATHSOL pp(stptr->GetClockPath(branch), NULL, (AGINGTYPE)x, DCC_NONE);
					tempsol.push_back(pp);
				}
				stptr->GetClockPath(branch)->SetDcc(DCC_NONE);
			}
			branch++;
		}
		for (int i = branch; i < stptr->Clock_Length(); i++){
			for (int j = branch; j < edptr->Clock_Length(); j++){
				for (int x = 0; x < 4; x++){
					for (int y = 0; y < 4; y++){
						if (x == 0 && y == 0)	continue;												
						edptr->GetClockPath(j)->SetDcc((AGINGTYPE)y);
						stptr->GetClockPath(i)->SetDcc((AGINGTYPE)x);
						if (!Vio_Check(pptr, year + ERROR, AgingRate(WORST, year + ERROR)) && Vio_Check(pptr, year - ERROR, AgingRate(WORST, year - ERROR))){
							PATHSOL pp(stptr->GetClockPath(i), edptr->GetClockPath(j), (AGINGTYPE)x, (AGINGTYPE)y);
							tempsol.push_back(pp);
						}
						stptr->GetClockPath(i)->SetDcc(DCC_NONE);
						edptr->GetClockPath(j)->SetDcc(DCC_NONE);
					}
				}
			}
		}
	}	
	conf[p][p] = tempsol.size();	//path p���Ѽ� => conf[a][b]/conf[a][a]*conf[b][b] ���X�󥭧� => �ѤU�Ѫ��������
	for (int i = p+1; i < PathC.size(); i++)
		conf[p][i] = conf[i][p] = 0;

	for (int f = 0; f < tempsol.size(); f++){	
		for (int i = p + 1; i < No_node; i++){
			int sc = 0;
			pptr = PathC[i];
			stptr = pptr->Gate(0);
			edptr = pptr->Gate(pptr->length() - 1);			
			if (stptr->GetType() == "PI"){
				for (int k = 0; k < edptr->Clock_Length(); k++){
					for (int x = 1; x < 4; x++){
						edptr->GetClockPath(k)->SetDcc((AGINGTYPE)x);
						if (!Vio_Check(pptr, year + ERROR, AgingRate(WORST, year + ERROR)) && Vio_Check(pptr, year - ERROR, AgingRate(WORST, year - ERROR))){							
							if (!CheckSolConflict(tempsol[f], pptr)){
								conf[p][i]++;
								conf[i][p]++;
							}
							sc++;
						}
						DCCrestore(tempsol[f]);
						edptr->GetClockPath(k)->SetDcc(DCC_NONE);
					}
				}
			}
			else if (edptr->GetType() == "PO"){
				for (int k = 0; k < stptr->Clock_Length(); k++){
					for (int x = 1; x < 4; x++){
						stptr->GetClockPath(k)->SetDcc((AGINGTYPE)x);
						if (!Vio_Check(pptr, year + ERROR, AgingRate(WORST, year + ERROR)) && Vio_Check(pptr, year - ERROR, AgingRate(WORST, year - ERROR))){							
							if (!CheckSolConflict(tempsol[f], pptr)){
								conf[p][i]++;
								conf[i][p]++;
							}
							sc++;
						}
						DCCrestore(tempsol[f]);
						stptr->GetClockPath(k)->SetDcc(DCC_NONE);
					}
				}
			}
			else{
				int branch = 0;
				while (stptr->GetClockPath(branch) == edptr->GetClockPath(branch)){
					for (int x = 1; x < 4; x++){
						stptr->GetClockPath(branch)->SetDcc((AGINGTYPE)x);
						if (!Vio_Check(pptr, year + ERROR, AgingRate(WORST, year + ERROR)) && Vio_Check(pptr, year - ERROR, AgingRate(WORST, year - ERROR))){
							if (!CheckSolConflict(tempsol[f], pptr)){
								conf[p][i]++;
								conf[i][p]++;
							}
							sc++;
						}
						stptr->GetClockPath(branch)->SetDcc(DCC_NONE);
					}
					branch++;
				}
				for (int k = branch; k < stptr->Clock_Length(); k++){
					for (int j = branch; j < edptr->Clock_Length(); j++){
						for (int x = 0; x < 4; x++){
							for (int y = 0; y < 4; y++){
								if (x == 0 && y == 0)	continue;								
								edptr->GetClockPath(j)->SetDcc((AGINGTYPE)y);
								stptr->GetClockPath(k)->SetDcc((AGINGTYPE)x);
								if (!Vio_Check(pptr, year + ERROR, AgingRate(WORST, year + ERROR)) && Vio_Check(pptr, year - ERROR, AgingRate(WORST, year - ERROR))){									
									if (!CheckSolConflict(tempsol[f], pptr)){
										conf[p][i]++;
										conf[i][p]++;
									}
									sc++;
								}
								DCCrestore(tempsol[f]);
								stptr->GetClockPath(k)->SetDcc(DCC_NONE);
								edptr->GetClockPath(j)->SetDcc(DCC_NONE);
							}
						}
					}
				}
			}		
		}
	}		
}

struct PN_W{
	int pn;
	double w;
	PN_W(int n, double ww) :pn(n), w(ww){}
};

bool PN_W_comp(PN_W a, PN_W b){
	if (a.w > b.w)
		return true;
	return false;
}

class HASHTABLE{
private:
	bool* exist;
	bool** choose;
	unsigned size;
public:
	HASHTABLE(unsigned s1,unsigned s2){	//s1�O�줸��
		size = s1;
		exist = new bool[1 << s1];
		choose = new bool*[1 << s1];
		for (int i = 0; i < (1<<s1); i++){
			exist[i] = false;
			choose[i] = new bool[s2];
		}
	}
	unsigned CalKey(){
		unsigned key = 0x0;
		unsigned temp = 0x0;
		for (int i = 0; i < PathC.size(); i++){
			temp <<= 1;
			if (PathC[i]->Is_Chosen())
				temp++;
			if ((i+1)%size == 0){
				key ^= temp;
				temp = 0x0;
			}			
		}
		key ^= temp;
		return key;
	}
	bool Exist(){
		unsigned key = CalKey();		
		if (!exist[key])
			return false;
		for (int i = 0; i < PathC.size(); i++){
			if (PathC[i]->Is_Chosen() != choose[key][i])
				return false;
		}
		return true;
	}
	void PutNowStatus(){
		unsigned key = CalKey();
		exist[key] = true;
		for (int i = 0; i < PathC.size(); i++)
			choose[key][i] = PathC[i]->Is_Chosen();
	}
};


bool ChooseVertexWithGreedyMDS(int year, double pre_rvalueb){	
	int No_node = PathC.size();
	static bool refresh = true;
	static int *degree = new int[No_node], *color = new int[No_node];
	static bool *nochoose = new bool[No_node];
	static HASHTABLE hashp(16, PathC.size());
	if (pre_rvalueb < 0){									//<0�N��W�����L��,�Ȱ��[�Jhash
		refresh = true;
		hashp.PutNowStatus();
		return false;
	}
	if (refresh){		
		for (int i = 0; i < No_node; i++){			
			PathC[i]->SetChoose(false);			
			degree[i] = 0;			
			color[i] = 1;			
			nochoose[i] = false;			
			for (int j = 0; j < No_node; j++){				//�`�N�i�঳�bCPInfo���o�����B���o����
				if (Check_Connect(i, j))				
					degree[i]++;
			}			
		}
		refresh = false;
	}
	
	int mini;
	//double min;
	int w_point = 0;
	for (int i = 0; i < No_node; i++)
		if (color[i] == 1)
			w_point++;
	vector<PN_W> cand;
	for (int i = 0; i < No_node; i++){
		if (color[i] == -1)	//�ª�����
			continue;
		if (nochoose[i])
			continue;
		PathC[i]->SetChoose(true);
		if (hashp.Exist()){
			PathC[i]->SetChoose(false);
			nochoose[i] = true;
			continue;
		}
		PathC[i]->SetChoose(false);
		double w = 0;		//���榹�ȯ����X�M���w�Ȥ��t
		w += 3*EstimateAddTimes(year, i);	//�[�Ji�I��W�[���Ѯt��		
		//w -= EstimateSolMines(i);	//�[�Ji�I��ѤU���Ѥ�Ҥ��X�󥭧�
		w += EstimatePSD(i);		//�[�Ji�I��W�[��"���зǮt"
		w -= (double)degree[i] / (double)w_point;	//�[�Ji�I��i��֪����I����
		cand.push_back(PN_W(i, w));
	}
	if (cand.size() == 0){
		refresh = true;
		hashp.PutNowStatus();
		return false;	//false�N��o�����I���n��sat
	}
	sort(cand.begin(), cand.end(), PN_W_comp);
	int ii = 0;
	while (ii < cand.size() - 1 && (rand() % 10) >= 9){	//10%�����|������t����
		ii++;
	}
	mini = cand[ii].pn;	
	for (int i = 0; i < No_node; i++){
		if (Check_Connect(mini, i) && color[i] == 1){
			for (int j = 0; j < No_node; j++){
				if (Check_Connect(i, j) && color[j] != -1)	//��->��,�����I��degree -1 (���I�w�]��degree = 0 ���L)
					degree[j]--;
			}
			color[i] = 0;	//�Q���I���j���אּ��
			if (color[mini] == 1)	//�Q���I�אּ��,���䪺degree -1
				degree[i]--;
		}
	}
	PathC[mini]->SetChoose(true);
	degree[mini] = 0;
	color[mini] = -1;	//�Q���I�אּ��
	return true;
}

map<GATE*, int> cbuffer_code;
map<int, GATE*> cbuffer_decode;

int HashAllClockBuffer(){
	cbuffer_code.clear();
	cbuffer_decode.clear();
	int k = 0;
	for (unsigned i = 0; i < PathR.size(); i++){
		PATH* pptr = &PathR[i];
		if (pptr->IsSafe())	continue;
		GATE* stptr = pptr->Gate(0);
		GATE* edptr = pptr->Gate(pptr->length() - 1);
		if (stptr->GetType() != "PI"){
			for (int j = 0; j < stptr->Clock_Length(); j++)
				if (cbuffer_code.find(stptr->GetClockPath(j)) == cbuffer_code.end()){
					cbuffer_code[stptr->GetClockPath(j)] = k;
					cbuffer_decode[k] = stptr->GetClockPath(j);
					k++;
			}
		}		
		if (edptr->GetType() != "PO"){
			for (int j = 0; j < edptr->Clock_Length(); j++)
			if (cbuffer_code.find(edptr->GetClockPath(j)) == cbuffer_code.end()){
				cbuffer_code[edptr->GetClockPath(j)] = k;
				cbuffer_decode[k] = edptr->GetClockPath(j);
				k++;
			}
		}
	}
	return k;
}


void CheckPathAttackbility(int year,double margin,bool flag){		
		period = 0.0;
		for (int i = 0; i < PathR.size(); i++){
			double pp = (1 + AgingRate(WORST, static_cast<double>(year + ERROR)))*(PathR[i].In_time(PathR[i].length() - 1) - PathR[i].Out_time(0)) + (1.0 + AgingRate(FF, static_cast<double>(year + ERROR)))*(PathR[i].Out_time(0) - PathR[i].In_time(0)) + (1.0 + AgingRate(DCC_NONE, static_cast<double>(year + ERROR)))*(PathR[i].GetCTH() - PathR[i].GetCTE()) + PathR[i].GetST();	//check�@�U
			pp *= margin;
			if (pp>period)
				period = pp;
		}
		if (flag)
			cout << "Period = " << period << endl;
	for (int i = 0; i < PathR.size(); i++){				
		PATH* pptr = &PathR[i];
		pptr->SetAttack(false);
		pptr->SetSafe(true);
		GATE* stptr = pptr->Gate(0);
		GATE* edptr = pptr->Gate(pptr->length() - 1);		
		int lst = stptr->Clock_Length();
		int led = edptr->Clock_Length();
		int branch = 1;		
		if (stptr->GetType() == "PI"){
			for (int j = 1; j < led; j++){
				for (int x = 1; x <= 3; x++){
					if (!Vio_Check(pptr, 0, j, DCC_NONE, (AGINGTYPE)x, year + ERROR))
						pptr->SetSafe(false);
					if (!Vio_Check(pptr, 0, j, DCC_NONE, (AGINGTYPE)x, year+ERROR) && Vio_Check(pptr, 0, j, DCC_NONE, (AGINGTYPE)x, year - ERROR))
						pptr->SetAttack(true);	
				}				
			}		
		}
		else if (edptr->GetType() == "PO"){
			for (int j = 1; j < lst; j++){
				for (int x = 1; x <= 3; x++){
					if (!Vio_Check(pptr, j, 0, (AGINGTYPE)x, DCC_NONE, year + ERROR))
						pptr->SetSafe(false);
					if (!Vio_Check(pptr, j, 0, (AGINGTYPE)x, DCC_NONE, year+ERROR) && Vio_Check(pptr, j, 0, (AGINGTYPE)x, DCC_NONE, year - ERROR))										
						pptr->SetAttack(true);	
				}				
			}	
		}
		while (branch < lst&&branch < led){			
			if (stptr->GetClockPath(branch) != edptr->GetClockPath(branch))
				break;
			for (int x = 1; x <= 3; x++){
				if (!Vio_Check(pptr, branch, branch, (AGINGTYPE)x, (AGINGTYPE)x, year + ERROR))
					pptr->SetSafe(false);
				if (!Vio_Check(pptr, branch, branch, (AGINGTYPE)x, (AGINGTYPE)x, year + ERROR) && Vio_Check(pptr, branch, branch, (AGINGTYPE)x, (AGINGTYPE)x, year - ERROR))
					pptr->SetAttack(true);				
			}
			branch++;
		}		
		for (int j = branch; j < lst; j++){
			for (int k = branch; k < led; k++){
				for (int x = 0; x < 3; x++){
					for (int y = 0; y < 3; y++){
						if (x == 0 && y == 0)	continue;
						if (!Vio_Check(pptr, j, k, (AGINGTYPE)x, (AGINGTYPE)y, year + ERROR))
							pptr->SetSafe(false);
						if (!Vio_Check(pptr, j, k, (AGINGTYPE)x, (AGINGTYPE)y, year+ERROR) && Vio_Check(pptr, j, k, (AGINGTYPE)x, (AGINGTYPE)y, year - ERROR))	
							pptr->SetAttack(true);
					}
				}
			}
		}
	}
	int aa, bb, cc, dd;
	aa = bb = cc = dd = 0;
	for (unsigned i = 0; i < PathR.size(); i++){
		PATH* pptr = &PathR[i];
		GATE* stptr = pptr->Gate(0);
		GATE* edptr = pptr->Gate(pptr->length() - 1);
		if (pptr->CheckAttack()){
			if (stptr->GetType() == "PI")
				aa++;
			else if (edptr->GetType() == "PO")
				bb++;
			else
				cc++;
			PathC.push_back(pptr);
		}
		else{
			if (pptr->IsSafe() == false)
				dd++;
		}
	}
	if (flag){
		for (int i = 0; i < PathC.size(); i++)
			cout << PathC[i]->Gate(0)->GetName() << " -> " << PathC[i]->Gate(PathC[i]->length() - 1)->GetName() << " Length = " << PathC[i]->length() << endl;
		cout << aa << ' ' << bb << ' ' << cc << ' ' << dd << endl;
	}
	return;
}

void CheckNoVio(double year){
	cout << "Checking Violation... ";
	for (int i = 0; i < PathR.size(); i++){
		if (!Vio_Check(&PathR[i], year, AgingRate(WORST, year))){
			cout << "Path" << i << " Violation!" << endl;
		}
	}
	cout << "No Violation!" << endl;
}

void GenerateSAT(string filename,int year){		
	fstream file;
	fstream temp;	
	file.open(filename.c_str(), ios::out);
	map<GATE*, bool> exclusive;
	HashAllClockBuffer();	//�C��clockbuffer���s�����bcbuffer_code�����������X*2+1,*2+2
	for (unsigned i = 0; i < PathR.size(); i++){
		PATH* pptr = &PathR[i];
		if (pptr->IsSafe())	continue;
		GATE* stptr = pptr->Gate(0);
		GATE* edptr = pptr->Gate(pptr->length()-1);
		int stn = 0, edn = 0;	//��m�I(����A�]�A�ۨ����|���v�T)
		int lst = stptr->Clock_Length();
		int led = edptr->Clock_Length();
		if (exclusive.find(stptr) == exclusive.end()){
			for (int j = 0; j < lst; j++){
				for (int k = j + 1; k < lst; k++){
					file << '-' << cbuffer_code[stptr->GetClockPath(j)] * 2 + 1 << ' ' << '-' << cbuffer_code[stptr->GetClockPath(k)] * 2 + 1 << " 0" << endl;
					file << '-' << cbuffer_code[stptr->GetClockPath(j)] * 2 + 2 << ' ' << '-' << cbuffer_code[stptr->GetClockPath(k)] * 2 + 1 << " 0" << endl;
					file << '-' << cbuffer_code[stptr->GetClockPath(j)] * 2 + 1 << ' ' << '-' << cbuffer_code[stptr->GetClockPath(k)] * 2 + 2 << " 0" << endl;
					file << '-' << cbuffer_code[stptr->GetClockPath(j)] * 2 + 2 << ' ' << '-' << cbuffer_code[stptr->GetClockPath(k)] * 2 + 2 << " 0" << endl;
				}
			}
			exclusive[stptr] = true;
		}
		if (exclusive.find(edptr) == exclusive.end()){
			for (int j = 0; j < led; j++){
				for (int k = j + 1; k < led; k++){
					file << '-' << cbuffer_code[edptr->GetClockPath(j)] * 2 + 1 << ' ' << '-' << cbuffer_code[edptr->GetClockPath(k)] * 2 + 1 << " 0" << endl;
					file << '-' << cbuffer_code[edptr->GetClockPath(j)] * 2 + 2 << ' ' << '-' << cbuffer_code[edptr->GetClockPath(k)] * 2 + 1 << " 0" << endl;
					file << '-' << cbuffer_code[edptr->GetClockPath(j)] * 2 + 1 << ' ' << '-' << cbuffer_code[edptr->GetClockPath(k)] * 2 + 2 << " 0" << endl;
					file << '-' << cbuffer_code[edptr->GetClockPath(j)] * 2 + 2 << ' ' << '-' << cbuffer_code[edptr->GetClockPath(k)] * 2 + 2 << " 0" << endl;
				}
			}
			exclusive[edptr] = true;
		}

		if (pptr->Is_Chosen() == false){		//�S���Q��쪺Path �[�J���i�L��������			
			if (stptr->GetType() == "PI"){
				for (edn = 0; edn < led; edn++){
					for (int x = 0; x <= 3; x++){
						if (!Vio_Check(pptr, 0, edn, DCC_NONE, (AGINGTYPE)x, year - ERROR)){
							if (x % 2 == 1)
								file << '-';
							file << cbuffer_code[edptr->GetClockPath(edn)] * 2 + 1 << ' ';
							if (x >= 2)
								file << '-';
							file << cbuffer_code[edptr->GetClockPath(edn)] * 2 + 2 << ' ';
							for (int j = 0; j < led; j++){
								if (j == edn)	continue;
								file << cbuffer_code[edptr->GetClockPath(j)] * 2 + 1 << ' ' << cbuffer_code[edptr->GetClockPath(j)] * 2 + 2 << ' ';
							}
							file << 0 << endl;
						}
					}
				}
			}
			else if (edptr->GetType() == "PO"){
				for (stn = 0; stn < lst; stn++){
					for (int x = 0; x <= 3; x++){
						if (!Vio_Check(pptr, stn, 0, (AGINGTYPE)x, DCC_NONE, year - ERROR)){
							if (x % 2 == 1)
								file << '-';
							file << cbuffer_code[stptr->GetClockPath(stn)] * 2 + 1 << ' ';
							if (x >= 2)
								file << '-';
							file << cbuffer_code[stptr->GetClockPath(stn)] * 2 + 2 << ' ';
							for (int j = 0; j < lst; j++){
								if (j == stn)	continue;
								file << cbuffer_code[stptr->GetClockPath(j)] * 2 + 1 << ' ' << cbuffer_code[stptr->GetClockPath(j)] * 2 + 2 << ' ';
							}
							file << 0 << endl;
						}
					}
				}
			}
			else{
				stn = edn = 0;
				while (stn < lst && edn < led){		//��b�@�P�ϤW
					if (stptr->GetClockPath(stn) != edptr->GetClockPath(edn))
						break;
					for (int x = 0; x <= 3; x++){	//0/00 NO DCC 1/01 slow aging DCC(20%) 2/10 fast aging DCC(80%)
						if (!Vio_Check(pptr, stn, edn, (AGINGTYPE)x, (AGINGTYPE)x, year - ERROR)){	//Vio_Check�p�G�S��violation�|�^��true
							if (x % 2 == 1)	//01 11 -> 10 00
								file << '-';
							file << cbuffer_code[stptr->GetClockPath(stn)] * 2 + 1 << ' ';
							if (x >= 2)	//10 11
								file << '-';
							file << cbuffer_code[stptr->GetClockPath(stn)] * 2 + 2 << ' ';
							for (int j = 0; j < stptr->Clock_Length(); j++){
								if (j == stn)	continue;
								file << cbuffer_code[stptr->GetClockPath(j)] * 2 + 1 << ' ' << cbuffer_code[stptr->GetClockPath(j)] * 2 + 2 << ' ';
							}
							for (int j = 0; j < edptr->Clock_Length(); j++){
								if (j == edn)	continue;
								file << cbuffer_code[edptr->GetClockPath(j)] * 2 + 1 << ' ' << cbuffer_code[edptr->GetClockPath(j)] * 2 + 2 << ' ';
							}
							file << 0 << endl;
						}
					}
					stn++;
					edn++;
				}
				int b_point = stn;
				for (; stn < lst; stn++){
					for (edn = b_point; edn < led; edn++){
						for (int x = 0; x <= 3; x++){
							for (int y = 0; y <= 3; y++){
								if (!Vio_Check(pptr, stn, edn, (AGINGTYPE)x, (AGINGTYPE)y, year - ERROR)){
									if (x % 2 == 1)	//01 11 -> 1[0] 0[0]
										file << '-';
									file << cbuffer_code[stptr->GetClockPath(stn)] * 2 + 1 << ' ';
									if (x >= 2)	//10 11
										file << '-';
									file << cbuffer_code[stptr->GetClockPath(stn)] * 2 + 2 << ' ';
									if (y % 2 == 1)
										file << '-';
									file << cbuffer_code[edptr->GetClockPath(edn)] * 2 + 1 << ' ';
									if (y >= 2)
										file << '-';
									file << cbuffer_code[edptr->GetClockPath(edn)] * 2 + 2 << ' ';
									for (int j = 0; j < stptr->Clock_Length(); j++){
										if (j == stn)	continue;
										file << cbuffer_code[stptr->GetClockPath(j)] * 2 + 1 << ' ' << cbuffer_code[stptr->GetClockPath(j)] * 2 + 2 << ' ';
									}
									for (int j = 0; j < edptr->Clock_Length(); j++){
										if (j == edn)	continue;
										file << cbuffer_code[edptr->GetClockPath(j)] * 2 + 1 << ' ' << cbuffer_code[edptr->GetClockPath(j)] * 2 + 2 << ' ';
									}
									file << 0 << endl;
								}
							}
						}
					}
				}
			}
		}		
		else{
			if (stptr->GetType() == "PI"){
				for (edn = 0; edn < led; edn++){
					for (int x = 0; x <= 3; x++){
						if (Vio_Check(pptr, 0, edn, DCC_NONE, (AGINGTYPE)x, year + ERROR) || !Vio_Check(pptr, 0, edn, DCC_NONE, (AGINGTYPE)x, year - ERROR)){
							if (x % 2 == 1)
								file << '-';
							file << cbuffer_code[edptr->GetClockPath(edn)] * 2 + 1 << ' ';
							if (x >= 2)
								file << '-';
							file << cbuffer_code[edptr->GetClockPath(edn)] * 2 + 2 << ' ';
							for (int j = 0; j < led; j++){
								if (j == edn)	continue;
								file << cbuffer_code[edptr->GetClockPath(j)] * 2 + 1 << ' ' << cbuffer_code[edptr->GetClockPath(j)] * 2 + 2 << ' ';
							}
							file << 0 << endl;
						}
					}
				}
			}
			else if (edptr->GetType() == "PO"){
				for (stn = 0; stn < lst; stn++){
					for (int x = 0; x <= 3; x++){
						if (Vio_Check(pptr, stn, 0, (AGINGTYPE)x, DCC_NONE, year + ERROR) || !Vio_Check(pptr, stn, 0, (AGINGTYPE)x, DCC_NONE, year - ERROR)){
							if (x % 2 == 1)
								file << '-';
							file << cbuffer_code[stptr->GetClockPath(stn)] * 2 + 1 << ' ';
							if (x >= 2)
								file << '-';
							file << cbuffer_code[stptr->GetClockPath(stn)] * 2 + 2 << ' ';
							for (int j = 0; j < lst; j++){
								if (j == stn)	continue;
								file << cbuffer_code[stptr->GetClockPath(j)] * 2 + 1 << ' ' << cbuffer_code[stptr->GetClockPath(j)] * 2 + 2 << ' ';
							}
							file << 0 << endl;
						}
					}
				}
			}
			else{
				stn = edn = 0;
				while (stn < lst && edn < led){		//��b�@�P�ϤW
					if (stptr->GetClockPath(stn) != edptr->GetClockPath(edn))
						break;
					for (int x = 0; x <= 3; x++){	//0/00 NO DCC 1/01 slow aging DCC(20%) 2/10 fast aging DCC(80%)
						if (Vio_Check(pptr, stn, edn, (AGINGTYPE)x, (AGINGTYPE)x, year + ERROR) || !Vio_Check(pptr, stn, edn, (AGINGTYPE)x, (AGINGTYPE)x, year - ERROR)){	//Vio_Check�p�G�S��violation�|�^��true
							if (x % 2 == 1)	//01 11 -> 10 00
								file << '-';
							file << cbuffer_code[stptr->GetClockPath(stn)] * 2 + 1 << ' ';
							if (x >= 2)	//10 11
								file << '-';
							file << cbuffer_code[stptr->GetClockPath(stn)] * 2 + 2 << ' ';
							for (int j = 0; j < stptr->Clock_Length(); j++){
								if (j == stn)	continue;
								file << cbuffer_code[stptr->GetClockPath(j)] * 2 + 1 << ' ' << cbuffer_code[stptr->GetClockPath(j)] * 2 + 2 << ' ';
							}
							for (int j = 0; j < edptr->Clock_Length(); j++){
								if (j == edn)	continue;
								file << cbuffer_code[edptr->GetClockPath(j)] * 2 + 1 << ' ' << cbuffer_code[edptr->GetClockPath(j)] * 2 + 2 << ' ';
							}
							file << 0 << endl;
						}
					}
					stn++;
					edn++;
				}
				int b_point = stn;
				for (; stn < lst; stn++){
					for (edn = b_point; edn < led; edn++){
						for (int x = 0; x <= 3; x++){
							for (int y = 0; y <= 3; y++){
								if (Vio_Check(pptr, stn, edn, (AGINGTYPE)x, (AGINGTYPE)y, year + ERROR) || !Vio_Check(pptr, stn, edn, (AGINGTYPE)x, (AGINGTYPE)y, year - ERROR)){
									if (x % 2 == 1)	//01 11 -> 1[0] 0[0]
										file << '-';
									file << cbuffer_code[stptr->GetClockPath(stn)] * 2 + 1 << ' ';
									if (x >= 2)	//10 11
										file << '-';
									file << cbuffer_code[stptr->GetClockPath(stn)] * 2 + 2 << ' ';
									if (y % 2 == 1)
										file << '-';
									file << cbuffer_code[edptr->GetClockPath(edn)] * 2 + 1 << ' ';
									if (y >= 2)
										file << '-';
									file << cbuffer_code[edptr->GetClockPath(edn)] * 2 + 2 << ' ';
									for (int j = 0; j < stptr->Clock_Length(); j++){
										if (j == stn)	continue;
										file << cbuffer_code[stptr->GetClockPath(j)] * 2 + 1 << ' ' << cbuffer_code[stptr->GetClockPath(j)] * 2 + 2 << ' ';
									}
									for (int j = 0; j < edptr->Clock_Length(); j++){
										if (j == edn)	continue;
										file << cbuffer_code[edptr->GetClockPath(j)] * 2 + 1 << ' ' << cbuffer_code[edptr->GetClockPath(j)] * 2 + 2 << ' ';
									}
									file << 0 << endl;
								}
							}
						}
					}
				}
			}
		}
	}
	file.close();
}

bool CallSatAndReadReport(){
	//cout << "Start Call SAT" << endl;
	for (int i = 0; i < PathC.size(); i++){
		GATE* stptr = PathC[i]->Gate(0);
		GATE* edptr = PathC[i]->Gate(PathC[i]->length() - 1);
		for (int i = 0; i < stptr->Clock_Length(); i++)
			stptr->GetClockPath(i)->SetDcc(DCC_NONE);
		for (int i = 0; i < edptr->Clock_Length(); i++)
			edptr->GetClockPath(i)->SetDcc(DCC_NONE);
	}
	system("minisat sat.cnf temp.sat");
	fstream file;
	file.open("temp.sat", ios::in);
	string line;
	getline(file, line);
	if (line.find("UNSAT")!=string::npos)
		return false;
	int n1,n2;
	while (file >> n1 >> n2){
		if (n1 < 0 && n2 < 0)
			cbuffer_decode[(-n1 - 1) / 2]->SetDcc(DCC_NONE);
		else if (n1>0 && n2 < 0)
			cbuffer_decode[(n1 - 1) / 2]->SetDcc(DCC_S);
		else if (n1<0 && n2>0)
			cbuffer_decode[(-n1 - 1) / 2]->SetDcc(DCC_F);
		else
			cbuffer_decode[(n1 - 1) / 2]->SetDcc(DCC_M);
	}
	file.close();
	int cdcc = 0;
	for (int i = 0; i < cbuffer_decode.size();i++)
		if (cbuffer_decode[i]->GetDcc() != DCC_NONE)
			cout << ++cdcc << " : " << cbuffer_decode[i]->GetName() << ' ' << cbuffer_decode[i]->GetDcc() << endl;
	return true;
}


double CalQuality(int year){
	cout << "Start CalQuality" << endl;
	double worst_all = (double)year;
	for (int i = 0; i < PathC.size(); i++){
		double e_upper = 10000, e_lower = 10000;
		for (int j = 0; j < PathC.size(); j++){			
			//�p��ɱq�����i�����I(���O�Ⱥ�Q���I)					
			double st = 1.0, ed = 10.0, mid;
			//cout << i << ' ' << j << endl;
			//cout << "y=" << EdgeA[i][j] << "x+" << EdgeB[i][j] << endl;
			while (ed - st > 0.0001){
				mid = (st + ed) / 2;
				double upper,lower;
				CalPreInv(AgingRate(WORST, mid), upper, lower, i, j);				//y = ax+b => ����lower bound/upper bound�h�D�̻���t�h��				
				double Aging_P;														
				if (upper > AgingRate(WORST, mid))													
					Aging_P = AgingRate(WORST, mid);
				else
					Aging_P = upper;
				if (Vio_Check(PathC[j], mid, Aging_P))
					st = mid;
				else
					ed = mid;
			}
			if (mid < e_upper)
				e_upper = mid;				//�̦����I(�]���o�Ϳ��~�̦��b����)

			while (ed - st > 0.0001){
				mid = (st + ed) / 2;
				double upper, lower;
				CalPreInv(AgingRate(WORST, mid), upper, lower, i, j);				
				double Aging_P;														//lower bound
				if (lower > AgingRate(WORST, mid))									//�i��<0 �ݹ��窬�p�ݬ�
					Aging_P = AgingRate(WORST, mid);
				else
					Aging_P = lower;
				if (Vio_Check(PathC[j], mid, Aging_P))
					st = mid;
				else
					ed = mid;
			}
			if (mid < e_lower)
				e_lower = mid;
		}
		if (absl((double)year - worst_all) < absl((double)year - e_upper))		//�����w�ɶ��̻������̮t�����p
			worst_all = e_upper;
		if (absl((double)year - worst_all) < absl((double)year - e_lower))
			worst_all = e_lower;
		
	}
	return worst_all;		//�令��bound�άOmontecarlo���覡
}
bool CheckImpact(PATH* pptr){
	GATE* gptr;
	gptr = pptr->Gate(0);
	if (gptr->GetType() != "PI"){
		for (int i = 0; i < gptr->Clock_Length();i++)
			if (gptr->GetClockPath(i)->GetDcc() != DCC_NONE)
				return true;
	}
	gptr = pptr->Gate(pptr->length() - 1);
	if (gptr->GetType() != "PO"){
		for (int i = 0; i < gptr->Clock_Length(); i++)
			if (gptr->GetClockPath(i)->GetDcc() != DCC_NONE)
				return true;
	}
	return false;
}
bool RefineResult(int year){		//�[�J�P�_�Q�v�T�I���ƶq & Refine����V
	double early = 10000.0;
	int earlyp = -1;
	int catk = 0, cimp = 0;
	for (int i = 0; i < PathR.size(); i++){
		PATH* pptr = &PathR[i];
		GATE* stptr = pptr->Gate(0);
		GATE* edptr = pptr->Gate(pptr->length() - 1);
		if (CheckImpact(pptr))
			cimp++;
		if (!Vio_Check(pptr, (double)year + ERROR, AgingRate(WORST, year + ERROR)))
			catk++;
		if (!Vio_Check(pptr, (double)year - ERROR, AgingRate(WORST, year - ERROR))){
			if (pptr->CheckAttack())
				cout << "*";	
			double st = 1.0, ed = 10, mid;
			while (ed - st>0.0001){
				mid = (st + ed) / 2;
				if (Vio_Check(&PathR[i], mid, AgingRate(WORST, mid)))
					st = mid;
				else
					ed = mid;
			}
			cout << i << " = " << mid << ' ';
			if (early > mid){
				early = mid;
				earlyp = i;
			}
		}
	}
	cout << endl << catk << " Paths Be Attacked." << endl;
	cout << cimp << " Paths Be Impacted." << endl;
	if (earlyp < 0)
		return false;
	//cout << earlyp << endl;
	fstream file;
	file.open("sat.cnf", ios::out | ios::app);
	if (!file)
		cout << "fail to open sat.cnf" << endl;
	PATH* pptr = &PathR[earlyp];
	if (pptr->Gate(0)->GetType() != "PI"){
		GATE* stptr = pptr->Gate(0);
		int ls = stptr->Clock_Length(), i;
		for (i = 0; i < ls && stptr->GetClockPath(i)->GetDcc() == DCC_NONE; i++);
		if (i < ls){
			switch (stptr->GetClockPath(i)->GetDcc()){
			case DCC_S:
				file << -(cbuffer_code[stptr->GetClockPath(i)] * 2 + 1) << ' ' << cbuffer_code[stptr->GetClockPath(i)] * 2 + 2 << ' ' << '0' << endl;
				break;
			case DCC_F:
				file << cbuffer_code[stptr->GetClockPath(i)] * 2 + 1 << ' ' << -(cbuffer_code[stptr->GetClockPath(i)] * 2 + 2) << ' ' << '0' << endl;
				break;
			case DCC_M:
				file << -(cbuffer_code[stptr->GetClockPath(i)] * 2 + 1) << ' ' << -(cbuffer_code[stptr->GetClockPath(i)] * 2 + 2) << ' ' << '0' << endl;
				break;
			default:
				break;
			}
		}
	}
	if (pptr->Gate(pptr->length()-1)->GetType() != "PO"){
		GATE* edptr = pptr->Gate(pptr->length() - 1);
		int le = edptr->Clock_Length(), i;
		for (i = 0; i < le && edptr->GetClockPath(i)->GetDcc() == DCC_NONE; i++);
		if (i < le){
			switch (edptr->GetClockPath(i)->GetDcc()){
			case DCC_S:
				file << -(cbuffer_code[edptr->GetClockPath(i)] * 2 + 1) << ' ' << cbuffer_code[edptr->GetClockPath(i)] * 2 + 2 << ' ' << '0' << endl;
				break;
			case DCC_F:
				file << cbuffer_code[edptr->GetClockPath(i)] * 2 + 1 << ' ' << -(cbuffer_code[edptr->GetClockPath(i)] * 2 + 2) << ' ' << '0' << endl;
				break;
			case DCC_M:
				file << -(cbuffer_code[edptr->GetClockPath(i)] * 2 + 1) << ' ' << -(cbuffer_code[edptr->GetClockPath(i)] * 2 + 2) << ' ' << '0' << endl;
				break;
			default:
				break;
			}
		}
	}
	file.close();
	return true;
}