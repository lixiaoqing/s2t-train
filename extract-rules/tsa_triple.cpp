#include "tsa_triple.h"

TSATriple::TSATriple(const string &line_tree,const string &line_str,const string &line_align)
{
	load_alignment(line_align);
	src_tree = new SyntaxTree(line_tree,&src_idx_to_tgt_span,&tgt_idx_to_src_idx);
	tgt_words = Split(line_str);
	tgt_sen_len = tgt_words.size();
}

void TSATriple::load_alignment(const string &line_align)
{
	src_idx_to_tgt_span.resize(1000,make_pair(-1,-1));  //TODO 句子长度不安全
	tgt_idx_to_src_span.resize(1000,make_pair(-1,-1));
	src_idx_to_tgt_idx.resize(1000);
	tgt_idx_to_src_idx.resize(1000);
	vector<string> alignments = Split(line_align);
	for (auto align : alignments)
	{
		vector<string> idx_pair = Split(align,"-");
		int src_idx = stoi(idx_pair.at(0));
		int tgt_idx = stoi(idx_pair.at(1));
		if (src_idx_to_tgt_span.at(src_idx).first == -1 || src_idx_to_tgt_span.at(src_idx).first > tgt_idx)
		{
			src_idx_to_tgt_span.at(src_idx).first = tgt_idx;
		}
		if (src_idx_to_tgt_span.at(src_idx).second == -1 || src_idx_to_tgt_span.at(src_idx).second < tgt_idx)
		{
			src_idx_to_tgt_span.at(src_idx).second = tgt_idx;
		}
		if (tgt_idx_to_src_span.at(tgt_idx).first == -1 || tgt_idx_to_src_span.at(tgt_idx).first > src_idx)
		{
			tgt_idx_to_src_span.at(tgt_idx).first = src_idx;
		}
		if (tgt_idx_to_src_span.at(tgt_idx).second == -1 || tgt_idx_to_src_span.at(tgt_idx).second < src_idx)
		{
			tgt_idx_to_src_span.at(tgt_idx).second = src_idx;
		}
		src_idx_to_tgt_idx.at(src_idx).push_back(tgt_idx);
		tgt_idx_to_src_idx.at(tgt_idx).push_back(src_idx);
	}
}

void TSATriple::extract_rules()
{
	extract_GHKM_rules(src_tree->root);
	attach_unaligned_words();
	extract_SPMT_rules();
	src_tree->dump(src_tree->root);
}

/**************************************************************************************
 1. 函数功能: 抽取当前子树包含的最小规则
 2. 入口参数: 当前子树的根节点
 3. 出口参数: 无
 4. 算法简介: 先序遍历当前子树，抽取每个每个节点的最小规则
************************************************************************************* */
void TSATriple::extract_GHKM_rules(SyntaxNode* node)
{
	if (node->type == 1) 																// 当前节点为边界节点
	{
		string src_tree_frag = node->label+"( ";
		vector<int> tgt_word_status(tgt_sen_len,-1);                               	    // 记录每个目标端单词的状态，i表示被源端第i个变量替换，-1表示没被替换
		int variable_num = -1;
		for (const auto child : node->children) 										// 对当前节点的每个孩子节点进行扩展，直到遇到边界节点或单词节点为止
		{
			find_frontier_frag(child,src_tree_frag,tgt_word_status,variable_num);
		}
		src_tree_frag += ")";
		Rule rule;
		rule.type = 1;
		rule.src_side = src_tree_frag;
		int tgt_idx=node->tgt_span.first; 												// 遍历当前节点tgt_span的每个单词，根据tgt_word_status生成规则目标端
		while (tgt_idx<=node->tgt_span.second)
		{
			if (tgt_word_status.at(tgt_idx) == -1)
			{
				rule.tgt_side += tgt_words.at(tgt_idx)+" ";
				tgt_idx++;
			}
			else
			{
				variable_num = tgt_word_status.at(tgt_idx);
				rule.tgt_side += "x"+to_string(variable_num)+" ";
				while(tgt_idx<tgt_sen_len && tgt_word_status.at(tgt_idx) == variable_num)
				{
					tgt_idx++; 															// 这些单词被同一个变量替换
				}
			}
		}
		node->rules.push_back(rule);
	}
	for (const auto child : node->children)
	{
		extract_GHKM_rules(child);
	}
}

