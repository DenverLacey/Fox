//
//  compiler.cpp
//  Fox
//
//  Created by Denver Lacey on 9/7/21.
//  Copyright © 2021 Denver Lacey. All rights reserved.
//

#include "compiler.h"

#include "error.h"
#include "interpreter.h"

Compiler::Compiler(
    Interpreter *interp,
    Data_Section &constants,
    Data_Section &str_constants,
    Function_Definition *function)
  : constants(constants),
    str_constants(str_constants)
{
    stack_top = 0;
    parent = nullptr;
    global = this;
    global_scope = nullptr;
    this->function = function;
    this->interp = interp;
}

Compiler::Compiler(Compiler *parent, Function_Definition *function)
  : constants(parent->constants),
    str_constants(parent->str_constants)
{
    stack_top = 0;
    this->parent = parent;
    global = parent->global;
    global_scope = parent->global_scope;
    this->function = function;
    this->interp = parent->interp;
}

Function_Definition *Compiler::compile(Ref<Typed_AST_Multiary> multi) {
    begin_scope();
    global_scope = &current_scope();
    for (auto &n : multi->nodes) {
        n->compile(*this);
    }
    
    return this->function;
}

void Compiler::emit_byte(uint8_t byte) {
    
    function->instructions.push_back(byte);
}

void Compiler::emit_opcode(Opcode op) {
    static_assert(sizeof(Opcode) == sizeof(uint8_t), "This function assumess Opcode is 8 bits large.");
    emit_byte(static_cast<uint8_t>(op));
}

void Compiler::emit_size(Size size) {
    emit_value<Size>(size);
}

void Compiler::emit_address(Address address) {
    emit_value<Address>(address);
}

size_t Compiler::emit_jump(Opcode jump_code, bool update_stack_top) {
    emit_opcode(jump_code);
    size_t jump = function->instructions.size();
    emit_value<size_t>(-1);
    if (update_stack_top &&
        (jump_code == Opcode::Jump_True ||
         jump_code == Opcode::Jump_False))
    {
        stack_top -= value_types::Bool.size();
    }
    return jump;
}

void Compiler::patch_jump(size_t jump) {
    size_t to = function->instructions.size();
    size_t *jump_size = reinterpret_cast<size_t *>(&function->instructions[jump]);
    *jump_size = to - jump - sizeof(size_t);
}

void Compiler::emit_loop(size_t loop_start) {
    emit_opcode(Opcode::Loop);
    size_t jump = function->instructions.size() - loop_start + sizeof(size_t);
    emit_value<size_t>(jump);
}

Variable &Compiler::put_variable(
    String id,
    Value_Type type,
    Address address,
    bool is_const)
{
    std::string sid = id.str();
    Compiler_Scope &s = current_scope();
    Variable v = { is_const, type, address };
    s.variables[sid] = v;
    return s.variables[sid];
}

void Compiler::put_variables_from_pattern(
    Typed_AST_Processed_Pattern &pp,
    Address address)
{
    Address next_variable_address = address;
    for (auto &b : pp.bindings) {
        if (b.id != "") {
            put_variable(b.id, b.type, next_variable_address);
        }
        next_variable_address += b.type.size();
    }
}

Find_Variable_Result Compiler::find_variable(String id) {
    std::string sid = id.str();
    for (Compiler_Scope *s = &current_scope(); s != nullptr; s = s->parent) {
        auto it = s->variables.find(sid);
        if (it != s->variables.end()) {
            return {
                it->second.is_const ?
                    Find_Variable_Result::Found_Constant :
                    Find_Variable_Result::Found,
                &s->variables[sid]
            };
        }
    }
    
    // check global scope
    auto it = global_scope->variables.find(sid);
    if (it != global_scope->variables.end()) {
        return {
            it->second.is_const ?
                Find_Variable_Result::Found_Constant :
                Find_Variable_Result::Found_Global,
            &global_scope->variables[sid]
        };
    }
    
    // nothing was found
    return { Find_Variable_Result::Not_Found, nullptr };
}

void Compiler::declare_constant(Typed_AST_Let &let) {
    Address old_top = stack_top;
    
    internal_verify(let.target->bindings.size() == 1, "'const' only works with single identifiers, for now.");
    auto id = let.target->bindings[0].id;
    
    verify(let.initializer->is_constant(*this), "Cannot initialize constant with non-constant expression.");
    
    Variable constant = { true, let.initializer->type };
    constant.type.is_mut = false; // incase initializer->type.is_mut
    
    switch (let.initializer->type.kind) {
        case Value_Type_Kind::Bool: {
            runtime::Bool value;
            evaluate_unchecked(let.initializer, value);
            constant.address = add_constant<runtime::Bool>(value);
        } break;
        case Value_Type_Kind::Char: {
            runtime::Char value;
            evaluate_unchecked(let.initializer, value);
            constant.address = add_constant<runtime::Char>(value);
        } break;
        case Value_Type_Kind::Int: {
            runtime::Int value;
            evaluate_unchecked(let.initializer, value);
            constant.address = add_constant<runtime::Int>(value);
        } break;
        case Value_Type_Kind::Float: {
            runtime::Float value;
            evaluate_unchecked(let.initializer, value);
            constant.address = add_constant<runtime::Float>(value);
        } break;
        case Value_Type_Kind::Str: {
            runtime::String value;
            evaluate_unchecked(let.initializer, value);
            constant.address = add_slice_constant(value.len, value.s);
            SMem.deallocate(value.s, value.len);
        } break;
        case Value_Type_Kind::Ptr:
            todo("declaring constants of pointer types not yet implemented.");
            break;
        case Value_Type_Kind::Array: {
            size_t size = let.initializer->type.size();
            void *data = alloca(size);
            evaluate_unchecked(let.initializer, data);
            constant.address = add_constant(data, size);
        } break;
        case Value_Type_Kind::Slice: {
            runtime::Slice value;
            evaluate_unchecked(let.initializer, value);
            constant.address = add_slice_constant(value.count, reinterpret_cast<char *>(value.data));
            free(value.data);
        } break;
        case Value_Type_Kind::Tuple: {
            size_t size = let.initializer->type.size();
            void *data = alloca(size);
            evaluate_unchecked(let.initializer, data);
            constant.address = add_constant(data, size);
        } break;
        case Value_Type_Kind::Range: {
            size_t size = let.initializer->type.size();
            void *data = alloca(size);
            evaluate_unchecked(let.initializer, data);
            constant.address = add_constant(data, size);
        } break;
            
        case Value_Type_Kind::Struct:
            todo("declaring constants of a struct type not yet implemented.");
            break;
        case Value_Type_Kind::Enum:
            todo("declaring constats of an enum type not yet implemented.");
            break;
            
        case Value_Type_Kind::Void:
            error("Cannot declare a constant of type (void).");
            break;
            
        default:
            internal_error("Unexpected Value_Type_Kind in Compiler::declare_variable(): %d.", let.initializer->type.kind);
    }
    
    std::string sid = id.str();
    current_scope().variables[sid] = constant;
    
    stack_top = old_top;
}

