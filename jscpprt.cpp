
#include "windows.h"
#include <stdio.h>
#include "jscpprt.h"
#include <exception>
#include <limits>
#include <math.h>
#include <time.h>
#include <sys\timeb.h>

value_ global_(new obj_);
value_ true_(true);
value_ false_(false);
value_ undefined;			// the prototypical undefined variable

static value_ NaN(std::numeric_limits<double>::quiet_NaN());

char *pzAppTitle_;

class TypeError : public exception {
public:
	virtual const char* what() const throw() { return "TypeError"; }
};

class incomp_operand : public exception {
public:
	virtual const char *what() const throw() { return "incompatible operand"; }
};

class bad_alloc : public exception {
public:
	virtual const char *what() const throw() { return "memory allocation failed"; }
};

class not_imp : public exception {
public:
	virtual const char *what() const throw() { return "unimplemented feature"; }
};

/////////////////////////////////////////////////////////////////////
// Objects

obj_::~obj_()
{
	prop* p = props;
	while (p) {
		prop* p2 = p->next;
		delete p;
		p = p2;
	}
}

value_ obj_::dot(const char* id)
// search parents, always return a value
{
	prop* p = props;
	while (p && p->id!=id) {
		p = p->next;
	}
	if (p) {
		return p->value;
	}
	// TODO: search parents!
	return undefined;
}

value_ obj_::at(value_ x)
{
	const char* id = x;
	prop* p = props;
	while (p && strcmp(p->id,id)) {
		p = p->next;
	}
	if (p) {
		return p->value;
	}
	// TODO: search up the prototype chain!
	return undefined;
}

value_& obj_::dotref(const char* id)
// search this obj only, add new prop if necessary
// assumes id has been made address-unique!
{
	prop* p = props;
	while (p && p->id!=id) {
		p = p->next;
	}
	if (p) {
		return p->value;
	}
	p = new prop;
	if (!p) {
		throw bad_alloc();
	}
	p->id = id;
	p->value = undefined;
	p->next = props;
	props = p;
	return p->value;
}

value_& obj_::atref(value_ x)
// index into this object
{
	// convert to string rep:
	const char* id = x;
	prop* p = props;
	while (p && strcmp(p->id,id)!=0) {
		p = p->next;
	}
	if (p) {
		return p->value;
	}
	p = new prop;
	if (!p) {
		throw bad_alloc();
	}
	p->id = strdup(id);
	p->value = undefined;
	p->next = props;
	props = p;
	return p->value;
}

/////////////////////////////////////////////////////////////////////
// Functions

value_::value_(func_* pfunc)
{
	t = TFUNC;
	v.f = pfunc;
	pfunc->atref("length") = pfunc->length;
}


/////////////////////////////////////////////////////////////////////
// Arrays

// constructor body
class Array_class_ : public func_ {
public:
	Array_class_() { length = 0; }
	virtual value_ call(value_ this_, int nargs, ...)
	{
		this_ = value_(new array_());
		return this_;
	}
};
// constructor
value_ Array(new Array_class_);

value_ MakeArray_(int len, ...)
{
	array_* a = new array_();
	if (len) {
		value_* arg = (value_*)(&len+1);
		for (int i = 0; i < len; i++) {
			a->atref(i) = arg[i];
		}
	}
	return a;
}

const char* array_::toString(void)
{
	if (flags & VALMAP) {
		// not implemented yet
		throw not_imp();
	}
	value_* vector = (value_*)pdata;
	if (0==len) {
		return "";
	}
	if (1==len) {
		// single element, just render that
		return vector[0].toString();
	}
	// multi-element array, build up by concatenation
	value_ s = vector[0].toString();
	for (int i = 1; i < len; i++) {
		s += ",";
		s += vector[i].toString();
	}
	return s;
}

/////////////////////////////////////////////////////////////////////
// Date

class date
{
public:
	static double now(void);
};

double date::now(void)
{
	struct timeb t;
	ftime(&t);
	return t.time*1000.0+t.millitm;
}

class getTime_class_ : public func_ {
public:
	getTime_class_() { length = 0; }
	virtual value_ call(value_ this_, int nargs, ...)
	{
		return this_.dot("[[value]]");
	}
};

static value_ getTime(new getTime_class_);

class Date_class_ : public func_ {
public:
	Date_class_() { length=0; }
	virtual value_ call(value_ this_, int nargs, ...)
	{
		// TODO: move this into prototype:
		this_.dotref("getTime") = getTime;
		
		if (nargs==0) {
			// "set to the current time (UTC)"
			this_.dotref("[[value]]") = date::now();
		} else {
			// TODO: handle 1,2,...7 args
			throw not_imp();
		}
		return this_;
	}
};
// constructor
value_ Date(new Date_class_);


/////////////////////////////////////////////////////////////////////
// Standard functions

