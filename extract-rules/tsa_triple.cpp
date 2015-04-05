#include "tsa_triple.h"

TSATriple::TSATriple(const string &line_tree,const string &line_str,const string &line_align)
{
	load_alignment(line_align);
	src_tree = new SyntaxTree(line_tree,&src_idx_to_tgt_span,&tgt_idx_to_src_idx);
	tgt_words = Split(line_str);
	extract_GHKM_rules(src_tree->root);
	attach_unaligned_words();
	src_tree->dump(src_tree->root);
}

void TSATriple::load_alignment(const string &line_align)
{
	src_idx_to_tgt_span.resize(1000,make_pair(-1,-1));  //TODO 句子长度不安全
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
		tgt_idx_to_src_idx.at(tgt_idx).push_back(src_idx);
	}
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
		vector<int> tgt_word_status(tgt_words.size(),-1);                               // 记录每个目标端单词的状态，i表示被源端第i个变量替换，-1表示没被替换
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
				while(tgt_idx<tgt_words.size() && tgt_word_status.at(tgt_idx) == variable_num)
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
	if (node->type == 1) 											                    //边界节点
	{
		variable_num++;
		src_tree_frag += node->label  + ":x" + to_string(variable_num) + " ";
		for (int tgt_idx=node->tgt_span.first;tgt_idx<=node->tgt_span.second;tgt_idx++)
		{
			tgt_word_status.at(tgt_idx) = variable_num; 								//根据当前节点的tgt_span更新目标端单词的状态
		}
	}
	if (node->type == 2)                 											    //非边界节点
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
	for (int tgt_idx=0;tgt_idx<tgt_words.size();tgt_idx++)
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
				if (node->rules.size()==0)
				{
					cout<<node->label<<' '<<node->tgt_span.first<<' '<<node->tgt_span.second<<endl;
				}
				Rule rule = node->rules.front();
				rule.type = 2;
				for (int j=i+1;j<=tgt_idx;j++)
				{
					rule.tgt_side += " " + tgt_words.at(j);
				}
				node->rules.push_back(rule);
			}
		}

		for (i=tgt_idx+1;i<tgt_words.size();i++)                        			//向右扫描，直到遇到有对齐的单词
		{
			if (tgt_idx_to_src_idx.at(i).size() > 0)
				break;
		}
		if (i<tgt_words.size()) 													//右边有单词有对齐
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
	}
}
