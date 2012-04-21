#include "semant.h"
#include "util.h"
#include "errormsg.h"
#include "env.h"
#include <assert.h>
#include <stdlib.h>

/* prototypes for functions local to this module */
static struct expty transExp(S_table venv, S_table tenv, A_exp a);
static struct expty transVar(S_table venv, S_table tenv, A_var v);
static void transDec(S_table venv, S_table tenv, A_dec d);
static Ty_ty transTy(S_table tenv, A_ty t);
static struct expty expTy(Tr_exp exp, Ty_ty ty);
static Ty_tyList makeFormalTys(S_table tenv, A_fieldList params);
static Ty_fieldList makeFieldTys(S_table tenv, A_fieldList fields);

/* expty constructor */
static struct expty expTy(Tr_exp exp, Ty_ty ty)
{
	struct expty e;
	e.exp = exp; e.ty = ty;
	return e;
}

static Ty_ty actual_ty(Ty_ty ty)
{
	if (ty->kind == Ty_name)
		return actual_ty(ty->u.name.ty);
	else
		return ty;
}


void SEM_transProg(A_exp exp)
{
	/* Set up the type and value environments */
	E_base_tenv();
	E_base_venv();
	transExp(tenv, venv, exp);
}

static struct expty transExp(S_table venv, S_table tenv, A_exp a)
{
	switch (a->kind) {
		case A_varExp:
		{
			return transVar(venv, tenv, a->u.var);
		}
		case A_nilExp:
		{
			return expTy(NULL, Ty_Nil());
		}
		case A_intExp:
		{
			return expTy(NULL, Ty_Int());
		}
		case A_stringExp:
		{
			return expTy(NULL, Ty_String());
		}
		case A_callExp:
		{
			A_expList args = NULL;
			Ty_tyList formals;
			E_enventry x = S_look(venv, a->u.call.func);
			if (x && x->kind == E_funEntry) {
				// check type of formals
				formals = x->u.fun.formals;
				for (args = a->u.call.args; args && formals;
						args = args->tail, formals = formals->tail) {
					struct expty arg = transExp(venv, tenv, args->head);
					if (arg.ty->kind != formals->head->kind)
						EM_error(args->head->pos, "incorrect type %s; expected %s",
							Ty_ToString(arg.ty->kind), Ty_ToString(formals->head->kind));
				}
				return expTy(NULL, actual_ty(x->u.fun.result));
			} else {
				EM_error(a->pos, "undefined function %s", S_name(a->u.call.func));
				return expTy(NULL, Ty_Int());
			}
		}
		case A_opExp:
		{
			A_oper oper = a->u.op.oper;
			struct expty left = transExp(venv, tenv, a->u.op.left);
			struct expty right = transExp(venv, tenv, a->u.op.right);
			switch (oper) {
				case A_plusOp:
				case A_minusOp:
				case A_timesOp:
				case A_divideOp:
					if (left.ty->kind != Ty_int)
						EM_error(a->u.op.left->pos, "integer required");
					if (right.ty->kind != Ty_int)
						EM_error(a->u.op.right->pos, "integer required");
					return expTy(NULL, Ty_Int());
			}
			if (oper == A_eqOp)
			{

			}
			else if (oper == A_neqOp)
			{

			}
			else if (oper == A_ltOp)
			{

			}
			else if (oper == A_leOp)
			{

			}
			else if (oper == A_gtOp)
			{

			}
			else if (oper == A_geOp)
			{

			}
			return expTy(NULL, Ty_Int());
		}
		case A_recordExp:
		{
			Ty_ty typ = S_look(tenv, a->u.record.typ);
			if (!typ)
				EM_error(a->pos, "undefined type");

			return expTy(NULL, Ty_Record(NULL));
		}
		case A_seqExp:
		{
			struct expty exp;
			A_expList seq;
			for (seq = a->u.seq; seq; seq = seq->tail)
				exp = transExp(venv, tenv, seq->head);
			return exp;
		}
		case A_assignExp:
		{
			struct expty var = transVar(venv, tenv, a->u.assign.var);
			struct expty exp = transExp(venv, tenv, a->u.assign.exp);
			if (exp.ty->kind != var.ty->kind)
				EM_error(a->u.assign.exp->pos, "expression not of expected type");
			return expTy(NULL, Ty_Void());
		}
		case A_ifExp:
		{
			struct expty test, then, elsee;
			test = transExp(venv, tenv, a->u.iff.test);
			if (test.ty->kind != Ty_int)
				EM_error(a->u.iff.test->pos, "integer required");
			then = transExp(venv, tenv, a->u.iff.then);
			if (a->u.iff.elsee)
				elsee = transExp(venv, tenv, a->u.iff.elsee);
			else if (then.ty->kind != Ty_void)
				EM_error(a->u.iff.then->pos, "must produce no value");
			return expTy(NULL, Ty_Void());
		}
		case A_whileExp:
		{
			struct expty test = transExp(venv, tenv, a->u.whilee.test);
			if (test.ty->kind != Ty_int)
				EM_error(a->u.whilee.test->pos, "integer required");
			struct expty body = transExp(venv, tenv, a->u.whilee.body);
			if (body.ty->kind != Ty_void)
				EM_error(a->u.whilee.body->pos, "must produce no value");
			return expTy(NULL, Ty_Void());
		}
		case A_forExp:
		{
			transExp(venv, tenv, a->u.forr.lo);
			transExp(venv, tenv, a->u.forr.hi);
			struct expty body = transExp(venv, tenv, a->u.forr.body);
			if (body.ty->kind != Ty_void)
				EM_error(a->u.forr.body->pos, "must produce no value");
			return expTy(NULL, Ty_Void());
		}
		case A_breakExp:
		{
			return expTy(NULL, Ty_Void());
		}
		case A_letExp:
		{
			struct expty exp;
			A_decList d;
			S_beginScope(venv);
			S_beginScope(tenv);
			for (d = a->u.let.decs; d; d = d->tail)
				transDec(venv, tenv, d->head);
			exp = transExp(venv, tenv, a->u.let.body);
			S_endScope(venv);
			S_endScope(tenv);
			return exp;
		}
		case A_arrayExp:
		{
			Ty_ty typ = S_look(tenv, a->u.array.typ);
			if (!typ)
				EM_error(a->pos, "undefined type");
			struct expty size = transExp(venv, tenv, a->u.array.size);
			struct expty init = transExp(venv, tenv, a->u.array.init);
			if (size.ty->kind != Ty_int)
				EM_error(a->u.array.size->pos, "integer required");
			if (init.ty->kind != Ty_int)
				EM_error(a->u.array.init->pos, "integer required");
			return expTy(NULL, Ty_Array(typ));
		}
	}
	assert(0);
}

