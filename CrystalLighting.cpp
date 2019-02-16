// C++11
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>
#include <set>
#include <string>
#include <cmath>
#include <utility>
#include <array>
#include <tuple>
//#include <fstream>
using namespace std;
const int SIZE=101;
double now_time=0.;
//ofstream outputfile("result.txt");
//下、右、上、左
int dx[4]={0,1,0,-1};
int dy[4]={1,0,-1,0};
//H,W
int H,W;
//ランタンどうしが干渉した時の罰点
double lantPun=0;
//座標
struct Point{
	int x,y;
};
//Crystalの状態を保持するクラス
struct Crystal{
	//座標
	Point point;
	//Crystalの色
	int color;
	//自分を照らしている光の情報
	int lights[3];
};
//Lanternの状態を保持するクラス
struct Lantern{
	//座標
	Point point;
	//Lanternの色
	int color;
};
//Obstacleの状態を保持するクラス
struct Obstacle{
	//座標
	Point point;
};
//Mirrorの状態を保持するクラス
struct Mirror{
	//座標
	Point point;
	//鏡のタイプ(/:0,\:1)
	int type;
};
//それぞれの物体について、リストを構築する
vector<Crystal> Crystals;
vector<Lantern> Lanterns;
vector<Obstacle> Obstacles;
vector<Mirror> Mirrors;
//それぞれの物体について盤面を構築する
int wallBoard[SIZE][SIZE]={},crystalBoard[SIZE][SIZE]={},lanternBoard[SIZE][SIZE]={},obstacleBoard[SIZE][SIZE]={},mirrorBoard[SIZE][SIZE]={};
//光が足りてないクリスタルの集合
vector<int> lessCrys;
//逆対応（lessCrysInv[クリスタルの番号]=lessCrysでの番号）
int lessCrysInv[10050]={};
//光が過剰なクリスタルの集合
vector<int> muchCrys;
//逆対応（muchCrysInv[クリスタルの番号]=muchCrysでの番号）
int muchCrysInv[10050]={};
//xorShift
inline int randxor(){
	static unsigned int x=123456789,y=987235098,z=957398509,w=879423879;
	unsigned int t;
	t=(x^(x<<11));
	x=y;
	y=z;
	z=w;
	w=(w^(w>>19))^(t^(t>>8));
	return abs((int)w);
}
//その座標にすでになんかあるかを調べる関数
inline bool is_occupied(int x,int y){
	//盤面の外;
	if(x<0 || W<=x || y<0 || H<=y)
		return 1;
	//壁;
	if(wallBoard[y][x]!=-1)
		return 1;
	//lantern;
	if(lanternBoard[y][x]!=-1)
		return 1;
	//obstacle;
	if(obstacleBoard[y][x]!=-1)
		return 1;
	//crystal;
	if(crystalBoard[y][x]!=-1)
		return 1;
	//mirror
	if(mirrorBoard[y][x]!=-1)
		return 1;
	return 0;
}
//ある座標から4方向に探索して、それぞれ何があったかを返す関数
inline pair<array<int,4>,array<int,4>> SearchFour(int X,int Y){
	//typeについて
	//0:空白(盤面の外),1:ランタン,2:クリスタル,3:壁,4:障害物
	//paramはそのオブジェクトのリストにおける番号
	array<int,4> type={},param={};
	//4方向に探索する
	for(int k=0;k<4;k++){
		//現在探索している方向
		int dir=k;
		//現在の座標
		Point P=Point{X,Y};
		//何かにぶつかるまで探索し続ける
		while(1){
			//次に見る座標
			Point nextP=Point{P.x+dx[dir],P.y+dy[dir]};
			//盤面の外
			if(nextP.x<0 || W<=nextP.x || nextP.y<0 || H<=nextP.y){
				type[k]=0;
				param[k]=0;
				break;
			}
			//ランタン
			if(lanternBoard[nextP.y][nextP.x]!=-1){
				type[k]=1;
				param[k]=lanternBoard[nextP.y][nextP.x];
				break;
			}
			//クリスタル
			if(crystalBoard[nextP.y][nextP.x]!=-1){
				type[k]=2;
				param[k]=crystalBoard[nextP.y][nextP.x];
				break;
			}
			//壁
			if(wallBoard[nextP.y][nextP.x]!=-1){
				type[k]=3;
				param[k]=0;
				break;
			}
			//障害物
			if(obstacleBoard[nextP.y][nextP.x]!=-1){
				type[k]=4;
				param[k]=obstacleBoard[nextP.y][nextP.x];
				break;
			}
			//鏡
			if(mirrorBoard[nextP.y][nextP.x]!=-1){
				int mirrorType=Mirrors[mirrorBoard[nextP.y][nextP.x]].type;
				//dirを変える
				if(mirrorType==1){
					if(dir==0)
						dir=1;
					else if(dir==1)
						dir=0;
					else if(dir==2)
						dir=3;
					else if(dir==3)
						dir=2;
				}
				else{
					if(dir==0)
						dir=3;
					else if(dir==1)
						dir=2;
					else if(dir==2)
						dir=1;
					else if(dir==3)
						dir=0;
				}
			}
			//進む
			P=nextP;
		}
	}
	return make_pair(type,param);
}
//ある座標からランダムに1方向に探索して、空白の座標の中からランダムに1つ選んで返す関数
inline Point RandPoint(int X,int Y){
	//探索する方向
	int dir=randxor()%4;
	//現在の座標
	Point P=Point{X,Y};
	//探索する
	for(int i=1;1;i++){
		//次の座標
		Point nextP=Point{P.x+dx[dir],P.y+dy[dir]};
		//次の座標になんかあるならそこで止める
		if(is_occupied(nextP.x,nextP.y)){
			//i==1なら、適当な座標を選んで終了
			if(i==1)
				return Point{(int)randxor()%W,(int)randxor()%H};
			//i>1なら、1<=R<iの中からランダ� な座標を選んで返す
			else{
				int R=1+(randxor()%(i-1));
				return Point{X+R*dx[dir],Y+R*dy[dir]};
			}

		}
		//進む
		P=nextP;
	}
	return Point{(int)randxor()%W,(int)randxor()%H};
}
//クリスタルに光が当たった/消えたときのスコア変化を計算する関数
inline double CrysLantInteractionDelta(int nowCrys,int lantColor,char op){
	//スコア変化
	double deltaScore=0.;
	//クリスタルに光を当てる前後のスコア
	double befScore=0.,aftScore=0.;
	//前後のlights
	int befLights[3]={},aftLights[3]={};
	//前後の光の合計
	int befLightsSum=0,aftLightsSum=0;
	//lightを写し取る
	for(int i=0;i<3;i++){
		befLights[i]=Crystals[nowCrys].lights[i];
		befLightsSum+=befLights[i];
		if(op=='+')
			aftLights[i]=Crystals[nowCrys].lights[i]+(lantColor&(1<<i));
		else
			aftLights[i]=Crystals[nowCrys].lights[i]-(lantColor&(1<<i));
		aftLightsSum+=aftLights[i];
	}
	//スコア計算
	//そのクリスタルに入っていてほしい色
	int crysColor=Crystals[nowCrys].color;
	int crysLights[3]={crysColor&(1),crysColor&(2),crysColor&(4)};
	//色が一致しているか？
	bool befComp=1,aftComp=1;
	//色の一致している数を見る
	int befMatch=3,aftMatch=3;
	//bef
	for(int i=0;i<3;i++){
		//色が一致してないならflagを0にしてbreak;
		if((!!befLights[i])!=(!!crysLights[i])){
			befComp=0;
			befMatch--;
		}
	}
	//aft
	for(int i=0;i<3;i++){
		//色が一致してないならflagを0にしてbreak;
		if((!!aftLights[i])!=(!!crysLights[i])){
			aftComp=0;
			aftMatch--;
		}
	}
	//bef
	//LightsSumが0ならスコアはゼロ
	if(befLightsSum==0)
		befScore=0.;
	//そうでないとき、一致していれば得点、そうでなければ減点
	else if(befComp)
		befScore=((crysColor==1 || crysColor==2 || crysColor==4)?20.:30.);
	else
		befScore=-10.;
	//aft
	//LightsSumが0ならスコアはゼロ
	if(aftLightsSum==0)
		aftScore=0.;
	//そうでないとき、一致していれば得点、そうでなければ減点
	else if(aftComp)
		aftScore=((crysColor==1 || crysColor==2 || crysColor==4)?20.:30.);
	else
		aftScore=-10.;

	//時間が8500msec以下のとき、一致数に応じて少し得点
	if(now_time<8500.){
		befScore+=3*befMatch;
		aftScore+=3*aftMatch;
	}
	//deltaScoreにaft-befを足す
	deltaScore+=aftScore-befScore;
	return deltaScore;
}
//クリスタルに光を当てる/消す関数
inline void CrysLantInteraction(int nowCrys,int lantColor,char op){
	//lessCrys,muchCrysを処理する
	if(lessCrysInv[nowCrys]!=-1){
		int temp=lessCrysInv[nowCrys];
		lessCrysInv[lessCrys.back()]=lessCrysInv[nowCrys];
		lessCrysInv[nowCrys]=-1;
		swap(lessCrys[temp],lessCrys.back());
		lessCrys.pop_back();
	}
	if(muchCrysInv[nowCrys]!=-1){
		int temp=muchCrysInv[nowCrys];
		muchCrysInv[muchCrys.back()]=muchCrysInv[nowCrys];
		muchCrysInv[nowCrys]=-1;
		swap(muchCrys[temp],muchCrys.back());
		muchCrys.pop_back();
	}
	//lightsを更新する
	for(int i=0;i<3;i++){
		if(op=='+')
			Crystals[nowCrys].lights[i]+=(lantColor&(1<<i));
		else
			Crystals[nowCrys].lights[i]-=(lantColor&(1<<i));
	}
	//lessCrys,muchCrysを処理する
	bool is_less=0,is_much=0;
	for(int i=0;i<3;i++){
		if((Crystals[nowCrys].color&(1<<i)) && Crystals[nowCrys].lights[i]==0)
			is_less=1;
		if(((Crystals[nowCrys].color&(1<<i))==0) && Crystals[nowCrys].lights[i])
			is_much=1;
	}
	if(is_less){
		lessCrys.push_back(nowCrys);
		lessCrysInv[nowCrys]=lessCrys.size()-1;
	}
	if(is_much){
		muchCrys.push_back(nowCrys);
		muchCrysInv[nowCrys]=muchCrys.size()-1;
	}
	return;
}
//ランタンを置く場所の候補を作る関数
inline pair<Point,int> LanternPutPoint(){
	//光が足りないクリスタルがないとき、適当に座標を選んで終了
	if(lessCrys.size()==0){
		return make_pair(Point{(int)randxor()%W,(int)randxor()%H},1<<((int)randxor()%3));
	}
	//光が足りないクリスタルをランダムに選ぶ
	int nowCrys=lessCrys[(int)randxor()%lessCrys.size()];
	//入れたい色を決める
	int color=1;
	for(int i=0;i<3;i++)
		if(Crystals[nowCrys].color&(1<<i) && Crystals[nowCrys].lights[i]==0)
			color=(1<<i);
	//そのクリスタルからランダムな方向に探索して、空白の座標を候補にする
	return make_pair(RandPoint(Crystals[nowCrys].point.x,Crystals[nowCrys].point.y),color);
}
//ランタンを置いたときのスコア変化を見る関数
inline tuple<int,vector<int>,vector<pair<int,int>>> PutLanternDelta(int X,int Y,int lantColor,int costLantern){
	double deltaScore=-costLantern;
	//光が当たるクリスタルの番号の集合
	vector<int> vCrys;
	//光が当たるランタンの番号の集合
	vector<pair<int,int>> vLantCrys;
	//光を遮るランタンとクリスタルの番号の集合
	//四方向に探索する
	pair<array<int,4>,array<int,4>> SF=SearchFour(X,Y);
	array<int,4> type=SF.first,param=SF.second;
	for(int k=0;k<4;k++){
		//ランタンがあれば罰点
		if(type[k]==1){
			deltaScore-=lantPun;
			//ランタンがあるとき、そのランタンの光を遮ることになる
			if(type[(k+2)%4]==2){
				deltaScore+=CrysLantInteractionDelta(param[(k+2)%4],Lanterns[param[k]].color,'-');
				vLantCrys.push_back(make_pair(param[k],param[(k+2)%4]));
			}
		}
		//クリスタルがあれば、スコア変化を計算してvCyrsに入れる
		//光を足す遷移
		if(type[k]==2){
			deltaScore+=CrysLantInteractionDelta(param[k],lantColor,'+');
			vCrys.push_back(param[k]);
		}
	}
	return make_tuple(deltaScore,vCrys,vLantCrys);
}
//ランタンを取り除いたときのスコア変化を見る関数
inline tuple<int,vector<int>,vector<pair<int,int>>> RemoveLanternDelta(int nowLant,int costLantern){
	double deltaScore=costLantern;
	//取り除くランタンの色
	int lantColor=Lanterns[nowLant].color;
	//光が当たるクリスタルの番号の集合
	vector<int> vCrys;
	vector<pair<int,int>> vLantCrys;
	//光を遮るランタンとクリスタルの番号の集合
	int X=Lanterns[nowLant].point.x,Y=Lanterns[nowLant].point.y;
	//四方向に探索する
	pair<array<int,4>,array<int,4>> SF=SearchFour(X,Y);
	array<int,4> type=SF.first,param=SF.second;
	for(int k=0;k<4;k++){
		//取り除くランタンが照らしているランタンの数だけ加点
		if(type[k]==1){
			deltaScore+=lantPun;
			//ランタンがあるとき、そのランタンの光が入射する
			if(type[(k+2)%4]==2){
				deltaScore+=CrysLantInteractionDelta(param[(k+2)%4],Lanterns[param[k]].color,'+');
				vLantCrys.push_back(make_pair(param[k],param[(k+2)%4]));
			}
		}
		//クリスタルがあれば、スコア変化を計算してvCrysに入れる
		//光を引く遷移
		if(type[k]==2){
			deltaScore+=CrysLantInteractionDelta(param[k],lantColor,'-');
			vCrys.push_back(param[k]);
		}
	}
	return make_tuple(deltaScore,vCrys,vLantCrys);
}
//ランタンを置く関数
inline void PutLantern(int X,int Y,int lantColor,vector<int> vCrys,vector<pair<int,int>> vLantCrys){
	//ランタンを作る
	Lantern newLant=Lantern{Point{X,Y},lantColor};
	int nowLant=Lanterns.size();
	lanternBoard[Y][X]=nowLant;
	Lanterns.push_back(newLant);
	//vCrysに入っているクリスタルのlightsを更新する
	for(int nowCrys:vCrys)
		CrysLantInteraction(nowCrys,lantColor,'+');
	//vLantCrys
	for(pair<int,int> P:vLantCrys)
		CrysLantInteraction(P.second,Lanterns[P.first].color,'-');
	return;
}
//ランタンを取り除く関数
inline void RemoveLantern(int nowLant,vector<int> vCrys,vector<pair<int,int>> vLantCrys){
	//ランタンの色
		int lantColor=Lanterns[nowLant].color;
	//vCrysに入っているクリスタルのlightsを更新する
	for(int nowCrys:vCrys)
		CrysLantInteraction(nowCrys,lantColor,'-');
	//vLantCrys
	for(pair<int,int> P:vLantCrys)
		CrysLantInteraction(P.second,Lanterns[P.first].color,'+');
	//ランタンを取り除く
	swap(lanternBoard[Lanterns[nowLant].point.y][Lanterns[nowLant].point.x],lanternBoard[Lanterns.back().point.y][Lanterns.back().point.x]);
	swap(Lanterns[nowLant],Lanterns.back());
	lanternBoard[Lanterns.back().point.y][Lanterns.back().point.x]=-1;
	Lanterns.pop_back();
	return;
}
//障害物を置いたときのスコアの変化を見る関数
inline double PutObstacleDelta(int X,int Y,int costObstacle){
	double deltaScore=-costObstacle;
	//4方向を探索する
	pair<array<int,4>,array<int,4>> SF=SearchFour(X,Y);
	array<int,4> type=SF.first,param=SF.second;
	for(int k=0;k<4;k++){
		//k方向と(k+2)%4方向に両方ランタンがあるとき
		if(type[k]==1 && type[(k+2)%4]==1)
			deltaScore+=lantPun/2.;
		//k方向にランタンかつ(k+2)%4方向にクリスタルがあるとき
		if(type[k]==1 && type[(k+2)%4]==2){
			int lantColor=Lanterns[param[k]].color;
			int nowCrys=param[(k+2)%4];
			deltaScore+=CrysLantInteractionDelta(nowCrys,lantColor,'-');
		}
	}
	return deltaScore;
}
//障害物を取り除いたときのスコアの変化を見る関数
inline double RemoveObstacleDelta(int nowObs,int costObstacle){
	double deltaScore=+costObstacle;
	int X=Obstacles[nowObs].point.x,Y=Obstacles[nowObs].point.y;
	//4方向を探索する
	pair<array<int,4>,array<int,4>> SF=SearchFour(X,Y);
	array<int,4> type=SF.first,param=SF.second;
	for(int k=0;k<4;k++){
		//k方向と(k+2)%4方向に両方ランタンがあるとき
		if(type[k]==1 && type[(k+2)%4]==1)
			deltaScore-=lantPun/2.;
		//k方向にランタンかつ(k+2)%4方向にクリスタルがあるとき
		if(type[k]==1 && type[(k+2)%4]==2){
			int lantColor=Lanterns[param[k]].color;
			int nowCrys=param[(k+2)%4];
			deltaScore+=CrysLantInteractionDelta(nowCrys,lantColor,'+');
		}
	}
	return deltaScore;
}
//障害物を置く関数
inline void PutObstacle(int X,int Y){
	//障害物を置く
	Obstacles.push_back(Obstacle{Point{X,Y}});
	obstacleBoard[Y][X]=Obstacles.size()-1;
	//4方向を探索する
	pair<array<int,4>,array<int,4>> SF=SearchFour(X,Y);
	array<int,4> type=SF.first,param=SF.second;
	for(int k=0;k<4;k++){
		//k方向にランタンかつ(k+2)%4方向にクリスタルがあるとき
		if(type[k]==1 && type[(k+2)%4]==2){
			int lantColor=Lanterns[param[k]].color;
			int nowCrys=param[(k+2)%4];
			CrysLantInteraction(nowCrys,lantColor,'-');
		}
	}
	return;
}
//障害物を取り除く関数
inline void RemoveObstacle(int nowObs){
	int X=Obstacles[nowObs].point.x,Y=Obstacles[nowObs].point.y;
	//4方向を探索する
	pair<array<int,4>,array<int,4>> SF=SearchFour(X,Y);
	array<int,4> type=SF.first,param=SF.second;
	for(int k=0;k<4;k++){
		//k方向にランタンかつ(k+2)%4方向にクリスタルがあるとき
		if(type[k]==1 && type[(k+2)%4]==2){
			int lantColor=Lanterns[param[k]].color;
			int nowCrys=param[(k+2)%4];
			CrysLantInteraction(nowCrys,lantColor,'+');
		}
	}
	//障害物を取り除く
	swap(obstacleBoard[Obstacles[nowObs].point.y][Obstacles[nowObs].point.x],obstacleBoard[Obstacles.back().point.y][Obstacles.back().point.x]);
	swap(Obstacles[nowObs],Obstacles.back());
	obstacleBoard[Obstacles.back().point.y][Obstacles.back().point.x]=-1;
	Obstacles.pop_back();
	return;
}
//障害物を置く場所の候補を決める関数
inline Point ObstaclePutPoint(){
	//光が過剰なクリスタルがないとき、適当に座標を選んで終了
	if(muchCrys.size()==0){
		return Point{(int)randxor()%W,(int)randxor()%H};
	}
	//光が過剰なクリスタルをランダムに選ぶ
	int nowCrys=muchCrys[(int)randxor()%muchCrys.size()];
	//そのクリスタルからランダムな方向に探索して、空白の座標を候補にする
	return RandPoint(Crystals[nowCrys].point.x,Crystals[nowCrys].point.y);
}
//障害物を鏡に変えたときのスコアの変化を見る関数
inline double ObsToMirDelta(int nowObs,int mirrorType,int costObstacle,int costMirror){
	double deltaScore=costObstacle-costMirror;
	int X=Obstacles[nowObs].point.x,Y=Obstacles[nowObs].point.y;
	//4方向を探索する
	pair<array<int,4>,array<int,4>> SF=SearchFour(X,Y);
	array<int,4> type=SF.first,param=SF.second;
	//鏡の部分の処理
	if(mirrorType==0){
		//0と1もしくは2と3が両方ランタンのとき、罰点
		for(int i:{0,2})
			if(type[i]==1 && type[i+1]==1)
				deltaScore-=lantPun;
		//ランタンの光がクリスタルに入るとき
		for(int i:{0,2}){
			if(type[i]==1 && type[i+1]==2){
				int lantColor=Lanterns[param[i]].color;
				int nowCrys=param[i+1];
				deltaScore+=CrysLantInteractionDelta(nowCrys,lantColor,'+');
			}
			if(type[i+1]==1 && type[i]==2){
				int lantColor=Lanterns[param[i+1]].color;
				int nowCrys=param[i];
				deltaScore+=CrysLantInteractionDelta(nowCrys,lantColor,'+');
			}
		}
	}
	//鏡B(\)のとき
	else{
		//3と0もしくは1と2が両方ランタンのとき、罰点
		for(int i:{1,3})
			if(type[i]==1 && type[(i+1)%4]==1)
				deltaScore-=lantPun;
		//ランタンの光がクリスタルに入るとき
		for(int i:{1,3}){
			if(type[i]==1 && type[(i+1)%4]==2){
				int lantColor=Lanterns[param[i]].color;
				int nowCrys=param[(i+1)%4];
				deltaScore+=CrysLantInteractionDelta(nowCrys,lantColor,'+');
			}
			if(type[(i+1)%4]==1 && type[i]==2){
				int lantColor=Lanterns[param[(i+1)%4]].color;
				int nowCrys=param[i];
				deltaScore+=CrysLantInteractionDelta(nowCrys,lantColor,'+');
			}
		}
	}
	return deltaScore;
}
//鏡を障害物に変えたときのスコア変化を見る関数
inline double MirToObsDelta(int nowMir,int costObstacle,int costMirror){
	double deltaScore=costMirror-costObstacle;
	int X=Mirrors[nowMir].point.x,Y=Mirrors[nowMir].point.y;
	int mirrorType=Mirrors[nowMir].type;
	//4方向を探索する
	pair<array<int,4>,array<int,4>> SF=SearchFour(X,Y);
	array<int,4> type=SF.first,param=SF.second;
	//鏡A(/)のとき
	if(mirrorType==0){
		//0と1もしくは2と3が両方ランタンのとき、罰点
		for(int i:{0,2})
			if(type[i]==1 && type[i+1]==1)
				deltaScore+=lantPun;
		//ランタンの光がクリスタルに入るとき
		for(int i:{0,2}){
			if(type[i]==1 && type[i+1]==2){
				int lantColor=Lanterns[param[i]].color;
				int nowCrys=param[i+1];
				deltaScore+=CrysLantInteractionDelta(nowCrys,lantColor,'-');
			}    
			if(type[i+1]==1 && type[i]==2){
				int lantColor=Lanterns[param[i+1]].color;
				int nowCrys=param[i];
				deltaScore+=CrysLantInteractionDelta(nowCrys,lantColor,'-');
			}    
		}
	}
	//鏡B(\)のとき
	else{
		//3と0もしくは1と2が両方ランタンのとき、罰点
		for(int i:{1,3})
			if(type[i]==1 && type[(i+1)%4]==1)
				deltaScore+=lantPun;
		//ランタンの光がクリスタルに入るとき
		for(int i:{1,3}){
			if(type[i]==1 && type[(i+1)%4]==2){
				int lantColor=Lanterns[param[i]].color;
				int nowCrys=param[(i+1)%4];
				deltaScore+=CrysLantInteractionDelta(nowCrys,lantColor,'-');
			}    
			if(type[(i+1)%4]==1 && type[i]==2){
				int lantColor=Lanterns[param[(i+1)%4]].color;
				int nowCrys=param[i];
				deltaScore+=CrysLantInteractionDelta(nowCrys,lantColor,'-');
			}    
		}
	}
	return deltaScore;
}
//障害物を鏡に変える関数
inline void ObsToMir(int nowObs,int mirrorType){
	int X=Obstacles[nowObs].point.x,Y=Obstacles[nowObs].point.y;
	//鏡を置く
	Mirrors.push_back(Mirror{Point{X,Y},mirrorType});
	mirrorBoard[Y][X]=Mirrors.size()-1;
	//4方向を探索する
	pair<array<int,4>,array<int,4>> SF=SearchFour(X,Y);
	array<int,4> type=SF.first,param=SF.second;
	//鏡としての処理
	//鏡A(/)のとき
	if(mirrorType==0){
		//ランタンの光がクリスタルに入るとき
		for(int i:{0,2}){
			if(type[i]==1 && type[i+1]==2){
				int lantColor=Lanterns[param[i]].color;
				int nowCrys=param[i+1];
				CrysLantInteraction(nowCrys,lantColor,'+');
			}    
			if(type[i+1]==1 && type[i]==2){
				int lantColor=Lanterns[param[i+1]].color;
				int nowCrys=param[i];
				CrysLantInteraction(nowCrys,lantColor,'+');
			}    
		}
	}
	//鏡B(\)のとき
	else{
		for(int i:{1,3}){
			if(type[i]==1 && type[(i+1)%4]==2){
				int lantColor=Lanterns[param[i]].color;
				int nowCrys=param[(i+1)%4];
				CrysLantInteraction(nowCrys,lantColor,'+');
			}    
			if(type[(i+1)%4]==1 && type[i]==2){
				int lantColor=Lanterns[param[(i+1)%4]].color;
				int nowCrys=param[i];
				CrysLantInteraction(nowCrys,lantColor,'+');
			}    
		}
	}
	//障害物を取り除く
	swap(obstacleBoard[Obstacles[nowObs].point.y][Obstacles[nowObs].point.x],obstacleBoard[Obstacles.back().point.y][Obstacles.back().point.x]);
	swap(Obstacles[nowObs],Obstacles.back());
	obstacleBoard[Obstacles.back().point.y][Obstacles.back().point.x]=-1;
	Obstacles.pop_back();
	return;
}
//鏡を障害物に変える関数
inline void MirToObs(int nowMir){
	int X=Mirrors[nowMir].point.x,Y=Mirrors[nowMir].point.y;
	int mirrorType=Mirrors[nowMir].type;
	//障害物を置く
	Obstacles.push_back(Obstacle{Point{X,Y}});
	obstacleBoard[Y][X]=Obstacles.size()-1;
	//4方向を探索する
	pair<array<int,4>,array<int,4>> SF=SearchFour(X,Y);
	array<int,4> type=SF.first,param=SF.second;
	//鏡としての処理
	//鏡A(/)のとき
	if(mirrorType==0){
		//ランタンの光がクリスタルに入るとき
		for(int i:{0,2}){
			if(type[i]==1 && type[i+1]==2){
				int lantColor=Lanterns[param[i]].color;
				int nowCrys=param[i+1];
				CrysLantInteraction(nowCrys,lantColor,'-');
			}    
			if(type[i+1]==1 && type[i]==2){
				int lantColor=Lanterns[param[i+1]].color;
				int nowCrys=param[i];
				CrysLantInteraction(nowCrys,lantColor,'-');
			}
		}
	}
	//鏡B(\)のとき
	else{
		//ランタンの光がクリスタルに入るとき
		for(int i:{1,3}){
			if(type[i]==1 && type[(i+1)%4]==2){
				int lantColor=Lanterns[param[i]].color;
				int nowCrys=param[(i+1)%4];
				CrysLantInteraction(nowCrys,lantColor,'-');
			}    
			if(type[(i+1)%4]==1 && type[i]==2){
				int lantColor=Lanterns[param[(i+1)%4]].color;
				int nowCrys=param[i];
				CrysLantInteraction(nowCrys,lantColor,'-');
			}    
		}
	}
	//鏡を取り除く
	swap(mirrorBoard[Mirrors[nowMir].point.y][Mirrors[nowMir].point.x],mirrorBoard[Mirrors.back().point.y][Mirrors.back().point.x]);
	swap(Mirrors[nowMir],Mirrors.back());
	mirrorBoard[Mirrors.back().point.y][Mirrors.back().point.x]=-1;
	Mirrors.pop_back();
	return;
}
//鏡を置いたときのスコア変化を見る関数
inline double PutMirrorDelta(int X,int Y,int mirrorType,int costMirror){
	double deltaScore=-costMirror;
	//4方向を探索する
	pair<array<int,4>,array<int,4>> SF=SearchFour(X,Y);
	array<int,4> type=SF.first,param=SF.second;
	//障害物としての処理
	for(int k=0;k<4;k++){
		//k方向と(k+2)%4方向に両方ランタンがあるとき
		if(type[k]==1 && type[(k+2)%4]==1)
			deltaScore+=lantPun/2.;
		//k方向にランタンかつ(k+2)%4方向にクリスタルがあるとき
		if(type[k]==1 && type[(k+2)%4]==2){
			int lantColor=Lanterns[param[k]].color;
			int nowCrys=param[(k+2)%4];
			deltaScore+=CrysLantInteractionDelta(nowCrys,lantColor,'-');
		}
	}
	//鏡としての処理
	if(mirrorType==0){
		//0と1もしくは2と3が両方ランタンのとき、罰点
		for(int i:{0,2})
			if(type[i]==1 && type[i+1]==1)
				deltaScore-=lantPun;
		//ランタンの光がクリスタルに入るとき
		for(int i:{0,2}){
			if(type[i]==1 && type[i+1]==2){
				int lantColor=Lanterns[param[i]].color;
				int nowCrys=param[i+1];
				deltaScore+=CrysLantInteractionDelta(nowCrys,lantColor,'+');
			}
			if(type[i+1]==1 && type[i]==2){
				int lantColor=Lanterns[param[i+1]].color;
				int nowCrys=param[i];
				deltaScore+=CrysLantInteractionDelta(nowCrys,lantColor,'+');
			}
		}
	}
	//鏡B(\)のとき
	else{
		//3と0もしくは1と2が両方ランタンのとき、罰点
		for(int i:{1,3})
			if(type[i]==1 && type[(i+1)%4]==1)
				deltaScore-=lantPun;
		//ランタンの光がクリスタルに入るとき
		for(int i:{1,3}){
			if(type[i]==1 && type[(i+1)%4]==2){
				int lantColor=Lanterns[param[i]].color;
				int nowCrys=param[(i+1)%4];
				deltaScore+=CrysLantInteractionDelta(nowCrys,lantColor,'+');
			}
			if(type[(i+1)%4]==1 && type[i]==2){
				int lantColor=Lanterns[param[(i+1)%4]].color;
				int nowCrys=param[i];
				deltaScore+=CrysLantInteractionDelta(nowCrys,lantColor,'+');
			}
		}
	}
	return deltaScore;
}
//鏡を取り除いたときのスコア変化を見る関数
inline double RemoveMirrorDelta(int nowMir,int costMirror){
	double deltaScore=costMirror;
	int X=Mirrors[nowMir].point.x,Y=Mirrors[nowMir].point.y;
	int mirrorType=Mirrors[nowMir].type;
	//4方向を探索する
	pair<array<int,4>,array<int,4>> SF=SearchFour(X,Y);
	array<int,4> type=SF.first,param=SF.second;
	//障害物としての処理
	for(int k=0;k<4;k++){
		//k方向と(k+2)%4方向に両方ランタンがあるとき
		if(type[k]==1 && type[(k+2)%4]==1)
			deltaScore-=lantPun/2.;
		//k方向にランタンかつ(k+2)%4方向にクリスタルがあるとき
		if(type[k]==1 && type[(k+2)%4]==2){
			int lantColor=Lanterns[param[k]].color;
			int nowCrys=param[(k+2)%4];
			deltaScore+=CrysLantInteractionDelta(nowCrys,lantColor,'+');
		}
	}
	//鏡としての処理
	//鏡A(/)のとき
	if(mirrorType==0){
		//0と1もしくは2と3が両方ランタンのとき、罰点
		for(int i:{0,2})
			if(type[i]==1 && type[i+1]==1)
				deltaScore+=lantPun;
		//ランタンの光がクリスタルに入るとき
		for(int i:{0,2}){
			if(type[i]==1 && type[i+1]==2){
				int lantColor=Lanterns[param[i]].color;
				int nowCrys=param[i+1];
				deltaScore+=CrysLantInteractionDelta(nowCrys,lantColor,'-');
			}    
			if(type[i+1]==1 && type[i]==2){
				int lantColor=Lanterns[param[i+1]].color;
				int nowCrys=param[i];
				deltaScore+=CrysLantInteractionDelta(nowCrys,lantColor,'-');
			}    
		}
	}
	//鏡B(\)のとき
	else{
		//3と0もしくは1と2が両方ランタンのとき、罰点
		for(int i:{1,3})
			if(type[i]==1 && type[(i+1)%4]==1)
				deltaScore+=lantPun;
		//ランタンの光がクリスタルに入るとき
		for(int i:{1,3}){
			if(type[i]==1 && type[(i+1)%4]==2){
				int lantColor=Lanterns[param[i]].color;
				int nowCrys=param[(i+1)%4];
				deltaScore+=CrysLantInteractionDelta(nowCrys,lantColor,'-');
			}    
			if(type[(i+1)%4]==1 && type[i]==2){
				int lantColor=Lanterns[param[(i+1)%4]].color;
				int nowCrys=param[i];
				deltaScore+=CrysLantInteractionDelta(nowCrys,lantColor,'-');
			}    
		}
	}
	return deltaScore;
}
//鏡を置く関数
inline void PutMirror(int X,int Y,int mirrorType){
	//鏡を置く
	Mirrors.push_back(Mirror{Point{X,Y},mirrorType});
	mirrorBoard[Y][X]=Mirrors.size()-1;
	//4方向を探索する
	pair<array<int,4>,array<int,4>> SF=SearchFour(X,Y);
	array<int,4> type=SF.first,param=SF.second;
	//障害物としての処理
	for(int k=0;k<4;k++){
		//k方向にランタンかつ(k+2)%4方向にクリスタルがあるとき
		if(type[k]==1 && type[(k+2)%4]==2){
			int lantColor=Lanterns[param[k]].color;
			int nowCrys=param[(k+2)%4];
			CrysLantInteraction(nowCrys,lantColor,'-');
		}
	}
	//鏡としての処理
	//鏡A(/)のとき
	if(mirrorType==0){
		//ランタンの光がクリスタルに入るとき
		for(int i:{0,2}){
			if(type[i]==1 && type[i+1]==2){
				int lantColor=Lanterns[param[i]].color;
				int nowCrys=param[i+1];
				CrysLantInteraction(nowCrys,lantColor,'+');
			}    
			if(type[i+1]==1 && type[i]==2){
				int lantColor=Lanterns[param[i+1]].color;
				int nowCrys=param[i];
				CrysLantInteraction(nowCrys,lantColor,'+');
			}    
		}
	}
	//鏡B(\)のとき
	else{
		for(int i:{1,3}){
			if(type[i]==1 && type[(i+1)%4]==2){
				int lantColor=Lanterns[param[i]].color;
				int nowCrys=param[(i+1)%4];
				CrysLantInteraction(nowCrys,lantColor,'+');
			}    
			if(type[(i+1)%4]==1 && type[i]==2){
				int lantColor=Lanterns[param[(i+1)%4]].color;
				int nowCrys=param[i];
				CrysLantInteraction(nowCrys,lantColor,'+');
			}    
		}
	}
	return;
}
//鏡を取り除く関数
inline void RemoveMirror(int nowMir){
	int X=Mirrors[nowMir].point.x,Y=Mirrors[nowMir].point.y;
	int mirrorType=Mirrors[nowMir].type;
	//4方向を探索する
	pair<array<int,4>,array<int,4>> SF=SearchFour(X,Y);
	array<int,4> type=SF.first,param=SF.second;
	//障害物としての処理
	for(int k=0;k<4;k++){
		//k方向にランタンかつ(k+2)%4方向にクリスタルがあるとき
		if(type[k]==1 && type[(k+2)%4]==2){
			int lantColor=Lanterns[param[k]].color;
			int nowCrys=param[(k+2)%4];
			CrysLantInteraction(nowCrys,lantColor,'+');
		}
	}
	//鏡としての処理
	//鏡A(/)のとき
	if(mirrorType==0){
		//ランタンの光がクリスタルに入るとき
		for(int i:{0,2}){
			if(type[i]==1 && type[i+1]==2){
				int lantColor=Lanterns[param[i]].color;
				int nowCrys=param[i+1];
				CrysLantInteraction(nowCrys,lantColor,'-');
			}    
			if(type[i+1]==1 && type[i]==2){
				int lantColor=Lanterns[param[i+1]].color;
				int nowCrys=param[i];
				CrysLantInteraction(nowCrys,lantColor,'-');
			}
		}
	}
	//鏡B(\)のとき
	else{
		//ランタンの光がクリスタルに入るとき
		for(int i:{1,3}){
			if(type[i]==1 && type[(i+1)%4]==2){
				int lantColor=Lanterns[param[i]].color;
				int nowCrys=param[(i+1)%4];
				CrysLantInteraction(nowCrys,lantColor,'-');
			}    
			if(type[(i+1)%4]==1 && type[i]==2){
				int lantColor=Lanterns[param[(i+1)%4]].color;
				int nowCrys=param[i];
				CrysLantInteraction(nowCrys,lantColor,'-');
			}    
		}
	}
	//鏡を取り除く
	swap(mirrorBoard[Mirrors[nowMir].point.y][Mirrors[nowMir].point.x],mirrorBoard[Mirrors.back().point.y][Mirrors.back().point.x]);
	swap(Mirrors[nowMir],Mirrors.back());
	mirrorBoard[Mirrors.back().point.y][Mirrors.back().point.x]=-1;
	Mirrors.pop_back();
	return;
}
//時間を測定する関数
const double cycles_per_sec=2800000;
inline double get_time(){
	uint32_t lo,hi;
	asm volatile("rdtsc":"=a"(lo),"=d"(hi));
	return (((uint64_t)hi<<32)|lo)/cycles_per_sec;
}
class CrystalLighting {
public:
	vector<string> placeItems(vector<string> targetBoard, int costLantern, int costMirror, int costObstacle, int maxMirrors, int maxObstacles) {
		const double start_time=get_time();
		int randTimes=0;
		//焼きなましに関する変数群
		//焼きなます時間
		double timeOver=9850.;
		//最初の温度
		double startTemp=50;
		//最後の温度
		double endTemp=1;
		//温度と確率
		double temp,probability;
		H=targetBoard.size();
		W=targetBoard[0].size();        
		//全部の盤面を-1で埋める
		for(int i=0;i<SIZE;i++){
			for(int j=0;j<SIZE;j++){
				wallBoard[i][j]=-1;
				crystalBoard[i][j]=-1;
				lanternBoard[i][j]=-1;
				obstacleBoard[i][j]=-1;
				mirrorBoard[i][j]=-1;
			}
		}
		//lessCrysInvとmuchCrysInvを-1で初期化する
		for(int i=0;i<10050;i++){
			lessCrysInv[i]=-1;
			muchCrysInv[i]=-1;
		}
		//与えられた盤面を上から見て、CrystalかObstacleがあれば盤面に加える
		for(int y=0;y<H;y++){
			for(int x=0;x<W;x++){
				//壁を見つけたとき
				if(targetBoard[y][x]=='X'){
					//wallBoardに加える
					wallBoard[y][x]=1;
				}
				//crystalを見つけたとき
				else if(targetBoard[y][x]!='.'){
					//Crystalsに加える
					Crystals.push_back(Crystal{Point{x,y},targetBoard[y][x]-'0',{0,0,0}});
					//crystalBoardに番号を入れる
					crystalBoard[y][x]=Crystals.size()-1;
					//すべてlessCrysに加える
					lessCrys.push_back(Crystals.size()-1);
					//lessCrysInvに加える
					lessCrysInv[Crystals.size()-1]=lessCrys.size()-1;
				}
			}
		}
		while(now_time<timeOver){
			
			if(now_time<5000)
				lantPun=60.*(5000.-now_time)/5000.;
			else
				lantPun=1e9;
			
			now_time=get_time()-start_time;
			//温度を決める
			temp=startTemp+(endTemp-startTemp)*now_time/timeOver;
			//遷移するかどうかを決める変数
			double R=(double)((int)randxor()%10000)/10000;
			//操作をランダムで決める
			//障害物などに関する操作
			int OP=((int)randxor())%25;
			//0:ランタンを置く 1:ランタンを取り除く
			int op=((int)randxor())%2;
			//障害物を置くとき
			if(OP==0){
				//障害物の数がmaxのとき、終了
				if(Obstacles.size()==maxObstacles)
					continue;
				//障害物を置く座標を決める
				Point P=ObstaclePutPoint();
				int X=P.x,Y=P.y;
				//その座標に既になんかあったら終了
				if(is_occupied(X,Y))
					continue;
				//その座標に障害物を置いたときにどれだけスコアが変わるかを調べる
				int deltaScore=PutObstacleDelta(X,Y,costObstacle);
				//焼きなまし
				probability=exp(deltaScore/temp);
				if(probability>R)
					PutObstacle(X,Y);
			}
			//障害物を取り除くとき
			else if(OP==1){
				//障害物の数が0のとき、終了
				if(Obstacles.size()==0)
					continue;
				//障害物をランダムに選ぶ
				int nowObs=(int)randxor()%Obstacles.size();
				//選んだ障害物を取り除いたときにどれだけスコアが変わるか調べる
				int deltaScore=RemoveObstacleDelta(nowObs,costObstacle);
				//焼きなまし
				probability=exp(deltaScore/temp);
				if(probability>R)
					RemoveObstacle(nowObs);
			}
			//鏡を置くとき
			else if(OP==2){
				//鏡の数がMaxのとき、終了
				if(Mirrors.size()==maxMirrors)
					continue;
				//鏡を置く座標を決める（障害物のものを使える）
				Point P=ObstaclePutPoint();
				int X=P.x,Y=P.y;
				//鏡のタイプを決める
				int mirrorType=(int)randxor()%2;
				//その座標に既になんかあったら終了
				if(is_occupied(X,Y))
					continue;
				//その座標に鏡を置いたときにどれだけスコアが変わるかを見る
				double deltaScore=PutMirrorDelta(X,Y,mirrorType,costMirror);
				//焼きなまし
				probability=exp(deltaScore/temp);
				if(probability>R)
					PutMirror(X,Y,mirrorType);
			}
			//鏡を取り除くとき
			else if(OP==3){
				//鏡の数が0のとき、終了
				if(Mirrors.size()==0)
					continue;
				//取り除く鏡を決める
				int nowMir=(int)randxor()%Mirrors.size();
				//選んだ鏡を取り除いたときにどれだけスコアが変わるかを見る
				double deltaScore=RemoveMirrorDelta(nowMir,costMirror);
				//焼きなまし
				probability=exp(deltaScore/temp);
				if(probability>R)
					RemoveMirror(nowMir);
			}
			//障害物を鏡に変えるとき
			else if(/*now_time>8000. &&*/ OP==4){
				//障害物の数が0のとき、終了
				if(Obstacles.size()==0)
					continue;
				//鏡の数がmaxのとき、終了
				if(Mirrors.size()==maxMirrors)
					continue;
				//障害物をランダムに選ぶ
				int nowObs=(int)randxor()%Obstacles.size();
				//ランダムでMirrorTypeを決める
				int mirrorType=(int)randxor()%2;
				//選んだ障害物を鏡に変えたときにどれだけスコアが変わるか調べる
				int deltaScore=ObsToMirDelta(nowObs,mirrorType,costObstacle,costMirror);
				//焼きなまし
				probability=exp(deltaScore/temp);
				if(probability>R)
					ObsToMir(nowObs,mirrorType);
			}
			//鏡を障害物に変えるとき
			else if(/*now_time>8000. &&*/ OP==5){
				//鏡の数が0のとき、終了
				if(Mirrors.size()==0)
					continue;
				//障害物の数がmaxのとき、終了
				if(Obstacles.size()==maxObstacles)
					continue;
				//鏡をランダムに選ぶ
				int nowMir=(int)randxor()%Mirrors.size();
				//選んだ鏡を障害物にしたときにどれだけスコアが変わるか見る
				int deltaScore=MirToObsDelta(nowMir,costObstacle,costMirror);
				//焼きなまし
				probability=exp(deltaScore/temp);
				if(probability>R)
					MirToObs(nowMir);
			}

			//ランタンを置くとき
			if(op==0){
				//ランタンを置く座標と色を決める
				pair<Point,int> LPP=LanternPutPoint();
				int X=LPP.first.x,Y=LPP.first.y,color=LPP.second;
				//その座標に既になんかあったら終了;
				if(is_occupied(X,Y))
					continue;
				//その座標にランタンを置いたときにどれだけスコアが変わるか調べる
				tuple<int,vector<int>,vector<pair<int,int>>> PLD=PutLanternDelta(X,Y,color,costLantern);
				//焼きなまし
				probability=exp(get<0>(PLD)/temp);
				if(probability>R)
					PutLantern(X,Y,color,get<1>(PLD),get<2>(PLD));
			}
			//ランタンを取り除くとき
			else if(op==1){
				//ランタンがないとき、終了
				if(Lanterns.size()==0)
					continue;
				//ランタンをランダムに選ぶ
				int nowLant=(int)randxor()%Lanterns.size();
				//選んだランタンを取り除いたときにどれだけスコアが変わるか調べる
				tuple<int,vector<int>,vector<pair<int,int>>> RLD=RemoveLanternDelta(nowLant,costLantern);
				//実際に遷移する
				//焼きなまし
				probability=exp(get<0>(RLD)/temp);
				if(probability>R){
					RemoveLantern(nowLant,get<1>(RLD),get<2>(RLD));
				}
			}
		}

		//全マス見て、障害物を置ける場所があれば置く
		for(int Y=0;Y<H;Y++){
			for(int X=0;X<W;X++){
				//障害物の数がmaxのとき、終了
				if(Obstacles.size()==maxObstacles)
					continue;
				//その座標に既になんかあったら終了
				if(is_occupied(X,Y))
					continue;
				//その座標に障害物を置いたときにどれだけスコアが変わるかを調べる
				int deltaScore=PutObstacleDelta(X,Y,costObstacle);
				if(deltaScore>0)
					PutObstacle(X,Y);
			}
		}
		//全部の障害物を見て、鏡に変えられる場所があったら変える
		for(int nowObs=0;nowObs<Obstacles.size();nowObs++){
			//鏡の数がmaxのとき、終了
			if(Mirrors.size()==maxMirrors)
				continue;
			//選んだ障害物を鏡に変えたときにどれだけスコアが変わるか調べる
			for(int mirrorType:{0,1}){
				int deltaScore=ObsToMirDelta(nowObs,mirrorType,costObstacle,costMirror);
				if(deltaScore>0){
					ObsToMir(nowObs,mirrorType);
					break;
				}
			}
		}
		//全マス見て、障害物を置ける場所があれば置く
		for(int Y=0;Y<H;Y++){
			for(int X=0;X<W;X++){
				//障害物の数がmaxのとき、終了
				if(Obstacles.size()==maxObstacles)
					continue;
				//その座標に既になんかあったら終了
				if(is_occupied(X,Y))
					continue;
				//その座標に障害物を置いたときにどれだけスコアが変わるかを調べる
				int deltaScore=PutObstacleDelta(X,Y,costObstacle);
				if(deltaScore>0)
					PutObstacle(X,Y);
			}
		}
		//全マス見て、ランタンを入れたほうがいい場所があったら入れる
		for(int Y=0;Y<H;Y++){
			for(int X=0;X<W;X++){
				//その座標に既になんかあったら終了;
				if(is_occupied(X,Y))
					continue;
				for(int color:{1,2,4}){
					//その座標にランタンを置いたときにどれだけスコアが変わるか調べる
					tuple<int,vector<int>,vector<pair<int,int>>> PLD=PutLanternDelta(X,Y,color,costLantern);
					//焼きなまし
					probability=exp(get<0>(PLD)/temp);
					if(get<0>(PLD)>0){
						PutLantern(X,Y,color,get<1>(PLD),get<2>(PLD));
						break;
					}
				}
			}
		}
		//全部のランタンを見て、取り除いたほうがいいものがあったら取り除く
		for(int nowLant=0;nowLant<Lanterns.size();nowLant++){
			//選んだランタンを取り除いたときにどれだけスコアが変わるか調べる
			tuple<int,vector<int>,vector<pair<int,int>>> RLD=RemoveLanternDelta(nowLant,costLantern);
			//実際に遷移する
			if(get<0>(RLD)>0){
				RemoveLantern(nowLant,get<1>(RLD),get<2>(RLD));
			}
		}
		/*
		outputfile<<"lessCrys"<<endl;
		for(int i:lessCrys){
			outputfile<<i<<" ";
		}
		outputfile<<endl;
		outputfile<<"muchCrys"<<endl;
		for(int i:muchCrys){
			outputfile<<i<<" ";
		}
		outputfile.close();
*/
		//出力
		vector<string> ret;
		//Lanterns
		for(Lantern L:Lanterns){
			ostringstream ost;
			ost<<L.point.y<<" "<<L.point.x<<" "<<L.color;
			ret.push_back(ost.str());
		}
		//Obstacles
		for(Obstacle O:Obstacles){
			ostringstream ost;
			ost<<O.point.y<<" "<<O.point.x<<" "<<"X";
			ret.push_back(ost.str());
		}
		//Mirrors
		for(Mirror M:Mirrors){
			ostringstream ost;
			ost<<M.point.y<<" "<<M.point.x<<" "<<(M.type==0?'/':'\\');
			ret.push_back(ost.str());
		}
		return ret;
	}
};
// -------8<------- end of solution submitted to the website -------8<-------

template<class T> void getVector(vector<T>& v) {
	for (int i = 0; i < v.size(); ++i)
		cin >> v[i];
}

int main() {
	CrystalLighting cl;
	int H;
	cin >> H;
	vector<string> targetBoard(H);
	getVector(targetBoard);
	int costLantern, costMirror, costObstacle, maxMirrors, maxObstacles;
	cin >> costLantern >> costMirror >> costObstacle >> maxMirrors >> maxObstacles;

	vector<string> ret = cl.placeItems(targetBoard, costLantern, costMirror, costObstacle, maxMirrors, maxObstacles);
	cout << ret.size() << endl;
	for (int i = 0; i < (int)ret.size(); ++i)
		cout << ret[i] << endl;
	cout.flush();
}