void Compiler::compile_constant(Variable constant) {
    Address old_top = stack_top;
    
    switch (constant.type.kind) {
        case Value_Type_Kind::Bool: {
            runtime::Bool value = get_constant<runtime::Bool>(constant.address);
            emit_opcode(value ? Opcode::Lit_True : Opcode::Lit_False);
        } break;
        case Value_Type_Kind::Char: {
            runtime::Char value = get_constant<runtime::Char>(constant.address);
            emit_opcode(Opcode::Lit_Char);
            emit_value<runtime::Char>(value);
        } break;
        case Value_Type_Kind::Int: {
            runtime::Int value = get_constant<runtime::Int>(constant.address);
            if (value == 0) {
                emit_opcode(Opcode::Lit_0);
            } else if (value == 1) {
                emit_opcode(Opcode::Lit_1);
            } else {
                emit_opcode(Opcode::Lit_Int);
                emit_value<runtime::Int>(value);
            }
        } break;
        case Value_Type_Kind::Float: {
            runtime::Float value = get_constant<runtime::Float>(constant.address);
            emit_opcode(Opcode::Lit_Float);
            emit_value<runtime::Float>(value);
        } break;
        case Value_Type_Kind::Str: {
            emit_opcode(Opcode::Load_Const_String);
            emit_value<size_t>(constant.address);
        } break;
        case Value_Type_Kind::Ptr: {
            runtime::Pointer value = get_constant<runtime::Pointer>(constant.address);
            emit_opcode(Opcode::Lit_Pointer);
            emit_value<runtime::Pointer>(value);
        } break;
        case Value_Type_Kind::Array: {
            emit_opcode(Opcode::Load_Const);
            emit_size(constant.type.size());
            emit_value<size_t>(constant.address);
        } break;
        case Value_Type_Kind::Slice:
            todo("Constant Slices not yet compilable.");
            break;
            
        case Value_Type_Kind::Tuple:
        case Value_Type_Kind::Range:
            emit_opcode(Opcode::Load_Const);
            emit_size(constant.type.size());
            emit_value<size_t>(constant.address);
            break;
            
        case Value_Type_Kind::Struct:
        case Value_Type_Kind::Enum:
            todo("Constant struct and enum values not compilable yet.");
            
        default:
            internal_error("Invalid Value_Type_kind in Compiler::compile_constant(): %d", constant.type.kind);
    }
    
    stack_top = old_top + constant.type.size();
}

size_t Compiler::add_constant(void *data, size_t size) {
    size_t alligned_size = (((size + Constants_Allignment - 1)) / Constants_Allignment) * Constants_Allignment;
    
    // search for identical constant
    for (size_t i = 0; i < constants.size(); i += Constants_Allignment) {
        if (i + alligned_size > constants.size()) {
            // this constant can't fit and therefore can't already be in
            // the constants data section
            break;
        }
        
        if (memcmp(data, &constants[i], size) == 0) {
            return i;
        }
    }
    
    // no identical so add in the new one
    size_t index = constants.size();
    size_t i;
    for (i = 0; i < size; i++) {
        constants.push_back(*((reinterpret_cast<uint8_t *>(data)) + i));
    }
    for (; i < alligned_size; i++) {
        constants.push_back(0);
    }
    
    return index;
}

size_t Compiler::add_slice_constant(size_t size, char *source) {
    // search for identical string
    size_t i = 0;
    while (i < str_constants.size()) {
        size_t index = i;
        size_t len = *reinterpret_cast<size_t *>(&str_constants[i]);
        i += sizeof(size_t);
        char *str = reinterpret_cast<char *>(&str_constants[i]);
        if (size == len && memcmp(source, str, len) == 0) {
            return index;
        }
        i += len;
    }
    
    // no identical so add new string
    size_t index = str_constants.size();
    
    // write 64 bit length into buffer
    for (int i = 0; i < sizeof(size_t); i++) {
        str_constants.push_back(*((reinterpret_cast<uint8_t *>(&size)) + i));
    }
    
    // write string into buffer
    for (size_t i = 0; i < size; i++) {
        str_constants.push_back(source[i]);
    }
    
    return index;
}

void *Compiler::get_constant(size_t constant) {
    return &constants[constant];
}

Compiler_Scope &Compiler::current_scope() {
    return scopes.front();
}

void Compiler::begin_scope() {
    Compiler_Scope *parent = scopes.empty() ? nullptr : &current_scope();
    Compiler_Scope next{};
    next.stack_bottom = stack_top;
    next.parent = parent;
    scopes.push_front(next);
}

void Compiler::end_scope() {
    Address flush_point = current_scope().stack_bottom;
    emit_opcode(Opcode::Flush);
    emit_address(flush_point);
    scopes.pop_front();
    stack_top = flush_point;
}

void Typed_AST_Bool::compile(Compiler &c) {
    c.emit_opcode(value ? Opcode::Lit_True : Opcode::Lit_False);
    c.stack_top += type.size();
}

void Typed_AST_Char::compile(Compiler &c) {
    c.emit_opcode(Opcode::Lit_Char);
    c.emit_value<runtime::Char>(value);
    c.stack_top += type.size();
}

void Typed_AST_Float::compile(Compiler &c) {
    c.emit_opcode(Opcode::Lit_Float);
    c.emit_value<runtime::Float>(value);
    c.stack_top += type.size();
}

void Typed_AST_Ident::compile(Compiler &c) {
    Address stack_top = c.stack_top;
    
    auto [status, v] = c.find_variable(id);
    switch (status) {
        case Find_Variable_Result::Not_Found:
            error("Unresolved identifier '%s'.", id.c_str());
            break;
        case Find_Variable_Result::Found:
        case Find_Variable_Result::Found_Global:
            c.emit_opcode(status == Find_Variable_Result::Found_Global ? Opcode::Push_Global_Value : Opcode::Push_Value);
            c.emit_size(v->type.size());
            c.emit_address(v->address);
            break;
        case Find_Variable_Result::Found_Constant:
            c.compile_constant(*v);
            break;
    }
    
    c.stack_top = stack_top + v->type.size();
}

void Typed_AST_Int::compile(Compiler &c) {
    if (value == 0) {
        c.emit_opcode(Opcode::Lit_0);
    } else if (value == 1) {
        c.emit_opcode(Opcode::Lit_1);
    } else {
        c.emit_opcode(Opcode::Lit_Int);
        c.emit_value<runtime::Int>(value);
    }
    c.stack_top += type.size();
}

void Typed_AST_Str::compile(Compiler &c) {
    size_t constant = c.add_slice_constant(value.size(), value.c_str());
    c.emit_opcode(Opcode::Load_Const_String);
    c.emit_value<size_t>(constant);
    c.stack_top += type.size();
}

void Typed_AST_Builtin::compile(Compiler &c) {
    internal_error("Call to Typed_AST_Builtin::compile() is disallowed.");
}