static struct expty transVar(S_table venv, S_table tenv, A_var v)
{
	switch(v->kind) {
		case A_simpleVar:
		{
			E_enventry x = S_look(venv, v->u.simple);
			if (x && x->kind == E_varEntry)
				return expTy(NULL, actual_ty(x->u.var.ty));
			else {
				EM_error(v->pos, "undefined variable %s", S_name(v->u.simple));
				return expTy(NULL, Ty_Int());
			}
		}
		case A_fieldVar:
		{
			struct expty e = transVar(venv, tenv, v->u.field.var);
			if (e.ty->kind != Ty_record) EM_error(v->u.field.var->pos, "not a record type");
			/* Cycle through record field type list looking for field we want */
			Ty_fieldList f;
			for (f = e.ty->u.record; f; f = f->tail) {
				if (f->head->name == v->u.field.sym) {
					return expTy(NULL, actual_ty(f->head->ty));
				}
			}
			EM_error(v->pos, "no such field %s for record type", S_name(v->u.field.sym));
			return expTy(NULL, Ty_Int());
		}
		case A_subscriptVar:
		{
			struct expty e = transVar(venv, tenv, v->u.subscript.var);
			if (e.ty->kind != Ty_array) EM_error(v->u.subscript.var->pos, "not an array type");
			struct expty index = transExp(venv, tenv, v->u.subscript.exp);
			if (index.ty->kind != Ty_int)
				EM_error(v->u.subscript.exp->pos, "integer required");
			return expTy(NULL, actual_ty(e.ty->u.array));
		}
	}
	assert(0);
}


/*
 * To maintain the same order as in the params list passed in,
 * we keep a pointer to the last list in the list of parameters;
 * this is where we insert every parameter type (by constructing a
 * type list with the currenct type).
 */
static Ty_tyList makeFormalTys(S_table tenv, A_fieldList params)
{
	Ty_tyList paramTys = NULL;
	Ty_tyList tailList = paramTys;
	A_fieldList paramList;
	for (paramList = params; paramList; paramList = paramList->tail) {
		Ty_ty t = S_look(tenv, paramList->head->name);
		if (!t) {
			EM_error(paramList->head->pos, "undefined type %s",
				S_name(paramList->head->name));
		}
		if (paramTys) {
			tailList->tail = Ty_TyList(t, NULL);
			tailList = tailList->tail;
		} else {
			paramTys = Ty_TyList(t, NULL);
			tailList = paramTys;
		}
	}
	return paramTys;
}


