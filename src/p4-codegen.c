/**
 * @file p4-codegen.c
 * @brief Compiler phase 4: code generation
 *
 * @author Dylan Roth & Dakota Scott
 */
#include "p4-codegen.h"

/**
 * @brief State/data for the code generator visitor
 */
typedef struct CodeGenData
{
    /**
     * @brief Reference to the epilogue jump label for the current function
     */
    Operand current_epilogue_jump_label;
    Operand bp;
    Operand sp;
    /* add any new desired state information (and clean it up in CodeGenData_free) */
} CodeGenData;

/**
 * @brief Allocate memory for code gen data
 * 
 * @returns Pointer to allocated structure
 */
CodeGenData* CodeGenData_new ()
{
    CodeGenData* data = (CodeGenData*)calloc(1, sizeof(CodeGenData));
    CHECK_MALLOC_PTR(data);
    data->current_epilogue_jump_label = empty_operand();
    data->bp = base_register();
    data->sp = stack_register();
    return data;
}

/**
 * @brief Deallocate memory for code gen data
 * 
 * @param data Pointer to the structure to be deallocated
 */
void CodeGenData_free (CodeGenData* data)
{
    /* free everything in data that is allocated on the heap */
    /* free "data" itself */
    free(data);
}

/**
 * @brief Macro for more convenient access to the error list inside a @c visitor
 * data structure
 */
#define DATA ((CodeGenData*)visitor->data)

/**
 * @brief Fills a register with the base address of a variable.
 * 
 * @param node AST node to emit code into (if needed)
 * @param variable Desired variable
 * @returns Virtual register that contains the base address
 */
Operand var_base (ASTNode* node, Symbol* variable)
{
    Operand reg = empty_operand();
    switch (variable->location) {
        case STATIC_VAR:
            reg = virtual_register();
            ASTNode_emit_insn(node,
                    ILOCInsn_new_2op(LOAD_I, int_const(variable->offset), reg));
            break;
        case STACK_PARAM:
        case STACK_LOCAL:
            reg = base_register();
            break;
        default:
            break;
    }
    return reg;
}

/**
 * @brief Calculates the offset of a scalar variable reference and fills a register with that offset.
 * 
 * @param node AST node to emit code into (if needed)
 * @param variable Desired variable
 * @returns Virtual register that contains the base address
 */
Operand var_offset (ASTNode* node, Symbol* variable)
{
    Operand op = empty_operand();
    switch (variable->location) {
        case STATIC_VAR:    op = int_const(0); break;
        case STACK_PARAM:
        case STACK_LOCAL:   op = int_const(variable->offset);
        default:
            break;
    }
    return op;
}

#ifndef SKIP_IN_DOXYGEN

/*
 * Macros for more convenient instruction generation
 */

#define EMIT0OP(FORM)             ASTNode_emit_insn(node, ILOCInsn_new_0op(FORM))
#define EMIT1OP(FORM,OP1)         ASTNode_emit_insn(node, ILOCInsn_new_1op(FORM,OP1))
#define EMIT2OP(FORM,OP1,OP2)     ASTNode_emit_insn(node, ILOCInsn_new_2op(FORM,OP1,OP2))
#define EMIT3OP(FORM,OP1,OP2,OP3) ASTNode_emit_insn(node, ILOCInsn_new_3op(FORM,OP1,OP2,OP3))
void CodeGenVisitor_previsit_program(NodeVisitor* visitor, ASTNode* node) {
    EMIT1OP(PUSH, DATA->bp);
}
void CodeGenVisitor_gen_program (NodeVisitor* visitor, ASTNode* node)
{
    /*
     * make sure "code" attribute exists at the program level even if there are
     * no functions (although this shouldn't happen if static analysis is run
     * first); also, don't include a print function here because there's not
     * really any need to re-print all the functions in the program node *
     */
    ASTNode_set_attribute(node, "code", InsnList_new(), (Destructor)InsnList_free);
    /* copy code from each function */
    FOR_EACH(ASTNode*, func, node->program.functions) {
        ASTNode_copy_code(node, func);
    }
}
void CodeGenVisitor_gen_literal (NodeVisitor* visitor, ASTNode* node) {
    //TODO handle non-integer literals
    DecafType literal_type = node->literal.type;
    Operand reg = virtual_register();
    ASTNode_set_temp_reg(node, reg);
    switch (literal_type)
    {
    case INT:
        EMIT2OP(LOAD_I, int_const(node->literal.integer), reg);
        break;
    case BOOL:
        if(node->literal.boolean) {
            EMIT2OP(LOAD_I, int_const(1), reg);
        } else {
            EMIT2OP(LOAD_I, int_const(0), reg);
        }
        break;
    default:
        break;
    }
    
    
}
void CodeGenVisitor_gen_assignment(NodeVisitor* visitor, ASTNode* node) {
    Operand store_register = virtual_register();
    //Operand value = ASTNode_get_temp_reg(node->assignment.value);
    EMIT2OP(LOAD_I, int_const(1), store_register);
    EMIT2OP(STORE_AI, store_register, int_const(1));
}
void CodeGenVisitor_previsit_literal (NodeVisitor* visitor, ASTNode* node) {
    
}
void CodeGenVisitor_previsit_funcdecl (NodeVisitor* visitor, ASTNode* node)
{
    /* generate a label reference for the epilogue that can be used while
     * generating the rest of the function (e.g., to be used when generating
     * code for a "return" statement) */
    DATA->current_epilogue_jump_label = anonymous_label();
}
void CodeGenVisitor_gen_previsit_return (NodeVisitor* visitor, ASTNode* node) {

}
void CodeGenVisitor_gen_return (NodeVisitor* visitor, ASTNode* node) {
    ASTNode_copy_code(node, node->funcreturn.value);
    Operand return_reg = ASTNode_get_temp_reg(node->funcreturn.value);
    EMIT2OP(I2I, return_reg, return_register());
}

