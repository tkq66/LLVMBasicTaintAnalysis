/**
 *  TaintPass.cpp
 *
 *  Performs taint analysis on simple programs over an LLVM IR pass.
 *  Supported structures are if-else adn while loop.
 *
 *  Created by CS5218 - Principles of Program Analysis
 *  Modified by Teekayu Klongtruajrok (A0174348X)
 *  Contact: e0210381@u.nus.edu
 */
#include <cstdio>
#include <iostream>
#include <set>
#include <stack>
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define MAIN_FUNCTION "main"
#define SRC_VAR_NAME "source"
#define SNK_VAR_NAME "sink"
#define LOOP_BEGIN_BLOCK_NAME "while.cond"
#define LOOP_END_BLOCK_NAME "while.end"

using namespace llvm;

enum AnalyzeLoopBackedgeSwtch {
    ON,
    OFF
};

std::set<Instruction*> generateCFG (BasicBlock*, std::set<Instruction*>, std::stack<BasicBlock*>, AnalyzeLoopBackedgeSwtch);
std::set<Instruction*> checkLeakage (BasicBlock*, std::set<Instruction*>);
bool isSameBlock (BasicBlock*, BasicBlock*);
bool isMainFunction (const char*);
bool isBeginLoop (const char*);
bool isEndLoop (const char*);
bool isSourceVar (const char*);
bool isSinkVar (const char*);
bool isSinkVar (const char*);
void printVars (std::set<Instruction*>);
void printInsts(std::set<Instruction*>);
void printLLVMValue (Value* v);

int main (int argc, char **argv) {
    // Read the IR file.
    LLVMContext Context;
    SMDiagnostic Err;
    std::unique_ptr<Module> M = parseIRFile(argv[1], Err, Context);
    if (M == nullptr) {
        fprintf(stderr, "error: failed to load LLVM IR file \"%s\"", argv[1]);
        return EXIT_FAILURE;
    }

    std::set<Instruction*> secretVars;

    for (auto &F: *M) {
        if (isMainFunction(F.getName().str().c_str())) {
            BasicBlock* BB = dyn_cast<BasicBlock>(F.begin());
            std::stack<BasicBlock*> loopCallStack;
            generateCFG(BB, secretVars, loopCallStack, ON);
        }
    }

    return 0;
}


std::set<Instruction*> generateCFG (BasicBlock* BB, std::set<Instruction*> secretVars, std::stack<BasicBlock*> loopCallStack, AnalyzeLoopBackedgeSwtch backedgeSwitch) {
  const char *blockName = BB->getName().str().c_str();
  printf("Label Name:%s\n", blockName);

  // Create local copies of parameters that can be updated
  AnalyzeLoopBackedgeSwtch newBackedgeSwitch = backedgeSwitch;
  std::stack<BasicBlock*> newLoopCallStack = !loopCallStack.empty() ? std::stack<BasicBlock*>(loopCallStack) : std::stack<BasicBlock*>();

  // Track loop layer by pushing them into the stack
  if (isBeginLoop(blockName)) {
      newLoopCallStack.push(BB);
  }
  // Untrack the loop when a loop ends
  if (isEndLoop(blockName)) {
      newLoopCallStack.pop();
      // Turn back on to prepare for any outer loops
      newBackedgeSwitch = ON;
  }

  std::set<Instruction*> newSecretVars = checkLeakage(BB,secretVars);

  // Pass secretVars list to child BBs and check them
  const TerminatorInst *tInst = BB->getTerminator();
  int branchCount = tInst->getNumSuccessors();

  std::set<Instruction*> consolidatedSecretVars(newSecretVars);
  for (int i = 0;  i < branchCount; ++i) {
      BasicBlock *next = tInst->getSuccessor(i);
      BasicBlock *prevLoopBegin = !newLoopCallStack.empty() ? newLoopCallStack.top() : nullptr;

      // If still analyzing loop backedge and the loop is going past the calling point, stop this recursion
      if (isEndLoop(next->getName().str().c_str()) &&
          (newBackedgeSwitch == OFF)) {
          return consolidatedSecretVars;
      }
      // Analyze loop backedge by running through the loop one more time before ending
      // to complete taint analysis of variable dependencies
      if (isEndLoop(next->getName().str().c_str()) &&
          (newBackedgeSwitch == ON) &&
          !newLoopCallStack.empty()) {
          // prevent repeating backedge analysis loop
          newBackedgeSwitch = OFF;
          // Run while cond again, now with knowledge of all the discovered secret var args of the first run,
          // and update the local secret var args context to propagate this knowledge to code after the loop
          newSecretVars = generateCFG(prevLoopBegin, consolidatedSecretVars, newLoopCallStack, newBackedgeSwitch);
      }
      // Terminate looping condition to acheive least fixed point solution
      if (isSameBlock(prevLoopBegin, next)) {
          return consolidatedSecretVars;
      }
      // Analyze the next instruction and get all the discovered from that analysis context
      std::set<Instruction*> contextDependentSecretVars =  generateCFG(next, newSecretVars, newLoopCallStack, newBackedgeSwitch);
      // Consolidate all the secret vars from a given pass
      consolidatedSecretVars.insert(contextDependentSecretVars.begin(), contextDependentSecretVars.end());
  }

  // Last Basic Block, check if secret leaks to public
  if (branchCount == 0){
    int flag = false;
    for (auto &S: newSecretVars) {
        if (isSinkVar(S->getName().str().c_str())) {
            flag = true;
        }
    }

    if (flag == true) {
        printf(ANSI_COLOR_RED "OMG, Secret leaks to the Public"
    	       ANSI_COLOR_RESET	"\n");
    }
    else {
        printf(ANSI_COLOR_GREEN "Secret does not leak to the Public"
    	       ANSI_COLOR_RESET	"\n");
    }
    printf("\n");
  }
  // Propagate discovered secret var args from the current context
  return consolidatedSecretVars;
}