struct Find_Static_Address_Result {
    enum {
        Not_Found,
        Found,
        Found_Global
    } status;
    Address address;
};

static bool emit_address_code(Compiler &c, Typed_AST &node);

static Find_Static_Address_Result find_static_address(Compiler &c, Typed_AST &node) {
    auto status = Find_Static_Address_Result::Found;
    Address address;
    
    switch (node.kind) {
        case Typed_AST_Kind::Ident: {
            auto id = dynamic_cast<Typed_AST_Ident *>(&node);
            internal_verify(id, "Failed to cast node to an Ident* in find_static_address.");
            
            auto [v_status, v] = c.find_variable(id->id);
            verify(v, "Unresolved identifier '%.*s'.", id->id.size(), id->id.c_str());
            
            if (v_status == Find_Variable_Result::Found_Global) {
                status = Find_Static_Address_Result::Found_Global;
            }
            
            address = v->address;
        } break;
        case Typed_AST_Kind::Subscript: {
            auto sub = dynamic_cast<Typed_AST_Binary *>(&node);
            internal_verify(sub, "Failed to cast node to a Binary* in find_static_address().");
            
            auto [array_status, array_address] = find_static_address(c, *sub->lhs);
            
            if (array_status == Find_Static_Address_Result::Not_Found ||
                sub->lhs->type.kind != Value_Type_Kind::Array ||
                !sub->rhs->is_constant(c))
            {
                status = Find_Static_Address_Result::Not_Found;
                break;
            }
            
            if (array_status == Find_Static_Address_Result::Found_Global) {
                status = Find_Static_Address_Result::Found_Global;
            }
            
            runtime::Int index;
            c.evaluate_unchecked(sub->rhs, index);
            
            address = array_address + index * sub->type.size();
        } break;
        case Typed_AST_Kind::Field_Access: {
            auto dot = dynamic_cast<Typed_AST_Field_Access *>(&node);
            internal_verify(dot, "Failed to cast node to Field_Access* in find_static_address().");
            
            if (dot->deref) {
                status = Find_Static_Address_Result::Not_Found;
                break;
            }
            
            auto [inst_status, inst_address] = find_static_address(c, *dot->instance);
            
            if (inst_status == Find_Static_Address_Result::Not_Found) {
                status = Find_Static_Address_Result::Not_Found;
                break;
            }
            
            if (inst_status == Find_Static_Address_Result::Found_Global) {
                status = Find_Static_Address_Result::Found_Global;
            }
            
            address = inst_address + dot->field_offset;
        } break;
            
        default:
            status = Find_Static_Address_Result::Not_Found;
            break;
    }
    
    return { status, address };
}

static bool emit_dynamic_address_code(Compiler &c, Typed_AST &node) {
    switch (node.kind) {
        case Typed_AST_Kind::Ident: {
            auto id = dynamic_cast<Typed_AST_Ident *>(&node);
            internal_verify(id, "Failed to cast node to Ident* in emit_dynamic_address_code().");
            
            auto [v_status, v] = c.find_variable(id->id);
            verify(v, "Unresolved identifier '%.*s'.", id->id.size(), id->id.c_str());
            
            c.emit_opcode(v_status == Find_Variable_Result::Found_Global ? Opcode::Push_Global_Pointer : Opcode::Push_Pointer);
            c.emit_address(v->address);
        } break;
        case Typed_AST_Kind::Deref: {
            auto deref = dynamic_cast<Typed_AST_Unary *>(&node);
            internal_verify(deref, "Failed to cast node to Unary* in emit_dynamic_address_code().");
            deref->sub->compile(c);
        } break;
        case Typed_AST_Kind::Subscript: {
            auto sub = dynamic_cast<Typed_AST_Binary *>(&node);
            internal_verify(sub, "Failed to cast node to Binary* in emit_dynamic_address_code().");
            
            Size element_size = sub->lhs->type.child_type()->size();
            
            if (!emit_address_code(c, *sub->lhs)) {
                return false;
            }
            
            if (sub->lhs->type.kind == Value_Type_Kind::Slice) {
                c.emit_opcode(Opcode::Load);
                c.emit_size(value_types::Ptr.size());
            }
            
            // offset = rhs * element_size
            sub->rhs->compile(c);
            c.emit_opcode(Opcode::Lit_Int);
            c.emit_value<runtime::Int>(element_size);
            c.emit_opcode(Opcode::Int_Mul);
            
            // address = &lhs + offset
            c.emit_opcode(Opcode::Int_Add);
        } break;
        case Typed_AST_Kind::Negative_Subscript: {
            auto sub = dynamic_cast<Typed_AST_Binary *>(&node);
            internal_verify(sub, "Failed to cast node to Binary* in emit_dynamic_address_code().");
            internal_verify(sub->lhs->type.kind == Value_Type_Kind::Slice, "sub->lhs is not a slice in Negative_Subscript part of emit_dynamic_address_code().");
            
            Size element_size = sub->type.size();
            runtime::Int index = -sub->rhs.cast<Typed_AST_Int>()->value;
            
            // (data, count) = result of compiling lhs
            sub->lhs->compile(c);
            
            // offset = (count - index) * element_size
            c.emit_opcode(Opcode::Lit_Int);
            c.emit_value<runtime::Int>(index);
            
            c.emit_opcode(Opcode::Int_Sub);
            
            c.emit_opcode(Opcode::Lit_Int);
            c.emit_value<runtime::Int>(element_size);
            
            c.emit_opcode(Opcode::Int_Mul);
            
            // element_ptr = data + offset
            c.emit_opcode(Opcode::Int_Add);
        } break;
        case Typed_AST_Kind::Field_Access: {
            auto dot = dynamic_cast<Typed_AST_Field_Access *>(&node);
            internal_verify(dot, "Failed to cast node to Field_Access* in emit_dynamic_address_code().");
            
            if (dot->deref) {
                dot->instance->compile(c);
            } else {
                emit_address_code(c, *dot->instance);
            }
            
            Size offset = dot->field_offset;
            if (offset == 0) {
                // do nothing
            } else if (offset == 1) {
                c.emit_opcode(Opcode::Lit_1);
                c.emit_opcode(Opcode::Int_Add);
            } else {
                c.emit_opcode(Opcode::Lit_Int);
                c.emit_value<runtime::Int>(offset);
                c.emit_opcode(Opcode::Int_Add);
            }
        } break;
            
        default:
            return false;
    }
    
    return true;
}

static bool emit_address_code(Compiler &c, Typed_AST &node) {
    Address stack_top = c.stack_top;
    bool success = true;
    
    auto [status, address] = find_static_address(c, node);
    
    auto push_type = Opcode::Push_Pointer;
    switch (status) {
        case Find_Static_Address_Result::Found_Global:
            push_type = Opcode::Push_Global_Pointer;
        case Find_Static_Address_Result::Found:
            c.emit_opcode(push_type);
            c.emit_address(address);
            break;
        case Find_Static_Address_Result::Not_Found:
            success = emit_dynamic_address_code(c, node);
            break;
            
        default:
            internal_error("Invalid status in emit_address_code(): %d.", status);
            break;
    }
    
    c.stack_top = stack_top + value_types::Ptr.size();
    return success;
}

