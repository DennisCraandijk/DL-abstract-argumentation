
#ifndef  AppearanceRank_h
#define AppearanceRank_h

#include "mtl/Vec.h"

namespace Minisat {

// 出現数の順位を把握するためのクラス
// 出現数は上位であるかどうかを気にするだけなので
// 上位n件以内であるものについてのみ完全に把握するが、
// それ以下の順位であるものに関しては全く把握しないようにしている。
// それぞれの変数について出現数は1つずつしか増えないため、
// そのように設計している
class AppearanceRank {
	vec<int> data;								// 変数がいくつ出現しているか
	vec<int> rank_to_var;							// 順位→変数
	vec<int> var_to_rank;							// 変数→順位		これがszより小さい場合上位変数。
																	// 上位変数以外の順位は全てszとなっている
	int sz;														// 上位変数のサイズ
public:
//	AppearanceRank() : data(NULL), rank_to_var(NULL) , var_to_rank (NULL), sz(0){}
	AppearanceRank() : sz(0){}

	void init(int var_num, int rank){
		sz = rank;
		data.clear();
		rank_to_var.clear();
		var_to_rank.clear();
		data.growTo(var_num,0);
		rank_to_var.growTo(rank);
		var_to_rank.growTo(var_num);

		for(int i=0;i<var_num;i++){
			if(i < rank){
				rank_to_var[i] = i;
				var_to_rank[i] = i;
			}
			else
				var_to_rank[i] = rank;
		}
	}

	bool upper(int v){
		return var_to_rank[v] < sz;
	}
	int size(){
		return sz;
	}
	void clear(){
		rank_to_var.clear();
		var_to_rank.clear();
		data.clear();
		sz=0;
	}

	// 変数vの出現数が増加
	void add(int v){
		data[v]++;

		int vrank = var_to_rank[v];		// 変数の順位
		assert(vrank <= sz);
		// 変数の順位が0位でないか、１つ上の順位の変数の出現数よりもこの変数の出現数が多いとき繰り返す
		while(vrank != 0 && data[v] > data[rank_to_var[vrank-1]])
			vrank--;
		assert(vrank >= 0);

		if(var_to_rank[v] != vrank){
			// 入れ替え先の変数についての変更		// 変数→順位の入れ替え
			if(var_to_rank[v] < sz)
				rank_to_var[var_to_rank[v]] = rank_to_var[vrank];		// vの順位を今までvrankだった変数に移す
			var_to_rank[rank_to_var[vrank]] = var_to_rank[v];		// vrank順位だった変数の順位をvの順位に移す

			// vについての変更
			var_to_rank[v] = vrank;
			rank_to_var[vrank] = v;		// vを上位ランクへ
		}
	}

	void printALL(){
		printf("Rank to Var\n");
		for(int i=0;i<sz;i++){
			printf("%d		%d	<-	%d	data:%d\n", i, rank_to_var[i], var_to_rank[rank_to_var[i]], data[rank_to_var[i]]);
		}
		printf("Var to Rank\n");
		for(int i=0;i<var_to_rank.size();i++){
			printf("%d	%d	data:%d\n", i, var_to_rank[i], data[i]);
		}
	}

private:
};

//=================================================================================================
}
#endif
