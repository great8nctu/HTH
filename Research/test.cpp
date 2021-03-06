
/*路徑讀取測試
string st, ed;
cout << "Press Startpoint & Endpoint of a path" << endl;
while (cin >> st >> ed){
	for (int i = 0; i < PathR.size(); i++){
		if (PathR[i].Gate(0)->GetName() == st && PathR[i].Gate(PathR[i].length() - 1)->GetName() == ed){
			cout << i << endl;
			cout << "CTH,CTE,AT,HT,ST" << endl;
			cout << PathR[i].GetCTH() << ',' << PathR[i].GetCTE() << ',' << PathR[i].GetAT() << ',' << PathR[i].GetHT() << ',' << PathR[i].GetST() << endl;
			GATE* gptr = PathR[i].Gate(0);
			cout << "Clock Path to Startpoint(" << st << ')' << endl;
			for (int j = 0; j < gptr->Clock_Length(); j++)
				cout << gptr->GetClockPath(j)->GetName() << ',' << gptr->GetClockPath(j)->GetInTime() << ',' << gptr->GetClockPath(j)->GetOutTime() << endl;
			cout << "Clock Path to EndPoint(" << ed << ')' << endl;
			gptr = PathR[i].Gate(PathR[i].length() - 1);
			for (int j = 0; j < gptr->Clock_Length(); j++)
				cout << gptr->GetClockPath(j)->GetName() << ',' << gptr->GetClockPath(j)->GetInTime() << ',' << gptr->GetClockPath(j)->GetOutTime() << endl;
			cout << "Path:" << endl;
			for (int j = 0; j < PathR[i].length(); j++)
				cout << PathR[i].Gate(j)->GetName() << ',' << PathR[i].In_time(j) << ',' << PathR[i].Out_time(j) << endl;
			cout << endl;
			//break;
		}
	}
}
*/

/*測試電路讀取用
string WireName;
while (cin >> WireName){
WIRE* w = Circuit[0].GetWire(WireName);
cout << w->GetInput()->GetName() << endl;
int n = w->No_Output();
for (int i = 0; i < n; i++)
cout << w->GetOutput(i)->GetName() << endl;
}
*/
/*
int k = 0;
for (unsigned i = 0; i < PathR.size(); i++){
	if (Choice[i]){
		cout << i << ' ';
		k++;
	}
}
cout << endl << k << endl;
*/

//if (stptr->GetName() == "u2_uk_K_r14_reg_19__u0" && edptr->GetName() == "g190305_u0_o")
//

//cout << "Start : " << pptr->Gate(0)->GetName() << " End : " << pptr->Gate(pptr->length() - 1)->GetName() << endl;
//cout << "Put DCC On : " << "NONE" << ' ' << edptr->GetClockPath(j)->GetName() << endl << "DCC TYPE" << x << endl;

/*
if (Vio_Check(pptr, stn, edn, ast, aed, year - 1)){
cout<<stptr->GetName() << ' ' << edptr->GetName() << endl;
cout << clks << ' ' << Tcq << ' ' << DelayP << ' ' << clkt << ' ' << pptr->GetST() << ' ' << period << endl << pptr->GetCTH() << endl;
}
*/


/*
int m, n, i;
double* pData;
m = n = PathC.size();
EdgeA = (double **)malloc(m*sizeof(double *)+m*n*sizeof(double));	//m個陣列開頭+m*n個空間
for (i = 0, pData = (double *)(EdgeA + m); i < m; i++, pData += n)	//前m(double*)個為陣列開頭,之後每個開頭(edgea[i])的位置相隔n(double)
EdgeA[i] = pData;
EdgeB = (double **)malloc(m*sizeof(double *)+m*n*sizeof(double));
for (i = 0, pData = (double *)(EdgeB + m); i < m; i++, pData += n)
EdgeB[i] = pData;
cor = (double **)malloc(m*sizeof(double *)+m*n*sizeof(double));
for (i = 0, pData = (double *)(cor + m); i < m; i++, pData += n)
cor[i] = pData;
*/


/*
for (int i = 0; i < ss; i++){
for (int j = i; j < ss; j++){
if (j == i){	//y = x
EdgeA[i][j] = 1;
EdgeB[i][j] = 0;
cor[i][j] = 0;
continue;
}
double a = (double)rand() / RAND_MAX*0.3 + 0.85;	//差距大概在15%
double b = 0.0;// (double)rand() / RAND_MAX - 0.5;	//-0.5~0.5
double c = (double)rand() * 2 / RAND_MAX - 1;
EdgeA[i][j] = a;		// y = ax+b
EdgeB[i][j] = b;
EdgeA[j][i] = 1 / a;	//	x = y/a-b/a
EdgeB[j][i] = -1*(b / a);
cor[i][j] = cor[j][i] = c;
}
}
*/

/*
cout << "Initial Estimate Soluation" << endl;
for (int i = 0; i < PathC.size(); i++){
CalSolMines(year, i);
cout << 100*(double)i / (double)PathC.size() << '%' << endl;
}
*/

