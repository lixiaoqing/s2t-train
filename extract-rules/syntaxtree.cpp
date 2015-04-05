#include "syntaxtree.h"

SyntaxTree::SyntaxTree(const string &line_of_tree,vector<pair<int,int> > *si2ts,vector<vector<int> > *ti2si)
{
	src_idx_to_tgt_span = si2ts;
	tgt_idx_to_src_idx = ti2si;
	if (line_of_tree.size() > 3)
	{
		build_tree_from_str(line_of_tree);
		check_frontier_for_nodes_in_subtree(root);
	}
	else
	{
		root = NULL;
	}
	dump(root);
	cout<<endl;
}

/**************************************************************************************
 1. 函数功能: 将字符串解析成句法树
 2. 入口参数: 一句话的句法分析结果，Berkeley Parser格式
 3. 出口参数: 无
 4. 算法简介: 见注释
************************************************************************************* */
void SyntaxTree::build_tree_from_str(const string &line_of_tree)
{
	vector<string> toks = Split(line_of_tree);
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
				cur_node->tgt_span = src_idx_to_tgt_span->at(word_index);
				cur_node->type = 0;
				words.push_back(toks[i]);
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
void SyntaxTree::check_frontier_for_nodes_in_subtree(SyntaxNode* node)
{
	if (node->children.empty() )                                                                                           // 单词节点
		return;
	for (const auto child : node->children)
	{
		check_frontier_for_nodes_in_subtree(child);
	}
	node->src_span = make_pair(node->children.front()->src_span.first,node->children.back()->src_span.second);            // 更新src_span
	int lbound = node->children.front()->tgt_span.first;                 												  // 遍历子节点，更新tgt_span
	int rbound = node->children.front()->tgt_span.second;
	for (const auto child : node->children)
	{
		if (child->tgt_span.first == -1)                                                                                   // 该孩子节点对空了
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

	int type = 1;           																								// 检查节点是否为边界节点
	if (lbound == -1)
	{
		type = 2;
	}
	else
	{
		for (int tgt_idx=lbound; tgt_idx<=rbound; tgt_idx++)
		{
			if (tgt_idx_to_src_idx->at(tgt_idx).size() <= 1)
				continue;
			for (int src_idx : tgt_idx_to_src_idx->at(tgt_idx))
			{
				if (src_idx < node->src_span.first || src_idx > node->src_span.second)
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

void SyntaxTree::dump(SyntaxNode* node)
{
	if (node == NULL)
		return;
	cout<<" ( "<<node->label<<' '<<node->src_span.first<<':'<<node->src_span.second<<' '<<node->tgt_span.first<<':'<<node->tgt_span.second<<' '<<node->type;
	//cout<<" ( "<<node->label<<' ';
	for (const auto &child : node->children)
		dump(child);
	cout<<" ) ";
}