class alert_class_ : public func_ {
public:
	alert_class_() { length=1; }
	virtual value_ call(value_ this_, int nargs, ...)
	{
		value_ msg;
		if (nargs > 0) {
			msg = ((value_*)(&nargs+1))[0];
		}
		MessageBox(NULL, msg.toString(), pzAppTitle_, MB_ICONEXCLAMATION | MB_OK);
		return undefined;
	}
};
value_ alert(new alert_class_);

class Object_class_ : public func_ {
public:
	Object_class_() { length=0; }
	virtual value_ call(value_ this_, int nargs, ...)
	{
		return this_;
	}
};

value_ Object(new Object_class_);

/////////////////////////////////////////////////////////////////////
// value_ methods

value_ value_::dot(const char* id)
{
	if (t==TOBJ) {
		return v.o->dot(id);
	}
	// TODO: support properties of primitive values
	throw incomp_operand();
} // dot

value_& value_::dotref(const char* id)
{
	if (t==TOBJ) {
		return v.o->dotref(id);
	}
	// TODO: support properties of primitive values
	throw incomp_operand();
} // dotref

value_ value_::eltcall(value_ x, int nargs, ...)
{
	value_ base = *this;
	if (base.t < TOBJ) {
		base = base.toObject();
	}
	value_ m = base.at(x);
	if (m.t != TFUNC) {
		throw TypeError();
	}
	func_* func = m.v.f;

#define arg0 ((value_*)(&nargs+1))[0]
#define arg1 ((value_*)(&nargs+1))[1]
#define arg2 ((value_*)(&nargs+1))[2]
#define arg3 ((value_*)(&nargs+1))[3]
#define arg4 ((value_*)(&nargs+1))[4]

	switch (nargs) {
	case 0:
		return func->call(*this, nargs);
	case 1:
		return func->call(*this, nargs, arg0);
	case 2:
		return func->call(*this, nargs, arg0, arg1);
	case 3:
		return func->call(*this, nargs, arg0,arg1,arg2);
	case 4:
		return func->call(*this, nargs, arg0,arg1,arg2,arg3);
	case 5:
		return func->call(*this, nargs, arg0,arg1,arg2,arg3,arg4);
	default:
		throw incomp_operand();
	} // switch
}

value_ value_::dotcall(const char* id, int nargs, ...)
{
	if (t<TOBJ) {
		// not an object
		throw incomp_operand();
	}
	// get the function member
	value_ m = v.o->dot(id);
	if (m.t != TFUNC) {
		throw incomp_operand();
	}
	func_* func = m.v.f;

#define arg0 ((value_*)(&nargs+1))[0]
#define arg1 ((value_*)(&nargs+1))[1]
#define arg2 ((value_*)(&nargs+1))[2]
#define arg3 ((value_*)(&nargs+1))[3]
#define arg4 ((value_*)(&nargs+1))[4]

	switch (nargs) {
	case 0:
		return func->call(*this, nargs);
	case 1:
		return func->call(*this, nargs, arg0);
	case 2:
		return func->call(*this, nargs, arg0, arg1);
	case 3:
		return func->call(*this, nargs, arg0,arg1,arg2);
	case 4:
		return func->call(*this, nargs, arg0,arg1,arg2,arg3);
	case 5:
		return func->call(*this, nargs, arg0,arg1,arg2,arg3,arg4);
	default:
		throw incomp_operand();
	} // switch
}

value_ value_::at(value_ x)
{
	if (t==TOBJ) {
		return v.o->at(x);
	}
	// TODO: support properties of primitive values
	throw incomp_operand();
} // dot

value_& value_::atref(value_ x)
{
	if (t==TOBJ) {
		return v.o->atref(x);
	}
	// TODO: support properties of primitive values
	throw incomp_operand();
} // dotref

value_ value_::toPrimitive(void) const
{
	// TODO: implement this!
	throw incomp_operand();
}

value_ value_::toObject(void) const
{
	// TODO: real implementation of toObject!
	if (t < TOBJ) {
		throw TypeError();
	}
	return *this;
} // toObject


const char *value_::toString(void) const
{
	if (t==TSTR) {
		return v.s;
	}
	if (t==TNUM) {
		if (_finite(v.d)) {
			char buf[32];
			//TODO: implement ECMAScript spec:
			if (v.d==(long)v.d) {
				sprintf(buf, "%ld", (long)v.d);
			} else {
				sprintf(buf, "%g", v.d);
			}
			return strdup(buf);
		}
		if (_isnan(v.d)) {
			return "NaN";
		}
		// leaves an infinity
		return "-Infinity" + (v.d > 0);
	}
	if (t==TBOOL) {
		return v.d ? "true" : "false";
	}
	if (t==TUNDEF) {
		return "undefined";
	}
	if (t==TNULL) {
		return "null";
	}
	if (t==TARRAY) {
		return ((array_*)v.o)->toString();
	}
	return toPrimitive().toString();
} // toString

