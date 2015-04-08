#include "rule_extractor.h"

RuleExtractor::RuleExtractor(const string &line_tree,const string &line_str,const string &line_align)
{
	tspair = new TreeStrPair(line_tree,line_str,line_align);
}

void RuleExtractor::extract_rules()
{
	extract_GHKM_rules(tspair->root);
	//extract_SPMT_rules();
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
	if (node->type == 1) 													// 当前节点为边界节点
	{
		Rule rule;
		rule.type = 1;
		rule.src_tree_frag.push_back(node);
		rule.src_node_status.push_back(-1);
		rule.tgt_spans_for_src_node.push_back(node->tgt_span);
		rule.tgt_word_status.resize(tspair->tgt_sen_len,-1);
		rule.variable_num = 0;
		for (const auto child : node->children) 							// 对当前节点的每个孩子节点进行扩展，直到遇到边界节点或单词节点为止
		{
			find_frontier_frag(child,rule);
		}
		node->rules.push_back(rule);
		attach_unaligned_words(node);
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
		rule.tgt_spans_for_src_node.push_back(node->tgt_span);
	}
	else if (node->type == 1)										                    //边界节点
	{
		rule.src_tree_frag.push_back(node);
		rule.src_node_status.push_back(rule.variable_num);
		rule.tgt_spans_for_src_node.push_back(node->tgt_span);
		for (int tgt_idx=node->tgt_span.first;tgt_idx<=node->tgt_span.second;tgt_idx++)
		{
			rule.tgt_word_status.at(tgt_idx) = rule.variable_num;						//根据当前节点的tgt_span更新目标端单词的状态
		}
		rule.variable_num++;
	}
	else if (node->type == 2)              											    //非边界节点
	{
		rule.src_tree_frag.push_back(node);
		rule.src_node_status.push_back(-2);
		rule.tgt_spans_for_src_node.push_back(make_pair(-1,-1));
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
void RuleExtractor::attach_unaligned_words(SyntaxNode* node)
{
	for (int tgt_idx=0;tgt_idx<tspair->tgt_sen_len;tgt_idx++)									//遍历目标语言句子的所有单词
	{
		if (tspair->tgt_idx_to_src_idx.at(tgt_idx).size() > 0)       							//跳过有对齐的单词
			continue;
		int i;
		for (i=tgt_idx-1;i>=0;i--)                         			    						//向左扫描，直到遇到有对齐的单词
		{
			if (tspair->tgt_idx_to_src_idx.at(i).size() > 0)
				break;
		}
		if (i>=0) 											 									//左边有单词有对齐
		{
			for (int j=0;j<node->rules.front().src_tree_frag.size();j++)	    				//遍历最小规则的每个节点，检查能否被依附
			{
				if (node->rules.front().src_tree_frag.at(j)->tgt_span.second == i				//被检查节点的右边界等于i
					&& node->rules.front().src_tree_frag.at(j)->type == 1)						//被检查节点为边界节点
				{
					Rule rule = node->rules.front();
					rule.type = 2;
					rule.tgt_spans_for_src_node.at(0).second = tgt_idx;							//更新规则根节点的目标端span
					rule.tgt_spans_for_src_node.at(j).second = tgt_idx;							//更新变量节点的在目标端的控制范围
					int variable_idx = node->rules.front().src_node_status.at(j);
					if (variable_idx >= 0)
					{
						for (int k=rule.tgt_spans_for_src_node.at(j).first;k<=rule.tgt_spans_for_src_node.at(j).second;k++)
						{
							rule.tgt_word_status.at(k) = variable_idx;
						}
					}
					node->rules.push_back(rule);
				}
			}
		}

		for (i=tgt_idx+1;i<tspair->tgt_sen_len;i++)                        		   				//向右扫描，直到遇到有对齐的单词
		{
			if (tspair->tgt_idx_to_src_idx.at(i).size() > 0)
				break;
		}
		if (i < tspair->tgt_sen_len) 															//右边有单词有对齐
		{
			for (int j=0;j<node->rules.front().src_tree_frag.size();j++)	    				//遍历最小规则的每个节点，检查能否被依附
			{
				if (node->rules.front().src_tree_frag.at(j)->tgt_span.first == i				//被检查节点的左边界等于i
					&& node->rules.front().src_tree_frag.at(j)->type == 1)						//被检查节点为边界节点
				{
					Rule rule = node->rules.front();
					rule.type = 2;
					rule.tgt_spans_for_src_node.at(0).first = tgt_idx;							//更新规则根节点的目标端span
					rule.tgt_spans_for_src_node.at(j).first = tgt_idx;							//更新变量节点的在目标端的控制范围
					int variable_idx = node->rules.front().src_node_status.at(j);
					if (variable_idx >= 0)
					{
						for (int k=rule.tgt_spans_for_src_node.at(j).first;k<=rule.tgt_spans_for_src_node.at(j).second;k++)
						{
							rule.tgt_word_status.at(k) = variable_idx;
						}
					}
					node->rules.push_back(rule);
				}
			}
		}
	}
}

/**************************************************************************************
 1. 函数功能: 抽取目标语言每一个span对应的句法短语规则
 2. 入口参数: 无
 3. 出口参数: 无
 4. 算法简介: 遍历目标语言每一个span
 			  1) 检查是否存在满足词对齐约束的源端短语
			  2) 寻找能覆盖源端短语的最低句法节点
			  3) 先序遍历句法子树，找出规则源端
************************************************************************************* */
void RuleExtractor::extract_SPMT_rules()
{
	for (int len=0;len<tspair->tgt_sen_len && len<MAX_PHRASE_LEN;len++)			  //遍历目标端所有的短语
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
			SyntaxNode *node = tspair->word_nodes.at(src_span.first)->father;     //从源端span最左端单词的词性节点往上找
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
			rule.tgt_spans_for_src_node.push_back(tgt_span);
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
			int lbound = min(tgt_span.first,node->tgt_span.first);               // 当前规则的左右边界
			int rbound = max(tgt_span.second,node->tgt_span.second);
			rule.tgt_spans_for_src_node.at(0) = make_pair(lbound,rbound);
			for (int tgt_idx=lbound;tgt_idx<=rbound;tgt_idx++)
			{
				if (rule.tgt_word_status.at(tgt_idx) == -1 && (tgt_idx<tgt_span.first || tgt_idx>tgt_span.second))
				{
					rule.tgt_word_status.at(tgt_idx) = -2;						 //跳过目标端span以外的未对齐的词
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
		for (int tgt_idx=node->tgt_span.first;tgt_idx<=node->tgt_span.second;tgt_idx++)
		{
			rule.tgt_word_status.at(tgt_idx) = rule.variable_num;						//根据当前节点的tgt_span更新目标端单词的状态
		}
		rule.variable_num++;
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

/**************************************************************************************
 1. 函数功能: 抽取当前子树包含的组合规则
 2. 入口参数: 当前子树的根节点
 3. 出口参数: 无
 4. 算法简介: 先序遍历当前子树，抽取每个每个节点的组合规则
************************************************************************************* */
void RuleExtractor::extract_compose_rules(SyntaxNode* node)
{
	vector<Rule>* rules_to_be_composed = &node->rules;
	vector<Rule>* composed_rules = new vector<Rule>;
	for (int compose_num=1;compose_num<MAX_RULE_SIZE;compose_num++)
	{
		for (auto rule : *rules_to_be_composed)
		{
			expand_rule(rule,composed_rules);
		}
		node->rules.insert(node->rules.end(),composed_rules->begin(),composed_rules->end());
		rules_to_be_composed = composed_rules;
	}
	for (const auto child : node->children)
	{
		extract_compose_rules(child);
	}
}

void RuleExtractor::expand_rule(Rule &rule,vector<Rule>* composed_rules)
{
	int variable_idx = -1;												//表示当前节点是规则中第几个变量节点
	for (int node_idx=0;node_idx<rule.src_tree_frag.size();node_idx++)
	{
		if (rule.src_node_status.at(node_idx) >= 0)							//遇到变量节点，进行扩展生成新规则
		{
			variable_idx++;
			Rule &min_rule = rule.src_tree_frag.at(node_idx)->rules.at(0);//TODO 每个变量节点只有唯一一个最小规则,可考虑扩展含未对齐词的最小规则
			assert(min_rule.type == 1);    //TODO
			Rule new_rule;
			new_rule.variable_num = rule.variable_num + min_rule.variable_num - 1;
			new_rule.type = 4;
			new_rule.size = rule.size + 1;
			new_rule.src_tree_frag.assign(rule.src_tree_frag.begin(),rule.src_tree_frag.begin()+node_idx);
			new_rule.src_tree_frag.insert(new_rule.src_tree_frag.end(),min_rule.src_tree_frag.begin(),min_rule.src_tree_frag.end());
			new_rule.src_tree_frag.insert(new_rule.src_tree_frag.end(),rule.src_tree_frag.begin()+node_idx+1,rule.src_tree_frag.end());
			new_rule.src_node_status.assign(rule.src_node_status.begin(),rule.src_node_status.begin()+node_idx);
			for (auto status : min_rule.src_node_status)
			{
				if (status == -1)
					new_rule.src_node_status.push_back(-2);						//最小规则的根节点变成组合规则的内部节点
				else if (status >= 0)
					new_rule.src_node_status.push_back(status+variable_idx);	//最小规则的变量节点需更新变量编号
				else
					new_rule.src_node_status.push_back(status);					//最小规则的内部节点和单词节点状态保持不变
			}
			for (int i=node_idx+1;i<rule.src_node_status.size();i++)
			{
				int status = rule.src_node_status.at(i);
				if (status >= 0)
					new_rule.src_node_status.push_back(status+min_rule.size-1);	//后面的的变量节点需更新变量编号
				else
					new_rule.src_node_status.push_back(status);					//内部节点和单词节点状态保持不变
			}
			//new_rule.tgt_span.first = min(rule.tgt_span.first,min_rule.tgt_span.first);
			//new_rule.tgt_span.second = max(rule.tgt_span.second,min_rule.tgt_span.second);  //TODO
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
		RuleExtractor tsa_triple(line_tree,line_str,line_align);
		tsa_triple.extract_rules();
	}
}
