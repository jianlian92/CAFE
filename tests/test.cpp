#include "CppUTest/TestHarness.h"
#include "CppUTest/CommandLineTestRunner.h"

extern "C" {
#include <utils_string.h>
#include <cafe_shell.h>
#include <tree.h>
#include <cafe.h>
};

#include <cafe_commands.h>
#include "lambda.h"

extern "C" {
	void show_sizes(FILE*, pCafeParam param, pCafeFamilyItem pitem, int i);
	int __cafe_cmd_lambda_tree(pArgument parg);
	void phylogeny_lambda_parse_func(pTree ptree, pTreeNode ptnode);
}

const char *newick_tree = "(((chimp:6,human:6):81,(mouse:17,rat:17):70):6,dog:9)";

void init_cafe_tree()
{
	char buf[100];
	strcpy(buf, "tree ");
	strcat(buf, newick_tree);
	cafe_shell_dispatch_command(buf);
}


int cafe_cmd_atoi(int argc, char* argv[])
{
	return atoi(argv[1]);
}

CafeShellCommand cafe_cmd_test[] =
{
	{ "atoi", cafe_cmd_atoi },
	{ NULL, NULL }
};


TEST_GROUP(FirstTestGroup)
{
};

TEST_GROUP(LambdaTests)
{
};

TEST(FirstTestGroup, TestStringSplitter)
{
	char c[10];
	pArrayList pArray;
	strcpy(c, "a b");
	pArray = string_pchar_space_split(c);
	LONGS_EQUAL(2, pArray->size);
	STRCMP_EQUAL("a", (char *)(pArray->array[0]));
	STRCMP_EQUAL("b", (char *)(pArray->array[1]));
}

TEST(FirstTestGroup, TestCafeFamilyNew)
{
	pCafeFamily fam;
	char fname[100];
	strcpy(fname, "Nonexistent.tab");
	POINTERS_EQUAL(NULL, cafe_family_new(fname, 1));

	strcpy(fname, "../example/example_data.tab");
	fam = cafe_family_new(fname, 1);
	LONGS_EQUAL(5, fam->num_species);
	STRCMP_EQUAL("Dog", fam->species[0]);
	LONGS_EQUAL(59, fam->flist->size);
	STRCMP_EQUAL("Rat", fam->species[4]);
	LONGS_EQUAL(59, fam->flist->size);
}

TEST(FirstTestGroup, TestCafeTree)
{
	char tree[100];
	strcpy(tree, newick_tree);
	int family_sizes[2] = { 1,1 };
	int rootfamily_sizes[2] = { 1,1 };
	pCafeTree cafe_tree = cafe_tree_new(tree, family_sizes, rootfamily_sizes, 0, 0);
	LONGS_EQUAL(128, cafe_tree->super.size);

	// Find chimp in the tree after two branches of length 6,81,6
	pTreeNode ptnode = (pTreeNode)cafe_tree->super.root;
	CHECK(tree_is_root(&cafe_tree->super, ptnode));
	ptnode = (pTreeNode)tree_get_child(ptnode, 0);
	pPhylogenyNode pnode = (pPhylogenyNode)ptnode;
	LONGS_EQUAL(6, pnode->branchlength);	// root to first branch = 6

	ptnode = (pTreeNode)tree_get_child(ptnode, 0);
	pnode = (pPhylogenyNode)ptnode;
	LONGS_EQUAL(81, pnode->branchlength); // 1st to 2nd = 81

	ptnode = (pTreeNode)tree_get_child(ptnode, 0);
	pnode = (pPhylogenyNode)ptnode;
	STRCMP_EQUAL("chimp", pnode->name);
	LONGS_EQUAL(6, pnode->branchlength);  // 2nd to chimp leaf = 6
	CHECK(tree_is_leaf(ptnode));


}

TEST(FirstTestGroup, TestShellDispatcher)
{
	char c[10];
	cafe_shell_init(1);

	CafeShellCommand *old = cafe_cmd;
	cafe_cmd[0] = cafe_cmd_test[0];
	cafe_cmd[1] = cafe_cmd_test[1];

	strcpy(c, "atoi 9528");
	LONGS_EQUAL(9528, cafe_shell_dispatch_command(c));

	strcpy(c, "# a comment");
	LONGS_EQUAL(0, cafe_shell_dispatch_command(c));

	strcpy(c, "unknown");
	LONGS_EQUAL(CAFE_SHELL_NO_COMMAND, cafe_shell_dispatch_command(c));
}