void Typed_AST_Nullary::compile(Compiler &c) {
    Address stack_top = c.stack_top;
    
    switch (kind) {
        case Typed_AST_Kind::Allocate:
            c.emit_opcode(Opcode::Allocate);
            c.emit_size(type.size());
            break;
            
        default:
            internal_error("Invalid nullary kind: %d.", kind);
            break;
    }
    
    c.stack_top = stack_top + type.size();
}

void Typed_AST_Unary::compile(Compiler &c) {
    Address stack_top = c.stack_top;
    switch (kind) {
        case Typed_AST_Kind::Negation:
            sub->compile(c);
            if (sub->type.kind == Value_Type_Kind::Int)
                c.emit_opcode(Opcode::Int_Neg);
            else
                c.emit_opcode(Opcode::Float_Neg);
            break;
        case Typed_AST_Kind::Not:
            sub->compile(c);
            c.emit_opcode(Opcode::Not);
            break;
        case Typed_AST_Kind::Address_Of:
        case Typed_AST_Kind::Address_Of_Mut:
            emit_address_code(c, *sub);
            break;
        case Typed_AST_Kind::Deref: {
            Size size = sub->type.child_type()->size();
            sub->compile(c);
            c.emit_opcode(Opcode::Load);
            c.emit_size(size);
        } break;
        case Typed_AST_Kind::Heap_Allocate: {
            sub->compile(c);
            c.emit_opcode(Opcode::Heap_Allocate);
        } break;
            
        default:
            internal_error("Kind is not a valid unary operation: %d.", kind);
            break;
    }
    c.stack_top = stack_top + type.size();
}

void Typed_AST_Return::compile(Compiler &c) {
    Address stack_top = c.stack_top;
    
    Size size = 0;
    if (sub) {
        size = sub->type.size();
        sub->compile(c);
    }
    
    c.emit_opcode(Opcode::Return);
    c.emit_size(size);
    
    c.stack_top = stack_top;
}

static void compile_assignment(Compiler &c, Typed_AST_Binary &b) {
    Address stack_top = c.stack_top;
    
    b.rhs->compile(c);
    bool success = emit_address_code(c, *b.lhs);
    verify(success, "Cannot assign to this kind of expression.");
        
    Size size = b.rhs->type.size();
    c.emit_opcode(Opcode::Move);
    c.emit_size(size);
    
    c.stack_top = stack_top;
}

static void compile_while_loop(Compiler &c, Typed_AST_Binary &b) {
    size_t loop_start = c.function->instructions.size();
    Address stack_top = c.stack_top;
    
    b.lhs->compile(c);
    size_t exit_jump = c.emit_jump(Opcode::Jump_False);
    
    b.rhs->compile(c);
    c.emit_loop(loop_start);
    c.patch_jump(exit_jump);
    
    c.stack_top = stack_top;
}

static void compile_logical_operator(Compiler &c, Typed_AST_Binary &b) {
    Address stack_top = c.stack_top;
    Size size_of_bool = value_types::Bool.size();
    
    b.lhs->compile(c);
    
    size_t jump;
    switch (b.kind) {
        case Typed_AST_Kind::And:
            jump = c.emit_jump(Opcode::Jump_False_No_Pop);
            c.emit_opcode(Opcode::Pop);
            c.emit_size(size_of_bool);
            break;
        case Typed_AST_Kind::Or:
            jump = c.emit_jump(Opcode::Jump_True_No_Pop);
            c.emit_opcode(Opcode::Pop);
            c.emit_size(size_of_bool);
            break;
        default:
            internal_error("Invalid node passed to compile_logical_operator(): %d.", b.kind);
            break;
    }
    
    b.rhs->compile(c);
    c.patch_jump(jump);
    
    c.stack_top = stack_top + size_of_bool;
}

static void emit_dynamic_offset_load(
    Compiler &c,
    Typed_AST &index,
    Size element_size)
{
    // offset = index * element_size
    index.compile(c);
    
    c.emit_opcode(Opcode::Lit_Int);
    c.emit_value<runtime::Int>(element_size);
    
    c.emit_opcode(Opcode::Int_Mul);
    
    // element_ptr = &lhs + offset
    c.emit_opcode(Opcode::Int_Add);
    
    // result = Load(element_size, element_ptr)
    c.emit_opcode(Opcode::Load);
    c.emit_size(element_size);
}

static void compile_range_subscript_operator(Compiler &c, Typed_AST_Binary &sub) {
    todo("Subscript with range not yet implemented.");
    
    Address stack_top = c.stack_top;
    c.stack_top = stack_top + sub.type.size();
}

static void compile_subscript_operator_for_constant(
    Compiler &c,
    Variable *v,
    Ref<Typed_AST> index)
{
    Address stack_top = c.stack_top;
    
    Size child_size = v->type.child_type()->size();
    
    if (!index->is_constant(c)) {
        todo("Non-constant index to subscript of constant array.");
    } else {
        runtime::Int idx;
        c.evaluate_unchecked(index, idx);
        
        c.emit_opcode(Opcode::Load_Const);
        c.emit_size(child_size);
        c.emit_address(v->address + idx * child_size);
    }
    
    c.stack_top = stack_top + child_size;
}

