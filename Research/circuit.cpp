#include<iostream>
#include<fstream>
#include<sstream>
#include<math.h>
#include "circuit.h"
#include<stdlib.h>
#define ERROR 1
#define MARGIN 1.0

using namespace std;

extern vector<CIRCUIT> Circuit;
extern vector<PATH> PathR;
extern vector<PATH*> PathC;
extern double **EdgeA;
extern double **EdgeB;
extern double **cor;
extern int **conf;
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
		if (s[i] != ' ' && s[i] != 9 && s[i] != 13)		//9為水平tab,13為換行
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
	while (file.getline(temp, 1000)){		//一行多個命令無法處理
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
					//可能再加入藉由之前讀入的module(非library)來建立gate,in/output的名稱不用記錄(在module內有),但要用以接到正確接口
					ok = true;
					break;
				}
			}
			if (!ok){	//假設第一個為output,其它為input
				string moduleN = temps.substr(0, temps.find(" "));	//module name,gate name,gate's output會寫在一行
				temps = temps.substr(temps.find(" ") + 1);
				string gateN = temps.substr(0, temps.find(" ("));
				temps = temps.substr(temps.find(" ") + 2);
				GATE* g = new GATE(gateN,moduleN);
				int st = temps.find("(");
				string ioN = temps.substr(st + 1, temps.find(")") - st - 1);
				g->SetOutput(Circuit[Nowmodule].GetWire(ioN));
				Circuit[Nowmodule].GetWire(ioN)->SetInput(g);
				temps = temps.substr(temps.find(")") + 1);
				while (file.getline(temp,1000)){		//gate's input 會在後面每行寫一個
					temps = temp;
					st = temps.find("(");
					string ioN = temps.substr(st + 1, temps.find(")") - st - 1);		//會有線接到1'b1,1'b0(常數)
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
	while (getline(file, line)){
		//cout << line << endl;
		if (line.find("Startpoint") != string::npos){
			if (PathR.size() >= MAXPATHS)
				return;
			p = new PATH();
			sp = line.substr(line.find("Startpoint") + 12);
			sp = sp.substr(0, sp.find(" "));
			spptr = Circuit[0].GetGate(sp);	//0為top-module
			if (spptr == NULL)	//起點為PI
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
			while (line.find("slack (MET)") == string::npos)	getline(file, line);
			continue;
		}
		getline(file, line);
		getline(file, line);		//這2行的會反應在第一個gate時間上 不用記
		//if (spptr->Clock_Length() == 0 && spptr->GetType()!="PI"){
		if (spptr->GetType() != "PI"){
			while (getline(file, line)){	//clock-source -> startpoint			
				line = RemoveSpace(line);
				if (sp == line.substr(0, line.find("/")))	break;
				if (line.find("(net)") != string::npos)	continue;
				else if (line.find("(in)") != string::npos){	//PI時間不計,如果有外部延遲後面分析再加入
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
		if (spptr->GetType() == "PI"){		//起點為PI的狀況
			while (line.find("(in)") == string::npos)	getline(file, line);
			p->AddGate(spptr, 0, TransStringToDouble(line.substr(line.find("&") + 1)));		//clock 到 起點的時間為0 (PI),tcq = 外部delay
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
		while (line.find("edge)") == string::npos)	getline(file, line);	//找clock [clock source] (rise/fall edge)
		if (period < 1)
			period = TransStringToDouble(line.substr(line.find("edge)") + 5));		

		//在long path中 終點的時間都是加入了一個period的狀況 要減去 因為後面會改用別的Tc'
		if (epptr->GetType() == "PO"){
			while (line.find("output external delay") == string::npos)	getline(file, line);
			double delay = TransStringToDouble(line.substr(line.find("-") + 1));
			p->SetCTE(0.0);	
			p->SetST(delay);	//PO的setup time為外部delay
		}
		else{
			while (line.find("clock source latency") == string::npos) getline(file, line);
			//if (epptr->Clock_Length() == 0){
				while (getline(file, line)){
					line = RemoveSpace(line);
					if (ep == line.substr(0, line.find("/"))){
						double cte = TransStringToDouble(line.substr(line.find("&") + 1));
						p->SetCTE(cte - period);		//這邊仍是有+1個period
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
						gptr->SetInTime(intime - period);		//檔案中為source -> ff的時間 + 1個period 需消去
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
		p->CalWeight();
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
			spptr = Circuit[0].GetGate(sp);	//0為top-module
			if (spptr == NULL)	//起點為PI
				spptr = new GATE(sp, "PI");
		}
		else if (line.find("Endpoint") != string::npos){	//不會有output為PO(沒有holdtime問題)
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
				else if (line.find("(in)") != string::npos){	//clock source時間不計,如果有外部延遲後面分析再加入
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
		if (spptr->GetType() == "PI"){		//起點為PI的狀況
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


		if (epptr->GetType() == "PO"){	//short部份應不會有output為PO
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
	//for (int i = 1; i < pptr->length() - 1; i++)		//前後的ff/PO/PI不用算
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
		double smallest = edptr->GetClockPath(1)->GetOutTime() - edptr->GetClockPath(1)->GetInTime();	//不含clock-source
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

void PATH::CalWeight(){
	GATE* stptr = gate_list[0];
	GATE* edptr = gate_list[gate_list.size() - 1];
	int same = 0;
	while (stptr->Clock_Length() > same && edptr->Clock_Length() > same){
		if (stptr->GetClockPath(same) != edptr->GetClockPath(same))
			break;
		same++;
	}
	weight = (stptr->Clock_Length()) + (edptr->Clock_Length()) - 2 * same;
}
double thershold = 0.8;
double t_slope = 0.95;

bool Check_Connect(int a, int b){
	if (EdgeA[a][b] > 1)
		return Check_Connect(b, a);
	if (cor[a][b]<thershold && cor[a][b]>-thershold)		//相關係數要超過thershold才視為有邊
		return false;
	if (EdgeA[a][b] < t_slope)	//斜率在範圍內
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
		for (int k = 0; k < No_node;k++){			
			double pv = 0;		//	期望值&解 (path i的, 由path k推得)
			int solc = 0;
			if (stptr->GetType() == "PI"){
				for (int j = 0; j < edptr->Clock_Length(); j++){
					for (int x = 1; x < 4; x++){
						edptr->GetClockPath(j)->SetDcc((AGINGTYPE)x);
						if (!Vio_Check(pptr, year+ERROR, AgingRate(WORST, year + ERROR)) && Vio_Check(pptr, year-ERROR, AgingRate(WORST, year - ERROR))){
							double st = year - ERROR, ed = year + ERROR, mid;
							while (ed - st>0.025){
								mid = (st + ed) / 2;
								double Aging_P = AgingRate(WORST, mid)*EdgeA[k][i] + EdgeB[k][i];	//要補上error,從第k個path點推到第i個path 找第i個path的期望值
								if (EdgeA[k][i]>1)
									Aging_P = AgingRate(WORST, mid);
								if (Vio_Check(pptr, mid, Aging_P))
									st = mid;
								else
									ed = mid;
							}
							pv += mid;
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
							while (ed - st>0.025){
								mid = (st + ed) / 2;
								double Aging_P = AgingRate(WORST, mid)*EdgeA[k][i] + EdgeB[k][i];	//要補上error
								if (EdgeA[k][i]>1)
									Aging_P = AgingRate(WORST, mid);
								if (Vio_Check(pptr, mid, Aging_P))
									st = mid;
								else
									ed = mid;
							}
							pv += mid;
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
							while (ed - st>0.025){
								mid = (st + ed) / 2;
								double Aging_P = AgingRate(WORST, mid)*EdgeA[k][i] + EdgeB[k][i];	//要補上error
								if (EdgeA[k][i]>1)
									Aging_P = AgingRate(WORST, mid);
								if (Vio_Check(pptr, mid, Aging_P))
									st = mid;
								else
									ed = mid;
							}
							pv += mid;
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
									while (ed - st>0.025){
										mid = (st + ed) / 2;
										double Aging_P = AgingRate(WORST, mid)*EdgeA[k][i] + EdgeB[k][i];	//要補上error
										if (EdgeA[k][i]>1)
											Aging_P = AgingRate(WORST, mid);
										if (Vio_Check(pptr, mid, Aging_P))
											st = mid;
										else
											ed = mid;
									}
									pv += mid;
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
			if (abs(pv - (double)year) > abs(max-(double)year))
				max = pv;
		}
		pptr->SetEstimateTime(max);
	}
}

double EstimateAddTimes(double year,int p){	//p是要加入的點
	double max = 0;
	for (int i = 0; i < PathC.size(); i++){
		if (PathC[i]->Is_Chosen() && absl(year - PathC[i]->GetEstimateTime())>max)
			max = abs(year - PathC[i]->GetEstimateTime());
	}
	if (absl(year - PathC[p]->GetEstimateTime()) > max)
		return absl(year - PathC[p]->GetEstimateTime()) - max;		//傳回的是加入p後會差多少 => 如果比原本的還小就是0 不然就回傳差值
	return 0.0;
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
	int c = 0;	//路上通過的有加DCC的gate數
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

double EstimateSolMines(int p){	//計算所有現有點在加入後少的解量之幾何平均
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
	int No_node = PathC.size();		//先計算路徑p的解並暫存
	vector<PATHSOL> tempsol;
	tempsol.clear();
	PATH *pptr = PathC[p];
	GATE *stptr = pptr->Gate(0), *edptr = pptr->Gate(pptr->length()-1);

	if (stptr->GetType() == "PI"){
		for (int i = 0; i < edptr->Clock_Length(); i++){
			for (int x = 1; x < 4; x++){	//只考慮有放的狀況
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
	conf[p][p] = tempsol.size();	//path p的解數 => conf[a][b]/conf[a][a]*conf[b][b] 之幾何平均 => 剩下解的平均比例
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

void ChooseVertexWithGreedyMDS(int year){
	int No_node = PathC.size();
	int* degree = new int[No_node];
	int degtot;
	for (int i = 0; i < No_node; i++){
		PathC[i]->SetChoose(false);
		degree[i] = 0;
		for (int j = 0; j < No_node; j++){
			if (Check_Connect(i, j))
				degree[i]++;
		}		
	}
	
	int* color = new int[No_node];
	for (int i = 0; i < No_node; i++)
		color[i] = 1;	//初始全白點
	bool chk = true;	
	
	while (chk){
		int maxi;
		double max;
		degtot = 0;
		for (int i = 0; i < No_node; i++)
			if (color[i] != -1)
				degtot += degree[i];
		max = -100;
		for (int i = 0; i < No_node; i++){
			if (color[i] == -1)	//黑的不選
				continue;
			double w = 0;
			w += EstimateAddTimes(year, i);
			w += 2*EstimateSolMines(i);
			if (degtot>0)
				w += (double)degree[i] / (double)degtot;
			if (w>max){
				max = w;
				maxi = i;
			}
		}		
		for (int i = 0; i < No_node; i++){
			if (Check_Connect(maxi, i)	&& color[i]==1){
				for (int j = 0; j < No_node; j++){
					if (Check_Connect(i, j) && color[j] != -1)	//白->灰,附近的點之degree -1 (黑點已設為degree = 0 跳過)
						degree[j]--;
				}
				color[i] = 0;	//被選點的隔壁改為灰
				if (color[maxi] == 1)	//被選點改為黑,旁邊的degree -1
					degree[i]--;
			}
		}		
		PathC[maxi]->SetChoose(true);
		degree[maxi] = 0;
		color[maxi] = -1;	//被選點改為黑
		chk = false;
		for (int i = 0; i < No_node; i++){
			if (color[i] == 1){
				chk = true;				
			}
		}		
	}
}

map<GATE*, int> cbuffer_code;
map<int, GATE*> cbuffer_decode;

int HashAllClockBuffer(){
	cbuffer_code.clear();
	cbuffer_decode.clear();
	int k = 0;
	for (unsigned i = 0; i < PathC.size(); i++){
		PATH* pptr = PathC[i];
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


void CheckPathAttackbility(int year){
		int aa, bb, cc, dd;
		aa = bb = cc = dd = 0;
		period = 0.0;
		for (int i = 0; i < PathR.size(); i++){
			double pp = (1.0 + AgingRate(NORMAL, year))*(PathR[i].In_time(PathR[i].length() - 1) - PathR[i].Out_time(0))+(1.0+AgingRate(FF,year))*(PathR[i].Out_time(0)-PathR[i].In_time(0))+ (1.0+AgingRate(DCC_NONE,year))*(PathR[i].GetCTH() - PathR[i].GetCTE());
			pp *= MARGIN;
			if (pp>period)
				period = pp;
		}
		cout << period << endl;
		//cin >> aa;
	for (int i = 0; i < PathR.size(); i++){		
		bool chk = false;
		PATH* pptr = &PathR[i];		
		GATE* stptr = pptr->Gate(0);
		GATE* edptr = pptr->Gate(pptr->length() - 1);		
		int lst = stptr->Clock_Length();
		int led = edptr->Clock_Length();
		int branch = 1;		
		if (stptr->GetType() == "PI"){
			for (int j = 1; j < led && !chk; j++){
				for (int x = 1; x <= 3; x++){
					if (!Vio_Check(pptr, 0, j, DCC_NONE, (AGINGTYPE)x, year+ERROR) && Vio_Check(pptr, 0, j, DCC_NONE, (AGINGTYPE)x, year - ERROR)){
						PathC.push_back(pptr);
						cout << "Start : " << stptr->GetName() << " End : " << edptr->GetName() << endl << "PI+FF" << endl;
						cout << "Put DCC On : " << stptr->GetClockPath(j)->GetName() << " NONE" << endl << "DCC TYPE : " << x << endl;
						aa++;
						chk = true;
						pptr->SetAttack(true);
						break;
					}
				}				
			}
		}
		else if (edptr->GetType() == "PO"){
			for (int j = 1; j < lst && !chk; j++){
				for (int x = 1; x <= 3; x++){
					if (!Vio_Check(pptr, j, 0, (AGINGTYPE)x, DCC_NONE, year+ERROR) && Vio_Check(pptr, j, 0, (AGINGTYPE)x, DCC_NONE, year - ERROR)){
						PathC.push_back(pptr);
						cout << "Start : " << stptr->GetName() << " End : " << edptr->GetName() << endl << "FF+PO" << endl;
						cout << "Put DCC On : " << stptr->GetClockPath(j)->GetName() << " NONE" << endl << "DCC TYPE : " << x << endl;
						bb++;
						chk = true;
						pptr->SetAttack(true);
						break;
					}
				}				
			}
		}
		while (branch < lst&&branch < led && !chk){			
			if (stptr->GetClockPath(branch) != edptr->GetClockPath(branch))
				break;
			for (int x = 1; x <= 3;x++)
				if (!Vio_Check(pptr, branch, branch, (AGINGTYPE)x, (AGINGTYPE)x, year+ERROR) && Vio_Check(pptr, branch, branch, (AGINGTYPE)x, (AGINGTYPE)x, year - ERROR)){
					PathC.push_back(pptr);
					cout << "Start : " << stptr->GetName() << " End : " << edptr->GetName() << endl << "FF+FF(branch)" << endl;
					cout << "Put DCC On : " << stptr->GetClockPath(branch)->GetName() << " " << edptr->GetClockPath(branch)->GetName() << endl << "DCC TYPE : " << x << endl;
					cc++;
					chk = true;
					pptr->SetAttack(true);
					break;
				}
			branch++;
		}
		if (!chk)
		for (int j = branch; j < lst && !chk; j++){
			for (int k = branch; k < led && !chk; k++){
				for (int x = 0; x < 3 && !chk; x++){
					for (int y = 0; y < 3; y++){
						if (x == 0 && y == 0)	continue;
						if (!Vio_Check(pptr, j, k, (AGINGTYPE)x, (AGINGTYPE)y, year+ERROR) && Vio_Check(pptr, j, k, (AGINGTYPE)x, (AGINGTYPE)y, year - ERROR)){
							PathC.push_back(pptr);
							cout << "Start : " << stptr->GetName() << " End : " << edptr->GetName() << endl << "FF+FF" << endl;
							cout << "Put DCC On : " << stptr->GetClockPath(branch)->GetName() << " " << edptr->GetClockPath(branch)->GetName() << endl << "DCC TYPE : " << x << ' ' << y << endl;
							dd++;
							chk = true;
							pptr->SetAttack(true);
							break;
						}
					}
				}
			}
		}
	}
	cout << aa << ' ' << bb << ' ' << cc << ' ' << dd << endl;
	return;
}

void GenerateSAT(string filename,int year){
	int ct = 0;
	for (int i = 0; i < PathC.size(); i++)		
		if (PathC[i]->Is_Chosen())	ct++;		
	cout << ct << endl;
	fstream file;
	fstream temp;	
	file.open(filename.c_str(), ios::out);
	map<GATE*, bool> exclusive;
	HashAllClockBuffer();	//每個clockbuffer之編號為在cbuffer_code內對應的號碼*2+1,*2+2	
	for (unsigned i = 0; i < PathC.size(); i++){
		PATH* pptr = PathC[i];
		GATE* stptr = pptr->Gate(0);
		GATE* edptr = pptr->Gate(pptr->length()-1);
		int stn = 0, edn = 0;	//放置點(之後，包括自身都會受影響)
		int lst = stptr->Clock_Length();
		int led = edptr->Clock_Length();
		
		if (!PathC[i]->Is_Chosen())	continue;

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
		while (stn < lst && edn < led){		//放在共同區上
			if (stptr->GetClockPath(stn) != edptr->GetClockPath(edn))
				break;
			for (int x = 0; x <= 3; x++){	//0/00 NO DCC 1/01 slow aging DCC(20%) 2/10 fast aging DCC(80%),需要加入3/11(50%DCC)?
				if (Vio_Check(pptr, stn, edn, (AGINGTYPE)x, (AGINGTYPE)x, year+ERROR) || !Vio_Check(pptr, stn, edn, (AGINGTYPE)x, (AGINGTYPE)x, year-ERROR)){	//Vio_Check如果沒有violation會回傳true
					if (x % 2 == 1)	//01 11 -> 10 00
						file << '-';
					file << cbuffer_code[stptr->GetClockPath(stn)] * 2 + 1<< ' ';
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
		if (stptr->GetType() == "PI"){
			for (edn = 0; edn < led; edn++){
				for (int x = 0; x <= 3; x++){
					if (Vio_Check(pptr, 0, edn, DCC_NONE, (AGINGTYPE)x, year+ERROR) || !Vio_Check(pptr, 0, edn, DCC_NONE, (AGINGTYPE)x, year-ERROR)){
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
					if (Vio_Check(pptr, stn, 0, (AGINGTYPE)x, DCC_NONE, year+ERROR) || !Vio_Check(pptr, stn, 0, (AGINGTYPE)x, DCC_NONE, year-ERROR)){
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
		int b_point = stn;
		for (; stn < lst; stn++){
			for (edn = b_point; edn < led; edn++){
				for (int x = 0; x <= 3; x++){
					for (int y = 0; y <= 3; y++){
						if (Vio_Check(pptr, stn, edn, (AGINGTYPE)x, (AGINGTYPE)y, year+ERROR) || !Vio_Check(pptr, stn, edn, (AGINGTYPE)x, (AGINGTYPE)y, year-ERROR)){
							if (x % 2 == 1)	//01 11 -> 1[0] 0[0]
								file << '-';
							file << cbuffer_code[stptr->GetClockPath(stn)] * 2 + 1<< ' ';
							if (x >= 2)	//10 11
								file << '-';
							file << cbuffer_code[stptr->GetClockPath(stn)] * 2 + 2 << ' ';
							if (y % 2 == 1)	
								file << '-';
							file << cbuffer_code[edptr->GetClockPath(edn)] * 2 + 1<< ' ';
							if (y >= 2)
								file << '-';
							file << cbuffer_code[edptr->GetClockPath(edn)] * 2 + 2 << ' ';
							for (int j = 0; j < stptr->Clock_Length(); j++){
								if (j == stn)	continue;
								file << cbuffer_code[stptr->GetClockPath(j)] * 2 + 1<< ' ' << cbuffer_code[stptr->GetClockPath(j)] * 2 + 2 << ' ';
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
	file.close();	
}

bool CallSatAndReadReport(){
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
	for (int i = 0; i < cbuffer_decode.size();i++)
		if (cbuffer_decode[i]->GetDcc() != DCC_NONE)
			cout << cbuffer_decode[i]->GetName() << ' ' << cbuffer_decode[i]->GetDcc() << endl;
		return true;
}


double CalQuality(int year){	//沒被選到的path提早出錯 -> 取差最遠的放入SAT重解	

	double worst_all = 0;	
	for (int i = 0; i < PathC.size(); i++){
		double best_case = 10000;
		for (int j = 0; j < PathC.size(); j++){			
			if (!PathC[j]->Is_Chosen())			
				continue;			
			double st = 1.0, ed = 10.0, mid;
			while (ed - st > 0.025){
				mid = (st + ed) / 2;
				double Aging_P = AgingRate(WORST, mid)*EdgeA[i][j] + EdgeB[i][j];	//y = ax+b+error(和相關係數有關)
				if (EdgeA[i][j]>1)
					Aging_P = AgingRate(WORST, mid);
				if (Vio_Check(PathC[j], mid, Aging_P))
					st = mid;
				else
					ed = mid;
			}
			if (mid < best_case)
				best_case = mid;			
		}
		if (worst_all < best_case)
			worst_all = best_case;
	}
	return worst_all;
}

bool RefineResult(int year){
	double early = 10000.0;
	int earlyp = -1;
	for (int i = 0; i < PathR.size(); i++){
		if (!PathR[i].Is_Chosen() && !Vio_Check(&PathR[i], (double)year - ERROR, AgingRate(WORST, year - ERROR))){
			if (PathR[i].CheckAttack())
				cout << "*";
			cout << i << ' ';
			double st = 1.0, ed = (double)year - ERROR, mid;
			while (ed - st>0.025){
				mid = (st + ed) / 2;
				if (Vio_Check(&PathR[i], mid, AgingRate(WORST, mid)))
					st = mid;
				else
					ed = mid;
			}
			if (early > mid){
				early = mid;
				earlyp = i;
			}
		}
	}
	if (earlyp < 0)
		return false;
	cout << endl;
	cout << earlyp << endl;
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