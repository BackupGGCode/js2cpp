// jscpprt.h - js to cpp runtime

class obj_;
class func_;
class array_;

class value_
{
public:
	typedef enum {
		TUNDEF,
		TNULL,
		TBOOL,			// boolean values, True and False
		TSTR,
		TNUM,
		TOBJ,
		TFUNC,			// function (special kind of object)
		TARRAY,			// array (all kinds)
	} VALTYPE;

	value_(void) : t(TUNDEF) {}
	value_(bool x) : t(TBOOL) { v.d=(double)x; }
	value_(int i) : t(TNUM) { v.d = i; }
	value_(long l) : t(TNUM) { v.d = l; }
	value_(double n) : t(TNUM) { v.d = n; }
	value_(const char *s) : t(TSTR) { v.s = s; }
	value_(obj_* pobj) : t(TOBJ) { v.o = pobj; }
	value_(func_* pfunc);

	inline bool isUndefined(void) const  { return t==TUNDEF; }
	inline operator bool() const		 { return toBool(); }
	inline operator const char *() const { return toString(); }
	inline operator long() const		 { return toInt32(); }
	inline operator double() const		 { return toNumber(); }

	func_* toFunc(void) const;

	value_ typeof(void) const;
	bool isFunction(void) const { return t==TFUNC; }

	value_ operator-(const value_& b) const;
	value_ operator-(void) const;
	value_ operator+(const value_& b) const;
	value_ operator*(const value_& b) const;
	value_ operator/(const value_& b) const;
	value_ operator%(const value_& b) const;
	bool operator<(const value_& b) const;
	bool operator>(const value_& b) const;
	bool operator!=(const value_&b) { return !operator==(b); }
	bool operator==(const value_&b);
	value_ &operator[](const value_&i);

	value_ dot(const char* id);
	value_& dotref(const char* id);
	value_ at(value_ x);
	value_& atref(value_ x);

	value_ dotcall(const char* id, int nargs, ...);
	value_ eltcall(value_ x, int nargs, ...);

	value_& operator+=(const value_& b);

	short	t;				// type of value_
	union {
		const char *s;		// pointer to malloc'd string (TSTR)
		double	d;			// 64-bit IEEE float (TNUM)
		obj_*	o;			// object pointer (TOBJ)
		func_*	f;			// function pointer (TFUNC)
	} v;

	bool toBool(void) const;
	long toInt32(void) const;
	const char* toString(void) const;
	double toNumber(void) const;
	value_ toPrimitive(void) const;
	value_ toObject(void) const;

};


class prop
{
public:
	prop*		next;		// linked-list
	const char*	id;			// name of this property
	value_		value;		// current value
};

class obj_
{

public:
	obj_() : klass("Object"), props(0) {}
	~obj_();

	value_ dot(const char* id);		// find a constant property (o.id) - always return a value
	value_& dotref(const char* id);	// search this obj only, add new prop if necessary
	value_ at(value_ x);			// find run-time dynamic property o[x]
	value_& atref(value_ x);

private:
	const char*		klass;
	prop*			props;
};


class func_ : public obj_
{
public:
	func_() {}
	virtual value_ call(value_ this_, int nargs_, ...) = 0;
	int			length;
};


class array_ : public obj_		// an array is a kind of object
{
public:
	typedef enum {
		VALMAP = 1,				// pdata points to a map<value_,value_>
		NONINT = 2,				// some indices are non-integers
	} FLAGS;

	array_() : flags(0), lo(0), len(0), pdata(0) {}

	value_ get(value_ x);		// get the value at index x
	value_& ref(value_ x);		// reference the value at index x

	const char* toString(void);

	unsigned		flags;
	int				lo;			// lowest integer index
	int				len;		// length (if compact vector)
	void*			pdata;		// pointer to a 'map' or to a value_ vector
};

// private RT functions used by compiler:
value_ preinc_(value_& v);
value_ predec_(value_& v);
value_ postinc_(value_& v);
value_ postdec_(value_& v);
value_ new_(void);			// create and return a new empty object
value_ identical_(value_& a, value_& b);

value_ MakeArray_(int len, ...);

// standard functions and objects

extern value_ true_;
extern value_ false_;
extern value_ global_;
extern value_ null_;
extern value_ undefined;