static void compile_subscript_operator(Compiler &c, Typed_AST_Binary &sub) {
    if (sub.rhs->type.kind == Value_Type_Kind::Range) {
        compile_range_subscript_operator(c, sub);
        return;
    } else if (sub.lhs->kind == Typed_AST_Kind::Ident) {
        String id = sub.lhs.cast<Typed_AST_Ident>()->id;
        auto [result, v] = c.find_variable(id);
        
        if (result == Find_Variable_Result::Found_Constant) {
            compile_subscript_operator_for_constant(c, v, sub.rhs);
            return;
        }
    }
    
    Address stack_top = c.stack_top;
    
    if (sub.lhs->type.kind == Value_Type_Kind::Array) {
        Size element_size = sub.lhs->type.child_type()->size();
        
        if (sub.rhs->type.kind == Value_Type_Kind::Int && sub.rhs->is_constant(c)) {
            runtime::Int index;
            c.evaluate_unchecked(sub.rhs, index);
            
            runtime::Int offset = index * element_size;
            
            auto [status, address] = find_static_address(c, *sub.lhs);
            switch (status) {
                case Find_Static_Address_Result::Found:
                    c.emit_opcode(Opcode::Push_Value);
                    c.emit_size(element_size);
                    c.emit_address(address + offset);
                    break;
                case Find_Static_Address_Result::Found_Global:
                    c.emit_opcode(Opcode::Push_Global_Value);
                    c.emit_size(element_size);
                    c.emit_address(address + offset);
                    break;
                case Find_Static_Address_Result::Not_Found:
                    bool success = emit_dynamic_address_code(c, *sub.lhs);
                    verify(success, "Cannot subscript this expression.");
                    emit_dynamic_offset_load(c, *sub.rhs, element_size);
                    break;
            }
        } else {
            bool success = emit_address_code(c, *sub.lhs);
            verify(success, "Cannot subscript this expression.");
            emit_dynamic_offset_load(c, *sub.rhs, element_size);
        }
    } else {
        bool success = emit_address_code(c, *sub.lhs);
//            if (!success) {
//                emit_bytecode(c, binary->lhs);
//                emit_byte(c, BYTE_PUSH_POINTER);
//                emit_address(c, stack_top);
//            }
        verify(success, "Can't subscript this expression.");
        
        // data = Load(&slice, sizeof(Pointer))
        c.emit_opcode(Opcode::Load);
        c.emit_size(value_types::Ptr.size());
        
        // element_ptr = data + index * sizeof(Element)
        sub.rhs->compile(c);
        
        c.emit_opcode(Opcode::Lit_Int);
        c.emit_value<runtime::Int>(sub.type.size());
        
        c.emit_opcode(Opcode::Int_Mul);
        
        c.emit_opcode(Opcode::Int_Add);
        
        // element = Load(element_ptr, sizeof(Element))
        c.emit_opcode(Opcode::Load);
        c.emit_size(sub.type.size());
            
//            if (!success) {
//                emit_byte(c, BYTE_SHIFT);
//                emit_size(c, size);
//                emit_size(c, size_of_type(c, binary->lhs->inferred_type));
//            }
    }
    
    c.stack_top = stack_top + sub.type.size();
}

static void compile_negative_subscript_operator(Compiler &c, Typed_AST_Binary &sub) {
    Address stack_top = c.stack_top;
    
    internal_verify(sub.lhs->type.kind == Value_Type_Kind::Slice, "In compile_negative_subscript_operator(), sub.lhs is not a slice.");
    
    // element_ptr = result of ...
    bool success = emit_address_code(c, sub);
    verify(success, "Cannot subscript this expression.");
    
    // element = Load(element_ptr, sizeof(Element))
    c.emit_opcode(Opcode::Load);
    c.emit_size(sub.type.size());
    
    c.stack_top = stack_top + sub.type.size();
}

static void compile_function_call(Compiler &c, Typed_AST_Binary &call) {
    Address stack_top = c.stack_top;
    
    call.rhs->compile(c);
    call.lhs->compile(c);
    c.emit_opcode(Opcode::Call);
    c.emit_size(call.lhs->type.data.func.arg_size());
    
    c.stack_top = stack_top + call.type.size();
}

static void compile_builtin_call(Compiler &c, Typed_AST_Binary &call) {
    Address stack_top = c.stack_top;
    
    auto builtin = call.lhs.cast<Typed_AST_Builtin>();
    internal_verify(builtin, "Failed to cast builtin to a Builtin*.");
    
    call.rhs->compile(c);
    c.emit_opcode(Opcode::Call_Builtin);
    c.emit_value<Builtin>(builtin->defn->builtin);
    c.emit_size(builtin->defn->type.data.func.arg_size());
    
    c.stack_top = stack_top + call.type.size();
}

void Typed_AST_Binary::compile(Compiler &c) {
    Address stack_top = c.stack_top;
    
    switch (kind) {
        case Typed_AST_Kind::Assignment:
            compile_assignment(c, *this);
            return;
        case Typed_AST_Kind::While:
            compile_while_loop(c, *this);
            return;
        case Typed_AST_Kind::Equal:
            lhs->compile(c);
            rhs->compile(c);
            if (lhs->type.kind == Value_Type_Kind::Str) {
                c.emit_opcode(Opcode::Str_Equal);
            } else {
                c.emit_opcode(Opcode::Equal);
                c.emit_size(lhs->type.size());
            }
            c.stack_top = stack_top + value_types::Bool.size();
            return;
        case Typed_AST_Kind::Not_Equal:
            lhs->compile(c);
            rhs->compile(c);
            if (lhs->type.kind == Value_Type_Kind::Str) {
                c.emit_opcode(Opcode::Str_Not_Equal);
            } else {
                c.emit_opcode(Opcode::Not_Equal);
                c.emit_size(lhs->type.size());
            }
            c.stack_top = stack_top + value_types::Bool.size();
            return;
        case Typed_AST_Kind::And:
        case Typed_AST_Kind::Or:
            compile_logical_operator(c, *this);
            return;
        case Typed_AST_Kind::Subscript:
            compile_subscript_operator(c, *this);
            return;
        case Typed_AST_Kind::Negative_Subscript:
            compile_negative_subscript_operator(c, *this);
            return;
        case Typed_AST_Kind::Range:
        case Typed_AST_Kind::Inclusive_Range:
            lhs->compile(c);
            rhs->compile(c);
            return;
        case Typed_AST_Kind::Function_Call:
            compile_function_call(c, *this);
            return;
        case Typed_AST_Kind::Builtin_Call:
            compile_builtin_call(c, *this);
            return;
    }
    
    Opcode op;
    switch (kind) {
        case Typed_AST_Kind::Addition:
            if (lhs->type.kind == Value_Type_Kind::Int)
                op = Opcode::Int_Add;
            else if (lhs->type.kind == Value_Type_Kind::Float)
                op = Opcode::Float_Add;
            else if (type.kind == Value_Type_Kind::Str)
                op = Opcode::Str_Add;
            break;
        case Typed_AST_Kind::Subtraction:
            if (lhs->type.kind == Value_Type_Kind::Int)
                op = Opcode::Int_Sub;
            else if (lhs->type.kind == Value_Type_Kind::Float)
                op = Opcode::Float_Sub;
            break;
        case Typed_AST_Kind::Multiplication:
            if (lhs->type.kind == Value_Type_Kind::Int)
                op = Opcode::Int_Mul;
            else if (lhs->type.kind == Value_Type_Kind::Float)
                op = Opcode::Float_Mul;
            break;
        case Typed_AST_Kind::Division:
            if (lhs->type.kind == Value_Type_Kind::Int)
                op = Opcode::Int_Div;
            else if (lhs->type.kind == Value_Type_Kind::Float)
                op = Opcode::Float_Div;
            break;
        case Typed_AST_Kind::Mod:
            op = Opcode::Mod;
            break;
            
        case Typed_AST_Kind::Less:
            if (lhs->type.kind == Value_Type_Kind::Int)
                op = Opcode::Int_Less_Than;
            else if (lhs->type.kind == Value_Type_Kind::Float)
                op = Opcode::Float_Less_Than;
            break;
        case Typed_AST_Kind::Less_Eq:
            if (lhs->type.kind == Value_Type_Kind::Int)
                op = Opcode::Int_Less_Equal;
            else if (lhs->type.kind == Value_Type_Kind::Float)
                op = Opcode::Float_Less_Equal;
            break;
        case Typed_AST_Kind::Greater:
            if (lhs->type.kind == Value_Type_Kind::Int)
                op = Opcode::Int_Greater_Than;
            else if (lhs->type.kind == Value_Type_Kind::Float)
                op = Opcode::Float_Greater_Than;
            break;
        case Typed_AST_Kind::Greater_Eq:
            if (lhs->type.kind == Value_Type_Kind::Int)
                op = Opcode::Int_Greater_Equal;
            else if (lhs->type.kind == Value_Type_Kind::Float)
                op = Opcode::Float_Greater_Equal;
            break;
            
        default:
            internal_error("Invalid binary operation: %d.", kind);
            break;
    }
    
    lhs->compile(c);
    rhs->compile(c);
    c.emit_opcode(op);
    
    c.stack_top = stack_top + type.size();
}