int lambda_cmd_helper()
{
	std::vector<std::string> strs;
	strs.push_back("lambda");
	strs.push_back("-s");
	strs.push_back("-t");
	strs.push_back("(((2,2)1,(1,1)1)1,1)");
	return cafe_cmd_lambda(strs);

}

TEST(LambdaTests, TestCmdLambda_FailsWithoutTree)
{
	cafe_shell_init(1);

	char buf[1000];
	setbuf(stderr, buf);

	LONGS_EQUAL(-1, lambda_cmd_helper());
	const char *expected = "ERROR(lambda): You did not specify tree: command 'tree'";
	STRCMP_CONTAINS(expected, buf);
	setbuf(stderr, NULL);
}

TEST(LambdaTests, TestCmdLambdaFailsWithoutLoad)
{
	cafe_shell_init(1);
	char buf[1000];

	strcpy(buf, "tree ");
	strcat(buf, newick_tree);
	cafe_shell_dispatch_command(buf);

	char errbuf[1000];
	setbuf(stderr, errbuf);
	char outbuf[10000];
	setbuf(stdout, outbuf);
	LONGS_EQUAL(-1, lambda_cmd_helper());
	const char *expected = "ERROR(lambda): Please load family (\"load\") and cafe tree (\"tree\") before running \"lambda\" command.";
	STRCMP_CONTAINS(expected, errbuf);
	setbuf(stderr, NULL);
	setbuf(stdout, NULL);
}

TEST(LambdaTests, TestCmdLambda)
{
	cafe_shell_init(1);
	init_cafe_tree();

	char buf[100];
	strcpy(buf, "load -i ../example/example_data.tab");
	cafe_shell_dispatch_command(buf);

	LONGS_EQUAL(0, lambda_cmd_helper());
};

TEST(LambdaTests, TestLambdaTree)
{
	cafe_shell_init(1);
	init_cafe_tree();
	char strs[2][100];
	strcpy(strs[0], "(((2,2)1,(1,1)1)1,1)");

	Argument arg;
	arg.argc = 1;
	char* argv[] = { strs[0], strs[1] };
	arg.argv = argv;
	__cafe_cmd_lambda_tree(&arg);
};


TEST(FirstTestGroup, TestShowSizes)
{
	char outbuf[10000];
	setbuf(stdout, outbuf);

	CafeParam param;
	param.rootfamily_sizes[0] = 29;
	param.rootfamily_sizes[1] = 31;
	param.family_sizes[0] = 37;
	param.family_sizes[1] = 41;

	CafeFamilyItem item;
	item.ref = 14;
	CafeTree tree;
	tree.rootfamilysizes[0] = 11;
	tree.rootfamilysizes[1] = 13;
	tree.familysizes[0] = 23;
	tree.familysizes[1] = 19;
	tree.rfsize = 17;
	param.pcafe = &tree;
	FILE* in = fmemopen(outbuf, 999, "w");
	show_sizes(in, &param, &item, 7);
	fclose(in);
	STRCMP_CONTAINS(">> 7 14", outbuf);
	STRCMP_CONTAINS("Root size: 11 ~ 13 , 17", outbuf);
	STRCMP_CONTAINS("Family size: 23 ~ 19", outbuf);
	STRCMP_CONTAINS("Root size: 29 ~ 31", outbuf);
	STRCMP_CONTAINS("Family size: 37 ~ 41", outbuf);
}

TEST(FirstTestGroup, TestPhylogenyLoadFromString)
{
	char outbuf[10000];
	strcpy(outbuf, "(((1,1)1,(2,2)2)2,2)");
	cafe_shell_init(1);
	init_cafe_tree();
	pTree tree = phylogeny_load_from_string(outbuf, tree_new, phylogeny_new_empty_node, phylogeny_lambda_parse_func);
	CHECK(tree != 0);
	LONGS_EQUAL(56, tree->size);

};