static void transDec(S_table venv, S_table tenv, A_dec d)
{
	switch (d->kind) {
		case A_functionDec:
		{
			A_fundecList funList;
			Ty_tyList formalTys;
			Ty_ty resultTy;
			/* "headers" first -- so we can deal with mutally recursive functions */
			for (funList = d->u.function; funList; funList = funList->tail) {
				if (funList->head->result) resultTy = S_look(tenv, funList->head->result);
				else resultTy = NULL;

				formalTys = makeFormalTys(tenv, funList->head->params);
				S_enter(venv, funList->head->name, E_FunEntry(formalTys, resultTy));
			}
			/* Now process the function bodies */
			E_enventry funEntry = NULL;
			for (funList = d->u.function; funList; funList = funList->tail) {
				/* Get formal types list from funEntry */
				funEntry = S_look(tenv, funList->head->name);
				S_beginScope(venv);
				/* Add formal parameters to value environment
				 * using the type list in the function entry and the params field list in
				 * the function abstract declaration */
				Ty_tyList paramTys = funEntry->u.fun.formals;
				A_fieldList paramFields;
				for (paramFields = funList->head->params; paramFields;
						paramFields = paramFields->tail, paramTys = paramTys->tail) {
					S_enter(venv, paramFields->head->name, E_VarEntry(paramTys->head));
				}
				struct expty e = transExp(venv, tenv, funList->head->body);
				if (funEntry->u.fun.result->kind != e.ty->kind)
					EM_error(funList->head->body->pos, "Incorrect return type %s; expected %s",
						Ty_ToString(e.ty->kind), Ty_ToString(funEntry->u.fun.result->kind));
				S_endScope(venv);
			}
			return;
		}
		case A_varDec:
		{
			struct expty e = transExp(venv, tenv, d->u.var.init);
			if (d->u.var.typ) {
				Ty_ty type = S_look(tenv, d->u.var.typ);
				if (!type)
					EM_error(d->pos, "undefined type %s", S_name(d->u.var.typ));
				if (type->kind != e.ty->kind)
					EM_error(d->pos, "type error: expected %s but expression is %s",
						S_name(d->u.var.typ), Ty_ToString(e.ty->kind));
				S_enter(venv, d->u.var.var, E_VarEntry(e.ty));
			} else {
				S_enter(venv, d->u.var.var, E_VarEntry(e.ty));
			}
			return;
		}
		case A_typeDec:
		{
			A_nametyList nameList;
			bool compositeType = FALSE; /* Array or record encountered in list */
			/* "headers" first */
			for (nameList = d->u.type; nameList; nameList = nameList->tail)
				S_enter(tenv, nameList->head->name, Ty_Name(nameList->head->name, NULL));
			/* now we can process the (possibly mutually) recursive bodies */
			for (nameList = d->u.type; nameList; nameList = nameList->tail) {
				Ty_ty t = transTy(tenv, nameList->head->ty);
				if (!compositeType) {
					if (t->kind == Ty_array || t->kind == Ty_record) compositeType = TRUE;
				}
				S_enter(tenv, nameList->head->name, t);
			}
			if (!compositeType)
				EM_error(d->pos, "Illegal type cycle; cycle must contain record or array type");
			return;
		}
	}
	assert(0);
}

static Ty_fieldList makeFieldTys(S_table tenv, A_fieldList fields)
{
	Ty_fieldList fieldTys = NULL;
	Ty_fieldList tailList = fieldTys;
	A_fieldList fieldList;
	for (fieldList = fields; fieldList; fieldList = fieldList->tail) {
		Ty_ty t = S_look(tenv, fieldList->head->name);
		if (!t) {
			EM_error(fieldList->head->pos, "undefined type %s",
				S_name(fieldList->head->name));
		}
		Ty_field f = Ty_Field(fieldList->head->name, t);
		if (fieldTys) {
			tailList->tail = Ty_FieldList(f, NULL);
			tailList = tailList->tail;
		} else {
			fieldTys = Ty_FieldList(f, NULL);
			tailList = fieldTys;
		}
	}
	return fieldTys;
}


static Ty_ty transTy(S_table tenv, A_ty t)
{
	Ty_ty ty;
	switch (t->kind) {
		case A_nameTy:
		{
			ty = S_look(tenv, t->u.name);
			if (!ty) {
				EM_error(t->pos, "undefined type %s",
					S_name(t->u.name));
			}
			return ty;
		}
		case A_recordTy:
		{
			Ty_fieldList fieldTys = makeFieldTys(tenv, t->u.record);
			return Ty_Record(fieldTys);
		}
		case A_arrayTy:
		{
			ty = S_look(tenv, t->u.name);
			if (!ty) {
				EM_error(t->pos, "undefined type %s",
					S_name(t->u.name));
			}
			return Ty_Array(ty);
		}
	}
	assert(0);
}