void Typed_AST_Ternary::compile(Compiler &c) {
    internal_error("Attempted compilation of Typed_AST_Ternary when there are no ternary operations.");
    Address stack_top = c.stack_top;
    c.stack_top = stack_top + type.size();
}

void Typed_AST_Multiary::compile(Compiler &c) {
    if (kind == Typed_AST_Kind::Block) c.begin_scope();
    for (auto &node : nodes) {
        node->compile(c);
    }
    if (kind == Typed_AST_Kind::Block) c.end_scope();
}

void Typed_AST_Array::compile(Compiler &c) {
    if (kind == Typed_AST_Kind::Array) {
        element_nodes->compile(c);
    } else {
        Size count = (Size)element_nodes->nodes.size();
        Address stack_top = c.stack_top;
        
        if (count == 0) {
            c.emit_opcode(Opcode::Clear_Allocate);
            c.emit_size(value_types::Slice.size());
        } else {
            Size alloc_size = count * type.child_type()->size();
            
            // slice data = result of element_nodes
            element_nodes->compile(c);
            
            // data ptr = result of heap allocate
            c.emit_opcode(Opcode::Heap_Allocate_Inline);
            c.emit_size(alloc_size);
            
            // slice.data = move slice data to data ptr
            c.emit_opcode(Opcode::Move_Push_Pointer);
            c.emit_size(alloc_size);
            
            // slice.count = count
            c.emit_opcode(Opcode::Lit_Int);
            c.emit_value<runtime::Int>(count);
        }
        
        c.stack_top = stack_top + value_types::Slice.size();
    }
}

void Typed_AST_Enum_Literal::compile(Compiler &c) {
    Address stack_top = c.stack_top;
    
    if (tag == 0) {
        c.emit_opcode(Opcode::Lit_0);
    } else if (tag == 1) {
        c.emit_opcode(Opcode::Lit_1);
    } else {
        c.emit_opcode(Opcode::Lit_Int);
        c.emit_value<runtime::Int>(tag);
    }
    
    c.stack_top += value_types::Int.size();
    
    if (payload) {
        payload->compile(c);   
    }
    
    if (c.stack_top < stack_top + type.size()) {
        Size remaining_size = type.size() - static_cast<Size>(c.stack_top - stack_top);
        c.emit_opcode(Opcode::Clear_Allocate);
        c.emit_size(remaining_size);
    }
    
    c.stack_top = stack_top + type.size();
}

void Typed_AST_If::compile(Compiler &c) {
    cond->compile(c);
    size_t else_jump = c.emit_jump(Opcode::Jump_False);
    size_t exit_jump;
    
    then->compile(c);
    if (else_) exit_jump = c.emit_jump(Opcode::Jump);
    c.patch_jump(else_jump);
    
    if (else_) {
        else_->compile(c);
        c.patch_jump(exit_jump);
    }
}

void Typed_AST_Type_Signature::compile(Compiler &c) {
    internal_error("Call to Typed_AST_Type_Signature::compile() is disallowed.");
}

void Typed_AST_Processed_Pattern::compile(Compiler &c) {
    internal_error("Call to Typed_AST_Processed_Pattern::compile() is disallowed.");
}

void Typed_AST_Match_Pattern::compile(Compiler &c) {
    internal_error("Call to Typed_AST_Match_Pattern::compile() is disallowed.");
}

static void compile_for_loop(Typed_AST_For &f, Compiler &c) {
    // initialize counter variable
    Variable counter_v = { false, value_types::Int, c.stack_top };
    c.emit_opcode(Opcode::Lit_0);
    c.stack_top += counter_v.type.size();
    
    if (f.counter != "") {
        c.put_variable(f.counter, counter_v.type, counter_v.address);
    }
    
    // retrieve reference to or initialize iterable variable
    Variable iterable_v;
    if (f.iterable->kind == Typed_AST_Kind::Ident) {
        auto set_id = f.iterable.cast<Typed_AST_Ident>();
        internal_verify(set_id, "Failed to cast f.iterable to Ident* in compile_for_loop().");
        auto [_, v] = c.find_variable(set_id->id);
        verify(v, "Unresolved identifier '%.*s'.", set_id->id.size(), set_id->id.c_str());
        iterable_v = *v;
    } else {
        iterable_v = { false, f.iterable->type, c.stack_top };
        f.iterable->compile(c);
    }
    
    // initialize target variable
    Variable target_v = { false, *iterable_v.type.child_type(), c.stack_top };
    c.put_variables_from_pattern(*f.target, target_v.address);
    c.emit_opcode(Opcode::Allocate);
    c.emit_size(target_v.type.size());
    c.stack_top += target_v.type.size();
    
    size_t loop_start = c.function->instructions.size();
    
    // test condition
    c.emit_opcode(Opcode::Push_Value);
    c.emit_size(counter_v.type.size());
    c.emit_address(counter_v.address);
    
    if (iterable_v.type.kind == Value_Type_Kind::Array) {
        if (iterable_v.type.data.array.count == 1) {
            c.emit_opcode(Opcode::Lit_1);
        } else {
            c.emit_opcode(Opcode::Lit_Int);
            c.emit_value<runtime::Int>(iterable_v.type.data.array.count);
        }
    } else {
        c.emit_opcode(Opcode::Push_Value);
        c.emit_size(value_types::Int.size());
        c.emit_address(iterable_v.address + value_types::Ptr.size());
    }
    
    c.emit_opcode(Opcode::Int_Less_Than);
    size_t exit_jump = c.emit_jump(Opcode::Jump_False, false);
    
    // target_v = iterable[counter]
    c.emit_opcode(Opcode::Push_Value);
    c.emit_size(counter_v.type.size());
    c.emit_address(counter_v.address);
    
    if (iterable_v.type.kind == Value_Type_Kind::Array) {
        c.emit_opcode(Opcode::Lit_Int);
        c.emit_value<runtime::Int>(target_v.type.size());
        
        c.emit_opcode(Opcode::Int_Mul);
        
        c.emit_opcode(Opcode::Push_Pointer);
        c.emit_address(iterable_v.address);
        
        c.emit_opcode(Opcode::Int_Add);
    } else {
        c.emit_opcode(Opcode::Lit_Int);
        c.emit_value<runtime::Int>(iterable_v.type.child_type()->size());
        
        c.emit_opcode(Opcode::Int_Mul);
        
        c.emit_opcode(Opcode::Push_Value);
//        c.emit_size(sizeof(size_t));
        c.emit_size(value_types::Ptr.size());
        c.emit_address(iterable_v.address);
        
        c.emit_opcode(Opcode::Int_Add);
    }
    
    c.emit_opcode(Opcode::Push_Pointer);
    c.emit_address(target_v.address);
    
    c.emit_opcode(Opcode::Copy);
    c.emit_size(target_v.type.size());
    
//    new_loop(c, loop_start, for_->label);
    f.body->compile(c);
//    patch_loop_controls(c, c->continues);
    
    // increment counter
    c.emit_opcode(Opcode::Push_Pointer);
    c.emit_address(counter_v.address);
    c.emit_opcode(Opcode::Inc);

    c.emit_loop(loop_start);

    c.patch_jump(exit_jump);
//    patch_loop_controls(c, c->breaks);
//    end_loop(c);
}

