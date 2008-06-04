// jsrt.h - Javascript runtime

namespace jsrt
{

	class jsString;
	class jsObject;
	class jsArray;

	typedef enum {
		dtUndef,
		dtNull,
		dtNumber,
		dtBool,
		dtString,
		dtObject,
		dtArray
	} ValDataType;

	class jsValue
	{
		short	type;
		union {
			double		 dvalue;
			jsString	*pstring;
			jsObject	*pobj;
			jsArray		*parray;
		}

	public:
		ValDataType Type(void) const { return type; }
		bool isUndef(void) const { return (type==dtUndef); }
		bool isNull(void) const { return (type==dtNull); }
		bool isNumber(void) const { return (type==dtNumber); }
		bool isBool(void) const { return (type==dtBool); }
		bool isString(void) const { return (type==dtString); }
		bool isObject(void) const { return (type==dtObject); }
		bool isArray(void) const { return (type==dtArray); }

		bool getNumber(double &d) const { return (type==dtNumber) ? (d=dvalue,true) : false; }

	}; // jsValue

}