/*
struct PATHSOL{
	GATE *a, *b;
	AGINGTYPE ta, tb;
	PATHSOL(GATE* x, GATE* y, AGINGTYPE m, AGINGTYPE n) :a(x), b(y), ta(m), tb(n){}
};

bool CheckSolConflict(PATHSOL f, PATH* p){
	GATE* stptr = p->Gate(0);
	GATE* edptr = p->Gate(p->length() - 1);
	if (f.a != NULL){
		if (f.a->GetDcc() != DCC_NONE && f.ta != f.a->GetDcc())
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
	GATE *stptr = pptr->Gate(0), *edptr = pptr->Gate(pptr->length() - 1);

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
	for (int i = p + 1; i < PathC.size(); i++)
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

*/


/*
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
*/


/*

void EstimateTimeEV(double year){
int No_node = PathC.size();
for (int i = 0; i < No_node; i++){
cout << '\r';
printf("NODE: %3d/%3d", i + 1, No_node);
PATH* pptr = PathC[i];
GATE* stptr = pptr->Gate(0);
GATE* edptr = pptr->Gate(pptr->length() - 1);
double max = 0;
double max2 = 0;
for (int k = 0; k < No_node;k++){
if (EdgeA[k][i]>9999)
continue;
double pv = 0;		//	期望值&解 (path i的, 由path k推得)
double pv2 = 0;		//	類標準差 -> sigma(xi - avg)^2 /N  用給定時間代替avg
int solc = 0;
if (stptr->GetType() == "PI"){
for (int j = 0; j < edptr->ClockLength(); j++){
for (int x = 1; x < 4; x++){
edptr->GetClockPath(j)->SetDcc((AGINGTYPE)x);
if (!Vio_Check(pptr, year+ERROR, AgingRate(WORST, year + ERROR)) && Vio_Check(pptr, year-ERROR, AgingRate(WORST, year - ERROR))){
double st = year - ERROR, ed = year + ERROR, mid;
while (ed - st>0.0001){
mid = (st + ed) / 2;
double Aging_P = CalPreAging(AgingRate(WORST, mid), k, i, year); //從第k個path點推到第i個path 找第i個path的期望值
if (Aging_P>AgingRate(WORST,mid))
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
for (int j = 0; j < stptr->ClockLength(); j++){
for (int x = 1; x < 4; x++){
stptr->GetClockPath(j)->SetDcc((AGINGTYPE)x);
if (!Vio_Check(pptr, year+ERROR, AgingRate(WORST, year + ERROR)) && Vio_Check(pptr, year-ERROR, AgingRate(WORST, year - ERROR))){
double st = year - ERROR, ed = year + ERROR, mid;
while (ed - st>0.0001){
mid = (st + ed) / 2;
double Aging_P = CalPreAging(AgingRate(WORST, mid), k, i, year);
if (Aging_P>AgingRate(WORST, mid))
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
double Aging_P = CalPreAging(AgingRate(WORST, mid), k, i, year);
if (Aging_P>AgingRate(WORST, mid))
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
for (int j = branch; j < stptr->ClockLength(); j++){
for (int j2 = branch; j2 < edptr->ClockLength(); j2++){
for (int x = 0; x < 4; x++){
for (int y = 0; y<4; y++){
if (x == 0 && y == 0)	continue;
edptr->GetClockPath(j2)->SetDcc((AGINGTYPE)y);
stptr->GetClockPath(j)->SetDcc((AGINGTYPE)x);
if (!Vio_Check(pptr, year+ERROR, AgingRate(WORST, year + ERROR)) && Vio_Check(pptr, year-ERROR, AgingRate(WORST, year - ERROR))){
double st = year - ERROR, ed = year + ERROR, mid;
while (ed - st>0.0001){
mid = (st + ed) / 2;
double Aging_P = CalPreAging(AgingRate(WORST, mid), k, i, year);
if (Aging_P>AgingRate(WORST, mid))
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
cout << endl;
}

double EstimateAddTimes(double year,int p){	//p是要加入的點
double max = 0;
for (int i = 0; i < PathC.size(); i++){
if (PathC[i]->Is_Chosen() && absl(year - PathC[i]->GetEstimateTime())>max)
max = absl(year - PathC[i]->GetEstimateTime());
}
if (absl(year - PathC[p]->GetEstimateTime()) > max)
return absl(year - PathC[p]->GetEstimateTime()) - max;		//傳回的是加入p後會差多少 => 如果比原本的還小就是0 不然就回傳差值
return 0.0;
}

double EstimatePSD(int p){
double max = 0;
double newmax = PathC[p]->GetPSD();
for (int i = 0; i < PathC.size(); i++){
if (PathC[i]->Is_Chosen() && PathC[i]->GetPSD()>max){		//同樣 傳回類標準差加入前後的最大值差 => 比原本小即0
max = PathC[i]->GetPSD();
}
}
if (max > newmax)
return 0.0;

return newmax - max;
}
*/