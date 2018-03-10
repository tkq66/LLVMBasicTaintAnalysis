#include <cstdio>
#include <iostream>
#include <set>
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"



using namespace llvm;

void generateCFG(BasicBlock*,std::set<Instruction*>);
std::set<Instruction*> checkLeakage(BasicBlock*,std::set<Instruction*>);

int main(int argc, char **argv)
{
    // Read the IR file.
    LLVMContext Context;
    SMDiagnostic Err;
    std::unique_ptr<Module> M = parseIRFile(argv[1], Err, Context);
    if (M == nullptr)
    {
      fprintf(stderr, "error: failed to load LLVM IR file \"%s\"", argv[1]);
      return EXIT_FAILURE;
    }

    std::set<Instruction*> secretVars;
    
    for (auto &F: *M)
      if (strncmp(F.getName().str().c_str(),"main",4) == 0){
	  BasicBlock* BB = dyn_cast<BasicBlock>(F.begin()); 
	  generateCFG(BB,secretVars);
      }

    return 0;
}


void generateCFG(BasicBlock* BB,std::set<Instruction*> secretVars)
{
  printf("Label Name:%s\n", BB->getName().str().c_str());
  std::set<Instruction*> newSecretVars = checkLeakage(BB,secretVars);

  // Pass secretVars list to child BBs and check them
  const TerminatorInst *TInst = BB->getTerminator();
  int NSucc = TInst->getNumSuccessors();
  for (int i = 0;  i < NSucc; ++i) {
    BasicBlock *Succ = TInst->getSuccessor(i);    
    generateCFG(Succ,newSecretVars);
  }

  // Last Basic Block, check if secret leaks to public
  if (NSucc == 0){
    int flag = false;
    for (auto &S: newSecretVars)
      if (strncmp(S->getName().str().c_str(),"public",6) == 0) {
	flag = true;
      }
    
    if (flag == true)
	printf(ANSI_COLOR_RED "OMG, Secret leaks to the Public"
	       ANSI_COLOR_RESET	"\n");
    else
	printf(ANSI_COLOR_GREEN "Secret does not leak to the Public"
	       ANSI_COLOR_RESET	"\n");
  } 
}

std::set<Instruction*> checkLeakage(BasicBlock* BB,
				    std::set<Instruction*> secretVars)
{
  std::set<Instruction*> newSecretVars(secretVars);
  
  // Loop through instructions in BB
  for (auto &I: *BB)
  {
    // Add secret variable to newSecretVars  
    if (strncmp(I.getName().str().c_str(),"secret",6) == 0){
      printf(ANSI_COLOR_BLUE "Secret found:%s" ANSI_COLOR_RESET "\n", 
	     I.getName().str().c_str());
      newSecretVars.insert(dyn_cast<Instruction>(&I));
    }

    if (isa<StoreInst>(I)){
      // Check store instructions
      Value* v = I.getOperand(0);
      Instruction* op1 = dyn_cast<Instruction>(v); 
      if (op1 != nullptr &&
	newSecretVars.find(op1) != newSecretVars.end()){
	newSecretVars.insert(dyn_cast<Instruction>(I.getOperand(1)));	
      }
    }else{	
      // Check all other instructions
      for (auto op = I.op_begin(); op != I.op_end(); op++) {
	Value* v = op->get();
	Instruction* inst = dyn_cast<Instruction>(v);
	if (inst != nullptr && newSecretVars.find(inst) != 
			       newSecretVars.end())
	  newSecretVars.insert(dyn_cast<Instruction>(&I));
      }
    }
  }
  return newSecretVars;
}