TEST(FirstTestGroup, Test_cafe_cmd_source)
{
	std::vector<std::string> strs;
	strs.push_back("source");
	LONGS_EQUAL( -1, cafe_cmd_source(strs));

	strs.push_back("nonexistent");
	LONGS_EQUAL(-1, cafe_cmd_source(strs));
};

TEST(LambdaTests, Test_arguments)
{
	cafe_shell_init(1);
	init_cafe_tree();
	std::vector<std::string> strs;
	strs.push_back("lambda");
	strs.push_back("-t");
	strs.push_back("(((2,2)1,(1,1)1)1,1)");
	pArrayList pal = lambda_build_argument(strs);
	lambda_args args = get_arguments(pal);
	CHECK_FALSE(args.search);
	CHECK_FALSE(args.tmp_param.checkconv);
	CHECK_TRUE(args.tmp_param.lambda_tree != 0);
	LONGS_EQUAL(2, args.tmp_param.num_lambdas);
	DOUBLES_EQUAL(0, args.vlambda, .001);
	LONGS_EQUAL(0, args.blambda);

	strs.push_back("-s");
	pal = lambda_build_argument(strs);
	args = get_arguments(pal);
	CHECK_TRUE(args.search);

	strs.push_back("-checkconv");
	pal = lambda_build_argument(strs);
	args = get_arguments(pal);
	CHECK_TRUE(args.tmp_param.checkconv);

	strs.push_back("-v");
	strs.push_back("14.6");
	pal = lambda_build_argument(strs);
	args = get_arguments(pal);
	DOUBLES_EQUAL(14.6, args.vlambda, .001);
	LONGS_EQUAL(2, args.blambda);

	strs.push_back("-k");
	strs.push_back("19");
	pal = lambda_build_argument(strs);
	args = get_arguments(pal);
	LONGS_EQUAL(19, args.tmp_param.k);

	strs.push_back("-f");
	pal = lambda_build_argument(strs);
	args = get_arguments(pal);
	LONGS_EQUAL(1, args.tmp_param.fixcluster0);
};


TEST(LambdaTests, Test_l_argument)
{
	cafe_shell_init(1);
	init_cafe_tree();
	std::vector<std::string> strs;
	strs.push_back("lambda");
	strs.push_back("-l");
	strs.push_back("15.6");
	strs.push_back("9.2");
	strs.push_back("21.8");
	pArrayList pal = lambda_build_argument(strs);
	lambda_args args = get_arguments(pal);
	LONGS_EQUAL(3, args.tmp_param.num_params);
	LONGS_EQUAL(1, args.blambda);
	DOUBLES_EQUAL(15.6, args.tmp_param.lambda[0], .001);
	DOUBLES_EQUAL(9.2, args.tmp_param.lambda[1], .001);
	DOUBLES_EQUAL(21.8, args.tmp_param.lambda[2], .001);
};

TEST(LambdaTests, Test_p_argument)
{
	cafe_shell_init(1);
	init_cafe_tree();
	std::vector<std::string> strs;
	strs.push_back("lambda");
	strs.push_back("-p");
	strs.push_back("15.6");
	strs.push_back("9.2");
	strs.push_back("21.8");
	pArrayList pal = lambda_build_argument(strs);
	lambda_args args = get_arguments(pal);
	LONGS_EQUAL(3, args.tmp_param.num_params);
	DOUBLES_EQUAL(15.6, args.tmp_param.k_weights[0], .001);
	DOUBLES_EQUAL(9.2, args.tmp_param.k_weights[1], .001);
	DOUBLES_EQUAL(21.8, args.tmp_param.k_weights[2], .001);
};

TEST(LambdaTests, Test_r_argument)
{
	cafe_shell_init(1);
	init_cafe_tree();
	std::vector<std::string> strs;
	strs.push_back("lambda");
	strs.push_back("-r");
	strs.push_back("-o");
	strs.push_back("test.txt");
	pArrayList pal = lambda_build_argument(strs);
	lambda_args args = get_arguments(pal);
	STRCMP_EQUAL("-r", args.pdist->opt);
	LONGS_EQUAL(1, args.pout->argc);
	STRCMP_EQUAL("test.txt", args.pout->argv[0]);
};

int main(int ac, char** av)
{
	return CommandLineTestRunner::RunAllTests(ac, av);
}