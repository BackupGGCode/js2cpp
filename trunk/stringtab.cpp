// stringtab.cpp - implementation of string table

#include "stringtab.h"
#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include <stddef.h>

namespace js2cpp
{

	StringTable::StringTable()
	{
		memset(m_bucket, 0, sizeof m_bucket);
	}

	StringTable::~StringTable()
	{
		int total = 0, maxdepth = 0;
		fprintf(stdout, "StringTable destruction report\n");
		for (int i = 0; i < BUCKETS; i++) {
			STAtom *atom = m_bucket[i];
			int depth = 0;
			while (atom) {
				STAtom *link = atom->m_link;
				free(atom->m_name);
				delete atom;
				atom = link;
				depth++;
			}
			total += depth;
			if (depth > maxdepth) {
				maxdepth = depth;
			}
		} // for i
		fprintf(stdout, "  total entries = %d\n", total);
		fprintf(stdout, "  average hash bucket size: %0.2f entries\n", (double)total / BUCKETS);
		fprintf(stdout, "  deepest hash bucket: %d entries\n", maxdepth);
	} // ~StringTable

	STAtom* AddToList(STAtom *&list, const char *pz, int n)
	{
		STAtom *atom = list;
		while (atom && (memcmp(atom->m_name, pz, n) || atom->m_name[n])) {
			atom = atom->m_link;
		}
		if (!atom) {
			// str is not in the list, add it
			atom = new STAtom;
			if (!atom) {
				return NULL;
			}
			char *str = (char*)malloc(n+1);
			if (!str) {
				delete atom;
				return NULL;
			}
			atom->m_id = 0;
			memcpy(str, pz, n);
			str[n]='\0';
			atom->m_name = str;
			// insert new atom at front of list
			atom->m_link = list;
			list = atom;
		} else {
			// found string in table
		}
		return atom;
	}

	STAtom *StringTable::Intern(const char *str)
	{
		int n = strlen(str);
		unsigned h = Hash(str, n) % BUCKETS;
		return AddToList(m_bucket[h], str, n);
	}

	STAtom *StringTable::Intern(const char *pz, int n)
	{
		unsigned h = Hash(pz, n) % BUCKETS;
		return AddToList(m_bucket[h], pz, n);
	} // Intern

	unsigned StringTable::Hash(const char *pz, int n)
	{
		unsigned h = 0;
		while (n--) {
			h = (h * 33) ^ *pz++;
		}
		return h;
	} // Hash

} // namespace