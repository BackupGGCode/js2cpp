// stringtab.h - string table
#ifndef STRINGTAB_H
#define STRINGTAB_H

namespace js2cpp
{

#define BUCKETS 4096
	struct STAtom;

	class StringTable
	{
	public:
		StringTable();
		virtual ~StringTable();

		STAtom* Intern(const char *str);
		// Intern's a nul-terminated string

		STAtom* Intern(const char *pz, int n);
		// Intern's the n-character string at pz.

	private:
		STAtom		*m_bucket[BUCKETS];

		unsigned Hash(const char *pz, int n);
	};

	struct STAtom
	{
		char		*m_name;
		STAtom		*m_link;
		int			 m_id;
	}; // STAtom

} // namespace

#endif