static void compile_for_range_loop(Typed_AST_For &f, Compiler &c) {
    auto range = f.iterable.cast<Typed_AST_Binary>();
    internal_verify(range, "Failed to cast iterable in compile_for_range_loop().");
    internal_verify(range->kind == Typed_AST_Kind::Range ||
                    range->kind == Typed_AST_Kind::Inclusive_Range, "Invalid kind for range variable in compile_for_range_loop(): %d.", range->kind);
    
    // ranges are simple so it should just be an identifier
    verify(f.target->bindings.size() == 1, "Incorrect pattern in for-loop.");
    
    // initialize target_v
    Variable &target_v = c.put_variable(f.target->bindings[0].id, f.target->bindings[0].type, c.stack_top);
    range->lhs->compile(c);
    
    // initialize counter_v if f.counter != ""
    Variable *counter_v = nullptr;
    if (f.counter != "") {
        counter_v = &c.put_variable(f.counter, value_types::Int, c.stack_top);
        c.emit_opcode(Opcode::Lit_0);
        c.stack_top += value_types::Int.size();
    }
    
    Variable end_v = { false, range->rhs->type, c.stack_top };
    range->rhs->compile(c);

    size_t loop_start = c.function->instructions.size();

    // test condition
    c.emit_opcode(Opcode::Push_Value);
    c.emit_size(target_v.type.size());
    c.emit_address(target_v.address);
    
    c.emit_opcode(Opcode::Push_Value);
    c.emit_size(end_v.type.size());
    c.emit_address(end_v.address);
    
    c.emit_opcode(f.iterable->type.data.range.inclusive ? Opcode::Int_Less_Equal : Opcode::Int_Less_Than);
    size_t exit_jump = c.emit_jump(Opcode::Jump_False, false);
    
    //    new_loop(c, loop_start, for_->label);
    f.body->compile(c);
    //    patch_loop_controls(c, c->continues);
    
    if (counter_v) {
        // increment counter
        c.emit_opcode(Opcode::Push_Pointer);
        c.emit_address(counter_v->address);
        c.emit_opcode(Opcode::Inc);
    }
    
    // increment target_v
    c.emit_opcode(Opcode::Push_Pointer);
    c.emit_address(target_v.address);
    c.emit_opcode(Opcode::Inc);
    
    c.emit_loop(loop_start);
    
    c.patch_jump(exit_jump);
//    patch_loop_controls(c, c->breaks);
}

void Typed_AST_For::compile(Compiler &c) {
    c.begin_scope();
    
    switch (kind) {
        case Typed_AST_Kind::For:
            compile_for_loop(*this, c);
            break;
        case Typed_AST_Kind::For_Range:
            compile_for_range_loop(*this, c);
            break;
            
        default:
            internal_error("Invalid Typed_AST_Kind in For::compile(): %d.", kind);
            break;
    }
    
    c.end_scope();
}

void Typed_AST_Match::compile(Compiler &c) {
    Address stack_top = c.stack_top;
    
    Variable cond_v = { false, cond->type, stack_top };
    cond->compile(c);
    c.stack_top = stack_top + cond_v.type.size();
    
    std::vector<size_t> in_jumps;
    in_jumps.reserve(arms->nodes.size());
    
    std::vector<size_t> out_jumps;
    out_jumps.reserve(arms->nodes.size());
    
    //
    // @HACK:
    //      Bit of a hack just to get this to work. There's probably a better way
    //      to achieve the same effect.
    //
    std::vector<std::vector<std::pair<String, Variable>>> idents;
    
    // arms conditions
    for (auto arm : arms->nodes) {
        auto a = arm.cast<Typed_AST_Binary>();
        internal_verify(a, "Failed to cast arm to Typed_AST_Binary in Typed_AST_Match::compile().");
        
        idents.emplace_back();
        
        // valuate equality
        auto arm_cond = a->lhs.cast<Typed_AST_Match_Pattern>();
        
        if (arm_cond->is_simple_value_pattern()) {
            c.emit_opcode(Opcode::Push_Value);
            c.emit_size(cond_v.type.size());
            c.emit_address(cond_v.address);
            
            for (auto &b : arm_cond->bindings) {
                b.value_node->compile(c);
            }
            
            if (cond_v.type.kind == Value_Type_Kind::Str) {
                c.emit_opcode(Opcode::Str_Equal);
            } else {
                c.emit_opcode(Opcode::Equal);
                c.emit_size(cond_v.type.size());
            }
        } else {
            switch (cond_v.type.kind) {
                case Value_Type_Kind::Tuple: {
                    bool not_first_comparison = false;
                    for (size_t i = 0; i < arm_cond->bindings.size(); i++) {
                        auto b = arm_cond->bindings[i];
                        if (b.is_none()) continue;
                        
                        Value_Type child_type = cond_v.type.data.tuple.child_types[i];
                        Size offset = cond_v.type.data.tuple.offset_of_type(i);
                        
                        if (b.kind == Typed_AST_Match_Pattern::Binding_Kind::Variable) {
                            Variable v = {
                                false,
                                b.variable_info.type,
                                cond_v.address + b.offset
                            };
                            idents.back().push_back({ b.variable_info.id, v });
                        } else {
                            c.emit_opcode(Opcode::Push_Value);
                            c.emit_size(child_type.size());
                            c.emit_address(cond_v.address + offset);
                            
                            b.value_node->compile(c);
                            
                            if (child_type.kind == Value_Type_Kind::Str) {
                                c.emit_opcode(Opcode::Str_Equal);
                            } else {
                                c.emit_opcode(Opcode::Equal);
                                c.emit_size(child_type.size());
                            }
                            
                            if (not_first_comparison) {
                                c.emit_opcode(Opcode::And);
                            } else {
                                not_first_comparison = true;
                            }
                        }
                    }
                } break;
                case Value_Type_Kind::Struct: {
                    todo("Implement struct types for match patterns in Typed_AST_Match::compile().");
                } break;
                case Value_Type_Kind::Enum: {
                    auto defn = cond_v.type.data.enum_.defn;
                    
                    auto tag = arm_cond->bindings[0].value_node.cast<Typed_AST_Int>();
                    internal_verify(tag, "Failed to retrieve tag in match pattern.");
                    
                    // cond_v.tag == tag
                    c.emit_opcode(Opcode::Push_Value);
                    c.emit_size(value_types::Int.size());
                    c.emit_address(cond_v.address);
                    
                    tag->compile(c);
                    
                    c.emit_opcode(Opcode::Equal);
                    c.emit_size(value_types::Int.size());
                    
                    for (size_t i = 1; i < arm_cond->bindings.size(); i++) {
                        auto b = arm_cond->bindings[i];
                        if (b.is_none()) continue;
                        
                        if (b.kind == Typed_AST_Match_Pattern::Binding_Kind::Variable) {
                            Variable v = {
                                false,
                                b.variable_info.type,
                                cond_v.address + b.offset
                            };
                            idents.back().push_back({ b.variable_info.id, v });
                        } else {
                            Value_Type field_type = b.value_node->type;
                            Size offset = b.offset;
                            
                            c.emit_opcode(Opcode::Push_Value);
                            c.emit_size(field_type.size());
                            c.emit_address(cond_v.address + offset);
                            
                            b.value_node->compile(c);
                            
                            if (field_type.kind == Value_Type_Kind::Str) {
                                c.emit_opcode(Opcode::Str_Equal);
                            } else {
                                c.emit_opcode(Opcode::Equal);
                                c.emit_size(field_type.size());
                            }
                            
                            c.emit_opcode(Opcode::And);
                        }
                    }
                } break;
                    
                default:
                    internal_error("Invalid type for match pattern: %d.", cond_v.type.kind);
                    break;
            }
        }
        
        in_jumps.emplace_back(c.emit_jump(Opcode::Jump_True));
    }
    
    if (default_arm) {
        default_arm->compile(c);
    }
    out_jumps.emplace_back(c.emit_jump(Opcode::Jump));
    
    for (size_t i = 0; i < arms->nodes.size(); i++) {
        auto arm = arms->nodes[i].cast<Typed_AST_Binary>();
        
        size_t in_jump = in_jumps[i];
        c.patch_jump(in_jump);
        
        c.begin_scope();
        
        for (auto [id, v] : idents[i]) {
            c.put_variable(id, v.type, v.address);
        }
        
        arm->rhs->compile(c);
        
        c.end_scope();
        
        if (i < arms->nodes.size() - 1) {
            out_jumps.emplace_back(c.emit_jump(Opcode::Jump));
        }
    }
    
    for (size_t out_jump : out_jumps) {
        c.patch_jump(out_jump);
    }
  
    c.emit_opcode(Opcode::Flush);
    c.emit_address(cond_v.address);
    
    c.stack_top = stack_top;
}