bool value_::toBool(void) const
{
	if (t==TBOOL || t==TNUM) {
		// TODO: What if it's a NaN? Supposed to return false.
		return v.d != 0;
	}
	if (t==TSTR) {
		// empty string is false, all others are true.
		return v.s[0] != 0;
	}
	if (t>=TOBJ) {
		// object, function, array
		return true;
	}
	// that leaves undefined or null, which are false
	return false;
}

long value_::toInt32(void) const
{
	double r;
	if (t==TNUM || t==TBOOL) {
		r = v.d;
	} else {
		r = toNumber();
	}
	if (!_finite(r)) {
		return 0;
	}
	return _copysign(floor(fabs(r)),r);
}

double value_::toNumber(void) const
{
	if (t==TNUM || t==TBOOL) {
		return v.d;
	}
	if (t==TUNDEF) {
		return NaN;
	}
	if (t==TNULL) {
		return 0.0;
	}
	if (t==TSTR) {
		double f;
		char c;
		if (1==sscanf(v.s, "%g %c", &f, &c)) {
			return f;
		}
		return NaN;
	}
	throw incomp_operand();
}

func_* value_::toFunc(void) const
{
	if (t != TFUNC) {
		throw TypeError();
	}
	return v.f;
}


value_ value_::typeof(void) const
{
	switch (t) {
	case TUNDEF:
		return "undefined";
	case TBOOL:
		return "boolean";
	case TSTR:
		return "string";
	case TNUM:
		return "number";
	case TFUNC:
		return "function";

	default:
		return "object";
	} // switch
}

// inc/dec
value_ postinc_(value_& v)
// increment v but return it's previous value
{
	double d = v.toNumber();
	v = d+1;
	return d;
}

// identity
value_ identical_(value_& a, value_& b)
{
	if (a.t != b.t) {
		return false;
	}
	switch (a.t) {
	case value_::TUNDEF:
	case value_::TNULL:
		return true;
	case value_::TNUM:
	case value_::TBOOL:
		// use IEEE equality.
		// Note that NaN is never equal to anything, and
		// +0=-0 and vice-versa.
		return a.v.d==b.v.d;
	case value_::TSTR:
		// interesting - for strings, use lexical equality
		return 0==strcmp(a.v.s,b.v.s);
	default:
		// object, array, function:
		break;
	} // switch
	// they must point to the same object.
	return a.v.o==b.v.o;
}

// assignment
value_& value_::operator+=(const value_& b)
{
	if (t==TNUM) {
		v.d += (double)b;
	} else if (t==TSTR) {
		const char *sb = b;
		int blen = strlen(sb);
		int len = strlen(v.s);		// our current length
		char *s = (char*)malloc(len+blen+1);
		if (!s) {
			throw bad_alloc();
		}
		memcpy(s, v.s, len);
		memcpy(s+len, sb, blen+1);
		v.s = s;
	} else {
		throw incomp_operand();
	}
	return *this;
}

// arithmetic operators
//
// unary -
value_ value_::operator-(void) const
{
	if (t==value_::TNUM) {
		return value_(-v.d);
	}
	throw incomp_operand();
}

// binary -
value_ value_::operator-(const value_& b) const
{
	if (t==value_::TNUM) {
		return value_(v.d - (double)b);
	}
	throw incomp_operand();
}

// binary +
value_ value_::operator+(const value_& b) const
{
	if (t==value_::TSTR) {
		const char *sb = (const char *)b;
		int alen = strlen(v.s);
		int blen = strlen(sb);
		char *s = (char*)malloc(alen+blen+1);
		memcpy(s, v.s, alen);
		memcpy(s+alen, sb, blen+1);
		return value_(s);
	}
	if (t==value_::TNUM) {
		return value_(v.d+(double)b);
	}
	throw incomp_operand();
}

// binary *
value_ value_::operator*(const value_& b) const
{
	if (t==value_::TNUM) {
		return value_(v.d * (double)b);
	}
	throw incomp_operand();
}

// division (binary /)
value_ value_::operator/(const value_& b) const
{
	if (t==value_::TNUM) {
		return value_(v.d / (double)b);
	}
	throw incomp_operand();
}

// remainder (binary %)
value_ value_::operator%(const value_& b) const
{
	if (t==value_::TNUM) {
		return value_(fmod(v.d, (double)b));
	}
	throw incomp_operand();
}

// equality
bool value_::operator==(const value_&b)
{
	if (this==&b) {
		return true;
	}
	if (t==value_::TNUM && b.t==value_::TNUM) {
		return v.d==b.v.d;
	}
	if (t==value_::TSTR && b.t==value_::TSTR) {
		return 0==strcmp(v.s,b.v.s);
	}
	// chicken out for now. TODO: operand alignment.
	throw incomp_operand();
}

// relation <
bool value_::operator<(const value_& b) const
{
	if (t==value_::TSTR) {
		return strcmp(v.s, b) < 0;
	}
	if (t==value_::TNUM) {
		return v.d < (double)b;
	}
	throw incomp_operand();
}