void CodeGenVisitor_gen_block (NodeVisitor* visitor, ASTNode* node) {
    FOR_EACH(ASTNode*, n, node->block.statements) {
        ASTNode_copy_code(node, n);
    }
    EMIT1OP(JUMP, DATA->current_epilogue_jump_label);
}

void CodeGenVisitor_gen_funcdecl (NodeVisitor* visitor, ASTNode* node)
{
    /* every function begins with the corresponding call label */
    EMIT1OP(LABEL, call_label(node->funcdecl.name));
    /* BOILERPLATE: TODO: implement prologue */
    EMIT1OP(PUSH, DATA->bp);
    EMIT2OP(I2I, DATA->sp, DATA->bp);
    EMIT3OP(ADD_I, DATA->sp, int_const(LOCAL_BP_OFFSET), DATA->sp);
    /* copy code from body */
    ASTNode_copy_code(node, node->funcdecl.body);
    EMIT1OP(LABEL, DATA->current_epilogue_jump_label);
    /* BOILERPLATE: TODO: implement epilogue */
    EMIT2OP(I2I, DATA->bp, DATA->sp);
    EMIT1OP(POP, DATA->bp);
    EMIT0OP(RETURN);
}
void CodeGenVisitor_gen_binaryop (NodeVisitor* visior, ASTNode* node) {
    ASTNode_copy_code(node, node->binaryop.left);
    Operand left_reg = ASTNode_get_temp_reg(node->binaryop.left);
    ASTNode_copy_code(node, node->binaryop.right);
    Operand right_reg = ASTNode_get_temp_reg(node->binaryop.right);
    Operand store_reg = virtual_register();
    ASTNode_set_temp_reg(node, store_reg);
    switch (node->binaryop.operator)
    {
        case ADDOP:
            EMIT3OP(ADD, left_reg, right_reg, store_reg); break;
        case MULOP:
            EMIT3OP(MULT, left_reg, right_reg, store_reg); break;
        case SUBOP:
            EMIT3OP(SUB, left_reg, right_reg, store_reg); break;
        case DIVOP:
            EMIT3OP(DIV, left_reg, right_reg, store_reg); break;
        case ANDOP:
            EMIT3OP(AND, left_reg, right_reg, store_reg); break;
        case EQOP:
            EMIT3OP(CMP_EQ, left_reg, right_reg, store_reg); break;
        case NEQOP:
            EMIT3OP(CMP_NE, left_reg, right_reg, store_reg); break;
        case LTOP:
            EMIT3OP(CMP_LT, left_reg, right_reg, store_reg); break;
        case LEOP:
            EMIT3OP(CMP_LE, left_reg, right_reg, store_reg); break;
        case GTOP:
            EMIT3OP(CMP_GT, left_reg, right_reg, store_reg); break;
        case GEOP:
            EMIT3OP(CMP_GE, left_reg, right_reg, store_reg); break;
        case MODOP:
            break;
            //printf("No Mod"); exit(1); break;
        default:
            break;
            //printf("Error, I really don't know why this would pop up"); exit(1); break;
    }
}

#endif
InsnList* generate_code (ASTNode* tree)
{
    if (!tree) {
        return false;
    }
    InsnList* iloc = InsnList_new();
    NodeVisitor* v = NodeVisitor_new();
    v->data = CodeGenData_new();
    v->dtor = (Destructor)CodeGenData_free;
    // FuncDecls
    v->previsit_funcdecl    = CodeGenVisitor_previsit_funcdecl;
    v->postvisit_funcdecl   = CodeGenVisitor_gen_funcdecl;
    // Return Statements
    v->postvisit_return     = CodeGenVisitor_gen_return;
    // Literals
    v->postvisit_literal    = CodeGenVisitor_gen_literal; 
    // Assignments
    v->postvisit_assignment = CodeGenVisitor_gen_assignment;
    // Binary Op
    v->postvisit_binaryop   = CodeGenVisitor_gen_binaryop;
    // Block Statements
    v->postvisit_block      = CodeGenVisitor_gen_block;
    // Program
    v->previsit_program     = CodeGenVisitor_previsit_program;
    v->postvisit_program    = CodeGenVisitor_gen_program;
    /* generate code into AST attributes */
    NodeVisitor_traverse_and_free(v, tree);

    /* copy generated code into new list (the AST may be deallocated before
     * the ILOC code is needed) */
    FOR_EACH(ILOCInsn*, i, (InsnList*)ASTNode_get_attribute(tree, "code")) {
        InsnList_add(iloc, ILOCInsn_copy(i));
    }
    return iloc;
}
