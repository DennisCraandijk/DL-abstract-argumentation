/*****************************************************************************************[Queue.h]
MiniSat -- Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
  2008 - Gilles Audemard, Laurent Simon

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

//#ifndef RecentLBDs_h
//#define RecentLBDs_h
#ifndef MovingAverage_h
#define MovingAverage_h

#include "mtl/Vec.h"

namespace Minisat {
//ターンにおけるプロパゲート数の移動平均を計算するためのクラス
class MovingAverage {
	vec<uint32_t> data;
	int first;				//データの中で一番古いもののインデックス
	uint64_t sum;		//合計
	uint32_t length;			//移動平均の長さ
	uint32_t turns;

public:
	MovingAverage() : first(0), sum(0), length(0), turns(0){}

	void init(uint32_t lg){
		length = lg;
	}

	void push(uint32_t num){
		if(data.size() < (int)length){
			data.push(num);
		}
		else{
			sum -= data[first];
			data[first++] = num;
			if(first >= (int)length) first = 0;
		}
		sum += num;
		turns++;
	}
	uint32_t turn() {return turns;}
	bool ready() {return data.size() == (int)length;}
	double average() {return (double)sum / (double)data.size();}
	void clear() {first=0; sum=0;data.clear();turns=0;}
	void clear(uint32_t lg){first=0; sum=0; data.clear();turns=0;length=lg;}
	uint32_t leng(){return length;}
};

//=================================================================================================
}
#endif