/**************************************************************************************
 1. 函数功能: 寻找当前节点的最小规则片段，并更新目标端单词状态
 2. 入口参数: 当前子树的根节点
 3. 出口参数: 源端规则片段，目标端单词状态，变量个数
 4. 算法简介: 如果当前节点为单词节点或者边界节点，则停止扩展，否则对子树进行先序遍历
************************************************************************************* */
void TSATriple::find_frontier_frag(const SyntaxNode* node,string &src_tree_frag,vector<int> &tgt_word_status,int &variable_num)
{
	if (node->type == 0) 																//单词节点
	{
		src_tree_frag += "( " + node->label + " ) ";
	}
	else if (node->type == 1)										                    //边界节点
	{
		variable_num++;
		src_tree_frag += node->label  + ":x" + to_string(variable_num) + " ";
		for (int tgt_idx=node->tgt_span.first;tgt_idx<=node->tgt_span.second;tgt_idx++)
		{
			tgt_word_status.at(tgt_idx) = variable_num; 								//根据当前节点的tgt_span更新目标端单词的状态
		}
	}
	else if (node->type == 2)              											    //非边界节点
	{
		src_tree_frag += "( " + node->label;
		for (const auto child : node->children)
		{
			find_frontier_frag(child,src_tree_frag,tgt_word_status,variable_num);
		}
		src_tree_frag +=  ") ";
	}
}

/**************************************************************************************
 1. 函数功能: 将目标端未对齐的单词向左（右）依附到最小规则上，形成新的规则
 2. 入口参数: 无
 3. 出口参数: 无
 4. 算法简介: 见注释
************************************************************************************* */
void TSATriple::attach_unaligned_words()
{
	for (int tgt_idx=0;tgt_idx<tgt_sen_len;tgt_idx++)
	{
		if (tgt_idx_to_src_idx.at(tgt_idx).size() > 0)       						//跳过有对齐的单词
			continue;
		int i;
		for (i=tgt_idx-1;i>=0;i--)                         			    			//向左扫描，直到遇到有对齐的单词
		{
			if (tgt_idx_to_src_idx.at(i).size() > 0)
				break;
		}
		if (i>=0) 											 						//左边有单词有对齐
		{
			for (auto node : src_tree->tgt_span_rbound_to_frontier_nodes.at(i))     //找到能依附的所有规则
			{
				assert(node->rules.size()>0);
				Rule rule = node->rules.front();
				rule.type = 2;
				for (int j=i+1;j<=tgt_idx;j++)
				{
					rule.tgt_side += " " + tgt_words.at(j);
				}
				node->rules.push_back(rule);
			}
		}

		for (i=tgt_idx+1;i<tgt_sen_len;i++)                        					//向右扫描，直到遇到有对齐的单词
		{
			if (tgt_idx_to_src_idx.at(i).size() > 0)
				break;
		}
		if (i<tgt_sen_len) 															//右边有单词有对齐
		{
			for (auto node : src_tree->tgt_span_lbound_to_frontier_nodes.at(i))     //找到能依附的所有规则
			{
				Rule rule = node->rules.front();
				rule.type = 2;
				for (int j=i-1;j>=tgt_idx;j--)
				{
					rule.tgt_side = tgt_words.at(j) + " " + rule.tgt_side;
				}
				node->rules.push_back(rule);
			}
		}
	}
}

void TSATriple::extract_SPMT_rules()
{
	for (int len=0;len<tgt_sen_len && len<MAX_PHRASE_LEN;len++)				     //遍历目标端所有的短语
	{
		for (int beg=0;beg+len<tgt_sen_len;beg++)
		{
			pair<int,int> tgt_span = make_pair(beg,beg+len);
			pair<int,int> src_span = cal_src_span_for_tgt_span(tgt_span); 	      //计算目标端短语对应的源端span
			if (src_span.first == -1)										      //目标短语内的单词全都对空
				continue;
			bool flag = check_alignment_for_src_span(src_span,tgt_span);	      //检查源端span中的单词是否对到了目标端span的外面
			if (flag == false)
				continue;
			//寻找能覆盖源端span的最低句法节点
			SyntaxNode *node = src_tree->word_nodes.at(src_span.first)->father;   //从源端span最左端单词的词性节点往上找
			while (node->src_span.second < src_span.second) 					  //当前节点无法覆盖源端span右边界（左边界肯定能被覆盖）
			{
				node = node->father;
			}
			if (node->type != 1)                								  //找到的根节点不是边界节点
				continue;
			//先序遍历以当前节点为根节点的子树，找出规则源端
			string src_tree_frag = node->label+"( ";
			vector<int> tgt_word_status(tgt_sen_len,-1);                         // 记录每个目标端单词的状态，i表示被源端第i个变量替换，-1表示没被替换
			int variable_num = -1;
			for (const auto child : node->children) 							 // 对当前节点的每个孩子节点进行扩展，直到遇到源端span以外的边界节点或单词节点
			{
				flag = find_syntax_phrase_frag(child,src_tree_frag,tgt_word_status,variable_num,src_span);
				if (flag == false)
					break;
			}
			if (flag == false)
				continue;
			src_tree_frag += ")";
			Rule rule;
			rule.type = 3;
			rule.src_side = src_tree_frag;
			int lbound = min(tgt_span.first,node->tgt_span.first);              // 当前规则的左右边界
			int rbound = max(tgt_span.second,node->tgt_span.second);
			int tgt_idx=lbound; 									            // 遍历当前规则目标端span的每个单词，根据tgt_word_status生成规则目标端
			while (tgt_idx<=rbound)
			{
				if (tgt_word_status.at(tgt_idx) == -1)
				{
					if (tgt_idx>=tgt_span.first || tgt_idx<=tgt_span.second)    //跳过目标端span以外的未对齐的词
					{
						rule.tgt_side += tgt_words.at(tgt_idx)+" ";
					}
					tgt_idx++;
				}
				else
				{
					variable_num = tgt_word_status.at(tgt_idx);
					rule.tgt_side += "x"+to_string(variable_num)+" ";
					while(tgt_idx<tgt_sen_len && tgt_word_status.at(tgt_idx) == variable_num)
					{
						tgt_idx++; 												 // 这些单词被同一个变量替换
					}
				}
			}
			node->rules.push_back(rule);
		}
	}
}

