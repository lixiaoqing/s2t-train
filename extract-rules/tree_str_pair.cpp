#include "tree_str_pair.h"

TreeStrPair::TreeStrPair(const string &line_tree,const string &line_str,const string &line_align)
{
	load_alignment(line_align);
	tgt_words = Split(line_str);
	tgt_sen_len = tgt_words.size();
	if (line_tree.size() > 3)
	{
		build_tree_from_str(line_tree);
		check_frontier_for_nodes_in_subtree(root);
	}
	else
	{
		root = NULL;
	}
}

void TreeStrPair::load_alignment(const string &line_align)
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

/**************************************************************************************
 1. 函数功能: 将字符串解析成句法树
 2. 入口参数: 一句话的句法分析结果，Berkeley Parser格式
 3. 出口参数: 无
 4. 算法简介: 见注释
************************************************************************************* */
void TreeStrPair::build_tree_from_str(const string &line_tree)
{
	vector<string> toks = Split(line_tree);
	SyntaxNode* cur_node;
	SyntaxNode* pre_node;
	int word_index = 0;
	for(size_t i=0;i<toks.size();i++)
	{
		//左括号情形，且去除"("作为终结符的特例(做终结符时，后继为“）”)
		if(toks[i]=="(" && i+1<toks.size() && toks[i+1]!=")")
		{
			string test=toks[i];
			if(i == 0)
			{
				root     = new SyntaxNode;
				pre_node = root;
				cur_node = root;
			}
			else
			{
				cur_node = new SyntaxNode;
				cur_node->father = pre_node;
				pre_node->children.push_back(cur_node);
				pre_node = cur_node;
			}
		}
		//右括号情形，去除右括号“）”做终结符的特例（做终结符时，前驱的前驱为“（,而且前驱不是")"
		else if(toks[i]==")" && !(i-2>=0 && toks[i-2] =="(" && toks[i-1] != ")"))
		{
			pre_node = pre_node->father;
			cur_node = pre_node;
		}
		//处理形如 （ VV 需要 ）其中VV节点这样的情形
		else if((i-1>=0 && toks[i-1]=="(") && (i+2<toks.size() && toks[i+2]==")"))
		{
			cur_node->label  = toks[i];
			cur_node         = new SyntaxNode;
			cur_node->father = pre_node;
			pre_node->children.push_back(cur_node);
		}
		//处理形如 VP （ VV 需要 ） VP这样的节点 或 需要 这样的节点
		else
		{
			cur_node->label = toks[i];
			//如果是“需要”的情形，则记录中文词的序号
			if(toks[i+1]==")")
			{
				cur_node->src_span = make_pair(word_index,word_index);
				cur_node->tgt_span = src_idx_to_tgt_span.at(word_index);
				cur_node->type = 0;
				word_nodes.push_back(cur_node);
				word_index++;
			}
		}
	}
}

/**************************************************************************************
 1. 函数功能: 检查以当前子树中的每个节点是否为边界节点
 2. 入口参数: 当前子树的根节点
 3. 出口参数: 无
 4. 算法简介: 1) 后序遍历当前子树
 			  2) 根据子节点的src_span和tgt_span计算当前节点的src_span和tgt_span
			  3) 检查tgt_span中的每个词是否都对齐到src_span中，从而确定当前节点是否为
			     边界节点
************************************************************************************* */
void TreeStrPair::check_frontier_for_nodes_in_subtree(SyntaxNode* node)
{
	if (node->children.empty() )                                                                               // 单词节点
		return;
	for (const auto child : node->children)
	{
		check_frontier_for_nodes_in_subtree(child);
	}
	node->src_span = make_pair(node->children.front()->src_span.first,node->children.back()->src_span.second);  // 更新src_span
	int lbound = node->children.front()->tgt_span.first;                 									    // 遍历子节点，更新tgt_span
	int rbound = node->children.front()->tgt_span.second;
	for (const auto child : node->children)
	{
		if (child->tgt_span.first == -1)                                                                        // 该孩子节点对空了
			continue;
		if (lbound == -1 || lbound > child->tgt_span.first)
		{
			lbound = child->tgt_span.first;
		}
		if (rbound == -1 || rbound < child->tgt_span.second)
		{
			rbound = child->tgt_span.second;
		}
	}
	node->tgt_span = make_pair(lbound,rbound);

	int type = 1;           																					// 检查节点是否为边界节点
	if (lbound == -1)
	{
		type = 2;
	}
	else
	{
		for (int tgt_idx=lbound; tgt_idx<=rbound; tgt_idx++)
		{
			if (tgt_idx_to_src_idx.at(tgt_idx).size() <= 1)
				continue;
			for (int src_idx : tgt_idx_to_src_idx.at(tgt_idx))
			{
				if (src_idx < node->src_span.first || src_idx > node->src_span.second) 						 // 目标语言单词对到了src_span外面
				{
					type = 2;
					goto end;
				}
			}
		}
	}
end:
	node->type = type;
}

void TreeStrPair::dump_all_rules(SyntaxNode* node)
{
	if (node == NULL)
		return;
	for (auto rule : node->rules)
	{
		dump_rule(rule);
	}
	for (const auto &child : node->children)
	{
		dump_all_rules(child);
	}
}

void TreeStrPair::dump_rule(Rule &rule)
{
	string src_side = rule.src_tree_frag.at(0)->label + " ";
	for (int i=1;i<rule.src_tree_frag.size();i++)
	{
		if (rule.src_tree_frag.at(i-1) != rule.src_tree_frag.at(i)->father)			//前一个节点不是是当前节点的父节点
		{
			SyntaxNode* node = rule.src_tree_frag.at(i-1);
			while(node != rule.src_tree_frag.at(i)->father)							//沿着前一个节点往上走，直到走到当前节点的父节点
			{
				src_side += ") ";
				node = node->father;
			}
		}
		src_side += "( ";		
		if (rule.src_node_status.at(i) < 0)											//规则源端内部节点或者单词节点
		{
			src_side += rule.src_tree_frag.at(i)->label + " ";
		}
		else 																		//规则源端变量节点
		{
			src_side += "x"+to_string(rule.src_node_status.at(i))+":"+rule.src_tree_frag.at(i)->label+" ";
		}
	}
	SyntaxNode* node = rule.src_tree_frag.back();
	while (node != rule.src_tree_frag.front())										//补齐剩下的右括号
	{
		src_side += ") ";
		node = node->father;
	}
	string tgt_side;
	int tgt_idx=rule.src_node_span.front().first; 							//遍历当前规则tgt_span的每个单词，根据tgt_word_status生成规则目标端
	while (tgt_idx<=rule.src_node_span.front().second)
	{
		if (rule.tgt_word_status.at(tgt_idx) == -1)
		{
			tgt_side += tgt_words.at(tgt_idx)+" ";
			tgt_idx++;
		}
		else if (rule.tgt_word_status.at(tgt_idx) >= 0)
		{
			int variable_num = rule.tgt_word_status.at(tgt_idx);
			tgt_side += "x"+to_string(variable_num)+" ";
			while(tgt_idx<tgt_words.size() && rule.tgt_word_status.at(tgt_idx) == variable_num)
			{
				tgt_idx++; 															// 这些单词被同一个变量替换
			}
		}
	}
	cout<<src_side<<" ||| "<<tgt_side<<" ||| "<<rule.type<<endl;
}