std::set<Instruction*> checkLeakage (BasicBlock* BB, std::set<Instruction*> secretVars) {
    std::set<Instruction*> newSecretVars(secretVars);

    // Loop through instructions in BB
    for (auto &I: *BB) {
        // Add secret variable to newSecretVars
        if (isSourceVar(I.getName().str().c_str())) {
            newSecretVars.insert(dyn_cast<Instruction>(&I));
        }

        if (isa<StoreInst>(I)) {
            // Check store instructions
            Value* v = I.getOperand(0);
            Instruction* op1 = dyn_cast<Instruction>(v);
            if (op1 != nullptr && newSecretVars.find(op1) != newSecretVars.end()) {
                newSecretVars.insert(dyn_cast<Instruction>(I.getOperand(1)));
            }
        }
        else {
            // Check all other instructions
            for (auto op = I.op_begin(); op != I.op_end(); op++) {
                Value* v = op->get();
                Instruction* inst = dyn_cast<Instruction>(v);
                if (inst != nullptr && newSecretVars.find(inst) != newSecretVars.end()) {
                    newSecretVars.insert(dyn_cast<Instruction>(&I));
                }
            }
        }
    }

    printVars(newSecretVars);
    return newSecretVars;
}

bool isSameBlock (BasicBlock* blockA, BasicBlock* blockB) {
    // Prevent nullptr reference
    if (!blockA || !blockB) {
        return false;
    }
    return blockA->getName().str() == blockB->getName().str();
}

bool isMainFunction (const char* functionName) {
    return strncmp(functionName, MAIN_FUNCTION, strlen(MAIN_FUNCTION)) == 0;
}

bool isBeginLoop (const char* instructionName) {
    return strncmp(instructionName, LOOP_BEGIN_BLOCK_NAME, strlen(LOOP_BEGIN_BLOCK_NAME)) == 0;
}

bool isEndLoop (const char* instructionName) {
    return strncmp(instructionName, LOOP_END_BLOCK_NAME, strlen(LOOP_END_BLOCK_NAME)) == 0;
}

bool isSourceVar (const char* varName) {
    return strncmp(varName, SRC_VAR_NAME, strlen(SRC_VAR_NAME)) == 0;
}

bool isSinkVar (const char* varName) {
    return strncmp(varName, SNK_VAR_NAME, strlen(SNK_VAR_NAME)) == 0;
}

void printVars (std::set<Instruction*> vars) {
    for (auto &S: vars) {
        printf("%s ", S->getName().str().c_str() );
    }
    printf("\n");
}

void printInsts(std::set<Instruction*> insts) {
    for (auto &S: insts) {
        printLLVMValue(dyn_cast<Value>(S));
    }
    printf("\n");
}

void printLLVMValue (Value* v) {
    std::string res;
    raw_string_ostream rso(res);
    v->print(rso, true);
    printf("%s\n", res.c_str());
}