/**************************************************************************************
 1. 函数功能: 寻找当前节点的句法短语规则片段，并更新目标端单词状态
 2. 入口参数: 当前子树的根节点
 3. 出口参数: 成功标识，源端规则片段，目标端单词状态，变量个数
 4. 算法简介: 如果当前节点为超过源端span的边界节点，则停止扩展，否则对子树进行先序遍历
 			  如果超过源端span的节点为非边界节点，则返回错误
************************************************************************************* */
bool TSATriple::find_syntax_phrase_frag(const SyntaxNode* node,string &src_tree_frag,vector<int> &tgt_word_status,int &variable_num,pair<int,int> src_span)
{
	if (node->type == 0) 																//单词节点
	{
		src_tree_frag += "( " + node->label + " ) ";
	}
	else if (node->type == 1 && (node->src_span.first > src_span.second || node->src_span.second < src_span.first))      //超过了源端span的边界节点
	{
		variable_num++;
		src_tree_frag += node->label  + ":x" + to_string(variable_num) + " ";
		for (int tgt_idx=node->tgt_span.first;tgt_idx<=node->tgt_span.second;tgt_idx++)
		{
			tgt_word_status.at(tgt_idx) = variable_num; 							    //根据当前节点的tgt_span更新目标端单词的状态
		}
	}
	else 																				//非边界节点或源端span内的边界节点
	{
		if (node->type == 2 && (node->src_span.first > src_span.second || node->src_span.second < src_span.first))       //超过了源端span的非边界节点
			return false;       //TODO 或许应该继续扩展
		src_tree_frag += "( " + node->label;
		for (const auto child : node->children)
		{
			bool flag = find_syntax_phrase_frag(child,src_tree_frag,tgt_word_status,variable_num,src_span);
			if (flag == false)
				return false;
		}
		src_tree_frag +=  ") ";
	}
	return true;
}

pair<int,int> TSATriple::cal_src_span_for_tgt_span(pair<int,int> tgt_span)
{
	int src_lbound = -1;
	int src_rbound = -1;
	for (int i=tgt_span.first;i<=tgt_span.second;i++)
	{
		if (tgt_idx_to_src_span.at(i).first == -1)
			continue;
		if (src_lbound == -1 || src_lbound > tgt_idx_to_src_span.at(i).first)
		{
			src_lbound = tgt_idx_to_src_span.at(i).first;
		}
		if (src_rbound == -1 || src_rbound < tgt_idx_to_src_span.at(i).second)
		{
			src_rbound = tgt_idx_to_src_span.at(i).second;
		}
	}
	return make_pair(src_lbound,src_rbound);
}

bool TSATriple::check_alignment_for_src_span(pair<int,int> src_span,pair<int,int> tgt_span)
{
	for (int j=src_span.first;j<=src_span.second;j++)
	{
		for (int tgt_idx : src_idx_to_tgt_idx.at(j))
		{
			if (tgt_idx < tgt_span.first || tgt_idx > tgt_span.second)
				return false;
		}
	}
	return true;
}

int main()
{
	ifstream ft("en.parse");
	ifstream fs("ch");
	ifstream fa("al");
	string line_tree,line_str,line_align;
	while(getline(ft,line_tree))
	{
		getline(fs,line_str);
		getline(fa,line_align);
		TSATriple tsa_triple(line_tree,line_str,line_align);
		tsa_triple.extract_rules();
	}
}