void Typed_AST_Let::compile(Compiler &c) {
    if (is_const) {
        c.declare_constant(*this);
        return;
    }
    
    Address stack_top = c.stack_top;
    
    Value_Type type = specified_type ? *specified_type->value_type : initializer->type;
    
    if (initializer) {
        initializer->compile(c);
    } else {
        c.emit_opcode(Opcode::Clear_Allocate);
        c.emit_size(type.size());
    }
    
    c.put_variables_from_pattern(*target, stack_top);
    
    c.stack_top = stack_top + type.size();
}

void Typed_AST_Field_Access::compile(Compiler &c) {
    Address stack_top = c.stack_top;
    
    if (deref) {
        bool success = emit_dynamic_address_code(c, *this);
        verify(success, "Cannot access field of this expression.");
        c.emit_opcode(Opcode::Load);
        c.emit_size(type.size());
    } else {
        auto [status, address] = find_static_address(c, *instance);
        switch (status) {
            case Find_Static_Address_Result::Found:
                c.emit_opcode(Opcode::Push_Value);
                c.emit_size(type.size());
                c.emit_address(address + field_offset);
                break;
            case Find_Static_Address_Result::Found_Global:
                c.emit_opcode(Opcode::Push_Global_Value);
                c.emit_size(type.size());
                c.emit_address(address + field_offset);
                break;
            case Find_Static_Address_Result::Not_Found:
                bool success = emit_dynamic_address_code(c, *this);
                verify(success, "Cannot access field of this expression.");
                c.emit_opcode(Opcode::Load);
                c.emit_size(type.size());
                break;
        }
    }
    
    c.stack_top = stack_top + type.size();
}

void Typed_AST_UUID::compile(Compiler &c) {
    if (type.kind == Value_Type_Kind::Type) {
        internal_error("Cannot compile a UUID of a type.");
    } else {
        Address stack_top = c.stack_top;
        
        switch (type.kind) {
            case Value_Type_Kind::Function: {
                Function_Definition *defn = c.interp->functions.get_func_by_uuid(uuid);
                internal_verify(defn, "Failed to retrieve Function_Definition in Typed_AST_UUID::compile().");
                c.emit_opcode(Opcode::Lit_Pointer);
                c.emit_value<runtime::Pointer>(defn);
            } break;
                
            default:
                internal_error("Invalid Value_Type_Kind in Typed_AST_UUID::compile().");
                break;
        }
        
        c.stack_top = stack_top + type.size();
    }
}

void Typed_AST_Fn_Declaration::compile(Compiler &c) {
    auto fn = c.interp->functions.get_func_by_uuid(defn->uuid);
    auto new_c = Compiler { &c, fn };
    
    new_c.begin_scope();
    for (size_t i = 0; i < defn->param_names.size(); i++) {
        new_c.put_variable(
            defn->param_names[i],
            defn->type.data.func.arg_types[i],
            new_c.stack_top
        );
        new_c.stack_top += defn->type.data.func.arg_types[i].size();
    }
    
    for (auto n : body->nodes) {
        n->compile(new_c);
    }
    
    if (fn->type.data.func.return_type->kind == Value_Type_Kind::Void) {
        new_c.emit_opcode(Opcode::Return);
        new_c.emit_size(0);
    }
}

void Typed_AST_Cast::compile(Compiler &c) {
    Address stack_top = c.stack_top;
    
    Opcode cast_op;
    switch (kind) {
        case Typed_AST_Kind::Cast_Bool_Int:
            cast_op = Opcode::Cast_Bool_Int;
            break;
        case Typed_AST_Kind::Cast_Char_Int:
            cast_op = Opcode::Cast_Char_Int;
            break;
        case Typed_AST_Kind::Cast_Int_Float:
            cast_op = Opcode::Cast_Int_Float;
            break;
        case Typed_AST_Kind::Cast_Float_Int:
            cast_op = Opcode::Cast_Float_Int;
            break;
            
        default:
            internal_error("Invalid Cast Kind: %d\n", kind);
            break;
    }

    expr->compile(c);
    c.emit_opcode(cast_op);
    
    c.stack_top = stack_top + type.size();
}
