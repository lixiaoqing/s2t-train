#ifndef STDAFX_H
#define STDAFX_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <set>
#include <vector>
#include <map>
#include <unordered_map>

#include <algorithm>
#include <bitset>
#include <queue>
#include <functional>
#include <limits>


#include <zlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <omp.h>


using namespace std;

const int MAX_SPMT_PHRASE_LEN = 5;		// 抽取SPMT规则时最大短语长度
const int MAX_LHS_NODE_NUM = 15;		// 规则左端最大节点数
const int MAX_RHS_WORD_NUM = 10;		// 规则右端最大单词数
const int MAX_RULE_SIZE = 4;			// 规则最多有几个更小的规则组成

#endif
