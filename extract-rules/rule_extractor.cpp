#include "rule_extractor.h"

RuleExtractor::RuleExtractor(const string &line_tree,const string &line_str,const string &line_align)
{
	tspair = new TreeStrPair(line_tree,line_str,line_align);
}

void RuleExtractor::extract_rules()
{
	extract_GHKM_rules(tspair->root);
	attach_unaligned_words();
	extract_SPMT_rules();
	tspair->dump_rule(tspair->root);
}

/**************************************************************************************
 1. 函数功能: 抽取当前子树包含的最小规则
 2. 入口参数: 当前子树的根节点
 3. 出口参数: 无
 4. 算法简介: 先序遍历当前子树，抽取每个每个节点的最小规则
************************************************************************************* */
void RuleExtractor::extract_GHKM_rules(SyntaxNode* node)
{
	if (node->type == 1) 																// 当前节点为边界节点
	{
		Rule rule;
		rule.type = 1;
		rule.src_tree_frag.push_back(node);
		rule.src_node_status.push_back(-1);
		rule.tgt_span = node->tgt_span;
		rule.tgt_word_status.resize(tspair->tgt_sen_len,-1);
		rule.variable_num = 0;
		for (const auto child : node->children) 										// 对当前节点的每个孩子节点进行扩展，直到遇到边界节点或单词节点为止
		{
			find_frontier_frag(child,rule);
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
void RuleExtractor::find_frontier_frag(SyntaxNode* node,Rule &rule)
{
	if (node->type == 0) 																//单词节点
	{
		rule.src_tree_frag.push_back(node);
		rule.src_node_status.push_back(-3);
	}
	else if (node->type == 1)										                    //边界节点
	{
		rule.src_tree_frag.push_back(node);
		rule.src_node_status.push_back(rule.variable_num);
		rule.variable_num++;
		for (int tgt_idx=node->tgt_span.first;tgt_idx<=node->tgt_span.second;tgt_idx++)
		{
			rule.tgt_word_status.at(tgt_idx) = rule.variable_num;						//根据当前节点的tgt_span更新目标端单词的状态
		}
	}
	else if (node->type == 2)              											    //非边界节点
	{
		rule.src_tree_frag.push_back(node);
		rule.src_node_status.push_back(-2);
		for (const auto child : node->children)
		{
			find_frontier_frag(child,rule);
		}
	}
}

/**************************************************************************************
 1. 函数功能: 将目标端未对齐的单词向左（右）依附到最小规则上，形成新的规则
 2. 入口参数: 无
 3. 出口参数: 无
 4. 算法简介: 见注释
************************************************************************************* */
void RuleExtractor::attach_unaligned_words()
{
	for (int tgt_idx=0;tgt_idx<tspair->tgt_sen_len;tgt_idx++)
	{
		if (tspair->tgt_idx_to_src_idx.at(tgt_idx).size() > 0)       						//跳过有对齐的单词
			continue;
		int i;
		for (i=tgt_idx-1;i>=0;i--)                         			    			//向左扫描，直到遇到有对齐的单词
		{
			if (tspair->tgt_idx_to_src_idx.at(i).size() > 0)
				break;
		}
		if (i>=0) 											 						//左边有单词有对齐
		{
			for (auto node : tspair->tgt_span_rbound_to_frontier_nodes.at(i))     //找到能依附的所有规则
			{
				assert(node->rules.size()>0);
				Rule rule = node->rules.front();
				rule.type = 2;
				rule.tgt_span.second = tgt_idx;
				node->rules.push_back(rule);
			}
		}

		for (i=tgt_idx+1;i<tspair->tgt_sen_len;i++)                        					//向右扫描，直到遇到有对齐的单词
		{
			if (tspair->tgt_idx_to_src_idx.at(i).size() > 0)
				break;
		}
		if (i<tspair->tgt_sen_len) 															//右边有单词有对齐
		{
			for (auto node : tspair->tgt_span_lbound_to_frontier_nodes.at(i))     //找到能依附的所有规则
			{
				Rule rule = node->rules.front();
				rule.type = 2;
				rule.tgt_span.first = tgt_idx;
				node->rules.push_back(rule);
			}
		}
	}
}

void RuleExtractor::extract_SPMT_rules()
{
	for (int len=0;len<tspair->tgt_sen_len && len<MAX_PHRASE_LEN;len++)				     //遍历目标端所有的短语
	{
		for (int beg=0;beg+len<tspair->tgt_sen_len;beg++)
		{
			pair<int,int> tgt_span = make_pair(beg,beg+len);
			pair<int,int> src_span = cal_src_span_for_tgt_span(tgt_span); 	      //计算目标端短语对应的源端span
			if (src_span.first == -1)										      //目标短语内的单词全都对空
				continue;
			bool flag = check_alignment_for_src_span(src_span,tgt_span);	      //检查源端span中的单词是否对到了目标端span的外面
			if (flag == false)
				continue;
			//寻找能覆盖源端span的最低句法节点
			SyntaxNode *node = tspair->word_nodes.at(src_span.first)->father;   //从源端span最左端单词的词性节点往上找
			while (node->src_span.second < src_span.second) 					  //当前节点无法覆盖源端span右边界（左边界肯定能被覆盖）
			{
				node = node->father;
			}
			if (node->type != 1)                								  //找到的根节点不是边界节点
				continue;
			//先序遍历以当前节点为根节点的子树，找出规则源端
		Rule rule;
		rule.type = 3;
		rule.src_tree_frag.push_back(node);
		rule.src_node_status.push_back(-1);
		rule.tgt_span = tgt_span;
		rule.tgt_word_status.resize(tspair->tgt_sen_len,-1);
		rule.variable_num = 0;
			for (const auto child : node->children) 							 // 对当前节点的每个孩子节点进行扩展，直到遇到源端span以外的边界节点或单词节点
			{
				flag = find_syntax_phrase_frag(child,rule,src_span);
				if (flag == false)
					break;
			}
			if (flag == false)
				continue;
			int lbound = min(tgt_span.first,node->tgt_span.first);              // 当前规则的左右边界
			int rbound = max(tgt_span.second,node->tgt_span.second);
			rule.tgt_span = make_pair(lbound,rbound);
			for (int tgt_idx=lbound;tgt_idx<=rbound;tgt_idx++)
			{
				if (rule.tgt_word_status.at(tgt_idx) == -1 && (tgt_idx<tgt_span.first || tgt_idx>tgt_span.second))
				{
					rule.tgt_word_status.at(tgt_idx) = -2;						//跳过目标端span以外的未对齐的词
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
bool RuleExtractor::find_syntax_phrase_frag(SyntaxNode* node,Rule &rule,pair<int,int> src_span)
{
	if (node->type == 0) 																//单词节点
	{
		rule.src_tree_frag.push_back(node);
		rule.src_node_status.push_back(-3);
	}
	else if (node->type == 1 && (node->src_span.first > src_span.second || node->src_span.second < src_span.first))      //超过了源端span的边界节点
	{
		rule.src_tree_frag.push_back(node);
		rule.src_node_status.push_back(rule.variable_num);
		rule.variable_num++;
		for (int tgt_idx=node->tgt_span.first;tgt_idx<=node->tgt_span.second;tgt_idx++)
		{
			rule.tgt_word_status.at(tgt_idx) = rule.variable_num;						//根据当前节点的tgt_span更新目标端单词的状态
		}
	}
	else 																				//非边界节点或源端span内的边界节点
	{
		if (node->type == 2 && (node->src_span.first > src_span.second || node->src_span.second < src_span.first))       //超过了源端span的非边界节点
			return false;       //TODO 或许应该继续扩展
		rule.src_tree_frag.push_back(node);
		rule.src_node_status.push_back(-2);
		for (const auto child : node->children)
		{
			bool flag = find_syntax_phrase_frag(child,rule,src_span);
			if (flag == false)
				return false;
		}
	}
	return true;
}

pair<int,int> RuleExtractor::cal_src_span_for_tgt_span(pair<int,int> tgt_span)
{
	int src_lbound = -1;
	int src_rbound = -1;
	for (int i=tgt_span.first;i<=tgt_span.second;i++)
	{
		if (tspair->tgt_idx_to_src_span.at(i).first == -1)
			continue;
		if (src_lbound == -1 || src_lbound > tspair->tgt_idx_to_src_span.at(i).first)
		{
			src_lbound = tspair->tgt_idx_to_src_span.at(i).first;
		}
		if (src_rbound == -1 || src_rbound < tspair->tgt_idx_to_src_span.at(i).second)
		{
			src_rbound = tspair->tgt_idx_to_src_span.at(i).second;
		}
	}
	return make_pair(src_lbound,src_rbound);
}

bool RuleExtractor::check_alignment_for_src_span(pair<int,int> src_span,pair<int,int> tgt_span)
{
	for (int j=src_span.first;j<=src_span.second;j++)
	{
		for (int tgt_idx : tspair->src_idx_to_tgt_idx.at(j))
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
		RuleExtractor tsa_triple(line_tree,line_str,line_align);
		tsa_triple.extract_rules();
	}
}
